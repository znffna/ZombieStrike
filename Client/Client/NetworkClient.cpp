#include "NetworkClient.h"

#pragma comment (lib, "WS2_32.LIB")

constexpr const char* LOOPBACK_IP = "127.0.0.1";

void NetworkingClient::Init()
{
    WSADATA wsaData;
    auto ret = WSAStartup(MAKEWORD(2, 2), &wsaData);

    c_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (c_socket == INVALID_SOCKET) error_display("소켓 생성 실패", WSAGetLastError());

    sockaddr_in serverAddr{};
    std::string serverIP = chooseServerIP();

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_NUM);

    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    if (WSAConnect(c_socket, reinterpret_cast<sockaddr*>(&serverAddr),
        sizeof(serverAddr), NULL, NULL, NULL, NULL) == SOCKET_ERROR)
        error_display("서버 연결 실패", WSAGetLastError());

    std::cout << "서버 연결 성공\n";

    // 예시,
    std::string name{ "Client" };

    // 1. 로그인 패킷 전송
    SendLoginPacket(name);

    // 수신 등록
    recv_packet();

    // 2. 이동 패킷 전송 (예시, 이 처리는 실제 게임의 Input시 호출됨.)
    SendMovePacket();
}

void NetworkingClient::Logout()
{
    closesocket(c_socket);
    WSACleanup();
}

void NetworkingClient::SendMovePacket()
{
    pkt_cs_update u_movePkt{};
    u_movePkt.header.size = sizeof(u_movePkt);
    u_movePkt.header.type = PKT_TYPE::C_S_UPDATE;
    u_movePkt.obj.meta.position = { 0.0f, 0.0f, 0.0f }; // 현재 위치
    u_movePkt.obj.meta.direction = { 1.0f, 0.0f, 0.0f }; // 이동 방향
    u_movePkt.obj.meta.speed = 5.0f; // 이동 속도
    u_movePkt.obj.meta.hp = 100; // 체력
    u_movePkt.obj.gun_type = GunType::BULLET_PISTOL; // 총 종류
    u_movePkt.obj.level = 1; // 레벨
    u_movePkt.obj.score = 0; // 점수
    u_movePkt.obj.damage = 0; // 공격력
    u_movePkt.obj.act_type = ActionType::NONE; // 행동 타입

    send_packet((char*)&u_movePkt);
}

void NetworkingClient::SendLoginPacket(std::string& name)
{
    pkt_cs_login loginPkt{};
    loginPkt.header.size = sizeof(loginPkt);
    loginPkt.header.type = PKT_TYPE::C_S_LOGIN;
    loginPkt.skin_type = 1;
    strcpy_s(loginPkt.name, MAX_NAME_SIZE, name.c_str());
    send_packet((char*)&loginPkt);
}

void NetworkingClient::error_display(const char* msg, int err_no)
{
    WCHAR* lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, err_no, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    std::cout << msg;
    std::wcout << L" 에러 " << lpMsgBuf << std::endl;
    LocalFree(lpMsgBuf);
    exit(1);
}

void NetworkingClient::recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    NetworkingClient* client = reinterpret_cast<NetworkingClient*>(p_over);

    if (err != 0 || num_bytes == 0) {
        client->is_running = false;
        return;
    }

    char* recv_p = client->recv_buffer;
    SIZE2 offset = 0;
    DWORD remain_bytes = client->remain_bytes + num_bytes;

    while (offset < remain_bytes) {
        if (remain_bytes - offset < sizeof(SIZE2)) {
			// 잔여 데이터가 패킷 길이보다 작을 경우
            memcpy(client->recv_buffer, recv_p, remain_bytes - offset);
            break;
        }

        SIZE2 size = *(SIZE2*)(recv_p);     // 패킷 길이

		if (num_bytes - offset < size) {
            // 수신된 패킷이 완전하지 않은 경우
            memcpy(client->recv_buffer, recv_p, remain_bytes - offset);
            break;
		}

        client->ProcessPacket(recv_p);

        offset += size;
        recv_p += size;
    }
    client->remain_bytes = remain_bytes - offset;

    // 다시 수신 등록
    client->recv_packet();
}

void NetworkingClient::send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{

}

void NetworkingClient::ProcessPacket(char* recv_p)  
{  
   PKT_TYPE type = *(PKT_TYPE*)(recv_p + 2); // 패킷 타입  

   switch (type) {  
   case S_C_OBJECT_ADD:  
   {  
       pkt_sc_object_add* addPkt = reinterpret_cast<pkt_sc_object_add*>(recv_p);  
       break;  
   }  
   case S_C_OBJECT_UPDATE:  
   {  
       pkt_sc_object_update* updatePkt = reinterpret_cast<pkt_sc_object_update*>(recv_p);  
       break;  
   }  
   case S_C_OBJECT_REMOVE:  
   {  
       pkt_sc_object_remove* removePkt = reinterpret_cast<pkt_sc_object_remove*>(recv_p);  
       break;  
   }  

   case S_C_STAGE_INFO:  
   {  
       break;  
   }  
   case S_C_SCORE_INFO:  
   {  
       break;  

   }  
   default:  
       break;  
   }  
}


void NetworkingClient::recv_packet()
{
    ZeroMemory(&recv_over, sizeof(recv_over));
    recv_wsabuf.buf = recv_buffer + remain_bytes;
    recv_wsabuf.len = sizeof(recv_buffer) - remain_bytes;
    DWORD recv_flag = 0;
    WSARecv(c_socket, &recv_wsabuf, 1, NULL, &recv_flag, &recv_over, recv_callback);
}

void NetworkingClient::send_packet(char* packet) {
    SendOverlapped* send_over = new SendOverlapped{ packet };
    int ret = WSASend(c_socket, &send_over->wsabuf, 1, 0, 0, &send_over->overlapped, send_callback);
    
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        error_display("WSASend 실패", WSAGetLastError());
    }
}
