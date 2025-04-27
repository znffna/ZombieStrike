#include <vector>
#include <queue>
#include <unordered_set>
#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <mutex>

// test 용
constexpr int MAP_WIDTH = 50;
constexpr int MAP_HEIGHT = 50;
// 플레이어, 좀비 시작 위치
const int ZOMBIE_START_X = 2;
const int ZOMBIE_START_Z = 2;
const int PLAYER_START_X = 40;
const int PLAYER_START_Z = 41;
// ==================

// 맵 파일 읽기
std::vector<std::vector<int>> LoadMap(const std::string& filename)
{
    std::vector<std::vector<int>> map;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "[Error] map.txt 파일 열기 실패!\n";
        exit(1);
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || !isdigit(line[0])) continue;
        std::vector<int> row;
        for (char c : line) {
            if (c == '0' || c == '1') {
                row.push_back(c - '0');
            }
        }

        if (!row.empty()) {

            map.push_back(row);

            /*          for (int v : row) {
                          std::cout << v;
                      }*/
        }
    }

    
    file.close();


    return map;
}

// 맵과 경로 표시
void PrintMap(const std::vector<std::vector<int>>& map, const std::vector<std::pair<int, int>>& path)
{
    std::vector<std::vector<char>> display(MAP_HEIGHT, std::vector<char>(MAP_WIDTH, '0'));

    for (int z = 0; z < MAP_HEIGHT; ++z) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (map[z][x] == 1)
                display[z][x] = '#'; // 벽
        }
    }

    for (auto& [x, z] : path) {
        display[z][x] = '*'; // 경로
    }

    display[ZOMBIE_START_Z][ZOMBIE_START_X] = 'Z'; // 좀비 시작
    display[PLAYER_START_Z][PLAYER_START_X] = 'P'; // 플레이어 목표

    for (int z = 0; z < MAP_HEIGHT; ++z) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            std::cout << display[z][x];
        }
        std::cout << "\n";
    }
}

// =========================================================


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

    std::vector<std::pair<int, int>> FindPathToPlayer() {
        return m_astar.FindPath((int)m_x, (int)m_z, (int)m_playerX, (int)m_playerZ);
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
int main()
{
    // 1. 맵 로드
    auto map = LoadMap("map.txt");

    // 2. ZombieAI 생성
    ZombieAI zombieAI(map, 1);
    zombieAI.SetPosition((float)ZOMBIE_START_X, (float)ZOMBIE_START_Z);
    zombieAI.SetPlayerPosition((float)PLAYER_START_X, (float)PLAYER_START_Z);

    // 3. 경로 찾기
    auto path = zombieAI.FindPathToPlayer();
    //auto path = zombieAI.m_astar.FindPath(ZOMBIE_START_X, ZOMBIE_START_Z, PLAYER_START_X, PLAYER_START_Z);

    if (path.empty()) {
        std::cout << "플레이어까지 경로를 찾을 수 없습니다!\n";
    }
    else {
        std::cout << "경로 찾기 성공! 경로 길이: " << path.size() << "\n";

        //for (int z = 0; z < MAP_HEIGHT; ++z) {
        //    for (int x = 0; x < MAP_WIDTH; ++x) {
        //        std::cout << map[z][x]; // 맵 출력
        //    }
        //    std::cout << "\n";

        //}
        //std::cout << "[경로 좌표]\n";
        //for (auto& [x, z] : path) {
        //    std::cout << "(" << x << "," << z << ") ";
        //}
        //std::cout << "\n";

        // 4. 맵 출력
        PrintMap(map, path);
    }

    return 0;
}