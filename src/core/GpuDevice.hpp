#pragma once

#include <deko3d.hpp>
#include <switch.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <memory>

namespace ui {

inline constexpr uint32_t kGpuAlign = DK_MEMBLOCK_ALIGNMENT;   // 0x1000

struct GpuPool {
    dk::UniqueMemBlock block;
    uint32_t size  = 0;
    uint32_t used  = 0;
    void*    cpuBase = nullptr;     // Non-null only if CPU-accessible
    DkGpuAddr gpuBase = 0;         // Cached GPU base address

    bool create(dk::Device dev, uint32_t sz, uint32_t flags) {
        size = (sz + kGpuAlign - 1) & ~(kGpuAlign - 1);
        block = dk::MemBlockMaker{dev, size}.setFlags(flags).create();
        if (flags & DkMemBlockFlags_CpuUncached)
            cpuBase = block.getCpuAddr();
        gpuBase = block.getGpuAddr();
        used = 0;
        return true;
    }

    // Sub-allocate (aligned)
    uint32_t alloc(uint32_t bytes, uint32_t align = 256) {
        uint32_t off = (used + align - 1) & ~(align - 1);
        if (off + bytes > size) return UINT32_MAX;
        used = off + bytes;
        return off;
    }

    DkGpuAddr gpuAddr(uint32_t off = 0) const { return gpuBase + off; }
    void*     cpuAddr(uint32_t off = 0) const { return (uint8_t*)cpuBase + off; }
};

class GpuDevice {
public:
    static constexpr int FB_WIDTH  = 1280;
    static constexpr int FB_HEIGHT = 720;
    static constexpr int NUM_FB    = 2;

    // Max resources
    static constexpr int MAX_TEXTURES   = 128;
    static constexpr int MAX_SAMPLERS   = 16;
    static constexpr int MAX_VERTICES   = 65536;
    static constexpr int VTX_BUF_SIZE   = MAX_VERTICES * 32;   // 32 bytes per vertex
    static constexpr int IDX_BUF_SIZE   = MAX_VERTICES * 6 * 4;// generous
    static constexpr int VS_UBO_SIZE    = 256;                 // projection matrix (64 used, 256-aligned)
    static constexpr int FS_UBO_SIZE    = 256;                 // fragment params (16 used, 256-aligned)
    static constexpr int CMD_BUF_SIZE   = 256 * 1024;
    static constexpr int CODE_POOL_SIZE = 128 * 1024;

    bool initialize();
    void shutdown();

    // Frame lifecycle
    int  beginFrame();              // Returns slot index
    void endFrame();                // Submit + present

    // Accessors
    dk::Device   device()  const { return m_dev; }
    dk::Queue    queue()   const { return m_queue; }
    dk::CmdBuf  cmdBuf()  const { return m_cmdbuf[m_slot]; }
    int          width()   const { return FB_WIDTH; }
    int          height()  const { return FB_HEIGHT; }
    int          slot()    const { return m_slot; }

    // Memory pools
    GpuPool& codePool()  { return m_codePool; }
    GpuPool& dataPool()  { return m_dataPool; }
    GpuPool& imagePool() { return m_imagePool; }

    // Descriptor management
    DkGpuAddr imgDescGpuAddr()  const { return m_dataPool.gpuAddr(m_imgDescOff); }
    DkGpuAddr samDescGpuAddr()  const { return m_dataPool.gpuAddr(m_samDescOff); }
    void*     imgDescCpuAddr()  const { return m_dataPool.cpuAddr(m_imgDescOff); }
    void*     samDescCpuAddr()  const { return m_dataPool.cpuAddr(m_samDescOff); }

    // Vertex / Index / UBO offsets per frame
    DkGpuAddr vtxGpuAddr(int frame)   const { return m_dataPool.gpuAddr(m_vtxOff[frame]); }
    void*     vtxCpuAddr(int frame)   const { return m_dataPool.cpuAddr(m_vtxOff[frame]); }
    DkGpuAddr idxGpuAddr(int frame)   const { return m_dataPool.gpuAddr(m_idxOff[frame]); }
    void*     idxCpuAddr(int frame)   const { return m_dataPool.cpuAddr(m_idxOff[frame]); }
    DkGpuAddr vsUboGpuAddr(int frame) const { return m_dataPool.gpuAddr(m_vsUboOff[frame]); }
    void*     vsUboCpuAddr(int frame) const { return m_dataPool.cpuAddr(m_vsUboOff[frame]); }
    DkGpuAddr fsUboGpuAddr(int frame) const { return m_dataPool.gpuAddr(m_fsUboOff[frame]); }
    void*     fsUboCpuAddr(int frame) const { return m_dataPool.cpuAddr(m_fsUboOff[frame]); }

    // Texture memory allocation (returns a new dk::MemBlock for image data)
    dk::UniqueMemBlock allocImageMemory(uint32_t size);

    // Upload data from CPU staging buffer to image
    void uploadTexture(dk::Image& dst, const void* pixels, uint32_t size, uint32_t width, uint32_t height);

    // Framebuffer / depth-stencil image access
    dk::Image&       fbImage(int i)       { return m_fbImages[i]; }
    const dk::Image& fbImage(int i) const { return m_fbImages[i]; }
    dk::Image&       dsImage()            { return m_dsImage; }
    const dk::Image& dsImage() const      { return m_dsImage; }

private:
    void createFramebuffers();
    void createDepthStencil();

    dk::UniqueDevice    m_dev;
    dk::UniqueQueue     m_queue;
    dk::UniqueCmdBuf    m_cmdbuf[NUM_FB];     // Per-slot command buffers
    dk::UniqueCmdBuf    m_uploadCmdbuf;
    dk::UniqueSwapchain m_swapchain;

    // Memory pools
    GpuPool m_fbPool;       // Framebuffer images
    GpuPool m_dsPool;       // Depth/stencil
    GpuPool m_cmdPool[NUM_FB]; // Command buffer memory (per-slot)
    GpuPool m_uploadCmdPool;// Upload command buffer (separate)
    GpuPool m_codePool;     // Shader code
    GpuPool m_dataPool;     // Vertices, indices, UBOs, descriptors
    GpuPool m_imagePool;    // Reserved (not used — textures allocate their own)
    GpuPool m_stagingPool;  // Staging buffer for texture uploads

    // Framebuffers
    dk::Image  m_fbImages[NUM_FB];
    dk::Image  m_dsImage;

    // Sub-allocation offsets within data pool
    uint32_t m_vtxOff[NUM_FB] {};
    uint32_t m_idxOff[NUM_FB] {};
    uint32_t m_vsUboOff[NUM_FB] {};
    uint32_t m_fsUboOff[NUM_FB] {};
    uint32_t m_imgDescOff = 0;
    uint32_t m_samDescOff = 0;

    int m_slot = -1;
};

} // namespace ui
