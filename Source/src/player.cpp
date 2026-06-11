#include "player.hpp"

LudoPlayer::LudoPlayer(int id, const std::string& name, const std::string& type, const std::string& color)
    : id(id), name(name), type(type), color(color) {
    initTokens();
}

void LudoPlayer::initTokens() {
    tokens.clear();
    for (int i = 0; i < 4; i++) {
        tokens.push_back(LudoPiece(i, id, color));
    }
}

int LudoPlayer::getActiveTokensCount() const {
    int count = 0;
    for (const auto& token : tokens) {
        if (token.step >= 0 && token.step < 56) {
            count++;
        }
    }
    return count;
}

int LudoPlayer::getFinishedTokensCount() const {
    int count = 0;
    for (const auto& token : tokens) {
        if (token.step == 56) {
            count++;
        }
    }
    return count;
}

bool LudoPlayer::isFinished() const {
    return getFinishedTokensCount() == 4;
}
