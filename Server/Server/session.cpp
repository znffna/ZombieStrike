#include "session.h"
#include "ZombieAI.h"
#include "network.h"

#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <cstring>
#include <print>

#define DEBUG_PRINT false
#define DEBUG_LOG(msg) do { if (DEBUG_PRINT) std::cout << msg << std::endl; } while (0)

extern int IN_g_player_n;
extern std::vector<ZombieAI*> g_zombies;
extern std::unordered_map<SIZEID, std::shared_ptr<GameObject>> g_gameObjects;



OVER_EXP::OVER_EXP(IO_OP op) : _io_op(op) {
    ZeroMemory(&_over, sizeof(_over));
    _wsabuf[0].buf = reinterpret_cast<CHAR*>(_buffer);
    _wsabuf[0].len = sizeof(_buffer);
}

PlayerSession::PlayerSession(SIZEID session_id, SOCKET s)
    : _c_socket(s), _recv_over(OP_RECV), _remained(0) {
    _id = session_id;
    _recv_over._wsabuf[0].len = sizeof(_recv_over._buffer);
    _recv_over._wsabuf[0].buf = reinterpret_cast<CHAR*>(_recv_over._buffer);
    _recv_over._over.hEvent = reinterpret_cast<HANDLE>(_id);
    do_recv();
}

PlayerSession::~PlayerSession() {
    closesocket(_c_socket);

    pkt_sc_object_remove rem_p;
    rem_p.header.size = sizeof(rem_p);
    rem_p.header.type = PKT_TYPE::S_C_OBJECT_REMOVE;
    rem_p.id = _id;

    for (auto& [id, obj] : g_gameObjects)
        if (id != _id)
            obj->do_send(&rem_p);
}

void PlayerSession::do_recv() {
    DWORD flags = 0;
    ZeroMemory(&_recv_over._over, sizeof(_recv_over._over));
    _recv_over._over.hEvent = reinterpret_cast<HANDLE>(_id);

    _recv_over._wsabuf[0].buf = reinterpret_cast<CHAR*>(_recv_over._buffer) + _remained;
    _recv_over._wsabuf[0].len = sizeof(_recv_over._buffer) - _remained;

    int ret = WSARecv(_c_socket, _recv_over._wsabuf, 1, 0, &flags, &_recv_over._over, g_recv_callback);
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        std::cout << "[do_recv] WSARecv failed: " << WSAGetLastError() << "\n";
        closesocket(_c_socket);
        g_gameObjects.erase(_id);
    }
}

void PlayerSession::recv_callback(int num_bytes) {
    SIZE2* p = _recv_over._buffer;
    SIZE3 total = _remained + num_bytes;
    SIZE3 offset = 0;

    while (offset < total) {
        SIZE2 packetSize = *p;
        if (offset + packetSize > total) break;

        process_packet(p);
        p += packetSize / sizeof(SIZE2);
        offset += packetSize;
    }

    _remained = total - offset;
    if (_remained > 0)
        memmove(_recv_over._buffer, p, _remained);

    do_recv();
}





void PlayerSession::do_send(void* buff) {
    OVER_EXP* send_ov = new OVER_EXP(OP_SEND);
    SIZE2 packet_size = reinterpret_cast<SIZE2*>(buff)[0];
    memcpy(send_ov->_buffer, buff, packet_size);
    send_ov->_wsabuf[0].buf = reinterpret_cast<CHAR*>(send_ov->_buffer);
    send_ov->_wsabuf[0].len = packet_size;

    DWORD size_sent;
    int ret = WSASend(_c_socket, send_ov->_wsabuf, 1, &size_sent, 0, &(send_ov->_over), g_send_callback);
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
        std::cout << "[do_send] WSASend failed: " << WSAGetLastError() << "\n";
}

void PlayerSession::send_obj_info() {
    pkt_sc_obj_info packet{};
    packet.header.size = sizeof(packet);
    packet.header.type = PKT_TYPE::S_C_OBJ_INFO;
    packet.id = _id;
    packet.obj_type = _obj_type;
    packet.skin_type = _skin_type;
    strcpy_s(packet.name, _name.c_str());
    packet.startposition = _position;
    packet.starthp = _hp;
    packet.velocity = _velocity;
    packet.look = _look;
    packet.pitch = _pitch;
    packet.act_type = _act_type;
    packet.gun_type = _gun_type;
    packet.level = _level;
    packet.score = _score;
    //packet.damage = _damage;

    do_send(&packet);
}

void PlayerSession::send_object_update() {
    pkt_sc_object_update p{};
    p.header.size = sizeof(p);
    p.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
    p.id = _id;
    p.velocity = _velocity;
    p.look = _look;
    p.pitch = _pitch;
    p.hp = _hp;
    p.gun_type = _gun_type;
    p.level = _level;
    p.score = _score;
    p.act_type = _act_type;

    do_send(&p);
}

void PlayerSession::process_packet(SIZE2* packet) {

    DEBUG_LOG("[PROCESS_PACKET] 호출됨: ID = " << _id << ", type = " << (int)packet[1]);

    const auto type = static_cast<PKT_TYPE>(packet[1]);
    switch (type) {
    case PKT_TYPE::C_S_LOGIN:        process_login_packet(packet); break;
    case PKT_TYPE::C_S_UPDATE:       process_update_packet(packet); break;
    case PKT_TYPE::C_S_SCORE_INFO:   process_score_info(packet); break;
    case PKT_TYPE::C_S_STAGE_INFO:   process_stage_info(packet); break;
    default:
        std::cout << "[WARN] Unknown PacketType: " << (int)type << "\n";
        break;
    }
}

void PlayerSession::process_login_packet(SIZE2* packet) {
    auto* login = reinterpret_cast<pkt_cs_login*>(packet);
    _obj_type = ObjectType::PLAYER;
    _skin_type = login->skin_type;
    _name = login->name;
    _position = START_POSITIONS[IN_g_player_n % 3];
    _velocity = {};
    _look = {};
    _pitch = 0.f;
    _hp = PLAYER_HP;
    _gun_type = BULLET_PISTOL;
    _level = 1;
    _score = 0;
    _damage = 0;
    _act_type = NONE;
    IN_g_player_n++;

    send_obj_info();

    pkt_sc_object_add pkt{};
    pkt.header.size = sizeof(pkt);
    pkt.header.type = S_C_OBJECT_ADD;
    pkt.id = _id;
    pkt.obj_type = _obj_type;
    pkt.skin_type = _skin_type;
    strcpy_s(pkt.name, _name.c_str());
    pkt.startposition = _position;
    pkt.starthp = _hp;
    pkt.gun_type = _gun_type;

	std::cout << "[LOGIN] 로그인 패킷 수신: ID = " << _id << ", 이름 = " << _name << "\n";
    for (auto& [id, obj] : g_gameObjects)
        if (id != _id)
            obj->do_send(&pkt);

    for (auto& [id, obj] : g_gameObjects) {
        if (id == _id) continue;
        auto other = dynamic_cast<PlayerSession*>(obj.get());
        if (!other) continue;

        pkt_sc_object_add other_pkt{};
        other_pkt.header.size = sizeof(other_pkt);
        other_pkt.header.type = S_C_OBJECT_ADD;
        other_pkt.id = other->_id;
        other_pkt.obj_type = other->_obj_type;
        other_pkt.skin_type = other->_skin_type;
        strcpy_s(other_pkt.name, other->_name.c_str());
        other_pkt.startposition = other->_position;
        other_pkt.starthp = other->_hp;
        do_send(&other_pkt);
    }
    DEBUG_LOG("[LOGIN] 유저 로그인 ID = " << _id);

    pkt_sc_object_add zombie_pkt{};
    for (auto zombie : g_zombies) {

        DEBUG_LOG("[Zombie] 로그인 클라이언트에게 전송: ID = " << zombie->GetID());

        zombie_pkt.header.size = sizeof(zombie_pkt);
        zombie_pkt.header.type = S_C_OBJECT_ADD;
        zombie_pkt.id = zombie->GetID();
        zombie_pkt.obj_type = ZOMBIE;
        zombie_pkt.skin_type = 0;
        strcpy_s(zombie_pkt.name, "Zombie");
        zombie_pkt.startposition = zombie->GetPosition();
        zombie_pkt.starthp = zombie->GetHP();
        do_send(&zombie_pkt);
    }
    DEBUG_LOG("[LOGIN] 현재 g_zombies 개수 = " << g_zombies.size());
}

void PlayerSession::process_update_packet(SIZE2* packet) {
    auto* update = reinterpret_cast<pkt_cs_update*>(packet);

    _position = update->position;
    _velocity = update->velocity;
    _look = update->look;
    _pitch = update->pitch;
    _hp = update->hp;
    _gun_type = update->gun_type;
    _level = update->level;
    _score = update->score;
    _damage = update->damage;
    _act_type = update->act_type;

    pkt_sc_object_update p{};
    p.header.size = sizeof(p);
    p.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
    p.id = _id;
    p.position = _position;
    p.velocity = _velocity;
    p.look = _look;
    p.pitch = _pitch;
    p.hp = _hp;
    p.gun_type = _gun_type;
    p.level = _level;
    p.score = _score;
    p.damage = _damage;
    p.act_type = _act_type;

    for (auto& [id, session] : g_gameObjects)
        if (id != _id)
            session->do_send(&p);
}
void PlayerSession::process_score_info(SIZE2* packet) {
    auto* p = reinterpret_cast<pkt_cs_score_info*>(packet);
    if (p->stage_score > 10000) {
        std::cout << "[SCORE_INFO] 유효하지 않은 점수 무시\n";
        return;
    }

    pkt_sc_score_info resp{};
    resp.header.size = sizeof(resp);
    resp.header.type = PKT_TYPE::S_C_SCORE_INFO;
    resp.stage_score = p->stage_score;

    for (auto& [id, session] : g_gameObjects)
        session->do_send(&resp);
}
void PlayerSession::process_stage_info(SIZE2* packet) {
    auto* p = reinterpret_cast<pkt_cs_stage_info*>(packet);
    if (p->currentStage < 1 || p->currentStage > 10 || p->timeLeft > 60000) {
        std::cout << "[STAGE_INFO] 유효하지 않은 값 무시\n";
        return;
    }

    pkt_sc_stage_info resp{};
    resp.header.size = sizeof(resp);
    resp.header.type = PKT_TYPE::S_C_STAGE_INFO;
    resp.currentStage = p->currentStage;
    resp.totalStages = 1;
    resp.timeLeft = p->timeLeft;

    for (auto& [id, session] : g_gameObjects)
        session->do_send(&resp);
}
