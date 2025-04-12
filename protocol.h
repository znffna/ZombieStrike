#pragma once
#include <cstdint>

// --------------------------
// 서버/클라 공통 상수 정의
// --------------------------
constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;

// 서버의 최대 세션 수
constexpr int MAX_USER = 5000; 
// 최대 플레이어 수
constexpr short MAX_PLAYER_COUNT = 3;
// 최대 좀비 수
constexpr short MAX_ZOMBIE_COUNT = 1000;

// 맵의 크기 정의
constexpr int W_WIDTH = 500;
constexpr int W_HEIGHT = 500;

// 시작 위치
const float START_POSITIONS[3][3] = {
    { 100.0f, 0.0f, 100.0f },
    { 110.0f, 0.0f, 100.0f },
    { 120.0f, 0.0f, 100.0f },
};
// 플레이어의 체력
const uint8_t PLAYER_HP = 500;

// 총 정보
struct BulletInfo {
    float speed;        // 총알 속도 (m/s 또는 게임 단위)
    float radius;       // 충돌 판정 반지름 (단위: meter)
    uint8_t damage;     // 명중 시 데미지
    uint8_t count;      // 발사 시 총알 개수 (샷건은 5~7)
};
// 총 종류 
enum BulletType : uint8_t {
    BULLET_PISTOL = 0,
    BULLET_RIFLE,
    BULLET_SHOTGUN,
    BULLET_MAX
};
static const BulletInfo BULLET_TABLE[BULLET_MAX] = {
    { 350.f, 0.15f, 25, 1 },   // BULLET_PISTOL
    { 850.f, 0.10f, 35, 1 },   // BULLET_RIFLE
    { 400.f, 0.20f, 12, 6 }    // BULLET_SHOTGUN
};
//BulletInfo GetBulletInfo(BulletType type)
//{
//    switch (type)
//    {
//    case BULLET_PISTOL:
//        return { 350.f, 0.15f, 25, 1 };
//    case BULLET_RIFLE:
//        return { 850.f, 0.10f, 35, 1 };
//    case BULLET_SHOTGUN:
//        return { 400.f, 0.20f, 12, 6 };
//    default:
//        return { 350.f, 0.15f, 25, 1 }; // 기본값
//    }
//}


// --------------------------
// 패킷 ID 정의
// --------------------------

// 공통 헤더
enum PKT_TYPE : uint8_t {

    C_S_LOGIN = 1,
    C_S_MOVE = 2,
    C_S_SHOOT = 3,

    S_C_LOGIN_OK = 104,
    S_C_LOGIN_FAIL = 105,
    S_C_PLAYER_INFO = 106,

    S_C_HIT_RESULT = 201,
    S_C_PLAYER_ADD = 202,
    S_C_PLAYER_UPDATE = 203,
    S_C_PLAYER_REMOVE = 204,
    S_C_ZOMBIE_ADD = 205,
    S_C_ZOMBIE_UPDATE = 206,
    S_C_ZOMBIE_REMOVE = 207,

    S_C_STAGE_INFO = 301,
    S_C_SCORE_INFO = 302,

    // ...
};

// --------------------------
// 패킷 구조체 정의
// --------------------------

#pragma pack (push, 1) 

// 게임 내 객체 타입 정의
enum ObjectType : uint8_t {
    PLAYER  = 1,
    ZOMBIE  = 2,
	BULLET  = 3,
	BOSS    = 4,

};

// 공격 종류 정의
enum ActionType : uint8_t {
	NONE    = 0,   // 배회
	ZMOVE   = 1,   // 좀비 이동
    ATTACK  = 2,   // 근접
    RANGED  = 3,   // 원거리
    POISON  = 4,   // 독

};

// 플레이어 정보 구조체
struct PlayerInfo {
    uint32_t id;
    float position[3];  // X, Y, Z
    char name[NAME_SIZE];
    uint8_t skin_type;  // 플레이어 객체 스킨
	uint8_t level;      // 레벨
    uint8_t hp;         // 플레이어 체력
    uint16_t score;     // 점수
    // ... 
};

// 좀비 정보 구조체
struct ZombieInfo {
    uint32_t id;
    ObjectType obj_type;    // 좀비 타입
    ActionType act_type;    // 플레이어 객체 타입
	uint8_t damage;         // 좀비 공격력
    float position[3];      // X, Y, Z
    uint8_t hp;             // 좀비 체력
    // ... 
    //ZombieInfo()
    //    : zombieId(0), obj_type(ObjectType::ZOMBIE), act_type(ActionType::NONE), damage(30), hp(500) {}
};


struct PacketHeader {
    uint16_t size;
    PKT_TYPE type;
};

// --------------------------
// 클라 -> 서버
// --------------------------
// 로그인 패킷
struct PKT_CS_LOGIN {
    PacketHeader header;
    char name[NAME_SIZE];
    uint8_t skin_type;
    PKT_CS_LOGIN() { header.type = PKT_TYPE::C_S_LOGIN; }
};

// 이동 패킷
struct PKT_CS_MOVE {
    PacketHeader header;
    float position[3];
    PKT_CS_MOVE() { header.type = PKT_TYPE::C_S_MOVE; }
};

// 총알 발사 패킷
struct PKT_CS_SHOOT {
    PacketHeader header;
    uint8_t GunType; // 총 종류
    //int hitZombieId;
    float bulletPos[3];
    float bulletDir[3];
    PKT_CS_SHOOT() { header.type = PKT_TYPE::C_S_SHOOT; }
};




// --------------------------
// 서버 ->  클라
// --------------------------
// 로그인 OK 패킷
struct PKT_SC_LOGIN_OK {
    PacketHeader header;
    uint32_t playerId;
    char name[NAME_SIZE];
	uint8_t skin_type;
    PKT_SC_LOGIN_OK() { header.type = PKT_TYPE::S_C_LOGIN_OK; }
};

//// 상태 전체 브로드캐스트 패킷
//struct PKT_SC_BROADCAST_STATE {
//    PacketHeader header;
//
//    uint32_t playerCount;    
//    PlayerInfo players[MAX_PLAYER_COUNT];  
//
//    uint32_t zombieCount;    
//    ZombieInfo zombies[MAX_ZOMBIE_COUNT]; 
//
//
//    PKT_SC_BROADCAST_STATE() { header.type = PacketType::BROADCAST_STATE; }
//};

// 총알 명중 결과
struct PKT_SC_HIT_RESULT {
    PacketHeader header;
    uint32_t shooterId;   // 누가 쐈는지
    uint32_t zombieId;
    uint8_t zombieHp;
    //uint8_t damage;       // 얼마나 깎였는지

    PKT_SC_HIT_RESULT() { header.type = PKT_TYPE::S_C_HIT_RESULT; }
};
struct ZombieHit {
    uint32_t zombieId;
    uint8_t hp;
    uint8_t damage;
};

struct PKT_SC_HIT_RESULT_MULTI {
    PacketHeader header;
    uint8_t hitCount;
    ZombieHit hits[10]; // 최대 10마리 좀비 피격 처리
};


// --- Object 관리 패킷 ---
// 객체 추가 패킷 (플레이어)
struct PKT_SC_PLAYER_ADD {
    PacketHeader header;
    uint32_t objectId;      // 플레이어의 고유 ID
    uint8_t skin_type;      // 플레이어 스킨 타입
    float position[3];      // 플레이어의 위치
    uint8_t level;          // 플레이어 레벨
    uint8_t hp;             // 플레이어 체력
    uint16_t score;         // 플레이어 점수
    PKT_SC_PLAYER_ADD() { header.type = PKT_TYPE::S_C_PLAYER_ADD; }
};
//객체 업데이트 패킷 (플레이어)
struct PKT_SC_PLAYER_UPDATE {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint16_t score;         
    uint8_t level;          
    PKT_SC_PLAYER_UPDATE() { header.type = PKT_TYPE::S_C_PLAYER_UPDATE; }
};
// 객체 삭제 패킷 (플레이어)
struct PKT_SC_PLAYER_REMOVE {
    PacketHeader header;
    uint32_t objectId;     
    PKT_SC_PLAYER_REMOVE() { header.type = PKT_TYPE::S_C_PLAYER_REMOVE; }
};

// 객체 추가 패킷 (좀비 , 보스)
struct PKT_SC_ZOMBIE_ADD {
    PacketHeader header;
    uint32_t objectId;      // 좀비의 고유 ID
    float position[3];      // 좀비의 위치
    uint8_t hp;             // 좀비 체력
    uint8_t skin_type;      // 좀비 스킨 
    uint8_t action_type;    // 좀비의 행동 유형
    uint8_t damage;         // 좀비의 공격력
    PKT_SC_ZOMBIE_ADD() { header.type = PKT_TYPE::S_C_ZOMBIE_ADD; }
};
// 객체 업데이트 패킷(좀비 , 보스)
struct PKT_SC_ZOMBIE_UPDATE {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint8_t action_type;   
    uint8_t damage;         
    PKT_SC_ZOMBIE_UPDATE() { header.type = PKT_TYPE::S_C_ZOMBIE_UPDATE; }
};
// 객체 삭제 패킷(좀비, 보스)
struct PKT_SC_ZOMBIE_REMOVE {
    PacketHeader header;
    uint32_t objectId;      
    PKT_SC_ZOMBIE_REMOVE() { header.type = PKT_TYPE::S_C_ZOMBIE_REMOVE; }
};





// --- 게임 상황 패킷 ---
// STAGE 정보
struct PKT_SC_STAGE_INFO {
    PacketHeader header;
    uint16_t currentStage;  
    uint16_t totalStages;
    uint32_t timeLeft;
    PKT_SC_STAGE_INFO() { header.type = PKT_TYPE::S_C_STAGE_INFO; }
};
// SCORE 정보
struct PKT_SC_SCORE_INFO {
    PacketHeader header;
    uint16_t stage_score;
    PKT_SC_SCORE_INFO() { header.type = PKT_TYPE::S_C_SCORE_INFO; }
};


#pragma pack (pop)

