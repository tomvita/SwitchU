#include <nxui/widgets/Image.hpp>
#include <nxui/core/Renderer.hpp>
#include <nxui/core/GpuDevice.hpp>
#include <algorithm>

namespace nxui {

bool Image::loadFromFile(GpuDevice& gpu, Renderer& ren, const std::string& path) {
    return m_ownedTex.loadFromFile(gpu, ren, path.c_str());
}

bool Image::loadFromPixels(GpuDevice& gpu, Renderer& ren,
                           const uint8_t* rgba, int w, int h) {
    return m_ownedTex.loadFromPixels(gpu, ren, rgba, w, h);
}

bool Image::loadFromMemory(GpuDevice& gpu, Renderer& ren,
                           const void* data, size_t size) {
    return m_ownedTex.loadFromMemory(gpu, ren, static_cast<const uint8_t*>(data), size);
}

const Texture* Image::texture() const {
    if (m_externalTex && m_externalTex->valid()) return m_externalTex;
    if (m_ownedTex.valid()) return &m_ownedTex;
    return nullptr;
}

void Image::sizeToTexture() {
    auto* tex = texture();
    if (tex) {
        m_rect.width  = (float)tex->width()  + m_padding.horizontal();
        m_rect.height = (float)tex->height() + m_padding.vertical();
    }
}

Rect Image::computeDrawRect() const {
    Rect cr = contentRect();
    auto* tex = texture();
    if (!tex) return cr;

    if (m_scaleMode == ScaleMode::Stretch) return cr;

    float texW = (float)tex->width();
    float texH = (float)tex->height();
    if (texW <= 0 || texH <= 0) return cr;

    float aspect = texW / texH;
    float boxAspect = cr.width / cr.height;

    float drawW, drawH;
    if (m_scaleMode == ScaleMode::Fit) {
        if (aspect > boxAspect) { drawW = cr.width; drawH = drawW / aspect; }
        else                    { drawH = cr.height; drawW = drawH * aspect; }
    } else { // Fill
        if (aspect > boxAspect) { drawH = cr.height; drawW = drawH * aspect; }
        else                    { drawW = cr.width; drawH = drawW / aspect; }
    }

    float dx = cr.x + (cr.width  - drawW) * 0.5f;
    float dy = cr.y + (cr.height - drawH) * 0.5f;
    return {dx, dy, drawW, drawH};
}

void Image::onRender(Renderer& ren) {
    auto* tex = texture();
    if (!tex || !tex->valid()) return;

    Rect dr = computeDrawRect();
    Color tint = m_tint.withAlpha(m_tint.a * m_opacity);

    if (m_cornerRadius > 0.f)
        ren.drawTextureRounded(tex, dr, m_cornerRadius, tint);
    else
        ren.drawTexture(tex, dr, tint);
}

} // namespace nxui
