#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include "../../protocol.h"

#pragma comment (lib, "WS2_32.LIB")

constexpr const char* LOOPBACK_IP = "127.0.0.1";

SOCKET c_socket;
char recv_buffer[1024];
WSABUF recv_wsabuf[1];
WSAOVERLAPPED recv_over;

bool is_running = true;
bool b_logout = false;
bool recv_ok = true;

void error_display(const char* msg, int err_no)
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

void CALLBACK recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    if (err != 0 || num_bytes == 0) {
        std::cout << "[클라] 연결 끊김 or 에러\n";
        is_running = false;
        return;
    }

    char* recv_p = recv_buffer;
    SIZE2 offset = 0;
    while (offset < num_bytes) {
        SIZE2 size = *(SIZE2*)(recv_p);     // 패킷 길이
        PKT_TYPE type = *(PKT_TYPE*)(recv_p + 2); // 패킷 타입

        std::cout << "[클라] 수신 패킷 :size = " << size << ", type = "<< static_cast<int>(type) << "\n";

        switch (type) {
        case S_C_HIT_RESULT:
        {
            break;
        }
        case S_C_OBJECT_ADD:
        {
            pkt_sc_object_add* addPkt = reinterpret_cast<pkt_sc_object_add*>(recv_p);

            std::cout << "[클라] S_C_OBJECT_ADD : id = " << addPkt->id << ", obj_type = " << addPkt->fixdata.obj_type << ", skin_type = " << addPkt->fixdata.skin_type
                << " name = " << addPkt->fixdata.name << ", startposition = (" << addPkt->fixdata.startposition.x << ", " << addPkt->fixdata.startposition.y << ", " << addPkt->fixdata.startposition.z << ") "
                << ", starthp = " << addPkt->fixdata.starthp << std::endl;
            break;

        }
        case S_C_OBJECT_UPDATE:
        {

            pkt_sc_object_update* updatePkt = reinterpret_cast<pkt_sc_object_update*>(recv_p);
            std::cout << "[클라] S_C_OBJECT_UPDATE : id = " << updatePkt->id << ",position = (" << updatePkt->obj.meta.position.x << ", " << updatePkt->obj.meta.position.y << ", " << updatePkt->obj.meta.position.z << ") "
                << ", direction = (" << updatePkt->obj.meta.direction.x << ", " << updatePkt->obj.meta.direction.y << ", " << updatePkt->obj.meta.direction.z << ") "
                << ", speed = " << updatePkt->obj.meta.speed << ", hp = " << updatePkt->obj.meta.hp
                << ", hp  =" << updatePkt->obj.meta.hp
                << ". gun_type = " << updatePkt->obj.gun_type
                << ", level = " << updatePkt->obj.level
                << ", score = " << updatePkt->obj.score
                << ", damage = " << updatePkt->obj.damage
                << ", act_type = " << updatePkt->obj.act_type << std::endl;
            break;
        }
        case S_C_OBJECT_REMOVE:
        {
            pkt_sc_object_remove* removePkt = reinterpret_cast<pkt_sc_object_remove*>(recv_p);
            std::cout << "[클라] S_C_OBJECT_REMOVE : id = " << removePkt->id << std::endl;
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
        offset += size;
        recv_p += size;
    }

    // 다시 수신 등록
    ZeroMemory(&recv_over, sizeof(recv_over));
    recv_wsabuf[0].buf = recv_buffer;
    recv_wsabuf[0].len = sizeof(recv_buffer);
    DWORD recv_flag = 0;
    WSARecv(c_socket, recv_wsabuf, 1, NULL, &recv_flag, &recv_over, recv_callback);
}

void send_packet(void* packet, int size) {


    WSABUF wsabuf[1];
    wsabuf[0].buf = reinterpret_cast<char*>(packet);
    wsabuf[0].len = size;
    WSAOVERLAPPED send_over = {};
    DWORD sent;
    int ret = WSASend(c_socket, wsabuf, 1, &sent, 0, &send_over, NULL);
    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        error_display("WSASend 실패", WSAGetLastError());
    }
}

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

int main()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

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

    std::cout << "아이디 입력: ";
    std::string name;
    std::cin >> name;

    // 수신 등록
    recv_wsabuf[0].buf = recv_buffer;
    recv_wsabuf[0].len = sizeof(recv_buffer);
    ZeroMemory(&recv_over, sizeof(recv_over));
    DWORD recv_flag = 0;
    WSARecv(c_socket, recv_wsabuf, 1, NULL, &recv_flag, &recv_over, recv_callback);

    // 1. 로그인 패킷 전송
    pkt_cs_login loginPkt{};
    loginPkt.header.size = sizeof(loginPkt);
    loginPkt.header.type = PKT_TYPE::C_S_LOGIN;
    loginPkt.skin_type = 1;
    strcpy_s(loginPkt.name, MAX_NAME_SIZE, name.c_str());
    send_packet(&loginPkt, sizeof(loginPkt));
	std::cout << "[클라] 로그인 패킷 전송 size = " << sizeof(loginPkt) << ", type = " << (int)loginPkt.header.type << "\n";

    Sleep(100); // 서버 처리 대기

    // 2. 이동 패킷 전송
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

    send_packet(&u_movePkt, sizeof(u_movePkt));
	std::cout << "[클라] 이동 패킷 전송 size = " << sizeof(u_movePkt) << ", type = " << (int)u_movePkt.header.type << "\n";

    // 메시지 루프 유지
    while (is_running)
        SleepEx(100, TRUE); // 콜백이 실행되도록 SleepEx(0, TRUE)

    closesocket(c_socket);
    WSACleanup();
    std::cout << "종료됨\n";
    return 0;
}