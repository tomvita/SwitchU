#include "GpuDevice.hpp"
#include <cstdio>
#include <cstring>
#include <array>

namespace ui {

bool GpuDevice::initialize() {
    m_dev   = dk::DeviceMaker{}.create();
    m_queue = dk::QueueMaker{m_dev}.setFlags(DkQueueFlags_Graphics).create();

    for (int i = 0; i < NUM_FB; ++i) {
        m_cmdPool[i].create(m_dev, CMD_BUF_SIZE, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached);
        m_cmdbuf[i] = dk::CmdBufMaker{m_dev}.create();
        m_cmdbuf[i].addMemory(m_cmdPool[i].block, 0, CMD_BUF_SIZE);
    }

    m_uploadCmdPool.create(m_dev, 64 * 1024, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached);
    m_uploadCmdbuf = dk::CmdBufMaker{m_dev}.create();
    m_uploadCmdbuf.addMemory(m_uploadCmdPool.block, 0, 64 * 1024);

    m_codePool.create(m_dev, CODE_POOL_SIZE,
        DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached | DkMemBlockFlags_Code);

    uint32_t dataSize = 0;
    for (int i = 0; i < NUM_FB; ++i) {
        dataSize += VTX_BUF_SIZE + 256;
        dataSize += IDX_BUF_SIZE + 256;
        dataSize += VS_UBO_SIZE + 256;
        dataSize += FS_UBO_SIZE + 256;
    }
    dataSize += MAX_TEXTURES * sizeof(DkImageDescriptor) + DK_IMAGE_DESCRIPTOR_ALIGNMENT;
    dataSize += MAX_SAMPLERS * sizeof(DkSamplerDescriptor) + DK_SAMPLER_DESCRIPTOR_ALIGNMENT;
    dataSize = (dataSize + kGpuAlign - 1) & ~(kGpuAlign - 1);
    m_dataPool.create(m_dev, dataSize, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached);

    for (int i = 0; i < NUM_FB; ++i) {
        m_vtxOff[i]   = m_dataPool.alloc(VTX_BUF_SIZE, 256);
        m_idxOff[i]   = m_dataPool.alloc(IDX_BUF_SIZE, 256);
        m_vsUboOff[i] = m_dataPool.alloc(VS_UBO_SIZE, DK_UNIFORM_BUF_ALIGNMENT);
        m_fsUboOff[i] = m_dataPool.alloc(FS_UBO_SIZE, DK_UNIFORM_BUF_ALIGNMENT);
    }
    m_imgDescOff = m_dataPool.alloc(MAX_TEXTURES * sizeof(DkImageDescriptor), DK_IMAGE_DESCRIPTOR_ALIGNMENT);
    m_samDescOff = m_dataPool.alloc(MAX_SAMPLERS * sizeof(DkSamplerDescriptor), DK_SAMPLER_DESCRIPTOR_ALIGNMENT);

    m_stagingPool.create(m_dev, 4 * 1024 * 1024, DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached);

    createFramebuffers();
    createDepthStencil();

    std::array<DkImage const*, NUM_FB> fbArray;
    for (int i = 0; i < NUM_FB; ++i) fbArray[i] = &m_fbImages[i];
    m_swapchain = dk::SwapchainMaker{m_dev, nwindowGetDefault(), fbArray}.create();

    return true;
}

void GpuDevice::createFramebuffers() {
    dk::ImageLayout fbLayout;
    dk::ImageLayoutMaker{m_dev}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_RGBA8_Unorm)
        .setDimensions(FB_WIDTH, FB_HEIGHT)
        .initialize(fbLayout);

    uint64_t fbSize  = fbLayout.getSize();
    uint64_t fbAlign = fbLayout.getAlignment();
    uint32_t totalFb = 0;
    for (int i = 0; i < NUM_FB; ++i) {
        totalFb = (totalFb + fbAlign - 1) & ~(fbAlign - 1);
        totalFb += fbSize;
    }
    m_fbPool.create(m_dev, totalFb, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image);

    uint32_t off = 0;
    for (int i = 0; i < NUM_FB; ++i) {
        off = (off + fbAlign - 1) & ~(fbAlign - 1);
        m_fbImages[i].initialize(fbLayout, m_fbPool.block, off);
        off += fbSize;
    }
}

void GpuDevice::createDepthStencil() {
    dk::ImageLayout dsLayout;
    dk::ImageLayoutMaker{m_dev}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_S8)
        .setDimensions(FB_WIDTH, FB_HEIGHT)
        .initialize(dsLayout);

    uint32_t dsSize = dsLayout.getSize();
    dsSize = (dsSize + kGpuAlign - 1) & ~(kGpuAlign - 1);
    m_dsPool.create(m_dev, dsSize, DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image);
    m_dsImage.initialize(dsLayout, m_dsPool.block, 0);
}

int GpuDevice::beginFrame() {
    m_slot = m_queue.acquireImage(m_swapchain);
    m_cmdbuf[m_slot].clear();
    return m_slot;
}

void GpuDevice::endFrame() {
    auto cmdList = m_cmdbuf[m_slot].finishList();
    m_queue.submitCommands(cmdList);
    m_queue.presentImage(m_swapchain, m_slot);
}

dk::UniqueMemBlock GpuDevice::allocImageMemory(uint32_t size) {
    size = (size + kGpuAlign - 1) & ~(kGpuAlign - 1);
    return dk::MemBlockMaker{m_dev, size}
        .setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image)
        .create();
}

void GpuDevice::uploadTexture(dk::Image& dst, const void* pixels, uint32_t size,
                              uint32_t w, uint32_t h)
{
    if (size > m_stagingPool.size) {
        std::fprintf(stderr, "[GpuDevice] Texture too large for staging (%u > %u)\n",
                     size, m_stagingPool.size);
        return;
    }
    std::memcpy(m_stagingPool.cpuBase, pixels, size);

    // Use dedicated upload command buffer
    m_uploadCmdbuf.clear();

    dk::ImageView view{dst};
    m_uploadCmdbuf.copyBufferToImage(
        {m_stagingPool.block.getGpuAddr(), 0, 0},
        view,
        {0, 0, 0, w, h, 1}
    );

    m_queue.submitCommands(m_uploadCmdbuf.finishList());
    m_queue.waitIdle();
}

void GpuDevice::shutdown() {
    if (m_queue) m_queue.waitIdle();
    m_swapchain    = {};
    for (int i = 0; i < NUM_FB; ++i)
        m_cmdbuf[i] = {};
    m_uploadCmdbuf = {};
    m_queue        = {};
    m_fbPool      = {};
    m_dsPool      = {};
    for (int i = 0; i < NUM_FB; ++i)
        m_cmdPool[i] = {};
    m_uploadCmdPool = {};
    m_codePool    = {};
    m_dataPool  = {};
    m_imagePool = {};
    m_stagingPool = {};
    m_dev       = {};
}

} // namespace ui
