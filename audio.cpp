#include "audio.h"

AudioSystem g_audio;

// =============================================================================
//  WAV Loader
// =============================================================================

struct WAVHeader {
    char riff[4];
    uint32_t size;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};

static bool readWAV(const std::string& path, std::vector<char>& data,
    ALenum& format, ALsizei& freq) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Audio: Cannot open file: " << path << std::endl;
        return false;
    }

    // Read RIFF header
    char riff[4];
    uint32_t fileSize;
    char wave[4];

    file.read(riff, 4);
    file.read(reinterpret_cast<char*>(&fileSize), 4);
    file.read(wave, 4);

    if (std::string(riff, 4) != "RIFF" || std::string(wave, 4) != "WAVE") {
        std::cerr << "Audio: Invalid WAV format in: " << path << std::endl;
        return false;
    }

    // Variables to store audio info
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    std::vector<char> audioData;
    bool foundData = false;

    // Parse chunks
    while (!file.eof() && !foundData) {
        char chunkId[4];
        uint32_t chunkSize;

        if (!file.read(chunkId, 4)) {
            break;
        }

        if (!file.read(reinterpret_cast<char*>(&chunkSize), 4)) {
            break;
        }

        if (std::string(chunkId, 4) == "fmt ") {
            // Read format chunk
            uint16_t audioFormat;
            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&numChannels), 2);
            file.read(reinterpret_cast<char*>(&sampleRate), 4);

            uint32_t byteRate;
            uint16_t blockAlign;
            file.read(reinterpret_cast<char*>(&byteRate), 4);
            file.read(reinterpret_cast<char*>(&blockAlign), 2);
            file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

            // Skip any extra data in fmt chunk (if chunkSize > 16)
            if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
        }
        else if (std::string(chunkId, 4) == "data") {
            // Read data chunk
            audioData.resize(chunkSize);
            if (!file.read(audioData.data(), chunkSize)) {
                std::cerr << "Audio: Failed to read data chunk in: " << path << std::endl;
                return false;
            }
            foundData = true;
            data = std::move(audioData);
        }
        else {
            // Skip unknown chunk
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    if (!foundData) {
        std::cerr << "Audio: Could not find data chunk in: " << path << std::endl;
        return false;
    }

    // Determine format
    if (numChannels == 1 && bitsPerSample == 16)
        format = AL_FORMAT_MONO16;
    else if (numChannels == 2 && bitsPerSample == 16)
        format = AL_FORMAT_STEREO16;
    else if (numChannels == 1 && bitsPerSample == 8)
        format = AL_FORMAT_MONO8;
    else if (numChannels == 2 && bitsPerSample == 8)
        format = AL_FORMAT_STEREO8;
    else {
        std::cerr << "Audio: Unsupported WAV format in: " << path
            << " (channels: " << numChannels
            << ", bits: " << bitsPerSample << ")" << std::endl;
        return false;
    }

    freq = sampleRate;
    return true;
}

// =============================================================================
//  AudioSystem Implementation
// =============================================================================

AudioSystem::AudioSystem()
    : m_device(nullptr), m_context(nullptr), m_initialized(false),
    m_enginePlaying(false), m_boostPlaying(false), m_currentBGM(-1),
    m_rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
}

AudioSystem::~AudioSystem() { cleanup(); }

bool AudioSystem::init() {
    if (m_initialized) return true;

    // Open device
    m_device = alcOpenDevice(nullptr);
    if (!m_device) {
        std::cerr << "Audio: Failed to open device\n";
        return false;
    }

    // Create context
    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context || !alcMakeContextCurrent(m_context)) {
        std::cerr << "Audio: Failed to create context\n";
        return false;
    }

    // Generate sources and buffers
    auto createSound = [this](Sound& s) {
        alGenSources(1, &s.source);
        alGenBuffers(1, &s.buffer);
        alSourcef(s.source, AL_GAIN, BGM_VOLUME);
        alSourcei(s.source, AL_LOOPING, AL_TRUE);
        };

    for (int i = 0; i < 3; i++) createSound(m_bgm[i]);
    createSound(m_engine);
    createSound(m_boost);

    // Set specific volumes
    alSourcef(m_engine.source, AL_GAIN, ENGINE_VOLUME);
    alSourcei(m_engine.source, AL_LOOPING, AL_TRUE);
    alSourcef(m_boost.source, AL_GAIN, BOOST_VOLUME);
    alSourcei(m_boost.source, AL_LOOPING, AL_FALSE);

    // Load sounds
    const std::string base = "./Assets/Audio/";
    const char* bgmFiles[] = { "bgm1.wav", "bgm2.wav", "bgm3.wav" };

    for (int i = 0; i < 3; i++) {
        m_bgm[i].loaded = loadWAV(base + bgmFiles[i], m_bgm[i].buffer);
        if (m_bgm[i].loaded) {
            alSourcei(m_bgm[i].source, AL_BUFFER, m_bgm[i].buffer);
        }
    }

    m_engine.loaded = loadWAV(base + "engine.wav", m_engine.buffer);
    if (m_engine.loaded) {
        alSourcei(m_engine.source, AL_BUFFER, m_engine.buffer);
    }

    m_boost.loaded = loadWAV(base + "boost.wav", m_boost.buffer);
    if (m_boost.loaded) {
        alSourcei(m_boost.source, AL_BUFFER, m_boost.buffer);
    }

    m_initialized = true;

    // Start random BGM
    std::uniform_int_distribution<int> dist(0, 2);
    m_currentBGM = dist(m_rng);
    if (m_bgm[m_currentBGM].loaded) {
        alSourcePlay(m_bgm[m_currentBGM].source);
    }

    return true;
}

void AudioSystem::cleanup() {
    if (!m_initialized) return;

    // Stop all
    for (int i = 0; i < 3; i++) {
        alSourceStop(m_bgm[i].source);
        alDeleteSources(1, &m_bgm[i].source);
        alDeleteBuffers(1, &m_bgm[i].buffer);
    }
    alSourceStop(m_engine.source);
    alSourceStop(m_boost.source);
    alDeleteSources(1, &m_engine.source);
    alDeleteBuffers(1, &m_engine.buffer);
    alDeleteSources(1, &m_boost.source);
    alDeleteBuffers(1, &m_boost.buffer);

    alcMakeContextCurrent(nullptr);
    if (m_context) alcDestroyContext(m_context);
    if (m_device) alcCloseDevice(m_device);

    m_initialized = false;
}

bool AudioSystem::loadWAV(const std::string& path, ALuint& buffer) {
    std::vector<char> data;
    ALenum format;
    ALsizei freq;

    if (!readWAV(path, data, format, freq)) {
        return false;
    }

    alBufferData(buffer, format, data.data(),
        static_cast<ALsizei>(data.size()), freq);
    return true;
}

void AudioSystem::update() {
    if (!m_initialized) return;

    // Check if current BGM stopped
    if (m_currentBGM >= 0 && m_bgm[m_currentBGM].loaded) {
        ALint state;
        alGetSourcei(m_bgm[m_currentBGM].source, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            // Play random BGM
            std::uniform_int_distribution<int> dist(0, 2);
            m_currentBGM = dist(m_rng);
            if (m_bgm[m_currentBGM].loaded) {
                alSourcePlay(m_bgm[m_currentBGM].source);
            }
        }
    }
}

void AudioSystem::playEngine(bool moving) {
    if (!m_initialized || !m_engine.loaded) return;

    if (moving && !m_enginePlaying) {
        alSourcePlay(m_engine.source);
        m_enginePlaying = true;
    }
    else if (!moving && m_enginePlaying) {
        alSourceStop(m_engine.source);
        m_enginePlaying = false;
    }
}

void AudioSystem::playBoost(bool active) {
    if (!m_initialized || !m_boost.loaded) return;

    if (active && !m_boostPlaying) {
        alSourcePlay(m_boost.source);
        m_boostPlaying = true;
    }
    else if (!active && m_boostPlaying) {
        alSourceStop(m_boost.source);
        m_boostPlaying = false;
    }
}

void AudioSystem::stopAll() {
    if (!m_initialized) return;
    for (int i = 0; i < 3; i++) alSourceStop(m_bgm[i].source);
    alSourceStop(m_engine.source);
    alSourceStop(m_boost.source);
    m_enginePlaying = false;
    m_boostPlaying = false;
}