#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include "player.hpp"
#include "piece.hpp"

struct LudoEffectParticle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float alpha;
    float size;
    float lifeTime;
};

struct LudoRingEffect {
    Vector2 position;
    float radius;
    float maxRadius;
    float alpha;
    Color color;
};

class LudoBoard {
private:
    static std::vector<LudoEffectParticle> particles;
    static std::vector<LudoRingEffect> ringEffects;

public:
    static Texture2D pieceTextures[4]; // green, yellow, blue, red
    static Texture2D diceTextures[6];  // dice-1 to dice-6
    static Texture2D shadowTexture;
    static Texture2D boardBgTexture;
    static bool isLoaded;
    static int currentBoardTextureSize;
    static void init();
    static void close();
    static void update(float deltaTime);
    
    // Core drawing functions
    static void draw(const std::vector<LudoPlayer>& players, int activePlayerId, const std::vector<int>& clickableTokenIds, int selectedTokenId = -1);
    
    // Draw the board backgrounds programmatically
    static void drawBoardBackground(float startX, float startY, float boardSize);
    
    // Trigger animations
    static void triggerCaptureEffect(float gx, float gy);
    static void triggerSafeEffect(float gx, float gy);
    
    // Helper to get screen position from grid coordinates
    static Vector2 getScreenPos(float startX, float startY, float boardSize, int gx, int gy);
};
