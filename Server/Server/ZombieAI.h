#pragma once
#include <vector>
#include <utility>
#include <fstream>
#include <string>
#include <unordered_map>
#include <memory>  
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
constexpr float deltaTime = 1.0f / 60.0f;

constexpr float ZOMBIE_HEIGHT = 1.8f;   // 좀비 키(미터 단위 가정)
constexpr float ZOMBIE_RADIUS = 0.35f;  // 어깨/몸통 반지름

constexpr float Z_ATTACK_RANGE = 1.2f;          // 월드 단위 (CELL_SIZE에 맞춰 조정 가능)
constexpr float Z_ATTACK_COOLDOWN = 1.0f;       // 초
constexpr float Z_ATTACK_ANIM_TIME = 1.5f;     // 공격 모션 유지 시간(초)

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

constexpr float Z_DEATH_DESPAWN_SEC = 1.5f;             // 죽는 애니메이션 길이(예: 1.5초)


class ZombieAI
{
public:
    class AStar;

    ZombieAI(const std::vector<std::vector<int>>& map, int id);
    ~ZombieAI();

    void SetPosition(float x, float z);   // 위치 및 타겟 설정
    //void SetPlayerPosition(float x, float z);

    void SetTargetPosition(float x, float z);
	void SetHP(int hp);                   // 체력 설정

    // ---[Attack]---
    void TriggerAttack(float animTime = Z_ATTACK_ANIM_TIME);
    bool IsAttacking() const;

    // ---[Pause]---
    void TriggerPause(float dur = Z_PAUSE_TIME);  // // ZombieAI::TriggerPause - 근접 시 잠깐 멈추기
    bool IsPausing() const;                       // // ZombieAI::IsPausing     - 현재 일시정지 여부


    // ---[Hit Stun]---
    // 외부(서버 피격 처리)에서 스턴 부여
    void SetStun(float seconds = ZOMBIE_HIT_STUN_SEC);
    // 현재 스턴 상태 조회
    bool IsStunned() const;

    Vec3 FindClosestPlayer(const std::vector<Vec3>& playerPositions);
    void FindPath();                      // AI 동작

    Vec3 AvoidPlayers(const std::vector<Vec3>& playerPositions); // 플레이어 회피

    // 총알 피격 처리: damage 만큼 HP 감소 (0 하한), Dirty 플래그 자동 세팅
    void ApplyDamage(SIZE2 damage);

    // 현재 사망 상태(HP==0)인지 조회
    bool IsDead() const;

    // 제거 상태 제어
    void MarkRemoved() noexcept;      // // ZombieAI::MarkRemoved - 사망 브로드캐스트 후 호출
    bool IsRemoved() const noexcept;  // // ZombieAI::IsRemoved - 업데이트/충돌 제외 판단

    bool WasRemoveNotified() const noexcept;   // REMOVE 송신 여부
    void MarkRemoveNotified() noexcept;        // REMOVE 보낼 때 세팅


    void Update(const std::vector<Vec3>& playerPositions, const std::vector<ZombieAI*>& allZombies, float deltaTime);


    std::vector<std::pair<int, int>> FindPathToPlayer();   // 정보 조회
    const std::vector<std::pair<int, int>>& GetPath() const;

    int GetID() const;   // 좀비 ID 및 상태 조회
    Vec3 GetPosition() const { return Vec3{ m_x, 0.0f, m_z }; }
    SIZE2 GetHP() const;

    //void AddDamage(SIZE2 amount); // 체력 설정
    
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
    SIZE2 m_hp = ZOMBIE_HP;
    std::vector<std::pair<int, int>> m_path;

    const std::vector<std::vector<int>>& m_map;
    size_t m_pathIndex;

    bool m_dirty = true;

    Vec3 GetNodeCenter(int x, int z) const;

 
    float m_attack_cd = 0.0f;    // 공격 쿨다운 및 공격 애니 타이머(초)
    float m_attack_left = 0.0f;

    float m_pause_left = 0.0f;   // 남은 정지 시간
    float m_pause_cd = 0.0f;   // 정지 쿨다운
   
    float m_stun_left = 0.0f;  // 피격 스턴 남은 시간(초)

    float m_death_left = 0.0f;   // 죽음 연출 남은 시간

    // 사망 후 제거 브로드캐스트 중복 방지
    bool m_removed = false;           // // ZombieAI::m_removed - 제거된 개체는 true
    bool m_remove_notified = false;   // // REMOVE 송신 플래그

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


