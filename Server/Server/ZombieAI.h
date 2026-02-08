#pragma once
#include <vector>
#include <utility>
#include <fstream>
#include <string>
#include <unordered_map>
#include <atomic> 
#include "../../protocol.h"

// 필수 정보 
constexpr float my_gCost = 1.0f;             // 이동 비용
constexpr float ZOMBIE_HALF_SIZE = 0.4f;     // 좀비 AABB 반 사이즈
constexpr SIZE2 ZOMBIE_HP = 500;             // 좀비 초기 체력
constexpr float ZOMBIE_DAMAGE = 10;          // 좀비 초기 공격력 
constexpr float Z_move_speed = 0.03f;         // 좀비 

constexpr float WORLD_WIDTH = 250.0f;
constexpr float WORLD_HEIGHT = 250.0f;
constexpr float GRID_WIDTH = 512.0f;
constexpr float GRID_HEIGHT = 512.0f;
constexpr float CELL_SIZE = WORLD_WIDTH / GRID_WIDTH; // 0.488..

static constexpr float REPATH_INTERVAL = 1.0f; // 1초마다 재탐색 허용
static constexpr int   ASTAR_MAX_EXPANSIONS = 4000;  // // AStar::FindPath - 탐색 상한(막힌 목표 폭주 방지)
static constexpr float REPATH_FAIL_COOLDOWN = 0.75f; // // ZombieAI::Update - 경로 실패 시 재탐색 쿨다운

constexpr float deltaTime = 1.0f / 60.0f;

constexpr float ZOMBIE_HEIGHT = 1.8f;   // 좀비 키(미터 단위 가정)
constexpr float ZOMBIE_RADIUS = 0.5f;  // 어깨/몸통 반지름

constexpr float Z_ATTACK_RANGE = 1.2f;                // 월드 단위 (CELL_SIZE에 맞춰 조정 가능)
constexpr float Z_ATTACK_COOLDOWN = 1.0f;             // 초
constexpr float Z_ATTACK_ANIM_TIME = 2.63333344f;     // 공격 모션 유지 시간(초)

//  좀비-좀비 분리력 파라미터
constexpr float Z_SEPARATION_RADIUS = 1.2f;     // 이 거리 안에 들어오면 서로 밀어냄
constexpr float Z_SEPARATION_FORCE = 0.12f;     // 프레임당 추가 이동량 스케일

// 좀비 피격 스턴 시간(초)
constexpr float ZOMBIE_HIT_STUN_SEC = 1.5f;

// 공격 중 이동 스케일 (0.0 = 완전 정지, 0.3 = 살짝 미끄러짐)
constexpr float Z_ATTACK_MOVE_SCALE = 0.0f;   // // 공격 모션 동안 이동량 배율

constexpr float Z_PAUSE_RANGE       = CELL_SIZE * 2.0f; // 플레이어와 이 거리 이하로 근접 시 정지 트리거
constexpr float Z_PAUSE_TIME        = 0.40f;            // 멈추는 시간(초)
constexpr float Z_PAUSE_COOLDOWN    = 1.20f;            // 다시 멈추기까지의 쿨다운(초)
constexpr float Z_PAUSE_MOVE_SCALE  = 0.0f;             // 정지 중 이동량 배율(0=완전정지)

enum class ZombieType : uint8_t
{
    NORMAL = 0,
    RUNNER = 1,
    TANKER = 2,
};

class ZombieAI
{
public:
   
    void SetType(ZombieType type);   // 타입 기반 스탯 적용 함수 추가
    ZombieType GetType() const { return m_type; }   // 타입 조회 추가(디버그/패킷 name 구분용)

    class AStar;

    ZombieAI(const std::vector<std::vector<int>>& map, int id);

    void SetPosition(float x, float z);   // 위치 및 타겟 설정
    //void SetPlayerPosition(float x, float z);

    void SetTargetPosition(float x, float z);
	void SetHP(int hp);                   // 체력 설정

    // ---[Attack]---
    void TriggerAttack(float animTime = Z_ATTACK_ANIM_TIME);
    bool IsAttacking() const;

    //  길따라가기 일시정지(공격과 별개)
    void TriggerPause(float dur = Z_PAUSE_TIME);  // 근접 시 잠깐 멈추기
    bool IsPausing() const;                       // 현재 일시정지 여부

    // ---[Hit Stun]---
    void SetStun(float seconds = ZOMBIE_HIT_STUN_SEC);  // 외부(서버 피격 처리)에서 스턴 부여
    bool IsStunned() const; // 현재 스턴 상태 조회

    Vec3 FindClosestPlayer(const std::vector<Vec3>& playerPositions);
    void FindPath();                      // AI 동작

    Vec3 AvoidPlayers(const std::vector<Vec3>& playerPositions); // 플레이어 회피
    
    SIZE1 GetActType() const { return m_act_type; } // 현재 액션 타입 조회(네트워크 송신/디버그용)
    void SetActType(SIZE1 t) { m_act_type = t; }    // 현재 액션 타입 설정(내부 상태머신/피격 반영용)

    void ApplyDamage(SIZE2 damage); // 총알 피격 처리: damage 만큼 HP 감소 (0 하한), Dirty 플래그 자동 세팅
    void AddPendingDamage(SIZE2 damage) noexcept;

    bool m_force_hit = false;   // ApplyDamage에서 세트되는 강제 HIT 플래그
    
    bool IsDead() const;    // 현재 사망 상태(HP==0)인지 조회
        
    // 제거 상태 제어
    void MarkRemoved() noexcept;      // 사망 브로드캐스트 후 호출
    bool IsRemoved() const noexcept;  // 업데이트/충돌 제외 판단

    void Update(const std::vector<Vec3>& playerPositions, const std::vector<ZombieAI*>& allZombies, float deltaTime);


    std::vector<std::pair<int, int>> FindPathToPlayer();   // 정보 조회
    const std::vector<std::pair<int, int>>& GetPath() const;

    int GetID() const;   // 좀비 ID 및 상태 조회
    Vec3 GetPosition() const { return Vec3{ m_x, 0.0f, m_z }; }
    SIZE2 GetHP() const;

    //void AddDamage(SIZE2 amount); // 체력 설정
    
    Object GetObjectinfo() const; // 패킷 정보 반환   

    bool IsDirty() const;
    void ClearDirty();

    Vec3 GetLookVectorToPlayer() const;
    float GetX() const;
    float GetZ() const;
    float GetPlayerX() const;
    float GetPlayerZ() const;

private:
   
    ZombieType m_type = ZombieType::NORMAL;  // 좀비 타입 저장

    
    float m_move_speed = Z_move_speed;  // 타입별 이동속도(기존 Z_move_speed 대체)

    
    float m_damage = ZOMBIE_DAMAGE; // 타입별 데미지(기존 ZOMBIE_DAMAGE 대체)

    
    float m_attack_cooldown = Z_ATTACK_COOLDOWN;    // 타입별 공격쿨(기존 Z_ATTACK_COOLDOWN 대체)

   // AStar* m_astar;
    std::unique_ptr<AStar> m_astar;

    float m_repath_timer = 0.0f;
    
    float m_repath_fail_cd = 0.0f; // - 경로 실패 쿨다운
    int   m_last_goal_cx = -1;     // - 마지막 목표 셀 X
    int   m_last_goal_cz = -1;     // - 마지막 목표 셀 Z


    int m_id;
    float m_x, m_z;
    float m_targetX, m_targetZ;
    SIZE2 m_hp = ZOMBIE_HP;
    std::vector<std::pair<int, int>> m_path;

    const std::vector<std::vector<int>>& m_map;
    size_t m_pathIndex;

    bool m_dirty = true;
    std::atomic<SIZE2> m_pending_damage{ 0 };

    Vec3 GetNodeCenter(int x, int z) const;

    // 공격 쿨다운 및 공격 애니 타이머(초)
    float m_attack_cd = 0.0f;
    float m_attack_left = 0.0f;

    float m_pause_left = 0.0f;   // 남은 정지 시간
    float m_pause_cd = 0.0f;   // 정지 쿨다운

    SIZE1 m_act_type = (SIZE1)ActionType::ZMOVE; 
   
    float m_stun_left = 0.0f;    // 피격 스턴 남은 시간(초)

    float m_hit_visual_left = 0.0f;
 
    bool m_removed = false;   // 사망 후 제거 브로드캐스트 중복 방지 , 제거된 개체는 true      
};

// Util 함수
std::vector<std::vector<int>> LoadMapBin(const std::string& filename);
std::pair<int, int> GetRandomPosition(const std::vector<std::vector<int>>& map);
std::pair<int, int> GetSpawnPointByIndexN(
    const std::vector<std::vector<int>>& map,
    const std::vector<std::pair<int, int>>& points,
    int spawn_index,
    int total_spawns);

// 스폰 후보 타일이 반경 r 칸 내에 장애물이 없는지 확인
bool IsAreaClear(const std::vector<std::vector<int>>&map, int x, int z, int radius = 4); // // IsAreaClear


