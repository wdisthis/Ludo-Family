#include "raylib.h"
#include "game.hpp"
#include "board.hpp"
#include "audio.hpp"
#include "ui.hpp"
#include <algorithm>

int main() {
    // Premium 1100x650 viewport size for side panels alongside 600x600 board
    InitWindow(1100, 650, "Ludo Cozy Club - Premium Family Board Game");
    SetTargetFPS(60);
    
    // Set window resizeable to allow sizing window to match fullscreen aspect changes
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    
    // Initialize game systems
    LudoGame::init();
    
    bool isFullscreen = false;
    
    // Main Game Loop
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        // Fullscreen toggle check (F11 or Alt+Enter)
        if (IsKeyPressed(KEY_F11) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            isFullscreen = !isFullscreen;
            if (isFullscreen) {
                int monitor = GetCurrentMonitor();
                int w = GetMonitorWidth(monitor);
                int h = GetMonitorHeight(monitor);
                
                SetWindowState(FLAG_WINDOW_UNDECORATED);
                SetWindowPosition(0, 0);
                SetWindowSize(w, h);
            } else {
                ClearWindowState(FLAG_WINDOW_UNDECORATED);
                SetWindowSize(1100, 650);
                
                int monitor = GetCurrentMonitor();
                int w = GetMonitorWidth(monitor);
                int h = GetMonitorHeight(monitor);
                SetWindowPosition((w - 1100) / 2, (h - 650) / 2);
            }
        }
        
        // Update game state
        LudoGame::update(deltaTime);
        
        // Draw directly to physical window
        BeginDrawing();
        ClearBackground(BLACK); // Black borders for letterboxing/pillarboxing outside active game area
        
        LudoGame::draw();
        
        EndDrawing();
    }
    
    // Cleanup resources
    LudoBoard::close();
    LudoAudio::close();
    LudoUIWidget::unloadFonts();
    CloseWindow();
    
    return 0;
}
