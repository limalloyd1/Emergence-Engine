#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using Grid = std::vector<std::string>;

void growPatch(Grid& map, int startX, int startY, int steps, std::mt19937& rng) {
    int rows = map.size();
    int cols = map[0].size();

    int x = startX;
    int y = startY;

    std::vector<std::pair<int, int>> dirs = {
        {0, -1}, {0, 1}, {-1, 0}, {1, 0}
    };

    for (int i = 0; i < steps; i++) {
        map[y][x] = '*';

        std::shuffle(dirs.begin(), dirs.end(), rng);

        x += dirs[0].first;
        y += dirs[0].second;

        x = std::clamp(x, 1, cols - 2);
        y = std::clamp(y, 1, rows - 2);
    }
}

Grid generateMap(int rows, int cols, int patches, int patchSize) {
    Grid map(rows, std::string(cols, ' '));

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_int_distribution<int> xDist(1, cols - 2);
    std::uniform_int_distribution<int> yDist(1, rows - 2);

    for (int i = 0; i < patches; i++) {
        int x = xDist(rng);
        int y = yDist(rng);

        growPatch(map, x, y, patchSize, rng);
    }

    return map;
}

void saveMap(const Grid& map, const std::string& path) {
    std::ofstream out(path);

    if (!out.is_open()) {
        throw std::runtime_error("Could not write map: " + path);
    }

    for (const std::string& line : map) {
        out << line << '\n';
    }
}

int main() {
    std::filesystem::create_directories("maps");

    int rows = 64;
    int cols = 128;
    int mapCount = 10;

    int patches = 18;
    int patchSize = 150;

    for (int i = 1; i <= mapCount; i++) {
        Grid map = generateMap(rows, cols, patches, patchSize);

        std::string path = "maps/map_" + std::to_string(i) + ".txt";
        saveMap(map, path);

        std::cout << "[+] Wrote " << path << '\n';
    }

    return 0;
}
