#pragma once

// --------------------------
// ����/Ŭ�� ���� ��� ����
// --------------------------
constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int NAME_SIZE = 20;

// ������ �ִ� ���� ��
constexpr int MAX_USER = 5000; 
// �ִ� �÷��̾� ��
constexpr short MAX_PLAYER_COUNT = 3;
// �ִ� ���� ��
constexpr short MAX_ZOMBIE_COUNT = 1000;

// ���� ũ�� ����
constexpr int W_WIDTH = 500;
constexpr int W_HEIGHT = 500;

// �� ����
struct BulletInfo {
    float speed;        // �Ѿ� �ӵ� (m/s �Ǵ� ���� ����)
    float radius;       // �浹 ���� ������ (����: meter)
    uint8_t damage;     // ���� �� ������
    uint8_t count;      // �߻� �� �Ѿ� ���� (������ 5~7)
};
// �� ���� 
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
        return { 350.f, 0.15f, 25, 1 }; // �⺻��
    }
}


// --------------------------
// ��Ŷ ID ����
// --------------------------

// Ŭ���̾�Ʈ -> ���� ��Ŷ
namespace PKT_CS {
    enum PKT_CS : uint8_t {
        LOGIN   = 1,
        MOVE    = 2,
		SHOOT   = 3,

		// ... �ʿ��ϸ� �߰�
    };
}

// ���� -> Ŭ���̾�Ʈ ��Ŷ
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

        // ... �ʿ��ϸ� �߰�
    };
}

// --------------------------
// ��Ŷ ����ü ����
// --------------------------

#pragma pack (push, 1) 

// ���� �� ��ü Ÿ�� ����
enum ObjectType : uint8_t {
    PLAYER  = 1,
    ZOMBIE  = 2,
	BULLET  = 3,
	BOSS    = 4,

};

// ���� ���� ����
enum ActionType : uint8_t {
	NONE    = 0,   // ��ȸ
	ZMOVE   = 1,   // ���� �̵�
    ATTACK  = 2,   // ����
    RANGED  = 3,   // ���Ÿ�
    POISON  = 4,   // ��

};

// �÷��̾� ���� ����ü
struct PlayerInfo {
    uint32_t playerId;
    float position[3];  // X, Y, Z
    uint8_t name[NAME_SIZE];
    ObjectType obj_type;  // �÷��̾� ��ü Ÿ��
	uint8_t level;      // ����
    uint8_t hp;         // �÷��̾� ü��
    uint16_t score;     // ����
    // ... 
};

// ���� ���� ����ü
struct ZombieInfo {
    uint32_t zombieId;
    ObjectType obj_type;    // ���� Ÿ��
    ActionType act_type;    // �÷��̾� ��ü Ÿ��
	uint8_t damage;         // ���� ���ݷ�
    float position[3];      // X, Y, Z
    uint8_t hp;             // ���� ü��
    // ... 
    //ZombieInfo()
    //    : zombieId(0), obj_type(ObjectType::ZOMBIE), act_type(ActionType::NONE), damage(30), hp(500) {}
};

// ���� ���
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
// Ŭ�� -> ����
// --------------------------
// �α��� ��Ŷ
struct PKT_CS_LOGIN {
    PacketHeader header;
    uint8_t name[NAME_SIZE];
    uint8_t obj_type;
    PKT_CS_LOGIN() { header.type = PacketType::LOGIN; }
};

// �̵� ��Ŷ
struct PKT_CS_MOVE {
    PacketHeader header;
    float position[3];
    PKT_CS_MOVE() { header.type = PacketType::MOVE; }
};

// �Ѿ� �߻� ��Ŷ
struct PKT_CS_SHOOT {
    PacketHeader header;
    uint8_t GunType; // �� ����
    //int hitZombieId;
    float bulletPos[3];
    float bulletDir[3];
    PKT_CS_SHOOT() { header.type = PacketType::SHOOT; }
};

// --------------------------
// ���� ->  Ŭ��
// --------------------------
// �α��� OK ��Ŷ
struct PKT_SC_LOGIN_OK {
    PacketHeader header;
    uint32_t playerId;
    uint8_t name[NAME_SIZE];
	uint8_t obj_type;
    PKT_SC_LOGIN_OK() { header.type = PacketType::LOGIN_OK; }
};

// ���� ��ü ��ε�ĳ��Ʈ ��Ŷ
struct PKT_SC_BROADCAST_STATE {
    PacketHeader header;

    uint32_t playerCount;    
    PlayerInfo players[MAX_PLAYER_COUNT];  

    uint32_t zombieCount;    
    ZombieInfo zombies[MAX_ZOMBIE_COUNT]; 


    PKT_SC_BROADCAST_STATE() { header.type = PacketType::BROADCAST_STATE; }
};

// �Ѿ� ���� ���
struct PKT_SC_HIT_RESULT {
    PacketHeader header;
    uint32_t shooterId;   // ���� ������
    uint32_t zombieId;
    uint8_t zombieHp;
    //uint8_t damage;       // �󸶳� �𿴴���

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
    ZombieHit hits[10]; // �ִ� 10���� ���� �ǰ� ó��
};

// --- Object ���� ��Ŷ ---
// ��ü �߰� ��Ŷ (�÷��̾�)
struct PKT_SC_PLAYER_ADD {
    PacketHeader header;
    uint32_t objectId;      // �÷��̾��� ���� ID
    float position[3];      // �÷��̾��� ��ġ
    uint8_t hp;             // �÷��̾� ü��
    uint16_t score;         // �÷��̾� ����
    uint8_t level;          // �÷��̾� ����
    PKT_SC_PLAYER_ADD() { header.type = PacketType::OBJECT_ADD; }
};
//��ü ������Ʈ ��Ŷ (�÷��̾�)
struct PKT_SC_PLAYER_UPDATE {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint16_t score;         
    uint8_t level;          
    PKT_SC_PLAYER_UPDATE() { header.type = PacketType::OBJECT_UPDATE; }
};
// ��ü ���� ��Ŷ (�÷��̾�)
struct PKT_SC_PLAYER_REMOVE {
    PacketHeader header;
    uint32_t objectId;     
    PKT_SC_PLAYER_REMOVE() { header.type = PacketType::OBJECT_REMOVE; }
};

// ��ü �߰� ��Ŷ (���� , ����)
struct PKT_SC_ZOMBIE_ADD {
    PacketHeader header;
    uint32_t objectId;      // ������ ���� ID
    float position[3];      // ������ ��ġ
    uint8_t hp;             // ���� ü��
    uint8_t action_type;    // ������ �ൿ ����
    uint8_t damage;         // ������ ���ݷ�
    PKT_SC_ZOMBIE_ADD() { header.type = PacketType::OBJECT_ADD; }
};
// ��ü ������Ʈ ��Ŷ(���� , ����)
struct PKT_SC_ZOMBIE_UPDATE {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint8_t action_type;   
    uint8_t damage;         
    PKT_SC_ZOMBIE_UPDATE() { header.type = PacketType::OBJECT_UPDATE; }
};
// ��ü ���� ��Ŷ(����, ����)
struct PKT_SC_ZOMBIE_REMOVE {
    PacketHeader header;
    uint32_t objectId;      
    PKT_SC_ZOMBIE_REMOVE() { header.type = PacketType::OBJECT_REMOVE; }
};

// --- ���� ��Ȳ ��Ŷ ---
// STAGE ����
struct PKT_SC_STAGE_INFO {
    PacketHeader header;
    uint16_t currentStage;  
    uint16_t totalStages;
    uint32_t timeLeft;
    PKT_SC_STAGE_INFO() { header.type = PacketType::STAGE_INFO; }
};
// SCORE ����
struct PKT_SC_SCORE_INFO {
    PacketHeader header;
    uint16_t stage_score;
    PKT_SC_SCORE_INFO() { header.type = PacketType::SCORE_INFO; }
};


#pragma pack (pop)

