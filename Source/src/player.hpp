#pragma once
#include <string>
#include <vector>
#include "piece.hpp"

struct LudoPlayer {
    int id;
    std::string name;
    std::string type; // "human", "ai-medium", "none"
    std::string color; // "green", "yellow", "blue", "red"
    std::vector<LudoPiece> tokens;

    LudoPlayer(int id, const std::string& name, const std::string& type, const std::string& color);
    
    void initTokens();
    int getActiveTokensCount() const;
    int getFinishedTokensCount() const;
    bool isFinished() const;
};
