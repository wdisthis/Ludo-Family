#include "board.hpp"
#include "rules.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include "game.hpp"

Texture2D LudoBoard::pieceTextures[4];
Texture2D LudoBoard::diceTextures[6];
Texture2D LudoBoard::shadowTexture;
Texture2D LudoBoard::boardBgTexture;
bool LudoBoard::isLoaded = false;
int LudoBoard::currentBoardTextureSize = 0;
std::vector<LudoEffectParticle> LudoBoard::particles;
std::vector<LudoRingEffect> LudoBoard::ringEffects;

static void DrawShield(float x, float y, float size, Color color) {
    // Draws a beautiful shield shape using points
    float w = size * 0.8f;
    float h = size * 0.9f;
    
    // Outer outline shadow
    DrawTriangle(Vector2{x, y - h/2.0f}, Vector2{x - w/2.0f, y - h/4.0f}, Vector2{x + w/2.0f, y - h/4.0f}, ColorAlpha(BLACK, 0.2f));
    DrawRectanglePro(Rectangle{x - w/2.0f, y - h/4.0f, w, h/2.0f}, Vector2{0, 0}, 0.0f, ColorAlpha(BLACK, 0.2f));
    DrawTriangle(Vector2{x - w/2.0f, y + h/4.0f}, Vector2{x, y + h/2.0f + 1}, Vector2{x + w/2.0f, y + h/4.0f}, ColorAlpha(BLACK, 0.2f));
    
    // Main shield body (offset slightly)
    DrawTriangle(Vector2{x, y - h/2.0f}, Vector2{x - w/2.0f, y - h/4.0f}, Vector2{x + w/2.0f, y - h/4.0f}, color);
    DrawRectanglePro(Rectangle{x - w/2.0f, y - h/4.0f, w, h/2.0f}, Vector2{0, 0}, 0.0f, color);
    DrawTriangle(Vector2{x - w/2.0f, y + h/4.0f}, Vector2{x, y + h/2.0f}, Vector2{x + w/2.0f, y + h/4.0f}, color);
    
    // Glossy light shield inlay
    Color inlayCol = Color{241, 245, 249, 230};
    float iw = w * 0.7f;
    float ih = h * 0.7f;
    DrawTriangle(Vector2{x, y - ih/2.0f}, Vector2{x - iw/2.0f, y - ih/4.0f}, Vector2{x + iw/2.0f, y - ih/4.0f}, inlayCol);
    DrawRectanglePro(Rectangle{x - iw/2.0f, y - ih/4.0f, iw, ih/2.0f}, Vector2{0, 0}, 0.0f, inlayCol);
    DrawTriangle(Vector2{x - iw/2.0f, y + ih/4.0f}, Vector2{x, y + ih/2.0f}, Vector2{x + iw/2.0f, y + ih/4.0f}, inlayCol);
    
    // Border line
    Color slateBorder = Color{71, 85, 105, 255};
    DrawTriangleLines(Vector2{x, y - h/2.0f}, Vector2{x - w/2.0f, y - h/4.0f}, Vector2{x + w/2.0f, y - h/4.0f}, slateBorder);
    DrawLineEx(Vector2{x - w/2.0f, y - h/4.0f}, Vector2{x - w/2.0f, y + h/4.0f}, 1.5f, slateBorder);
    DrawLineEx(Vector2{x + w/2.0f, y - h/4.0f}, Vector2{x + w/2.0f, y + h/4.0f}, 1.5f, slateBorder);
    DrawTriangleLines(Vector2{x - w/2.0f, y + h/4.0f}, Vector2{x, y + h/2.0f}, Vector2{x + w/2.0f, y + h/4.0f}, slateBorder);
}

void LudoBoard::init() {
    boardBgTexture.id = 0;
    currentBoardTextureSize = 0;

    // Load piece textures
    std::string colors[4] = {"green", "yellow", "blue", "red"};
    for (int i = 0; i < 4; i++) {
        std::string path = "assets/images/pieces/piece-" + colors[i] + ".png";
        Image img = LoadImage(path.c_str());
        if (img.data != nullptr) {
            pieceTextures[i] = LoadTextureFromImage(img);
            UnloadImage(img);
        }
    }
    
    // Load dice textures
    for (int i = 0; i < 6; i++) {
        std::string path = "assets/images/dice/dice-" + std::to_string(i + 1) + ".png";
        Image img = LoadImage(path.c_str());
        if (img.data != nullptr) {
            diceTextures[i] = LoadTextureFromImage(img);
            UnloadImage(img);
        }
    }
    
    // Shadow image
    Image shadowImg = LoadImage("assets/images/pieces/piece-shadow.png");
    if (shadowImg.data != nullptr) {
        shadowTexture = LoadTextureFromImage(shadowImg);
        UnloadImage(shadowImg);
    }
    
    isLoaded = true;
    particles.clear();
    ringEffects.clear();
}

void LudoBoard::close() {
    if (isLoaded) {
        UnloadTexture(boardBgTexture);
        for (int i = 0; i < 4; i++) {
            UnloadTexture(pieceTextures[i]);
        }
        for (int i = 0; i < 6; i++) {
            UnloadTexture(diceTextures[i]);
        }
        UnloadTexture(shadowTexture);
    }
}

void LudoBoard::update(float deltaTime) {
    // Update particles
    for (auto it = particles.begin(); it != particles.end();) {
        it->position.x += it->velocity.x * deltaTime;
        it->position.y += it->velocity.y * deltaTime;
        it->lifeTime -= deltaTime;
        it->alpha = it->lifeTime / 0.75f; // Fade out over 0.75s
        
        if (it->lifeTime <= 0.0f) {
            it = particles.erase(it);
        } else {
            it++;
        }
    }
    
    // Update ring effects
    for (auto it = ringEffects.begin(); it != ringEffects.end();) {
        it->radius += (it->maxRadius - it->radius) * 5.0f * deltaTime;
        it->alpha -= 1.17f * deltaTime; // Fade out over ~0.85s
        
        if (it->alpha <= 0.0f) {
            it = ringEffects.erase(it);
        } else {
            it++;
        }
    }
}

void LudoBoard::triggerCaptureEffect(float gx, float gy) {
    // Convert logic coordinates to board center position
    Vector2 pos = getScreenPos(0, 0, 600, gx, gy);
    pos.x += 20; // Center in 40x40 cell
    pos.y += 20;
    
    // Spawn 16 sparks in a circular burst
    Color sparkCols[4] = {
        Color{255, 77, 77, 255},  // Red
        Color{255, 204, 0, 255},  // Yellow
        Color{46, 204, 113, 255}, // Green
        Color{59, 130, 246, 255}  // Blue
    };
    
    for (int i = 0; i < 16; i++) {
        float angle = (i / 16.0f) * 2.0f * PI;
        float speed = 25.0f + (rand() % 100);
        
        LudoEffectParticle p;
        p.position = pos;
        p.velocity = Vector2{cosf(angle) * speed, sinf(angle) * speed};
        p.color = sparkCols[rand() % 4];
        p.alpha = 1.0f;
        p.size = 3.0f + (rand() % 4);
        p.lifeTime = 0.6f + (rand() % 10) * 0.02f;
        particles.push_back(p);
    }
    
    // Expanding shockwave ring
    LudoRingEffect ring;
    ring.position = pos;
    ring.radius = 5.0f;
    ring.maxRadius = 55.0f;
    ring.alpha = 1.0f;
    ring.color = Color{255, 77, 77, 255};
    ringEffects.push_back(ring);
}

void LudoBoard::triggerSafeEffect(float gx, float gy) {
    Vector2 pos = getScreenPos(0, 0, 600, gx, gy);
    pos.x += 20;
    pos.y += 20;
    
    // Green safe zone ring
    LudoRingEffect ring;
    ring.position = pos;
    ring.radius = 5.0f;
    ring.maxRadius = 45.0f;
    ring.alpha = 1.0f;
    ring.color = Color{34, 197, 94, 255};
    ringEffects.push_back(ring);
}

Vector2 LudoBoard::getScreenPos(float startX, float startY, float boardSize, int gx, int gy) {
    float CS = boardSize / 15.0f;
    return Vector2{
        startX + gx * CS,
        startY + gy * CS
    };
}

void LudoBoard::drawBoardBackground(float startX, float startY, float boardSize) {
    if (boardBgTexture.id != 0) {
        DrawTexturePro(boardBgTexture, 
                       Rectangle{0.0f, 0.0f, (float)boardBgTexture.width, (float)boardBgTexture.height}, 
                       Rectangle{startX, startY, boardSize, boardSize}, 
                       Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        // Draw border
        DrawRectangleLinesEx(Rectangle{startX, startY, boardSize, boardSize}, 4.0f, Color{30, 41, 59, 255});
        return;
    }
    
    float CS = boardSize / 15.0f;
    
    // Define premium palette colors
    Color greenCol = Color{34, 197, 94, 255};   // Green
    Color yellowCol = Color{245, 158, 11, 255}; // Yellow
    Color blueCol = Color{37, 99, 235, 255};   // Blue
    Color redCol = Color{239, 68, 68, 255};     // Red
    
    Color slateDark = Color{30, 41, 59, 255};
    Color offWhite = Color{248, 250, 252, 255};
    Color socketBg = Color{226, 232, 240, 255};
    
    // 1. Draw outer frame background
    DrawRectangle(startX, startY, boardSize, boardSize, WHITE);
    
    // 2. Draw Yard bases
    DrawRectangle(startX, startY, 6 * CS, 6 * CS, greenCol);
    DrawRectangle(startX + 9 * CS, startY, 6 * CS, 6 * CS, yellowCol);
    DrawRectangle(startX, startY + 9 * CS, 6 * CS, 6 * CS, redCol);
    DrawRectangle(startX + 9 * CS, startY + 9 * CS, 6 * CS, 6 * CS, blueCol);
    
    // 3. Draw inner cards in yards
    DrawRectangleRounded(Rectangle{startX + CS, startY + CS, 4 * CS, 4 * CS}, 0.15f, 4, offWhite);
    DrawRectangleRounded(Rectangle{startX + 10 * CS, startY + CS, 4 * CS, 4 * CS}, 0.15f, 4, offWhite);
    DrawRectangleRounded(Rectangle{startX + CS, startY + 10 * CS, 4 * CS, 4 * CS}, 0.15f, 4, offWhite);
    DrawRectangleRounded(Rectangle{startX + 10 * CS, startY + 10 * CS, 4 * CS, 4 * CS}, 0.15f, 4, offWhite);
    
    // 4. Draw base sockets
    int baseCells[4][4][2] = {
        {{2, 2}, {4, 2}, {2, 4}, {4, 4}}, // Green
        {{11, 2}, {13, 2}, {11, 4}, {13, 4}}, // Yellow
        {{2, 11}, {4, 11}, {2, 13}, {4, 13}}, // Red
        {{11, 11}, {13, 11}, {11, 13}, {13, 13}}  // Blue
    };
    
    for (int p = 0; p < 4; p++) {
        for (int s = 0; s < 4; s++) {
            int gx = baseCells[p][s][0];
            int gy = baseCells[p][s][1];
            Vector2 sPos = getScreenPos(startX, startY, boardSize, gx, gy);
            DrawCircle(sPos.x, sPos.y, CS * 0.5f - 4, socketBg);
            DrawCircleLines(sPos.x, sPos.y, CS * 0.5f - 4, ColorAlpha(BLACK, 0.15f));
            DrawCircle(sPos.x, sPos.y, 6.0f, ColorAlpha(BLACK, 0.08f));
        }
    }
    
    // 5. Draw grid cells
    // Top track
    for (int r = 0; r < 6; r++) {
        for (int c = 6; c <= 8; c++) {
            Vector2 cellPos = getScreenPos(startX, startY, boardSize, c, r);
            Color fill = WHITE;
            if (c == 7 && r >= 1) fill = yellowCol; // Home path
            else if (c == 8 && r == 1) fill = yellowCol; // Start point
            
            DrawRectangle(cellPos.x, cellPos.y, CS, CS, fill);
            DrawRectangleLines(cellPos.x, cellPos.y, CS, CS, Color{226, 232, 240, 255}); // Slate 200 gridlines
        }
    }
    
    // Left track
    for (int r = 6; r <= 8; r++) {
        for (int c = 0; c < 6; c++) {
            Vector2 cellPos = getScreenPos(startX, startY, boardSize, c, r);
            Color fill = WHITE;
            if (r == 7 && c >= 1) fill = greenCol;
            else if (r == 6 && c == 1) fill = greenCol;
            
            DrawRectangle(cellPos.x, cellPos.y, CS, CS, fill);
            DrawRectangleLines(cellPos.x, cellPos.y, CS, CS, Color{226, 232, 240, 255});
        }
    }
    
    // Bottom track
    for (int r = 9; r < 15; r++) {
        for (int c = 6; c <= 8; c++) {
            Vector2 cellPos = getScreenPos(startX, startY, boardSize, c, r);
            Color fill = WHITE;
            if (c == 7 && r <= 13) fill = redCol;
            else if (c == 6 && r == 13) fill = redCol;
            
            DrawRectangle(cellPos.x, cellPos.y, CS, CS, fill);
            DrawRectangleLines(cellPos.x, cellPos.y, CS, CS, Color{226, 232, 240, 255});
        }
    }
    
    // Right track
    for (int r = 6; r <= 8; r++) {
        for (int c = 9; c < 15; c++) {
            Vector2 cellPos = getScreenPos(startX, startY, boardSize, c, r);
            Color fill = WHITE;
            if (r == 7 && c <= 13) fill = blueCol;
            else if (r == 8 && c == 13) fill = blueCol;
            
            DrawRectangle(cellPos.x, cellPos.y, CS, CS, fill);
            DrawRectangleLines(cellPos.x, cellPos.y, CS, CS, Color{226, 232, 240, 255});
        }
    }
    
    // 6. Draw central finished home wedges
    Vector2 center = {startX + 7.5f * CS, startY + 7.5f * CS};
    // Green (left)
    DrawTriangle(center, Vector2{startX + 6*CS, startY + 9*CS}, Vector2{startX + 6*CS, startY + 6*CS}, greenCol);
    DrawTriangleLines(center, Vector2{startX + 6*CS, startY + 9*CS}, Vector2{startX + 6*CS, startY + 6*CS}, slateDark);
    
    // Yellow (top)
    DrawTriangle(center, Vector2{startX + 6*CS, startY + 6*CS}, Vector2{startX + 9*CS, startY + 6*CS}, yellowCol);
    DrawTriangleLines(center, Vector2{startX + 6*CS, startY + 6*CS}, Vector2{startX + 9*CS, startY + 6*CS}, slateDark);
    
    // Blue (right)
    DrawTriangle(center, Vector2{startX + 9*CS, startY + 6*CS}, Vector2{startX + 9*CS, startY + 9*CS}, blueCol);
    DrawTriangleLines(center, Vector2{startX + 9*CS, startY + 6*CS}, Vector2{startX + 9*CS, startY + 9*CS}, slateDark);
    
    // Red (bottom)
    DrawTriangle(center, Vector2{startX + 9*CS, startY + 9*CS}, Vector2{startX + 6*CS, startY + 9*CS}, redCol);
    DrawTriangleLines(center, Vector2{startX + 9*CS, startY + 9*CS}, Vector2{startX + 6*CS, startY + 9*CS}, slateDark);
    
    // 7. Draw safe zone checkpoints with custom shield outline
    // Safe cells coordinates
    DrawShield(startX + 6*CS + CS/2.0f, startY + 13*CS + CS/2.0f, CS, redCol);     // Red Start
    DrawShield(startX + 1*CS + CS/2.0f, startY + 6*CS + CS/2.0f, CS, greenCol);    // Green Start
    DrawShield(startX + 8*CS + CS/2.0f, startY + 1*CS + CS/2.0f, CS, yellowCol);   // Yellow Start
    DrawShield(startX + 13*CS + CS/2.0f, startY + 8*CS + CS/2.0f, CS, blueCol);    // Blue Start
    
    DrawShield(startX + 8*CS + CS/2.0f, startY + 12*CS + CS/2.0f, CS, redCol);     // Neutral Red safe
    DrawShield(startX + 2*CS + CS/2.0f, startY + 8*CS + CS/2.0f, CS, greenCol);    // Neutral Green safe
    DrawShield(startX + 6*CS + CS/2.0f, startY + 2*CS + CS/2.0f, CS, yellowCol);   // Neutral Yellow safe
    DrawShield(startX + 12*CS + CS/2.0f, startY + 6*CS + CS/2.0f, CS, blueCol);    // Neutral Blue safe
    
    // 8. Draw thick outer frame
    DrawRectangleLinesEx(Rectangle{startX, startY, boardSize, boardSize}, 4.0f, slateDark);
}

void LudoBoard::draw(const std::vector<LudoPlayer>& players, int activePlayerId, const std::vector<int>& clickableTokenIds, int selectedTokenId) {
    float boardSize = 600.0f * LudoGame::scaleFactor;
    float CS = boardSize / 15.0f;
    float startX = LudoGame::offsetX + 250.0f * LudoGame::scaleFactor;
    float startY = LudoGame::offsetY + 25.0f * LudoGame::scaleFactor;
    
    // Dynamically rasterize SVG to match the current screen size for perfect crispness
    int targetSize = (int)boardSize;
    if (targetSize < 10) targetSize = 600;
    
    if (boardBgTexture.id == 0 || currentBoardTextureSize != targetSize) {
        if (boardBgTexture.id != 0) {
            UnloadTexture(boardBgTexture);
        }
        Image boardImg = LoadImageSvg("assets/images/board.svg", targetSize, targetSize);
        if (boardImg.data != nullptr) {
            boardBgTexture = LoadTextureFromImage(boardImg);
            UnloadImage(boardImg);
            currentBoardTextureSize = targetSize;
        } else {
            boardBgTexture.id = 0;
            currentBoardTextureSize = 0;
        }
    }
    
    // 1. Draw base board structure
    drawBoardBackground(startX, startY, boardSize);
    
    // 2. Draw active effects (rings)
    for (const auto& ring : ringEffects) {
        float rx = startX + ring.position.x * LudoGame::scaleFactor;
        float ry = startY + ring.position.y * LudoGame::scaleFactor;
        DrawCircleLines(rx, ry, ring.radius * LudoGame::scaleFactor, ColorAlpha(ring.color, ring.alpha));
        DrawCircleLines(rx, ry, (ring.radius + 1.5f) * LudoGame::scaleFactor, ColorAlpha(ring.color, ring.alpha * 0.7f));
    }
    
    // 3. Map tokens to board cells to manage stack layout correctly
    // We will group pieces by coordinate keys
    struct TokenInfo {
        const LudoPiece* piece;
        const LudoPlayer* player;
        bool isClickable;
        bool isSelected;
    };
    
    std::map<std::string, std::vector<TokenInfo>> cellMap;
    
    for (const auto& player : players) {
        if (player.type == "none" || player.isFinished()) continue;
        
        for (const auto& piece : player.tokens) {
            std::vector<int> coords = piece.getCoordinates();
            std::string key = std::to_string(coords[0]) + "_" + std::to_string(coords[1]);
            
            bool clickable = false;
            if (player.id == activePlayerId) {
                for (int cid : clickableTokenIds) {
                    if (cid == piece.id) {
                        clickable = true;
                        break;
                    }
                }
            }
            
            bool selected = (player.id == activePlayerId && piece.id == selectedTokenId);
            
            cellMap[key].push_back({&piece, &player, clickable, selected});
        }
    }
    
    // 4. Draw tokens according to stack sizes
    for (const auto& pair : cellMap) {
        std::string key = pair.first;
        size_t dash = key.find('_');
        int gx = std::stoi(key.substr(0, dash));
        int gy = std::stoi(key.substr(dash + 1));
        
        const auto& items = pair.second;
        int totalInCell = items.size();
        
        Vector2 cellTL = getScreenPos(startX, startY, boardSize, gx, gy);
        
        for (int i = 0; i < totalInCell; i++) {
            const auto& item = items[i];
            
            // Calculate base drawing position and diameter of token
            float radius = 15.0f * LudoGame::scaleFactor; // Default single centered
            Vector2 pos = {cellTL.x + CS/2.0f, cellTL.y + CS/2.0f};
            
            bool isBase = (item.piece->step == -1);
            
            if (!isBase) {
                if (totalInCell == 2) {
                    radius = 11.0f * LudoGame::scaleFactor;
                    if (i == 0) pos = {cellTL.x + 11.0f * LudoGame::scaleFactor, cellTL.y + CS/2.0f};
                    else pos = {cellTL.x + 29.0f * LudoGame::scaleFactor, cellTL.y + CS/2.0f};
                } else if (totalInCell == 3) {
                    radius = 9.0f * LudoGame::scaleFactor;
                    if (i == 0) pos = {cellTL.x + CS/2.0f, cellTL.y + 10.0f * LudoGame::scaleFactor};
                    else if (i == 1) pos = {cellTL.x + 10.0f * LudoGame::scaleFactor, cellTL.y + 30.0f * LudoGame::scaleFactor};
                    else pos = {cellTL.x + 30.0f * LudoGame::scaleFactor, cellTL.y + 30.0f * LudoGame::scaleFactor};
                } else if (totalInCell >= 4) {
                    radius = 8.5f * LudoGame::scaleFactor;
                    if (i == 0) pos = {cellTL.x + 11.0f * LudoGame::scaleFactor, cellTL.y + 11.0f * LudoGame::scaleFactor};
                    else if (i == 1) pos = {cellTL.x + 29.0f * LudoGame::scaleFactor, cellTL.y + 11.0f * LudoGame::scaleFactor};
                    else if (i == 2) pos = {cellTL.x + 11.0f * LudoGame::scaleFactor, cellTL.y + 29.0f * LudoGame::scaleFactor};
                    else pos = {cellTL.x + 29.0f * LudoGame::scaleFactor, cellTL.y + 29.0f * LudoGame::scaleFactor};
                }
            } else {
                // Centered in yard base sockets (which are exactly at cellTL in our grid system)
                pos = cellTL;
                radius = 14.0f * LudoGame::scaleFactor;
            }
            
            // Pulse animation for eligible tokens
            float hopY = 0.0f;
            float pulseScale = 1.0f;
            if (item.isClickable) {
                // Bobbing hop animation
                float time = GetTime() * 5.0f;
                hopY = -fabsf(sinf(time)) * 4.0f * LudoGame::scaleFactor;
                pulseScale = 1.0f + sinf(time * 2.0f) * 0.05f;
                
                // Draw eligible sonar wave ping
                float pingRadius = radius * (1.2f + fabsf(sinf(time)) * 0.6f);
                Color pingCol = YELLOW;
                if (item.player->color == "green") pingCol = Color{34, 197, 94, 150};
                else if (item.player->color == "yellow") pingCol = Color{245, 158, 11, 150};
                else if (item.player->color == "blue") pingCol = Color{37, 99, 235, 150};
                else if (item.player->color == "red") pingCol = Color{239, 68, 68, 150};
                DrawCircleLines(pos.x, pos.y + hopY, pingRadius, pingCol);
            }
            
            // Draw floor shadow
            float shadowRadius = radius * (1.1f + hopY * 0.05f);
            if (isLoaded && shadowTexture.id != 0) {
                float shadowW = shadowRadius * 2.3f;
                float shadowH = shadowRadius * 0.6f;
                DrawTexturePro(
                    shadowTexture,
                    Rectangle{0, 0, (float)shadowTexture.width, (float)shadowTexture.height},
                    Rectangle{pos.x, pos.y + radius * 0.8f, shadowW, shadowH},
                    Vector2{shadowW/2.0f, shadowH/2.0f},
                    0.0f,
                    ColorAlpha(WHITE, 0.45f + hopY * 0.05f)
                );
            } else {
                DrawEllipse(pos.x, pos.y + radius * 0.8f, shadowRadius, shadowRadius * 0.25f, ColorAlpha(BLACK, 0.3f));
            }
            
            // Draw token image or colored circles if images failed to load
            bool drawnTexture = false;
            if (isLoaded && pieceTextures[item.player->id].id != 0) {
                float size = radius * 2.2f * pulseScale;
                Rectangle sourceRec = {0.0f, 0.0f, (float)pieceTextures[item.player->id].width, (float)pieceTextures[item.player->id].height};
                // Token image has pivot centered at bottom or center, we draw centered
                Rectangle destRec = {pos.x, pos.y + hopY, size, size};
                DrawTexturePro(pieceTextures[item.player->id], sourceRec, destRec, Vector2{size/2.0f, size/2.0f}, 0.0f, WHITE);
                drawnTexture = true;
            }
            
            if (!drawnTexture) {
                // Fallback colored token circles (sleek 3D circle look)
                Color mainC = RED;
                Color darkC = Color{127, 29, 29, 255};
                if (item.player->color == "green") { mainC = Color{34, 197, 94, 255}; darkC = Color{20, 83, 45, 255}; }
                else if (item.player->color == "yellow") { mainC = Color{245, 158, 11, 255}; darkC = Color{120, 53, 4, 255}; }
                else if (item.player->color == "blue") { mainC = Color{37, 99, 235, 255}; darkC = Color{23, 37, 84, 255}; }
                
                // Border outline
                DrawCircle(pos.x, pos.y + hopY, radius, darkC);
                // Inner bulb
                DrawCircle(pos.x, pos.y + hopY, radius - 2, mainC);
                // Gloss highlight
                DrawCircle(pos.x - radius/3.0f, pos.y - radius/3.0f + hopY, radius * 0.3f, ColorAlpha(WHITE, 0.4f));
            }
            
            // Draw selection highlighter outline if selected
            if (item.isSelected) {
                DrawCircleLines(pos.x, pos.y + hopY, radius + 2.0f, WHITE);
                DrawCircleLines(pos.x, pos.y + hopY, radius + 3.0f, Color{59, 130, 246, 255});
            }
        }
    }
    
    // 5. Draw particle explosions
    for (const auto& p : particles) {
        float px = startX + p.position.x * LudoGame::scaleFactor;
        float py = startY + p.position.y * LudoGame::scaleFactor;
        DrawCircle(px, py, p.size * LudoGame::scaleFactor, ColorAlpha(p.color, p.alpha));
    }
}
