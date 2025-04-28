#include <iostream>
#include <vector>
#include <random>

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(0, 9); // 0~9 ¡ﬂ

constexpr int MAP_WIDTH = 50;
constexpr int MAP_HEIGHT = 50;

int main()
{
    // 1. 50x50 ∏  ¡ÿ∫Ò
    std::vector<std::vector<int>> map(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, 0));

    for (int z = 0; z < MAP_HEIGHT; ++z)
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            int r = dis(gen);
            map[z][x] = (r < 2) ? 1 : 0; // æ‡ 20% »Æ∑¸∑Œ ∫Æ
        }
    }

    // 3. ∏  √‚∑¬
    std::cout << "[∏  ªÛ≈¬ √‚∑¬]\n";
    for (int z = 0; z < MAP_HEIGHT; ++z)
    {
        for (int x = 0; x < MAP_WIDTH; ++x)
        {
            std::cout << map[z][x];
        }
        std::cout << "\n"; 
    }

    return 0;
}