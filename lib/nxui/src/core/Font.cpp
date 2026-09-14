#include <nxui/core/Font.hpp>
#include <nxui/core/Renderer.hpp>
#include <nxui/core/GpuDevice.hpp>
#include <SDL2/SDL.h>
#include <switch.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace nxui {

namespace {

// Decodes the UTF-8 sequence at i and advances past it. Malformed bytes are
// consumed one at a time and reported as U+FFFD.
Uint32 nextCodepoint(const std::string& text, std::size_t& i) {
    const unsigned char lead = static_cast<unsigned char>(text[i]);
    const std::size_t length = lead < 0x80 ? 1
        : (lead >> 5) == 0x6 ? 2
        : (lead >> 4) == 0xE ? 3
        : (lead >> 3) == 0x1E ? 4
        : 0;
    if (length == 0 || i + length > text.size()) {
        ++i;
        return 0xFFFD;
    }
    Uint32 codepoint = length == 1 ? lead
        : length == 2 ? (lead & 0x1F)
        : length == 3 ? (lead & 0x0F)
        : (lead & 0x07);
    for (std::size_t k = 1; k < length; ++k) {
        const unsigned char next = static_cast<unsigned char>(text[i + k]);
        if ((next & 0xC0) != 0x80) {
            ++i;
            return 0xFFFD;
        }
        codepoint = (codepoint << 6) | (next & 0x3F);
    }
    i += length;
    return codepoint;
}

struct SystemFonts {
    PlFontData data[PlSharedFontType_Total]{};
    s32 count = 0;
};

// The console's shared fonts, ordered for the system language (so Han
// characters use Chinese or Japanese shapes to match it). Loaded once per
// process; pl keeps the font data mapped for the process lifetime.
const SystemFonts& systemFonts() {
    static SystemFonts fonts;
    static bool loaded = false;
    if (loaded)
        return fonts;
    loaded = true;

    Result rc = plInitialize(PlServiceType_User);
    if (R_FAILED(rc)) {
        std::fprintf(stderr, "[Font] plInitialize failed: 0x%X\n", rc);
        return fonts;
    }

    u64 languageCode = 0;
    if (R_SUCCEEDED(setGetSystemLanguage(&languageCode)) &&
        R_SUCCEEDED(plGetSharedFont(languageCode, fonts.data,
                                    PlSharedFontType_Total, &fonts.count)))
        return fonts;

    fonts.count = 0;
    for (int type = 0; type < PlSharedFontType_Total; ++type) {
        if (R_SUCCEEDED(plGetSharedFontByType(&fonts.data[fonts.count],
                                              static_cast<PlSharedFontType>(type))))
            ++fonts.count;
    }
    return fonts;
}

} // namespace

Font::~Font() {
    closeFallbacks();
    if (m_font) TTF_CloseFont(m_font);
}

bool Font::load(GpuDevice& gpu, Renderer& ren,
                const std::string& path, int ptSize)
{
    TTF_Font* newFont = TTF_OpenFont(path.c_str(), ptSize);
    if (!newFont) {
        std::fprintf(stderr, "[Font] TTF_OpenFont failed: %s\n", TTF_GetError());
        return false;
    }

    if (m_font)
        TTF_CloseFont(m_font);
    closeFallbacks();

    m_font = newFont;
    m_gpu = &gpu;
    m_ren = &ren;
    m_ptSize = ptSize;
    ++m_revision;
    clearCache();
    return true;
}

void Font::closeFallbacks() {
    for (auto& slot : m_fallbacks) {
        if (slot.font)
            TTF_CloseFont(slot.font);
    }
    m_fallbacks.clear();
}

TTF_Font* Font::fontForCodepoint(Uint32 codepoint) const {
    if (codepoint < 0x80 || TTF_GlyphIsProvided32(m_font, codepoint))
        return m_font;

    const SystemFonts& fonts = systemFonts();
    if (m_fallbacks.size() != static_cast<std::size_t>(fonts.count))
        m_fallbacks.assign(static_cast<std::size_t>(fonts.count), FallbackSlot{});

    for (s32 index = 0; index < fonts.count; ++index) {
        FallbackSlot& slot = m_fallbacks[static_cast<std::size_t>(index)];
        if (!slot.font && !slot.failed) {
            SDL_RWops* data = SDL_RWFromConstMem(fonts.data[index].address,
                                                 static_cast<int>(fonts.data[index].size));
            slot.font = data ? TTF_OpenFontRW(data, 1, m_ptSize) : nullptr;
            slot.failed = slot.font == nullptr;
            if (slot.failed)
                std::fprintf(stderr, "[Font] system font %d failed: %s\n",
                             static_cast<int>(fonts.data[index].type), TTF_GetError());
        }
        if (slot.font && TTF_GlyphIsProvided32(slot.font, codepoint))
            return slot.font;
    }
    // No font has it: the loaded font draws its missing-glyph box.
    return m_font;
}

void Font::splitRuns(const std::string& text, std::vector<TextRun>& runs) const {
    runs.clear();
    std::size_t i = 0;
    while (i < text.size()) {
        const std::size_t start = i;
        TTF_Font* font = fontForCodepoint(nextCodepoint(text, i));
        if (!runs.empty() && runs.back().font == font)
            runs.back().text.append(text, start, i - start);
        else
            runs.push_back({font, text.substr(start, i - start)});
    }
}

// Draws each run with its own font side by side on a shared baseline.
SDL_Surface* Font::renderRuns(const std::vector<TextRun>& runs) const {
    int width = 0;
    int ascent = TTF_FontAscent(m_font);
    int descent = TTF_FontDescent(m_font);
    for (const auto& run : runs) {
        int runWidth = 0, runHeight = 0;
        TTF_SizeUTF8(run.font, run.text.c_str(), &runWidth, &runHeight);
        width += runWidth;
        ascent = std::max(ascent, TTF_FontAscent(run.font));
        descent = std::min(descent, TTF_FontDescent(run.font));
    }
    const int height = std::max(ascent - descent, TTF_FontHeight(m_font));
    if (width <= 0 || height <= 0)
        return nullptr;

    SDL_Surface* target = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32,
                                                         SDL_PIXELFORMAT_ARGB8888);
    if (!target)
        return nullptr;
    SDL_FillRect(target, nullptr, 0);

    const SDL_Color white = {255, 255, 255, 255};
    int x = 0;
    for (const auto& run : runs) {
        int runWidth = 0, runHeight = 0;
        TTF_SizeUTF8(run.font, run.text.c_str(), &runWidth, &runHeight);
        if (SDL_Surface* piece = TTF_RenderUTF8_Blended(run.font, run.text.c_str(), white)) {
            // Runs don't overlap, so copy pixels as-is: blending onto the
            // transparent target would darken the glyph edges.
            SDL_SetSurfaceBlendMode(piece, SDL_BLENDMODE_NONE);
            SDL_Rect dst = {x, ascent - TTF_FontAscent(run.font), piece->w, piece->h};
            SDL_BlitSurface(piece, nullptr, target, &dst);
            SDL_FreeSurface(piece);
        }
        x += runWidth;
    }
    return target;
}

Vec2 Font::measure(const std::string& text) const {
    if (!m_font || text.empty()) return {0, 0};
    // Rendered strings already carry their exact dimensions. Avoid invoking
    // SDL_ttf again for every label on every frame.
    const auto cached = m_lruMap.find(text);
    if (cached != m_lruMap.end())
        return {static_cast<float>(cached->second->w),
                static_cast<float>(cached->second->h)};

    std::vector<TextRun> runs;
    splitRuns(text, runs);
    if (runs.size() == 1 && runs.front().font == m_font) {
        int w = 0, h = 0;
        TTF_SizeUTF8(m_font, text.c_str(), &w, &h);
        return {(float)w, (float)h};
    }

    int width = 0;
    int ascent = TTF_FontAscent(m_font);
    int descent = TTF_FontDescent(m_font);
    for (const auto& run : runs) {
        int runWidth = 0, runHeight = 0;
        TTF_SizeUTF8(run.font, run.text.c_str(), &runWidth, &runHeight);
        width += runWidth;
        ascent = std::max(ascent, TTF_FontAscent(run.font));
        descent = std::min(descent, TTF_FontDescent(run.font));
    }
    return {(float)width, (float)std::max(ascent - descent, TTF_FontHeight(m_font))};
}

void Font::clearCache() {
    m_lruList.clear();
    m_lruMap.clear();
    m_cacheBytes = 0;
    m_maintenanceRequested = false;
}

void Font::trimCache(std::size_t maxEntries, std::size_t maxBytes) {
    while (!m_lruList.empty() &&
           (m_lruList.size() > maxEntries || m_cacheBytes > maxBytes)) {
        auto victim = std::prev(m_lruList.end());
        const std::size_t bytes = victim->tex.allocationSize();
        m_lruMap.erase(victim->key);
        m_lruList.erase(victim);
        m_cacheBytes = bytes <= m_cacheBytes ? m_cacheBytes - bytes : 0;
    }
    m_maintenanceRequested = false;
}

Texture* Font::getOrRender(GpuDevice& gpu, Renderer& ren, const std::string& text) {
    // Cache hit: promote to the front.
    auto it = m_lruMap.find(text);
    if (it != m_lruMap.end()) {
        // Move to front (most-recently-used)
        m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
        return &it->second->tex;
    }

    // Render the string via SDL_ttf, switching fonts for characters the
    // loaded font lacks.
    std::vector<TextRun> runs;
    splitRuns(text, runs);
    SDL_Surface* surface = nullptr;
    if (runs.size() == 1 && runs.front().font == m_font) {
        SDL_Color white = {255, 255, 255, 255};
        surface = TTF_RenderUTF8_Blended(m_font, text.c_str(), white);
    } else {
        surface = renderRuns(runs);
    }
    if (!surface) return nullptr;

    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
    SDL_FreeSurface(surface);
    if (!rgba) return nullptr;

    SDL_LockSurface(rgba);
    const uint8_t* pixels = static_cast<const uint8_t*>(rgba->pixels);
    int w = rgba->w, h = rgba->h, pitch = rgba->pitch;

    constexpr std::size_t kTextureAllocationAlignment = 4096u;
    const std::size_t estimatedBytes =
        (static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u +
         kTextureAllocationAlignment - 1u) &
        ~(kTextureAllocationAlignment - 1u);
    if (m_lruList.size() >= kMaxCacheEntries ||
        m_cacheBytes + estimatedBytes > kMaxCacheBytes ||
        gpu.imageMemoryAvailable() < estimatedBytes + kGpuHeadroom) {
        m_maintenanceRequested = true;
        SDL_UnlockSurface(rgba);
        SDL_FreeSurface(rgba);
        return nullptr;
    }

    m_lruList.emplace_front();
    auto& entry = m_lruList.front();
    entry.key = text;
    entry.w = w;
    entry.h = h;
    if (!entry.tex.loadFromSurface(gpu, ren, pixels, w, h, pitch)) {
        m_lruList.pop_front();
        m_maintenanceRequested = true;
        SDL_UnlockSurface(rgba);
        SDL_FreeSurface(rgba);
        return nullptr;
    }
    m_cacheBytes += entry.tex.allocationSize();

    SDL_UnlockSurface(rgba);
    SDL_FreeSurface(rgba);

    m_lruMap[text] = m_lruList.begin();
    return &m_lruList.front().tex;
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

} // namespace nxui
