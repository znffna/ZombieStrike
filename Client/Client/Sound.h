#pragma once


#include <string>
#include <unordered_map>

#include <fmod.hpp>
#include <fmod_errors.h>

#include "stdafx.h"

class Sound
{
public:
    // =========================
    // Life Cycle
    // =========================
    static bool Initialize();
    static void Update();
    static void Shutdown();

    // =========================
    // 2D Sound
    // =========================
    static void PlaySound(
        const std::string& filepath
    );

    static void PlayMusic(
        const std::string& filepath
    );

    static void StopMusic();

    static void SetSoundVolume(float volume); // SFX
    static void SetMusicVolume(float volume); // BGM

    // =========================
    // 3D Sound
    // =========================
    static void Play3DSound(
        const std::string& filepath,
        const XMFLOAT3& position,
        float minDistance = 1.0f,
        float maxDistance = 50.0f
    );

    static void SetListener(
        const XMFLOAT3& position,
        const XMFLOAT3& forward,
        const XMFLOAT3& up,
        const XMFLOAT3& velocity = XMFLOAT3(0, 0, 0)
    );

private:
    static FMOD::Sound* LoadSound(
        const std::string& filepath,
        FMOD_MODE mode
    );

    static FMOD_VECTOR ToFMOD(const XMFLOAT3& v);

private:
    static FMOD::System* m_pSystem;

    static std::unordered_map<std::string, FMOD::Sound*> m_SoundCache;

    static FMOD::Channel* m_pMusicChannel;

    static float m_SoundVolume;
    static float m_MusicVolume;
};

// Example Usage:
/*
// 초기화
Sound::Initialize();

// 매 프레임
Sound::SetListener(camPos, camForward, camUp, camVelocity);
Sound::Update();

// 2D
Sound::PlaySound("Assets/Sound/ui_click.wav");
Sound::PlayMusic("Assets/Sound/bgm.mp3");

// 3D
Sound::Play3DSound("Assets/Sound/gunshot.wav", muzzlePos);

// 종료
Sound::Shutdown();
*/