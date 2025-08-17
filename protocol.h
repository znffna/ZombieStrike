#pragma once
#include <cstdint>

// --------------------------
// 서버/클라 공통 상수 정의
// --------------------------
using SIZEID = uint32_t;
using SIZE1 = uint8_t;
using SIZE2 = uint16_t;
using SIZE3 = uint32_t;

constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int MAX_NAME_SIZE = 20;

constexpr int MAX_USER = 5000;          // 서버의 최대 세션 수
constexpr short MAX_PLAYER_COUNT = 3;   // 최대 플레이어 수

constexpr short MAX_ZOMBIE_COUNT = 30;  // 최대 좀비 수

constexpr int W_WIDTH = 250;            // 맵의 크기 정의
constexpr int W_HEIGHT = 250;

constexpr SIZE2 PLAYER_HP = 500;            // 플레이어 체력  
constexpr SIZE2 GUN_DAMAGE = 300;           // 총기 데미지 - 일단 2방에 죽나 tset용  
constexpr SIZE2 ZOMBIE_HP = 500;            // 좀비 초기 체력
constexpr float ZOMBIE_DAMAGE = 10;         // 좀비 초기 공격력 
constexpr float Z_move_speed = 0.03f;       // 좀비 스피드

enum ObjectType : SIZE1 {
    PLAYER = 1,
    ZOMBIE = 2,
    BULLET = 3,
    BOSS = 4,

};

// 공격 종류 정의
enum ActionType : SIZE1 {
    NONE = 0,   // 배회
    ZMOVE = 1,  // 좀비 이동
    ATTACK = 2,  // 근접
    RANGED = 3,  // 원거리
    DEAD = 4,    // 독
    HIT = 5,    


};

enum SkinType : SIZE1 {
    PLAYER_NORMAL = 0,
    PLAYER_POLICE,
    PLAYER_SOLDIER,

    // 좀비 스킨 타입
    ZOMBIE_NORMAL = 10,
    ZOMBIE_RUNNER,
    ZOMBIE_WITCH,
    ZOMBIE_BOSS,
};


// 총 종류 
enum GunType : SIZE1 {
    BULLET_PISTOL = 0,
    BULLET_RIFLE,
    BULLET_SHOTGUN,
    BULLET_MAX
};

enum PKT_TYPE : SIZE1 {

    C_S_LOGIN = 1,
    C_S_UPDATE,

    C_S_SHOOT,
    C_S_HIT,
    S_C_SHOOT,

    //S_C_LOGIN_OK = 14,
    //S_C_LOGIN_FAIL = 15,
    S_C_OBJ_INFO,
    //S_C_HIT_RESULT,

    // 오브젝트 패킷 공통 처리용
    S_C_OBJECT_ADD,
    S_C_OBJECT_UPDATE,
    S_C_HIT_RESULT,
    S_C_OBJECT_REMOVE,

    C_S_STAGE_INFO,
    S_C_STAGE_INFO,

    C_S_SCORE_INFO,
    S_C_SCORE_INFO,
    // ...
};


inline const char* ToString(ActionType action) {
    switch (action) {
    case NONE:   return "NONE";
    case ZMOVE:  return "ZMOVE";
    case ATTACK: return "ATTACK";
    case RANGED: return "RANGED";
    case DEAD:   return "DEAD";
    case HIT:    return "HIT";
    default:     return "UNKNOWN";
    }
}

inline const char* ToString(ObjectType type) {
    switch (type) {
    case PLAYER: return "PLAYER";
    case ZOMBIE: return "ZOMBIE";
    case BULLET: return "BULLET";
    case BOSS:   return "BOSS";
    default:     return "UNKNOWN";
    }
}
inline const char* ToString(GunType gun) {
    switch (gun) {
    case BULLET_PISTOL:  return "PISTOL";
    case BULLET_RIFLE:   return "RIFLE";
    case BULLET_SHOTGUN: return "SHOTGUN";
    default:             return "UNKNOWN";
    }
}
inline const char* ToString(PKT_TYPE type) {
    switch (type) {
    case C_S_LOGIN:         return "C_S_LOGIN";
    case C_S_UPDATE:        return "C_S_UPDATE";
    case C_S_SHOOT:         return "C_S_SHOOT";
    case C_S_HIT:           return "C_S_HIT";
    case S_C_SHOOT:         return "S_C_SHOOT";
    case S_C_OBJ_INFO:      return "S_C_OBJ_INFO";
    case S_C_OBJECT_ADD:    return "S_C_OBJECT_ADD";
    case S_C_OBJECT_UPDATE: return "S_C_OBJECT_UPDATE";
    case S_C_HIT_RESULT:    return "S_C_HIT_RESULT";
    case S_C_OBJECT_REMOVE: return "S_C_OBJECT_REMOVE";
	case C_S_STAGE_INFO:    return "C_S_STAGE_INFO";
    case S_C_STAGE_INFO:    return "S_C_STAGE_INFO";
    case C_S_SCORE_INFO:    return "C_S_SCORE_INFO";
    case S_C_SCORE_INFO:    return "S_C_SCORE_INFO";
    default:                return "UNKNOWN_PACKET";
    }
}


// --------------------------
// 패킷 구조체 정의
// --------------------------
#pragma pack (push, 1) 

struct Vec3 {
    float x, y, z;

    constexpr Vec3(float _x = 0.f, float _y = 0.f, float _z = 0.f)
        : x(_x), y(_y), z(_z) {}

    // 스칼라 곱
    Vec3 operator*(float scalar) const {
        return Vec3(x * scalar, y * scalar, z * scalar);
    }
    // 누적 덧셈
    Vec3& operator+=(const Vec3& rhs) {
        x += rhs.x; 
        y += rhs.y; 
        z += rhs.z;
        return *this;
    }
    // 벡터 덧셈
    Vec3 operator+(const Vec3& rhs) const {
        return Vec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    // 벡터 뺼셈
    Vec3 operator-(const Vec3& rhs) const {
        return Vec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    // 벡터 정규화
    Vec3 Normalize() const {
        float len = sqrtf(x * x + y * y + z * z);
        return (len > 0.f) ? Vec3(x / len, y / len, z / len) : Vec3();
    }

    // 벡터 길이
    float Length() const {
        return sqrtf(x * x + y * y + z * z);
    }

    float LengthSquared() const {
        return x * x + y * y + z * z;
    }
    // 내적
    float Dot(const Vec3& rhs) const {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

};

constexpr Vec3 START_POSITIONS[3] = {
    { 100.0f, 0.0f, 100.0f },
    { 110.0f, 0.0f, 100.0f },
    { 120.0f, 0.0f, 100.0f },
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
    Vec3 startposition;         // 초기 위치
    SIZE2 starthp;              // 체력
    GunType gun_type;           // 총 종류
};


struct Object {  	
    Vec3 position;              // 위치
    Vec3 velocity;              // 방향 * 속도
    Vec3 look;                  // 보는방향
    float pitch;                // 피치
    SIZE2 hp;                   // 체력

	GunType gun_type;           // 총 종류
    SIZE1 level;                // 레벨
    SIZE2 score;                // 점수
    SIZE2 damage;               // 공격력
    SIZE1 act_type;             // NONE, Player, ZMOVE, ATTACK, ...
    SIZE1 move_input;           // 이동 입력
};


struct PacketHeader {
    SIZE2 size;
    PKT_TYPE type;
};

// --------------------------
// 클라 -> 서버
// --------------------------
// 로그인 패킷
struct pkt_cs_login {
    PacketHeader header{sizeof(*this), PKT_TYPE::C_S_LOGIN };
    SIZE1 skin_type;
    char name[MAX_NAME_SIZE];
};

// 플레이어 업데이트 패킷
struct pkt_cs_update {
    PacketHeader header{ sizeof(*this), PKT_TYPE::C_S_UPDATE };
    Vec3 position;              // 위치
    Vec3 velocity;              // 방향 * 속도
    Vec3 look;                  // 보는방향
    float pitch;                // 피치
    SIZE2 hp;                   // 체력

    GunType gun_type;           // 총 종류
    SIZE1 level;                // 레벨
    SIZE2 score;                // 점수
    SIZE2 damage;               // 공격력
    SIZE1 act_type;             // NONE, Player, ZMOVE, ATTACK, ...
	SIZE1 move_input;           // 이동 입력
};

// 총알 발사 패킷
struct pkt_cs_shoot {
    PacketHeader header{sizeof(*this), PKT_TYPE::C_S_SHOOT };
	//SIZEID id;                      // 누가 쐈는지
 //   SIZE1 GunType;                  // 총 종류
 //   //int hitZombieId;
    float bulletPos[3];
    float bulletDir[3];
};

//// 총알 명중 패킷
//struct pkt_cs_hit {
//    PacketHeader header{sizeof(*this),PKT_TYPE::pkt_cs_hit };
//    SIZEID shooterId;               // 누가 쐈는지
//    SIZEID zombieId;                // 맞은 좀비 ID
//};


// --------------------------
// 서버 ->  클라
// --------------------------
// 총알 명중 결과
struct pkt_sc_hit_result {
    PacketHeader header{sizeof(*this), PKT_TYPE::S_C_HIT_RESULT };
    SIZEID shooterId;               // 누가 쐈는지
    SIZEID zombieId;
    SIZE2 zombieHp;
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

struct pkt_sc_shoot {
    PacketHeader header{ sizeof(*this), PKT_TYPE::S_C_SHOOT }; 
    SIZEID shooterId;      // 누가 쐈는지
    GunType gun_type;      // 총 종류(이펙트/사운드)
    float bulletPos[3];    // 발사 원점
    float bulletDir[3];    // 정규화 방향
};

// 로그인 결과 패킷
struct pkt_sc_obj_info {
    PacketHeader header{ sizeof(*this), PKT_TYPE::S_C_OBJ_INFO };
    SIZEID id;                  // ID
    ObjectType obj_type;        // 객체 타입
    SIZE1 skin_type;            // 스킨 타입
    char name[MAX_NAME_SIZE];   // 이름
    Vec3 startposition;         // 초기 위치
	Vec3 velocity;              // 방향 * 속도
	Vec3 look;                  // 보는방향
	float pitch;                // 피치
    SIZE2 starthp;              // 체력
    SIZE1 level;                // 레벨
    SIZE2 score;                // 점수
    SIZE2 damage;               // 공격력
    GunType gun_type;           // 총 종류
    SIZE1 act_type;             // NONE, Player, ZMOVE, ATTACK, ...
    SIZE1 move_input;           // 이동 입력
};

// --- Object 관리 패킷 ---
struct pkt_sc_object_add {
    PacketHeader header{sizeof(*this), PKT_TYPE::S_C_OBJECT_ADD };
	SIZEID id;                  // ID
	ObjectType obj_type;        // 객체 타입
	SIZE1 skin_type;            // 스킨 타입
	char name[MAX_NAME_SIZE];   // 이름
    Vec3 startposition;         // 초기 위치
    SIZE2 starthp;              // 체력
    GunType gun_type;           // 총 종류
	SIZE1 act_type;			    // NONE, Player, ZMOVE, ATTACK, ...
    SIZE1 move_input;           // 이동 입력
};

// 객체 업데이트
struct pkt_sc_object_update {
    PacketHeader header{sizeof(*this),PKT_TYPE::S_C_OBJECT_UPDATE };
	SIZEID id;                  // ID
    Vec3 position;              // 위치
    Vec3 velocity;              // 방향 * 속도
    Vec3 look;                  // 보는방향
    float pitch;                // 피치
    SIZE2 hp;                   // 체력

    GunType gun_type;           // 총 종류
    SIZE1 level;                // 레벨
    SIZE2 score;                // 점수
    SIZE2 damage;               // 공격력
    SIZE1 act_type;             // NONE, Player, ZMOVE, ATTACK, ...
    SIZE1 move_input;           // 이동 입력
};
// 객체 삭제
struct pkt_sc_object_remove {
    PacketHeader header{sizeof(*this),PKT_TYPE::S_C_OBJECT_REMOVE };
    SIZEID id;
};

// --- 게임 상황 패킷 ---
// STAGE 정보
struct pkt_cs_stage_info {
	PacketHeader header{ sizeof(*this),PKT_TYPE::C_S_STAGE_INFO };
	SIZE2 currentStage;
    SIZE3 timeLeft;
};

struct pkt_sc_stage_info {
    PacketHeader header{sizeof(*this),PKT_TYPE::S_C_STAGE_INFO };
    SIZE2 currentStage;
    SIZE2 totalStages;
    SIZE3 timeLeft;
};
// SCORE 정보
struct pkt_cs_score_info {
    PacketHeader header{ sizeof(*this),PKT_TYPE::C_S_SCORE_INFO };
    SIZE2 stage_score;
};
struct pkt_sc_score_info {
    PacketHeader header{sizeof(*this),PKT_TYPE::S_C_SCORE_INFO };
    SIZE2 stage_score;      // 스테이지 점수
    SIZE2 alive;            // 생존자 수
    SIZE2 total_spawned;;   // 스폰 좀비 수 
    SIZE2 total_killed;     // 죽인 좀비 수
};  

#pragma pack (pop)

