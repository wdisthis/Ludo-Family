#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include "player.hpp"

struct LudoLogEntry {
    std::string message;
    Color color;
};

enum GameScreenState {
    SCREEN_MENU,
    SCREEN_GAMEPLAY,
    SCREEN_SETTINGS,
    SCREEN_WIN_SCREEN
};

enum GameplaySubState {
    SUBSTATE_TURN_START,
    SUBSTATE_WAIT_FOR_ROLL,
    SUBSTATE_DICE_ROLLING,
    SUBSTATE_EVALUATE_MOVES,
    SUBSTATE_WAIT_FOR_HUMAN_MOVE,
    SUBSTATE_PIECE_MOVING,
    SUBSTATE_POST_MOVE_CHECK,
    SUBSTATE_PIECE_REWINDING,
    SUBSTATE_TURN_END
};

struct PlayerSetup {
    std::string name;
    std::string type; // "human", "ai-medium", "none"
};

class LudoGame {
public:
    static GameScreenState screenState;
    static GameScreenState prevScreenState;
    static GameplaySubState subState;
    static std::string gameMode; // "vs-ai", "local"
    
    static std::vector<LudoPlayer> players;
    static int activePlayerIndex;
    static int currentRollValue;
    static std::vector<int> winnersList;
    
    // Match statistics
    static int capturesCount;
    static int rollsCount;
    
    // Logs console messages
    static std::vector<LudoLogEntry> matchLogs;
    
    // Setup configs
    static PlayerSetup playerConfigs[4];
    
    // UI control variables
    static bool isSoundMenuTriggered;
    static float aiSpeedCoeff; // 1.0f = normal, 2.0f = fast
    static int textCursorTick;
    static bool isTextBoxActive[4];
    
    // Timer & animation states
    static float diceRollTimer;
    static float stateTimer;
    static int selectedPieceId;
    
    // Moving animation variables
    static int movingPieceId;
    static int movingRemainingSteps;
    static float movingProgress;
    static bool isEnteringFromBase;
    
    // Rewinding variables
    static int rewindPlayerId;
    static int rewindPieceId;
    static float rewindProgress;
    static bool isBonusTurnGranted;

    // Scaling variables for high-definition rendering
    static float scaleFactor;
    static float offsetX;
    static float offsetY;

    // Core methods
    static void init();
    static void start(const std::string& mode);
    static void restart();
    static void update(float deltaTime);
    static void draw();
    
    static void logMessage(const std::string& msg, Color color = Color{203, 213, 225, 255});
    static Color getPlayerColor(int playerId);
    static void nextTurn();
    static Vector2 getMousePosition();
};
