#pragma once
#include "core/GridModel.hpp"
#include <nxui/core/Texture.hpp>
#include <nxui/core/GpuDevice.hpp>
#include <nxui/core/Renderer.hpp>
#include <nxui/core/ThreadPool.hpp>
#include <vector>
#include <string>
#include <future>
#include <cstdint>

struct PendingApp {
    std::string         id;
    std::string         title;
    uint64_t            titleId = 0;
    uint32_t            viewFlags = 0;
    std::vector<uint8_t> iconData;
    uint8_t*            rgba = nullptr;
    int                 w = 0, h = 0;
    bool                scaledWithMalloc = false;
};

class AppListLoader {
public:
    void load(nxui::GpuDevice& gpu, nxui::Renderer& ren,
              GridModel& model, std::vector<nxui::Texture>& outTextures);

    void startAsync(nxui::ThreadPool& pool);

    bool isReady() const;

    void finalize(nxui::GpuDevice& gpu, nxui::Renderer& ren,
                  GridModel& model, std::vector<nxui::Texture>& outTextures);

private:
    void fetchAndDecode();

    std::future<void>       m_future;
    std::vector<PendingApp> m_pending;
};
