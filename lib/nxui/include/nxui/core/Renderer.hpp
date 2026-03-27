#pragma once
#include <nxui/core/Types.hpp>
#include <nxui/core/GpuDevice.hpp>
#ifdef NXUI_BACKEND_DEKO3D
#include <deko3d.hpp>
#endif
#include <cstdint>
#include <vector>
#include <string>

namespace nxui {

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
    int32_t useTexture;
    float   param1;
    float   param2;
    float   param3;
    float   extra[4 * 4 * 3];
};

// Shader program IDs
enum class ShaderProgram {
    Basic,
    BlurH,
    BlurV,
    Wave,
    Gradient,
    Count
};

// Renderer
class Renderer {
public:
    explicit Renderer(GpuDevice& gpu);
    ~Renderer();

    /// Set the base directory for compiled shader (.dksh) files.
    /// Must be called BEFORE initialize(). Default: "romfs:/shaders/"
    static void setShaderBasePath(const std::string& path) { s_shaderBasePath = path; }
    static const std::string& shaderBasePath() { return s_shaderBasePath; }

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

    // ─── Post-processing API ────────────────────────────────
    void captureToOffscreen();
    void drawOffscreen(int target, const Rect& dest, const Color& tint = Color::white());
    void applyBlur(float radius = 1.0f, int passes = 2);
    void applyWave(float time, float amplitude, float frequency);

    // Switch shader program (flushes current batch)
    void useShader(ShaderProgram prog);

    // Push custom FS uniforms (flushes current batch)
    void pushFsUniforms(const FsUniforms& fs);

    // Scissor/clip stack
    void pushClipRect(const Rect& r);
    void popClipRect();

    // Screen dimensions
    int width()  const { return m_gpu.width(); }
    int height() const { return m_gpu.height(); }

    // Flush current batch
    void flush();

    // Debug
    uint32_t vertexCount() const { return m_vtxCount; }
    void setBoxWireframeEnabled(bool enabled) { m_boxWireframeEnabled = enabled; }
    bool boxWireframeEnabled() const { return m_boxWireframeEnabled; }

    // Texture descriptor management
#ifdef NXUI_BACKEND_DEKO3D
    int registerTexture(const dk::ImageView& view);
    void updateTexture(int slot, const dk::ImageView& view);
#endif
    void bindTexture(int slot);
    void resetTextureSlots();

    GpuDevice& gpu() { return m_gpu; }

    // Offscreen descriptor slots
    int offscreenDescSlot(int target) const { return m_offDescSlot[target]; }

private:
    // Emit geometry helpers
    void addVertex(float x, float y, float u, float v, const Color& c);
    void addQuad(float x0, float y0, float x1, float y1,
                 float u0, float v0, float u1, float v1, const Color& c);
    void addQuadGrad(float x0, float y0, float x1, float y1,
                     float u0, float v0, float u1, float v1,
                     const Color& cTop, const Color& cBot);

#ifdef NXUI_BACKEND_DEKO3D
    void loadShaders();
    void setupSampler();
    void updateProjection();
    void bindRenderTarget(int offscreenIdx);
    void restoreRenderTarget();
#endif

    GpuDevice& m_gpu;

#ifdef NXUI_BACKEND_DEKO3D
    static constexpr int SHADER_COUNT = (int)ShaderProgram::Count;
    dk::Shader m_vertShaders[SHADER_COUNT];
    dk::Shader m_fragShaders[SHADER_COUNT];
    ShaderProgram m_curShader = ShaderProgram::Basic;
#endif

    // Batching state
    Vertex2D*  m_vtxBase   = nullptr;
    uint32_t   m_vtxCount  = 0;
    uint32_t   m_vtxBatchStart = 0;
    int        m_curTexSlot = -1;
    bool       m_texturing  = false;

    // Clip stack
    std::vector<Rect> m_clipStack;

    // Texture descriptor tracking
    int m_nextDescSlot = 0;
    static constexpr int WHITE_TEX_SLOT = 0;

    // Offscreen target descriptor slots
    int m_offDescSlot[GpuDevice::NUM_OFFSCREEN] = {};

#ifdef NXUI_BACKEND_DEKO3D
    dk::Image          m_whiteImage;
    dk::UniqueMemBlock m_whiteMemBlock;

    // Track whether any image/sampler descriptor has been written
    // via CPU memcpy since the last GPU-side barrier.  When true,
    // flush() inserts barrier(DkBarrier_None, DkInvalidateFlags_Descriptors)
    // before the next draw call so the GPU re-reads from memory.
    bool m_descDirty = false;
#endif

#ifdef NXUI_BACKEND_SDL2
    // SDL2 backend: textures tracked by slot for binding
    std::vector<SDL_Texture*> m_texSlots;
    SDL_Texture* m_boundTex = nullptr;

    // Vertex buffer (CPU-side for SDL2)
    std::vector<Vertex2D> m_vtxBuf;
#endif

    bool m_boxWireframeEnabled = false;

    static inline std::string s_shaderBasePath = "romfs:/shaders/";
};

} // namespace nxui
