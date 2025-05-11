#pragma once
#include <vector>
#include <utility>
#include <fstream>
#include <string>
#include <unordered_map>
#include "../../protocol.h"

// -------------------------------
// 상수 정의
// -------------------------------
constexpr float my_gCost            = 1.0f;         // A* 탐색 기본 이동 비용 (대각선 제외)
constexpr float ZOMBIE_HALF_SIZE    = 0.4f;         // 좀비 AABB 충돌 크기 반지름 (가로 절반 단위)
constexpr SIZE2 ZOMBIE_HP           = 500;          // 좀비 기본 체력
constexpr float ZOMBIE_DAMAGE       = 10.0f;        // 좀비가 입히는 기본 근접 데미지
constexpr float Z_move_speed        = 0.05f;        // 좀비 이동 속도 (unit/frame)

constexpr float WORLD_WIDTH         = 250.0f;       // 게임 월드 전체 너비 (단위: 유닛)
constexpr float WORLD_HEIGHT        = 250.0f;       // 게임 월드 전체 높이
constexpr float GRID_WIDTH          = 512.0f;       // 맵의 가로 셀 개수
constexpr float GRID_HEIGHT         = 512.0f;       // 맵의 세로 셀 개수
constexpr float CELL_SIZE           = WORLD_WIDTH / GRID_WIDTH;  // 셀 한 칸 크기 (단위: 유닛)

constexpr float REPATH_INTERVAL     = 1.0f;         // A* 재탐색 주기 (초 단위)
constexpr float DELTATIME           = 1.0f / 60.0f; // 프레임 시간 (60FPS 기준)
constexpr float REPALSTRENGTH       = 0.05f;        // 좀비 간 반발력 (충돌 시 힘의 크기)
constexpr float attackRange         = 1.5f;

class SESSION;

class ZombieAI
{
public:
    class AStar;

    ZombieAI(const std::vector<std::vector<int>>& map, int id);

    // 위치 및 타겟 설정
    void SetPosition(float x, float z);                  // 좀비 위치 설정
    void SetTargetPosition(float x, float z);            // 플레이어 추적 목표 위치 설정
    void SetHP(int hp);                                  // 좀비 체력 설정

    // 플레이어 탐지 및 경로 설정
    Vec3 FindClosestPlayer(const std::vector<Vec3>& playerPositions);  // 가장 가까운 플레이어 위치 반환
    void FindPath();                                     // A* 경로 탐색 실행
    void Update(const std::vector<Vec3>& playerPositions,
        const std::vector<ZombieAI*>& allZombies);      // 매 프레임 업데이트 (경로 재탐색, 이동 등)

    // 경로 조회
    std::vector<std::pair<int, int>> FindPathToPlayer();              // 경로 계산 결과 반환
    const std::vector<std::pair<int, int>>& GetPath() const;         // 경로 참조 반환


    // 상태 조회
    int GetID() const;                                               // 좀비 고유 ID
    Vec3 GetPosition() const { return Vec3{ zom_x, 0.0f, zom_z }; }  // 현재 위치 반환 (Y는 0 고정)
    SIZE2 GetHP() const;                                             // 현재 체력 조회

    // bool CheckBulletHit(const Vec3& bulletPos, const Vec3& bulletDir, float bulletRadius); // 총알과의 충돌 체크
    void ApplyDamage(SIZE2 damage);                                  // 데미지 적용 및 체력 감소
    bool IsDead() const;     


    // 상태 변경 감지 (Dirty 플래그)
    bool IsDirty() const;            // 브로드캐스트 대상인지 여부
    void ClearDirty();               // 상태 동기화 후 플래그 클리어

    // 위치 상세 접근 - 유틸 
    float GetX() const;
    float GetZ() const;
    float GetPlayerX() const;        // 타겟팅된 플레이어 좌표 반환
    float GetPlayerZ() const;
    ActionType GetActionType() const; // 행동 타입 반환

private:
    int zom_id;                      // 좀비 고유 ID
    float zom_x, zom_z;              // 현재 위치
    SIZE2 zom_hp;                    // 현재 체력
    ActionType zom_act_type = ActionType::NONE;
	float m_attack_cooldown = 0.0f; // 공격 쿨타임


    float zom_targetX, zom_targetZ;  // 추적 타겟 위치
    int zom_targetPlayerID;          // 추적 중인 플레이어의 ID

    std::unique_ptr<AStar> zom_astar; // A* 경로 탐색기
    float m_repath_timer = 0.0f;      // 마지막 탐색 후 시간 누적

    std::vector<std::pair<int, int>> zom_path; // A* 경로 결과
    size_t zom_pathIndex;            // 현재 경로 인덱스

    bool zom_needsUpdate = true;     // 서버 → 클라 동기화 플래그 (Dirty 개념)

    Vec3 GetNodeCenter(int x, int z) const; // 셀 중심 좌표 계산용 - 유틸
};

// -------------------------------
// 외부 유틸 함수
// -------------------------------
std::vector<std::vector<int>> LoadMapBin(const std::string& filename);
std::pair<int, int> GetRandomPosition(const std::vector<std::vector<int>>& map);



