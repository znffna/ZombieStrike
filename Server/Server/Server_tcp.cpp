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

struct Zombie;

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

enum COMP_TYPE { OP_RECV, OP_SEND };

class OVER_EXP {
public:
    WSAOVERLAPPED overlapped;
    WSABUF wsabuf;
    char buffer[1024];
    COMP_TYPE compType;

    OVER_EXP() {
        ZeroMemory(&overlapped, sizeof(overlapped));
        wsabuf.buf = buffer;
        wsabuf.len = sizeof(buffer);
        compType = OP_RECV;
    }

    OVER_EXP(const char* sendData, size_t len) {
        ZeroMemory(&overlapped, sizeof(overlapped));
        wsabuf.buf = buffer;
        memcpy(buffer, sendData, len);
        wsabuf.len = static_cast<ULONG>(len);
        compType = OP_SEND;
    }
};

void CALLBACK IoCallback(DWORD err, DWORD bytesTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD flags);
enum SESSION_STATE { ST_FREE, ST_ALLOC, ST_INGAME };

struct SESSION {
    SOCKET socket;
    int id;
    SESSION_STATE state = ST_FREE;
    OVER_EXP* recv_over = nullptr;
    int prev_remain = 0;
    std::mutex lock;

    void do_recv() {
        if (!recv_over) {
            recv_over = new OVER_EXP();
            recv_over->overlapped.hEvent = (HANDLE)this;
        }
        DWORD flags = 0;
        int ret = WSARecv(socket, &recv_over->wsabuf, 1, 0, &flags, &recv_over->overlapped, IoCallback);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cout << "WSARecv failed\n";
        }
    }

    void disconnect() {
        closesocket(socket);
        if (recv_over) delete recv_over;
        recv_over = nullptr;
        socket = INVALID_SOCKET;
        {
            std::lock_guard<std::mutex> lg(lock);
            state = ST_FREE;
        }
    }
};

struct Quaternion {
    float x = 0, y = 0, z = 0, w = 1;
};

struct ShootPacket {
    int playerId;
    int hitZombieId;    // 클라에서 맞췄다고 판단한 좀비 ID
    Vector3 bulletPos;  // 총알 발사 위치
    Vector3 bulletDir;  // 총알 방향
};

struct Player {
    int id;
    Vector3 position;
    Quaternion rotation;
    int hp = PLAYER_HP;
    bool isShooting = false;
    SESSION* session = nullptr; // SESSION 연결

};

struct Zombie {
    int id;
    Vector3 position;
    Quaternion rotation;
    int hp = ZOMBIE_HP;
    short state = 0;
    bool dirty = true;

    int targetPlayerId = -1;    // 현재 따라가는 플레이어 ID
    std::vector<Vector3> path;  // A* 경로
    int pathIndex = 0;          // 현재 따라가는 인덱스

};

struct BroadcastPacket {
    int playerCount;
    struct {
        int id;
        Vector3 position;
        Quaternion rotation;
        int hp;
    } players[MAX_PLAYER_COUNT];

    int zombieCount;
    struct {
        int id;
        Vector3 position;
        Quaternion rotation;
        int hp;
    } zombies[MAX_ZOMBIE_COUNT];
};

class ThreadPool {
public:
    ThreadPool(size_t n) {
        for (size_t i = 0; i < n; ++i)
            workers.emplace_back([this]() { WorkerLoop(); });
    }
    ~ThreadPool() { Stop(); }

    void Enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(m);
            tasks.push(task);
        }
        cv.notify_one();
    }

private:
    void WorkerLoop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m);
                cv.wait(lock, [this]() { return stop || !tasks.empty(); });
                if (stop && tasks.empty()) return;
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
    }

    void Stop() {
        {
            std::unique_lock<std::mutex> lock(m);
            stop = true;
        }
        cv.notify_all();
        for (auto& t : workers) t.join();
    }

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex m;
    std::condition_variable cv;
    bool stop = false;
};

std::vector<SESSION*> sessions;
std::vector<Player*> players;
std::vector<Zombie*> zombies;
std::mutex playersMutex;
std::mutex zombiesMutex;
bool serverRunning = true;
int nextPlayerId = 1;
ThreadPool pool(std::thread::hardware_concurrency());

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

void broadcastState_NoLock() {
    BroadcastPacket pkt;
    pkt.playerCount = static_cast<int>(players.size());
    for (int i = 0; i < players.size(); ++i) {
        pkt.players[i].id = players[i]->id;
        pkt.players[i].position = players[i]->position;
        pkt.players[i].rotation = players[i]->rotation;
        pkt.players[i].hp = players[i]->hp;
    }

    pkt.zombieCount = static_cast<int>(zombies.size());
    for (int i = 0; i < zombies.size(); ++i) {
        pkt.zombies[i].id = zombies[i]->id;
        pkt.zombies[i].position = zombies[i]->position;
        pkt.zombies[i].rotation = zombies[i]->rotation;
        pkt.zombies[i].hp = zombies[i]->hp;
    }

    for (auto& player : players) {
        WSABUF wsabuf;
        wsabuf.buf = (char*)&pkt;
        wsabuf.len = sizeof(BroadcastPacket);
        DWORD sentBytes = 0;
        OVERLAPPED sendOv = {};
        //int ret = WSASend(player->socket, &wsabuf, 1, &sentBytes, 0, &sendOv, NULL);
        int ret = WSASend(player->session->socket, &wsabuf, 1, &sentBytes, 0, &sendOv, NULL);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            std::cout << "Broadcast failed: Player ID " << player->id << "\n";
        }
    }
}

void broadcastState() {
    broadcastState_NoLock();
}

void CALLBACK IoCallback(DWORD err, DWORD bytesTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD flags) {

    SESSION* session = (SESSION*)lpOverlapped->hEvent;

    if (err != 0 || bytesTransferred == 0)
    {
        std::cout << "Player ID " << session->id << " disconnected\n";

        std::lock_guard<std::mutex> lock(playersMutex);

        // 연결된 Player 찾기 (SESSION과 연결된 Player 검색)
        auto it = std::find_if(players.begin(), players.end(), [&](Player* p) { return p->session == session; });
        if (it != players.end())
        {
            Player* player = *it;
            session->disconnect();         // 소켓 닫기 + Recv OVER_EXP 해제
            delete session;                // SESSION 메모리 해제
            players.erase(it);             // players 벡터에서 제거
            delete player;                 // Player 메모리 해제
            broadcastState_NoLock();       // 모든 클라이언트에 상태 브로드캐스트
        }
        return;
    }

    // --- 정상적으로 데이터 수신된 경우 ---
    Player* player = nullptr;
    {
        std::lock_guard<std::mutex> lock(playersMutex);
        for (auto& p : players)  // SESSION과 연결된 Player 찾아
        {
            if (p->session == session)
            {
                player = p;
                break;
            }
        }
    }

    if (player == nullptr)  // 예외 처리 (SESSION은 있는데 Player가 없을 때)
    {
        std::cout << "Player not found for SESSION ID: " << session->id << "\n";
        return;
    }
    processPlayerPacket(player, session->recv_over->buffer, bytesTransferred); // 수신된 데이터 처리 (ShootPacket 등)

    broadcastState(); // 모든 클라이언트에게 브로드캐스트

    session->do_recv();  // 다시 Recv 등록 , Recv OVER_EXP 초기화 후 WSARecv 재호출 , (IOCP 환경에서는.. ) 
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

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        error_display("WSAStartup failed", WSAGetLastError());
    else
        std::cout << "WSAStartup 성공\n";

    SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (listenSocket == INVALID_SOCKET)
        error_display("Socket creation failed", WSAGetLastError()); \
    else
        std::cout << "Socket creation 성공\n";

    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_NUM);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        error_display("Bind failed", WSAGetLastError());
    else
        std::cout << "Bind 성공\n";

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
        error_display("Listen failed", WSAGetLastError());
    else
        std::cout << "Listen 성공\n";

    std::cout << "Zombie Strike 3D Server running on port: " << PORT_NUM << "\n";

    std::thread(serverControl).detach();

    while (serverRunning) {
        sockaddr_in clientAddr;
        int addrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &addrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cout << "Accept failed\n";
            continue;
        }

        std::lock_guard<std::mutex> lock(playersMutex);
        if (players.size() >= MAX_PLAYER_COUNT) {
            std::cout << "Max clients reached, rejecting connection\n";
            closesocket(clientSocket);
            continue;
        }

        SESSION* session = new SESSION;
        session->socket = clientSocket;
        session->id = nextPlayerId;
        session->state = ST_ALLOC;
        sessions.push_back(session);    // 세션 등록

        Player* player = new Player;
        player->id = nextPlayerId++;
        player->session = session;
        players.push_back(player);      // 플레이어 등록

        std::cout << "Player connected, ID: " << player->id << "\n";

        session->do_recv();
        broadcastState_NoLock();
    }

    std::cout << "서버 종료 중...\n";
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
