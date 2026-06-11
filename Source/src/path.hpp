#pragma once
#include <vector>
#include <map>

class LudoPath {
public:
    static const std::vector<std::vector<int>> commonPath;
    static const std::map<int, int> startIndices;
    static const std::map<int, std::vector<std::vector<int>>> homeRuns;
    static const std::map<int, std::vector<int>> finishes;
    static const std::map<int, std::vector<std::vector<int>>> baseSockets;

    static std::vector<int> getPathCoordinates(int playerId, int step, int tokenIndex = 0);
};
