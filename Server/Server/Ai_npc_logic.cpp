#include <vector>
#include <queue>
#include <unordered_set>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <random>
#include <conio.h> 
#include "../../protocol.h"

// test 용
constexpr int MAP_WIDTH = 1024;
constexpr int MAP_HEIGHT = 1024;
// 플레이어, 좀비 시작 위치
constexpr int ZOMBIE_START_X = 2;
constexpr int ZOMBIE_START_Z = 2;
constexpr int PLAYER_START_X = 580;
constexpr int PLAYER_START_Z = 545;
// 추가 정보
constexpr int NUM_ZOMBIES = 50;          // 추가: 생성할 좀비 수
constexpr float CELL_SIZE = 1.0f;        // 노드당 크기
constexpr float ZOMBIE_HALF_SIZE = 0.4f; // 좀비 AABB 반 사이즈
// ================== 맵 파일 읽기 ==================

//std::vector<std::vector<int>> LoadMap(const std::string& filename)
//{
//    std::vector<std::vector<int>> map;
//    std::ifstream file(filename);
//    if (!file.is_open()) {
//        std::cout << "[Error] map.txt 파일 열기 실패!\n";
//        exit(1);
//    }
//
//    std::string line;
//    while (std::getline(file, line))
//    {
//        if (line.empty() || !isdigit(line[0])) continue;
//        std::vector<int> row;
//        for (char c : line) {
//            if (c == '0' || c == '1') {
//                row.push_back(c - '0');
//            }
//        }
//
//        if (!row.empty()) {
//            map.push_back(row);
//        }
//    }
//
//    
//    file.close();
//
//
//    return map;
//}
std::vector<std::vector<int>> LoadMapBin(const std::string& filename)
{
    const int MAP_SIZE = 1024; // obstacle_mask.bin은 1024 x 1024
    std::vector<std::vector<int>> map(MAP_SIZE, std::vector<int>(MAP_SIZE, 0));

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "[ERROR] obstacle_mask.bin 열기 실패!\n";
        exit(1);
    }

    for (int y = 0; y < MAP_SIZE; ++y) {
        for (int x = 0; x < MAP_SIZE; ++x) {
            char value;
            file.read(&value, 1);
            if (file.eof()) {
                std::cerr << "[ERROR] 파일 끝에 도달했습니다. 크기가 너무 작습니다.\n";
                exit(1);
            }
            map[y][x] = (value == 1) ? 1 : 0; // 1 = 장애물, 0 = 길
        }
    }

    file.close();
    std::cout << "[OK] obstacle_mask.bin 로드 완료\n";
    return map;
}


// ======================= FindPath ==========================

constexpr int my_gCost = 1.0f;

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
        return std::abs(x1 - x2) + std::abs(z1 - z2); // (가로, 세로) 대각선 x
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

// ======================= ZombieAI ======================

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

    void FindPath()
    {
        m_path = m_astar.FindPath((int)m_x, (int)m_z, (int)m_playerX, (int)m_playerZ);
        m_pathIndex = 1;
    }

    void Update(std::vector<ZombieAI*>& allZombies)
    {
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

        // AABB 충돌 회피
        Vec3 avoidance(0, 0, 0);
        for (auto& other : allZombies)
        {
            if (other == this) continue;

            float dx = std::abs(m_x - other->GetX());
            float dz = std::abs(m_z - other->GetZ());

            if (dx < ZOMBIE_HALF_SIZE * 2 && dz < ZOMBIE_HALF_SIZE * 2) // AABB 겹쳤으면
            {
                Vec3 push(m_x - other->GetX(), 0, m_z - other->GetZ());
                if (push.Length() > 0.001f)
                    avoidance += push.Normalize() * 0.1f; // 밀어내기
            }
        }

        Vec3 moveDir = (toTarget + avoidance).Normalize();
        float moveSpeed = 0.1f;

        m_x += moveDir.x * moveSpeed;
        m_z += moveDir.z * moveSpeed;
    }


    std::vector<std::pair<int, int>> FindPathToPlayer() {
        return m_astar.FindPath((int)m_x, (int)m_z, (int)m_playerX, (int)m_playerZ);
    }

    int GetID() const { return m_id; }
    float GetX() const { return m_x; }
    float GetZ() const { return m_z; }
    // test 용도 
    float GetPlayerX() const { return m_playerX; }
    float GetPlayerZ() const { return m_playerZ; }

    const std::vector<std::pair<int, int>>& GetPath() const {
        return m_path;
    }

private:
    AStar m_astar;
    int m_id;
    float m_x = 0.0f;
    float m_z = 0.0f;
    float m_playerX = 0.0f;
    float m_playerZ = 0.0f;

    std::vector<std::pair<int, int>> m_path;
    size_t m_pathIndex = 0;

    Vec3 GetNodeCenter(int x, int z) const {
        constexpr float cellSize = 1.0f;
        return Vec3(x * cellSize + cellSize * 0.5f, 0.0f, z * cellSize + cellSize * 0.5f);
    }

}; 

// =========================================================
 
// ================== PrintMap ================== 

std::pair<int, int> GetRandomPosition(const std::vector<std::vector<int>>& map)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distX(412, 611);
    std::uniform_int_distribution<> distZ(412, 611);

    while (true) {
        int x = distX(gen);
        int z = distZ(gen);
        if (map[z][x] == 0) { // 이동 가능한 곳
            return { x, z };
        }
    }
}


void PrintMap(
    const std::vector<std::vector<int>>& map, 
    const std::vector<ZombieAI*>& zombies)
{
	int zombieCount = 0;
    int height = map.size();
    int width = map[0].size();

    std::vector<std::vector<char>> display(height, std::vector<char>(width, '0'));

    // 벽 표시
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            if (map[z][x] == 1)
                display[z][x] = '#';
        }
    }
    // 경로 표시
    for (auto zombie : zombies)
    {
        for (auto& [x, z] : zombie->GetPath()) // m_path를 
        {
            if (x >= 0 && x < width && z >= 0 && z < height)
            {
                if (display[z][x] == ' ') 
                    display[z][x] = '*';
            }
        }
    }
    // 좀비 위치 표시
    for (auto zombie : zombies)
    {
        int zx = (int)zombie->GetX();
        int zz = (int)zombie->GetZ();
        if (zx >= 0 && zx < width && zz >= 0 && zz < height)
            display[zz][zx] = 'Z';
    }

    // 플레이어 위치 표시
    if (PLAYER_START_X >= 0 && PLAYER_START_X < width && PLAYER_START_Z >= 0 && PLAYER_START_Z < height)
        display[PLAYER_START_Z][PLAYER_START_X] = 'P';

    // 맵 출력
    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            std::cout << display[z][x];
            if (display[z][x] == 'Z')
                zombieCount++;
        }
        std::cout << "\n";
    }
    // 출력 끝나고 난 뒤, 좀비 개수 체크
    std::cout << "\n[현재 맵에 표시된 좀비 수] : " << zombieCount << "\n";
    if (zombieCount == NUM_ZOMBIES)
        std::cout << "[ok] 좀비 전부 찍혔습니다!\n";
    else
        std::cout << "[bad] 좀비 수가 맞지 않습니다! (" << zombieCount << " / " << NUM_ZOMBIES << ")\n";
}

void PrintMap2(
    const std::vector<std::vector<int>>& map,
    const std::vector<ZombieAI*>& zombies,
    int startY, int startX,
    int height, int width)
{
    int zombieCount = 0;

    // 출력
    for (int z = startY; z < startY + height; ++z) {
        for (int x = startX; x < startX + width; ++x) {
            if (z < 0 || z >= MAP_HEIGHT || x < 0 || x >= MAP_WIDTH) {
                std::cout << ' ';
                continue;
            }

            char ch = map[z][x] == 1 ? '#' : ' ';

            // 경로 위에 있으면 *
            for (auto zombie : zombies)
            {
                for (auto& [px, pz] : zombie->GetPath())
                {
                    if (px == x && pz == z) {
                        ch = '.';
                        break;
                    }
                }
            }

            // 좀비가 이 자리에 있으면 Z
            for (auto zombie : zombies)
            {
                int zx = (int)zombie->GetX();
                int zz = (int)zombie->GetZ();
                if (zx == x && zz == z) {
                    ch = 'Z';
                    zombieCount++;
                    break;
                }
            }

            // 플레이어 위치는 항상 최우선
            if ((int)PLAYER_START_X == x && (int)PLAYER_START_Z == z)
                ch = 'P';

            std::cout << ch;
        }
        std::cout << "\n";
    }

    std::cout << "\n[현재 맵에 표시된 좀비 수] : " << zombieCount << "\n";
    if (zombieCount == NUM_ZOMBIES)
        std::cout << "[ok] 좀비 전부 찍혔습니다!\n";
    else
        std::cout << "[bad] 좀비 수가 맞지 않습니다! (" << zombieCount << " / " << NUM_ZOMBIES << ")\n";
}


// =========================================================
int main()
{
    auto map = LoadMapBin("../../Map/Node/obstacle_mask.bin");

    std::vector<ZombieAI*> zombies;

    for (int i = 0; i < NUM_ZOMBIES; ++i)
    {
        auto [x, z] = GetRandomPosition(map);

        ZombieAI* zombie = new ZombieAI(map, i + 1);
        zombie->SetPosition((float)x, (float)z);
        zombie->SetPlayerPosition((float)PLAYER_START_X, (float)PLAYER_START_Z);
        zombie->FindPath(); // 처음에 경로 찾기

        zombies.push_back(zombie);
    }


    while (true)
    {
        std::cout << "[엔터를 누르면 시작 / ESC를 누르면 종료]\n";

        int key = _getch(); // 엔터 대기

        // 모든 좀비 랜덤 위치 이동
        if (key == 13) {
            for (auto z : zombies)
            {
                auto [newX, newZ] = GetRandomPosition(map);
                z->SetPosition((float)newX, (float)newZ);
                z->SetPlayerPosition((float)PLAYER_START_X, (float)PLAYER_START_Z);
                z->FindPath(); // 경로 다시 찾기
            }

            PrintMap2(map, zombies, 412, 412, 200, 200);
        }
        else if (key == 27) // ESC (ASCII 27)
        {
            exit(-1);
        }
        
    }

    for (auto z : zombies)
        delete z;

    return 0;
}
