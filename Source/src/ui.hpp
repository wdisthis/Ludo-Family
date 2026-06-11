#pragma once
#include "raylib.h"
#include <string>
#include <vector>

class LudoUIWidget {
public:
    static Font mainFont;
    static Font boldFont;
    static bool isFontLoaded;

    static void initFonts();
    static void unloadFonts();
    
    // Premium custom button with subtle hover animation and rounded corners
    static bool button(Rectangle rect, const std::string& text, Color baseColor, Color hoverColor, Color textColor, float roundness = 0.25f);
    
    // Icon button
    static bool iconButton(Rectangle rect, const char* symbol, bool active = false);
    
    // Sleek text field
    static void textBox(Rectangle rect, std::string& text, int maxChars, bool& active, int& cursorTick);
    
    // Player type selector (Human / AI / Inactive dropdown-style options)
    static void typeSelector(Rectangle rect, std::string& selectedType);
    
    // Slider for volume controls
    static float slider(Rectangle rect, const std::string& labelText, float value);
    
    // Utility to draw text with shadow
    static void drawTextWithShadow(const std::string& text, float posX, float posY, float fontSize, Color color, Font font);
};
