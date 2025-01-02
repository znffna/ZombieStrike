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

// COMP_TYPE ������ ���� : IOCP���� ����� COMP_TYPE ������
enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };

// OVER_EXP Ŭ���� ���� : WSAOVERLAPPED ����ü�� WSABUF ����ü�� ����� ������ Ŭ����
class OVER_EXP {
public:
	WSAOVERLAPPED _over;		// WSAOVERLAPPED ����ü : ù��° ��������� �����Ͽ� _over ��������� OVER_EXP Ŭ������ ���� �ּҷ� ����
	WSABUF _wsabuf;				// WSABUF ����ü : ������ ������ �ּҿ� ũ�⸦ �����ϴ� ����ü
	char _send_buf[BUF_SIZE];	// ��Ŷ�� �����ϴ� ����
	COMP_TYPE _comp_type;		// COMP_TYPE ������ ����
	OVER_EXP()
	{
		// ������ : Recv�� OVER_EXP ��ü�� ������ �� ���
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		// ������ : Send�� OVER_EXP ��ü�� ������ �� ���
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};

// S_STATE ������ ���� : ������ ���¸� ��Ÿ���� ������
enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };

// ���� Ŭ���� ���� : ������ ����(��, Ŭ���̾�Ʈ ����)�� �����ϴ� Ŭ����
class SESSION {
	OVER_EXP _recv_over; // Recv�� OVER_EXP ��ü : �� ���� OVERLAPPED ����ü�� ù��° ��������� �����Ƿ� SESSION Ŭ������ ���� �ּҷ� ����

public:
	mutex _s_lock;		// S_STATE ��ü�� ���� ���ؽ�
	S_STATE _state;		// ������ ���� - ��Ƽ ������ ȯ�濡�� mutex�� ����Ͽ� ���¸� ����
	int _id;			// ������ ID
	SOCKET _socket;		// ������ ����
	int		_prev_remain;  // ������ ó������ ���� ��Ŷ �������� ũ�� : ��Ŷ ������ ���� ���
public:
	SESSION() // ������ : ������� �ʱ�ȭ
	{
		_id = -1;
		_socket = 0;
		_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	// do_recv �Լ� : Recv �Լ��� ȣ���Ͽ� Ŭ���̾�Ʈ�κ��� �����͸� �޴� �Լ�
	void do_recv() 
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
			&_recv_over._over, 0);
	}

	// do_send �Լ� : Send �Լ��� ȣ���Ͽ� Ŭ���̾�Ʈ�� �����͸� ������ �Լ�
	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}
	
};

// ���� ���� ���� : Ŭ���̾�Ʈ ������ �����ϴ� clients �迭
array<SESSION, MAX_USER> clients;

// ���� ���� ���� : ���� ���ϰ� Ŭ���̾�Ʈ ����, AcceptEx �Լ����� ����� OVER_EXP ��ü
SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

// get_new_client_id �Լ� : ���ο� Ŭ���̾�Ʈ ID�� ��ȯ�ϴ� �Լ� - ST_FREE ������ Ŭ���̾�Ʈ�� ã�Ƽ� ID�� ��ȯ
// �̴� Ŭ���̾�Ʈ�� ������ �迭�ν� �����ϱ� ������ ���
int get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ clients[i]._s_lock };
		if (clients[i]._state == ST_FREE)
			return i;
	}
	return -1;
}

// process_packet �Լ� : ��Ŷ �������� ��Ŷ�� ó���ϴ� �Լ�
void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
		// TODO : case ���� ����Ͽ� ��Ŷ type�� ���� ó��
	}
}

void disconnect(int c_id)
{
	for (auto& cl : clients)
	{
		// TODO : Ŭ���̾�Ʈ�� ���� ó�� ���� �ٸ� Ŭ���̾�Ʈ�� �������Ḧ �˸��� ��Ŷ�� ������ ��.
		// SC_DISCONNECT ��Ŷ�� ������ �Լ��� �����ؾ� ��.
		// SC_DISCONNECT ��Ŷ�� protocol.h�� ���ǵǾ� ����.(�Ǵ� �ؾ� ��)
		// for(auto& cl : clients) { cl.do_send(SC_DISCONNECT);}
	}
	
	// Ŭ���̾�Ʈ ������ �ݰ�, Ŭ���̾�Ʈ�� ���¸� ST_FREE�� ����
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

		// GetQueuedCompletionStatus �Լ��� ����Ͽ� IOCP���� �߻��� �̺�Ʈ�� ������ : retrun ���� FALSE�̸� ����
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);

		// OVER_EXP ��ü�� ��ȯ : OVERLAPPED ����ü�� �ּҰ� OVER_EXP ��ü�� ���� �ּҷ� �����Ǿ� ����
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			// ���� ó��
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				// �ֿܼ� GetQueuedCompletionStatus ���� �޽��� ���
				cout << "GQCS Error on client[" << key << "]\n"; 

				// Ŭ���̾�Ʈ ���� ���� �� Ŭ���̾�Ʈ ���� ����
				disconnect(static_cast<int>(key));

				// OP_SEND�� ��� OVER_EXP ��ü ���� : OP_SEND�� ��쿡�� OVER_EXP ��ü�� �����Ҵ��Ͽ� ����ϹǷ�
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		// Ŭ���̾�Ʈ ������ ����� ��� �Ǵ� ���� �����Ͱ� ���� ��� -> �ش� ������ �����Ų��.
		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key));

			// OP_SEND�� ��� OVER_EXP ��ü ���� : OP_SEND�� ��쿡�� OVER_EXP ��ü�� �����Ҵ��Ͽ� ����ϹǷ�
			if (ex_over->_comp_type == OP_SEND) delete ex_over;
			continue;
		}

		// �̺�Ʈ�� ������ ���� ó��
		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			// ���ο� Ŭ���̾�Ʈ ������ ����
			int client_id = get_new_client_id();
			if (client_id != -1) {
				{
					lock_guard<mutex> ll(clients[client_id]._s_lock);
					clients[client_id]._state = ST_ALLOC;
				}
				// Ŭ���̾�Ʈ ���� ����
				clients[client_id]._id = client_id;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = g_c_socket;

				// Ŭ���̾�Ʈ ������ IOCP�� ����
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, client_id, 0);

				// Recv �Լ� ȣ��
				clients[client_id].do_recv();

				// ���ο� Ŭ���̾�Ʈ�� �ޱ� ���� ���ο� Ŭ���̾�Ʈ ���� ����
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				// Ŭ���̾�Ʈ�� MAX_USER�� �ʰ��� ��� : �ֿܼ� ���
				cout << "Max user exceeded.\n";
			}

			// AcceptEx �Լ� ȣ�� ���� OVER_EXP ��ü �ʱ�ȭ
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);

			// AcceptEx �Լ� ȣ�� - �̹� �̺�Ʈ�� AcceptEX �̺�Ʈ�̹Ƿ� ���� AcceptEx �̺�Ʈ�� ���� �ٽ� ȣ��
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			// �̹��� ���� �������� ũ��� ������ ó������ ���� �������� ũ�⸦ ���Ͽ� remain_data�� ����
			int remain_data = num_bytes + clients[key]._prev_remain;

			// ��Ŷ ������ ���� ������ ���� : ��Ŷ ������ ���� ������ ó������ ���� �������� ũ�⸦ �̿�
			char* p = ex_over->_send_buf;

			// ��Ŷ ����
			while (remain_data > 0) {
				// ��Ŷ�� HEADER �κ��� �̿��Ͽ� ��Ŷ�� ũ�⸦ �˾Ƴ� : �� ũ��� ��Ŷ HEADER�� ���ǿ� ���� �޶��� �� ����(������ char ũ��)
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					// ��Ŷ�� ������ �� �ִ� ��� : process_packet �Լ� ȣ��
					process_packet(static_cast<int>(key), p);
					// p�� ó������ ���� �������� ���� ��ġ�� ����
					p = p + packet_size; 
					// ���� �������� ũ�� ����
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			// ó������ ���� �������� ũ�⸦ ������ _prev_remain�� ����
			clients[key]._prev_remain = remain_data;
			// ó������ ���� �����͸� _send_buf�� ����
			if (remain_data > 0) {
				// remain_data ũ�⸸ŭ�� �����͸� _send_buf�� ����
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			// �ٽ� Recv �Լ� ȣ��
			clients[key].do_recv();
			break;
		}
		case OP_SEND:
			// Send �̺�Ʈ�� ��� Send�� ���������� ���������� �ǹ� : ���� �Ҵ��ߴ� OVER_EXP ��ü ����
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
	// AcceptEx ����
	// https://docs.microsoft.com/en-us/windows/win32/api/mswsock/nf-mswsock-acceptex
	// �Ʒ��� addr_size + 16 �� 16�� ��ó
	// [in] dwLocalAddressLength
	// The number of bytes reserved for the local address information.This value must be at least 16 bytes more than the maximum address length for the transport protocol in use.
	// ���� : ���� �ּ� ������ ���� ����� ����Ʈ ���Դϴ�. �� ���� ��� ���� ���� ���������� �ִ� �ּ� ���̺��� ��� 16 ����Ʈ �̻��̾���մϴ�.
	//
	// [in] dwRemoteAddressLength
	// The number of bytes reserved for the remote address information.This value must be at least 16 bytes more than the maximum address length for the transport protocol in use.
	// ���� : ���� �ּ� ������ ���� ����� ����Ʈ ���Դϴ�. �� ���� ��� ���� ���� ���������� �ִ� �ּ� ���̺��� ��� 16 ����Ʈ �̻��̾���մϴ�.
	////////////////////////////////////////////////////////////////////////////////////////

	// ���� �ý��ۿ��� ��� ������ ������ ���� �����ͼ� num_threads ������ ����
	int num_threads = std::thread::hardware_concurrency();

	// Worker Threads
	vector <thread> worker_threads;
	// num_threads ��ŭ �����带 �����ϰ� worker_thread �Լ��� ����
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);

	// Server�� ���Ḧ ���
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);

	// Clean up
	WSACleanup();
}
