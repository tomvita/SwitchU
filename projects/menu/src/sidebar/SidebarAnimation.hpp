#pragma once
#include <nxui/core/Texture.hpp>
#include <nxui/core/GpuDevice.hpp>
#include <nxui/core/Renderer.hpp>
#include <vector>
#include <string>

class SidebarAnimation {
public:
    bool load(nxui::GpuDevice& gpu, nxui::Renderer& ren, const std::string& webpPath);

    void update(float dt, bool focused);

    nxui::Texture* currentFrame();

    void reset();

    bool hasFrames() const { return !m_frames.empty(); }

    int  frameCount() const { return static_cast<int>(m_frames.size()); }

private:
    std::vector<nxui::Texture> m_frames;
    std::vector<int>           m_durationsMs;
    int                        m_frameIndex  = 0;
    float                      m_elapsedMs   = 0.f;
};
