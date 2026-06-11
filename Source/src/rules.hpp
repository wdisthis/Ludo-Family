#pragma once
#include <vector>
#include "piece.hpp"

class LudoRules {
public:
    static const std::vector<std::vector<int>> safeCells;
    
    static bool canTokenMove(int playerId, const LudoPiece& token, int roll);
    static bool isSafeCell(int gx, int gy);
};
