#pragma once
#include <vector>
#include <utility>
#include <fstream>
#include <string>
#include <unordered_map>
#include "../../protocol.h"


class ZombieAI
{
public:
    class AStar;

    ZombieAI(const std::vector<std::vector<int>>& map, int id);
    void SetPosition(float x, float z);
    void SetPlayerPosition(float x, float z);
    void FindPath();
    void Update(const std::vector<Vec3>& playerPositions, const std::vector<ZombieAI*>& allZombies);
    std::vector<std::pair<int, int>> FindPathToPlayer();

    int GetID() const;
    Vec3 GetPosition() const { return Vec3{ m_x, 0.0f, m_z }; }
    SIZE2 GetHP() const { return m_hp; }
    const std::vector<std::pair<int, int>>& GetPath() const;

    float GetX() const;
    float GetZ() const;
    float GetPlayerX() const;
    float GetPlayerZ() const;

private:
    AStar* m_astar;

    int m_id;
    float m_x, m_z;
    float m_playerX, m_playerZ;
    SIZE2 m_hp;  

    std::vector<std::pair<int, int>> m_path;
    size_t m_pathIndex;

    Vec3 GetNodeCenter(int x, int z) const;
};

// Util ÇÔ¼ö
std::vector<std::vector<int>> LoadMapBin(const std::string& filename);
std::pair<int, int> GetRandomPosition(const std::vector<std::vector<int>>& map);



