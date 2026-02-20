#pragma once
#include "Types.hpp"
#include <deko3d.hpp>
#include <string>
#include <memory>

namespace ui {

class GpuDevice;
class Renderer;

class Texture {
public:
    Texture() = default;
    ~Texture() = default;

    // dk::UniqueHandle has no UniqueHandle(UniqueHandle&&), only UniqueHandle(T&&).
    // We must provide explicit move ops that cast through the base type.
    Texture(Texture&& o) noexcept
        : m_image(o.m_image)
        , m_mem(static_cast<dk::MemBlock&&>(o.m_mem))
        , m_width(o.m_width), m_height(o.m_height)
        , m_slot(o.m_slot), m_valid(o.m_valid)
    {
        o.m_width = o.m_height = 0;
        o.m_slot = -1;
        o.m_valid = false;
    }
    Texture& operator=(Texture&& o) noexcept {
        if (this != &o) {
            m_mem   = nullptr; // release old
            m_image = o.m_image;
            m_mem   = static_cast<dk::MemBlock&&>(o.m_mem);
            m_width = o.m_width;  m_height = o.m_height;
            m_slot  = o.m_slot;   m_valid  = o.m_valid;
            o.m_width = o.m_height = 0;
            o.m_slot = -1;
            o.m_valid = false;
        }
        return *this;
    }
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Load from RGBA pixel data already in memory
    bool loadFromPixels(GpuDevice& gpu, Renderer& ren,
                        const uint8_t* rgba, int w, int h);

    // Load from image file (PNG/JPG via stb_image)
    bool loadFromFile(GpuDevice& gpu, Renderer& ren, const std::string& path);

    // Load from in-memory image data (JPEG/PNG via stb_image)
    bool loadFromMemory(GpuDevice& gpu, Renderer& ren,
                        const uint8_t* data, size_t dataSize);

    // Load from SDL_Surface-style data (RGBA8, row pitch may differ)
    bool loadFromSurface(GpuDevice& gpu, Renderer& ren,
                         const uint8_t* data, int w, int h, int pitch);

    int  width()  const { return m_width; }
    int  height() const { return m_height; }
    bool valid()  const { return m_valid; }

    // Descriptor slot in the renderer's image descriptor set
    int  descriptorSlot() const { return m_slot; }

    dk::Image&       image()       { return m_image; }
    const dk::Image& image() const { return m_image; }

private:
    dk::Image          m_image;
    dk::UniqueMemBlock m_mem;
    int m_width  = 0;
    int m_height = 0;
    int m_slot   = -1;
    bool m_valid = false;
};

} // namespace ui
