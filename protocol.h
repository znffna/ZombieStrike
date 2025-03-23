#pragma once
#include <cstdint>

// --------------------------
// ����/Ŭ�� ���� ��� ����
// --------------------------
constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;

// ������ �ִ� ���� ��
constexpr int MAX_USER = 5000; 

// ���� ũ�� ����
constexpr int W_WIDTH = 400;
constexpr int W_HEIGHT = 400;

// --------------------------
// ��Ŷ ID ����
// --------------------------

// Ŭ���̾�Ʈ -> ���� ��Ŷ
namespace PKT_CS {
    enum PKT_CS : uint8_t {
        LOGIN = 1,
        MOVE = 2,
		SHOOT = 3,

		// ... �ʿ��ϸ� �߰�
    };
}

// ���� -> Ŭ���̾�Ʈ ��Ŷ
namespace PKT_SC {
    enum : uint8_t {
        LOGIN_OK = 1,
        LOGIN_FAIL = 2,
        BROADCAST_STATE = 3,
		HIT_RESULT = 4,
            
        // ... �ʿ��ϸ� �߰�
    };
}

// --------------------------
// ��Ŷ ����ü ����
// --------------------------

#pragma pack (push, 1) // �е� X

// Ŭ�� -> ���� �α��� ��Ŷ
struct PKT_CS_LOGIN {
    uint8_t id = PKT_CS::LOGIN;
    char name[NAME_SIZE];
};

// Ŭ�� -> ���� �̵� ��Ŷ
struct PKT_CS_MOVE {
    uint8_t id = PKT_CS::MOVE;
    float x, y, z;
};

// Ŭ�� -> ���� �Ѿ� �߻� ��Ŷ
struct PKT_CS_SHOOT {
    uint8_t id = PKT_CS::SHOOT;
    int hitZombieId;
    float bulletPosX, bulletPosY, bulletPosZ;
    float bulletDirX, bulletDirY, bulletDirZ;
};

// ���� -> Ŭ�� �α��� OK ��Ŷ
struct PKT_SC_LOGIN_OK {
    uint8_t id = PKT_SC::LOGIN_OK;
    int playerId;
};

// ���� -> Ŭ�� ���� ��ü ��ε�ĳ��Ʈ ��Ŷ
struct PKT_SC_BROADCAST_STATE {
    uint8_t id = PKT_SC::BROADCAST_STATE;
    int playerCount;
    // �÷��̾� ������ �ʿ信 ���� �߰�

    int zombieCount;
    // ���� ���� �ʿ信 ���� �߰�
};

// ���� -> Ŭ�� �Ѿ� ���� ���
struct PKT_SC_HIT_RESULT {
    uint8_t id = PKT_SC::HIT_RESULT;
    int zombieId;
    int zombieHp;
};

#pragma pack (pop)

