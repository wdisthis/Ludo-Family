#pragma once
#include <vector>
#include "player.hpp"
#include "piece.hpp"

class LudoAI {
public:
    // Returns index of the chosen token (0-3) in player.tokens or -1 if none is eligible
    static int selectMove(const LudoPlayer& player, int roll, const std::vector<LudoPlayer>& players);
};
