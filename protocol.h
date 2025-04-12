#pragma once
#include <cstdint>

// --------------------------
// ����/Ŭ�� ���� ��� ����
// --------------------------
constexpr int PORT_NUM = 4000;
constexpr int BUF_SIZE = 200;
constexpr int MAX_NAME_SIZE = 20;

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
	S_C_PLAYER_MOVE = 203,

    S_C_PLAYER_UPDATE = 204,
    S_C_PLAYER_REMOVE = 205,

    S_C_ZOMBIE_ADD = 301,
    S_C_ZOMBIE_UPDATE = 302,
    S_C_ZOMBIE_REMOVE = 303,

    S_C_STAGE_INFO = 401,
    S_C_SCORE_INFO = 402,

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
    char name[MAX_NAME_SIZE];
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
struct pkt_cs_login {
    PacketHeader header;
    char name[MAX_NAME_SIZE];
    uint8_t skin_type;
    pkt_cs_login() { header.type = PKT_TYPE::C_S_LOGIN; }
};

// �̵� ��Ŷ
struct pkt_cs_move {
    PacketHeader header;
    float direction[3];     // ����ȭ�� ���� ����
    float speed;            // �̵� �ӵ� (����: m/s �Ǵ� ����/s)
    pkt_cs_move() { header.type = PKT_TYPE::C_S_MOVE; }
};

// �Ѿ� �߻� ��Ŷ
struct pkt_cs_shoot {
    PacketHeader header;
    uint8_t GunType; // �� ����
    //int hitZombieId;
    float bulletPos[3];
    float bulletDir[3];
    pkt_cs_shoot() { header.type = PKT_TYPE::C_S_SHOOT; }
};

// �Ѿ� ���� ��Ŷ
struct pkt_cs_hit {
	PacketHeader header;
	uint32_t shooterId;   // ���� ������
	uint32_t zombieId;    // ���� ���� ID
	pkt_cs_hit() { header.type = PKT_TYPE::C_S_SHOOT; }
};


// --------------------------
// ���� ->  Ŭ��
// --------------------------

// �Ѿ� ���� ���
struct pkt_sc_hit_result {
    PacketHeader header;
    uint32_t shooterId;   // ���� ������
    uint32_t zombieId;
    uint8_t zombieHp;
    //uint8_t damage;       // �󸶳� �𿴴���

    pkt_sc_hit_result() { header.type = PKT_TYPE::S_C_HIT_RESULT; }
};
struct ZombieHit {
    uint32_t zombieId;
    uint8_t hp;
    uint8_t damage;
};
struct pkt_sc_hit_multi_result {
    PacketHeader header;
    uint8_t hitCount;
    ZombieHit hits[10]; // �ִ� 10���� ���� �ǰ� ó��
};


// --- Object ���� ��Ŷ ---
// ��ü �߰� ��Ŷ (�÷��̾�)
struct pkt_sc_player_add {
    PacketHeader header;
    uint32_t objectId;      // �÷��̾��� ���� ID
	char name[MAX_NAME_SIZE];   // �÷��̾� �̸�
    uint8_t skin_type;      // �÷��̾� ��Ų Ÿ��
    float position[3];      // �÷��̾��� ��ġ
    uint8_t level;          // �÷��̾� ����
    uint8_t hp;             // �÷��̾� ü��
    uint16_t score;         // �÷��̾� ����
    pkt_sc_player_add() { header.type = PKT_TYPE::S_C_PLAYER_ADD; }
};
// ��ü �̵� ��Ŷ (�÷��̾�)
struct pkt_sc_player_move {
    PacketHeader header;
    uint32_t objectId;
    float position[3];
	pkt_sc_player_move() { header.type = PKT_TYPE::S_C_PLAYER_MOVE; }
};

//��ü ������Ʈ ��Ŷ (�÷��̾�)
struct pkt_sc_plyaer_update {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint16_t score;         
    uint8_t level;          
    pkt_sc_plyaer_update() { header.type = PKT_TYPE::S_C_PLAYER_UPDATE; }
};
// ��ü ���� ��Ŷ (�÷��̾�)
struct pkt_sc_player_remove {
    PacketHeader header;
    uint32_t objectId;     
    pkt_sc_player_remove() { header.type = PKT_TYPE::S_C_PLAYER_REMOVE; }
};

// ��ü �߰� ��Ŷ (���� , ����)
struct pkt_sc_zombie_add {
    PacketHeader header;
    uint32_t objectId;      // ������ ���� ID
    float position[3];      // ������ ��ġ
    uint8_t hp;             // ���� ü��
    uint8_t skin_type;      // ���� ��Ų 
    uint8_t action_type;    // ������ �ൿ ����
    uint8_t damage;         // ������ ���ݷ�
    pkt_sc_zombie_add() { header.type = PKT_TYPE::S_C_ZOMBIE_ADD; }
};
// ��ü ������Ʈ ��Ŷ(���� , ����)
struct pkt_sc_zombie_update {
    PacketHeader header;
    uint32_t objectId;     
    float position[3];      
    uint8_t hp;            
    uint8_t action_type;   
    uint8_t damage;         
    pkt_sc_zombie_update() { header.type = PKT_TYPE::S_C_ZOMBIE_UPDATE; }
};
// ��ü ���� ��Ŷ(����, ����)
struct pkt_sc_zombie_remove {
    PacketHeader header;
    uint32_t objectId;      
    pkt_sc_zombie_remove() { header.type = PKT_TYPE::S_C_ZOMBIE_REMOVE; }
};



// --- ���� ��Ȳ ��Ŷ ---
// STAGE ����
struct pkt_sc_stage_info {
    PacketHeader header;
    uint16_t currentStage;  
    uint16_t totalStages;
    uint32_t timeLeft;
    pkt_sc_stage_info() { header.type = PKT_TYPE::S_C_STAGE_INFO; }
};
// SCORE ����
struct pkt_sc_score_info {
    PacketHeader header;
    uint16_t stage_score;
    pkt_sc_score_info() { header.type = PKT_TYPE::S_C_SCORE_INFO; }
};

#pragma pack (pop)

