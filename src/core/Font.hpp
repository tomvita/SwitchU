#pragma once
#include "Types.hpp"
#include "Texture.hpp"
#include <SDL2/SDL_ttf.h>
#include <string>
#include <unordered_map>

namespace ui {

class Renderer;
class GpuDevice;

class Font {
public:
    Font() = default;
    ~Font();

    bool load(GpuDevice& gpu, Renderer& ren,
              const std::string& path, int ptSize);

    // Render a string to a texture (cached) and draw it
    void draw(Renderer& ren, const std::string& text,
              const Vec2& pos, const Color& color, float scale = 1.f);

    // Measure text dimensions
    Vec2 measure(const std::string& text) const;

    int ptSize() const { return m_ptSize; }

private:
    struct CachedGlyph {
        Texture  texture;
        int      w = 0, h = 0;
    };

    // Render full string to texture (cache by string)
    Texture* getOrRender(GpuDevice& gpu, Renderer& ren, const std::string& text);

    TTF_Font* m_font = nullptr;
    int       m_ptSize = 0;
    GpuDevice* m_gpu = nullptr;
    Renderer*  m_ren = nullptr;

    // String-level texture cache
    struct CachedString {
        Texture tex;
        int w = 0, h = 0;
    };
    std::unordered_map<std::string, CachedString> m_cache;
};

} // namespace ui
