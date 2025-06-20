#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <queue>
#include <mutex>
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

struct RawPacket {
	std::unique_ptr<char[]> buffer;

    RawPacket(PacketHeader* header, DWORD size) {
		buffer = std::make_unique<char[]>(size);
		memcpy(buffer.get(), header, size);
    }

    PacketHeader* header() const {
		return reinterpret_cast<PacketHeader*>(buffer.get());
    }

    template <typename T>
    T* as() {
		return reinterpret_cast<T*>(buffer.get());
	}
};

class ExtentOverlapped {
public:
	WSAOVERLAPPED _overlapped; // overlapped의 주소가 곧 SendOverlapped의 주소
    char _buffer[1024];
	WSABUF _wsabuf;

    // recv용 생성자
    ExtentOverlapped() {
		ZeroMemory(&_overlapped, sizeof(_overlapped));
		ZeroMemory(_buffer, sizeof(_buffer));
		ZeroMemory(&_wsabuf, sizeof(_wsabuf));
	}

    // send용 생성자
    ExtentOverlapped(char* packet) {
        ZeroMemory(&_overlapped, sizeof(_overlapped));
        _wsabuf.buf = _buffer;
        _wsabuf.len = *(reinterpret_cast<SIZE2*>(packet));
        memcpy(_buffer, packet, _wsabuf.len);
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

    std::thread recvThread;

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

    // 동기적 별도스레드 recv_Loop 제작
	std::mutex write_lock; // 쓰기 작업을 위한 뮤텍스
	std::queue<RawPacket> read_queue; // 수신된 패킷을 저장하는 큐
	std::queue<RawPacket> write_queue; // 수신된 패킷을 저장하는 큐

    void recv_loop();    
    void StorePacket(PacketHeader* pktHeader, DWORD size);
	std::queue<RawPacket>& GetReadQueue();

    void ProcessReadQueuePacket();
};