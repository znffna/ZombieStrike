#pragma once
#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include "../../protocol.h"
#include "NetworkClient.h"

#pragma comment (lib, "WS2_32.LIB")

constexpr const char* LOOPBACK_IP = "127.0.0.1";
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
private:
    ExtentOverlapped recv_over;
    //WSAOVERLAPPED recv_over;
    SOCKET c_socket;
    //char recv_buffer[1024];
    //WSABUF recv_wsabuf;
	DWORD remain_bytes = 0;

    bool is_connect = false; // 종료 여부
	bool is_running = false; // recv loop이 수행중인지 확인

    std::atomic_bool m_sentLoadingFinish{ false }; // - 로딩 완료 패킷 중복 전송 방지 플래그

    std::thread recvThread;

    void SetConnect(bool connect = true) { is_connect = connect; }
    void SetRunning(bool running = false) { is_running = running; }

public:
    NetworkingClient();  
    NetworkingClient(const NetworkingClient&) = delete; // 복사 생성자 삭제  
    NetworkingClient& operator=(const NetworkingClient&) = delete; // 복사 할당 연산자 삭제  

    static NetworkingClient& Instance() {  
        static NetworkingClient instance;  
        return instance;  
    }  

	bool IsConnect() const { return is_connect; }
	bool IsRunning() const { return is_running; }

	// 소켓 초기화 및 서버 연결
    void CheckSocket(); // 소켓 상태 확인

	// Initialize / Destroy
    bool Connect();   // 소켓 초기화 및 서버 연결
    void Logout(); // 소켓의 연결 종료

	// IP 주소 로드 
	std::string LoadIPAddress() const; // 
    std::string LoadIPAddressFromFile(const std::string& filename) const;

    bool StartRecvLoop(); // recv loop 시작
    void error_display(const char* msg, int err_no);

    void recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag);
    void send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag);
    
    void ProcessPacket(PacketHeader* recv_p);
    void recv_packet();
    void send_packet(char* packet);

    // SendPacket (테스트로 구현)
    void SendLoginPacket(std::string& name);

    // 로딩 완료 신호 1회 전송
    void SendLoadingFinishPacket();
    // 재접속 대비 리셋
    void ResetLoadingFinishFlag() { m_sentLoadingFinish.store(false, std::memory_order_release); }

    // 동기적 별도스레드 recv_Loop 제작
	std::mutex write_lock; // 쓰기 작업을 위한 뮤텍스
	std::vector<RawPacket> read_queue; // 실제 패킷처리를 하기위해 제공할 큐(직접 다루는것은 Scene에서)
	std::vector<RawPacket> write_queue; // 수신된 패킷을 저장하는 큐

    void recv_loop();    
    void StorePacket(PacketHeader* pktHeader, DWORD size);
	std::vector<RawPacket>& GetReadQueue();

    void ProcessReadQueuePacket();
};