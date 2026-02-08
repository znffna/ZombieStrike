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

constexpr float STUN_TIME = 1.0f; // 추가: 생성할 좀비 수
constexpr float RUNNER_speed = 4.0f; // 추가: 생성할 좀비 수

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

    if (m_map[endZ][endX] != 0) { // // AStar::FindPath - 목표가 벽이면 즉시 실패
        return {};
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

    std::vector<Node*> allocated;          // // AStar::FindPath - 모든 노드 소유(일괄 해제)
    allocated.reserve(ASTAR_MAX_EXPANSIONS + 32);
    allocated.push_back(startNode);        // // AStar::FindPath - startNode 등록

    std::vector<std::pair<int, int>> path;
    auto cleanup_open = [&]() { // // AStar::FindPath - DEBUG/SAFE: openList 잔여 노드 해제
        while (!openList.empty()) openList.pop();
       /* while (!openList.empty()) {
            delete openList.top();
            openList.pop();
        }*/
    };

    int expansions = 0; // // AStar::FindPath - DEBUG/SAFE: 확장 카운터

    while (!openList.empty())
    {
        Node* current = openList.top(); openList.pop();
        int key = current->z * m_width + current->x;

        if (closedList.find(key) != closedList.end()) {
            //delete current;
            continue;
        }

        closedList.insert(key);

        // // AStar::FindPath - 탐색 상한(도달 불가능 목표에서 폭주 방지)
        if (++expansions > ASTAR_MAX_EXPANSIONS) { // // AStar::FindPath - 탐색 상한(폭주 방지)
            //delete current;                         // // AStar::FindPath - current 해제
            //path.clear();
            cleanup_open();                         // // AStar::FindPath - 잔여 해제
            break;
        }

        if (current->x == endX && current->z == endZ)
        {
            Node* node = current;
            while (node) {
                path.emplace_back(node->x, node->z);
                node = node->parent;
            }
            std::reverse(path.begin(), path.end());
            //delete current;      // // AStar::FindPath - current 해제
            cleanup_open();      // // AStar::FindPath - 잔여 해제
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
            allocated.push_back(neighbor); // // AStar::FindPath - neighbor 등록

            float stepCost = (dir < 4) ? 1.0f : 1.41421356f;  // // A* 직/대각 비용
            neighbor->gCost = current->gCost + stepCost;      // // A* 누적 gCost 갱신

            neighbor->hCost = Heuristic(nx, nz, endX, endZ);
            neighbor->parent = current;
            openList.push(neighbor);
        }
    }
    cleanup_open(); // // AStar::FindPath - 안전 정리(잔여가 있을 경우)
    for (auto* n : allocated) delete n; // // AStar::FindPath - 일괄 해제
    return path;
}

// ------------------- ZombieAI 구현 -----------------------

void ZombieAI::SetType(ZombieType type)
{
    m_type = type;

    switch (m_type)
    {
    case ZombieType::RUNNER:
        m_move_speed = Z_move_speed * RUNNER_speed;
        m_damage = ZOMBIE_DAMAGE * 0.8f;
        m_attack_cooldown = Z_ATTACK_COOLDOWN * 0.75f;
        m_hp = static_cast<SIZE2>(ZOMBIE_HP * 0.7f);
        break;

    case ZombieType::TANKER:
        m_move_speed = Z_move_speed * 0.65f;
        m_damage = ZOMBIE_DAMAGE * 1.6f;
        m_attack_cooldown = Z_ATTACK_COOLDOWN * 1.10f;
        m_hp = static_cast<SIZE2>(ZOMBIE_HP * 2.2f);
        break;

    default: // NORMAL
        m_move_speed = Z_move_speed;
        m_damage = ZOMBIE_DAMAGE;
        m_attack_cooldown = Z_ATTACK_COOLDOWN;
        m_hp = ZOMBIE_HP;
        break;
    }
    m_move_speed *= m_speed_mul; 

    m_dirty = true;
}

void ZombieAI::ApplySpeedRandomMul(float mul)
{
    if (mul < 0.8f) mul = 0.8f;   // 하한
    if (mul > 1.3f) mul = 1.3f;   // 상한
    m_speed_mul = mul;          
    m_dirty = true;              
}

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
    m_attack_cd = std::max(m_attack_cd, m_attack_cooldown);
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
    if (m_hp == 0) return;  // 사망 상태면 무시
    if (damage >= m_hp) m_hp = 0;
    else                m_hp -= damage;
    
    m_stun_left = std::max(m_stun_left, ZOMBIE_HIT_STUN_SEC);   // 피격 시 스턴 갱신(중첩 시 남은 시간이 더 짧으면 연장)

    m_attack_left = 0.0f;   
    m_pause_left = 0.0f;

    if (m_hp > 0) {  // HIT 상태 강제 트리거
        m_hit_visual_left = std::max(m_hit_visual_left, STUN_TIME);
        m_force_hit = true;          
        m_act_type = ActionType::HIT;
    }

    if (m_hp == 0) {
        //m_attack_left = 0.0f;   // 사망 즉시 행동 차단
        //m_pause_left = 0.0f;    // 사망 즉시 정지 해제
        m_act_type = (SIZE1)ActionType::DEATH;
    }

    m_dirty = true;
}

void ZombieAI::AddPendingDamage(SIZE2 damage) noexcept
{
    if (damage == 0) return;
    m_pending_damage.fetch_add(damage, std::memory_order_relaxed);
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

    const SIZE2 pending = m_pending_damage.exchange(0, std::memory_order_acq_rel);
    if (pending > 0) {

        std::cout << "[DMG-DBG] zid=" << m_id
            << " pending=" << pending
            << " hp_before=" << (int)m_hp
            << "\n";

        ApplyDamage(pending); 

        std::cout << "[DMG-DBG] zid=" << m_id
            << " hp_after=" << (int)m_hp
            << " stun_left=" << m_stun_left
            << " hitVis=" << m_hit_visual_left
            << "\n";

    }

    if (playerPositions.empty()) return;

    if (m_hit_visual_left > 0.0f) {
        m_hit_visual_left -= deltaTime;
        if (m_hit_visual_left < 0.0f) m_hit_visual_left = 0.0f;
    }

    if (m_attack_cd > 0.0f) { m_attack_cd -= deltaTime; if (m_attack_cd < 0.0f) m_attack_cd = 0.0f; }
    if (m_attack_left > 0.0f) { m_attack_left -= deltaTime; if (m_attack_left < 0.0f) m_attack_left = 0.0f; }

    //  일시정지(pause) 타이머/쿨다운 감소
    if (m_pause_cd > 0.0f) { m_pause_cd -= deltaTime; if (m_pause_cd < 0.0f) m_pause_cd = 0.0f; }
    if (m_pause_left > 0.0f) { m_pause_left -= deltaTime; if (m_pause_left < 0.0f) m_pause_left = 0.0f; }

    if (m_stun_left > 0.0f) {                     
        m_stun_left -= deltaTime;                
        if (m_stun_left < 0.0f) m_stun_left = 0.0f;
       
        m_attack_left = 0.0f;                    
        m_pause_left = 0.0f;                    

        m_dirty = true;                           
        return;                                   
    }

    if (m_hit_visual_left > 0.0f) {               
        m_attack_left = 0.0f;                     
        m_pause_left = 0.0f;                     
        m_dirty = true;                           
        return;                                   
    }

    // 스턴 상태면 이동/경로탐색 모두 중지
    //if (m_stun_left > 0.0f) {
    //    m_stun_left -= deltaTime;
    //    if (m_stun_left < 0.0f) m_stun_left = 0.0f;

    //    // 멈춤 처리: 이 프레임에선 아무 것도 하지 않음(위치/속도 유지)
    //    // 클라 동기화를 위해 Dirty 플래그만 유지
    //    m_dirty = true;
    //    return;
    //}
 
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

        //if (dist <= Z_ATTACK_RANGE && m_attack_cd <= 0.0f && m_stun_left <= 0.0f)
        if (dist <= effectiveAttackRange && m_attack_cd <= 0.0f && m_stun_left <= 0.0f && !IsAttacking()) 
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
        // // ZombieAI::Update - 경로 실패 쿨다운 감소(폭주 방지)
        if (m_repath_fail_cd > 0.0f) {
            m_repath_fail_cd -= deltaTime;
            if (m_repath_fail_cd < 0.0f) m_repath_fail_cd = 0.0f;
        }

        // // ZombieAI::Update - 목표를 "셀 단위"로 비교(불필요 재탐색 방지)
        const int goal_cx = (int)(newTarget.x / CELL_SIZE);
        const int goal_cz = (int)(newTarget.z / CELL_SIZE);

        const bool goalChanged = (goal_cx != m_last_goal_cx) || (goal_cz != m_last_goal_cz);

        m_repath_timer += deltaTime;

        // // ZombieAI::Update - 재탐색은 1초 단위로만, 그리고 목표가 바뀌거나 경로가 끊겼을 때만
        const bool needRepath =
            (m_repath_timer >= REPATH_INTERVAL) &&
            (goalChanged || m_path.empty() || m_pathIndex >= m_path.size());

        if (needRepath && m_repath_fail_cd <= 0.0f) { // // ZombieAI::Update - 실패 쿨다운 중엔 A* 금지
            SetTargetPosition(newTarget.x, newTarget.z); // // ZombieAI::Update - 타겟 갱신
            FindPath();                                  // // ZombieAI::Update - 경로 재계산

            m_last_goal_cx = goal_cx;                    // // ZombieAI::Update - 마지막 목표 셀 갱신
            m_last_goal_cz = goal_cz;
            m_repath_timer = 0.0f;                       // // ZombieAI::Update - 타이머 리셋

            if (m_path.empty()) {
                m_repath_fail_cd = REPATH_FAIL_COOLDOWN; // // ZombieAI::Update - 경로 실패 시 잠깐 쉬고 재시도
            }
        }
    }

    //else {
    //    bool needRepath =
    //        (int)(newTarget.x) != (int)(m_targetX) ||
    //        (int)(newTarget.z) != (int)(m_targetZ) ||
    //        m_path.empty() ||
    //        m_pathIndex >= m_path.size() ||
    //        m_repath_timer > REPATH_INTERVAL;
    //
    //    if (needRepath) {
    //        //DEBUG_LOG("[ZombieAI::Update] ID = %d -> 타겟 변경 또는 재계산 필요", m_id);
    //        SetTargetPosition(newTarget.x, newTarget.z);   
    //        FindPath();                                    
    //        m_repath_timer = 0;                           
    //    }
    //    else {
    //        m_repath_timer += deltaTime;                  
    //    }
    //}


    // 3. 이동 처리 (경로 따라 이동)
    //if (m_path.empty() || m_pathIndex >= m_path.size()) return;
    if (m_path.empty() || m_pathIndex >= m_path.size()) {
        Vec3 myPos(m_x, 0, m_z);
        Vec3 toPlayer = (closest - myPos);
        if (toPlayer.LengthSquared() > 0.0001f) {
            Vec3 moveDir = toPlayer.Normalize();
            Vec3 finalMove = moveDir * m_move_speed;

            if (IsAttacking() || IsPausing()) finalMove = finalMove * 0.0f; // // ZombieAI::Update - 공격/정지 중엔 정지

            Vec3 nextPos = myPos + finalMove; // // ZombieAI::Update - 다음 위치
            int nx = (int)(nextPos.x / CELL_SIZE);
            int nz = (int)(nextPos.z / CELL_SIZE);

            if (nx < 0 || nx >= (int)m_map[0].size() || nz < 0 || nz >= (int)m_map.size() || m_map[nz][nx] != 0) {
                finalMove = Vec3(0, 0, 0); // // ZombieAI::Update - 벽이면 이동 0
            }

            m_x += finalMove.x;
            m_z += finalMove.z;
            m_dirty = true;
        }
        return;
    }


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
    Vec3 nextPos = currentPos + moveDir * m_move_speed;

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
        Vec3 finalMove = moveDir * m_move_speed + wallPush + separation;

        // 공격 중 이동량 배율 적용 (기본 0.0f → 완전 정지)
        if (IsAttacking()) {
            finalMove = finalMove * Z_ATTACK_MOVE_SCALE;   // // 공격중 이동 억제
        }

        //
        const float prevX = m_x; // DEBUG(이동 전 좌표)
        const float prevZ = m_z;
        //

        m_x += finalMove.x;
        m_z += finalMove.z;
        m_dirty = true; // // 상태 변경 브로드캐스트

        //
        //static float s_dbg_accum = 0.0f;                   // // ZombieAI::Update - DEBUG 누적 타이머
        //s_dbg_accum += deltaTime;                          // // ZombieAI::Update - DEBUG 누적 타이머
        //static int s_watch_id = -1;                 // // ZombieAI::Update - DEBUG(감시할 좀비 id 1마리)
        //if (s_watch_id == -1) s_watch_id = m_id;    // // ZombieAI::Update - DEBUG(처음 호출된 좀비 id로 고정)

        //if (m_id == s_watch_id && s_dbg_accum >= 0.5f) {          // // ZombieAI::Update - DEBUG(좀비0만, 0.5초마다)
        //    s_dbg_accum = 0.0f;

        //    const float moved2 =
        //        (m_x - prevX) * (m_x - prevX) + (m_z - prevZ) * (m_z - prevZ);

        //    std::cout
        //        << "[ZDBG][Update] id=" << m_id
        //        << " dt=" << deltaTime
        //        << " pos=(" << m_x << "," << m_z << ")"
        //        << " prev=(" << prevX << "," << prevZ << ")"
        //        << " finalMove=(" << finalMove.x << "," << finalMove.z << ")"
        //        << " moved2=" << moved2
        //        << " pathIdx=" << m_pathIndex
        //        << " pathSz=" << m_path.size()
        //        << " atkLeft=" << m_attack_left
        //        << " stunLeft=" << m_stun_left
        //        << " pauseLeft=" << m_pause_left
        //        << "\n";
        //}
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
    info.damage = static_cast<float>(m_damage);

    ActionType act = ActionType::ZMOVE;

    if (m_hp == 0)
    {
        act = ActionType::DEATH;                
    }
    else if (m_hit_visual_left > 0.0f)
    {
        info.velocity = Vec3(0, 0, 0);
        act = ActionType::HIT;
    }
    else if (m_attack_left > 0.0f)
    {
        act = ActionType::ATTACK;
    }
    //else if (m_stun_left > 0.0f)
    //{
    //    act = ActionType::HIT;                  
    //}
    else if (m_pause_left > 0.0f)
    {
        act = ActionType::SCREAM;     // pause를 SCREAM으로 보여주고 싶으면 이걸로
    }
    else
    {
        act = ActionType::ZMOVE;                 
    }

    info.act_type = (SIZE1)act;                 

    //std::cout << "[ZOMBIE INFO] name=Zombie_" << m_id
    //    << " act_type=" << (int)info.act_type
    //    << "(" << ToString((ActionType)info.act_type) << ")"
    //    << "\n";

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

    // std::uniform_int_distribution<> distX(0, W - 1); // 전체 맵에서 시도

    //std::uniform_int_distribution<> distX(100, 130);   
    //std::uniform_int_distribution<> distZ(100, 110);

    std::uniform_int_distribution<> distX(150, 151);// 가로 좌 ->우 
    std::uniform_int_distribution<> distZ(180, 181);


    //std::uniform_int_distribution<> distX(150, 151);// 가로 좌 ->우 
    //std::uniform_int_distribution<> distZ(100, 101);

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


// 포인트 수(2~4)에 따라 가능한 한 균등하게 분배
std::pair<int, int> GetSpawnPointByIndexN(
    const std::vector<std::vector<int>>& /*map*/,
    const std::vector<std::pair<int, int>>& points,
    int spawn_index,
    int total_spawns)
{
    // // GetSpawnPointByIndexN: 방어 코드 (포인트가 1개 이하거나 4개 초과면 0번 사용)
    if (points.empty()) return { -1, -1 };
    const int P = static_cast<int>(points.size());
    if (P == 1) return points[0];

    // 포인트 개수 제한(요구: 최대 4개)
    const int MAX_POINTS = 4;
    const int useP = std::min(P, MAX_POINTS);

    // // GetSpawnPointByIndexN: 총 스폰 수를 useP로 가능한 한 균등 분할
    // 각 포인트에 배정되는 기본 몫(base), 그리고 앞에서부터 remainder개 포인트에 +1 배정
    const int base = total_spawns / useP;
    const int remainder = total_spawns % useP;

    // spawn_index가 어느 구간(bucket)에 속하는지 계산
    int acc = 0;
    for (int bucket = 0; bucket < useP; ++bucket) {
        const int size_of_bucket = base + ((bucket < remainder) ? 1 : 0);
        if (spawn_index < acc + size_of_bucket) {
            return points[bucket];
        }
        acc += size_of_bucket;
    }

    // // GetSpawnPointByIndexN: 이론상 도달 X, 방어적 처리
    return points[(useP - 1)];
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
//    // // [main] - 맵 로드
//    auto map = LoadMapBin("node/ob_mask_te_1.bin");
//
//    std::vector<ZombieAI*> zombies;
//
//    // // [main] - 플레이어 위치 선정
//    auto [px, pz] = GetRandomPlayerPosition(map);
//    DEBUG_LOG("[TEST] player pos = (" << px << ", " << pz << ")");
//
//    // // [main] - 좀비 스폰
//    for (int i = 0; i < NUM_ZOMBIES; ++i)
//    {
//        auto [zx, zz] = GetRandomPosition(map);
//
//        ZombieAI* zombie = new ZombieAI(map, i + 1);
//        zombie->SetPosition((float)zx * CELL_SIZE, (float)zz * CELL_SIZE);
//        zombie->SetTargetPosition((float)px * CELL_SIZE, (float)pz * CELL_SIZE);
//        zombie->FindPath();   // // ZombieAI::FindPath
//
//        zombies.push_back(zombie);
//    }
//
//    DEBUG_LOG("[TEST] zombies spawned");
//
//    // // [main] - 테스트 루프
//    while (true)
//    {
//        std::cout << "[ENTER] 리스폰 / [ESC] 종료\n";
//
//        int key = _getch();
//
//        if (key == 13) // ENTER
//        {
//            for (auto* z : zombies)
//            {
//                auto [nx, nz] = GetRandomPosition(map);
//                z->SetPosition((float)nx * CELL_SIZE, (float)nz * CELL_SIZE);
//                z->SetTargetPosition((float)px * CELL_SIZE, (float)pz * CELL_SIZE);
//                z->FindPath();
//            }
//
//            PrintMap2(
//                map,
//                zombies,
//                0, 0,
//                GRID_HEIGHT,
//                GRID_WIDTH,
//                px * CELL_SIZE,
//                pz * CELL_SIZE
//            );
//        }
//        else if (key == 27) // ESC
//        {
//            break;
//        }
//    }
//
//    // // [main] - 정리
//    for (auto* z : zombies)
//        delete z;
//
//    zombies.clear();
//
//    return 0;
//}
