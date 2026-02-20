#include "AudioManager.hpp"
#include <SDL2/SDL.h>
#include <cstdio>

namespace ui {

AudioManager::~AudioManager() { shutdown(); }

bool AudioManager::initialize() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::fprintf(stderr, "[Audio] SDL_Init(AUDIO) failed: %s\n", SDL_GetError());
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        std::fprintf(stderr, "[Audio] Mix_OpenAudio failed: %s\n", Mix_GetError());
        return false;
    }
    Mix_AllocateChannels(8);  // 8 channels for SFX
    m_initialized = true;
    setVolume(m_volume);
    return true;
}

void AudioManager::shutdown() {
    Mix_HookMusicFinished(nullptr);
    s_instance = nullptr;
    Mix_HaltMusic();
    for (auto* m : m_tracks) Mix_FreeMusic(m);
    m_tracks.clear();
    for (auto& [id, chunk] : m_sfx) Mix_FreeChunk(chunk);
    m_sfx.clear();
    if (m_initialized) {
        Mix_CloseAudio();
        m_initialized = false;
    }
}

void AudioManager::loadTrack(const std::string& path) {
    Mix_Music* music = Mix_LoadMUS(path.c_str());
    if (!music) {
        std::fprintf(stderr, "[Audio] Failed to load %s: %s\n", path.c_str(), Mix_GetError());
        return;
    }
    m_tracks.push_back(music);
}

AudioManager* AudioManager::s_instance = nullptr;

void AudioManager::onTrackFinished() {
    if (s_instance) s_instance->nextTrack();
}

void AudioManager::play() {
    if (m_tracks.empty()) return;
    s_instance = this;
    Mix_HookMusicFinished(onTrackFinished);
    Mix_PlayMusic(m_tracks[m_current], 1);
}

void AudioManager::stop() {
    Mix_HookMusicFinished(nullptr);
    Mix_HaltMusic();
}

void AudioManager::nextTrack() {
    if (m_tracks.empty()) return;
    m_current = (m_current + 1) % (int)m_tracks.size();
    play();
}

void AudioManager::setVolume(float vol) {
    m_volume = vol;
    Mix_VolumeMusic((int)(vol * MIX_MAX_VOLUME));
}

void AudioManager::loadSfx(Sfx id, const std::string& path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        std::fprintf(stderr, "[Audio] Failed to load SFX %s: %s\n", path.c_str(), Mix_GetError());
        return;
    }
    Mix_VolumeChunk(chunk, (int)(m_sfxVolume * MIX_MAX_VOLUME));
    m_sfx[static_cast<int>(id)] = chunk;
}

void AudioManager::playSfx(Sfx id) {
    auto it = m_sfx.find(static_cast<int>(id));
    if (it != m_sfx.end()) {
        Mix_PlayChannel(-1, it->second, 0);
    }
}

void AudioManager::setSfxVolume(float vol) {
    m_sfxVolume = vol;
    for (auto& [id, chunk] : m_sfx) {
        Mix_VolumeChunk(chunk, (int)(vol * MIX_MAX_VOLUME));
    }
}

} // namespace ui
