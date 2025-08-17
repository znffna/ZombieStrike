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

// test , 플레이어, 좀비 시작 위치
constexpr int ZOMBIE_START_X = 2;
constexpr int ZOMBIE_START_Z = 2;
constexpr int PLAYER_START_X = 580;
constexpr int PLAYER_START_Z = 545;
constexpr int NUM_ZOMBIES = 50; // 추가: 생성할 좀비 수

static bool IsAABBCollision(float x1, float z1, float x2, float z2, float half, float tolerance = 1.0f)
{   // 겹침 판단
    float range = half * tolerance * 2.0f;
    return (std::abs(x1 - x2) < range) && (std::abs(z1 - z2) < range);
}

bool IsTooClose(float x1, float z1, float x2, float z2, float minDist)
{   // 접근 제한, 거리 유지 
    float dx = x1 - x2;
    float dz = z1 - z2;
    return (dx * dx + dz * dz) < (minDist * minDist);
}
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
            float stepCost = (dir < 4) ? 1.0f : 1.41421356f;  // // A* 직/대각 비용
            neighbor->gCost = current->gCost + stepCost;      // // A* 누적 gCost 갱신

            neighbor->hCost = Heuristic(nx, nz, endX, endZ);
            neighbor->parent = current;
            openList.push(neighbor);
        }
    }

    return path;
}

// ------------------- ZombieAI 구현 -----------------------

ZombieAI::ZombieAI(const std::vector<std::vector<int>>& map, int id)
    : m_map(map), m_astar(nullptr), m_pathIndex(0),
    m_id(id), m_x(0), m_z(0), m_targetX(0), m_targetZ(0), m_hp(ZOMBIE_HP), 
    m_dirty(true)
{
    m_astar = std::make_unique<AStar>(map);
    //m_astar = new AStar(map);
}

void ZombieAI::SetPosition(float x, float z) {
    if (std::abs(m_x - x) > 0.01f || std::abs(m_z - z) > 0.01f)
        m_dirty = true;
    m_x = x;
    m_z = z;
}
void ZombieAI::SetHP(int hp) {
	m_hp = hp;
	m_dirty = true;
}


void ZombieAI::SetTargetPosition(float x, float z) {
    m_targetX = x; m_targetZ = z;
    // look 계산
}

std::vector<std::pair<int, int>> ZombieAI::FindPathToPlayer() {
    return m_astar->FindPath((int)m_x, (int)m_z, (int)m_targetX, (int)m_targetZ);
}

void ZombieAI::FindPath() {
    //m_path = m_astar->FindPath((int)m_x, (int)m_z, (int)m_playerX, (int)m_playerZ);
    if (!m_astar)
    {
		DEBUG_LOG("[ERROR] m_astar is nullptr");
        return;
    }

    int startX = static_cast<int>(m_x / CELL_SIZE);
    int startZ = static_cast<int>(m_z / CELL_SIZE);
    int endX = static_cast<int>(m_targetX / CELL_SIZE);
    int endZ = static_cast<int>(m_targetZ / CELL_SIZE);

    m_path = m_astar->FindPath(startX, startZ, endX, endZ);

    m_pathIndex = 1;
}

Vec3  ZombieAI::FindClosestPlayer(const std::vector<Vec3>& playerPositions)
{
    float minDistanceSq = FLT_MAX;
    Vec3 closestPlayer;

    Vec3 myPos(m_x, 0, m_z);

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

Vec3 ZombieAI::AvoidPlayers(const std::vector<Vec3>& playerPositions)
{
    Vec3 avoid(0, 0, 0);
    Vec3 myPos(m_x, 0, m_z);

    for (const auto& playerPos : playerPositions)
    {
        float dx = std::abs(myPos.x - playerPos.x);
        float dz = std::abs(myPos.z - playerPos.z);

        if (dx < ZOMBIE_HALF_SIZE * 2 && dz < ZOMBIE_HALF_SIZE * 2)
        {
            Vec3 push = myPos - playerPos;
            if (push.Length() > 0.001f)
                avoid += push.Normalize() * 0.1f;
        }
    }

    return avoid;
}

void ZombieAI::TriggerAttack(float animTime)
{
    // 공격 모션 시간 설정, 쿨다운 갱신
    m_attack_left = std::max(m_attack_left, animTime);
    m_attack_cd = std::max(m_attack_cd, Z_ATTACK_COOLDOWN);
    m_dirty = true;
}

bool ZombieAI::IsAttacking() const
{
    return m_attack_left > 0.0f;
}

// 근접 시 잠깐 멈춤
void ZombieAI::TriggerPause(float dur)
{
    if (dur <= 0.0f) return;
    m_pause_left = std::max(m_pause_left, dur);
    m_pause_cd = std::max(m_pause_cd, Z_PAUSE_COOLDOWN);
    m_dirty = true;
}

bool ZombieAI::IsPausing() const
{
    return m_pause_left > 0.0f;
}

void ZombieAI::ApplyDamage(SIZE2 damage)
{
    if (m_hp == 0) return;
    if (damage >= m_hp) m_hp = 0;
    else                m_hp -= damage;

    // 피격 시 스턴 갱신(중첩 시 남은 시간이 더 짧으면 연장)
    m_stun_left = std::max(m_stun_left, ZOMBIE_HIT_STUN_SEC);

    if (m_hp == 0) {
        m_attack_left = 0.0f;   // // ApplyDamage: 사망 즉시 행동 차단
        m_pause_left = 0.0f;    // // ApplyDamage: 사망 즉시 정지 해제
    }

    m_dirty = true;
}

bool ZombieAI::IsDead() const
{
    return m_hp == 0;
}

void ZombieAI::SetStun(float seconds)
{
    m_stun_left = std::max(m_stun_left, seconds);
    m_dirty = true;
}

bool ZombieAI::IsStunned() const
{
    return m_stun_left > 0.0f;
}

void ZombieAI::MarkRemoved() noexcept {
    m_removed = true;
    m_dirty = true;
}

bool ZombieAI::IsRemoved() const noexcept {
    return m_removed;
}

void ZombieAI::Update(const std::vector<Vec3>& playerPositions, const std::vector<ZombieAI*>& allZombies, float deltaTime)
{
    if (IsRemoved()) return;

    if (playerPositions.empty()) return;

    if (m_attack_cd > 0.0f) { m_attack_cd -= deltaTime; if (m_attack_cd < 0.0f) m_attack_cd = 0.0f; }
    if (m_attack_left > 0.0f) { m_attack_left -= deltaTime; if (m_attack_left < 0.0f) m_attack_left = 0.0f; }

    //  일시정지(pause) 타이머/쿨다운 감소
    if (m_pause_cd > 0.0f) { m_pause_cd -= deltaTime; if (m_pause_cd < 0.0f) m_pause_cd = 0.0f; }
    if (m_pause_left > 0.0f) { m_pause_left -= deltaTime; if (m_pause_left < 0.0f) m_pause_left = 0.0f; }


    // 스턴 상태면 이동/경로탐색 모두 중지
    if (m_stun_left > 0.0f) {
        m_stun_left -= deltaTime;
        if (m_stun_left < 0.0f) m_stun_left = 0.0f;

        // 멈춤 처리: 이 프레임에선 아무 것도 하지 않음(위치/속도 유지)
        // 클라 동기화를 위해 Dirty 플래그만 유지
        m_dirty = true;
        return;
    }

    // 1. 가장 가까운 플레이어 위치 계산
    Vec3 closest = FindClosestPlayer(playerPositions);

    // 근접 시 길따라가기를 잠깐 멈춤(공격과 별개)
    {
        Vec3 myPos(m_x, 0, m_z);
        float dx = closest.x - myPos.x;
        float dz = closest.z - myPos.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        if (dist <= Z_PAUSE_RANGE && m_pause_cd <= 0.0f && !IsAttacking()) {
            TriggerPause();        // 잠깐 정지
            // m_dirty는 TriggerPause 내에서 true로 설정됨
        }
    }

    // 공격 트리거: 일정 거리 이내 + 쿨다운 끝 + 스턴 아님
    {
        Vec3 myPos(m_x, 0, m_z);
        float dx = closest.x - myPos.x;
        float dz = closest.z - myPos.z;
        float dist = std::sqrt(dx * dx + dz * dz);

        // [추가] 맵 셀 크기를 고려한 최소 공격거리 보정(너무 타이트하면 못 멈출 수 있음)
        const float ATTACK_RANGE_FLOOR = CELL_SIZE * 2.0f; // 셀 2칸 이내면 충분히 근접
        const float effectiveAttackRange = std::max(Z_ATTACK_RANGE, ATTACK_RANGE_FLOOR);

        if (dist <= Z_ATTACK_RANGE && m_attack_cd <= 0.0f && m_stun_left <= 0.0f)
        {
            TriggerAttack();
            // 공격 프레임에선 이동을 멈춰 '들이치기' 모션처럼 보이게 할 수도 있음.
            // 여기서는 이동 로직을 계속 타되, act_type으로 모션을 표현 (GetObjectinfo에서 처리)
        }
    }

    // 2. 타겟 위치 설정 및 경로 재계산
    Vec3 newTarget = closest;

    // 일시정지(pause) 중이면 경로 재계산/타겟 세팅을 잠깐 멈춘다
    if (IsPausing()) {                          // // ZombieAI::Update - pause 중
        m_repath_timer += deltaTime;            // // 일단 타이머만 누적 → 해제 후 즉시 재탐색 유도
        // SetTargetPosition / FindPath 호출 안 함
    }
    else {
        bool needRepath =
            (int)(newTarget.x) != (int)(m_targetX) ||
            (int)(newTarget.z) != (int)(m_targetZ) ||
            m_path.empty() ||
            m_pathIndex >= m_path.size() ||
            m_repath_timer > REPATH_INTERVAL;

        if (needRepath) {
            //DEBUG_LOG("[ZombieAI::Update] ID = %d -> 타겟 변경 또는 재계산 필요", m_id);
            SetTargetPosition(newTarget.x, newTarget.z);   
            FindPath();                                    
            m_repath_timer = 0;                           
        }
        else {
            m_repath_timer += deltaTime;                  
        }
    }


    // 3. 이동 처리 (경로 따라 이동)
    if (m_path.empty() || m_pathIndex >= m_path.size()) return;
    auto& targetNode = m_path[m_pathIndex];
    Vec3 targetPos = GetNodeCenter(targetNode.first, targetNode.second);

    Vec3 currentPos(m_x, 0, m_z);
    Vec3 toTarget = targetPos - currentPos;
    float distance = toTarget.Length();

    DEBUG_LOG("[ZombieAI::Update] ID = " << m_id
        << ", Cur = (" << m_x << ", " << m_z << ")"
        << ", Target = (" << targetNode.first << ", " << targetNode.second << ")"
        << ", WorldTarget = (" << targetPos.x << ", " << targetPos.z << ")"
        << ", Distance = " << distance);


    if (distance < 0.1f) {
        // 공격 모션 중이면 제자리에서 멈춰 때리기 → pathIndex 증가 금지
        if (IsAttacking()) {
            m_dirty = true;          // 상태 브로드캐스트 유지
            return;                  // 이동·인덱스 진행 모두 중단
        }
        m_pathIndex++;               // 평상시에는 다음 노드로 진행
        return;
    }
    if (IsPausing()) {
        // 경로는 그대로 두고, 정지 시간 끝나면 다시 같은 노드로 이어서 추격
        // (여기서 return으로 빠져도 되고, 아래 이동 단계에서 finalMove=0이라 멈춘다)
    }

    Vec3 moveDir = toTarget.Normalize();
    Vec3 nextPos = currentPos + moveDir * Z_move_speed;

    // [REPLACE] 4. 좀비↔좀비 분리력(Separation force) 계산
    Vec3 separation(0, 0, 0);
    {
        for (auto* other : allZombies) {
            if (!other || other->GetID() == m_id) continue;
            float ox = other->GetX();
            float oz = other->GetZ();

            float sx = m_x - ox;
            float sz = m_z - oz;
            float d2 = sx * sx + sz * sz;

            // 반지름 안에 있으면 반발력 추가 (가까울수록 강하게)
            const float r = Z_SEPARATION_RADIUS;
            const float r2 = r * r;
            if (d2 > 0.0001f && d2 < r2) {
                float d = std::sqrt(d2);
                float strength = (r - d) / r; // 0~1
                separation.x += (sx / d) * strength;
                separation.z += (sz / d) * strength;
            }
        }
        if (separation.LengthSquared() > 0.0f) {
            separation = separation.Normalize() * Z_SEPARATION_FORCE;
        }
    }

    // 5. 장애물 충돌 검사
    Vec3 wallPush(0, 0, 0);
    for (int dz = -1; dz <= 1; ++dz) {
        for (int dx = -1; dx <= 1; ++dx) {
            int checkX = (int)(nextPos.x / CELL_SIZE) + dx;
            int checkZ = (int)(nextPos.z / CELL_SIZE) + dz;

            if (checkX < 0 || checkX >= m_map[0].size() ||
                checkZ < 0 || checkZ >= m_map.size()) continue;

            if (m_map[checkZ][checkX] != 0) {
                Vec3 wallCenter = GetNodeCenter(checkX, checkZ);
                Vec3 away = nextPos - wallCenter;
                if (away.LengthSquared() > 0.01f)
                    wallPush += away.Normalize() * 0.1f;
            }
        }
    }

    // 6. 최종 이동 (공격 중 잠깐 정지 → 다시 추격)
    {
        Vec3 finalMove = moveDir * Z_move_speed + wallPush + separation;

        // 공격 중 이동량 배율 적용 (기본 0.0f → 완전 정지)
        if (IsAttacking()) {
            finalMove = finalMove * Z_ATTACK_MOVE_SCALE;   // // 공격중 이동 억제
        }

        m_x += finalMove.x;
        m_z += finalMove.z;
        m_dirty = true; // // 상태 변경 브로드캐스트
    }

}

Vec3 ZombieAI::GetLookVectorToPlayer() const {
    Vec3 zombiePos(m_x, 0, m_z);
    Vec3 targetPos(m_targetX, 0, m_targetZ);
    Vec3 direction = (targetPos - zombiePos).Normalize();
    return direction;
}

Vec3 ZombieAI::GetNodeCenter(int x, int z) const {
    return Vec3((x + 0.5f) * CELL_SIZE, 0, (z + 0.5f) * CELL_SIZE);

}
int ZombieAI::GetID() const { return m_id; }
SIZE2 ZombieAI::GetHP() const { return m_hp; }



bool ZombieAI::IsDirty() const { return m_dirty; }
void ZombieAI::ClearDirty() { m_dirty = false; }
const std::vector<std::pair<int, int>>& ZombieAI::GetPath() const { return m_path; }


float ZombieAI::GetX() const { return m_x; }
float ZombieAI::GetZ() const { return m_z; }
float ZombieAI::GetPlayerX() const { return m_targetX; }
float ZombieAI::GetPlayerZ() const { return m_targetZ; }


Object ZombieAI::GetObjectinfo() const {
    Object info{};
    info.position = Vec3(m_x, 0, m_z);
    info.velocity = Vec3(0, 0, 1); // 현재 방향 지정 안함
	info.look = GetLookVectorToPlayer();
    info.hp = m_hp;
    info.pitch = 0.1f;
    info.gun_type = GunType::BULLET_MAX; // 좀비는 총 안씀
    info.level = 0;
    info.score = 0;
    info.damage = ZOMBIE_DAMAGE;
    info.act_type =
        (m_hp == 0) ? ActionType::DEAD :
        (m_stun_left > 0.0f) ? ActionType::HIT :   // 스턴 표현은 HIT 재사용
        ((m_attack_left > 0.0f) || (m_pause_left > 0.0f)) ? ActionType::ATTACK : // 일시정지 중에도 공격 준비 모션처럼 표시
        ActionType::ZMOVE;

    return info;
}

bool IsAreaClear(const std::vector<std::vector<int>>& map, int x, int z, int radius)
{
    const int H = static_cast<int>(map.size());
    if (H == 0) return false;
    const int W = static_cast<int>(map[0].size());

    // 중심칸 자체가 벽이면 바로 실패
    if (x < 0 || x >= W || z < 0 || z >= H) return false;
    if (map[z][x] != 0) return false;

    const int xmin = std::max(0, x - radius);
    const int xmax = std::min(W - 1, x + radius);
    const int zmin = std::max(0, z - radius);
    const int zmax = std::min(H - 1, z + radius);

    // 네모 반경(맨해튼이 아니라 사각 반경) 검사
    for (int zz = zmin; zz <= zmax; ++zz) {
        for (int xx = xmin; xx <= xmax; ++xx) {
            if (map[zz][xx] != 0) return false;
        }
    }
    return true;
}

// ------------------- 맵 로딩 & 랜덤 위치 -----------------------

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

            // z축반전 위애래
            //int flippedZ = GRID_WIDTH - 1 - z;
            //map[flippedZ][x] = (value == 0) ? 0 : 1;
            map[z][x] = (value == 0) ? 0 : 1; // 0 = 길, 1 = 장애물
        }
    }
	DEBUG_LOG("[OK] 512x512 맵 로드 완료");
    return map;
}

// 5x5 확인용 함수 (중심 기준 2칸 범위 확인)
bool IsSurroundingsFree(const std::vector<std::vector<int>>& map, int x, int z) {
    constexpr int RANGE = 2; // 범위 조절: 1 = 3x3, 2 = 5x5, 3 = 7x7 ...

    for (int dz = -RANGE; dz <= RANGE; ++dz) {
        for (int dx = -RANGE; dx <= RANGE; ++dx) {
            int nx = x + dx;
            int nz = z + dz;

            if (nx < 0 || nx >= GRID_WIDTH || nz < 0 || nz >= GRID_HEIGHT)
                return false;
            if (map[nz][nx] != 0)
                return false;
        }
    }
    return true;
}

std::pair<int, int> GetRandomPosition(const std::vector<std::vector<int>>& map)
{
    const int H = static_cast<int>(map.size());
    if (H == 0) {
        DEBUG_LOG("[ERROR] 맵이 비어있습니다.");
        exit(1);
    }
    const int W = static_cast<int>(map[0].size());

    static std::random_device rd;
    static std::mt19937 gen(rd());
    //    std::uniform_int_distribution<> distX(0, W - 1); // 전체 맵에서 시도
    std::uniform_int_distribution<> distX(100, 150);   
    std::uniform_int_distribution<> distZ(100, 150);

    // 1차: 랜덤 시도
    const int MAX_ATTEMPTS = 1000; // 시도 횟수 
    for (int attempts = 0; attempts < MAX_ATTEMPTS; ++attempts) {
        const int x = distX(gen);
        const int z = distZ(gen);
        if (IsAreaClear(map, x, z, 4)) {               //  반경 4칸 검사
            return { x, z };
        }
    }

    // 2차: 폴백 스캔(확실한 자리를 보장)
    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            if (IsAreaClear(map, x, z, 4)) {
                return { x, z };
            }
        }

    }

    DEBUG_LOG("[ERROR] 스폰 가능한 위치를 찾지 못했습니다. (반경 4칸 조건 과엄 가능)");
    exit(1);
}

std::pair<int, int> GetRandomPlayerPosition(const std::vector<std::vector<int>>& map)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distX(100, 100);
    std::uniform_int_distribution<> distZ(100, 150);

    int attempts = 0;
    while (attempts < 100) {
        int x = distX(gen), z = distZ(gen);
        if (map[z][x] == 0) return { x, z };
        ++attempts;
    }

    DEBUG_LOG("[ERROR] 플레이어 위치 찾기 실패 (해당 구역이 전부 장애물일 수 있음)");
    exit(1);

}



// ======================== 맵 그려보는 용 ========================
void PrintMap2(
    const std::vector<std::vector<int>>& map,
    const std::vector<ZombieAI*>& zombies,
    int startZ, int startX,
    int height, int width,
    float playerX, float playerZ)
{
    int zombieCount = 0;

    // 출력
    for (int z = startZ; z < startZ + height; ++z) {
        for (int x = startX; x < startX + width; ++x) {
            if (z < 0 || z >= GRID_HEIGHT || x < 0 || x >= GRID_WIDTH) {
                std::cout << ' ';
                continue;
            }

            // 출력: 지형
            //char ch = map[z][x] == 1 ? ' ' : '# ';
            char ch = map[z][x] == 1 ? '#' : '0 ';

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
                int zx = static_cast<int>(zombie->GetX() / CELL_SIZE);
                int zz = static_cast<int>(zombie->GetZ() / CELL_SIZE);
                
                

                if (zx == x && zz == z) {
                    ch = 'Z';
                    zombieCount++;
                    break;
                }
            }

            if (x == static_cast<int>(playerX / CELL_SIZE) && z == static_cast<int>(playerZ / CELL_SIZE))
                ch = 'P';

            std::cout << ch;
        }
        std::cout << "\n";
    }

	DEBUG_LOG("[DEBUG] 좀비 수: " << zombieCount);
    if (zombieCount == NUM_ZOMBIES)
		DEBUG_LOG("[ok] 좀비 전부 찍혔습니다!");
    else
		DEBUG_LOG("[bad] 좀비 수가 맞지 않습니다! (" << zombieCount << " / " << NUM_ZOMBIES << ")");
}


//int main()
//{
//    auto map = LoadMapBin("Node/ob_mask_te_1.bin");
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
//        zombie->SetTargetPosition((float)px, (float)pz);
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
//                z->SetTargetPosition((float)px, (float)pz);  // 같은 플레이어에게 재설정
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