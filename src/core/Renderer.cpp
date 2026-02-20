#include "Renderer.hpp"
#include "Texture.hpp"
#include "Font.hpp"
#include <cstring>
#include <cstdio>
#include <cmath>

namespace ui {

static void ortho(float* m, float w, float h) {
    std::memset(m, 0, 16 * sizeof(float));
    m[ 0] =  2.f / w;
    m[ 5] = -2.f / h;     // Y flipped
    m[10] = -1.f;
    m[12] = -1.f;
    m[13] =  1.f;
    m[15] =  1.f;
}

Renderer::Renderer(GpuDevice& gpu) : m_gpu(gpu) {}
Renderer::~Renderer() {}

bool Renderer::initialize() {
    std::printf("[Renderer] Loading shaders...\n");
    loadShaders();
    std::printf("[Renderer] Setting up sampler...\n");
    setupSampler();

    // Create 1×1 white pixel texture
    std::printf("[Renderer] Creating 1x1 white texture...\n");
    {
        dk::ImageLayout layout;
        dk::ImageLayoutMaker{m_gpu.device()}
            .setFlags(0)
            .setFormat(DkImageFormat_RGBA8_Unorm)
            .setDimensions(1, 1)
            .initialize(layout);

        m_whiteMemBlock = m_gpu.allocImageMemory(layout.getSize());
        m_whiteImage.initialize(layout, m_whiteMemBlock, 0);

        uint32_t white = 0xFFFFFFFF;
        m_gpu.uploadTexture(m_whiteImage, &white, 4, 1, 1);

        // Register as descriptor slot 0
        dk::ImageView view{m_whiteImage};
        int slot = registerTexture(view);
        std::printf("[Renderer] White texture registered at slot %d\n", slot);
    }

    std::printf("[Renderer] Init complete\n");
    return true;
}

void Renderer::loadShaders() {
    auto loadDksh = [&](dk::Shader& out, const char* path) {
        std::printf("[Renderer] Loading shader: %s\n", path);
        FILE* f = std::fopen(path, "rb");
        if (!f) {
            std::printf("[Renderer] FAILED to open shader: %s\n", path);
            return;
        }
        std::fseek(f, 0, SEEK_END);
        long sz = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        std::printf("[Renderer] Shader %s: %ld bytes\n", path, sz);

        uint32_t off = m_gpu.codePool().alloc(sz, DK_SHADER_CODE_ALIGNMENT);
        if (off == UINT32_MAX) {
            std::printf("[Renderer] Code pool alloc FAILED for %ld bytes\n", sz);
            std::fclose(f);
            return;
        }
        void* dst = m_gpu.codePool().cpuAddr(off);
        std::fread(dst, 1, sz, f);
        std::fclose(f);

        std::printf("[Renderer] ShaderMaker offset=%u\n", off);
        dk::ShaderMaker{m_gpu.codePool().block, off}.initialize(out);
        std::printf("[Renderer] Shader initialized OK\n");
    };

    loadDksh(m_vertShader, "romfs:/shaders/basic_vsh.dksh");
    loadDksh(m_fragShader, "romfs:/shaders/basic_fsh.dksh");
}

void Renderer::setupSampler() {
    dk::SamplerDescriptor samDesc;
    samDesc.initialize(
        dk::Sampler{}
            .setFilter(DkFilter_Linear, DkFilter_Linear)
            .setWrapMode(DkWrapMode_ClampToEdge, DkWrapMode_ClampToEdge, DkWrapMode_ClampToEdge)
    );
    // Write to slot 0 in sampler descriptor set
    std::memcpy(m_gpu.samDescCpuAddr(), &samDesc, sizeof(samDesc));
}

int Renderer::registerTexture(const dk::ImageView& view) {
    int slot = m_nextDescSlot++;
    dk::ImageDescriptor desc;
    desc.initialize(view);
    auto* base = static_cast<dk::ImageDescriptor*>(m_gpu.imgDescCpuAddr());
    base[slot] = desc;
    return slot;
}

void Renderer::bindTexture(int slot) {
    if (slot != m_curTexSlot) {
        bool newTexturing = (slot >= 0);
        if (newTexturing != m_texturing || slot != m_curTexSlot)
            flush();
        m_curTexSlot = slot;
        m_texturing = newTexturing;
    }
}

void Renderer::beginFrame() {
    int slot = m_gpu.slot();
    m_vtxBase  = static_cast<Vertex2D*>(m_gpu.vtxCpuAddr(slot));
    m_vtxCount = 0;
    m_vtxBatchStart = 0;
    m_curTexSlot = -1;
    m_texturing  = false;
    m_clipStack.clear();

    auto cmd = m_gpu.cmdBuf();

    // Render targets: color + stencil
    dk::ImageView colorTarget{m_gpu.fbImage(slot)};
    dk::ImageView dsTarget{m_gpu.dsImage()};
    cmd.bindRenderTargets(&colorTarget, &dsTarget);

    // Viewport & scissor
    cmd.setViewports(0, DkViewport{0.f, 0.f,
        (float)m_gpu.width(), (float)m_gpu.height(), 0.f, 1.f});
    cmd.setScissors(0, DkScissor{0, 0, (uint32_t)m_gpu.width(), (uint32_t)m_gpu.height()});

    // Clear color to dark Wii U blue
    cmd.clearColor(0, DkColorMask_RGBA, 0.05f, 0.08f, 0.15f, 1.f);
    // Stencil-only (S8) — clear stencil only, not depth
    cmd.clearDepthStencil(false, 0.f, 0xFF, 0);

    // Shaders
    cmd.bindShaders(DkStageFlag_GraphicsMask, {&m_vertShader, &m_fragShader});

    // Blend: pre-multiplied alpha (src-over)
    cmd.bindColorState(dk::ColorState{}
        .setBlendEnable(0, true));
    dk::BlendState blendState;
    blendState.setFactors(DkBlendFactor_SrcAlpha, DkBlendFactor_InvSrcAlpha,
                          DkBlendFactor_One, DkBlendFactor_InvSrcAlpha);
    DkBlendState rawBlendState = blendState;
    cmd.bindBlendStates(0, rawBlendState);

    // Default: no stencil test
    cmd.bindDepthStencilState(dk::DepthStencilState{}.setDepthTestEnable(false));

    // Rasterizer: no culling
    cmd.bindRasterizerState(dk::RasterizerState{}.setCullMode(DkFace_None));

    // Vertex attribs
    static const std::array<DkVtxAttribState, 3> attribs = {{
        DkVtxAttribState{0, 0, offsetof(Vertex2D, x), DkVtxAttribSize_2x32, DkVtxAttribType_Float, 0},
        DkVtxAttribState{0, 0, offsetof(Vertex2D, u), DkVtxAttribSize_2x32, DkVtxAttribType_Float, 0},
        DkVtxAttribState{0, 0, offsetof(Vertex2D, r), DkVtxAttribSize_4x32, DkVtxAttribType_Float, 0},
    }};
    static const DkVtxBufferState bufState = {sizeof(Vertex2D), 0};
    cmd.bindVtxAttribState(attribs);
    cmd.bindVtxBufferState(bufState);

    // Descriptors
    cmd.bindImageDescriptorSet(m_gpu.imgDescGpuAddr(), GpuDevice::MAX_TEXTURES);
    cmd.bindSamplerDescriptorSet(m_gpu.samDescGpuAddr(), GpuDevice::MAX_SAMPLERS);

    // Projection UBO
    updateProjection();
}

void Renderer::endFrame() {
    flush();
}

void Renderer::updateProjection() {
    int slot = m_gpu.slot();
    auto* ubo = static_cast<uint8_t*>(m_gpu.vsUboCpuAddr(slot));
    VsUniforms vs;
    ortho(vs.projection, (float)m_gpu.width(), (float)m_gpu.height());
    std::memcpy(ubo, &vs, sizeof(vs));

    auto cmd = m_gpu.cmdBuf();
    cmd.bindUniformBuffer(DkStage_Vertex, 0, m_gpu.vsUboGpuAddr(slot), GpuDevice::VS_UBO_SIZE);
}


void Renderer::flush() {
    uint32_t batchVerts = m_vtxCount - m_vtxBatchStart;
    if (batchVerts == 0) return;

    auto cmd = m_gpu.cmdBuf();
    int slot = m_gpu.slot();

    // Fragment uniform: textured?
    FsUniforms fs;
    fs.useTexture = m_texturing ? 1 : 0;
    fs.pad[0] = fs.pad[1] = fs.pad[2] = 0;
    // Push FS UBO data (256-byte aligned, separate from VS)
    auto fsUboAddr = m_gpu.fsUboGpuAddr(slot);
    cmd.pushConstants(fsUboAddr, GpuDevice::FS_UBO_SIZE,
                      0, sizeof(fs), &fs);
    // Binding index 1 matches shader: layout(std140, binding = 1) uniform FsUniforms
    cmd.bindUniformBuffer(DkStage_Fragment, 1, fsUboAddr, GpuDevice::FS_UBO_SIZE);

    // Texture binding via texture handle (imageSlot, samplerSlot=0)
    int texSlot = (m_texturing && m_curTexSlot >= 0) ? m_curTexSlot : WHITE_TEX_SLOT;
    cmd.bindTextures(DkStage_Fragment, 0, dkMakeTextureHandle(texSlot, 0));

    // Bind vertex buffer — offset to this batch's start
    DkGpuAddr vtxAddr = m_gpu.vtxGpuAddr(slot) + m_vtxBatchStart * sizeof(Vertex2D);
    cmd.bindVtxBuffer(0, vtxAddr, batchVerts * sizeof(Vertex2D));

    // Draw
    cmd.draw(DkPrimitive_Triangles, batchVerts, 1, 0, 0);

    // Advance batch start (don't reset m_vtxCount — preserve previous batches' data)
    m_vtxBatchStart = m_vtxCount;
}

void Renderer::addVertex(float x, float y, float u, float v, const Color& c) {
    if (m_vtxCount >= GpuDevice::MAX_VERTICES) {
        std::printf("[Renderer] WARN: vertex buffer full (%u), dropping vertex\n", m_vtxCount);
        return;
    }
    auto& vtx  = m_vtxBase[m_vtxCount++];
    vtx.x = x; vtx.y = y;
    vtx.u = u; vtx.v = v;
    vtx.r = c.r; vtx.g = c.g; vtx.b = c.b; vtx.a = c.a;
}

void Renderer::addQuad(float x0, float y0, float x1, float y1,
                        float u0, float v0, float u1, float v1,
                        const Color& c)
{
    addVertex(x0, y0, u0, v0, c);
    addVertex(x1, y0, u1, v0, c);
    addVertex(x1, y1, u1, v1, c);
    addVertex(x0, y0, u0, v0, c);
    addVertex(x1, y1, u1, v1, c);
    addVertex(x0, y1, u0, v1, c);
}

void Renderer::addQuadGrad(float x0, float y0, float x1, float y1,
                            float u0, float v0, float u1, float v1,
                            const Color& cTop, const Color& cBot)
{
    addVertex(x0, y0, u0, v0, cTop);
    addVertex(x1, y0, u1, v0, cTop);
    addVertex(x1, y1, u1, v1, cBot);
    addVertex(x0, y0, u0, v0, cTop);
    addVertex(x1, y1, u1, v1, cBot);
    addVertex(x0, y1, u0, v1, cBot);
}

void Renderer::drawRect(const Rect& r, const Color& c) {
    bindTexture(-1);
    addQuad(r.x, r.y, r.right(), r.bottom(), 0, 0, 1, 1, c);
}

void Renderer::drawRectOutline(const Rect& r, const Color& c, float t) {
    drawRect({r.x, r.y, r.width, t}, c);                // top
    drawRect({r.x, r.bottom() - t, r.width, t}, c);     // bottom
    drawRect({r.x, r.y + t, t, r.height - 2*t}, c);     // left
    drawRect({r.right()-t, r.y + t, t, r.height - 2*t}, c); // right
}

void Renderer::drawGradientRect(const Rect& r, const Color& top, const Color& bottom) {
    bindTexture(-1);
    addQuadGrad(r.x, r.y, r.right(), r.bottom(), 0, 0, 1, 1, top, bottom);
}

void Renderer::drawRoundedRect(const Rect& r, const Color& c, float radius) {
    if (radius <= 0.f) { drawRect(r, c); return; }
    float rad = std::min(radius, std::min(r.width, r.height) * 0.5f);

    bindTexture(-1);

    auto cx = r.x + r.width * 0.5f;
    auto cy = r.y + r.height * 0.5f;

    // Fan from center
    constexpr int segs = 8; // per corner
    const float pi2 = 3.14159265f * 0.5f;

    struct {float cx, cy; float a0;} corners[4] = {
        {r.right() - rad, r.y + rad,          0},
        {r.x + rad,       r.y + rad,          pi2},
        {r.x + rad,       r.bottom() - rad,   pi2*2},
        {r.right() - rad, r.bottom() - rad,   pi2*3},
    };

    // Build perimeter points
    std::vector<Vec2> pts;
    for (auto& cn : corners) {
        for (int i = 0; i <= segs; ++i) {
            float a = cn.a0 + pi2 * i / segs;
            pts.push_back({cn.cx + std::cos(a) * rad, cn.cy - std::sin(a) * rad});
        }
    }

    // Triangle fan from center
    for (size_t i = 0; i < pts.size(); ++i) {
        auto& p0 = pts[i];
        auto& p1 = pts[(i + 1) % pts.size()];
        addVertex(cx, cy, 0, 0, c);
        addVertex(p0.x, p0.y, 0, 0, c);
        addVertex(p1.x, p1.y, 0, 0, c);
    }
}

void Renderer::drawRoundedRectOutline(const Rect& r, const Color& c, float radius, float t) {
    if (radius <= 0.f) { drawRectOutline(r, c, t); return; }
    float rad = std::min(radius, std::min(r.width, r.height) * 0.5f);

    bindTexture(-1);

    constexpr int segs = 8;
    const float pi2 = 3.14159265f * 0.5f;

    struct {float cx, cy; float a0;} corners[4] = {
        {r.right() - rad, r.y + rad,          0},
        {r.x + rad,       r.y + rad,          pi2},
        {r.x + rad,       r.bottom() - rad,   pi2*2},
        {r.right() - rad, r.bottom() - rad,   pi2*3},
    };

    std::vector<Vec2> outer, inner;
    for (auto& cn : corners) {
        for (int i = 0; i <= segs; ++i) {
            float a = cn.a0 + pi2 * i / segs;
            float ca = std::cos(a), sa = std::sin(a);
            outer.push_back({cn.cx + ca * rad,       cn.cy - sa * rad});
            inner.push_back({cn.cx + ca * (rad - t), cn.cy - sa * (rad - t)});
        }
    }

    // Triangle strip between outer and inner
    for (size_t i = 0; i < outer.size(); ++i) {
        size_t j = (i + 1) % outer.size();
        addVertex(outer[i].x, outer[i].y, 0, 0, c);
        addVertex(inner[i].x, inner[i].y, 0, 0, c);
        addVertex(outer[j].x, outer[j].y, 0, 0, c);

        addVertex(inner[i].x, inner[i].y, 0, 0, c);
        addVertex(inner[j].x, inner[j].y, 0, 0, c);
        addVertex(outer[j].x, outer[j].y, 0, 0, c);
    }
}

void Renderer::drawCircle(const Vec2& center, float radius, const Color& c, int segments) {
    bindTexture(-1);
    const float pi2 = 3.14159265f * 2.f;
    for (int i = 0; i < segments; ++i) {
        float a0 = pi2 * i / segments;
        float a1 = pi2 * (i + 1) / segments;
        addVertex(center.x, center.y, 0, 0, c);
        addVertex(center.x + std::cos(a0) * radius, center.y + std::sin(a0) * radius, 0, 0, c);
        addVertex(center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius, 0, 0, c);
    }
}

void Renderer::drawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Color& c) {
    bindTexture(-1);
    addVertex(p1.x, p1.y, 0, 0, c);
    addVertex(p2.x, p2.y, 0, 0, c);
    addVertex(p3.x, p3.y, 0, 0, c);
}

void Renderer::drawLine(const Vec2& from, const Vec2& to, const Color& c, float thickness) {
    Vec2 d = (to - from).normalized();
    Vec2 n = {-d.y, d.x};
    float ht = thickness * 0.5f;
    Vec2 a = from + n * ht, b = from - n * ht;
    Vec2 cc = to + n * ht,  dd = to - n * ht;
    bindTexture(-1);
    addVertex(a.x, a.y, 0, 0, c);
    addVertex(b.x, b.y, 0, 0, c);
    addVertex(cc.x, cc.y, 0, 0, c);
    addVertex(b.x, b.y, 0, 0, c);
    addVertex(dd.x, dd.y, 0, 0, c);
    addVertex(cc.x, cc.y, 0, 0, c);
}

void Renderer::drawTexture(const Texture* tex, const Rect& dest, const Color& tint) {
    if (!tex) return;
    bindTexture(tex->descriptorSlot());
    addQuad(dest.x, dest.y, dest.right(), dest.bottom(), 0, 0, 1, 1, tint);
}

void Renderer::drawTextureSub(const Texture* tex, const Rect& src, const Rect& dest, const Color& tint) {
    if (!tex) return;
    float tw = (float)tex->width(), th = (float)tex->height();
    float u0 = src.x / tw, v0 = src.y / th;
    float u1 = src.right() / tw, v1 = src.bottom() / th;
    bindTexture(tex->descriptorSlot());
    addQuad(dest.x, dest.y, dest.right(), dest.bottom(), u0, v0, u1, v1, tint);
}

void Renderer::drawTextureRounded(const Texture* tex, const Rect& dest, float radius, const Color& tint) {
    if (!tex) { return; }
    if (radius <= 0) { drawTexture(tex, dest, tint); return; }
    float rad = std::min(radius, std::min(dest.width, dest.height) * 0.5f);

    bindTexture(tex->descriptorSlot());

    // Center of the rect (fan pivot)
    float fcx = dest.x + dest.width * 0.5f;
    float fcy = dest.y + dest.height * 0.5f;

    // Helper: convert world pos to UV in [0,1]
    auto toUV = [&](float px, float py) -> Vec2 {
        return {(px - dest.x) / dest.width,
                (py - dest.y) / dest.height};
    };

    constexpr int segs = 8;
    const float pi2 = 3.14159265f * 0.5f;

    struct { float cx, cy; float a0; } corners[4] = {
        {dest.right() - rad, dest.y + rad,          0},
        {dest.x + rad,       dest.y + rad,          pi2},
        {dest.x + rad,       dest.bottom() - rad,   pi2 * 2},
        {dest.right() - rad, dest.bottom() - rad,   pi2 * 3},
    };

    // Build perimeter
    std::vector<Vec2> pts;
    for (auto& cn : corners) {
        for (int i = 0; i <= segs; ++i) {
            float a = cn.a0 + pi2 * i / segs;
            pts.push_back({cn.cx + std::cos(a) * rad,
                           cn.cy - std::sin(a) * rad});
        }
    }

    // Fan from center with UV coords
    Vec2 cuv = toUV(fcx, fcy);
    for (size_t i = 0; i < pts.size(); ++i) {
        auto& p0 = pts[i];
        auto& p1 = pts[(i + 1) % pts.size()];
        Vec2 uv0 = toUV(p0.x, p0.y);
        Vec2 uv1 = toUV(p1.x, p1.y);
        addVertex(fcx,  fcy,  cuv.x, cuv.y, tint);
        addVertex(p0.x, p0.y, uv0.x, uv0.y, tint);
        addVertex(p1.x, p1.y, uv1.x, uv1.y, tint);
    }
}

void Renderer::drawText(const std::string& text, const Vec2& pos, Font* font,
                         const Color& color, float scale) {
    if (!font || text.empty()) return;
    font->draw(*this, text, pos, color, scale);
}

void Renderer::pushClipRect(const Rect& r) {
    flush();
    Rect clip = r;
    if (!m_clipStack.empty()) {
        auto& prev = m_clipStack.back();
        float x0 = std::max(clip.x, prev.x);
        float y0 = std::max(clip.y, prev.y);
        float x1 = std::min(clip.right(), prev.right());
        float y1 = std::min(clip.bottom(), prev.bottom());
        clip = {x0, y0, std::max(0.f, x1 - x0), std::max(0.f, y1 - y0)};
    }
    m_clipStack.push_back(clip);
    m_gpu.cmdBuf().setScissors(0, DkScissor{
        (uint32_t)clip.x, (uint32_t)clip.y,
        (uint32_t)clip.width, (uint32_t)clip.height});
}

void Renderer::popClipRect() {
    flush();
    if (!m_clipStack.empty()) m_clipStack.pop_back();
    if (m_clipStack.empty()) {
        m_gpu.cmdBuf().setScissors(0, DkScissor{
            0, 0, (uint32_t)m_gpu.width(), (uint32_t)m_gpu.height()});
    } else {
        auto& r = m_clipStack.back();
        m_gpu.cmdBuf().setScissors(0, DkScissor{
            (uint32_t)r.x, (uint32_t)r.y, (uint32_t)r.width, (uint32_t)r.height});
    }
}

} // namespace ui
