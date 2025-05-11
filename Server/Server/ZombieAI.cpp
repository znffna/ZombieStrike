#include "ZombieAI.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <conio.h> 

// test , 플레이어, 좀비 시작 위치
const int BIN_WIDTH = 512;
const int BIN_HEIGHT = 512;
const int MAP_WIDTH = 250;
const int MAP_HEIGHT = 250;
constexpr int ZOMBIE_START_X = 2;
constexpr int ZOMBIE_START_Z = 2;
constexpr int PLAYER_START_X = 580;
constexpr int PLAYER_START_Z = 545;
constexpr int NUM_ZOMBIES = 50;          // 추가: 생성할 좀비 수
// 필수 정보 
constexpr float my_gCost = 1.0f;         // 이동 비용
constexpr float CELL_SIZE = 1.0f;        // 노드당 크기
constexpr float ZOMBIE_HALF_SIZE = 0.4f; // 좀비 AABB 반 사이즈
constexpr float ZOMBIE_HP = 500;         // 좀비 초기 체력


// -------------------- A* 내부 클래스 -----------------------
class ZombieAI::AStar
{
public:
    AStar(const std::vector<std::vector<int>>& map)
        : m_map(map), m_width(map[0].size()), m_length(map.size()) {}

    std::vector<std::pair<int, int>> FindPath(int startX, int startZ, int endX, int endZ);

private:
    std::vector<std::vector<int>> m_map;
    int m_width;
    int m_length;

    float Heuristic(int x1, int z1, int x2, int z2) {
        return std::abs(x1 - x2) + std::abs(z1 - z2);
    }
};

struct Node
{
    int x, z;
    float gCost, hCost;
    float fCost() const { return gCost + hCost; }
    Node* parent = nullptr;
};

std::vector<std::pair<int, int>> ZombieAI::AStar::FindPath(int startX, int startZ, int endX, int endZ)
{
    auto cmp = [](Node* a, Node* b) { return a->fCost() > b->fCost(); };
    std::priority_queue<Node*, std::vector<Node*>, decltype(cmp)> openList(cmp);
    std::unordered_set<int> closedList;

    Node* startNode = new Node{ startX, startZ, 0.0f, Heuristic(startX, startZ, endX, endZ) };
    openList.push(startNode);

    std::vector<std::pair<int, int>> path;

    while (!openList.empty())
    {
        Node* current = openList.top(); openList.pop();
        int key = current->z * m_width + current->x;

        if (closedList.find(key) != closedList.end()) {
            delete current;
            continue;
        }

        closedList.insert(key);

        if (current->x == endX && current->z == endZ)
        {
            Node* node = current;
            while (node) {
                path.emplace_back(node->x, node->z);
                node = node->parent;
            }
            std::reverse(path.begin(), path.end());
            delete current;
            break;
        }

        const int dirX[4] = { 1, -1, 0, 0 };
        const int dirZ[4] = { 0, 0, 1, -1 };

        for (int dir = 0; dir < 4; ++dir)
        {
            int nx = current->x + dirX[dir];
            int nz = current->z + dirZ[dir];
            if (nx < 0 || nx >= m_width || nz < 0 || nz >= m_length) continue;
            if (m_map[nz][nx] != 0) continue;

            int nextKey = nz * m_width + nx;
            if (closedList.find(nextKey) != closedList.end()) continue;

            Node* neighbor = new Node{ nx, nz };
            neighbor->gCost = current->gCost + my_gCost;
            neighbor->hCost = Heuristic(nx, nz, endX, endZ);
            neighbor->parent = current;
            openList.push(neighbor);
        }
    }

    return path;
}// ================== 맵 파일 읽기 ==================

// ------------------- ZombieAI 구현 -----------------------

ZombieAI::ZombieAI(const std::vector<std::vector<int>>& map, int id)
    : m_astar(nullptr), m_id(id), m_x(0), m_z(0), m_playerX(0), m_playerZ(0), m_pathIndex(0), m_hp(ZOMBIE_HP)
{
    m_astar = new AStar(map);
}

void ZombieAI::SetPosition(float x, float z) {
    m_x = x; m_z = z;
}
void ZombieAI::SetPlayerPosition(float x, float z) {
    m_playerX = x; m_playerZ = z;
}
void ZombieAI::FindPath() {
    m_path = m_astar->FindPath((int)m_x, (int)m_z, (int)m_playerX, (int)m_playerZ);
    m_pathIndex = 1;
}
std::vector<std::pair<int, int>> ZombieAI::FindPathToPlayer() {
    return m_astar->FindPath((int)m_x, (int)m_z, (int)m_playerX, (int)m_playerZ);
}

int ZombieAI::GetID() const { return m_id; }
float ZombieAI::GetX() const { return m_x; }
float ZombieAI::GetZ() const { return m_z; }
float ZombieAI::GetPlayerX() const { return m_playerX; }
float ZombieAI::GetPlayerZ() const { return m_playerZ; }
const std::vector<std::pair<int, int>>& ZombieAI::GetPath() const { return m_path; }

Vec3 ZombieAI::GetNodeCenter(int x, int z) const {
    return Vec3(x * CELL_SIZE + 0.5f, 0, z * CELL_SIZE + 0.5f);
}

void ZombieAI::Update(const std::vector<Vec3>& playerPositions, const std::vector<ZombieAI*>& allZombies)
{
    if (playerPositions.empty()) return;

    // 1. 가장 가까운 플레이어 위치 계산
    float minDist = FLT_MAX;
    Vec3 closest;

    for (const auto& pos : playerPositions)
    {
        Vec3 myPos(m_x, 0, m_z);
        float dist = (pos - myPos).LengthSquared();
        if (dist < minDist)
        {
            minDist = dist;
            closest = pos;
        }
    }

    // 2. 타겟 위치 설정 및 경로 재계산
    SetPlayerPosition(closest.x, closest.z); 
    FindPath();

    // 3. 이동 처리 (경로 따라 이동)
    if (m_path.empty() || m_pathIndex >= m_path.size())
        return;

    auto& targetNode = m_path[m_pathIndex];
    Vec3 targetPos = GetNodeCenter(targetNode.first, targetNode.second);

    Vec3 currentPos(m_x, 0, m_z);
    Vec3 toTarget = targetPos - currentPos;
    float distance = toTarget.Length();

    if (distance < 0.1f) {
        m_pathIndex++;
        return;
    }

    toTarget = toTarget.Normalize();

    // 단순 방향 이동 (충돌 회피 없음)
    //Vec3 moveDir = toTarget;
    //float moveSpeed = 0.1f;

    // 4. 충돌 회피 처리
    Vec3 avoidance(0, 0, 0);
    for (auto* other : allZombies)
    {
        if (other->GetID() == m_id)
            continue;

        float dx = std::abs(m_x - other->GetX());
        float dz = std::abs(m_z - other->GetZ());

        if (dx < ZOMBIE_HALF_SIZE * 2 && dz < ZOMBIE_HALF_SIZE * 2)
        {
            Vec3 push(m_x - other->GetX(), 0, m_z - other->GetZ());
            if (push.Length() > 0.001f)
                avoidance += push.Normalize() * 0.1f;
        }
    }

    Vec3 moveDir = (toTarget + avoidance).Normalize();
    float moveSpeed = 0.1f;

    m_x += moveDir.x * moveSpeed;
    m_z += moveDir.z * moveSpeed;
}



// ------------------- 맵 로딩 & 랜덤 위치 -----------------------

std::vector<std::vector<int>> LoadMapBin(const std::string& filename)
{
    //const int MAP_SIZE = 1024;
    //std::vector<std::vector<int>> map(MAP_SIZE, std::vector<int>(MAP_SIZE, 0));
    //std::ifstream file(filename, std::ios::binary);
    //if (!file.is_open()) {
    //    std::cerr << "[ERROR] obstacle_mask.bin 열기 실패!\n";
    //    exit(1);
    //}

    //for (int y = 0; y < MAP_SIZE; ++y)
    //    for (int x = 0; x < MAP_SIZE; ++x)
    //        map[y][x] = file.get() == 1 ? 1 : 0;

    //std::cout << "[OK] obstacle_mask.bin 로드 완료\n";
    //return map;

    std::vector<std::vector<int>> map(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, 0));
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ERROR] obstacle_mask.bin 열기 실패!\n";
        exit(1);
    }

    for (int z = 0; z < MAP_HEIGHT; ++z) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            // 각 2x2 블록의 평균 or 대표값 (왼쪽 위 픽셀 기준)
            int bin_x = x * 2;
            int bin_z = z * 2;

            file.seekg(bin_z * BIN_WIDTH + bin_x, std::ios::beg);
            char value;
            file.read(&value, 1);
            map[z][x] = (value == 1) ? 1 : 0;
        }
    }

}
//std::vector<std::vector<int>> LoadMapBin(const std::string& filename)
//{
//    const int MAP_SIZE = 1024; // obstacle_mask.bin은 1024 x 1024
//    std::vector<std::vector<int>> map(MAP_SIZE, std::vector<int>(MAP_SIZE, 0));
//
//    std::ifstream file(filename, std::ios::binary);
//    if (!file.is_open()) {
//        std::cout << "[ERROR] obstacle_mask.bin 열기 실패!\n";
//        exit(1);
//    }
//
//    for (int y = 0; y < MAP_SIZE; ++y) {
//        for (int x = 0; x < MAP_SIZE; ++x) {
//            char value;
//            file.read(&value, 1);
//            if (file.eof()) {
//                std::cerr << "[ERROR] 파일 끝에 도달했습니다. 크기가 너무 작습니다.\n";
//                exit(1);
//            }
//            map[y][x] = (value == 1) ? 1 : 0; // 1 = 장애물, 0 = 길
//        }
//    }
//
//    file.close();
//    std::cout << "[OK] obstacle_mask.bin 로드 완료\n";
//    return map;
//}

std::pair<int, int> GetRandomPosition(const std::vector<std::vector<int>>& map)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distX(30, 50);
    std::uniform_int_distribution<> distZ(30, 50);

    while (true) {
        int x = distX(gen), z = distZ(gen);
        if (map[z][x] == 0) return { x, z };
    }
}



// ======================= ZombieAI ======================

//class ZombieAI
//{
//public:
//    ZombieAI(const std::vector<std::vector<int>>& map, int id)
//        : m_astar(map), m_id(id) {
//    }
//
//    void SetPosition(float x, float z) {
//        m_x = x;
//        m_z = z;
//    }
//
//    void SetPlayerPosition(float x, float z) {
//        m_playerX = x;
//        m_playerZ = z;
//    }
//
//    void FindPath()
//    {
//        m_path = m_astar.FindPath((int)m_x, (int)m_z, (int)m_playerX, (int)m_playerZ);
//        m_pathIndex = 1;
//    }
//
//    void Update(std::vector<ZombieAI*>& allZombies)
//    {
//        if (m_path.empty() || m_pathIndex >= m_path.size())
//            return;
//
//        auto& targetNode = m_path[m_pathIndex];
//        Vec3 targetPos = GetNodeCenter(targetNode.first, targetNode.second);
//
//        Vec3 currentPos(m_x, 0, m_z);
//        Vec3 toTarget = targetPos - currentPos;
//        float distance = toTarget.Length();
//
//        if (distance < 0.1f) {
//            m_pathIndex++;
//            return;
//        }
//
//        toTarget = toTarget.Normalize();
//
//        // AABB 충돌 회피
//        Vec3 avoidance(0, 0, 0);
//        for (auto& other : allZombies)
//        {
//            if (other == this) continue;
//
//            float dx = std::abs(m_x - other->GetX());
//            float dz = std::abs(m_z - other->GetZ());
//
//            if (dx < ZOMBIE_HALF_SIZE * 2 && dz < ZOMBIE_HALF_SIZE * 2) // AABB 겹쳤으면
//            {
//                Vec3 push(m_x - other->GetX(), 0, m_z - other->GetZ());
//                if (push.Length() > 0.001f)
//                    avoidance += push.Normalize() * 0.1f; // 밀어내기
//            }
//        }
//
//        Vec3 moveDir = (toTarget + avoidance).Normalize();
//        float moveSpeed = 0.1f;
//
//        m_x += moveDir.x * moveSpeed;
//        m_z += moveDir.z * moveSpeed;
//    }
//
//
//    std::vector<std::pair<int, int>> FindPathToPlayer() {
//        return m_astar.FindPath((int)m_x, (int)m_z, (int)m_playerX, (int)m_playerZ);
//    }
//
//    int GetID() const { return m_id; }
//    float GetX() const { return m_x; }
//    float GetZ() const { return m_z; }
//    // test 용도 
//    float GetPlayerX() const { return m_playerX; }
//    float GetPlayerZ() const { return m_playerZ; }
//
//    const std::vector<std::pair<int, int>>& GetPath() const {
//        return m_path;
//    }
//
//private:
//    AStar m_astar;
//    int m_id;
//    float m_x = 0.0f;
//    float m_z = 0.0f;
//    float m_playerX = 0.0f;
//    float m_playerZ = 0.0f;
//
//    std::vector<std::pair<int, int>> m_path;
//    size_t m_pathIndex = 0;
//
//    Vec3 GetNodeCenter(int x, int z) const {
//        constexpr float cellSize = 1.0f;
//        return Vec3(x * cellSize + cellSize * 0.5f, 0.0f, z * cellSize + cellSize * 0.5f);
//    }
//
//}; 

// ======================== test 용 ========================

//void PrintMap(
//    const std::vector<std::vector<int>>& map, 
//    const std::vector<ZombieAI*>& zombies)
//{
//	int zombieCount = 0;
//    int height = map.size();
//    int width = map[0].size();
//
//    std::vector<std::vector<char>> display(height, std::vector<char>(width, '0'));
//
//    // 벽 표시
//    for (int z = 0; z < height; ++z) {
//        for (int x = 0; x < width; ++x) {
//            if (map[z][x] == 1)
//                display[z][x] = '#';
//        }
//    }
//    // 경로 표시
//    for (auto zombie : zombies)
//    {
//        for (auto& [x, z] : zombie->GetPath()) // m_path를 
//        {
//            if (x >= 0 && x < width && z >= 0 && z < height)
//            {
//                if (display[z][x] == ' ') 
//                    display[z][x] = '*';
//            }
//        }
//    }
//    // 좀비 위치 표시
//    for (auto zombie : zombies)
//    {
//        int zx = (int)zombie->GetX();
//        int zz = (int)zombie->GetZ();
//        if (zx >= 0 && zx < width && zz >= 0 && zz < height)
//            display[zz][zx] = 'Z';
//    }
//
//    // 플레이어 위치 표시
//    if (PLAYER_START_X >= 0 && PLAYER_START_X < width && PLAYER_START_Z >= 0 && PLAYER_START_Z < height)
//        display[PLAYER_START_Z][PLAYER_START_X] = 'P';
//
//    // 맵 출력
//    for (int z = 0; z < height; ++z) {
//        for (int x = 0; x < width; ++x) {
//            std::cout << display[z][x];
//            if (display[z][x] == 'Z')
//                zombieCount++;
//        }
//        std::cout << "\n";
//    }
//    // 출력 끝나고 난 뒤, 좀비 개수 체크
//    std::cout << "\n[현재 맵에 표시된 좀비 수] : " << zombieCount << "\n";
//    if (zombieCount == NUM_ZOMBIES)
//        std::cout << "[ok] 좀비 전부 찍혔습니다!\n";
//    else
//        std::cout << "[bad] 좀비 수가 맞지 않습니다! (" << zombieCount << " / " << NUM_ZOMBIES << ")\n";
//}
//
//void PrintMap2(
//    const std::vector<std::vector<int>>& map,
//    const std::vector<ZombieAI*>& zombies,
//    int startY, int startX,
//    int height, int width)
//{
//    int zombieCount = 0;
//
//    // 출력
//    for (int z = startY; z < startY + height; ++z) {
//        for (int x = startX; x < startX + width; ++x) {
//            if (z < 0 || z >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH) {
//                std::cout << ' ';
//                continue;
//            }
//
//            char ch = map[z][x] == 1 ? '#' : ' ';
//
//            // 경로 위에 있으면 *
//            for (auto zombie : zombies)
//            {
//                for (auto& [px, pz] : zombie->GetPath())
//                {
//                    if (px == x && pz == z) {
//                        ch = '.';
//                        break;
//                    }
//                }
//            }
//
//            // 좀비가 이 자리에 있으면 Z
//            for (auto zombie : zombies)
//            {
//                int zx = (int)zombie->GetX();
//                int zz = (int)zombie->GetZ();
//                if (zx == x && zz == z) {
//                    ch = 'Z';
//                    zombieCount++;
//                    break;
//                }
//            }
//
//            // 플레이어 위치는 항상 최우선
//            if ((int)PLAYER_START_X == x && (int)PLAYER_START_Z == z)
//                ch = 'P';
//
//            std::cout << ch;
//        }
//        std::cout << "\n";
//    }
//
//    std::cout << "\n[현재 맵에 표시된 좀비 수] : " << zombieCount << "\n";
//    if (zombieCount == NUM_ZOMBIES)
//        std::cout << "[ok] 좀비 전부 찍혔습니다!\n";
//    else
//        std::cout << "[bad] 좀비 수가 맞지 않습니다! (" << zombieCount << " / " << NUM_ZOMBIES << ")\n";
//}
//
//
//int main()
//{
//    auto map = LoadMapBin("../../Map/Node/ob_mask_te_2.bin");
//
//    std::vector<ZombieAI*> zombies;
//
//    for (int i = 0; i < NUM_ZOMBIES; ++i)
//    {
//        auto [x, z] = GetRandomPosition(map);
//
//        ZombieAI* zombie = new ZombieAI(map, i + 1);
//        zombie->SetPosition((float)x, (float)z);
//        zombie->SetPlayerPosition((float)PLAYER_START_X, (float)PLAYER_START_Z);
//        zombie->FindPath(); // 처음에 경로 찾기
//
//        zombies.push_back(zombie);
//    }
//
//
//    while (true)
//    {
//        std::cout << "[엔터를 누르면 시작 / ESC를 누르면 종료]\n";
//
//        int key = _getch(); // 엔터 대기
//
//        // 모든 좀비 랜덤 위치 이동
//        if (key == 13) {
//            for (auto z : zombies)
//            {
//                auto [newX, newZ] = GetRandomPosition(map);
//                z->SetPosition((float)newX, (float)newZ);
//                z->SetPlayerPosition((float)PLAYER_START_X, (float)PLAYER_START_Z);
//                z->FindPath(); // 경로 다시 찾기
//            }
//
//            PrintMap2(map, zombies, 412, 412, 200, 200);
//        }
//        else if (key == 27) // ESC (ASCII 27)
//        {
//            exit(-1);
//        }
//        
//    }
//
//    for (auto z : zombies)
//        delete z;
//
//    return 0;
//}
