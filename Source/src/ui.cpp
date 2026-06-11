#include "ui.hpp"
#include <iostream>
#include <cmath>
#include "game.hpp"

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
