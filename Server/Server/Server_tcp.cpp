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
    uint8_t GunType; // 총 종류
    float bulletPos[3];
    float bulletDir[3];
};
struct Zombie {
    ZombieInfo zombie_info;
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
    uint16_t        _buffer[1024];
    WSABUF          _wsabuf[1];
};

class SESSION;

std::unordered_map<uint8_t, SESSION> g_users;

class SESSION {
public:
    SOCKET          _c_socket;
    uint8_t        _id;

    OVER_EXP        _recv_over{OP_RECV};
    int             _remained = 0;

    float           _position[3];
    std::string     _name;
    uint8_t         _hp;
    uint16_t        _score;
    uint8_t         _level;
    uint8_t         _skin_type;


    void do_recv() {
        DWORD flags = 0;
        ZeroMemory(&_recv_over._over, sizeof(_recv_over._over));
        _recv_over._over.hEvent = reinterpret_cast<HANDLE>(_id); // 세션 ID를 이벤트 핸들로 사용

        _recv_over._wsabuf[0].buf = reinterpret_cast<CHAR*>(_recv_over._buffer) + _remained;	//prev_remain 부분에 이어서 수신하기 위해서
        _recv_over._wsabuf[0].len = sizeof(_recv_over._buffer) - _remained;

        int ret = WSARecv(_c_socket, _recv_over._wsabuf, 1, 0, &flags, &_recv_over._over, g_recv_callback);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cout << "WSARecv failed\n";
        }
    }

public: 
    SESSION() {
        std::cout << "DEFAULT SESSION CONSTRUCTOR CALLED!!\n";
        exit(-1);
    }
	SESSION(uint32_t session_id, SOCKET s) : _id(session_id), _c_socket(s) 
    {
        _recv_over._wsabuf[0].len = sizeof(_recv_over._buffer);
        _recv_over._wsabuf[0].buf = reinterpret_cast<CHAR* >(_recv_over._buffer);

        _recv_over._over.hEvent = reinterpret_cast<HANDLE>(session_id);

        _remained = 0;
		do_recv();
	}
    ~SESSION() 
    {
		pkt_sc_player_remove p;
		p.header.size = sizeof(p);
		p.header.type = PKT_TYPE::S_C_PLAYER_REMOVE;
		p.objectId = _id;
        for (auto& u : g_users) {
			if (u.first != _id) // 나를 제외한 상대방에게 알리고
				u.second.do_send(&p);
        }
		closesocket(_c_socket);
    }

    void recv_callback(int num_bytes) {
        // ----- 패킷 조립 시작 -----
        uint16_t* p = _recv_over._buffer;
        int total = _remained + num_bytes;

        // 앞에 남은 데이터 있으면 이어붙임
        if (_remained > 0)
            memmove(p, p + _remained, num_bytes);

        uint16_t* packet = p;
        int offset = 0;

        while (p + 1 <= p + total) {
            uint16_t packetSize = *p;

            if (p + packetSize > p + total) break; // 아직 패킷 완성이 안 됨

            std::cout << "[RECV][" << _id << "] packetSize = " << (int)packetSize << ", Raw = ";
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
        uint16_t packet_size = reinterpret_cast<uint16_t*>(buff)[0];
        memcpy(send_ov->_buffer, buff, packet_size);
        send_ov->_wsabuf[0].len = packet_size;
        DWORD size_sent;

		std::cout << "[do_send] size = " << packet_size << ", type = " << (int)reinterpret_cast<uint16_t*>(buff)[1] << std::endl;
        int ret = WSASend(_c_socket, send_ov->_wsabuf, 1, &size_sent, 0, &(send_ov->_over), g_send_callback);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cout << "WSASend failed\n";
        }
    }

	void process_packet(uint16_t* packet) {

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
            _name = loginPacket->name;
            _skin_type = loginPacket->skin_type;
            _position[0] = START_POSITIONS[IN_g_player_n][0];
            _position[1] = START_POSITIONS[IN_g_player_n][1];
            _position[2] = START_POSITIONS[IN_g_player_n][2];
            _hp = PLAYER_HP;
            _level = 1;
            _score = 0;
            IN_g_player_n++;
            
			std::cout << "[process_packet][RECV][" << _id << "] Login: " << _name << "\n";
			std::cout << "[process_packet][RECV][" << _id << "] Skin Type: " << (int)_skin_type << "\n";
            send_player_info_packet();

			pkt_sc_player_add p_Add_P;
			p_Add_P.header.size = sizeof(p_Add_P);
			p_Add_P.header.type = PKT_TYPE::S_C_PLAYER_ADD;
			p_Add_P.objectId = _id;
			strcpy_s(p_Add_P.name, _name.c_str());
			p_Add_P.skin_type = _skin_type;
			p_Add_P.position[0] = _position[0];
			p_Add_P.position[1] = _position[1];
			p_Add_P.position[2] = _position[2];
			p_Add_P.hp = _hp;
			p_Add_P.level = _level;
			p_Add_P.score = _score;


            for (auto& u : g_users) {
                if (u.first != _id) // 나를 제외한 상대방에게 알리고
                    u.second.do_send(&p_Add_P);
            }
			for (auto& u : g_users) {
				if (u.first != _id) {// 나를 제외한 상대방의 정보를 나에게 알리고
					pkt_sc_player_add p_Add_P;
                    p_Add_P.header.size = sizeof(p_Add_P);
                    p_Add_P.header.type = PKT_TYPE::S_C_PLAYER_ADD;
                    p_Add_P.objectId = u.first;
					strcpy_s(p_Add_P.name, u.second._name.c_str());
                    p_Add_P.skin_type = u.second._skin_type;
                    p_Add_P.position[0] = u.second._position[0];
                    p_Add_P.position[1] = u.second._position[1];
                    p_Add_P.position[2] = u.second._position[2];
                    p_Add_P.hp = u.second._hp;
                    p_Add_P.level = u.second._level;
                    p_Add_P.score = u.second._score;
					do_send(&p_Add_P);
				}
			}

            break;
        }
        case PKT_TYPE::C_S_MOVE:
        {
			pkt_cs_move* movePacket = reinterpret_cast<pkt_cs_move*>(packet);

            float deltaTime = 1.0f / 60.0f; // 서버 틱 레이트 기준 (예: 60fps)
            // 이동 거리 = 방향 * 속도 * 시간
            _position[0] += movePacket->direction[0] * movePacket->speed * deltaTime;
            _position[1] += movePacket->direction[1] * movePacket->speed * deltaTime;
            _position[2] += movePacket->direction[2] * movePacket->speed * deltaTime;

            send_player_update();
            break;
        }

        case PKT_TYPE::C_S_SHOOT:
            break;
        default:
            std::cout << "[WARN] Unknown PacketType: " << packet_type << "\n";
            break;
        }
	}

    void send_player_info_packet() {
		pkt_sc_player_add p;
		p.header.size = sizeof(p);
		p.header.type = PKT_TYPE::S_C_PLAYER_INFO;
		p.objectId = _id;
		p.skin_type = _skin_type;
        p.position[0] = _position[0];
		p.position[1] = _position[1];
		p.position[2] = _position[2];
		p.hp = _hp;
		p.level = _level;
		p.score = _score;
        std::cout << "[SEND]" << "size: " << p.header.size << ", type: " << (int)p.header.type << std::endl;
        do_send(&p);
    }
    void send_player_move() {
        pkt_sc_player_move p;
		p.header.size = sizeof(p);
		p.header.type = PKT_TYPE::S_C_PLAYER_MOVE;
		p.objectId = _id;
		p.position[0] = _position[0];
		p.position[1] = _position[1];
		p.position[2] = _position[2];
		do_send(&p);
    }

    void send_player_update() {
        pkt_sc_player_update p;
		p.header.size = sizeof(p);
		p.header.type = PKT_TYPE::S_C_PLAYER_UPDATE;
		p.objectId = _id;
		p.position[0] = _position[0];
		p.position[1] = _position[1];
		p.position[2] = _position[2];
		p.hp = _hp;
		p.level = _level;
		p.score = _score;
        do_send(&p);
    }

};
void CALLBACK g_send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    OVER_EXP* send_ov = reinterpret_cast<OVER_EXP*>(p_over);
    delete send_ov;
}

void CALLBACK g_recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    auto my_id = reinterpret_cast<uint32_t>(p_over->hEvent);

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

	uint32_t clientId = 0;

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
