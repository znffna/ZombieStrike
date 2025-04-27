#pragma once
#include <vector>
#include <queue>
#include <unordered_set>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <mutex>

const int my_gCost = 1.0f;

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
    std::vector<std::vector<int>> m_map;              // 0: passable, 1: blocked
    int m_width;
    int m_length;

    float Heuristic(int x1, int z1, int x2, int z2)
    {
        return std::abs(x1 - x2) + std::abs(z1 - z2); // Manhattan Distance
    }
};

std::vector<std::pair<int, int>> AStar::FindPath(int startX, int startZ, int endX, int endZ)  //delete 사용
{
    auto cmp = [](Node* a, Node* b) { return a->fCost() > b->fCost(); };                     // 우선순위 판단
    std::priority_queue<  Node*, std::vector<Node*>, decltype(cmp)  > openList(cmp);         // openList .  fCost가 가장 작은 Node
    std::unordered_set<int> closedList;                                                      // 이미 방문한 노드를 기록

    Node* startNode = new Node{ startX, startZ, 0.0f, Heuristic(startX, startZ, endX, endZ) }; // gCost=0, hCost=추정거리
    openList.push(startNode);

    std::vector<std::pair<int, int>> path;

    while (!openList.empty())  
    {
        Node* current = openList.top();
        openList.pop();

        int key = current->z * m_width + current->x;
        if (closedList.find(key) != closedList.end())
        {
            delete current; 
            continue;
        }
        closedList.insert(key);

        if (current->x == endX && current->z == endZ)
        {
            Node* node = current;
            while (node)
            {
                path.emplace_back(node->x, node->z);
                node = node->parent;
            }
            std::reverse(path.begin(), path.end());

            delete current; 
            break;
        }
        //if문 사용x
        const int dirX[4] = { 1, -1, 0, 0 };
        const int dirZ[4] = { 0, 0, 1, -1 };

        for (int dir = 0; dir < 4; ++dir)
        {
            int nextX = current->x + dirX[dir];
            int nextZ = current->z + dirZ[dir];

            if (nextX < 0 || nextX >= m_width || nextZ < 0 || nextZ >= m_length)
                continue;
            if (m_map[nextZ][nextX] != 0)
                continue;

            int nextKey = nextZ * m_width + nextX;
            if (closedList.find(nextKey) != closedList.end())
                continue;

            Node* neighbor = new Node{ nextX, nextZ };
            neighbor->gCost = current->gCost + my_gCost;            // 한 칸 이동 비용 1
            neighbor->hCost = Heuristic(nextX, nextZ, endX, endZ);
            neighbor->parent = current;
            openList.push(neighbor);
        }
    }

    return path;
}

// =========================================================

class ZombieAI
{
public:
    ZombieAI(const std::vector<std::vector<int>>& map, int id)
        : m_astar(map), m_id(id) {
    }

    void SetPosition(float x, float z) {
        m_x = x;
        m_z = z;
    }

    void SetPlayerPosition(float x, float z) {
        m_playerX = x;
        m_playerZ = z;
    }

    void Update() {
        auto path = m_astar.FindPath((int)m_x, (int)m_z, (int)m_playerX, (int)m_playerZ);
        if (path.size() >= 2) {
            m_x = static_cast<float>(path[1].first);
            m_z = static_cast<float>(path[1].second);
        }
    }

    int GetID() const { return m_id; }
    float GetX() const { return m_x; }
    float GetZ() const { return m_z; }

private:
    AStar m_astar;
    int m_id;
    float m_x = 0.0f;
    float m_z = 0.0f;
    float m_playerX = 0.0f;
    float m_playerZ = 0.0f;
}; 

// =========================================================