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

// 맵의 크기 정의
constexpr int W_WIDTH = 400;
constexpr int W_HEIGHT = 400;

// --------------------------
// 패킷 ID 정의
// --------------------------

// 클라이언트 -> 서버 패킷
namespace PKT_CS {
    enum PKT_CS : uint8_t {
        LOGIN = 1,
        MOVE = 2,
		SHOOT = 3,

		// ... 필요하면 추가
    };
}

// 서버 -> 클라이언트 패킷
namespace PKT_SC {
    enum : uint8_t {
        LOGIN_OK = 1,
        LOGIN_FAIL = 2,
        BROADCAST_STATE = 3,
		HIT_RESULT = 4,
            
        // ... 필요하면 추가
    };
}

// --------------------------
// 패킷 구조체 정의
// --------------------------

#pragma pack (push, 1) // 패딩 X

// 클라 -> 서버 로그인 패킷
struct PKT_CS_LOGIN {
    uint8_t id = PKT_CS::LOGIN;
    char name[NAME_SIZE];
};

// 클라 -> 서버 이동 패킷
struct PKT_CS_MOVE {
    uint8_t id = PKT_CS::MOVE;
    float x, y, z;
};

// 클라 -> 서버 총알 발사 패킷
struct PKT_CS_SHOOT {
    uint8_t id = PKT_CS::SHOOT;
    int hitZombieId;
    float bulletPosX, bulletPosY, bulletPosZ;
    float bulletDirX, bulletDirY, bulletDirZ;
};

// 서버 -> 클라 로그인 OK 패킷
struct PKT_SC_LOGIN_OK {
    uint8_t id = PKT_SC::LOGIN_OK;
    int playerId;
};

// 서버 -> 클라 상태 전체 브로드캐스트 패킷
struct PKT_SC_BROADCAST_STATE {
    uint8_t id = PKT_SC::BROADCAST_STATE;
    int playerCount;
    // 플레이어 정보는 필요에 따라 추가

    int zombieCount;
    // 좀비 정보 필요에 따라 추가
};

// 서버 -> 클라 총알 명중 결과
struct PKT_SC_HIT_RESULT {
    uint8_t id = PKT_SC::HIT_RESULT;
    int zombieId;
    int zombieHp;
};

#pragma pack (pop)

