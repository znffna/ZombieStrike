#include "ZombieAI.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <memory>
#include <conio.h> 

constexpr bool DEBUG_PRINT = false;
#define DEBUG_LOG(msg) \
    do { if (DEBUG_PRINT) std::cout << msg << std::endl; } while (0)

// -----------------------------
// A* Pathfinding 내부 클래스
// -----------------------------
struct Node
{
    int x, z;
    float gCost, hCost;
    float fCost() const { return gCost + hCost; }
    Node* parent = nullptr;
};

class ZombieAI::AStar
{
public:
    AStar(const std::vector<std::vector<int>>& map)
        : m_map(map), m_width(map[0].size()), m_height(map.size()) {}

    std::vector<std::pair<int, int>> FindPath(int startX, int startZ, int endX, int endZ);

private:
    std::vector<std::vector<int>> m_map;
    int m_width = 0;
    int m_height = 0;

	// 대각선 비용을 고려한 휴리스틱 함수
    float Heuristic(int x1, int z1, int x2, int z2) {
        int dx = std::abs(x1 - x2);
        int dz = std::abs(z1 - z2);
        return (float)(dx + dz - std::min(dx, dz));
        //return 1.414f * std::min(dx, dz) + std::abs(dx - dz); 
    }
};

std::vector<std::pair<int, int>> ZombieAI::AStar::FindPath(int startX, int startZ, int endX, int endZ)
{
    if (startX < 0 || startZ < 0 || endX < 0 || endZ < 0 ||
        startX >= m_width || startZ >= m_height ||
        endX >= m_width || endZ >= m_height)
    {
        DEBUG_LOG("[AStar::FindPath] Invalid coordinates: start(%d, %d), end(%d, %d)", startX, startZ, endX, endZ);
        return {}; // 빈 경로 반환
    }

    auto cmp = [](Node* a, Node* b) { return a->fCost() > b->fCost(); };
    std::priority_queue<Node*, std::vector<Node*>, decltype(cmp)> openList(cmp);
    std::unordered_set<int> closedList;

    Node* startNode = new Node{ startX, startZ, 0.0f, Heuristic(startX, startZ, endX, endZ) };
    if (!startNode)
    {
		DEBUG_LOG("[ERROR] Failed to allocate startNode");
        return {};
    }
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

		// 8방향 탐색
        const int dirX[8] = { 1, -1,  0,  0,  1,  1, -1, -1 };
        const int dirZ[8] = { 0,  0,  1, -1,  1, -1,  1, -1 };

        for (int dir = 0; dir < 8; ++dir)
        {
            int nx = current->x + dirX[dir];
            int nz = current->z + dirZ[dir];

            if (nx < 0 || nx >= m_width || nz < 0 || nz >= m_height) continue;
            if (m_map[nz][nx] != 0) continue;

            // 대각선 진입 시, 양쪽 인접칸 중 하나라도 벽이면 skip
            if (dir >= 4)
            {
                int adj1_x = current->x + dirX[dir];
                int adj1_z = current->z;
                int adj2_x = current->x;
                int adj2_z = current->z + dirZ[dir];

                if (m_map[adj1_z][adj1_x] != 0 || m_map[adj2_z][adj2_x] != 0)
                    continue;
            }

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
}

// -----------------------------
// ZombieAI 구현
// -----------------------------
ZombieAI::ZombieAI(const std::vector<std::vector<int>>& map, int id)
    : zom_astar(std::make_unique<AStar>(map)),
    zom_id(id), zom_x(0), zom_z(0), zom_targetX(0), zom_targetZ(0),
    zom_pathIndex(0), zom_hp(ZOMBIE_HP), zom_needsUpdate(true)
{}

void ZombieAI::SetPosition(float x, float z) {
    if (std::abs(zom_x - x) > 0.01f || std::abs(zom_z - z) > 0.01f)
        zom_needsUpdate = true;
    zom_x = x;
    zom_z = z;
}

void ZombieAI::SetTargetPosition(float x, float z) {
    zom_targetX = x; zom_targetZ = z;
}

void ZombieAI::SetHP(int hp) {
    zom_hp = hp;
    zom_needsUpdate = true;
}

//bool ZombieAI::CheckBulletHit(const Vec3& bulletPos, const Vec3& bulletDir, float bulletRadius)
//{
//    Vec3 zombiePos = GetPosition();
//
//    Vec3 toZombie = zombiePos - bulletPos;
//
//    // 직선 거리 중, 진행 방향 기준으로의 투영 거리
//    float projLength = toZombie.x * bulletDir.x + toZombie.z * bulletDir.z;
//
//    if (projLength < 0.0f) return false; // 음수면 총알 방향 반대이므로 충돌 없음
//
//    Vec3 closestPoint = bulletPos + bulletDir * projLength;  // 투영 벡터로부터 가장 가까운 거리 계산 (직선-점 거리) 
//    float distSq = (zombiePos - closestPoint).LengthSquared();
//
//    return distSq <= bulletRadius * bulletRadius; // 거리 제곱이 반지름 제곱보다 작으면 충돌
//}

void ZombieAI::ApplyDamage(SIZE2 damage) {
    if (damage >= zom_hp) zom_hp = 0;
    else zom_hp -= damage;
    zom_needsUpdate = true;
}
bool ZombieAI::IsDead() const { return zom_hp == 0; }

void ZombieAI::FindPath() {
    int startX = static_cast<int>(zom_x / CELL_SIZE);
    int startZ = static_cast<int>(zom_z / CELL_SIZE);
    int endX = static_cast<int>(zom_targetX / CELL_SIZE);
    int endZ = static_cast<int>(zom_targetZ / CELL_SIZE);

    zom_path = zom_astar->FindPath(startX, startZ, endX, endZ);

    if (!zom_path.empty()) {
        zom_act_type = ActionType::ZMOVE;
    }
    zom_pathIndex = 1;
}

std::vector<std::pair<int, int>> ZombieAI::FindPathToPlayer() {
    return zom_astar->FindPath((int)zom_x, (int)zom_z, (int)zom_targetX, (int)zom_targetZ);

}


void ZombieAI::Update(const std::vector<Vec3>& playerPositions, const std::vector<ZombieAI*>& allZombies)
{
    if (playerPositions.empty()) return;

    // [1] 가장 가까운 플레이어 위치 계산
    Vec3 closest = FindClosestPlayer(playerPositions);
    Vec3 newTarget = closest;

    bool needRepath =
        (int)(newTarget.x) != (int)(zom_targetX) ||
        (int)(newTarget.z) != (int)(zom_targetZ) ||
        zom_path.empty() ||
        zom_pathIndex >= zom_path.size() ||
        m_repath_timer > REPATH_INTERVAL;

    if (needRepath) {
        if (DEBUG_PRINT && zom_id == 10000 ) {
			DEBUG_LOG("[ZombieAI::Update] ID = " << zom_id
				<< ", Cur = (" << zom_x << ", " << zom_z << ")"
				<< ", Target = (" << newTarget.x << ", " << newTarget.z << ")");
        }

        SetTargetPosition(newTarget.x, newTarget.z);
        FindPath();
        m_repath_timer = 0;
    }
    else {
        m_repath_timer += DELTATIME;
    }



    // [2] 먼저 ATTACK 상태이면 처리하고 return
    if (zom_act_type == ATTACK) {
        m_attack_cooldown += DELTATIME;

        if (m_attack_cooldown > 1.0f) {
            m_attack_cooldown = 0.0f;

            zom_needsUpdate = true; // 공격처리 해야함!! 

            zom_act_type = ZMOVE;
            FindPath();
            return;
        }

        return; // ATTACK 상태는 이동 안 함
    }

    // [3] 이동 처리 (경로 따라 이동)
    if (zom_path.empty() || zom_pathIndex >= zom_path.size()) return;

    auto& targetNode = zom_path[zom_pathIndex];
    Vec3 targetPos = GetNodeCenter(targetNode.first, targetNode.second);

    Vec3 currentPos(zom_x, 0, zom_z);
    Vec3 toTarget = targetPos - currentPos;
    float distance = toTarget.Length();

    // [4] 이동 처리 (경로 따라 이동)
    if (distance < attackRange)
    {
        zom_act_type = ActionType::ATTACK;
        zom_path.clear();  // 경로 이동 중단
        zom_pathIndex = 0;

        zom_needsUpdate = true;
        return;  // ATTACK 상태면 이동하지 않음
    }

    if (distance < 0.1f) {
        zom_pathIndex++;
        return;
    }

    toTarget = toTarget.Normalize();


    // [5] 충돌 회피 처리 - 반발력 ,
    Vec3 avoidance(0, 0, 0);

    for (auto* other : allZombies)
    {
        if (other->GetID() == zom_id) continue;

        Vec3 diff = Vec3(zom_x, 0, zom_z) - Vec3(other->GetX(), 0, other->GetZ());
        float distSqr = diff.LengthSquared();

        if (distSqr < 0.25f && distSqr > 0.0001f) // 거리 < 0.5만 처리 (ZOMBIE_HALF_SIZE * 2)^2
        {
            float strength = 1.0f - (distSqr / 0.25f);  // 가까울수록 강하게
            avoidance += diff.Normalize() * strength * REPALSTRENGTH;
        }
    }

    Vec3 moveDir = (toTarget + avoidance).Normalize();
    float moveSpeed = Z_move_speed;

    zom_x += moveDir.x * moveSpeed;
    zom_z += moveDir.z * moveSpeed;
    zom_needsUpdate = true;
}

Vec3  ZombieAI::FindClosestPlayer(const std::vector<Vec3>& playerPositions)
{
    float minDistanceSq = FLT_MAX;
    Vec3 closestPlayer;
    Vec3 myPos(zom_x, 0, zom_z);


    for (const auto& playerPos : playerPositions)
    {
        float dx = playerPos.x - myPos.x;
        float dz = playerPos.z - myPos.z;
        float distanceSq = dx * dx + dz * dz;

        if (distanceSq < minDistanceSq)
        {
            minDistanceSq = distanceSq;
            closestPlayer = playerPos;
        }
    }

    return closestPlayer;
}


bool ZombieAI::IsDirty() const { return zom_needsUpdate; }
void ZombieAI::ClearDirty() { zom_needsUpdate = false; }
const std::vector<std::pair<int, int>>& ZombieAI::GetPath() const { return zom_path; }

int ZombieAI::GetID() const { return zom_id; }
SIZE2 ZombieAI::GetHP() const { return zom_hp; }

ActionType ZombieAI::GetActionType() const {
	if (zom_act_type == ActionType::ZMOVE) return ActionType::ZMOVE;
	else if (zom_act_type == ActionType::ATTACK) return ActionType::ATTACK;
	else return ActionType::NONE;
}
float ZombieAI::GetX() const { return zom_x; }
float ZombieAI::GetZ() const { return zom_z; }
float ZombieAI::GetPlayerX() const { return zom_targetX; }
float ZombieAI::GetPlayerZ() const { return zom_targetZ; }
Vec3 ZombieAI::GetNodeCenter(int x, int z) const {
    return Vec3(x * CELL_SIZE + 0.5f, 0, z * CELL_SIZE + 0.5f);
}


// ------------------- 맵 로딩 & 출력 & 랜덤 위치 -----------------------
// test , 플레이어, 좀비 시작 위치
//constexpr int ZOMBIE_START_X = 2;
//constexpr int ZOMBIE_START_Z = 2;
//constexpr int PLAYER_START_X = 580;
//constexpr int PLAYER_START_Z = 545;
//constexpr int NUM_ZOMBIES = 50;          
std::vector<std::vector<int>> LoadMapBin(const std::string& filename)
{
    std::vector<std::vector<int>> map(GRID_WIDTH, std::vector<int>(GRID_WIDTH, 0));
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        DEBUG_LOG("[ERROR] obstacle_mask.bin 열기 실패!");
        exit(1);
    }

    for (int z = 0; z < GRID_WIDTH; ++z) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            // 각 2x2 블록의 평균 or 대표값 (왼쪽 위 픽셀 기준)
            char value;
            file.read(&value, 1);
            if (file.eof()) {
                DEBUG_LOG("[ERROR] 파일 끝에 도달했습니다. 크기가 너무 작습니다.");
                exit(1);
            }
            map[z][x] = (value == 0) ? 0 : 1; // 0 = 길, 1 = 장애물
        }
    }
    DEBUG_LOG("[OK] 512x512 맵 로드 완료");
    return map;
}
std::pair<int, int> GetRandomPosition(const std::vector<std::vector<int>>& map)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distX(100, 150);
    std::uniform_int_distribution<> distZ(100, 150);

    //while (true) {
    //    int x = distX(gen), z = distZ(gen);
    //    if (map[z][x] == 0) return { x, z };
    //}

    int attempts = 0;
    while (attempts < 100) {
        int x = distX(gen), z = distZ(gen);
        if (map[z][x] == 0) return { x, z };
        ++attempts;
    }

    DEBUG_LOG("[ERROR] 좀비 위치 찾기 실패 (장애물로 꽉 찼을 수 있음)");
    exit(1);

}
//
//std::pair<int, int> GetRandomPlayerPosition(const std::vector<std::vector<int>>& map)
//{
//    static std::random_device rd;
//    static std::mt19937 gen(rd());
//    std::uniform_int_distribution<> distX(100, 100);
//    std::uniform_int_distribution<> distZ(100, 150);
//
//    int attempts = 0;
//    while (attempts < 100) {
//        int x = distX(gen), z = distZ(gen);
//        if (map[z][x] == 0) return { x, z };
//        ++attempts;
//    }
//
//    DEBUG_LOG("[ERROR] 플레이어 위치 찾기 실패 (해당 구역이 전부 장애물일 수 있음)");
//    exit(1);
//
//}
//void PrintMap2(
//    const std::vector<std::vector<int>>& map,
//    const std::vector<ZombieAI*>& zombies,
//    int startZ, int startX,
//    int height, int width,
//    float playerX, float playerZ)
//{
//    int zombieCount = 0;
//
//    // 출력
//    for (int z = startZ; z < startZ + height; ++z) {
//        for (int x = startX; x < startX + width; ++x) {
//            if (z < 0 || z >= GRID_HEIGHT || x < 0 || x >= GRID_WIDTH) {
//                std::cout << ' ';
//                continue;
//            }
//
//            // 출력: 지형
//            //char ch = map[z][x] == 1 ? ' ' : '# ';
//            char ch = map[z][x] == 1 ? '#' : '0 ';
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
//                int zx = static_cast<int>(zombie->GetX() / CELL_SIZE);
//                int zz = static_cast<int>(zombie->GetZ() / CELL_SIZE);
//                
//
//                if (zx == x && zz == z) {
//                    ch = 'Z';
//                    zombieCount++;
//                    break;
//                }
//            }
//
//            if (x == static_cast<int>(playerX / CELL_SIZE) && z == static_cast<int>(playerZ / CELL_SIZE))
//                ch = 'P';
//
//            std::cout << ch;
//        }
//        std::cout << "\n";
//    }
//
//	DEBUG_LOG("[DEBUG] 좀비 수: " << zombieCount);
//    if (zombieCount == NUM_ZOMBIES)
//		DEBUG_LOG("[ok] 좀비 전부 찍혔습니다!");
//    else
//		DEBUG_LOG("[bad] 좀비 수가 맞지 않습니다! (" << zombieCount << " / " << NUM_ZOMBIES << ")");
//}
//
//
//int main()
//{
//    auto map = LoadMapBin("../../Map/Node/ob_mask_te_1.bin");
//
//    std::vector<ZombieAI*> zombies;
//
//    // 1. 플레이어 랜덤 위치 선정
//    auto [px, pz] = GetRandomPlayerPosition(map);
//    DEBUG_LOG("[TEST] Player 위치: (" << px << ", " << pz << ")");
//
//  // 2. 좀비들 스폰
//    for (int i = 0; i < 50; ++i)
//    {
//        auto [zx, zz] = GetRandomPosition(map); // 이미 장애물 피함
//
//        ZombieAI* zombie = new ZombieAI(map, i + 1);
//        zombie->SetPosition((float)zx, (float)zz);
//        zombie->SetPlayerPosition((float)px, (float)pz);
//        zombie->FindPath(); // A* 수행
//
//        zombies.push_back(zombie);
//    }
//	DEBUG_LOG("[TEST] 좀비들 스폰 완료");
//
//
//    while (true)
//    {
//		DEBUG_LOG("[엔터를 누르면 시작 / esc를 누르면 종료]");
//
//        int key = _getch(); // 엔터 대기
//
//        // 모든 좀비 랜덤 위치 이동
//        if (key == 13) {
//            for (auto z : zombies)
//            {
//                auto [newx, newz] = GetRandomPosition(map);
//                z->SetPosition((float)newx, (float)newz);
//                z->SetPlayerPosition((float)px, (float)pz);  // 같은 플레이어에게 재설정
//                z->FindPath();
//            }
//
//            //PrintMap2(map, zombies, pz - 100, px - 100, 50, 50, px, pz);  // Player 좌표 넘김
//            PrintMap2(map, zombies, 0, 0, 512, 512, px, pz);
//        }
//        else if (key == 27) {
//            break;
//        }
//    }
//        
//    for (auto z : zombies)
//        delete z;
//
//    return 0;
//}