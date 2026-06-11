#include "rules.hpp"

const std::vector<std::vector<int>> LudoRules::safeCells = {
    {6, 13}, {1, 6}, {8, 1}, {13, 8},
    {8, 12}, {2, 8}, {6, 2}, {12, 6}
};

bool LudoRules::canTokenMove(int playerId, const LudoPiece& token, int roll) {
    if (token.step == -1) {
        return roll == 6;
    }
    return (token.step + roll) <= 56;
}

bool LudoRules::isSafeCell(int gx, int gy) {
    for (const auto& cell : safeCells) {
        if (cell[0] == gx && cell[1] == gy) {
            return true;
        }
    }
    return false;
}
