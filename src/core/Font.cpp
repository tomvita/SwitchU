#include "Font.hpp"
#include "Renderer.hpp"
#include "GpuDevice.hpp"
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstring>

namespace ui {

Font::~Font() {
    if (m_font) TTF_CloseFont(m_font);
}

bool Font::load(GpuDevice& gpu, Renderer& ren,
                const std::string& path, int ptSize)
{
    m_gpu = &gpu;
    m_ren = &ren;
    m_ptSize = ptSize;

    m_font = TTF_OpenFont(path.c_str(), ptSize);
    if (!m_font) {
        std::fprintf(stderr, "[Font] TTF_OpenFont failed: %s\n", TTF_GetError());
        return false;
    }
    return true;
}

Vec2 Font::measure(const std::string& text) const {
    if (!m_font || text.empty()) return {0, 0};
    int w = 0, h = 0;
    TTF_SizeUTF8(m_font, text.c_str(), &w, &h);
    return {(float)w, (float)h};
}

Texture* Font::getOrRender(GpuDevice& gpu, Renderer& ren, const std::string& text) {
    auto it = m_cache.find(text);
    if (it != m_cache.end()) return &it->second.tex;

    // Render white text (we tint via vertex color)
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(m_font, text.c_str(), white);
    if (!surface) return nullptr;

    // Convert to RGBA32 if not already
    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return nullptr;

    // Default-construct entry in-place (dk::UniqueMemBlock is not move-constructible)
    auto& cs = m_cache[text];
    cs.w = rgba->w;
    cs.h = rgba->h;

    SDL_LockSurface(rgba);
    cs.tex.loadFromSurface(gpu, ren,
        static_cast<const uint8_t*>(rgba->pixels), rgba->w, rgba->h, rgba->pitch);
    SDL_UnlockSurface(rgba);
    SDL_FreeSurface(rgba);

    return &cs.tex;
}

void Font::draw(Renderer& ren, const std::string& text,
                const Vec2& pos, const Color& color, float scale) {
    if (!m_font || !m_gpu || text.empty()) return;

    Texture* tex = getOrRender(*m_gpu, ren, text);
    if (!tex || !tex->valid()) return;

    float w = tex->width()  * scale;
    float h = tex->height() * scale;
    ren.drawTexture(tex, {pos.x, pos.y, w, h}, color);
}

} // namespace ui
