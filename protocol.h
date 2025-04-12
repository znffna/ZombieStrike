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
// �ִ� �÷��̾� ��
constexpr short MAX_PLAYER_COUNT = 3;
// �ִ� ���� ��
constexpr short MAX_ZOMBIE_COUNT = 1000;

// ���� ũ�� ����
constexpr int W_WIDTH = 500;
constexpr int W_HEIGHT = 500;

// ���� ��ġ
const float START_POSITIONS[3][3] = {
    { 100.0f, 0.0f, 100.0f },
    { 110.0f, 0.0f, 100.0f },
    { 120.0f, 0.0f, 100.0f },
};
// �÷��̾��� ü��
const uint8_t PLAYER_HP = 500;

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
//        return { 350.f, 0.15f, 25, 1 }; // �⺻��
//    }
//}


// --------------------------
// ��Ŷ ID ����
// --------------------------

// ���� ���
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
    uint32_t id;
    float position[3];  // X, Y, Z
    char name[NAME_SIZE];
    uint8_t skin_type;  // �÷��̾� ��ü ��Ų
	uint8_t level;      // ����
    uint8_t hp;         // �÷��̾� ü��
    uint16_t score;     // ����
    // ... 
};

// ���� ���� ����ü
struct ZombieInfo {
    uint32_t id;
    ObjectType obj_type;    // ���� Ÿ��
    ActionType act_type;    // �÷��̾� ��ü Ÿ��
	uint8_t damage;         // ���� ���ݷ�
    float position[3];      // X, Y, Z
    uint8_t hp;             // ���� ü��
    // ... 
    //ZombieInfo()
    //    : zombieId(0), obj_type(ObjectType::ZOMBIE), act_type(ActionType::NONE), damage(30), hp(500) {}
};


struct PacketHeader {
    uint16_t size;
    PKT_TYPE type;
};

// --------------------------
// Ŭ�� -> ����
// --------------------------
// �α��� ��Ŷ
struct PKT_CS_LOGIN {
    PacketHeader header;
    char name[NAME_SIZE];
    uint8_t skin_type;
    PKT_CS_LOGIN() { header.type = PKT_TYPE::C_S_LOGIN; }
};

// �̵� ��Ŷ
struct PKT_CS_MOVE {
    PacketHeader header;
    float position[3];
    PKT_CS_MOVE() { header.type = PKT_TYPE::C_S_MOVE; }
};

// �Ѿ� �߻� ��Ŷ
struct PKT_CS_SHOOT {
    PacketHeader header;
    uint8_t GunType; // �� ����
    //int hitZombieId;
    float bulletPos[3];
    float bulletDir[3];
    PKT_CS_SHOOT() { header.type = PKT_TYPE::C_S_SHOOT; }
};




// --------------------------
// ���� ->  Ŭ��
// --------------------------
// �α��� OK ��Ŷ
struct PKT_SC_LOGIN_OK {
    PacketHeader header;
    uint32_t playerId;
    char name[NAME_SIZE];
	uint8_t skin_type;
    PKT_SC_LOGIN_OK() { header.type = PKT_TYPE::S_C_LOGIN_OK; }
};

//// ���� ��ü ��ε�ĳ��Ʈ ��Ŷ
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

// �Ѿ� ���� ���
struct PKT_SC_HIT_RESULT {
    PacketHeader header;
    uint32_t shooterId;   // ���� ������
    uint32_t zombieId;
    uint8_t zombieHp;
    //uint8_t damage;       // �󸶳� �𿴴���

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
    ZombieHit hits[10]; // �ִ� 10���� ���� �ǰ� ó��
};


// --- Object ���� ��Ŷ ---
// ��ü �߰� ��Ŷ (�÷��̾�)
struct PKT_SC_PLAYER_ADD {
    PacketHeader header;
    uint32_t objectId;      // �÷��̾��� ���� ID
    uint8_t skin_type;      // �÷��̾� ��Ų Ÿ��
    float position[3];      // �÷��̾��� ��ġ
    uint8_t level;          // �÷��̾� ����
    uint8_t hp;             // �÷��̾� ü��
    uint16_t score;         // �÷��̾� ����
    PKT_SC_PLAYER_ADD() { header.type = PKT_TYPE::S_C_PLAYER_ADD; }
};
//��ü ������Ʈ ��Ŷ (�÷��̾�)
struct PKT_SC_PLAYER_UPDATE {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint16_t score;         
    uint8_t level;          
    PKT_SC_PLAYER_UPDATE() { header.type = PKT_TYPE::S_C_PLAYER_UPDATE; }
};
// ��ü ���� ��Ŷ (�÷��̾�)
struct PKT_SC_PLAYER_REMOVE {
    PacketHeader header;
    uint32_t objectId;     
    PKT_SC_PLAYER_REMOVE() { header.type = PKT_TYPE::S_C_PLAYER_REMOVE; }
};

// ��ü �߰� ��Ŷ (���� , ����)
struct PKT_SC_ZOMBIE_ADD {
    PacketHeader header;
    uint32_t objectId;      // ������ ���� ID
    float position[3];      // ������ ��ġ
    uint8_t hp;             // ���� ü��
    uint8_t skin_type;      // ���� ��Ų 
    uint8_t action_type;    // ������ �ൿ ����
    uint8_t damage;         // ������ ���ݷ�
    PKT_SC_ZOMBIE_ADD() { header.type = PKT_TYPE::S_C_ZOMBIE_ADD; }
};
// ��ü ������Ʈ ��Ŷ(���� , ����)
struct PKT_SC_ZOMBIE_UPDATE {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint8_t action_type;   
    uint8_t damage;         
    PKT_SC_ZOMBIE_UPDATE() { header.type = PKT_TYPE::S_C_ZOMBIE_UPDATE; }
};
// ��ü ���� ��Ŷ(����, ����)
struct PKT_SC_ZOMBIE_REMOVE {
    PacketHeader header;
    uint32_t objectId;      
    PKT_SC_ZOMBIE_REMOVE() { header.type = PKT_TYPE::S_C_ZOMBIE_REMOVE; }
};





// --- ���� ��Ȳ ��Ŷ ---
// STAGE ����
struct PKT_SC_STAGE_INFO {
    PacketHeader header;
    uint16_t currentStage;  
    uint16_t totalStages;
    uint32_t timeLeft;
    PKT_SC_STAGE_INFO() { header.type = PKT_TYPE::S_C_STAGE_INFO; }
};
// SCORE ����
struct PKT_SC_SCORE_INFO {
    PacketHeader header;
    uint16_t stage_score;
    PKT_SC_SCORE_INFO() { header.type = PKT_TYPE::S_C_SCORE_INFO; }
};


#pragma pack (pop)

