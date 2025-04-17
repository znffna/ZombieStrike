#pragma once
#include <cstdint>

// --------------------------
// 서버/클라 공통 상수 정의
// --------------------------
constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int MAX_NAME_SIZE = 20;

// 서버의 최대 세션 수
constexpr int MAX_USER = 5000; 
// 최대 플레이어 수
constexpr short MAX_PLAYER_COUNT = 3;
// 최대 좀비 수
constexpr short MAX_ZOMBIE_COUNT = 1000;

// 맵의 크기 정의
constexpr int W_WIDTH = 500;
constexpr int W_HEIGHT = 500;

using SIZEID  = uint32_t;
using SIZE1   = uint8_t;
using SIZE2   = uint16_t;
using SIZE3   = uint32_t;


// 플레이어의 체력
const SIZE2 PLAYER_HP = 500;


// --------------------------
// 패킷 ID 정의
// --------------------------

// 공통 헤더
enum PKT_TYPE : SIZE1 {

    C_S_LOGIN = 1,
    C_S_UPDATE ,
    C_S_SHOOT ,
	C_S_HIT,

    //S_C_LOGIN_OK = 14,
    //S_C_LOGIN_FAIL = 15,
    //S_C_PLAYER_INFO = 16,
    S_C_HIT_RESULT = 10,

    // 오브젝트 패킷 공통 처리용
    S_C_OBJECT_ADD = 30,
    S_C_OBJECT_UPDATE ,
    S_C_OBJECT_REMOVE ,

    S_C_STAGE_INFO = 40,
    S_C_SCORE_INFO,
    // ...
};

// --------------------------
// 패킷 구조체 정의
// --------------------------
#pragma pack (push, 1) 

struct Vector3 {
    float x, y, z;

    constexpr Vector3(float _x = 0.f, float _y = 0.f, float _z = 0.f)
        : x(_x), y(_y), z(_z) {}

    // 스칼라 곱
    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    // 누적 덧셈
    Vector3& operator+=(const Vector3& rhs) {
        x += rhs.x; 
        y += rhs.y; 
        z += rhs.z;
        return *this;
    }
    // 벡터 덧셈
    Vector3 operator+(const Vector3& rhs) const {
        return Vector3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    // 벡터 정규화
    Vector3 Normalize() const {
        float len = sqrtf(x * x + y * y + z * z);
        return (len > 0.f) ? Vector3(x / len, y / len, z / len) : Vector3();
    }

    // 벡터 길이
    float Length() const {
        return sqrtf(x * x + y * y + z * z);
    }
};


// 시작 위치
constexpr Vector3 START_POSITIONS[3] = {
    { 100.0f, 0.0f, 100.0f },
    { 110.0f, 0.0f, 100.0f },
    { 120.0f, 0.0f, 100.0f },
};

// 게임 내 객체 타입 정의
enum ObjectType : SIZE1 {
    PLAYER  = 1,
    ZOMBIE  = 2,
	BULLET  = 3,
	BOSS    = 4,

};

// 공격 종류 정의
enum ActionType : SIZE1 {
	NONE    = 0,   // 배회
	ZMOVE   = 1,   // 좀비 이동
    ATTACK  = 2,   // 근접
    RANGED  = 3,   // 원거리
    POISON  = 4,   // 독

};

enum SkinType : SIZE1 {
    PLAYER_NORMAL = 0,
    PLAYER_POLICE ,
    PLAYER_SOLDIER ,

    // 좀비 스킨 타입
    ZOMBIE_NORMAL = 10,
    ZOMBIE_RUNNER ,
    ZOMBIE_WITCH ,
    ZOMBIE_BOSS,
};


// 총 종류 
enum GunType : SIZE1 {
    BULLET_PISTOL = 0,
    BULLET_RIFLE,
    BULLET_SHOTGUN,
    BULLET_MAX
};
// 총 정보
struct BulletInfo {
    float speed;         // 총알 속도 (m/s 또는 게임 단위)
    float radius;        // 충돌 판정 반지름 (단위: meter)
    SIZE1 damage;        // 명중 시 데미지
    SIZE1 count;         // 발사 시 총알 개수 (샷건은 5~7)
};
constexpr BulletInfo BULLET_TABLE[] = {
	 { 350.f, 0.15f, 25, 1 }, // BULLET_PISTOL
	 { 850.f, 0.10f, 35, 1 }, // BULLET_RIFLE
	 { 400.f, 0.20f, 12, 6 }  // BULLET_SHOTGUN
};

struct Objectfixdata {          // 고정정보
    ObjectType obj_type;
    SIZE1 skin_type;
    char name[MAX_NAME_SIZE];
    Vector3 startposition;      // 초기 위치
    SIZE2 starthp;              // 체력
};

struct ObjectMeta {             // 필수정보
    Vector3 position;           // 위치
    Vector3 direction;          // 방향
    float speed;                // 이동 속도 (단위: m/s 또는 유닛/s)
    SIZE2 hp;                   // 체력
};

struct ObjectDynamicInfo {  	// 동적정보
    ObjectMeta meta;            // 필수정보
	GunType gun_type;           // 총 종류
    SIZE1 level;                // 레벨
    SIZE2 score;                // 점수
    SIZE2 damage;               // 공격력
    SIZE1 act_type;             // NONE, Player, ZMOVE, ATTACK, ...
};

//// 플레이어 정보 구조체
//struct PlayerInfo {
//    uint8_t id;
//    float position[3];  // X, Y, Z
//    char name[MAX_NAME_SIZE];
//    uint8_t skin_type;  // 플레이어 객체 스킨
//	uint8_t level;      // 레벨
//    uint8_t hp;         // 플레이어 체력
//    uint16_t score;     // 점수
//    // ... 
//};
//// 좀비 정보 구조체
//struct ZombieInfo {
//    uint32_t id;
//    ObjectType obj_type;    // 좀비 타입
//    ActionType act_type;    // 플레이어 객체 타입
//	uint8_t damage;         // 좀비 공격력
//    float position[3];      // X, Y, Z
//    uint8_t hp;             // 좀비 체력
//    // ... 
//    //ZombieInfo()
//    //    : zombieId(0), obj_type(ObjectType::ZOMBIE), act_type(ActionType::NONE), damage(30), hp(500) {}
//};

struct PacketHeader {
    SIZE2 size;
    PKT_TYPE type;
};

// --------------------------
// 클라 -> 서버
// --------------------------
// 로그인 패킷
struct pkt_cs_login {
    PacketHeader header;
    SIZE1 skin_type;
    char name[MAX_NAME_SIZE];

    pkt_cs_login() { header.type = PKT_TYPE::C_S_LOGIN; }
};

// 플레이어 업데이트 패킷
struct pkt_cs_update {
    PacketHeader header;
    ObjectDynamicInfo obj;          // 플레이어 정보
    pkt_cs_update() { header.type = PKT_TYPE::C_S_UPDATE; }
};

// 총알 발사 패킷
struct pkt_cs_shoot {
    PacketHeader header;
	SIZEID id;                      // 누가 쐈는지
    SIZE1 GunType;                  // 총 종류
    //int hitZombieId;
    float bulletPos[3];
    float bulletDir[3];
    pkt_cs_shoot() { header.type = PKT_TYPE::C_S_SHOOT; }
};

// 총알 명중 패킷
struct pkt_cs_hit {
	PacketHeader header;
    SIZEID shooterId;               // 누가 쐈는지
    SIZEID zombieId;                // 맞은 좀비 ID
	pkt_cs_hit() { header.type = PKT_TYPE::C_S_SHOOT; }
};


// --------------------------
// 서버 ->  클라
// --------------------------
// 총알 명중 결과
struct pkt_sc_hit_result {
    PacketHeader header;
    SIZEID shooterId;               // 누가 쐈는지
    SIZEID zombieId;
    SIZE2 zombieHp;
    //uint8_t damage;               // 얼마나 깎였는지

    pkt_sc_hit_result() { header.type = PKT_TYPE::S_C_HIT_RESULT; }
};
struct ZombieHit {
    SIZEID zombieId;
    SIZE2 hp;
    SIZE2 damage;
};
struct pkt_sc_hit_multi_result {
    PacketHeader header;
    uint8_t hitCount;
    ZombieHit hits[10];             // 최대 10마리 좀비 피격 처리
};


// --- Object 관리 패킷 ---
struct pkt_sc_object_add {
	PacketHeader header;
	SIZEID id; // ID
	Objectfixdata fixdata;          // 고정정보
    pkt_sc_object_add() { header.type = PKT_TYPE::S_C_OBJECT_ADD; }
};

// 객체 업데이트
struct pkt_sc_object_update {
    PacketHeader header;
	SIZEID id; // ID
    ObjectDynamicInfo obj;          // 동적정보
    pkt_sc_object_update() { header.type = PKT_TYPE::S_C_OBJECT_UPDATE; }
};
// 객체 삭제
struct pkt_sc_object_remove {
    PacketHeader header;
    SIZEID id;
    // ObjectType obj_type;
    pkt_sc_object_remove() { header.type = PKT_TYPE::S_C_OBJECT_REMOVE; }
};

// --- 게임 상황 패킷 ---
// STAGE 정보
struct pkt_sc_stage_info {
    PacketHeader header;
    SIZE2 currentStage;
    SIZE2 totalStages;
    SIZE3 timeLeft;
    pkt_sc_stage_info() { header.type = PKT_TYPE::S_C_STAGE_INFO; }
};
// SCORE 정보
struct pkt_sc_score_info {
    PacketHeader header;
    SIZE2 stage_score;
    pkt_sc_score_info() { header.type = PKT_TYPE::S_C_SCORE_INFO; }
};

#pragma pack (pop)

