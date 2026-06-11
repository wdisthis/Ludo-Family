#include "piece.hpp"
#include "path.hpp"

LudoPiece::LudoPiece(int id, int playerId, const std::string& color)
    : id(id), playerId(playerId), color(color), step(-1) {}

bool LudoPiece::moveStep(int count) {
    if (step == -1 && count == 6) {
        step = 0;
        return true;
    }
    if (step >= 0 && (step + count) <= 56) {
        step += count;
        return true;
    }
    return false;
}

void LudoPiece::resetToBase() {
    step = -1;
}

std::vector<int> LudoPiece::getCoordinates() const {
    return LudoPath::getPathCoordinates(playerId, step, id);
}
