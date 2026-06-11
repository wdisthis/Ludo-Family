#include "ui.hpp"
#include <iostream>
#include <cmath>
#include "game.hpp"
#include "board.hpp"
#include "audio.hpp"

Font LudoUIWidget::mainFont;
Font LudoUIWidget::boldFont;
bool LudoUIWidget::isFontLoaded = false;

void LudoUIWidget::initFonts() {
    // Attempt to load Segoe UI on Windows at a high resolution for vector-sharp text
    mainFont = LoadFontEx("C:\\Windows\\Fonts\\segoeui.ttf", 72, NULL, 0);
    boldFont = LoadFontEx("C:\\Windows\\Fonts\\segoeuib.ttf", 80, NULL, 0);
    
    if (mainFont.texture.id != 0 && boldFont.texture.id != 0) {
        isFontLoaded = true;
        // Generate mipmaps for smooth scaling down
        GenTextureMipmaps(&mainFont.texture);
        SetTextureFilter(mainFont.texture, TEXTURE_FILTER_TRILINEAR);
        GenTextureMipmaps(&boldFont.texture);
        SetTextureFilter(boldFont.texture, TEXTURE_FILTER_TRILINEAR);
    } else {
        std::cout << "Falling back to default Raylib font." << std::endl;
        mainFont = GetFontDefault();
        boldFont = GetFontDefault();
        isFontLoaded = false;
    }
}

void LudoUIWidget::unloadFonts() {
    if (isFontLoaded) {
        UnloadFont(mainFont);
        UnloadFont(boldFont);
    }
}

bool LudoUIWidget::button(Rectangle rect, const std::string& text, Color baseColor, Color hoverColor, Color textColor, float roundness) {
    Vector2 mousePos = LudoGame::getMousePosition(); // Already scaled to 1100x650 virtual space
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    bool clicked = false;
    
    Color col = baseColor;
    float scale = 1.0f;
    Rectangle drawRect = rect;
    
    if (hovered) {
        col = hoverColor;
        scale = 1.03f; // Slight pop scaling on hover
        drawRect.width *= scale;
        drawRect.height *= scale;
        drawRect.x -= (drawRect.width - rect.width) / 2.0f;
        drawRect.y -= (drawRect.height - rect.height) / 2.0f;
        
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            col = ColorAlpha(hoverColor, 0.8f);
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            clicked = true;
        }
    }
    
    // Scale virtual coordinates to physical screen coordinates
    Rectangle scaledDrawRect = {
        LudoGame::offsetX + drawRect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + drawRect.y * LudoGame::scaleFactor,
        drawRect.width * LudoGame::scaleFactor,
        drawRect.height * LudoGame::scaleFactor
    };
    
    // Draw button shadow in physical space
    Rectangle scaledShadow = scaledDrawRect;
    scaledShadow.y += 3.0f * LudoGame::scaleFactor;
    DrawRectangleRounded(scaledShadow, roundness, 8, ColorAlpha(BLACK, 0.15f));
    
    // Draw main button body
    DrawRectangleRounded(scaledDrawRect, roundness, 8, col);
    
    // Draw text centered
    float fontSize = (isFontLoaded ? 18.0f : 10.0f) * scale * LudoGame::scaleFactor;
    Vector2 textSize = MeasureTextEx(isFontLoaded ? boldFont : mainFont, text.c_str(), fontSize, 1.0f);
    Vector2 textPos = {
        roundf(scaledDrawRect.x + (scaledDrawRect.width - textSize.x) / 2.0f),
        roundf(scaledDrawRect.y + (scaledDrawRect.height - textSize.y) / 2.0f)
    };
    
    DrawTextEx(isFontLoaded ? boldFont : mainFont, text.c_str(), textPos, fontSize, 1.0f, textColor);
    
    return clicked;
}

bool LudoUIWidget::iconButton(Rectangle rect, const char* symbol, bool active) {
    Vector2 mousePos = LudoGame::getMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    bool clicked = false;
    
    Color bgCol = active ? Color{59, 130, 246, 255} : Color{30, 41, 59, 255}; // blue vs Slate 800
    if (hovered) {
        bgCol = active ? Color{37, 99, 235, 255} : Color{51, 65, 85, 255}; // bright
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            clicked = true;
        }
    }
    
    Rectangle scaledRect = {
        LudoGame::offsetX + rect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + rect.y * LudoGame::scaleFactor,
        rect.width * LudoGame::scaleFactor,
        rect.height * LudoGame::scaleFactor
    };
    
    DrawRectangleRounded(scaledRect, 0.25f, 4, bgCol);
    
    float fontSize = (isFontLoaded ? 18.0f : 10.0f) * LudoGame::scaleFactor;
    Vector2 textSize = MeasureTextEx(mainFont, symbol, fontSize, 1.0f);
    Vector2 textPos = {
        roundf(scaledRect.x + (scaledRect.width - textSize.x) / 2.0f),
        roundf(scaledRect.y + (scaledRect.height - textSize.y) / 2.0f)
    };
    DrawTextEx(mainFont, symbol, textPos, fontSize, 1.0f, WHITE);
    
    return clicked;
}

void LudoUIWidget::textBox(Rectangle rect, std::string& text, int maxChars, bool& active, int& cursorTick) {
    Vector2 mousePos = LudoGame::getMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        active = hovered;
    }
    
    if (active) {
        // Read keystrokes
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && (text.length() < (size_t)maxChars)) {
                text += (char)key;
            }
            key = GetCharPressed();
        }
        
        if (IsKeyPressed(KEY_BACKSPACE) && !text.empty()) {
            text.pop_back();
        }
    }
    
    Rectangle scaledRect = {
        LudoGame::offsetX + rect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + rect.y * LudoGame::scaleFactor,
        rect.width * LudoGame::scaleFactor,
        rect.height * LudoGame::scaleFactor
    };
    
    // Draw box
    Color bgCol = active ? Color{30, 41, 59, 255} : Color{15, 23, 42, 255};
    Color borderCol = active ? Color{59, 130, 246, 255} : Color{51, 65, 85, 255};
    
    DrawRectangleRounded(scaledRect, 0.15f, 4, bgCol);
    DrawRectangleRoundedLines(scaledRect, 0.15f, 4, 1.5f * LudoGame::scaleFactor, borderCol);
    
    // Draw text inside
    float fontSize = (isFontLoaded ? 16.0f : 10.0f) * LudoGame::scaleFactor;
    Vector2 textPos = { 
        roundf(scaledRect.x + 10 * LudoGame::scaleFactor), 
        roundf(scaledRect.y + (scaledRect.height - fontSize) / 2.0f) 
    };
    DrawTextEx(mainFont, text.c_str(), textPos, fontSize, 1.0f, WHITE);
    
    // Draw cursor
    if (active && (cursorTick / 30) % 2 == 0) {
        float textWidth = MeasureTextEx(mainFont, text.c_str(), fontSize, 1.0f).x;
        DrawRectangle(
            roundf(scaledRect.x + 12 * LudoGame::scaleFactor + textWidth), 
            roundf(scaledRect.y + (scaledRect.height - fontSize) / 2.0f), 
            roundf(2.0f * LudoGame::scaleFactor), 
            roundf(fontSize), 
            Color{59, 130, 246, 255}
        );
    }
}

void LudoUIWidget::typeSelector(Rectangle rect, std::string& selectedType) {
    Vector2 mousePos = LudoGame::getMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    bool clicked = false;
    
    if (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        clicked = true;
    }
    
    // Cycle modes
    if (clicked) {
        if (selectedType == "human") {
            selectedType = "ai-medium";
        } else if (selectedType == "ai-medium") {
            selectedType = "none";
        } else {
            selectedType = "human";
        }
    }
    
    std::string dispText = "Human";
    Color btnCol = Color{34, 197, 94, 255}; // Green
    if (selectedType == "ai-medium") {
        dispText = "AI (Medium)";
        btnCol = Color{59, 130, 246, 255}; // Blue
    } else if (selectedType == "none") {
        dispText = "Inactive";
        btnCol = Color{100, 116, 139, 255}; // Slate
    }
    
    if (hovered) {
        btnCol = ColorAlpha(btnCol, 0.85f);
    }
    
    Rectangle scaledRect = {
        LudoGame::offsetX + rect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + rect.y * LudoGame::scaleFactor,
        rect.width * LudoGame::scaleFactor,
        rect.height * LudoGame::scaleFactor
    };
    
    DrawRectangleRounded(scaledRect, 0.15f, 4, btnCol);
    
    float fontSize = (isFontLoaded ? 15.0f : 10.0f) * LudoGame::scaleFactor;
    Vector2 textSize = MeasureTextEx(boldFont, dispText.c_str(), fontSize, 1.0f);
    Vector2 textPos = {
        roundf(scaledRect.x + (scaledRect.width - textSize.x) / 2.0f),
        roundf(scaledRect.y + (scaledRect.height - textSize.y) / 2.0f)
    };
    DrawTextEx(boldFont, dispText.c_str(), textPos, fontSize, 1.0f, WHITE);
}

float LudoUIWidget::slider(Rectangle rect, const std::string& labelText, float value) {
    Vector2 mousePos = LudoGame::getMousePosition();
    
    Rectangle scaledRect = {
        LudoGame::offsetX + rect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + rect.y * LudoGame::scaleFactor,
        rect.width * LudoGame::scaleFactor,
        rect.height * LudoGame::scaleFactor
    };
    
    // Draw label
    float fontSize = (isFontLoaded ? 14.0f : 10.0f) * LudoGame::scaleFactor;
    DrawTextEx(mainFont, labelText.c_str(), Vector2{
        roundf(scaledRect.x), 
        roundf(scaledRect.y - 18 * LudoGame::scaleFactor)
    }, fontSize, 1.0f, Color{148, 163, 184, 255});
    
    // Draw bar background
    DrawRectangleRounded(scaledRect, 0.5f, 4, Color{30, 41, 59, 255});
    
    // Slide range
    float handleX = scaledRect.x + value * scaledRect.width;
    bool clicked = CheckCollisionPointCircle(mousePos, Vector2{rect.x + value * rect.width, rect.y + rect.height/2.0f}, 8.0f) || 
                   (CheckCollisionPointRec(mousePos, rect) && IsMouseButtonDown(MOUSE_LEFT_BUTTON));
                   
    if (clicked && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        value = (mousePos.x - rect.x) / rect.width;
        if (value < 0.0f) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
    }
    
    // Draw progress bar
    Rectangle progress = scaledRect;
    progress.width = value * scaledRect.width;
    DrawRectangleRounded(progress, 0.5f, 4, Color{59, 130, 246, 255});
    
    // Draw handle
    DrawCircle(scaledRect.x + value * scaledRect.width, scaledRect.y + scaledRect.height/2.0f, 8.0f * LudoGame::scaleFactor, WHITE);
    DrawCircle(scaledRect.x + value * scaledRect.width, scaledRect.y + scaledRect.height/2.0f, 6.0f * LudoGame::scaleFactor, Color{59, 130, 246, 255});
    
    return value;
}

void LudoUIWidget::drawTextWithShadow(const std::string& text, float posX, float posY, float fontSize, Color color, Font font) {
    float scaledX = roundf(LudoGame::offsetX + posX * LudoGame::scaleFactor);
    float scaledY = roundf(LudoGame::offsetY + posY * LudoGame::scaleFactor);
    float scaledSize = fontSize * LudoGame::scaleFactor;
    
    // Offset shadow in physical space
    DrawTextEx(font, text.c_str(), Vector2{roundf(scaledX + 2.0f * LudoGame::scaleFactor), roundf(scaledY + 2.0f * LudoGame::scaleFactor)}, scaledSize, 1.0f, ColorAlpha(BLACK, 0.3f));
    DrawTextEx(font, text.c_str(), Vector2{scaledX, scaledY}, scaledSize, 1.0f, color);
}

bool LudoUIWidget::modeButton(Rectangle rect, const std::string& title, const std::string& desc, bool selected, int modeType) {
    Vector2 mousePos = LudoGame::getMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    bool clicked = false;
    
    float scale = 1.0f;
    Rectangle drawRect = rect;
    
    if (hovered) {
        scale = 1.03f;
        drawRect.width *= scale;
        drawRect.height *= scale;
        drawRect.x -= (drawRect.width - rect.width) / 2.0f;
        drawRect.y -= (drawRect.height - rect.height) / 2.0f;
        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            clicked = true;
        }
    }
    
    Rectangle scaledDrawRect = {
        LudoGame::offsetX + drawRect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + drawRect.y * LudoGame::scaleFactor,
        drawRect.width * LudoGame::scaleFactor,
        drawRect.height * LudoGame::scaleFactor
    };
    
    // Draw button shadow
    Rectangle scaledShadow = scaledDrawRect;
    scaledShadow.y += 3.0f * LudoGame::scaleFactor;
    DrawRectangleRounded(scaledShadow, 0.25f, 4, ColorAlpha(BLACK, 0.1f));
    
    // Select base pastel colors
    Color baseCol = Color{30, 41, 59, 120};
    Color textCol = Color{148, 163, 184, 255};
    Color descCol = Color{100, 116, 139, 180};
    Color borderCol = Color{51, 65, 85, 100};
    
    if (selected) {
        baseCol = Color{30, 41, 59, 255};
        textCol = WHITE;
        descCol = Color{226, 232, 240, 255};
        borderCol = (modeType == 0) ? Color{59, 130, 246, 255} : Color{168, 85, 247, 255};
    }
    
    // Draw button body
    DrawRectangleRounded(scaledDrawRect, 0.25f, 4, baseCol);
    
    // Draw outline border
    DrawRectangleRoundedLines(scaledDrawRect, 0.25f, 4, (selected ? 2.0f : 1.0f) * LudoGame::scaleFactor, borderCol);
    
    // Draw Icon on the left
    float iconSize = 40.0f * LudoGame::scaleFactor;
    float iconX = scaledDrawRect.x + 15.0f * LudoGame::scaleFactor;
    float iconY = scaledDrawRect.y + (scaledDrawRect.height - iconSize) / 2.0f;
    
    if (modeType == 0) {
        // Draw Robot Face
        float rx = iconX + iconSize * 0.1f;
        float ry = iconY + iconSize * 0.25f;
        float rw = iconSize * 0.8f;
        float rh = iconSize * 0.65f;
        
        // Head
        DrawRectangleRounded(Rectangle{rx, ry, rw, rh}, 0.2f, 4, textCol);
        
        // Antenna
        DrawLineEx(Vector2{iconX + iconSize * 0.5f, iconY + iconSize * 0.05f}, Vector2{iconX + iconSize * 0.5f, ry}, 3.0f * LudoGame::scaleFactor, textCol);
        DrawCircle(iconX + iconSize * 0.5f, iconY + iconSize * 0.05f, 3.5f * LudoGame::scaleFactor, textCol);
        
        // Eyes
        DrawCircle(rx + rw * 0.3f, ry + rh * 0.4f, 4.0f * LudoGame::scaleFactor, baseCol);
        DrawCircle(rx + rw * 0.7f, ry + rh * 0.4f, 4.0f * LudoGame::scaleFactor, baseCol);
        
        // Mouth
        DrawLineEx(Vector2{rx + rw * 0.35f, ry + rh * 0.75f}, Vector2{rx + rw * 0.65f, ry + rh * 0.75f}, 2.0f * LudoGame::scaleFactor, baseCol);
        
        // Ears
        DrawRectangle(rx - 3.0f * LudoGame::scaleFactor, ry + rh * 0.3f, 3.0f * LudoGame::scaleFactor, rh * 0.4f, textCol);
        DrawRectangle(rx + rw, ry + rh * 0.3f, 3.0f * LudoGame::scaleFactor, rh * 0.4f, textCol);
    } else {
        // Draw two figures holding hands
        float cx1 = iconX + iconSize * 0.32f;
        float cx2 = iconX + iconSize * 0.68f;
        float cy = iconY + iconSize * 0.28f;
        float hr = iconSize * 0.18f; // head radius
        
        // Heads
        DrawCircle(cx1, cy, hr, textCol);
        DrawCircle(cx2, cy, hr, textCol);
        
        // Bodies
        float bodyY = cy + hr + 2.0f * LudoGame::scaleFactor;
        float bodyH = iconSize * 0.45f;
        
        DrawRectangleRounded(Rectangle{cx1 - hr, bodyY, hr * 2.0f, bodyH}, 0.5f, 4, textCol);
        DrawRectangleRounded(Rectangle{cx2 - hr, bodyY, hr * 2.0f, bodyH}, 0.5f, 4, textCol);
        
        // Hand bridge connection
        DrawLineEx(Vector2{cx1, bodyY + bodyH * 0.3f}, Vector2{cx2, bodyY + bodyH * 0.3f}, 3.0f * LudoGame::scaleFactor, textCol);
    }
    
    // Draw Title and Description
    float textX = drawRect.x + 70.0f;
    float titleY = drawRect.y + 16.0f;
    float descY = drawRect.y + 40.0f;
    
    float titleSize = 16.5f * LudoGame::scaleFactor;
    float descSize = 11.5f * LudoGame::scaleFactor;
    
    DrawTextEx(boldFont, title.c_str(), Vector2{roundf(LudoGame::offsetX + textX * LudoGame::scaleFactor), roundf(LudoGame::offsetY + titleY * LudoGame::scaleFactor)}, titleSize, 1.0f, textCol);
    DrawTextEx(mainFont, desc.c_str(), Vector2{roundf(LudoGame::offsetX + textX * LudoGame::scaleFactor), roundf(LudoGame::offsetY + descY * LudoGame::scaleFactor)}, descSize, 1.0f, descCol);
    
    return clicked;
}

bool LudoUIWidget::toggleSwitch(Rectangle rect, bool& value, const std::string& label) {
    Vector2 mousePos = LudoGame::getMousePosition();
    
    // Check click on the switch track
    bool hovered = CheckCollisionPointRec(mousePos, rect);
    
    // Also check click on the label next to it (e.g. left of the switch)
    float fontSize = (isFontLoaded ? 14.0f : 10.0f);
    float labelWidth = MeasureTextEx(boldFont, label.c_str(), fontSize * LudoGame::scaleFactor, 1.0f).x / LudoGame::scaleFactor;
    Rectangle labelRect = { rect.x - labelWidth - 10.0f, rect.y, labelWidth + 10.0f, rect.height };
    bool labelHovered = CheckCollisionPointRec(mousePos, labelRect);
    
    bool clicked = false;
    if ((hovered || labelHovered) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        value = !value;
        clicked = true;
    }
    
    Rectangle scaledRect = {
        LudoGame::offsetX + rect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + rect.y * LudoGame::scaleFactor,
        rect.width * LudoGame::scaleFactor,
        rect.height * LudoGame::scaleFactor
    };
    
    // Draw label text
    float scaledFontSize = fontSize * LudoGame::scaleFactor;
    Vector2 textPos = {
        roundf(LudoGame::offsetX + labelRect.x * LudoGame::scaleFactor),
        roundf(scaledRect.y + (scaledRect.height - scaledFontSize) / 2.0f)
    };
    DrawTextEx(boldFont, label.c_str(), textPos, scaledFontSize, 1.0f, Color{30, 41, 59, 255});
    
    // Draw switch track
    Color trackCol = value ? Color{115, 164, 176, 255} : Color{148, 163, 184, 150};
    DrawRectangleRounded(scaledRect, 0.5f, 4, trackCol);
    
    // Draw switch knob (white circle)
    float r = scaledRect.height * 0.42f;
    float knobX = value ? (scaledRect.x + scaledRect.width - r - 3.0f * LudoGame::scaleFactor) : (scaledRect.x + r + 3.0f * LudoGame::scaleFactor);
    DrawCircle(knobX, scaledRect.y + scaledRect.height/2.0f, r, WHITE);
    
    return clicked;
}

void LudoUIWidget::playerCard(Rectangle rect, int playerId, const std::string& name) {
    Rectangle scaledRect = {
        LudoGame::offsetX + rect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + rect.y * LudoGame::scaleFactor,
        rect.width * LudoGame::scaleFactor,
        rect.height * LudoGame::scaleFactor
    };
    
    // Background colors: Green, Yellow, Blue, Red
    Color bgColors[4] = {
        Color{76, 175, 80, 255},   // Green
        Color{251, 192, 45, 255},  // Yellow
        Color{33, 150, 243, 255},  // Blue
        Color{244, 67, 54, 255}    // Red
    };
    
    // Draw shadow
    Rectangle shadow = scaledRect;
    shadow.y += 3.0f * LudoGame::scaleFactor;
    DrawRectangleRounded(shadow, 0.15f, 4, ColorAlpha(BLACK, 0.12f));
    
    // Draw card background
    DrawRectangleRounded(scaledRect, 0.15f, 4, bgColors[playerId]);
    
    // Draw player name at the top
    float fontSize = 16.0f * LudoGame::scaleFactor;
    Vector2 textSz = MeasureTextEx(boldFont, name.c_str(), fontSize, 1.0f);
    Vector2 textPos = {
        roundf(scaledRect.x + (scaledRect.width - textSz.x) / 2.0f),
        roundf(scaledRect.y + 15.0f * LudoGame::scaleFactor)
    };
    
    Color textCol = (playerId == 1) ? Color{93, 64, 55, 255} : WHITE; // dark brown for Yellow
    DrawTextEx(boldFont, name.c_str(), textPos, fontSize, 1.0f, textCol);
    
    // Draw pawn texture silhouette/image in the center
    if (isFontLoaded && LudoBoard::isLoaded && LudoBoard::pieceTextures[playerId].id != 0) {
        float pW = 55.0f * LudoGame::scaleFactor;
        float pH = 55.0f * LudoGame::scaleFactor;
        Rectangle dest = {
            scaledRect.x + (scaledRect.width - pW) / 2.0f,
            scaledRect.y + 50.0f * LudoGame::scaleFactor,
            pW,
            pH
        };
        DrawTexturePro(
            LudoBoard::pieceTextures[playerId],
            Rectangle{0, 0, (float)LudoBoard::pieceTextures[playerId].width, (float)LudoBoard::pieceTextures[playerId].height},
            dest,
            Vector2{0, 0},
            0.0f,
            WHITE
        );
    }
}

void LudoUIWidget::playerConfigCard(Rectangle rect, int playerId, std::string& name, std::string& type, bool& nameActive, int cursorTick) {
    Vector2 mousePos = LudoGame::getMousePosition();
    
    // Scale card rect to physical screen coordinates
    Rectangle scaledCard = {
        LudoGame::offsetX + rect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + rect.y * LudoGame::scaleFactor,
        rect.width * LudoGame::scaleFactor,
        rect.height * LudoGame::scaleFactor
    };
    
    // 1. Draw Card Shadow & Background
    Rectangle shadow = scaledCard;
    shadow.y += 3.0f * LudoGame::scaleFactor;
    DrawRectangleRounded(shadow, 0.1f, 4, ColorAlpha(BLACK, 0.12f));
    
    // Dark slate card body
    DrawRectangleRounded(scaledCard, 0.1f, 4, Color{30, 41, 59, 220});
    
    // Colored header/banner
    Color bgColors[4] = {
        Color{76, 175, 80, 255},   // Green
        Color{251, 192, 45, 255},  // Yellow
        Color{33, 150, 243, 255},  // Blue
        Color{244, 67, 54, 255}    // Red
    };
    Rectangle headerRect = { rect.x, rect.y, rect.width, 35.0f };
    Rectangle scaledHeader = {
        LudoGame::offsetX + headerRect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + headerRect.y * LudoGame::scaleFactor,
        headerRect.width * LudoGame::scaleFactor,
        headerRect.height * LudoGame::scaleFactor
    };
    DrawRectangleRounded(scaledHeader, 0.1f, 4, bgColors[playerId]);
    DrawRectangle(scaledHeader.x, scaledHeader.y + scaledHeader.height - 5.0f * LudoGame::scaleFactor, scaledHeader.width, 5.0f * LudoGame::scaleFactor, bgColors[playerId]);
    
    // Colored card outline border
    DrawRectangleRoundedLines(scaledCard, 0.1f, 4, 1.5f * LudoGame::scaleFactor, bgColors[playerId]);
    
    // Header Label
    std::string colorNames[4] = { "GREEN", "YELLOW", "BLUE", "RED" };
    float headerFontSize = 14.0f * LudoGame::scaleFactor;
    Vector2 headerTextSz = MeasureTextEx(boldFont, colorNames[playerId].c_str(), headerFontSize, 1.0f);
    Vector2 headerTextPos = {
        roundf(scaledHeader.x + (scaledHeader.width - headerTextSz.x) / 2.0f),
        roundf(scaledHeader.y + (scaledHeader.height - headerTextSz.y) / 2.0f)
    };
    Color headerTextCol = (playerId == 1) ? Color{93, 64, 55, 255} : WHITE;
    DrawTextEx(boldFont, colorNames[playerId].c_str(), headerTextPos, headerFontSize, 1.0f, headerTextCol);
    
    // 2. Draw Name Textbox
    Rectangle tbRect = { rect.x + 15.0f, rect.y + 48.0f, rect.width - 30.0f, 32.0f };
    textBox(tbRect, name, 12, nameActive, cursorTick);
    
    // 3. Draw Mode Button
    Rectangle mbRect = { rect.x + 15.0f, rect.y + 92.0f, rect.width - 30.0f, 32.0f };
    bool mbHovered = CheckCollisionPointRec(mousePos, mbRect);
    
    if (mbHovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
        if (type == "human") {
            type = "ai-medium";
        } else if (type == "ai-medium") {
            type = "none";
        } else {
            type = "human";
        }
        LudoAudio::playSFX("piece-move");
    }
    
    Rectangle scaledMb = {
        LudoGame::offsetX + mbRect.x * LudoGame::scaleFactor,
        LudoGame::offsetY + mbRect.y * LudoGame::scaleFactor,
        mbRect.width * LudoGame::scaleFactor,
        mbRect.height * LudoGame::scaleFactor
    };
    
    Color mbCol = Color{148, 163, 184, 255}; // Gray for Inactive
    std::string mbText = "INACTIVE";
    if (type == "human") {
        mbCol = Color{34, 197, 94, 255}; // Green
        mbText = "HUMAN";
    } else if (type == "ai-medium") {
        mbCol = Color{59, 130, 246, 255}; // Blue
        mbText = "COMPUTER";
    }
    
    if (mbHovered) {
        mbCol = ColorAlpha(mbCol, 0.85f);
    }
    
    DrawRectangleRounded(scaledMb, 0.15f, 4, mbCol);
    
    float mbFontSize = 12.0f * LudoGame::scaleFactor;
    Vector2 mbTextSz = MeasureTextEx(boldFont, mbText.c_str(), mbFontSize, 1.0f);
    Vector2 mbTextPos = {
        roundf(scaledMb.x + (scaledMb.width - mbTextSz.x) / 2.0f),
        roundf(scaledMb.y + (scaledMb.height - mbTextSz.y) / 2.0f)
    };
    DrawTextEx(boldFont, mbText.c_str(), mbTextPos, mbFontSize, 1.0f, WHITE);
    
    // 4. Draw Player Piece silhouette
    if (LudoBoard::isLoaded && LudoBoard::pieceTextures[playerId].id != 0) {
        float pW = 35.0f * LudoGame::scaleFactor;
        float pH = 35.0f * LudoGame::scaleFactor;
        Rectangle dest = {
            scaledCard.x + (scaledCard.width - pW) / 2.0f,
            scaledCard.y + 138.0f * LudoGame::scaleFactor,
            pW,
            pH
        };
        
        Color pieceTint = WHITE;
        if (type == "none") {
            pieceTint = ColorAlpha(WHITE, 0.25f);
        }
        
        DrawTexturePro(
            LudoBoard::pieceTextures[playerId],
            Rectangle{0, 0, (float)LudoBoard::pieceTextures[playerId].width, (float)LudoBoard::pieceTextures[playerId].height},
            dest,
            Vector2{0, 0},
            0.0f,
            pieceTint
        );
    }
}
