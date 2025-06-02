#pragma once
#include <vector>
#include <utility>
#include <fstream>
#include <string>
#include <unordered_map>
#include "../../protocol.h"

// 필수 정보 
constexpr float my_gCost = 1.0f;             // 이동 비용
constexpr float ZOMBIE_HALF_SIZE = 0.4f;     // 좀비 AABB 반 사이즈
constexpr SIZE2 ZOMBIE_HP = 500;             // 좀비 초기 체력
constexpr float ZOMBIE_DAMAGE = 10;          // 좀비 초기 체력
constexpr float Z_move_speed = 0.05f;         // 좀비 

constexpr float WORLD_WIDTH = 250.0f;
constexpr float WORLD_HEIGHT = 250.0f;
constexpr float GRID_WIDTH = 512.0f;
constexpr float GRID_HEIGHT = 512.0f;
constexpr float CELL_SIZE = WORLD_WIDTH / GRID_WIDTH; // 0.488..

static constexpr float REPATH_INTERVAL = 1.0f; // 1초마다 재탐색 허용
constexpr float deltaTime = 1.0f / 60.0f;

class ZombieAI
{
public:
    class AStar;

    ZombieAI(const std::vector<std::vector<int>>& map, int id);

    void SetPosition(float x, float z);   // 위치 및 타겟 설정
    //void SetPlayerPosition(float x, float z);

    void SetTargetPosition(float x, float z);
	void SetHP(int hp);                   // 체력 설정

    Vec3 FindClosestPlayer(const std::vector<Vec3>& playerPositions);
    void FindPath();                      // AI 동작

    Vec3 AvoidPlayers(const std::vector<Vec3>& playerPositions); // 플레이어 회피

    void Update(const std::vector<Vec3>& playerPositions, const std::vector<ZombieAI*>& allZombies, float deltaTime);


    std::vector<std::pair<int, int>> FindPathToPlayer();   // 정보 조회
    const std::vector<std::pair<int, int>>& GetPath() const;

    int GetID() const;   // 좀비 ID 및 상태 조회
    Vec3 GetPosition() const { return Vec3{ m_x, 0.0f, m_z }; }
    SIZE2 GetHP() const;

    void AddDamage(SIZE2 amount); // 체력 설정
    
    Object GetObjectinfo() const; // 패킷 정보 반환   

    // Dirty 플래그
    bool IsDirty() const;
    void ClearDirty();

    Vec3 GetLookVectorToPlayer() const;
    float GetX() const;
    float GetZ() const;
    float GetPlayerX() const;
    float GetPlayerZ() const;

private:
   // AStar* m_astar;
    std::unique_ptr<AStar> m_astar;

    float m_repath_timer = 0.0f;

    int m_id;
    float m_x, m_z;
    float m_targetX, m_targetZ;
    SIZE2 m_hp;  
    std::vector<std::pair<int, int>> m_path;

    const std::vector<std::vector<int>>& m_map;
    size_t m_pathIndex;

    bool m_dirty = true;

    Vec3 GetNodeCenter(int x, int z) const;
};

// Util 함수
std::vector<std::vector<int>> LoadMapBin(const std::string& filename);
std::pair<int, int> GetRandomPosition(const std::vector<std::vector<int>>& map);



