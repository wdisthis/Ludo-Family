#pragma once
#include <string>
#include <vector>

struct LudoPiece {
    int id;          // Token index (0 to 3)
    int playerId;    // Owner player ID (0 to 3)
    std::string color; // "green", "yellow", "blue", "red"
    int step;        // -1 = base, 0-55 = track/home-run, 56 = finished

    LudoPiece(int id, int playerId, const std::string& color);
    
    bool moveStep(int count);
    void resetToBase();
    std::vector<int> getCoordinates() const;
};
