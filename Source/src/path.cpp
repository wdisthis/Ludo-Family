#include "path.hpp"

const std::vector<std::vector<int>> LudoPath::commonPath = {
    {6, 13}, {6, 12}, {6, 11}, {6, 10}, {6, 9},
    {5, 8}, {4, 8}, {3, 8}, {2, 8}, {1, 8}, {0, 8},
    {0, 7},
    {0, 6}, {1, 6}, {2, 6}, {3, 6}, {4, 6}, {5, 6},
    {6, 5}, {6, 4}, {6, 3}, {6, 2}, {6, 1}, {6, 0},
    {7, 0},
    {8, 0}, {8, 1}, {8, 2}, {8, 3}, {8, 4}, {8, 5},
    {9, 6}, {10, 6}, {11, 6}, {12, 6}, {13, 6}, {14, 6},
    {14, 7},
    {14, 8}, {13, 8}, {12, 8}, {11, 8}, {10, 8}, {9, 8},
    {8, 9}, {8, 10}, {8, 11}, {8, 12}, {8, 13}, {8, 14},
    {7, 14}, {6, 14}
};

const std::map<int, int> LudoPath::startIndices = {
    {0, 13}, // Green starts at [1, 6] (commonPath[13])
    {1, 26}, // Yellow starts at [8, 1] (commonPath[26])
    {2, 39}, // Blue starts at [13, 8] (commonPath[39])
    {3, 0}   // Red starts at [6, 13] (commonPath[0])
};

const std::map<int, std::vector<std::vector<int>>> LudoPath::homeRuns = {
    {0, {{1, 7}, {2, 7}, {3, 7}, {4, 7}, {5, 7}}},
    {1, {{7, 1}, {7, 2}, {7, 3}, {7, 4}, {7, 5}}},
    {2, {{13, 7}, {12, 7}, {11, 7}, {10, 7}, {9, 7}}},
    {3, {{7, 13}, {7, 12}, {7, 11}, {7, 10}, {7, 9}}}
};

const std::map<int, std::vector<int>> LudoPath::finishes = {
    {0, {6, 7}},
    {1, {7, 6}},
    {2, {8, 7}},
    {3, {7, 8}}
};

const std::map<int, std::vector<std::vector<int>>> LudoPath::baseSockets = {
    {0, {{2, 2}, {4, 2}, {2, 4}, {4, 4}}},
    {1, {{11, 2}, {13, 2}, {11, 4}, {13, 4}}},
    {2, {{11, 11}, {13, 11}, {11, 13}, {13, 13}}},
    {3, {{2, 11}, {4, 11}, {2, 13}, {4, 13}}}
};

std::vector<int> LudoPath::getPathCoordinates(int playerId, int step, int tokenIndex) {
    if (step == -1) {
        auto it = baseSockets.find(playerId);
        if (it != baseSockets.end() && tokenIndex >= 0 && tokenIndex < 4) {
            return it->second[tokenIndex];
        }
    }
    if (step >= 0 && step <= 50) {
        auto startIt = startIndices.find(playerId);
        if (startIt != startIndices.end()) {
            int startIndex = startIt->second;
            int index = (startIndex + step) % 52;
            return commonPath[index];
        }
    }
    if (step >= 51 && step <= 55) {
        auto homeIt = homeRuns.find(playerId);
        if (homeIt != homeRuns.end() && (step - 51) < (int)homeIt->second.size()) {
            return homeIt->second[step - 51];
        }
    }
    if (step >= 56) {
        auto finIt = finishes.find(playerId);
        if (finIt != finishes.end()) {
            return finIt->second;
        }
    }
    return {7, 7};
}
