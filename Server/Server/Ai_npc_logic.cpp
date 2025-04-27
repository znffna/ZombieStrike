#include <vector>
#include <queue>
#include <unordered_set>
#include <cmath>

struct Node
{
    int x, z;
    float gCost;                                    // 시작점 → 현재까지 이동비용
    float hCost;                                    // 현재 → 목표까지 추정비용
    float fCost() const { return gCost + hCost; }
    Node* parent = nullptr;
};


class AStar
{
public:
    AStar(const std::vector<std::vector<int>>& map)
        : m_map(map), m_width(map[0].size()), m_length(map.size()) {
    }

    std::vector<std::pair<int, int>> FindPath(int startX, int startZ, int endX, int endZ);

private:
    std::vector<std::vector<int>> m_map; // 0: passable, 1: blocked
    int m_width;
    int m_length;

    float Heuristic(int x1, int z1, int x2, int z2)
    {
        return std::abs(x1 - x2) + std::abs(z1 - z2); // Manhattan Distance
    }
};
