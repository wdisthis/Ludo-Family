#include "game.hpp"
#include "board.hpp"
#include "rules.hpp"
#include "path.hpp"
#include "ai.hpp"
#include "audio.hpp"
#include "ui.hpp"
#include "rlgl.h"
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cmath>

static void drawSparkleStar(float cx, float cy, float size) {
    float r = size * 0.5f;
    float ir = r * 0.22f; // inner radius
    
    Vector2 pts[8] = {
        { cx, cy - r },
        { cx + ir, cy - ir },
        { cx + r, cy },
        { cx + ir, cy + ir },
        { cx, cy + r },
        { cx - ir, cy + ir },
        { cx - r, cy },
        { cx - ir, cy - ir }
    };
    
    for (int i = 0; i < 8; i++) {
        DrawTriangle(Vector2{cx, cy}, pts[(i + 1) % 8], pts[i], WHITE);
    }
}

// Static allocations
GameScreenState LudoGame::screenState = SCREEN_MENU;
GameScreenState LudoGame::prevScreenState = SCREEN_MENU;
GameplaySubState LudoGame::subState = SUBSTATE_TURN_START;
std::string LudoGame::gameMode = "vs-ai";
std::vector<LudoPlayer> LudoGame::players;
int LudoGame::activePlayerIndex = 0;
int LudoGame::currentRollValue = 0;
std::vector<int> LudoGame::winnersList;
int LudoGame::capturesCount = 0;
int LudoGame::rollsCount = 0;
std::vector<LudoLogEntry> LudoGame::matchLogs;
PlayerSetup LudoGame::playerConfigs[4];
bool LudoGame::isSoundMenuTriggered = false;
float LudoGame::aiSpeedCoeff = 1.0f;
int LudoGame::textCursorTick = 0;
bool LudoGame::isTextBoxActive[4] = {false, false, false, false};
float LudoGame::diceRollTimer = 0.0f;
float LudoGame::stateTimer = 0.0f;
int LudoGame::selectedPieceId = -1;
int LudoGame::movingPieceId = -1;
int LudoGame::movingRemainingSteps = 0;
float LudoGame::movingProgress = 0.0f;
bool LudoGame::isEnteringFromBase = false;
int LudoGame::rewindPlayerId = -1;
int LudoGame::rewindPieceId = -1;
float LudoGame::rewindProgress = 0.0f;
bool LudoGame::isBonusTurnGranted = false;

float LudoGame::scaleFactor = 1.0f;
float LudoGame::offsetX = 0.0f;
float LudoGame::offsetY = 0.0f;

void LudoGame::init() {
    std::srand(std::time(nullptr));
    
    // Set default player configurations
    playerConfigs[0] = {"Green", "ai-medium"};
    playerConfigs[1] = {"Yellow", "ai-medium"};
    playerConfigs[2] = {"Blue", "ai-medium"};
    playerConfigs[3] = {"Red", "human"};
    
    LudoBoard::init();
    LudoAudio::init();
    LudoUIWidget::initFonts();
}

void LudoGame::start(const std::string& mode) {
    gameMode = mode;
    players.clear();
    winnersList.clear();
    capturesCount = 0;
    rollsCount = 0;
    matchLogs.clear();
    
    std::string colors[4] = {"green", "yellow", "blue", "red"};
    for (int i = 0; i < 4; i++) {
        players.push_back(LudoPlayer(i, playerConfigs[i].name, playerConfigs[i].type, colors[i]));
    }
    
    logMessage("Game started! Good luck!");
    LudoAudio::stopBGM(); // Matches game.js
    
    screenState = SCREEN_GAMEPLAY;
    activePlayerIndex = 3; // Red starts (Player 3)
    subState = SUBSTATE_TURN_END; // Forces nextTurn() call
}

void LudoGame::restart() {
    screenState = SCREEN_MENU;
    LudoAudio::startBGM();
}

void LudoGame::logMessage(const std::string& msg, Color color) {
    if (!matchLogs.empty() && matchLogs.back().message == msg) {
        return; // Don't spam duplicate consecutive messages
    }
    matchLogs.push_back(LudoLogEntry{msg, color});
    // Keep only last 10 messages for layout
    if (matchLogs.size() > 10) {
        matchLogs.erase(matchLogs.begin());
    }
    std::cout << "[LOG] " << msg << std::endl;
}

Color LudoGame::getPlayerColor(int playerId) {
    switch (playerId) {
        case 0: return Color{34, 197, 94, 255};   // Green
        case 1: return Color{245, 158, 11, 255};  // Yellow
        case 2: return Color{37, 99, 235, 255};   // Blue
        case 3: return Color{239, 68, 68, 255};   // Red
        default: return Color{203, 213, 225, 255}; // Slate / Off-white
    }
}

void LudoGame::nextTurn() {
    if (screenState != SCREEN_GAMEPLAY) return;
    
    int iterations = 0;
    do {
        activePlayerIndex = (activePlayerIndex + 1) % 4;
        iterations++;
    } while ((players[activePlayerIndex].type == "none" || players[activePlayerIndex].isFinished()) && iterations < 5);
    
    currentRollValue = 0;
    selectedPieceId = -1;
    subState = SUBSTATE_TURN_START;
    stateTimer = 0.0f;
    
    const auto& activePlayer = players[activePlayerIndex];
    
    std::string colorCaps = activePlayer.color;
    colorCaps[0] = toupper(colorCaps[0]);
    if (activePlayer.type == "human") {
        logMessage("Your turn, " + activePlayer.name + "!", getPlayerColor(activePlayer.id));
    } else {
        logMessage(activePlayer.name + " is preparing to roll...", getPlayerColor(activePlayer.id));
    }
}

void LudoGame::update(float deltaTime) {
    // Dynamically calculate scaling offsets relative to the window size
    float W = (float)GetScreenWidth();
    float H = (float)GetScreenHeight();
    
    if (screenState == SCREEN_MENU) {
        scaleFactor = std::min(W / 1100.0f, H / 650.0f);
        offsetX = (W - 1100.0f * scaleFactor) * 0.5f;
        offsetY = (H - 650.0f * scaleFactor) * 0.5f;
    } else {
        // Board fits the screen height exactly
        scaleFactor = H / 600.0f;
        offsetX = -20.0f * scaleFactor;
        offsetY = -25.0f * scaleFactor;
    }

    LudoAudio::update();
    LudoBoard::update(deltaTime);
    
    // Manage typing blinking cursor
    textCursorTick++;
    if (textCursorTick > 60) textCursorTick = 0;
    
    // Process according to Screen State
    if (screenState == SCREEN_MENU) {
        // Simple VS AI / Pass & Play quick selection
        // Start game button trigger is handled in drawing click checks
        return;
    }
    
    if (screenState == SCREEN_SETTINGS || screenState == SCREEN_WIN_SCREEN) {
        // Pause loop operations
        return;
    }
    
    if (screenState == SCREEN_GAMEPLAY) {
        const auto& activePlayer = players[activePlayerIndex];
        
        switch (subState) {
            case SUBSTATE_TURN_START: {
                if (activePlayer.type == "none" || activePlayer.isFinished()) {
                    subState = SUBSTATE_TURN_END;
                } else if (activePlayer.type == "human") {
                    subState = SUBSTATE_WAIT_FOR_ROLL;
                } else {
                    // AI Delay before rolling
                    stateTimer += deltaTime;
                    float delayLimit = (aiSpeedCoeff > 1.5f) ? 0.25f : 0.8f;
                    if (stateTimer >= delayLimit) {
                        subState = SUBSTATE_DICE_ROLLING;
                        diceRollTimer = 0.0f;
                        LudoAudio::playSFX("dice-roll");
                    }
                }
                break;
            }
            case SUBSTATE_WAIT_FOR_ROLL: {
                // Waits for button click in UI or direct keyboard spacebar roll
                if (IsKeyPressed(KEY_SPACE)) {
                    subState = SUBSTATE_DICE_ROLLING;
                    diceRollTimer = 0.0f;
                    LudoAudio::playSFX("dice-roll");
                }
                break;
            }
            case SUBSTATE_DICE_ROLLING: {
                diceRollTimer += deltaTime;
                if (diceRollTimer >= 1.0f) {
                    currentRollValue = 1 + (rand() % 6);
                    rollsCount++;
                    logMessage(activePlayer.name + " rolled a " + std::to_string(currentRollValue), getPlayerColor(activePlayer.id));
                    
                    if (currentRollValue == 6) {
                       LudoAudio::playSFX("six-rolled");
                    }
                    subState = SUBSTATE_EVALUATE_MOVES;
                    stateTimer = 0.0f;
                }
                break;
            }
            case SUBSTATE_EVALUATE_MOVES: {
                // Find eligible moves
                std::vector<int> eligibleTokenIds;
                for (int i = 0; i < 4; i++) {
                    if (LudoRules::canTokenMove(activePlayer.id, activePlayer.tokens[i], currentRollValue)) {
                        eligibleTokenIds.push_back(i);
                    }
                }
                
                if (eligibleTokenIds.empty()) {
                    if (stateTimer == 0.0f) {
                        logMessage("No moves for " + activePlayer.name, getPlayerColor(activePlayer.id));
                    }
                    stateTimer += deltaTime;
                    if (stateTimer >= 1.0f) {
                        subState = SUBSTATE_TURN_END;
                    }
                } else {
                    if (activePlayer.type == "human") {
                        subState = SUBSTATE_WAIT_FOR_HUMAN_MOVE;
                    } else {
                        // AI delay before making a choice
                        stateTimer += deltaTime;
                        float delayLimit = (aiSpeedCoeff > 1.5f) ? 0.2f : 0.6f;
                        if (stateTimer >= delayLimit) {
                            int bestMove = LudoAI::selectMove(activePlayer, currentRollValue, players);
                            if (bestMove != -1) {
                                selectedPieceId = bestMove;
                                movingPieceId = bestMove;
                                isEnteringFromBase = (activePlayer.tokens[bestMove].step == -1);
                                movingRemainingSteps = isEnteringFromBase ? 1 : currentRollValue;
                                movingProgress = 0.0f;
                                subState = SUBSTATE_PIECE_MOVING;
                            } else {
                                subState = SUBSTATE_TURN_END;
                            }
                        }
                    }
                }
                break;
            }
            case SUBSTATE_WAIT_FOR_HUMAN_MOVE: {
                // Wait for mouse clicks on the board cells containing eligible tokens
                if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    Vector2 mousePos = LudoGame::getMousePosition();
                    // Board is rendered at offset x=250, y=25, size=600
                    float CS = 40.0f;
                    float bx = mousePos.x - 250.0f;
                    float by = mousePos.y - 25.0f;
                    
                    if (bx >= 0.0f && bx < 600.0f && by >= 0.0f && by < 600.0f) {
                        int clickX = (int)(bx / CS);
                        int clickY = (int)(by / CS);
                        
                        // Check if active player has an eligible piece at these coordinates
                        for (int i = 0; i < 4; i++) {
                            if (LudoRules::canTokenMove(activePlayer.id, activePlayer.tokens[i], currentRollValue)) {
                                std::vector<int> coords = activePlayer.tokens[i].getCoordinates();
                                bool isClickMatched = false;
                                if (activePlayer.tokens[i].step == -1) {
                                    // Base pieces are drawn centered at grid corners (cellTL)
                                    // Check distance between virtual mouse (bx, by) and base coordinate (coords[0]*CS, coords[1]*CS)
                                    float dx = bx - coords[0] * CS;
                                    float dy = by - coords[1] * CS;
                                    float dist = sqrtf(dx*dx + dy*dy);
                                    if (dist <= 20.0f) {
                                        isClickMatched = true;
                                    }
                                } else {
                                    // Track pieces are matched by grid cell
                                    if (coords[0] == clickX && coords[1] == clickY) {
                                        isClickMatched = true;
                                    }
                                }
                                
                                if (isClickMatched) {
                                    selectedPieceId = i;
                                    movingPieceId = i;
                                    isEnteringFromBase = (activePlayer.tokens[i].step == -1);
                                    movingRemainingSteps = isEnteringFromBase ? 1 : currentRollValue;
                                    movingProgress = 0.0f;
                                    subState = SUBSTATE_PIECE_MOVING;
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
            }
            case SUBSTATE_PIECE_MOVING: {
                movingProgress += deltaTime / ((aiSpeedCoeff > 1.5f) ? 0.09f : 0.22f);
                if (movingProgress >= 1.0f) {
                    movingProgress = 0.0f;
                    
                    auto& piece = players[activePlayerIndex].tokens[movingPieceId];
                    if (isEnteringFromBase) {
                        piece.step = 0; // Exits to start index
                        LudoAudio::playSFX("piece-enter-home");
                        movingRemainingSteps = 0;
                    } else {
                        piece.step++;
                        LudoAudio::playSFX("piece-move");
                        movingRemainingSteps--;
                    }
                    
                    if (movingRemainingSteps == 0) {
                        subState = SUBSTATE_POST_MOVE_CHECK;
                    }
                }
                break;
            }
            case SUBSTATE_POST_MOVE_CHECK: {
                auto& piece = players[activePlayerIndex].tokens[movingPieceId];
                std::vector<int> coords = piece.getCoordinates();
                
                   if (piece.step == 56) {
                       LudoAudio::playSFX("piece-finish");
                       LudoBoard::triggerSafeEffect(coords[0], coords[1]);
                       logMessage(activePlayer.name + " piece " + std::to_string(piece.id + 1) + " finished!", getPlayerColor(activePlayer.id));
                       
                       if (activePlayer.isFinished()) {
                           // Check if player already ranked
                           if (std::find(winnersList.begin(), winnersList.end(), activePlayerIndex) == winnersList.end()) {
                               winnersList.push_back(activePlayerIndex);
                               logMessage("🏆 " + activePlayer.name + " finished all tokens! Ranked #" + std::to_string(winnersList.size()), getPlayerColor(activePlayer.id));
                           }
                        
                        // Count active participants
                        int participants = 0;
                        for (int i = 0; i < 4; i++) {
                            if (players[i].type != "none") participants++;
                        }
                        
                        // Game ends if only 1 active player is left playing
                        if ((int)winnersList.size() >= participants - 1) {
                            // Rank remaining players
                            for (int i = 0; i < 4; i++) {
                                if (players[i].type != "none" && std::find(winnersList.begin(), winnersList.end(), i) == winnersList.end()) {
                                    winnersList.push_back(i);
                                }
                            }
                            
                            screenState = SCREEN_WIN_SCREEN;
                            LudoAudio::startBGM();
                            LudoAudio::playSFX("win");
                            return;
                        }
                    }
                } else if (LudoRules::isSafeCell(coords[0], coords[1])) {
                    LudoAudio::playSFX("piece-safe");
                    LudoBoard::triggerSafeEffect(coords[0], coords[1]);
                } else {
                    // Check captures
                    bool captured = false;
                    for (auto& otherPlayer : players) {
                        if (otherPlayer.id == activePlayer.id || otherPlayer.type == "none" || otherPlayer.isFinished()) {
                            continue;
                        }
                        
                        for (auto& otherToken : otherPlayer.tokens) {
                            if (otherToken.step == -1 || otherToken.step == 56) continue;
                            
                            std::vector<int> otherCoords = otherToken.getCoordinates();
                            if (otherCoords[0] == coords[0] && otherCoords[1] == coords[1]) {
                                captured = true;
                                rewindPlayerId = otherPlayer.id;
                                rewindPieceId = otherToken.id;
                                rewindProgress = 0.0f;
                                
                                logMessage(activePlayer.name + " captured " + otherPlayer.name + "!", getPlayerColor(activePlayer.id));
                                LudoBoard::triggerCaptureEffect(coords[0], coords[1]);
                                LudoAudio::playSFX("piece-capture");
                                
                                isBonusTurnGranted = (currentRollValue == 6);
                                subState = SUBSTATE_PIECE_REWINDING;
                                return;
                            }
                        }
                    }
                }
                
                // Set turn options
                isBonusTurnGranted = (currentRollValue == 6);
                subState = SUBSTATE_TURN_END;
                break;
            }
            case SUBSTATE_PIECE_REWINDING: {
                rewindProgress += deltaTime / 0.08f; // fast rewinding pace
                if (rewindProgress >= 1.0f) {
                    rewindProgress = 0.0f;
                    
                    auto& rewPiece = players[rewindPlayerId].tokens[rewindPieceId];
                    rewPiece.step--;
                    LudoAudio::playSFX("piece-move");
                    
                    if (rewPiece.step <= -1) {
                        rewPiece.resetToBase();
                        LudoAudio::playSFX("piece-safe");
                        
                        capturesCount++;
                        subState = SUBSTATE_TURN_END;
                    }
                }
                break;
            }
            case SUBSTATE_TURN_END: {
                if (isBonusTurnGranted && !players[activePlayerIndex].isFinished()) {
                    logMessage("Rolled 6! Extra turn for " + activePlayer.name, getPlayerColor(activePlayer.id));
                    subState = SUBSTATE_TURN_START;
                } else {
                    nextTurn();
                }
                break;
            }
        }
    }
}

void LudoGame::draw() {
    float W = (float)GetScreenWidth();
    float H = (float)GetScreenHeight();
    
    // Define all scale factor variations
    float defaultScale = std::min(W / 1100.0f, H / 650.0f);
    float defaultOffsetX = (W - 1100.0f * defaultScale) * 0.5f;
    float defaultOffsetY = (H - 650.0f * defaultScale) * 0.5f;
    
    float boardScale = H / 600.0f;
    float boardX = (W - H) * 0.5f;
    float boardOffsetX = boardX - 250.0f * boardScale;
    float boardOffsetY = -25.0f * boardScale;
    
    float panelSpace = boardX; // Space on left and right of the board
    float leftScale = boardScale;
    float leftOffsetX = boardOffsetX;
    float leftOffsetY = boardOffsetY;
    
    float rightScale = boardScale;
    float rightOffsetX = boardOffsetX;
    float rightOffsetY = boardOffsetY;
    
    if (panelSpace > 0.0f) {
        // Center and scale the left/right panels identically in the side spaces
        leftScale = std::min(panelSpace / 250.0f, boardScale);
        float leftPanelX = (panelSpace - 250.0f * leftScale) * 0.5f;
        leftOffsetX = leftPanelX;
        leftOffsetY = (H - 650.0f * leftScale) * 0.5f;
        
        rightScale = leftScale; // Equal size
        float rightPanelX = (boardX + H) + (panelSpace - 250.0f * rightScale) * 0.5f;
        rightOffsetX = rightPanelX - 850.0f * rightScale;
        rightOffsetY = leftOffsetY;
    }

    // Set scaling parameters for drawing background and menu
    if (screenState == SCREEN_MENU) {
        scaleFactor = defaultScale;
        offsetX = defaultOffsetX;
        offsetY = defaultOffsetY;
    } else {
        scaleFactor = boardScale;
        offsetX = boardOffsetX;
        offsetY = boardOffsetY;
    }

    // Draw background
    // Premium background gradient decoration across the entire physical screen
    DrawRectangleGradientV(0, 0, W, H, Color{15, 23, 42, 255}, Color{30, 41, 59, 255}); // Slate 900 -> Slate 800
    
    // Draw ambient glows dynamically scaled
    DrawCircleGradient(offsetX + 200.0f * scaleFactor, offsetY + 100.0f * scaleFactor, 400.0f * scaleFactor, Color{34, 197, 94, 25}, Color{34, 197, 94, 0});
    DrawCircleGradient(offsetX + 900.0f * scaleFactor, offsetY + 500.0f * scaleFactor, 500.0f * scaleFactor, Color{59, 130, 246, 25}, Color{59, 130, 246, 0});

    if (screenState == SCREEN_MENU) {
        // Draw faint decorative boards in the top-left and top-right corners
        if (LudoBoard::isLoaded && LudoBoard::boardBgTexture.id != 0) {
            float bgBoardSize = 250.0f * scaleFactor;
            // Top-left
            DrawTexturePro(LudoBoard::boardBgTexture, 
                           Rectangle{0, 0, (float)LudoBoard::boardBgTexture.width, (float)LudoBoard::boardBgTexture.height}, 
                           Rectangle{offsetX + 50.0f * scaleFactor, offsetY + 50.0f * scaleFactor, bgBoardSize, bgBoardSize}, 
                           Vector2{0, 0}, 0.0f, ColorAlpha(WHITE, 0.06f));
            // Top-right
            DrawTexturePro(LudoBoard::boardBgTexture, 
                           Rectangle{0, 0, (float)LudoBoard::boardBgTexture.width, (float)LudoBoard::boardBgTexture.height}, 
                           Rectangle{offsetX + 800.0f * scaleFactor, offsetY + 50.0f * scaleFactor, bgBoardSize, bgBoardSize}, 
                           Vector2{0, 0}, 0.0f, ColorAlpha(WHITE, 0.06f));
        }
    }
    
    if (screenState == SCREEN_MENU) {
        float logoY = 50.0f;
        
        // Draw Title "LUDO FAMILY"
        LudoUIWidget::drawTextWithShadow("LUDO FAMILY", 550 - MeasureTextEx(LudoUIWidget::boldFont, "LUDO FAMILY", 44, 1.0f).x/2.0f, logoY + 30, 44, WHITE, LudoUIWidget::boldFont);
        
        // Mode Selection box label
        LudoUIWidget::drawTextWithShadow("Game Mode Selection", 250, logoY + 110, 15, WHITE, LudoUIWidget::boldFont);
        
        // 2 Mode buttons/cards (VS AI vs Local Pass & Play)
        Rectangle modeVsAI = {250, logoY + 135, 290, 70};
        Rectangle modeLocal = {560, logoY + 135, 290, 70};
        
        bool vsAIClicked = LudoUIWidget::modeButton(modeVsAI, "Vs Computer AI", "Play against smart AI players", (gameMode == "vs-ai"), 0);
        bool localClicked = LudoUIWidget::modeButton(modeLocal, "Local Pass & Play", "Play with family on one screen", (gameMode == "local"), 1);
            
        if (vsAIClicked) {
            gameMode = "vs-ai";
            playerConfigs[0].type = "ai-medium";
            playerConfigs[1].type = "ai-medium";
            playerConfigs[2].type = "ai-medium";
            playerConfigs[3].type = "human";
        }
        if (localClicked) {
            gameMode = "local";
            playerConfigs[0].type = "human";
            playerConfigs[1].type = "human";
            playerConfigs[2].type = "human";
            playerConfigs[3].type = "human";
        }
        
        // 4 Player Cards horizontally
        for (int i = 0; i < 4; i++) {
            float px = 80.0f + i * 245.0f;
            float py = logoY + 245.0f;
            
            Rectangle cardRec = { px, py, 200, 185 };
            LudoUIWidget::playerConfigCard(cardRec, i, playerConfigs[i].name, playerConfigs[i].type, isTextBoxActive[i], textCursorTick);
        }
        
        // START ADVENTURE pulse button (moved down to logoY + 470)
        Rectangle startBtnRec = {375, logoY + 470, 350, 45};
        float pulse = 1.0f + sinf(GetTime() * 4.0f) * 0.015f;
        Rectangle scaledStartBtnRec = startBtnRec;
        scaledStartBtnRec.width *= pulse;
        scaledStartBtnRec.height *= pulse;
        scaledStartBtnRec.x -= (scaledStartBtnRec.width - startBtnRec.width) / 2.0f;
        scaledStartBtnRec.y -= (scaledStartBtnRec.height - startBtnRec.height) / 2.0f;
        
        bool startClicked = LudoUIWidget::button(scaledStartBtnRec, "START ADVENTURE", Color{235, 87, 87, 255}, Color{220, 75, 75, 255}, WHITE, 0.2f);
        if (startClicked) {
            // Check that at least two players are active
            int activeCnt = 0;
            for (int i = 0; i < 4; i++) {
                if (playerConfigs[i].type != "none") activeCnt++;
            }
            
            if (activeCnt < 2) {
                playerConfigs[0].type = "ai-medium";
            }
            start(gameMode);
        }

        // SETTINGS button below Start Adventure (matching size)
        Rectangle settingsBtnRec = {375, logoY + 525, 350, 45};
        bool settingsClicked = LudoUIWidget::button(settingsBtnRec, "SETTINGS", Color{71, 85, 105, 255}, Color{100, 116, 139, 255}, WHITE, 0.2f);
        if (settingsClicked) {
            prevScreenState = SCREEN_MENU;
            screenState = SCREEN_SETTINGS;
        }
        
        // Sparkle Star Decoration in bottom right
        drawSparkleStar(offsetX + 1030.0f * scaleFactor, offsetY + 590.0f * scaleFactor, 30.0f * scaleFactor);
        
        return;
    }
    
    // Draw Main Gameplay layout
    // Board is drawn at startX = offsetX + 20*scale, startY = offsetY + 25*scale, size = 600*scale.
    std::vector<int> clickableTokenIds;
    if (subState == SUBSTATE_WAIT_FOR_HUMAN_MOVE) {
        const auto& activePlayer = players[activePlayerIndex];
        for (int i = 0; i < 4; i++) {
            if (LudoRules::canTokenMove(activePlayer.id, activePlayer.tokens[i], currentRollValue)) {
                clickableTokenIds.push_back(i);
            }
        }
    }
    
    // Draw the board directly (it incorporates scaleFactor and offsetX/offsetY inside LudoBoard::draw)
    LudoBoard::draw(players, activePlayerIndex, clickableTokenIds, selectedPieceId);
    
    // ----------------------------------------------------
    // ----------------------------------------------------
    // Switch to left panel scaling for drawing scorecard elements
    // ----------------------------------------------------
    scaleFactor = leftScale;
    offsetX = leftOffsetX;
    offsetY = leftOffsetY;

    // Left panel: Players scorecard (Shifted to x=20, using board scale and offset)
    // ----------------------------------------------------
    LudoUIWidget::drawTextWithShadow("Players", 20, 30, 20, WHITE, LudoUIWidget::boldFont);
    
    Color playerCardColors[4] = {
        Color{34, 197, 94, 255},  // Green
        Color{245, 158, 11, 255}, // Yellow
        Color{37, 99, 235, 255},  // Blue
        Color{239, 68, 68, 255}   // Red
    };
    
    for (int i = 0; i < 4; i++) {
        const auto& p = players[i];
        float cardY = 65 + i * 72;
        Rectangle cardRec = {
            offsetX + 20.0f * scaleFactor,
            offsetY + cardY * scaleFactor,
            210.0f * scaleFactor,
            62.0f * scaleFactor
        };
        
        Color bg = Color{15, 23, 42, 200}; // Slate 900
        Color border = Color{51, 65, 85, 255}; // Slate 700
        
        if (p.type == "none") {
            // Inactive player greyed out
            DrawRectangleRounded(cardRec, 0.15f, 4, Color{30, 41, 59, 100});
            DrawRectangleRoundedLines(cardRec, 0.15f, 4, 1.0f * scaleFactor, Color{51, 65, 85, 100});
            DrawCircle(offsetX + 45.0f * scaleFactor, offsetY + (cardY + 31.0f) * scaleFactor, 10.0f * scaleFactor, Color{100, 116, 139, 255});
            LudoUIWidget::drawTextWithShadow(p.name, 70, cardY + 12, 14, Color{100, 116, 139, 255}, LudoUIWidget::boldFont);
            LudoUIWidget::drawTextWithShadow("Inactive", 70, cardY + 31, 12, Color{100, 116, 139, 255}, LudoUIWidget::mainFont);
            continue;
        }
        
        bool isActive = (i == activePlayerIndex);
        if (isActive) {
            border = playerCardColors[i]; // Highlights card with player's color
            bg = ColorAlpha(playerCardColors[i], 0.15f);
        }
        
        DrawRectangleRounded(cardRec, 0.15f, 4, bg);
        DrawRectangleRoundedLines(cardRec, 0.15f, 4, 2.0f * scaleFactor, border);
        
        // Color avatar dot
        DrawCircle(offsetX + 45.0f * scaleFactor, offsetY + (cardY + 31.0f) * scaleFactor, 10.0f * scaleFactor, playerCardColors[i]);
        DrawCircle(offsetX + 45.0f * scaleFactor, offsetY + (cardY + 31.0f) * scaleFactor, 6.0f * scaleFactor, WHITE);
        
        // Name and Status
        LudoUIWidget::drawTextWithShadow(p.name, 70, cardY + 12, 14, WHITE, LudoUIWidget::boldFont);
        
        std::string status = "Waiting...";
        if (p.isFinished()) status = "Finished!";
        else if (isActive) {
            if (subState == SUBSTATE_WAIT_FOR_ROLL || subState == SUBSTATE_TURN_START) status = "Rolling...";
            else if (subState == SUBSTATE_DICE_ROLLING) status = "Rolling...";
            else status = "Moving piece...";
        }
        
        LudoUIWidget::drawTextWithShadow(status, 70, cardY + 31, 12, Color{148, 163, 184, 255}, LudoUIWidget::mainFont);
        
        // Token progress counter
        std::string countStr = std::to_string(p.getActiveTokensCount()) + "/4";
        if (p.isFinished()) countStr = "Done";
        LudoUIWidget::drawTextWithShadow(countStr, 170, cardY + 23, 13, WHITE, LudoUIWidget::boldFont);
    }
    
    bool isLeftPlayer = (activePlayerIndex == 0 || activePlayerIndex == 3);
    
    // Draw Left Dice Panel permanently
    LudoUIWidget::drawTextWithShadow("Roll Dice", 20, 360, 16, WHITE, LudoUIWidget::boldFont);
    Rectangle leftDiceRec = {
        offsetX + 20.0f * scaleFactor,
        offsetY + 390.0f * scaleFactor,
        210.0f * scaleFactor,
        235.0f * scaleFactor
    };
    DrawRectangleRounded(leftDiceRec, 0.15f, 4, Color{15, 23, 42, 200});
    DrawRectangleRoundedLines(leftDiceRec, 0.15f, 4, 1.0f * scaleFactor, Color{51, 65, 85, 255});
    
    // Inside left dice container:
    DrawTextEx(LudoUIWidget::mainFont, "CURRENT TURN", Vector2{roundf(offsetX + 30.0f * scaleFactor), roundf(offsetY + 400.0f * scaleFactor)}, 10.0f * scaleFactor, 1.0f, Color{148, 163, 184, 255});
    
    if (isLeftPlayer) {
        std::string colorName = players[activePlayerIndex].color;
        colorName[0] = toupper(colorName[0]);
        LudoUIWidget::drawTextWithShadow(colorName + " Player", 30, 415, 14, playerCardColors[activePlayerIndex], LudoUIWidget::boldFont);
    } else {
        LudoUIWidget::drawTextWithShadow("Waiting...", 30, 415, 14, Color{100, 116, 139, 255}, LudoUIWidget::boldFont);
    }
    
    // Draw Dice Face
    int displayFaceLeft = 1;
    if (isLeftPlayer) {
        displayFaceLeft = currentRollValue;
        if (subState == SUBSTATE_DICE_ROLLING) {
            displayFaceLeft = 1 + (rand() % 6);
        }
        if (displayFaceLeft < 1) displayFaceLeft = 1;
    }
    
    Vector2 dicePosLeft = {20 + 105.0f, 390 + 100.0f};
    bool diceDrawnLeft = false;
    if (LudoBoard::isLoaded && LudoBoard::diceTextures[displayFaceLeft - 1].id != 0) {
        float diceSize = 55.0f;
        Color diceTint = isLeftPlayer ? WHITE : ColorAlpha(WHITE, 0.4f);
        if (isLeftPlayer && subState == SUBSTATE_DICE_ROLLING) {
            float shakeAngle = sinf(GetTime() * 40.0f) * 15.0f;
            diceSize = 55.0f + sinf(GetTime() * 30.0f) * 3.0f;
            Rectangle source = {0.0f, 0.0f, (float)LudoBoard::diceTextures[displayFaceLeft - 1].width, (float)LudoBoard::diceTextures[displayFaceLeft - 1].height};
            Rectangle dest = {
                offsetX + dicePosLeft.x * scaleFactor,
                offsetY + dicePosLeft.y * scaleFactor,
                diceSize * scaleFactor,
                diceSize * scaleFactor
            };
            DrawTexturePro(LudoBoard::diceTextures[displayFaceLeft - 1], source, dest, Vector2{(diceSize * scaleFactor)/2.0f, (diceSize * scaleFactor)/2.0f}, shakeAngle, diceTint);
        } else {
            Rectangle source = {0.0f, 0.0f, (float)LudoBoard::diceTextures[displayFaceLeft - 1].width, (float)LudoBoard::diceTextures[displayFaceLeft - 1].height};
            Rectangle dest = {
                offsetX + (dicePosLeft.x - 27.5f) * scaleFactor,
                offsetY + (dicePosLeft.y - 27.5f) * scaleFactor,
                55.0f * scaleFactor,
                55.0f * scaleFactor
            };
            DrawTexturePro(LudoBoard::diceTextures[displayFaceLeft - 1], source, dest, Vector2{0.0f, 0.0f}, 0.0f, diceTint);
        }
        diceDrawnLeft = true;
    }
    
    if (!diceDrawnLeft) {
        Color slateDark = Color{30, 41, 59, 255};
        Color diceBg = isLeftPlayer ? WHITE : ColorAlpha(WHITE, 0.4f);
        Rectangle fallbackDiceRec = {
            offsetX + (dicePosLeft.x - 22.5f) * scaleFactor,
            offsetY + (dicePosLeft.y - 22.5f) * scaleFactor,
            45.0f * scaleFactor,
            45.0f * scaleFactor
        };
        DrawRectangleRounded(fallbackDiceRec, 0.2f, 4, diceBg);
        DrawRectangleRoundedLines(fallbackDiceRec, 0.2f, 4, 2.0f * scaleFactor, slateDark);
        if (isLeftPlayer) {
            std::string numStr = std::to_string(displayFaceLeft);
            float fs = 18.0f * scaleFactor;
            Vector2 textSz = MeasureTextEx(LudoUIWidget::boldFont, numStr.c_str(), fs, 1.0f);
            DrawTextEx(LudoUIWidget::boldFont, numStr.c_str(), Vector2{
                roundf(offsetX + dicePosLeft.x * scaleFactor - textSz.x/2.0f), 
                roundf(offsetY + dicePosLeft.y * scaleFactor - textSz.y/2.0f)
            }, fs, 1.0f, slateDark);
        }
    }
    
    // Roll button on the left
    Rectangle leftRollBtnRec = {35, 565, 180, 35};
    bool leftCanRoll = (players[activePlayerIndex].type == "human" && subState == SUBSTATE_WAIT_FOR_ROLL && isLeftPlayer);
    
    Color lrBaseCol = leftCanRoll ? Color{239, 68, 68, 255} : Color{71, 85, 105, 255};
    Color lrHoverCol = leftCanRoll ? Color{220, 38, 38, 255} : Color{71, 85, 105, 255};
    
    std::string leftRollBtnText = "Roll Dice";
    if (!isLeftPlayer) {
        leftRollBtnText = "Roll on Right";
    } else if (subState == SUBSTATE_DICE_ROLLING) {
        leftRollBtnText = "Rolling...";
    } else if (currentRollValue > 0) {
        leftRollBtnText = "Rolled " + std::to_string(currentRollValue);
    }
    
    bool leftRollClicked = LudoUIWidget::button(leftRollBtnRec, leftRollBtnText, lrBaseCol, lrHoverCol, WHITE, 0.15f);
    if (leftCanRoll && leftRollClicked) {
        subState = SUBSTATE_DICE_ROLLING;
        diceRollTimer = 0.0f;
        LudoAudio::playSFX("dice-roll");
    }
    
    // ----------------------------------------------------
    // Switch to panel scaling for drawing right panel elements
    // ----------------------------------------------------
    scaleFactor = rightScale;
    offsetX = rightOffsetX;
    offsetY = rightOffsetY;
    
    // Settings, Restart buttons (now positioned at the top right of the right panel)
    Rectangle btnSettings = {1050, 20, 30, 30};
    Rectangle btnRestart = {1015, 20, 30, 30};
    
    if (LudoUIWidget::iconButton(btnRestart, "R")) {
        restart();
    }
    if (LudoUIWidget::iconButton(btnSettings, "S")) {
        prevScreenState = SCREEN_GAMEPLAY;
        screenState = SCREEN_SETTINGS;
    }
    
    // Right action panel: Dice roll area & logs (x=870 to 1080)
    LudoUIWidget::drawTextWithShadow("Turn Actions", 870, 30, 20, WHITE, LudoUIWidget::boldFont);
    
    // Turn banner
    Rectangle turnBannerRec = {
        offsetX + 870.0f * scaleFactor,
        offsetY + 65.0f * scaleFactor,
        210.0f * scaleFactor,
        50.0f * scaleFactor
    };
    DrawRectangleRounded(turnBannerRec, 0.15f, 4, Color{30, 41, 59, 255});
    DrawTextEx(LudoUIWidget::mainFont, "CURRENT TURN", Vector2{roundf(offsetX + 880.0f * scaleFactor), roundf(offsetY + 71.0f * scaleFactor)}, 10.0f * scaleFactor, 1.0f, Color{148, 163, 184, 255});
    
    if (!isLeftPlayer) {
        std::string colorName = players[activePlayerIndex].color;
        colorName[0] = toupper(colorName[0]);
        LudoUIWidget::drawTextWithShadow(colorName + " Player", 880, 85, 17, playerCardColors[activePlayerIndex], LudoUIWidget::boldFont);
    } else {
        LudoUIWidget::drawTextWithShadow("Waiting...", 880, 85, 17, Color{100, 116, 139, 255}, LudoUIWidget::boldFont);
    }
    
    // Dice visual container
    Rectangle diceContainerRec = {
        offsetX + 870.0f * scaleFactor,
        offsetY + 125.0f * scaleFactor,
        210.0f * scaleFactor,
        150.0f * scaleFactor
    };
    DrawRectangleRounded(diceContainerRec, 0.15f, 4, Color{15, 23, 42, 200});
    DrawRectangleRoundedLines(diceContainerRec, 0.15f, 4, 1.0f * scaleFactor, Color{51, 65, 85, 255});
    
    // Draw Dice Face
    int displayFaceRight = 1;
    if (!isLeftPlayer) {
        displayFaceRight = currentRollValue;
        if (subState == SUBSTATE_DICE_ROLLING) {
            displayFaceRight = 1 + (rand() % 6);
        }
        if (displayFaceRight < 1) displayFaceRight = 1;
    }
    
    Vector2 dicePosRight = {870 + 105.0f, 125 + 75.0f};
    bool diceDrawnRight = false;
    if (LudoBoard::isLoaded && LudoBoard::diceTextures[displayFaceRight - 1].id != 0) {
        float diceSize = 75.0f;
        Color diceTint = !isLeftPlayer ? WHITE : ColorAlpha(WHITE, 0.4f);
        if (!isLeftPlayer && subState == SUBSTATE_DICE_ROLLING) {
            // Dice shake/rotation visual
            float shakeAngle = sinf(GetTime() * 40.0f) * 15.0f;
            diceSize = 75.0f + sinf(GetTime() * 30.0f) * 4.0f;
            Rectangle source = {0.0f, 0.0f, (float)LudoBoard::diceTextures[displayFaceRight - 1].width, (float)LudoBoard::diceTextures[displayFaceRight - 1].height};
            Rectangle dest = {
                offsetX + dicePosRight.x * scaleFactor,
                offsetY + dicePosRight.y * scaleFactor,
                diceSize * scaleFactor,
                diceSize * scaleFactor
            };
            DrawTexturePro(LudoBoard::diceTextures[displayFaceRight - 1], source, dest, Vector2{(diceSize * scaleFactor)/2.0f, (diceSize * scaleFactor)/2.0f}, shakeAngle, diceTint);
        } else {
            Rectangle source = {0.0f, 0.0f, (float)LudoBoard::diceTextures[displayFaceRight - 1].width, (float)LudoBoard::diceTextures[displayFaceRight - 1].height};
            Rectangle dest = {
                offsetX + (dicePosRight.x - 37.5f) * scaleFactor,
                offsetY + (dicePosRight.y - 37.5f) * scaleFactor,
                75.0f * scaleFactor,
                75.0f * scaleFactor
            };
            DrawTexturePro(LudoBoard::diceTextures[displayFaceRight - 1], source, dest, Vector2{0.0f, 0.0f}, 0.0f, diceTint);
        }
        diceDrawnRight = true;
    }
    
    if (!diceDrawnRight) {
        Color slateDark = Color{30, 41, 59, 255};
        Color diceBg = !isLeftPlayer ? WHITE : ColorAlpha(WHITE, 0.4f);
        Rectangle fallbackDiceRec = {
            offsetX + (dicePosRight.x - 30.0f) * scaleFactor,
            offsetY + (dicePosRight.y - 30.0f) * scaleFactor,
            60.0f * scaleFactor,
            60.0f * scaleFactor
        };
        DrawRectangleRounded(fallbackDiceRec, 0.2f, 4, diceBg);
        DrawRectangleRoundedLines(fallbackDiceRec, 0.2f, 4, 2.0f * scaleFactor, slateDark);
        if (!isLeftPlayer) {
            std::string numStr = std::to_string(displayFaceRight);
            float fs = 24.0f * scaleFactor;
            Vector2 textSz = MeasureTextEx(LudoUIWidget::boldFont, numStr.c_str(), fs, 1.0f);
            DrawTextEx(LudoUIWidget::boldFont, numStr.c_str(), Vector2{
                roundf(offsetX + dicePosRight.x * scaleFactor - textSz.x/2.0f), 
                roundf(offsetY + dicePosRight.y * scaleFactor - textSz.y/2.0f)
            }, fs, 1.0f, slateDark);
        }
    }
    
    // Roll button
    Rectangle rollBtnRec = {885, 290, 180, 35};
    bool canRoll = (players[activePlayerIndex].type == "human" && subState == SUBSTATE_WAIT_FOR_ROLL && !isLeftPlayer);
    
    Color rBaseCol = canRoll ? Color{239, 68, 68, 255} : Color{71, 85, 105, 255};
    Color rHoverCol = canRoll ? Color{220, 38, 38, 255} : Color{71, 85, 105, 255};
    
    std::string rollBtnText = "Roll Dice";
    if (isLeftPlayer) {
        rollBtnText = "Roll on Left";
    } else if (subState == SUBSTATE_DICE_ROLLING) {
        rollBtnText = "Rolling...";
    } else if (currentRollValue > 0) {
        rollBtnText = "Rolled " + std::to_string(currentRollValue);
    }
    
    bool rollClicked = LudoUIWidget::button(rollBtnRec, rollBtnText, rBaseCol, rHoverCol, WHITE, 0.15f);
    if (canRoll && rollClicked) {
        subState = SUBSTATE_DICE_ROLLING;
        diceRollTimer = 0.0f;
        LudoAudio::playSFX("dice-roll");
    }
    
    // Match log Console (bottom right)
    LudoUIWidget::drawTextWithShadow("Match Log", 870, 360, 16, WHITE, LudoUIWidget::boldFont);
    Rectangle logRec = {
        offsetX + 870.0f * scaleFactor,
        offsetY + 390.0f * scaleFactor,
        210.0f * scaleFactor,
        235.0f * scaleFactor
    };
    DrawRectangleRounded(logRec, 0.05f, 4, Color{15, 23, 42, 220});
    DrawRectangleRoundedLines(logRec, 0.05f, 4, 1.5f * scaleFactor, Color{51, 65, 85, 255});
    
    // Draw logs list
    float logY = 390.0f + 12.0f;
    for (size_t i = 0; i < matchLogs.size(); i++) {
        float fs = 11.5f * scaleFactor;
        Vector2 textPos = {
            roundf(offsetX + (870.0f + 15.0f) * scaleFactor),
            roundf(offsetY + logY * scaleFactor)
        };
        DrawTextEx(LudoUIWidget::mainFont, matchLogs[i].message.c_str(), textPos, fs, 1.0f, matchLogs[i].color);
        logY += 19.5f;
    }
    
    // ----------------------------------------------------
    // Modals Overlays (Settings & Win screen)
    // ----------------------------------------------------
    // Switch to default centering scale for modal overlays
    scaleFactor = defaultScale;
    offsetX = defaultOffsetX;
    offsetY = defaultOffsetY;

    if (screenState == SCREEN_SETTINGS) {
        // Dark background overlay
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.6f));
        
        // Dialog container (400x320)
        Rectangle dlgRec = {
            offsetX + 350.0f * scaleFactor,
            offsetY + 165.0f * scaleFactor,
            400.0f * scaleFactor,
            320.0f * scaleFactor
        };
        DrawRectangleRounded(dlgRec, 0.12f, 4, Color{30, 41, 59, 255}); // Slate 800
        DrawRectangleRoundedLines(dlgRec, 0.12f, 4, 2.0f * scaleFactor, Color{59, 130, 246, 255}); // Highlight Blue
        
        LudoUIWidget::drawTextWithShadow("Settings", 375, 185, 22, WHITE, LudoUIWidget::boldFont);
        
        // Vol BGM Slider
        Rectangle bgmRec = {375, 250, 350, 8};
        float curBGM = LudoAudio::getBGMVolume();
        float newBGM = LudoUIWidget::slider(bgmRec, "Cozy Marimba BGM", curBGM);
        if (newBGM != curBGM) LudoAudio::setBGMVolume(newBGM);
        
        // Vol SFX Slider
        Rectangle sfxRec = {375, 310, 350, 8};
        float curSFX = LudoAudio::getSFXVolume();
        float newSFX = LudoUIWidget::slider(sfxRec, "Game Sound Effects", curSFX);
        if (newSFX != curSFX) LudoAudio::setSFXVolume(newSFX);
        
        // Gameplay speed selection slider / button
        LudoUIWidget::drawTextWithShadow("AI Gameplay Speed", 375, 350, 14, Color{148, 163, 184, 255}, LudoUIWidget::mainFont);
        Rectangle speedBtnRec = {375, 375, 160, 28};
        std::string speedLabel = (aiSpeedCoeff > 1.5f) ? "Fast Speed" : "Normal Speed";
        Color speedBtnCol = (aiSpeedCoeff > 1.5f) ? Color{239, 68, 68, 255} : Color{59, 130, 246, 255};
        
        if (LudoUIWidget::button(speedBtnRec, speedLabel, speedBtnCol, ColorAlpha(speedBtnCol, 0.8f), WHITE, 0.15f)) {
            if (aiSpeedCoeff > 1.5f) aiSpeedCoeff = 1.0f;
            else aiSpeedCoeff = 2.0f;
        }
        
        // Close Button
        Rectangle closeRec = {475, 430, 150, 35};
        if (LudoUIWidget::button(closeRec, "Save & Close", Color{71, 85, 105, 255}, Color{100, 116, 139, 255}, WHITE, 0.2f)) {
            screenState = prevScreenState;
        }
    }
    
    if (screenState == SCREEN_WIN_SCREEN) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.7f));
        
        // Celebration dialog
        Rectangle winRec = {
            offsetX + 350.0f * scaleFactor,
            offsetY + 80.0f * scaleFactor,
            400.0f * scaleFactor,
            490.0f * scaleFactor
        };
        DrawRectangleRounded(winRec, 0.12f, 4, Color{15, 23, 42, 255});
        DrawRectangleRoundedLines(winRec, 0.12f, 4, 3.0f * scaleFactor, Color{245, 158, 11, 255}); // Gold border
        
        // Trophy emoji drawn with text scale
        LudoUIWidget::drawTextWithShadow("🏆", 550 - MeasureTextEx(LudoUIWidget::boldFont, "🏆", 50, 1.0f).x/2.0f, 110, 50, WHITE, LudoUIWidget::boldFont);
        LudoUIWidget::drawTextWithShadow("Victory Celebrations!", 550 - MeasureTextEx(LudoUIWidget::boldFont, "Victory Celebrations!", 22, 1.0f).x/2.0f, 175, 22, Color{245, 158, 11, 255}, LudoUIWidget::boldFont);
        
        // Winner details
        std::string winName = "Nobody";
        if (!winnersList.empty()) {
            winName = players[winnersList[0]].name;
        }
        std::string winnerAnn = winName + " wins the match!";
        LudoUIWidget::drawTextWithShadow(winnerAnn, 550 - MeasureTextEx(LudoUIWidget::boldFont, winnerAnn.c_str(), 16, 1.0f).x/2.0f, 215, 16, WHITE, LudoUIWidget::boldFont);
        
        // Rankings
        LudoUIWidget::drawTextWithShadow("Leaderboard Rankings:", 380, 255, 14, Color{148, 163, 184, 255}, LudoUIWidget::mainFont);
        for (size_t i = 0; i < winnersList.size(); i++) {
            std::string rankStr = "#" + std::to_string(i + 1) + ": " + players[winnersList[i]].name;
            LudoUIWidget::drawTextWithShadow(rankStr, 395, 280 + i * 26, 14, (i == 0 ? Color{245, 158, 11, 255} : WHITE), LudoUIWidget::boldFont);
        }
        
        // Match Achievements
        LudoUIWidget::drawTextWithShadow("Match Achievements:", 380, 395, 14, Color{148, 163, 184, 255}, LudoUIWidget::mainFont);
        std::string capsStr = "Captured pieces: " + std::to_string(capturesCount);
        std::string rollsStr = "Dice rolls: " + std::to_string(rollsCount);
        LudoUIWidget::drawTextWithShadow(capsStr, 395, 420, 13, WHITE, LudoUIWidget::mainFont);
        LudoUIWidget::drawTextWithShadow(rollsStr, 395, 442, 13, WHITE, LudoUIWidget::mainFont);
        
        // Actions
        Rectangle btnAgain = {380, 500, 160, 40};
        Rectangle btnMenu = {560, 500, 160, 40};
        
        if (LudoUIWidget::button(btnAgain, "PLAY AGAIN", Color{34, 197, 94, 255}, Color{22, 163, 74, 255}, WHITE, 0.15f)) {
            start(gameMode);
        }
        
        if (LudoUIWidget::button(btnMenu, "MAIN MENU", Color{71, 85, 105, 255}, Color{100, 116, 139, 255}, WHITE, 0.15f)) {
            restart();
        }
    }
}

Vector2 LudoGame::getMousePosition() {
    Vector2 mouse = ::GetMousePosition();
    float virtualWidth = 1100.0f;
    float virtualHeight = 650.0f;
    
    // Scale and translate using static parameters
    Vector2 virtualMouse = { 0 };
    if (scaleFactor > 0.0001f) {
        virtualMouse.x = (mouse.x - offsetX) / scaleFactor;
        virtualMouse.y = (mouse.y - offsetY) / scaleFactor;
    }
    
    // Clamp inside virtual resolution bounds
    if (virtualMouse.x < 0.0f) virtualMouse.x = 0.0f;
    if (virtualMouse.x > virtualWidth) virtualMouse.x = virtualWidth;
    if (virtualMouse.y < 0.0f) virtualMouse.y = 0.0f;
    if (virtualMouse.y > virtualHeight) virtualMouse.y = virtualHeight;
    
    return virtualMouse;
}
