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

using SIZEID  = uint32_t;
using SIZE1   = uint8_t;
using SIZE2   = uint16_t;
using SIZE3   = uint32_t;


// �÷��̾��� ü��
const SIZE2 PLAYER_HP = 500;


// --------------------------
// ��Ŷ ID ����
// --------------------------

// ���� ���
enum PKT_TYPE : SIZE1 {

    C_S_LOGIN = 1,
    C_S_UPDATE ,
    C_S_SHOOT ,
	C_S_HIT,

    //S_C_LOGIN_OK = 14,
    //S_C_LOGIN_FAIL = 15,
    //S_C_PLAYER_INFO = 16,
    S_C_HIT_RESULT = 10,

    // ������Ʈ ��Ŷ ���� ó����
    S_C_OBJECT_ADD = 30,
    S_C_OBJECT_UPDATE ,
    S_C_OBJECT_REMOVE ,

    S_C_STAGE_INFO = 40,
    S_C_SCORE_INFO,
    // ...
};

// --------------------------
// ��Ŷ ����ü ����
// --------------------------
#pragma pack (push, 1) 

struct Vector3 {
    float x, y, z;

    constexpr Vector3(float _x = 0.f, float _y = 0.f, float _z = 0.f)
        : x(_x), y(_y), z(_z) {}

    // ��Į�� ��
    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }
    // ���� ����
    Vector3& operator+=(const Vector3& rhs) {
        x += rhs.x; 
        y += rhs.y; 
        z += rhs.z;
        return *this;
    }
    // ���� ����
    Vector3 operator+(const Vector3& rhs) const {
        return Vector3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    // ���� ����ȭ
    Vector3 Normalize() const {
        float len = sqrtf(x * x + y * y + z * z);
        return (len > 0.f) ? Vector3(x / len, y / len, z / len) : Vector3();
    }

    // ���� ����
    float Length() const {
        return sqrtf(x * x + y * y + z * z);
    }
};


// ���� ��ġ
constexpr Vector3 START_POSITIONS[3] = {
    { 100.0f, 0.0f, 100.0f },
    { 110.0f, 0.0f, 100.0f },
    { 120.0f, 0.0f, 100.0f },
};

// ���� �� ��ü Ÿ�� ����
enum ObjectType : SIZE1 {
    PLAYER  = 1,
    ZOMBIE  = 2,
	BULLET  = 3,
	BOSS    = 4,

};

// ���� ���� ����
enum ActionType : SIZE1 {
	NONE    = 0,   // ��ȸ
	ZMOVE   = 1,   // ���� �̵�
    ATTACK  = 2,   // ����
    RANGED  = 3,   // ���Ÿ�
    POISON  = 4,   // ��

};

enum SkinType : SIZE1 {
    PLAYER_NORMAL = 0,
    PLAYER_POLICE ,
    PLAYER_SOLDIER ,

    // ���� ��Ų Ÿ��
    ZOMBIE_NORMAL = 10,
    ZOMBIE_RUNNER ,
    ZOMBIE_WITCH ,
    ZOMBIE_BOSS,
};


// �� ���� 
enum GunType : SIZE1 {
    BULLET_PISTOL = 0,
    BULLET_RIFLE,
    BULLET_SHOTGUN,
    BULLET_MAX
};
// �� ����
struct BulletInfo {
    float speed;         // �Ѿ� �ӵ� (m/s �Ǵ� ���� ����)
    float radius;        // �浹 ���� ������ (����: meter)
    SIZE1 damage;        // ���� �� ������
    SIZE1 count;         // �߻� �� �Ѿ� ���� (������ 5~7)
};
constexpr BulletInfo BULLET_TABLE[] = {
	 { 350.f, 0.15f, 25, 1 }, // BULLET_PISTOL
	 { 850.f, 0.10f, 35, 1 }, // BULLET_RIFLE
	 { 400.f, 0.20f, 12, 6 }  // BULLET_SHOTGUN
};

struct Objectfixdata {          // ��������
    ObjectType obj_type;
    SIZE1 skin_type;
    char name[MAX_NAME_SIZE];
    Vector3 startposition;      // �ʱ� ��ġ
    SIZE2 starthp;              // ü��
};

struct ObjectMeta {             // �ʼ�����
    Vector3 position;           // ��ġ
    Vector3 direction;          // ����
    float speed;                // �̵� �ӵ� (����: m/s �Ǵ� ����/s)
    SIZE2 hp;                   // ü��
};

struct ObjectDynamicInfo {  	// ��������
    ObjectMeta meta;            // �ʼ�����
	GunType gun_type;           // �� ����
    SIZE1 level;                // ����
    SIZE2 score;                // ����
    SIZE2 damage;               // ���ݷ�
    SIZE1 act_type;             // NONE, Player, ZMOVE, ATTACK, ...
};

//// �÷��̾� ���� ����ü
//struct PlayerInfo {
//    uint8_t id;
//    float position[3];  // X, Y, Z
//    char name[MAX_NAME_SIZE];
//    uint8_t skin_type;  // �÷��̾� ��ü ��Ų
//	uint8_t level;      // ����
//    uint8_t hp;         // �÷��̾� ü��
//    uint16_t score;     // ����
//    // ... 
//};
//// ���� ���� ����ü
//struct ZombieInfo {
//    uint32_t id;
//    ObjectType obj_type;    // ���� Ÿ��
//    ActionType act_type;    // �÷��̾� ��ü Ÿ��
//	uint8_t damage;         // ���� ���ݷ�
//    float position[3];      // X, Y, Z
//    uint8_t hp;             // ���� ü��
//    // ... 
//    //ZombieInfo()
//    //    : zombieId(0), obj_type(ObjectType::ZOMBIE), act_type(ActionType::NONE), damage(30), hp(500) {}
//};

struct PacketHeader {
    SIZE2 size;
    PKT_TYPE type;
};

// --------------------------
// Ŭ�� -> ����
// --------------------------
// �α��� ��Ŷ
struct pkt_cs_login {
    PacketHeader header;
    SIZE1 skin_type;
    char name[MAX_NAME_SIZE];

    pkt_cs_login() { header.type = PKT_TYPE::C_S_LOGIN; }
};

// �÷��̾� ������Ʈ ��Ŷ
struct pkt_cs_update {
    PacketHeader header;
    ObjectDynamicInfo obj;          // �÷��̾� ����
    pkt_cs_update() { header.type = PKT_TYPE::C_S_UPDATE; }
};

// �Ѿ� �߻� ��Ŷ
struct pkt_cs_shoot {
    PacketHeader header;
	SIZEID id;                      // ���� ������
    SIZE1 GunType;                  // �� ����
    //int hitZombieId;
    float bulletPos[3];
    float bulletDir[3];
    pkt_cs_shoot() { header.type = PKT_TYPE::C_S_SHOOT; }
};

// �Ѿ� ���� ��Ŷ
struct pkt_cs_hit {
	PacketHeader header;
    SIZEID shooterId;               // ���� ������
    SIZEID zombieId;                // ���� ���� ID
	pkt_cs_hit() { header.type = PKT_TYPE::C_S_SHOOT; }
};


// --------------------------
// ���� ->  Ŭ��
// --------------------------
// �Ѿ� ���� ���
struct pkt_sc_hit_result {
    PacketHeader header;
    SIZEID shooterId;               // ���� ������
    SIZEID zombieId;
    SIZE2 zombieHp;
    //uint8_t damage;               // �󸶳� �𿴴���

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
    ZombieHit hits[10];             // �ִ� 10���� ���� �ǰ� ó��
};


// --- Object ���� ��Ŷ ---
struct pkt_sc_object_add {
	PacketHeader header;
	SIZEID id; // ID
	Objectfixdata fixdata;          // ��������
    pkt_sc_object_add() { header.type = PKT_TYPE::S_C_OBJECT_ADD; }
};

// ��ü ������Ʈ
struct pkt_sc_object_update {
    PacketHeader header;
	SIZEID id; // ID
    ObjectDynamicInfo obj;          // ��������
    pkt_sc_object_update() { header.type = PKT_TYPE::S_C_OBJECT_UPDATE; }
};
// ��ü ����
struct pkt_sc_object_remove {
    PacketHeader header;
    SIZEID id;
    // ObjectType obj_type;
    pkt_sc_object_remove() { header.type = PKT_TYPE::S_C_OBJECT_REMOVE; }
};

// --- ���� ��Ȳ ��Ŷ ---
// STAGE ����
struct pkt_sc_stage_info {
    PacketHeader header;
    SIZE2 currentStage;
    SIZE2 totalStages;
    SIZE3 timeLeft;
    pkt_sc_stage_info() { header.type = PKT_TYPE::S_C_STAGE_INFO; }
};
// SCORE ����
struct pkt_sc_score_info {
    PacketHeader header;
    SIZE2 stage_score;
    pkt_sc_score_info() { header.type = PKT_TYPE::S_C_SCORE_INFO; }
};

#pragma pack (pop)

