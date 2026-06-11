#include "ai.hpp"
#include "rules.hpp"
#include "path.hpp"
#include <cstdlib>
#include <ctime>

int LudoAI::selectMove(const LudoPlayer& player, int roll, const std::vector<LudoPlayer>& players) {
    std::vector<int> eligibleIndices;
    for (int i = 0; i < 4; i++) {
        if (LudoRules::canTokenMove(player.id, player.tokens[i], roll)) {
            eligibleIndices.push_back(i);
        }
    }
    
    if (eligibleIndices.empty()) return -1;
    
    // 1. Capture priority rule
    for (int idx : eligibleIndices) {
        const auto& token = player.tokens[idx];
        int targetStep = (token.step == -1) ? 0 : (token.step + roll);
        std::vector<int> targetCoords = LudoPath::getPathCoordinates(player.id, targetStep, token.id);
        
        if (!LudoRules::isSafeCell(targetCoords[0], targetCoords[1])) {
            for (const auto& otherPlayer : players) {
                if (otherPlayer.id == player.id || otherPlayer.type == "none" || otherPlayer.isFinished()) {
                    continue;
                }
                
                for (const auto& otherToken : otherPlayer.tokens) {
                    if (otherToken.step == -1 || otherToken.step == 56) continue;
                    
                    std::vector<int> otherCoords = otherToken.getCoordinates();
                    if (otherCoords[0] == targetCoords[0] && otherCoords[1] == targetCoords[1]) {
                        return idx; // Found capturing move!
                    }
                }
            }
        }
    }
    
    // 2. Base release rule (step = -1, roll = 6)
    for (int idx : eligibleIndices) {
        if (player.tokens[idx].step == -1 && roll == 6) {
            return idx;
        }
    }
    
    // 3. Landing on safe cell rule
    for (int idx : eligibleIndices) {
        const auto& token = player.tokens[idx];
        int targetStep = (token.step == -1) ? 0 : (token.step + roll);
        std::vector<int> targetCoords = LudoPath::getPathCoordinates(player.id, targetStep, token.id);
        if (LudoRules::isSafeCell(targetCoords[0], targetCoords[1])) {
            return idx;
        }
    }
    
    // 4. Furthest step rule
    int bestIdx = -1;
    int maxStep = -2;
    for (int idx : eligibleIndices) {
        if (player.tokens[idx].step > maxStep) {
            maxStep = player.tokens[idx].step;
            bestIdx = idx;
        }
    }
    if (bestIdx != -1) return bestIdx;
    
    // 5. Random fallback
    int randIdx = std::rand() % eligibleIndices.size();
    return eligibleIndices[randIdx];
}
