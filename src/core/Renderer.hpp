#pragma once
#include "Types.hpp"
#include "GpuDevice.hpp"
#include <deko3d.hpp>
#include <cstdint>
#include <vector>
#include <string>

namespace ui {

class Texture;
class Font;

struct Vertex2D {
    float x, y;         // Position
    float u, v;         // Texture coordinate
    float r, g, b, a;   // Color (premultiplied alpha)
};
static_assert(sizeof(Vertex2D) == 32);

struct VsUniforms {
    float projection[16];   // Ortho matrix (top-left origin)
};

struct FsUniforms {
    int32_t useTexture;     // 0 = vertex-color only, 1 = sample texture
    int32_t pad[3];
};

// Renderer
class Renderer {
public:
    explicit Renderer(GpuDevice& gpu);
    ~Renderer();

    // Must be called once after GpuDevice::initialize()
    bool initialize();

    // Frame scope
    void beginFrame();
    void endFrame();

    // ─── 2D Draw API ────────────────────────────────────────
    void drawRect(const Rect& r, const Color& c);
    void drawRectOutline(const Rect& r, const Color& c, float thickness = 1.f);
    void drawRoundedRect(const Rect& r, const Color& c, float radius);
    void drawRoundedRectOutline(const Rect& r, const Color& c, float radius, float thickness = 1.f);
    void drawCircle(const Vec2& center, float radius, const Color& c, int segments = 32);
    void drawTriangle(const Vec2& p1, const Vec2& p2, const Vec2& p3, const Color& c);
    void drawLine(const Vec2& from, const Vec2& to, const Color& c, float thickness = 1.f);
    void drawGradientRect(const Rect& r, const Color& top, const Color& bottom);
    void drawTexture(const Texture* tex, const Rect& dest, const Color& tint = Color::white());
    void drawTextureSub(const Texture* tex, const Rect& src, const Rect& dest, const Color& tint = Color::white());
    void drawTextureRounded(const Texture* tex, const Rect& dest, float radius, const Color& tint = Color::white());
    void drawText(const std::string& text, const Vec2& pos, Font* font, const Color& color, float scale = 1.f);

    // Scissor/clip stack
    void pushClipRect(const Rect& r);
    void popClipRect();

    // Screen dimensions
    int width()  const { return m_gpu.width(); }
    int height() const { return m_gpu.height(); }

    // Flush current batch (internal, but exposed for texture manager)
    void flush();

    // Debug: total vertices emitted this frame
    uint32_t vertexCount() const { return m_vtxCount; }

    // Texture descriptor management
    int registerTexture(const dk::ImageView& view);
    void bindTexture(int slot);

    GpuDevice& gpu() { return m_gpu; }

private:
    // Emit geometry helpers
    void addVertex(float x, float y, float u, float v, const Color& c);
    void addQuad(float x0, float y0, float x1, float y1,
                 float u0, float v0, float u1, float v1, const Color& c);
    void addQuadGrad(float x0, float y0, float x1, float y1,
                     float u0, float v0, float u1, float v1,
                     const Color& cTop, const Color& cBot);

    void loadShaders();
    void setupSampler();
    void updateProjection();

    GpuDevice& m_gpu;

    // Shaders
    dk::Shader m_vertShader;
    dk::Shader m_fragShader;

    // Batching state
    Vertex2D*  m_vtxBase   = nullptr;    // CPU pointer to current frame's VTX buffer
    uint32_t   m_vtxCount  = 0;          // Total vertices written this frame
    uint32_t   m_vtxBatchStart = 0;      // Where the current un-flushed batch starts
    int        m_curTexSlot = -1;         // Currently bound texture descriptor slot (-1 = none)
    bool       m_texturing  = false;      // Current batch mode

    // Clip stack
    std::vector<Rect> m_clipStack;

    // Texture descriptor tracking
    int m_nextDescSlot = 0;
    static constexpr int WHITE_TEX_SLOT = 0;

    // 1×1 white pixel texture for solid-color rendering
    dk::Image          m_whiteImage;
    dk::UniqueMemBlock m_whiteMemBlock;
};

} // namespace ui
