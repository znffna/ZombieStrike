#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include "../../protocol.h"
#include "NetworkClient.h"

#pragma comment (lib, "WS2_32.LIB")

extern std::string SERVER_IP;
constexpr const char* LOOPBACK_IP = "127.0.0.1";
#define USSING_IP LOOPBACK_IP

extern bool g_bNetworkDebugMode;

// 클라이언트 네트워크 클래스
// 소켓을 사용하여 서버와 통신하는 기능을 포함
// MultiSCene에서 생성되어 WSARecv와 WSASend를 이용한 callback을 통한 로직 구현.

extern std::string GetPacketName(PKT_TYPE packetType);

class NetworkingClient;

class ExtentOverlapped {
public:
	WSAOVERLAPPED _overlapped; // overlapped의 주소가 곧 SendOverlapped의 주소
    char _buffer[1024];
	WSABUF _wsabuf;
    NetworkingClient* _owner;

    ExtentOverlapped() {
		ZeroMemory(&_overlapped, sizeof(_overlapped));
		ZeroMemory(_buffer, sizeof(_buffer));
		ZeroMemory(&_wsabuf, sizeof(_wsabuf));
        _owner = nullptr;
	}

    // recv용 생성자
    ExtentOverlapped(NetworkingClient* owner) {
        ZeroMemory(&_overlapped, sizeof(_overlapped));
        _wsabuf.buf = _buffer;
        _wsabuf.len = sizeof(_buffer);
        _owner = owner;
    }

    // send용 생성자
    ExtentOverlapped(char* packet, NetworkingClient* owner = nullptr) {
        ZeroMemory(&_overlapped, sizeof(_overlapped));
        _wsabuf.buf = _buffer;
        _wsabuf.len = *(reinterpret_cast<SIZE2*>(packet));
        memcpy(_buffer, packet, _wsabuf.len);
        _owner = owner;
    }
};

class COnlineScene;

void CALLBACK g_recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag);
void CALLBACK g_send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag);

extern std::string GetPacketName(PKT_TYPE packetType);

class NetworkingClient {
public:
    ExtentOverlapped recv_over;
    //WSAOVERLAPPED recv_over;
    SOCKET c_socket;
    //char recv_buffer[1024];
    //WSABUF recv_wsabuf;
	DWORD remain_bytes = 0;

    bool is_connect = false; // 종료 여부
	bool is_running = false; // recv loop이 수행중인지 확인

    COnlineScene* m_pScene; // Scene 포인터
public:
	NetworkingClient(COnlineScene* pScene);

	// setter / getter
    void SetConnect(bool connect) { { std::string debugOutput = "Connect = "; debugOutput += connect ? "True" : "False"; debugOutput += "\n"; OutputDebugStringA(debugOutput.c_str()); } is_connect = connect; }
	bool IsConnect() { return is_connect; }

	void SetRunning(bool running) { is_running = running; }
	bool IsRunning() { return is_running; }

	void CheckSocket(); // 소켓 상태 확인

	// 소켓 초기화 및 서버 연결

	bool Connect();   // 소켓 초기화 및 서버 연결
	bool StartRecvLoop(); // recv loop 시작
    void Logout(); // Scene의 종료시 호출하도록 구현할 것
    void error_display(const char* msg, int err_no);

    void recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag);
    void send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag);
    
    void ProcessPacket(PacketHeader* recv_p);
    void recv_packet();
    void send_packet(char* packet);

    // SendPacket (테스트로 구현)
    void SendLoginPacket(std::string& name);

    std::string chooseServerIP() {
        std::cout << "\n===== 서버 접속 방법 선택 =====\n";
        std::cout << "1. 직접 입력한 서버 IP (192. ...)\n";
        std::cout << "2. 루프백 IP (" << LOOPBACK_IP << ") 로 접속\n";
        std::cout << "==============================" << std::endl;
        std::cout << "선택 (1/2): ";

        int choice;
        std::cin >> choice;

        if (choice == 1) {
            std::string ip;
            std::cout << "서버 IP 입력: ";
            std::cin >> ip;
            return ip;
        }
        else if (choice == 2) {
            std::cout << (std::string("루프백 IP (") + LOOPBACK_IP + ") 로 접속");
            return LOOPBACK_IP;
        }
        else {
            std::cout << ("잘못된 선택!");
        }
        return "";
    }
};