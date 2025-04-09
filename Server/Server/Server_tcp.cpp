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

enum COMP_TYPE { OP_RECV, OP_SEND };

class OVER_EXP {
public:
    WSAOVERLAPPED overlapped;
    WSABUF wsabuf[1];
    char buffer[1024];
    COMP_TYPE compType;


    OVER_EXP(COMP_TYPE op) : compType(op) {
        ZeroMemory(&overlapped, sizeof(overlapped));

        wsabuf[0].buf = reinterpret_cast<CHAR*>(buffer);
        wsabuf[0].len = sizeof(buffer);
    }
};

void CALLBACK IoCallback(DWORD err, DWORD bytesTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD flags);
enum SESSION_STATE { ST_FREE, ST_ALLOC, ST_INGAME };

struct SESSION {
    SOCKET          socket;
    uint32_t        id;

    SESSION_STATE   state = ST_FREE;
    OVER_EXP*       recv_over = nullptr;

    int             prev_remain = 0;
    std::mutex      lock;


    void do_recv() {
        if (!recv_over) {
            recv_over = new OVER_EXP(OP_RECV);
            recv_over->overlapped.hEvent = (HANDLE)this;
        }
        DWORD flags = 0;
        int ret = WSARecv(socket, recv_over->wsabuf, 1, 0, &flags, &recv_over->overlapped, IoCallback);
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

    void sendPacket(SESSION* s, void *buff) {
        OVER_EXP* sendOv = new OVER_EXP(OP_SEND);
        const unsigned char packet_size = reinterpret_cast<unsigned char*>(buff)[0];
        memcpy(sendOv->buffer, buff, packet_size);
        sendOv->wsabuf[0].len = packet_size;
        DWORD size_sent;

        int ret = WSASend(s->socket, sendOv->wsabuf, 1, &size_sent, 0, &(sendOv->overlapped), IoCallback);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
            delete sendOv;
        }
    }
    //void send_player_info_packet()
    //{
    //    sc_packet_avatar_info p;
    //    p.size = sizeof(p);
    //    p.type = S2C_P_AVATAR_INFO;
    //    p.id = _id;
    //    p.x = _x;
    //    p.y = _y;
    //    p.level = 1;
    //    p.hp = 100;
    //    p.exp = 200;
    //    do_send(&p);
    //}

    //void send_player_position()
    //{
    //    sc_packet_move p;
    //    p.size = sizeof(p);
    //    p.type = S2C_P_MOVE;
    //    p.id = _id;
    //    p.x = _x;
    //    p.y = _y;
    //    do_send(&p);
    //}

    //void process_packet(unsigned char* p)
    //{
    //    const unsigned char packet_type = p[1];
    //    switch (packet_type) {
    //    case C2S_P_LOGIN:
    //    {
    //        cs_packet_login* packet = reinterpret_cast<cs_packet_login*>(p);
    //        _name = packet->name;
    //        _x = 4;
    //        _y = 4;
    //        send_player_info_packet();
    //        break;
    //    }
    //    case C2S_P_MOVE: {
    //        cs_packet_move* packet = reinterpret_cast<cs_packet_move*>(p);
    //        switch (packet->direction) {
    //        case MOVE_UP: if (_y > 0) _y = _y - 1; break;
    //        case MOVE_DOWN: if (_y < (MAP_HEIGHT - 1)) _y = _y + 1; break;
    //        case MOVE_LEFT: if (_x > 0) _x = _x - 1; break;
    //        case MOVE_RIGHT: if (_x < (MAP_WIDTH - 1)) _x = _x + 1; break;
    //        }
    //        send_player_position();
    //        break;
    //    }
    //    default:
    //        std::cout << "Error Invalid Packet Type\n";
    //        exit(-1);
    //    }
    //}

};
struct Quaternion {
    float x = 0, y = 0, z = 0, w = 1;
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

struct ShootPacket {
    uint8_t GunType; // 총 종류
    float bulletPos[3];
    float bulletDir[3];
};
struct Player {
    PlayerInfo player_info;
    bool isShooting = false;
    SESSION* session = nullptr; // SESSION 연결

};
struct Zombie {
    ZombieInfo zombie_info;
};

std::vector<SESSION*> session_st;
std::vector<Player*> player_st;
std::vector<Zombie*> zombie_st;
std::mutex playersMutex;
std::mutex zombiesMutex;
bool serverRunning = true;
uint32_t nextPlayerId = 1;
ThreadPool pool(std::thread::hardware_concurrency());


void broadcastState_NoLock() {
    //BroadcastPacket pkt;
    //pkt.playerCount = static_cast<int>(players.size());
    //for (int i = 0; i < players.size(); ++i) {
    //    pkt.players[i].id = players[i]->id;
    //    pkt.players[i].position = players[i]->position;
    //    pkt.players[i].rotation = players[i]->rotation;
    //    pkt.players[i].hp = players[i]->hp;
    //}

    //pkt.zombieCount = static_cast<int>(zombies.size());
    //for (int i = 0; i < zombies.size(); ++i) {
    //    pkt.zombies[i].id = zombies[i]->id;
    //    pkt.zombies[i].position = zombies[i]->position;
    //    pkt.zombies[i].rotation = zombies[i]->rotation;
    //    pkt.zombies[i].hp = zombies[i]->hp;
    //}

    //for (auto& player : players) {
    //    WSABUF wsabuf;
    //    wsabuf.buf = (char*)&pkt;
    //    wsabuf.len = sizeof(BroadcastPacket);
    //    DWORD sentBytes = 0;
    //    OVERLAPPED sendOv = {};
    //    //int ret = WSASend(player->socket, &wsabuf, 1, &sentBytes, 0, &sendOv, NULL);
    //    int ret = WSASend(player->session->socket, &wsabuf, 1, &sentBytes, 0, &sendOv, NULL);
    //    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
    //        std::cout << "Broadcast failed: Player ID " << player->id << "\n";
    //    }
    //}
}


void processPlayerPacket(Player* player, char* buffer, int bytesTransferred) {
    if (bytesTransferred >= sizeof(ShootPacket)) {
        ShootPacket shootPkt;
        memcpy(&shootPkt, buffer, sizeof(ShootPacket));
    }
}


void CALLBACK IoCallback(DWORD err, DWORD bytesTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD flags) {

    SESSION* session = (SESSION*)lpOverlapped->hEvent;

    if (err != 0 || bytesTransferred == 0)
    {
        std::cout << "Player ID " << session->id << " disconnected\n";

        std::lock_guard<std::mutex> lock(playersMutex);

        // 연결된 Player 찾기 (SESSION과 연결된 Player 검색)
        auto it = std::find_if(player_st.begin(), player_st.end(), [&](Player* p) { return p->session == session; });
        if (it != player_st.end())
        {
            Player* player = *it;
            session->disconnect();         // 소켓 닫기 + Recv OVER_EXP 해제
            delete session;                // SESSION 메모리 해제
            player_st.erase(it);             // players 벡터에서 제거
            delete player;                 // Player 메모리 해제
            broadcastState_NoLock();       // 모든 클라이언트에 상태 브로드캐스트
        }
        return;
    }

    // --- 정상적으로 데이터 수신된 경우 ---
    Player* player = nullptr;
    {
        std::lock_guard<std::mutex> lock(playersMutex);
        for (auto& p : player_st)  // SESSION과 연결된 Player 찾아
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

    broadcastState_NoLock(); // 모든 클라이언트에게 브로드캐스트

    session->do_recv();  // 다시 Recv 등록 , Recv OVER_EXP 초기화 후 WSARecv 재호출 
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
        if (player_st.size() >= MAX_PLAYER_COUNT) {
            std::cout << "Max clients reached, rejecting connection\n";
            closesocket(clientSocket);
            continue;
        }
        SESSION* session = new SESSION;
        session->socket = clientSocket;
        session->id = nextPlayerId;
        session->state = ST_ALLOC;
        session_st.push_back(session);    // 세션 등록

        Player* player = new Player;
        player->player_info.id= nextPlayerId++;
        player->session = session;
        player_st.push_back(player);      // 플레이어 등록

        std::cout << "Player connected, ID: " << player->player_info.id << "\n";

        session->do_recv();
        broadcastState_NoLock();
    }

    std::cout << "서버 종료 중...\n";
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
