#pragma once
#include <vector>
#include <utility>
#include <fstream>
#include <string> 
#include "../../protocol.h"

class ZombieAI
{
public:
    class AStar;

    ZombieAI(const std::vector<std::vector<int>>& map, int id);
    void SetPosition(float x, float z);
    void SetPlayerPosition(float x, float z);
    void FindPath();
    void Update(std::vector<ZombieAI*>& allZombies);
    std::vector<std::pair<int, int>> FindPathToPlayer();

    int GetID() const;
    float GetX() const;
    float GetZ() const;
    float GetPlayerX() const;
    float GetPlayerZ() const;
    const std::vector<std::pair<int, int>>& GetPath() const;

private:
    AStar* m_astar;

    int m_id;
    float m_x, m_z;
    float m_playerX, m_playerZ;

    std::vector<std::pair<int, int>> m_path;
    size_t m_pathIndex;

    Vec3 GetNodeCenter(int x, int z) const;
};

// Util ÇÔ¼ö
std::vector<std::vector<int>> LoadMapBin(const std::string& filename);
std::pair<int, int> GetRandomPosition(const std::vector<std::vector<int>>& map);



