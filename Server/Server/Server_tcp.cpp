#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <queue>
#include <cmath>
#include <string>
#include <algorithm>
#include "../../protocol.h"
#include "ZombieAI.h" 

#pragma comment(lib, "ws2_32.lib")

constexpr bool DEBUG_PRINT = false;
#define DEBUG_LOG(msg) \
    do { if (DEBUG_PRINT) std::cout << msg << std::endl; } while (0)

void error_display(const char* msg, int err_no) {
    WCHAR* lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    std::cout << msg;
    std::wcout << L" Error: " << lpMsgBuf << std::endl;
    LocalFree(lpMsgBuf);
    exit(1);
}


class SESSION;
std::atomic<bool> serverRunning = true;         // 서버 종료 여부
std::mutex g_zombie_mutex;                         // 좀비 쓰레드 보호용

std::vector<std::vector<int>> g_map;               // 맵 데이터
std::vector<ZombieAI*> g_zombies;                  // 좀비 객체 리스트
std::unordered_map<SIZEID, SESSION> g_users;       // 클라이언트 세션 관리

SIZEID g_next_client_id = 0;                       // 클라이언트 고유 ID 부여용
short g_next_spawn_slot = 0;                       // 플레이어 시작 위치 인덱스 (START_POSITIONS)

std::atomic<SIZE2> g_total_stages = 3;
std::atomic<SIZE2> g_current_stage = 1;
constexpr SIZE2 g_max_stage = 3;
std::atomic<SIZE3> g_time_left = 180;

short IN_g_player_n = 0;


void CALLBACK g_recv_callback(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void CALLBACK g_send_callback(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);


enum IO_OP { OP_RECV, OP_SEND };
class OVER_EXP {
public:

    OVER_EXP(IO_OP op) : _io_op(op) {
        ZeroMemory(&_over, sizeof(_over));

        _wsabuf[0].buf = reinterpret_cast<CHAR*>(_buffer);
        _wsabuf[0].len = sizeof(_buffer);
    }

    WSAOVERLAPPED   _over;
    IO_OP           _io_op;
    SIZE2           _buffer[1024];
    WSABUF          _wsabuf[1];
};

class SESSION {
public:
    SOCKET          _c_socket;
    SIZEID          _id;

    OVER_EXP        _recv_over{OP_RECV};
    SIZE2           _remained = 0;

    // 게임 정보
    ObjectType      _obj_type = ObjectType::PLAYER;
    SIZE1           _skin_type;
    std::string     _name;

    Vec3            _position;
    Vec3            _velocity;
	float           _pitch;

    SIZE2           _hp;
    GunType         _gun_type;     
    SIZE1           _level;
    SIZE2           _score;
    SIZE2           _damage;               
	ActionType	    _act_type = ActionType::NONE;

    void do_recv() {
        DWORD flags = 0;
        ZeroMemory(&_recv_over._over, sizeof(_recv_over._over));
        _recv_over._over.hEvent = reinterpret_cast<HANDLE>(_id); // 세션 ID를 이벤트 핸들로 사용

        _recv_over._wsabuf[0].buf = reinterpret_cast<CHAR*>(_recv_over._buffer) + _remained;	//prev_remain 부분에 이어서 수신하기 위해서
        _recv_over._wsabuf[0].len = sizeof(_recv_over._buffer) - _remained;

        int ret = WSARecv(_c_socket, _recv_over._wsabuf, 1, 0, &flags, &_recv_over._over, g_recv_callback);
        if (ret == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSA_IO_PENDING) {
				std::cout << "[do_recv] WSARecv failed: " << err << "\n";
                closesocket(_c_socket);
                g_users.erase(_id);
            }
            else {
				DEBUG_LOG("[do_recv] WSARecv: IO_PENDING (정상)\n");
            }
        }
        else {
			DEBUG_LOG("[do_recv] WSARecv: 즉시 수신 완료 (ret == 0)\n");
        }
    }

public: 
    SESSION() {
		DEBUG_LOG("[SESSION] Default constructor called\n");
        exit(-1);
    }
	SESSION(SIZEID session_id, SOCKET s) : _id(session_id), _c_socket(s)
    {
        _recv_over._wsabuf[0].len = sizeof(_recv_over._buffer);
        _recv_over._wsabuf[0].buf = reinterpret_cast<CHAR* >(_recv_over._buffer);

        _recv_over._over.hEvent = reinterpret_cast<HANDLE>(session_id);

        _remained = 0;
		do_recv();
	}
    ~SESSION() 
    {
		DEBUG_LOG("[SESSION] Destructor called ID =\n", _id);

        pkt_sc_object_remove rem_p;
        rem_p.header.size = sizeof(rem_p);
        rem_p.header.type = PKT_TYPE::S_C_OBJECT_REMOVE;
        rem_p.id= _id;
        for (auto& u : g_users) {
			if (u.first != _id) // 나를 제외한 상대방에게 알리고
				u.second.do_send(&rem_p);
        }
		closesocket(_c_socket);
    }

    void recv_callback(int num_bytes) {
        // ----- 패킷 조립 시작 -----
        SIZE2* p = _recv_over._buffer;
        SIZE3 total = _remained + num_bytes;
        SIZE3 offset = 0;

        while (offset < total) {
            SIZE2 packetSize = *p;

            if (offset + packetSize > total) break; // 아직 패킷 완성이 안 됨

			DEBUG_LOG("[RECV][" << _id << "] packetSize = " << (SIZE3)packetSize << std::endl);

			process_packet(p);    // 패킷 처리
            p += (packetSize)/sizeof(SIZE2);      // 다음 패킷으로 이동
            offset += packetSize;
        }

        // 조립 안 된 데이터는 앞으로 당겨서 저장
        _remained = total - offset;

        if (_remained > 0)
            memmove(_recv_over._buffer, p, _remained);

        do_recv(); // 다음 수신
	}

    void do_send(void* buff) {
        OVER_EXP* send_ov = new OVER_EXP(OP_SEND);
        SIZE2 packet_size = reinterpret_cast<SIZE2*>(buff)[0];
        memcpy(send_ov->_buffer, buff, packet_size);
        send_ov->_wsabuf[0].buf = reinterpret_cast<CHAR*>(send_ov->_buffer);
        send_ov->_wsabuf[0].len = packet_size;
        DWORD size_sent;

		DEBUG_LOG("[do_send] ID = " << _id << ", size = " << packet_size << ", type = " << (int)reinterpret_cast<SIZE2*>(buff)[1] << std::endl);
        int ret = WSASend(_c_socket, send_ov->_wsabuf, 1, &size_sent, 0, &(send_ov->_over), g_send_callback);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			std::cout << "[do_send] WSASend failed: " << WSAGetLastError() << "\n";
        }
    }

    void send_player_add() {
        pkt_sc_object_add packet;
		ZeroMemory(&packet, sizeof(packet));
		packet.header.size = sizeof(packet);
        packet.header.type = PKT_TYPE::S_C_OBJECT_ADD;
		packet.id = _id;
		packet.obj_type = _obj_type;
		packet.skin_type = _skin_type;
		strcpy_s(packet.name, _name.c_str());
		packet.startposition = _position;
		packet.hp = _hp;

		packet.gun_type = _gun_type;
        packet.act_type = _act_type;
		packet.damage = _damage;
        do_send(&packet);
    }

    void broadcast_my_spawn() {
        pkt_sc_object_add packet;
        packet.header.size = sizeof(packet);
        packet.header.type = PKT_TYPE::S_C_OBJECT_ADD;
        packet.id = _id;
        packet.obj_type = _obj_type;
        packet.skin_type = _skin_type;
        strcpy_s(packet.name, _name.c_str());
        packet.startposition = _position;
        packet.hp = _hp;

        packet.gun_type = _gun_type;
        packet.act_type = _act_type;
        packet.damage = _damage;

        for (auto& [id, session] : g_users) {
            if (id != _id) {
                session.do_send(&packet);
            }
        }
    }
    void send_all_other_players() {
        for (auto& [id, session] : g_users) {
            if (id == _id) continue;

            pkt_sc_object_add packet;
            packet.header.size = sizeof(packet);
            packet.header.type = PKT_TYPE::S_C_OBJECT_ADD;
            packet.id = session._id;
			packet.obj_type = session._obj_type;
			packet.skin_type = session._skin_type;
			strcpy_s(packet.name, session._name.c_str());
			packet.startposition = session._position;
			packet.hp = session._hp;

			packet.gun_type = session._gun_type;
			packet.act_type = session._act_type;
			packet.damage = ZOMBIE_DAMAGE;
			do_send(&packet);
		}
    }
    
	void send_all_zombies() {
        for (auto* zombie : g_zombies) {
            pkt_sc_object_add packet;
            packet.header.size = sizeof(packet);
            packet.header.type = PKT_TYPE::S_C_OBJECT_ADD;

            packet.id = zombie->GetID();
            packet.obj_type = ObjectType::ZOMBIE;
            packet.skin_type = 0;
            strcpy_s(packet.name, "Zombie");
            packet.startposition = zombie->GetPosition();
            packet.hp = zombie->GetHP();
            packet.gun_type = GunType::BULLET_MAX;

            do_send(&packet);  // ← 자기 자신에게만 전송
        }
    }

	void process_packet(SIZE2* packet) {

		const unsigned char packet_type = packet[1];
        if (packet_type == 0) {
            std::cout << "[ERROR] Invalid Packet Type\n";
            return;
        }
		DEBUG_LOG("[process_packet] ID = " << _id << ", packet_type = " << (int)packet_type << "\n");

        switch (packet_type) {
        case ::PKT_TYPE::C_S_LOGIN:
        {
            pkt_cs_login* loginPacket = reinterpret_cast<pkt_cs_login*>(packet);
            _obj_type   = ObjectType::PLAYER;
            _skin_type  = loginPacket->skin_type;
            _name       = loginPacket->name;

            _position   = START_POSITIONS[IN_g_player_n];
            _velocity   = Vec3(0, 0, 0);
            _pitch      = 0.0f;

            _hp         = PLAYER_HP;
			_gun_type   = GunType::BULLET_PISTOL; // 총 종류
            _level      = 1;
            _score      = 0;
            _damage     = 0;
			_act_type   = ActionType::NONE;
            
            IN_g_player_n++;

			DEBUG_LOG("[process_packet][RECV][" << (int)_id << "] C_S_LOGIN: " << _name << "\n");
			DEBUG_LOG("[process_packet][RECV][" << (int)_id << "] C_S_LOGIN: " << _skin_type << "\n");

            send_player_add();            // 본인에게 자신 정보 전송
            broadcast_my_spawn();         // 다른 유저에게 나의 정보를 알림
            send_all_other_players();     // 다른 유저들의 정보를 나에게 알려줌
            send_all_zombies();           // 서버 좀비 정보도 전송

            break;
        }
        case PKT_TYPE::C_S_UPDATE:
        {
            auto* p = reinterpret_cast<pkt_cs_update*>(packet);

            // [1] 클라이언트에서 보낸 정보 저장
            float deltaTime = 1.0f / 60.0f; // 서버 틱 레이트 기준 (예: 60fps)

            _position = p->position;
            _velocity = p->velocity;
            _pitch = p->pitch;
            _hp = p->hp;
			_gun_type = p->gun_type;
			_level = p->level;
			_score = p->score;
			_damage = p->damage;

            pkt_sc_object_update packet_update;
            packet_update.header.size = sizeof(packet_update);
            packet_update.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
            packet_update.id = _id;

			packet_update.position = _position;
			packet_update.velocity = _velocity;
			packet_update.pitch = _pitch;
			packet_update.hp = _hp;
			packet_update.gun_type = _gun_type;
			packet_update.level = _level;
			packet_update.score = _score;
			packet_update.damage = _damage;

            for (auto& [id, session] : g_users) {
                if (id != _id)
                    session.do_send(&packet_update);
            }
            break;
        }
        case PKT_TYPE::C_S_STAGE_INFO:
        {
            std::cout << "[RECV][" << _id << "] C_S_STAGE_INFO 요청 수신\n";

            pkt_sc_stage_info packet;
            packet.header.size = sizeof(packet);
            packet.header.type = PKT_TYPE::S_C_STAGE_INFO;
            packet.currentStage = g_current_stage;
            packet.totalStages = g_total_stages;
            //packet.timeLeft = g_stage_time_left;

            for (auto& [_, session] : g_users)
                session.do_send(&packet);

            break;
        }

        default:
            std::cout << "[WARN] Unknown PacketType: " << packet_type << "\n";
            break;
        }
	}

};

void CALLBACK g_send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    OVER_EXP* send_ov = reinterpret_cast<OVER_EXP*>(p_over);
    delete send_ov;
}

void CALLBACK g_recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    auto my_id = reinterpret_cast<SIZEID>(p_over->hEvent);

    if (g_users.find(my_id) == g_users.end()) {
		std::cout << "[ERROR] Invalid session ID: " << my_id << "\n";
        return;
    }
    g_users[my_id].recv_callback(num_bytes);

}

void TryDamagePlayer(ZombieAI& zombie)
{
    constexpr float attackRange = 1.5f;

    for (auto& [id, session] : g_users)
    {
        if (session._obj_type != ObjectType::PLAYER) continue;
        if (session._hp == 0) continue;

        Vec3 diff = session._position - zombie.GetPosition();
        if (diff.LengthSquared() < attackRange * attackRange)
        {
            session._hp = (session._hp > ZOMBIE_DAMAGE) ? session._hp - ZOMBIE_DAMAGE : 0;

            DEBUG_LOG("[ZOMBIE] ID = " << zombie.GetID()
                << " attacked player ID = " << id
                << " -> HP = " << session._hp);

            // 상태 갱신 전송
            pkt_sc_object_add packet;
            packet.header.size = sizeof(packet);
            packet.header.type = PKT_TYPE::S_C_OBJECT_ADD;
            packet.id = session._id;
            packet.obj_type = session._obj_type;
            packet.skin_type = session._skin_type;
            strcpy_s(packet.name, session._name.c_str());
            packet.startposition = session._position;
            packet.hp = session._hp;

            packet.gun_type = session._gun_type;

            for (auto& [_, s] : g_users)
                s.do_send(&packet);

            if (session._hp == 0) {
                // 죽음 처리 또는 패킷 전송
            }
        }
    }
}

void ZombieAIThread() {
    while (serverRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 1. 플레이어 위치 수집
        std::vector<Vec3> playerPositions;
        for (auto& [id, session] : g_users) {
            if (session._obj_type == ObjectType::PLAYER)
                playerPositions.push_back(session._position);
        }
        // [2] 패킷 준비
        pkt_sc_object_update packet;
        packet.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;

        // [3] 한 번에 모든 좀비 처리
        for (auto* zombie : g_zombies) {
            zombie->Update(playerPositions, g_zombies);

            if (zombie->GetActionType() == ATTACK)
                TryDamagePlayer(*zombie);
            pkt_sc_object_update p;
            p.header.size = sizeof(p);
            p.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
            p.id = zombie->GetID();
            p.position = zombie->GetPosition();
            p.velocity = Vec3(0, 0, 0);
            p.pitch = 0.0f;
            p.hp = zombie->GetHP();
            p.gun_type = GunType::BULLET_MAX;
            p.act_type = zombie->GetActionType();  

            p.level = 0;
            p.score = 0;
            p.damage = ZOMBIE_DAMAGE;

            for (auto& [_, session] : g_users)
                session.do_send(&p);

            zombie->ClearDirty(); 
        }
    }
}

void SpawnZombies(int count) {
    for (int i = 0; i < count; ++i) {
        auto [x, z] = GetRandomPosition(g_map);
        ZombieAI* zombie = new ZombieAI(g_map, 10000 + i);
        zombie->SetPosition((float)x, (float)z);
        zombie->SetHP(ZOMBIE_HP);
        g_zombies.push_back(zombie);

        // 좀비 정보를 모든 플레이어에게 전송
        pkt_sc_object_add p;
        p.header.size = sizeof(p);
        p.header.type = PKT_TYPE::S_C_OBJECT_ADD;

        p.id = zombie->GetID();
        p.obj_type = ObjectType::ZOMBIE;
        p.skin_type = 0;
        strcpy_s(p.name, "Zombie");
        p.startposition = zombie->GetPosition();
        p.hp = zombie->GetHP();
        p.gun_type = GunType::BULLET_MAX;

        for (auto& [id, session] : g_users)
            session.do_send(&p);
    }
}


void serverControl() {
    while (true) {
        char cmd;
        std::cin >> cmd;
        if (cmd == 'q') {
            std::cout << "서버 종료 명령\n";
            serverRunning = false;
            break;
        }
    }
}

int main() {

    std::wcout.imbue(std::locale("korean")); 

    g_map = LoadMapBin("../../Map/Node/obstacle_mask.bin");

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        error_display("WSAStartup failed", WSAGetLastError());
    else
        std::cout << "WSAStartup Success\n";

    SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (s_socket == INVALID_SOCKET)
        error_display("Socket creation failed", WSAGetLastError()); \
    else
        std::cout << "Socket creation Success\n";

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_NUM);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind (s_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
        error_display("Bind failed", WSAGetLastError());
    else { std::cout << "Bind Success\n"; }

    if (listen (s_socket, SOMAXCONN) == SOCKET_ERROR)
        error_display("Listen failed", WSAGetLastError());
    else { std::cout << "Listen Success\n"; }


    std::cout << "Zombie Strike 3D Server running on port: " << PORT_NUM << "\n";

    std::thread(serverControl).detach();
    std::thread(ZombieAIThread).detach();
    SpawnZombies(MAX_ZOMBIE_COUNT);

    SIZEID clientId = 0;
    INT serverAddr_size = sizeof(SOCKADDR_IN);

    while (serverRunning) {
        auto c_socket = WSAAccept(s_socket,reinterpret_cast<sockaddr*>(&serverAddr), &serverAddr_size, NULL, NULL);
        if (c_socket == INVALID_SOCKET) {
            std::cout << "Accept failed\n";
            continue;
        }

        g_users.try_emplace(clientId, clientId, c_socket);
        clientId++;
    }

    std::cout << "서버 종료 중...\n";
    closesocket(s_socket);
    WSACleanup();
}
