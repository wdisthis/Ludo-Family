#include "audio.hpp"
#include <iostream>
#include <vector>

Music LudoAudio::bgm;
std::map<std::string, Sound> LudoAudio::sounds;
float LudoAudio::bgmVolume = 0.5f;
float LudoAudio::sfxVolume = 0.7f;
bool LudoAudio::audioEnabled = true;
bool LudoAudio::isBgmPlaying = false;

void LudoAudio::init() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        std::cerr << "Audio device could not be initialized!" << std::endl;
        return;
    }
    
    // Load Music Stream
    bgm = LoadMusicStream("assets/audio/bgm.mp3");
    bgm.looping = true;
    SetMusicVolume(bgm, bgmVolume);
    
    // Load Sound Effects
    std::vector<std::string> sfxNames = {
        "dice-roll", "piece-move", "piece-capture", "piece-enter-home",
        "piece-finish", "piece-safe", "turn-change", "six-rolled", "win", "lose"
    };
    
    for (const auto& name : sfxNames) {
        std::string path = "assets/audio/" + name + ".mp3";
        Sound snd = LoadSound(path.c_str());
        SetSoundVolume(snd, sfxVolume);
        sounds[name] = snd;
    }
    
    startBGM();
}

void LudoAudio::close() {
    UnloadMusicStream(bgm);
    for (auto& pair : sounds) {
        UnloadSound(pair.second);
    }
    sounds.clear();
    CloseAudioDevice();
}

void LudoAudio::update() {
    if (audioEnabled && isBgmPlaying) {
        UpdateMusicStream(bgm);
    }
}

void LudoAudio::playSFX(const std::string& name) {
    if (!audioEnabled) return;
    
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        PlaySound(it->second);
    }
}

void LudoAudio::startBGM() {
    if (!audioEnabled) return;
    PlayMusicStream(bgm);
    isBgmPlaying = true;
}

void LudoAudio::stopBGM() {
    StopMusicStream(bgm);
    isBgmPlaying = false;
}

void LudoAudio::toggleAudio() {
    audioEnabled = !audioEnabled;
    if (audioEnabled) {
        SetMusicVolume(bgm, bgmVolume);
        for (auto& pair : sounds) {
            SetSoundVolume(pair.second, sfxVolume);
        }
        startBGM();
    } else {
        stopBGM();
    }
}

void LudoAudio::setBGMVolume(float volume) {
    bgmVolume = volume;
    if (audioEnabled) {
        SetMusicVolume(bgm, bgmVolume);
    }
}

void LudoAudio::setSFXVolume(float volume) {
    sfxVolume = volume;
    if (audioEnabled) {
        for (auto& pair : sounds) {
            SetSoundVolume(pair.second, sfxVolume);
        }
    }
}
