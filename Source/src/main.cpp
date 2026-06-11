#include "raylib.h"
#include "game.hpp"
#include "board.hpp"
#include "audio.hpp"
#include "ui.hpp"
#include <algorithm>

int main() {
    // Configure window flags before InitWindow
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);
    
    // Premium 1100x650 viewport size for side panels alongside 600x600 board
    InitWindow(1100, 650, "Ludo Family");
    SetTargetFPS(60);
    
    // Set window resizeable to allow sizing window to match fullscreen aspect changes
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    
    // Initialize game systems
    LudoGame::init();
    
    // Main Game Loop
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        // Fullscreen toggle check (F11 or Alt+Enter)
        if (IsKeyPressed(KEY_F11) || (IsKeyDown(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            ToggleBorderlessWindowed();
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
