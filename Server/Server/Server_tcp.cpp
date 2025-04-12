#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <queue>
#include <set>
#include <map>
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


struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Quaternion {
    float x = 0, y = 0, z = 0, w = 1;
};
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
    unsigned char   _buffer[1024];
    WSABUF          _wsabuf[1];
};

class SESSION;
std::unordered_map<uint32_t, SESSION> g_users;

class SESSION {
public:
    SOCKET          _c_socket;
    uint32_t        _id;

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
		PKT_SC_PLAYER_REMOVE p;
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
        unsigned char* p = _recv_over._buffer;
        int total = _remained + num_bytes;

        // 앞에 남은 데이터 있으면 이어붙임
        if (_remained > 0)
            memmove(p, p + _remained, num_bytes);

        unsigned char* packet = p;
        int offset = 0;

        while (p + 1 <= p + total) {
            unsigned char packetSize = *p;

            if (p + packetSize > p + total) break; // 아직 패킷 완성이 안 됨

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


	void process_packet(unsigned char* packet) {
		const unsigned char packet_type = packet[1];
        //_recv_over._buffer[] = 0;

        const unsigned char packet_type = packet[1];
        if (packet_type == 0) {
            std::cout << "[ERROR] Invalid Packet Type\n";
            return;
        }

        short playerNum = 0;
        switch (packet_type) {
        case ::PKT_TYPE::C_S_LOGIN:
        {
            PKT_CS_LOGIN* loginPacket = reinterpret_cast<PKT_CS_LOGIN*>(packet);
            _name = loginPacket->name;
            _skin_type = loginPacket->skin_type;
            _position[0] = START_POSITIONS[playerNum][0];
            _position[1] = START_POSITIONS[playerNum][1];
            _position[2] = START_POSITIONS[playerNum][2];
            _hp = PLAYER_HP;
            _level = 1;
            _score = 0;
            playerNum++;
            send_player_info_packet();

            for (auto& u : g_users) {
                if (u.first != _id) // 나를 제외한 상대방에게 알리고
                    u.second.do_send(&loginPacket);
            }

        }
        case PKT_TYPE::C_S_MOVE:
            //Move(this, (PKT_CS_MOVE*)packet);
            break;
        case PKT_TYPE::C_S_SHOOT:

            break;
        default:
            std::cout << "[WARN] Unknown PacketType: " << packet_type << "\n";
            break;
        }

	}

    void do_send(void *buff) {
        OVER_EXP* send_ov = new OVER_EXP(OP_SEND);
        unsigned char packet_size = reinterpret_cast<unsigned char*>(buff)[0];
        memcpy(send_ov->_buffer, buff, packet_size);
        send_ov->_wsabuf[0].len = packet_size;
        DWORD size_sent;

        int ret = WSASend(_c_socket, send_ov->_wsabuf, 1, &size_sent, 0, &(send_ov->_over), g_send_callback);
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			std::cout << "WSASend failed\n";
		}
    }

    void send_player_info_packet() {
		PKT_SC_PLAYER_ADD p;
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
        do_send(&p);
    }

    void send_player_update() {
		PKT_SC_PLAYER_UPDATE p;
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

    SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
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

        SOCKET clientSocket = WSAAccept(s_socket, reinterpret_cast<sockaddr*>(&serverAddr), &serverAddr_size, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept failed\n";
            continue;
        }serverControl;

        auto c_socket = WSAAccept(s_socket,reinterpret_cast<sockaddr*>(&serverAddr_size), &serverAddr_size, NULL, NULL);
        g_users.try_emplace(clientId, clientId, c_socket);

        clientId++;
    }

    std::cout << "서버 종료 중...\n";
    closesocket(s_socket);
    WSACleanup();
}
