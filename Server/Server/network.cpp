#include "network.h"
#include "session.h"
#include "../../protocol.h"
#include <unordered_map>
#include <memory>

#define DEBUG_PRINT false
#define DEBUG_LOG(msg) do { if (DEBUG_PRINT) std::cout << msg << std::endl; } while (0)


extern std::unordered_map<SIZEID, std::shared_ptr<GameObject>> g_gameObjects;

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


void CALLBACK g_send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag) {
    OVER_EXP* send_ov = reinterpret_cast<OVER_EXP*>(p_over);
    delete send_ov;
}

void CALLBACK g_recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag) {

    auto my_id = reinterpret_cast<SIZEID>(p_over->hEvent);
    auto it = g_gameObjects.find(my_id);

    DEBUG_LOG("[RECV_CALLBACK] ID = " << my_id << ", 받은 바이트 = " << num_bytes);

    if (it == g_gameObjects.end()) {
        std::cout << "[ERROR] Invalid session ID: " << my_id << "\n";
        return;
    }

    auto player = dynamic_cast<PlayerSession*>(it->second.get());
    if (!player) {
        std::cout << "[ERROR] Object is not a PlayerSession\n";
        return;
    }

    player->recv_callback(num_bytes);
}

