#pragma once

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

BulletInfo GetBulletInfo(BulletType type)
{
    switch (type)
    {
    case BULLET_PISTOL:
        return { 350.f, 0.15f, 25, 1 };
    case BULLET_RIFLE:
        return { 850.f, 0.10f, 35, 1 };
    case BULLET_SHOTGUN:
        return { 400.f, 0.20f, 12, 6 };
    default:
        return { 350.f, 0.15f, 25, 1 }; // 기본값
    }
}


// --------------------------
// 패킷 ID 정의
// --------------------------

// 클라이언트 -> 서버 패킷
namespace PKT_CS {
    enum PKT_CS : uint8_t {
        LOGIN   = 1,
        MOVE    = 2,
		SHOOT   = 3,

		// ... 필요하면 추가
    };
}

// 서버 -> 클라이언트 패킷
namespace PKT_SC {
    enum : uint8_t {
        LOGIN_OK            = 1,
        LOGIN_FAIL          = 2,
        BROADCAST_STATE     = 3,
        HIT_RESULT          = 4,
        OBJECT_ADD          = 5,
        OBJECT_UPDATE       = 6,
        OBJECT_REMOVE       = 7,
        STAGE_INFO          = 8,
        SCORE_INFO          = 9,

        // ... 필요하면 추가
    };
}

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
    uint32_t playerId;
    float position[3];  // X, Y, Z
    uint8_t name[NAME_SIZE];
    ObjectType obj_type;  // 플레이어 객체 타입
	uint8_t level;      // 레벨
    uint8_t hp;         // 플레이어 체력
    uint16_t score;     // 점수
    // ... 
};

// 좀비 정보 구조체
struct ZombieInfo {
    uint32_t zombieId;
    ObjectType obj_type;    // 좀비 타입
    ActionType act_type;    // 플레이어 객체 타입
	uint8_t damage;         // 좀비 공격력
    float position[3];      // X, Y, Z
    uint8_t hp;             // 좀비 체력
    // ... 
    //ZombieInfo()
    //    : zombieId(0), obj_type(ObjectType::ZOMBIE), act_type(ActionType::NONE), damage(30), hp(500) {}
};

// 공통 헤더
enum PacketType : uint8_t {
    LOGIN = 1,
    MOVE = 2,
    SHOOT = 3,
    LOGIN_OK = 100,
    LOGIN_FAIL = 101,
    BROADCAST_STATE = 102,
    HIT_RESULT = 103,
    OBJECT_ADD = 104,
    OBJECT_UPDATE = 105,
    OBJECT_REMOVE = 106,
    STAGE_INFO = 107,
    SCORE_INFO = 108,
    // ...
};

struct PacketHeader {
    uint16_t size;
    PacketType type;
};

// --------------------------
// 클라 -> 서버
// --------------------------
// 로그인 패킷
struct PKT_CS_LOGIN {
    PacketHeader header;
    uint8_t name[NAME_SIZE];
    uint8_t obj_type;
    PKT_CS_LOGIN() { header.type = PacketType::LOGIN; }
};

// 이동 패킷
struct PKT_CS_MOVE {
    PacketHeader header;
    float position[3];
    PKT_CS_MOVE() { header.type = PacketType::MOVE; }
};

// 총알 발사 패킷
struct PKT_CS_SHOOT {
    PacketHeader header;
    uint8_t GunType; // 총 종류
    //int hitZombieId;
    float bulletPos[3];
    float bulletDir[3];
    PKT_CS_SHOOT() { header.type = PacketType::SHOOT; }
};

// --------------------------
// 서버 ->  클라
// --------------------------
// 로그인 OK 패킷
struct PKT_SC_LOGIN_OK {
    PacketHeader header;
    uint32_t playerId;
    uint8_t name[NAME_SIZE];
	uint8_t obj_type;
    PKT_SC_LOGIN_OK() { header.type = PacketType::LOGIN_OK; }
};

// 상태 전체 브로드캐스트 패킷
struct PKT_SC_BROADCAST_STATE {
    PacketHeader header;

    uint32_t playerCount;    
    PlayerInfo players[MAX_PLAYER_COUNT];  

    uint32_t zombieCount;    
    ZombieInfo zombies[MAX_ZOMBIE_COUNT]; 


    PKT_SC_BROADCAST_STATE() { header.type = PacketType::BROADCAST_STATE; }
};

// 총알 명중 결과
struct PKT_SC_HIT_RESULT {
    PacketHeader header;
    uint32_t shooterId;   // 누가 쐈는지
    uint32_t zombieId;
    uint8_t zombieHp;
    //uint8_t damage;       // 얼마나 깎였는지

    PKT_SC_HIT_RESULT() { header.type = PacketType::HIT_RESULT; }
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
    float position[3];      // 플레이어의 위치
    uint8_t hp;             // 플레이어 체력
    uint16_t score;         // 플레이어 점수
    uint8_t level;          // 플레이어 레벨
    PKT_SC_PLAYER_ADD() { header.type = PacketType::OBJECT_ADD; }
};
//객체 업데이트 패킷 (플레이어)
struct PKT_SC_PLAYER_UPDATE {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint16_t score;         
    uint8_t level;          
    PKT_SC_PLAYER_UPDATE() { header.type = PacketType::OBJECT_UPDATE; }
};
// 객체 삭제 패킷 (플레이어)
struct PKT_SC_PLAYER_REMOVE {
    PacketHeader header;
    uint32_t objectId;     
    PKT_SC_PLAYER_REMOVE() { header.type = PacketType::OBJECT_REMOVE; }
};

// 객체 추가 패킷 (좀비 , 보스)
struct PKT_SC_ZOMBIE_ADD {
    PacketHeader header;
    uint32_t objectId;      // 좀비의 고유 ID
    float position[3];      // 좀비의 위치
    uint8_t hp;             // 좀비 체력
    uint8_t action_type;    // 좀비의 행동 유형
    uint8_t damage;         // 좀비의 공격력
    PKT_SC_ZOMBIE_ADD() { header.type = PacketType::OBJECT_ADD; }
};
// 객체 업데이트 패킷(좀비 , 보스)
struct PKT_SC_ZOMBIE_UPDATE {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint8_t action_type;   
    uint8_t damage;         
    PKT_SC_ZOMBIE_UPDATE() { header.type = PacketType::OBJECT_UPDATE; }
};
// 객체 삭제 패킷(좀비, 보스)
struct PKT_SC_ZOMBIE_REMOVE {
    PacketHeader header;
    uint32_t objectId;      
    PKT_SC_ZOMBIE_REMOVE() { header.type = PacketType::OBJECT_REMOVE; }
};

// --- 게임 상황 패킷 ---
// STAGE 정보
struct PKT_SC_STAGE_INFO {
    PacketHeader header;
    uint16_t currentStage;  
    uint16_t totalStages;
    uint32_t timeLeft;
    PKT_SC_STAGE_INFO() { header.type = PacketType::STAGE_INFO; }
};
// SCORE 정보
struct PKT_SC_SCORE_INFO {
    PacketHeader header;
    uint16_t stage_score;
    PKT_SC_SCORE_INFO() { header.type = PacketType::SCORE_INFO; }
};


#pragma pack (pop)

