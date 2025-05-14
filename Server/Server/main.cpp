#include <iostream>
#include <thread>
#include <locale>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "../../protocol.h"
#include "network.h"
#include "session.h"
#include "zombie_system.h"
#include "ZombieAI.h"

#pragma comment(lib, "ws2_32.lib")

#define DEBUG_PRINT true
#define DEBUG_LOG(msg) do { if (DEBUG_PRINT) std::cout << msg << std::endl; } while (0)

// 전역
std::vector<ZombieAI*> g_zombies;
std::unordered_map<SIZEID, std::shared_ptr<GameObject>> g_gameObjects;
std::vector<std::vector<int>> g_map;
bool serverRunning = true;
int IN_g_player_n = 0;


constexpr SIZEID START_SESSION_ID = 0;

void serverControl() {
    while (true) {
        char cmd;
        std::cin >> cmd;
        if (cmd == 'q') {
            std::cout << "[서버 종료 명령]\n";
            serverRunning = false;
            break;
        }
    }
}

int main() {
    std::wcout.imbue(std::locale("korean"));

    g_map = LoadMapBin("Node/ob_mask_te_1.bin");

    DEBUG_LOG("[Map] 로딩 완료: 행 수 = " << g_map.size());

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        error_display("WSAStartup failed", WSAGetLastError());

    SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket == INVALID_SOCKET)
        error_display("Socket creation failed", WSAGetLastError());

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_NUM);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
        error_display("Bind failed", WSAGetLastError());

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
        error_display("Listen failed", WSAGetLastError());

    std::cout << "Zombie Strike 3D 서버 실행 중 (포트: " << PORT_NUM << ")\n";

    // 쓰레드 시작
    std::thread(serverControl).detach();
    std::thread(ZombieAIThread).detach();

    SpawnZombies(MAX_ZOMBIE_COUNT);
    DEBUG_LOG("[MAIN] 좀비 생성 완료, 개수 = " << g_zombies.size());

    // 접속 처리
    SIZEID clientId = START_SESSION_ID;
    SOCKADDR_IN clientAddr;
    int addrSize = sizeof(clientAddr);

    while (serverRunning) {
        SOCKET clientSocket = WSAAccept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept failed\n";
            continue;
        }

        g_gameObjects.try_emplace(clientId, std::make_shared<PlayerSession>(clientId, clientSocket));
        clientId++;
    }

    std::cout << "서버 종료 중...\n";
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
