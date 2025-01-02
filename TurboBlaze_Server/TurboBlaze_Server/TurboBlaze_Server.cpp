#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include "../../protocol.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;

// COMP_TYPE 열거형 정의 : IOCP에서 사용할 COMP_TYPE 열거형
enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };

// OVER_EXP 클래스 정의 : WSAOVERLAPPED 구조체와 WSABUF 구조체를 멤버로 가지는 클래스
class OVER_EXP {
public:
	WSAOVERLAPPED _over;		// WSAOVERLAPPED 구조체 : 첫번째 멤버변수로 설정하여 _over 멤버변수를 OVER_EXP 클래스의 시작 주소로 설정
	WSABUF _wsabuf;				// WSABUF 구조체 : 데이터 버퍼의 주소와 크기를 저장하는 구조체
	char _send_buf[BUF_SIZE];	// 패킷을 저장하는 버퍼
	COMP_TYPE _comp_type;		// COMP_TYPE 열거형 변수
	OVER_EXP()
	{
		// 생성자 : Recv용 OVER_EXP 객체를 생성할 때 사용
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		// 생성자 : Send용 OVER_EXP 객체를 생성할 때 사용
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};

// S_STATE 열거형 정의 : 세션의 상태를 나타내는 열거형
enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };

// 세션 클래스 정의 : 세션의 정보(즉, 클라이언트 정보)를 저장하는 클래스
class SESSION {
	OVER_EXP _recv_over; // Recv용 OVER_EXP 객체 : 이 역시 OVERLAPPED 구조체를 첫번째 멤버변수로 가지므로 SESSION 클래스의 시작 주소로 설정

public:
	mutex _s_lock;		// S_STATE 객체에 대한 뮤텍스
	S_STATE _state;		// 세션의 상태 - 멀티 스레드 환경에서 mutex를 사용하여 상태를 변경
	int _id;			// 세션의 ID
	SOCKET _socket;		// 세션의 소켓
	int		_prev_remain;  // 이전에 처리하지 못한 패킷 데이터의 크기 : 패킷 조립을 위해 사용
public:
	SESSION() // 생성자 : 멤버변수 초기화
	{
		_id = -1;
		_socket = 0;
		_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	// do_recv 함수 : Recv 함수를 호출하여 클라이언트로부터 데이터를 받는 함수
	void do_recv() 
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
			&_recv_over._over, 0);
	}

	// do_send 함수 : Send 함수를 호출하여 클라이언트로 데이터를 보내는 함수
	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}
	
};

// 전역 변수 선언 : 클라이언트 정보를 저장하는 clients 배열
array<SESSION, MAX_USER> clients;

// 전역 변수 선언 : 서버 소켓과 클라이언트 소켓, AcceptEx 함수에서 사용할 OVER_EXP 객체
SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

// get_new_client_id 함수 : 새로운 클라이언트 ID를 반환하는 함수 - ST_FREE 상태인 클라이언트를 찾아서 ID를 반환
// 이는 클라이언트의 세션을 배열로써 관리하기 때문에 사용
int get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ clients[i]._s_lock };
		if (clients[i]._state == ST_FREE)
			return i;
	}
	return -1;
}

// process_packet 함수 : 패킷 조립이후 패킷을 처리하는 함수
void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
		// TODO : case 문을 사용하여 패킷 type에 따라 처리
	}
}

void disconnect(int c_id)
{
	for (auto& cl : clients)
	{
		// TODO : 클라이언트의 종료 처리 이전 다른 클라이언트에 접속종료를 알리는 패킷을 보내야 함.
		// SC_DISCONNECT 패킷을 보내는 함수를 구현해야 함.
		// SC_DISCONNECT 패킷은 protocol.h에 정의되어 있음.(또는 해야 함)
		// for(auto& cl : clients) { cl.do_send(SC_DISCONNECT);}
	}
	
	// 클라이언트 소켓을 닫고, 클라이언트의 상태를 ST_FREE로 변경
	closesocket(clients[c_id]._socket);

	lock_guard<mutex> ll(clients[c_id]._s_lock);
	clients[c_id]._state = ST_FREE;
}

void worker_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;

		// GetQueuedCompletionStatus 함수를 사용하여 IOCP에서 발생한 이벤트를 가져옴 : retrun 값이 FALSE이면 에러
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);

		// OVER_EXP 객체로 변환 : OVERLAPPED 구조체의 주소가 OVER_EXP 객체의 시작 주소로 설정되어 있음
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			// 에러 처리
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				// 콘솔에 GetQueuedCompletionStatus 에러 메시지 출력
				cout << "GQCS Error on client[" << key << "]\n"; 

				// 클라이언트 소켓 종료 및 클라이언트 상태 변경
				disconnect(static_cast<int>(key));

				// OP_SEND일 경우 OVER_EXP 객체 삭제 : OP_SEND일 경우에만 OVER_EXP 객체를 동적할당하여 사용하므로
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		// 클라이언트 소켓이 종료된 경우 또는 받은 데이터가 없는 경우 -> 해당 세션을 종료시킨다.
		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key));

			// OP_SEND일 경우 OVER_EXP 객체 삭제 : OP_SEND일 경우에만 OVER_EXP 객체를 동적할당하여 사용하므로
			if (ex_over->_comp_type == OP_SEND) delete ex_over;
			continue;
		}

		// 이벤트의 종류에 따라 처리
		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			// 새로운 클라이언트 세션을 생성
			int client_id = get_new_client_id();
			if (client_id != -1) {
				{
					lock_guard<mutex> ll(clients[client_id]._s_lock);
					clients[client_id]._state = ST_ALLOC;
				}
				// 클라이언트 정보 설정
				clients[client_id]._id = client_id;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = g_c_socket;

				// 클라이언트 소켓을 IOCP와 연결
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, client_id, 0);

				// Recv 함수 호출
				clients[client_id].do_recv();

				// 새로운 클라이언트를 받기 위해 새로운 클라이언트 소켓 생성
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				// 클라이언트가 MAX_USER를 초과한 경우 : 콘솔에 출력
				cout << "Max user exceeded.\n";
			}

			// AcceptEx 함수 호출 전에 OVER_EXP 객체 초기화
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);

			// AcceptEx 함수 호출 - 이번 이벤트가 AcceptEX 이벤트이므로 다음 AcceptEx 이벤트를 위해 다시 호출
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			// 이번에 받은 데이터의 크기와 이전에 처리하지 못한 데이터의 크기를 합하여 remain_data에 저장
			int remain_data = num_bytes + clients[key]._prev_remain;

			// 패킷 조립을 위한 포인터 설정 : 패킷 조립을 위해 이전에 처리하지 못한 데이터의 크기를 이용
			char* p = ex_over->_send_buf;

			// 패킷 조립
			while (remain_data > 0) {
				// 패킷의 HEADER 부분을 이용하여 패킷의 크기를 알아냄 : 이 크기는 패킷 HEADER의 정의에 따라 달라질 수 있음(예제는 char 크기)
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					// 패킷을 조립할 수 있는 경우 : process_packet 함수 호출
					process_packet(static_cast<int>(key), p);
					// p를 처리하지 못한 데이터의 시작 위치로 설정
					p = p + packet_size; 
					// 남은 데이터의 크기 갱신
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			// 처리하지 못한 데이터의 크기를 세션의 _prev_remain에 저장
			clients[key]._prev_remain = remain_data;
			// 처리하지 못한 데이터를 _send_buf에 복사
			if (remain_data > 0) {
				// remain_data 크기만큼의 데이터를 _send_buf에 복사
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			// 다시 Recv 함수 호출
			clients[key].do_recv();
			break;
		}
		case OP_SEND:
			// Send 이벤트인 경우 Send가 정상적으로 보내졌음을 의미 : 동적 할당했던 OVER_EXP 객체 삭제
			delete ex_over;
			break;
		}
	}
}

int main()
{
	HANDLE h_iocp;
	WSADATA WSAData;

	// Initialize Winsock
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	// Bind
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

	// Listen
	listen(g_s_socket, SOMAXCONN);

	// Address size
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);

	// Create IOCP
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);

	// Associate with IOCP
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);

	// AcceptEx
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;

	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	////////////////////////////////////////////////////////////////////////////////////////
	// AcceptEx 설명
	// https://docs.microsoft.com/en-us/windows/win32/api/mswsock/nf-mswsock-acceptex
	// 아래는 addr_size + 16 의 16의 출처
	// [in] dwLocalAddressLength
	// The number of bytes reserved for the local address information.This value must be at least 16 bytes more than the maximum address length for the transport protocol in use.
	// 번역 : 로컬 주소 정보를 위해 예약된 바이트 수입니다. 이 값은 사용 중인 전송 프로토콜의 최대 주소 길이보다 적어도 16 바이트 이상이어야합니다.
	//
	// [in] dwRemoteAddressLength
	// The number of bytes reserved for the remote address information.This value must be at least 16 bytes more than the maximum address length for the transport protocol in use.
	// 번역 : 원격 주소 정보를 위해 예약된 바이트 수입니다. 이 값은 사용 중인 전송 프로토콜의 최대 주소 길이보다 적어도 16 바이트 이상이어야합니다.
	////////////////////////////////////////////////////////////////////////////////////////

	// 현재 시스템에서 사용 가능한 스레드 수를 가져와서 num_threads 변수에 저장
	int num_threads = std::thread::hardware_concurrency();

	// Worker Threads
	vector <thread> worker_threads;
	// num_threads 만큼 스레드를 생성하고 worker_thread 함수를 실행
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);

	// Server의 종료를 대기
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);

	// Clean up
	WSACleanup();
}
