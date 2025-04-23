#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include "../../protocol.h"
#include "NetworkClient.h"

#pragma comment (lib, "WS2_32.LIB")

constexpr const char* LOOPBACK_IP = "127.0.0.1";

// 클라이언트 네트워크 클래스
// 소켓을 사용하여 서버와 통신하는 기능을 포함
// MultiSCene에서 생성되어 WSARecv와 WSASend를 이용한 callback을 통한 로직 구현.

class SendOverlapped {
public:
	WSAOVERLAPPED overlapped; // overlapped의 주소가 곧 SendOverlapped의 주소
    SOCKET c_socket;
    char send_buffer[1024];
	WSABUF wsabuf;

	SendOverlapped(char* packet) {
		ZeroMemory(&overlapped, sizeof(overlapped));
		wsabuf.buf = send_buffer;
        wsabuf.len = *(reinterpret_cast<SIZE2*>(packet));
		memcpy(send_buffer, packet, wsabuf.len);
	}
};

class CScene;

class NetworkingClient {
public:
    WSAOVERLAPPED recv_over;
    SOCKET c_socket;
    char recv_buffer[1024];
    WSABUF recv_wsabuf;

	DWORD remain_bytes = 0;

    bool is_running = true; // 종료 여부
	bool is_recvLoopDone = false; // recv loop 종료 여부

	CScene* m_pScene; // Scene 포인터
public:
	NetworkingClient(CScene* pScene);

	void Connect();   // 소켓 초기화 및 서버 연결
    void Logout(); // Scene의 종료시 호출하도록 구현할 것
    void error_display(const char* msg, int err_no);

    static void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag);
    static void CALLBACK send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag);
    
    void ProcessPacket(PacketHeader* recv_p);
    void recv_packet();
    void send_packet(char* packet);

    // SendPacket (테스트로 구현)
    void SendLoginPacket(std::string& name);
    void SendMovePacket();

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