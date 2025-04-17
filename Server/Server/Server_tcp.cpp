#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <queue>
#include "../../protocol.h"
#include <print>

#pragma comment(lib, "ws2_32.lib")


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

struct ShootPacket {
    SIZE1 GunType; // 총 종류
    float bulletPos[3];
    float bulletDir[3];
};

struct Zombie {
	SIZEID id;
    Objectfixdata zombieobj;
	ObjectMeta zombiemeta;
    SIZE2 damage;               
    SIZE1 act_type;             
};

std::vector<Zombie*> zombie_st;
std::mutex zombiesMutex;

bool serverRunning = true;

short IN_g_player_n= 0;

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

class SESSION;

std::unordered_map<SIZEID, SESSION> g_users;

class SESSION {
public:
    SOCKET          _c_socket;
    SIZEID          _id;

    OVER_EXP        _recv_over{OP_RECV};
    SIZE2           _remained = 0;

    ObjectType      _obj_type;
    SIZE1           _skin_type;
    std::string     _name;

    Vector3         _position;
    Vector3         _direction;
	float           _speed;
    SIZE2           _hp;
    GunType         _gun_type;     
    SIZE1           _level;
    SIZE2           _score;
    SIZE2           _damage;               
    SIZE1           _act_type;            

    void do_recv() {
        DWORD flags = 0;
        ZeroMemory(&_recv_over._over, sizeof(_recv_over._over));
        _recv_over._over.hEvent = reinterpret_cast<HANDLE>(_id); // 세션 ID를 이벤트 핸들로 사용

        _recv_over._wsabuf[0].buf = reinterpret_cast<CHAR*>(_recv_over._buffer) + _remained;	//prev_remain 부분에 이어서 수신하기 위해서
        _recv_over._wsabuf[0].len = sizeof(_recv_over._buffer) - _remained;

        int ret = WSARecv(_c_socket, _recv_over._wsabuf, 1, 0, &flags, &_recv_over._over, g_recv_callback);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cout << "[ERROR] WSARecv failed for session " << _id << "\n";
            closesocket(_c_socket);
            g_users.erase(_id);
        }
    }

public: 
    SESSION() {
        std::cout << "DEFAULT SESSION CONSTRUCTOR CALLED!!\n";
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
        std::cout << "[SESSION::~SESSION] Removing ID = " << _id << "\n";

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

        // 앞에 남은 데이터 있으면 이어붙임
        if (_remained > 0)
            memmove(p, p + _remained, num_bytes);

        SIZE2* packet = p;
        SIZE3 offset = 0;

        while (p + 1 <= p + total) {
            SIZE2 packetSize = *p;

            if (p + packetSize > p + total) break; // 아직 패킷 완성이 안 됨

            std::cout << "[RECV][" << _id << "] packetSize = " << (SIZE3)packetSize << ", Raw = ";
            for (int i = 0; i < packetSize; ++i)
                printf("%02X ", p[i]);
            std::cout << std::endl;

			process_packet(p);    // 패킷 처리
            p += packetSize;      // 다음 패킷으로 이동
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
        send_ov->_wsabuf[0].len = packet_size;
        DWORD size_sent;

		std::cout << "[do_send] size = " << packet_size << ", type = " << (int)reinterpret_cast<SIZE2*>(buff)[1] << std::endl;
        int ret = WSASend(_c_socket, send_ov->_wsabuf, 1, &size_sent, 0, &(send_ov->_over), g_send_callback);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cout << "WSASend failed\n";
        }
    }
    void send_object_update() {
        pkt_sc_object_update p_update;
        p_update.header.size = sizeof(p_update);
        p_update.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
        p_update.id = _id;
        p_update.obj.meta.position = _position;
        p_update.obj.meta.direction = _direction;
        p_update.obj.meta.speed = _speed;
        p_update.obj.meta.hp = _hp;

        p_update.obj.gun_type = _gun_type;
        p_update.obj.level = _level;
        p_update.obj.score = _score;
        p_update.obj.damage = _damage;
        p_update.obj.act_type = _act_type;
        do_send(&p_update);
    }


	void process_packet(SIZE2* packet) {

		const unsigned char packet_type = packet[1];
        if (packet_type == 0) {
            std::cout << "[ERROR] Invalid Packet Type\n";
            return;
        }
		std::cout << "[RECV][" << _id << "] Packet Type: " << (int)packet_type << "\n";

        switch (packet_type) {
        case ::PKT_TYPE::C_S_LOGIN:
        {
            pkt_cs_login* loginPacket = reinterpret_cast<pkt_cs_login*>(packet);
            _obj_type   = ObjectType::PLAYER;
            _skin_type  = loginPacket->skin_type;
            _name       = loginPacket->name;
            _position   = START_POSITIONS[IN_g_player_n];
            _direction  = { 0.0f,0.0f, 0.0f };
            _speed      = 0.0f;
            _hp         = PLAYER_HP;
			_gun_type   = GunType::BULLET_PISTOL; // 총 종류
            _level      = 1;
            _score      = 0;
            _damage     = 0;
			_act_type   = ActionType::NONE;
            
            IN_g_player_n++;
			std::cout << "[process_packet][RECV][" << (int)_id << "] Login: " << _name << "\n";
			std::cout << "[process_packet][RECV][" << (int)_id << "] Skin Type: " << (int)_skin_type << "\n";
            send_object_update();

			pkt_sc_object_add p_Add_P;
			p_Add_P.header.size = sizeof(p_Add_P);
			p_Add_P.header.type = PKT_TYPE::S_C_OBJECT_ADD;
			p_Add_P.id = _id;
			p_Add_P.fixdata.obj_type = ObjectType::PLAYER;
			p_Add_P.fixdata.skin_type = _skin_type;
            strcpy_s(p_Add_P.fixdata.name, _name.c_str());
			p_Add_P.fixdata.startposition = _position;
			p_Add_P.fixdata.starthp = _hp;
            

            for (auto& u : g_users) {
                if (u.first != _id) // 나를 제외한 상대방에게 알리고
                    u.second.do_send(&p_Add_P);
            }
			for (auto& u : g_users) {
				if (u.first != _id) {// 나를 제외한 상대방의 정보를 나에게 알리고
                    pkt_sc_object_add p_Add_P;
                    p_Add_P.header.size = sizeof(p_Add_P);
                    p_Add_P.header.type = PKT_TYPE::S_C_OBJECT_ADD;

                    p_Add_P.id = u.first;
                    p_Add_P.fixdata.obj_type = ObjectType::PLAYER;
                    p_Add_P.fixdata.skin_type = _skin_type;
                    strcpy_s(p_Add_P.fixdata.name, _name.c_str());
                    p_Add_P.fixdata.startposition = u.second._position;
                    p_Add_P.fixdata.starthp = u.second._hp;
					do_send(&p_Add_P);
				}
			}
            break;
        }
        case PKT_TYPE::C_S_UPDATE:
        {
			pkt_cs_update* updatePacket = reinterpret_cast<pkt_cs_update*>(packet);

            float deltaTime = 1.0f / 60.0f; // 서버 틱 레이트 기준 (예: 60fps)
            // 이동 거리 = 방향 * 속도 * 시간
            _position += updatePacket->obj.meta.direction * updatePacket->obj.meta.speed * deltaTime;
         
            pkt_sc_object_update u_move_p;
			u_move_p.header.size = sizeof(u_move_p);
			u_move_p.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
			u_move_p.id = _id;
			u_move_p.obj.meta.position = _position;
			u_move_p.obj.meta.direction = _direction;
			u_move_p.obj.meta.speed = _speed;
			u_move_p.obj.meta.hp = _hp;
			u_move_p.obj.gun_type = _gun_type;
			u_move_p.obj.level = _level;
			u_move_p.obj.score = _score;
			u_move_p.obj.damage = _damage;
			u_move_p.obj.act_type = _act_type;
			for (auto& u : g_users) {
				u.second.do_send(&u_move_p); // 나포함 모두에게 알림 좌표의 이동을
			}

            break;
        }

        case PKT_TYPE::C_S_SHOOT:
            break;
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

    g_users[my_id].recv_callback(num_bytes);

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

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        error_display("WSAStartup failed", WSAGetLastError());
    else
        std::cout << "WSAStartup 성공\n";

    SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (s_socket == INVALID_SOCKET)
        error_display("Socket creation failed", WSAGetLastError()); \
    else
        std::cout << "Socket creation 성공\n";

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_NUM);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind (s_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
        error_display("Bind failed", WSAGetLastError());
    else { std::cout << "Bind 성공\n"; }

    if (listen (s_socket, SOMAXCONN) == SOCKET_ERROR)
        error_display("Listen failed", WSAGetLastError());
    else { std::cout << "Listen 성공\n"; }

    INT serverAddr_size = sizeof(SOCKADDR_IN);

    std::cout << "Zombie Strike 3D Server running on port: " << PORT_NUM << "\n";

    std::thread(serverControl).detach();

    SIZEID clientId = 0;

    while (serverRunning) {
        auto c_socket = WSAAccept(s_socket,reinterpret_cast<sockaddr*>(&serverAddr), &serverAddr_size, NULL, NULL);
        if (c_socket == INVALID_SOCKET) {
            std::cout << "Accept failed\n";
            continue;
        }serverControl;

        g_users.try_emplace(clientId, clientId, c_socket);
        clientId++;
    }

    std::cout << "서버 종료 중...\n";
    closesocket(s_socket);
    WSACleanup();
}
