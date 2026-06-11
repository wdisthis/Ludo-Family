#pragma once
#include "raylib.h"
#include <string>
#include <map>

class LudoAudio {
private:
    static Music bgm;
    static std::map<std::string, Sound> sounds;
    static float bgmVolume;
    static float sfxVolume;
    static bool audioEnabled;
    static bool isBgmPlaying;

public:
    static void init();
    static void close();
    static void update(); // Call in main update loop
    
    static void playSFX(const std::string& name);
    static void startBGM();
    static void stopBGM();
    static void toggleAudio();
    static bool isAudioEnabled() { return audioEnabled; }
    
    static void setBGMVolume(float volume); // 0.0 to 1.0
    static void setSFXVolume(float volume); // 0.0 to 1.0
    static float getBGMVolume() { return bgmVolume; }
    static float getSFXVolume() { return sfxVolume; }
};
