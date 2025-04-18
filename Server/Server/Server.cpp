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
std::unordered_map<int, Zombie*> zombieMap;
const int MAP_WIDTH = 100;          // 유니티에서 타일맵 사이즈
const int MAP_HEIGHT = 100;
int tileMap[MAP_HEIGHT][MAP_WIDTH]; // 0: 빈 공간, 1: 벽/장애물

const int PLAYER_HP = 100;
const int ZOMBIE_HP = 50;

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct AStarNode {
    int x, y;
    float gCost, hCost;
    int parentX = -1;
    int parentY = -1;
    float fCost() const { return gCost + hCost; }
    bool operator>(const AStarNode& other) const { return fCost() > other.fCost(); }
};

std::vector<Vector3> FindPath(Vector3 startPos, Vector3 targetPos) {
    std::vector<Vector3> path;

    int startX = static_cast<int>(startPos.x);
    int startY = static_cast<int>(startPos.z);
    int targetX = static_cast<int>(targetPos.x);
    int targetY = static_cast<int>(targetPos.z);

    auto heuristic = [](int x1, int y1, int x2, int y2) {
        return abs(x1 - x2) + abs(y1 - y2);
        };

    std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openList;
    std::map<std::pair<int, int>, AStarNode> allNodes;
    std::set<std::pair<int, int>> closedList;

    AStarNode startNode = { startX, startY, 0.0f, static_cast<float>(heuristic(startX, startY, targetX, targetY)), -1, -1 };
    openList.push(startNode);
    allNodes[{startX, startY}] = startNode;

    while (!openList.empty()) {
        AStarNode current = openList.top();
        openList.pop();

        if (current.x == targetX && current.y == targetY) {
            AStarNode trace = current;
            while (trace.parentX != -1) {
                path.push_back({ static_cast<float>(trace.x), 0.0f, static_cast<float>(trace.y) });
                trace = allNodes[{trace.parentX, trace.parentY}];
            }
            path.push_back({ static_cast<float>(startX), 0.0f, static_cast<float>(startY) });
            std::reverse(path.begin(), path.end());
            return path;
        }

        closedList.insert({ current.x, current.y });

        const int dx[4] = { -1, 1, 0, 0 };
        const int dy[4] = { 0, 0, -1, 1 };

        for (int dir = 0; dir < 4; ++dir) {
            int nx = current.x + dx[dir];
            int ny = current.y + dy[dir];

            if (nx < 0 || ny < 0 || nx >= MAP_WIDTH || ny >= MAP_HEIGHT)
                continue;
            if (tileMap[ny][nx] == 1)
                continue; // 장애물
            if (closedList.find({ nx, ny }) != closedList.end())
                continue;

            float newGCost = current.gCost + 1.0f;
            float hCost = static_cast<float>(heuristic(nx, ny, targetX, targetY));

            AStarNode next = { nx, ny, newGCost, hCost, current.x, current.y };

            if (allNodes.find({ nx, ny }) != allNodes.end()) {
                if (allNodes[{nx, ny}].gCost <= newGCost) continue;
            }

            openList.push(next);
            allNodes[{nx, ny}] = next;
        }
    }

    return path; // 경로 없음
}

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
    //SOCKET socket;
    //OVER_EXP* recv_over = nullptr;
    //char buffer[1024];
    //OVERLAPPED overlapped;
    //WSABUF wsabuf;
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

    void UpdateAI(const std::vector<Player*>& players) {
        if (hp <= 0) return;

        float minDistSq = FLT_MAX;  // 가장 가까운 플레이어 찾기

        Player* targetPlayer = nullptr;
        for (Player* p : players) {
            float dx = p->position.x - position.x;
            float dz = p->position.z - position.z;
            float distSq = dx * dx + dz * dz;
            if (distSq < minDistSq) {
                minDistSq = distSq;
                targetPlayerId = p->id;
                targetPlayer = p;
            }
        }

        if (targetPlayer) {
            Vector3 target = targetPlayer->position;
            if (path.empty() || pathIndex >= path.size()) {
                path = FindPath(position, target);
                pathIndex = 0;
            }

            if (pathIndex < path.size()) {
                Vector3 next = path[pathIndex];
                position.x += (next.x - position.x) * 0.1f;
                position.z += (next.z - position.z) * 0.1f;
                if (fabsf(position.x - next.x) < 0.1f && fabsf(position.z - next.z) < 0.1f) {
                    pathIndex++;
                }
                dirty = true;
            }
        }
    }
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
    //std::lock_guard<std::mutex> lock1(playersMutex);
    //std::lock_guard<std::mutex> lock2(zombiesMutex);
    broadcastState_NoLock();
}

bool checkRaySphereIntersection(Vector3 rayOrigin, Vector3 rayDir, Vector3 sphereCenter, float radius) {
    Vector3 oc = { sphereCenter.x - rayOrigin.x, sphereCenter.y - rayOrigin.y, sphereCenter.z - rayOrigin.z };
    float t = oc.x * rayDir.x + oc.y * rayDir.y + oc.z * rayDir.z;
    Vector3 closestPoint = { rayOrigin.x + rayDir.x * t, rayOrigin.y + rayDir.y * t, rayOrigin.z + rayDir.z * t };
    float distSq = powf(sphereCenter.x - closestPoint.x, 2) + powf(sphereCenter.y - closestPoint.y, 2) + powf(sphereCenter.z - closestPoint.z, 2);
    return distSq <= radius * radius;
}

void processShoot(Player* shooter, ShootPacket& shootPkt) {
    std::lock_guard<std::mutex> lock(zombiesMutex);
    auto it = zombieMap.find(shootPkt.hitZombieId);
    if (it == zombieMap.end()) return; // 해당 좀비 없음
    Zombie* z = it->second;
    if (z->hp <= 0) return; // 이미 죽음

    if (!checkRaySphereIntersection(shootPkt.bulletPos, shootPkt.bulletDir, z->position, 1.0f)) {
        std::cout << "Suspicious hit rejected\n";
        return;
    }

    z->hp -= 10;
    z->dirty = true;
    if (z->hp <= 0)
        std::cout << "Zombie " << z->id << " dead!\n";
}

void processPlayerPacket(Player* player, char* buffer, int bytesTransferred) {
    if (bytesTransferred >= sizeof(ShootPacket)) {
        ShootPacket shootPkt;
        memcpy(&shootPkt, buffer, sizeof(ShootPacket));
        processShoot(player, shootPkt);
    }
}

void checkZombieHit(Player* player) { // 플레이어와 좀비 가까우면 
    std::lock_guard<std::mutex> lock(zombiesMutex);
    for (Zombie* z : zombies) {
        if (z->hp <= 0) continue;
        float distSq = pow(player->position.x - z->position.x, 2) + pow(player->position.z - z->position.z, 2);
        if (distSq < 1.0f) { // 간단 충돌
            z->hp -= 10;
            z->dirty = true;
            if (z->hp <= 0) std::cout << "Zombie " << z->id << " dead!\n";
        }
    }
}

//void CALLBACK IoCallback(DWORD err, DWORD bytesTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
//{
//    Player* player = (Player*)lpOverlapped->hEvent;
//    if (err != 0 || bytesTransferred == 0) {
//        std::cout << "Player ID " << player->id << " disconnected\n";
//        {
//            std::lock_guard<std::mutex> lock(playersMutex);
//            closesocket(player->socket);
//            delete player->session;
//            if (player->recv_over) delete player->recv_over;
//            players.erase(std::remove(players.begin(), players.end(), player), players.end());
//            delete player;
//            broadcastState_NoLock();
//        }
//        return;
//    }
//
//
//    processPlayerPacket(player, player->recv_over->buffer, bytesTransferred); //클라로부터 받은 패킷 처리
//    //checkZombieHit(player);
//    broadcastState(); // 현재 상태 모든 클라에게 동기화
//
//    //ZeroMemory(&player->overlapped, sizeof(OVERLAPPED));
//    //player->overlapped.hEvent = (HANDLE)player;
//    //player->wsabuf.buf = player->buffer;
//    //player->wsabuf.len = sizeof(player->buffer);
//
//    //ZeroMemory(&player->recv_over->overlapped, sizeof(OVERLAPPED));
//    //player->recv_over->overlapped.hEvent = (HANDLE)player;
//    player->session->do_recv();
//    DWORD recvBytes = 0, flagsRecv = 0;
//    //int ret = WSARecv(player->socket, &player->wsabuf, 1, &recvBytes, &flagsRecv, &player->overlapped, IoCallback);
//    int ret = WSARecv(player->socket, &player->recv_over->wsabuf, 1, &recvBytes, &flagsRecv, &player->recv_over->overlapped, IoCallback);
//    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
//        std::cout << "WSARecv re-register failed\n";
//    }
//}

//void registerPlayerRecv(Player* player) { //SESSION 도입 전 -> do_recv()로 대체
//    //ZeroMemory(&player->overlapped, sizeof(OVERLAPPED));
//    //player->overlapped.hEvent = (HANDLE)player;
//    //player->wsabuf.buf = player->buffer;
//    //player->wsabuf.len = sizeof(player->buffer);
//
//    if (!player->recv_over) {
//        player->recv_over = new OVER_EXP(); // 새로 Recv용 OVER_EXP 할당
//        player->recv_over->overlapped.hEvent = (HANDLE)player;
//    }
//
//    DWORD recvBytes = 0, flagsRecv = 0;
//    //int ret = WSARecv(player->socket, &player->wsabuf, 1, &recvBytes, &flagsRecv, &player->overlapped, IoCallback);
//    int ret = WSARecv(player->socket, &player->recv_over->wsabuf, 1, &recvBytes, &flagsRecv, &player->recv_over->overlapped, IoCallback);
//    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
//        std::cout << "WSARecv failed\n";
//    }
//}

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
void zombieAILoop() {
    for (int i = 0; i < MAX_ZOMBIE_COUNT; ++i) {
        Zombie* z = new Zombie();
        z->id = i;
        zombies.push_back(z);
        zombieMap[z->id] = z; // zombieMap에 등록도 잊지 말기
    }

    while (serverRunning) {
        std::lock_guard<std::mutex> lock1(playersMutex);
        std::lock_guard<std::mutex> lock2(zombiesMutex);

        for (Zombie* z : zombies) {
            pool.Enqueue([z]() { z->UpdateAI(players); }); // players 전달
        }

        broadcastState_NoLock();
        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // 약 30 FPS
    }
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
        error_display("Socket creation failed", WSAGetLastError());\
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
    std::thread(zombieAILoop).detach();

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

        //Player* player = new Player;
        //player->id = nextPlayerId++;
        //player->socket = clientSocket;
        //players.push_back(player);

        //std::cout << "Player connected, ID: " << player->id << "\n";
        //registerPlayerRecv(player);
        //broadcastState_NoLock();

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
