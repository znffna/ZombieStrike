#include "Sound.h"
#include <fmod_errors.h>
#include <iostream>

// =========================
// Static Members
// =========================
FMOD::System* Sound::m_pSystem = nullptr;
std::unordered_map<std::string, FMOD::Sound*> Sound::m_SoundCache;
FMOD::Channel* Sound::m_pMusicChannel = nullptr;

float Sound::m_SoundVolume = 1.0f;
float Sound::m_MusicVolume = 1.0f;

// =========================
// Internal Helper
// =========================
static void CheckFMOD(FMOD_RESULT result)
{
    if (result != FMOD_OK)
    {
        std::cerr << "[FMOD ERROR] "
            << FMOD_ErrorString(result)
            << std::endl;
    }
}

FMOD_VECTOR Sound::ToFMOD(const XMFLOAT3& v)
{
    return FMOD_VECTOR{ v.x, v.y, v.z };
}

// =========================
// Life Cycle
// =========================
bool Sound::Initialize()
{
    FMOD_RESULT result;

    result = FMOD::System_Create(&m_pSystem);
    CheckFMOD(result);

    result = m_pSystem->init(
        512,
        FMOD_INIT_NORMAL,
        nullptr
    );
    CheckFMOD(result);

    return result == FMOD_OK;
}

void Sound::Update()
{
    if (m_pSystem)
        m_pSystem->update();
}

void Sound::Shutdown()
{
    for (auto& pair : m_SoundCache)
    {
        pair.second->release();
    }
    m_SoundCache.clear();

    if (m_pMusicChannel)
    {
        m_pMusicChannel->stop();
        m_pMusicChannel = nullptr;
    }

    if (m_pSystem)
    {
        m_pSystem->close();
        m_pSystem->release();
        m_pSystem = nullptr;
    }
}

// =========================
// Sound Load
// =========================
FMOD::Sound* Sound::LoadSound(
    const std::string& filepath,
    FMOD_MODE mode
)
{
    auto it = m_SoundCache.find(filepath);
    if (it != m_SoundCache.end())
        return it->second;

    FMOD::Sound* sound = nullptr;

    FMOD_RESULT result =
        m_pSystem->createSound(
            filepath.c_str(),
            mode,
            nullptr,
            &sound
        );

    CheckFMOD(result);

    if (result == FMOD_OK)
    {
        m_SoundCache[filepath] = sound;
        return sound;
    }

    return nullptr;
}

// =========================
// 2D Sound
// =========================
void Sound::PlaySound(const std::string& filepath)
{
    if (!m_pSystem)
        return;

    FMOD::Sound* sound = LoadSound(
        filepath,
        FMOD_DEFAULT | FMOD_LOOP_OFF
    );

    if (!sound)
        return;

    FMOD::Channel* channel = nullptr;
    m_pSystem->playSound(sound, nullptr, false, &channel);

    if (channel)
        channel->setVolume(m_SoundVolume);
}

void Sound::PlayMusic(const std::string& filepath)
{
    if (!m_pSystem)
        return;

    if (m_pMusicChannel)
    {
        bool isPlaying = false;
        m_pMusicChannel->isPlaying(&isPlaying);
        if (isPlaying)
            m_pMusicChannel->stop();
    }

    FMOD::Sound* music = LoadSound(
        filepath,
        FMOD_DEFAULT | FMOD_LOOP_NORMAL
    );

    if (!music)
        return;

    m_pSystem->playSound(
        music,
        nullptr,
        false,
        &m_pMusicChannel
    );

    if (m_pMusicChannel)
        m_pMusicChannel->setVolume(m_MusicVolume);
}

void Sound::StopMusic()
{
    if (m_pMusicChannel)
    {
        m_pMusicChannel->stop();
        m_pMusicChannel = nullptr;
    }
}

void Sound::SetSoundVolume(float volume)
{
    m_SoundVolume = volume;
}

void Sound::SetMusicVolume(float volume)
{
    m_MusicVolume = volume;

    if (m_pMusicChannel)
        m_pMusicChannel->setVolume(volume);
}

// =========================
// 3D Sound
// =========================
void Sound::Play3DSound(
    const std::string& filepath,
    const XMFLOAT3& position,
    float minDistance,
    float maxDistance
)
{
    if (!m_pSystem)
        return;

    FMOD::Sound* sound = LoadSound(
        filepath,
        FMOD_3D | FMOD_3D_LINEARROLLOFF
    );

    if (!sound)
        return;

    FMOD::Channel* channel = nullptr;

    // paused = true → 위치 설정 후 재생
    m_pSystem->playSound(sound, nullptr, true, &channel);

    if (!channel)
        return;

    FMOD_VECTOR pos = ToFMOD(position);
    FMOD_VECTOR vel = { 0,0,0 };

    channel->set3DAttributes(&pos, &vel);
    channel->set3DMinMaxDistance(minDistance, maxDistance);
    channel->setVolume(m_SoundVolume);
    channel->setPaused(false);
}

void Sound::SetListener(
    const XMFLOAT3& position,
    const XMFLOAT3& forward,
    const XMFLOAT3& up,
    const XMFLOAT3& velocity
)
{
    if (!m_pSystem)
        return;

    FMOD_VECTOR pos = ToFMOD(position);
    FMOD_VECTOR vel = ToFMOD(velocity);
    FMOD_VECTOR fwd = ToFMOD(forward);
    FMOD_VECTOR upv = ToFMOD(up);

    m_pSystem->set3DListenerAttributes(
        0,
        &pos,
        &vel,
        &fwd,
        &upv
    );
}