#pragma once
#include <SDL2/SDL_mixer.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace ui {

enum class Sfx {
    Navigate,
    Activate,
    PageChange,
    ModalShow,
    ModalHide,
    LaunchGame,
    ThemeToggle,
};

class AudioManager {
public:
    AudioManager() = default;
    ~AudioManager();

    bool initialize();
    void shutdown();

    // Background music
    void loadTrack(const std::string& path);
    void play();
    void stop();
    void nextTrack();
    void setVolume(float vol);

    // Sound effects
    void loadSfx(Sfx id, const std::string& path);
    void playSfx(Sfx id);
    void setSfxVolume(float vol);

private:
    static AudioManager* s_instance;
    static void onTrackFinished();

    std::vector<Mix_Music*> m_tracks;
    int   m_current = 0;
    float m_volume  = 0.5f;
    bool  m_initialized = false;

    std::unordered_map<int, Mix_Chunk*> m_sfx;
    float m_sfxVolume = 0.7f;
};

} // namespace ui
