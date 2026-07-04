#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <random>
#include <chrono>

// =============================================================================
//  Audio System - OpenAL-Soft Implementation
// =============================================================================

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool init();
    void cleanup();

    // Core functions
    void update();  // Call in main loop for BGM management

    // Sound controls
    void playEngine(bool moving);
    void playBoost(bool active);
    void stopAll();

private:
    struct Sound {
        ALuint source = 0;
        ALuint buffer = 0;
        bool loaded = false;
    };

    bool loadWAV(const std::string& path, ALuint& buffer);
    ALuint createSource();

    ALCdevice* m_device;
    ALCcontext* m_context;
    bool m_initialized;

    // Sounds
    Sound m_bgm[3];
    Sound m_engine;
    Sound m_boost;

    // State
    bool m_enginePlaying;
    bool m_boostPlaying;
    int m_currentBGM;
    std::mt19937 m_rng;

    // Constants
    static constexpr float BGM_VOLUME = 0.4f;
    static constexpr float ENGINE_VOLUME = 0.65f;
    static constexpr float BOOST_VOLUME = 0.65f;
};

extern AudioSystem g_audio;