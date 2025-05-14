#pragma once
#include <winsock2.h>
#include <string>
#include "network.h"
#include "../../protocol.h"

enum IO_OP { OP_RECV, OP_SEND };

class OVER_EXP {
public:
    WSAOVERLAPPED _over;
    IO_OP _io_op;
    SIZE2 _buffer[1024];
    WSABUF _wsabuf[1];

    OVER_EXP(IO_OP op);
};


class GameObject {
public:
    SIZEID          _id;
    ObjectType      _obj_type;
    Vec3            _position{}, _velocity{}, _look{};
    SIZE2           _hp = 0;
    SIZE1           _skin_type;
    SIZE1           _act_type = NONE;

    virtual void do_recv() {};
    virtual void recv_callback(int) {};
    virtual void do_send(void* buff) {};
    virtual void send_obj_info() = 0;
    virtual void send_object_update() = 0;
    virtual void process_packet(SIZE2* packet) = 0;

    virtual ~GameObject() = default;
};

class ZombieObject : public GameObject {
public:
    SIZE2           _damage = 0;

    ZombieObject(SIZEID id) {
        _id = id;
        _obj_type = ObjectType::ZOMBIE;
    }
};

class PlayerSession : public GameObject {
public:
    SOCKET          _c_socket;
    std::string     _name;
    float           _pitch = 0.f;
    GunType         _gun_type = BULLET_PISTOL;
    SIZE1           _level = 1;
    SIZE2           _score = 0;
    SIZE2           _damage = 0; // 필요없음

    OVER_EXP _recv_over;
    SIZE2 _remained;

    PlayerSession(SIZEID session_id, SOCKET s);
    ~PlayerSession();

    void do_recv() override;
    void recv_callback(int num_bytes) override;
    void do_send(void* buff) override;
    void process_packet(SIZE2* packet) override;

    void process_login_packet(SIZE2* packet);
    void process_update_packet(SIZE2* packet);
    void process_score_info(SIZE2* packet);
    void process_stage_info(SIZE2* packet);

    void send_obj_info() override;
    void send_object_update() override;
};

