#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <queue>
#include <print>

#include "../../protocol.h"
#include "ZombieAI.h" 

#pragma comment(lib, "ws2_32.lib")
constexpr double FRAME_INTERVAL_MS = 1000.0 / 60.0;

constexpr bool DEBUG_PRINT = false;
#define DEBUG_LOG(msg) \
    do { if (DEBUG_PRINT) std::cout << msg << std::endl; } while (0)

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

struct ShootPacket {
    SIZE1 GunType; // 총 종류
    float bulletPos[3];
    float bulletDir[3];
};

struct Zombie {
	SIZEID id;
    Object zombieobj;
    SIZE2 damage;               
    SIZE1 act_type;             
};

std::vector<ZombieAI*> g_zombies; // ZombieAI 객체를 서버가 관리
// std::vector<std::unique_ptr<ZombieAI>> g_zombies;
std::vector<std::vector<int>> g_map; // 맵 데이터
std::mutex zombiesMutex;

bool serverRunning = true;
short IN_g_player_n= 0;

class SESSION;
std::unordered_map<SIZEID, SESSION> g_users;
// std::unordered_map<SIZEID, std::shared_ptr<SESSION>>로 교체

void CALLBACK g_recv_callback(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void CALLBACK g_send_callback(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);


bool validate_score_info(const pkt_cs_score_info* p) {
    return (p->stage_score <= 10000);
}

bool validate_stage_info(const pkt_cs_stage_info* p) {
    return (p->currentStage >= 1 && p->currentStage <= 10 && p->timeLeft <= 60000);

}
enum IO_OP { OP_RECV, OP_SEND };
class OVER_EXP {
public:

    OVER_EXP(IO_OP op) : _io_op(op) {
        ZeroMemory(&_over, sizeof(_over));

        _wsabuf[0].buf = reinterpret_cast<CHAR*>(_buffer);
        _wsabuf[0].len = sizeof(_buffer);
    }

    WSAOVERLAPPED   _over;
    IO_OP           _io_op;
    SIZE2           _buffer[1024];
    WSABUF          _wsabuf[1];
};

class SESSION {
public:
    SOCKET          _c_socket;
    SIZEID          _id;

    OVER_EXP        _recv_over{OP_RECV};
    SIZE2           _remained = 0;

    ObjectType      _obj_type;
    SIZE1           _skin_type;
    std::string     _name;

    Vec3            _position;
    Vec3            _velocity;
    Vec3            _look;
	float           _pitch;
    SIZE2           _hp;
    GunType         _gun_type;     
    SIZE1           _level;
    SIZE2           _score;
    SIZE2           _damage;               
    SIZE1           _act_type;            

    void do_recv() {
        DWORD flags = 0;
        ZeroMemory(&_recv_over._over, sizeof(_recv_over._over));
        _recv_over._over.hEvent = reinterpret_cast<HANDLE>(_id); // 세션 ID를 이벤트 핸들로 사용

        _recv_over._wsabuf[0].buf = reinterpret_cast<CHAR*>(_recv_over._buffer) + _remained;	//prev_remain 부분에 이어서 수신하기 위해서
        _recv_over._wsabuf[0].len = sizeof(_recv_over._buffer) - _remained;

        int ret = WSARecv(_c_socket, _recv_over._wsabuf, 1, 0, &flags, &_recv_over._over, g_recv_callback);
        if (ret == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err != WSA_IO_PENDING) {
				std::cout << "[do_recv] WSARecv failed: " << err << "\n";
                closesocket(_c_socket);
                g_users.erase(_id);
            }
            else {
				DEBUG_LOG("[do_recv] WSARecv: IO_PENDING (정상)\n");
            }
        }
        else {
			DEBUG_LOG("[do_recv] WSARecv: 즉시 수신 완료 (ret == 0)\n");
        }
    }

public: 
    SESSION() {
		DEBUG_LOG("[SESSION] Default constructor called\n");
        exit(-1);
    }
	SESSION(SIZEID session_id, SOCKET s) : _id(session_id), _c_socket(s)
    {
        _recv_over._wsabuf[0].len = sizeof(_recv_over._buffer);
        _recv_over._wsabuf[0].buf = reinterpret_cast<CHAR* >(_recv_over._buffer);

        _recv_over._over.hEvent = reinterpret_cast<HANDLE>(session_id);

        _remained = 0;
		do_recv();
	}
    ~SESSION() 
    {
		DEBUG_LOG("[SESSION] Destructor called ID =\n", _id);

        pkt_sc_object_remove rem_p;
        rem_p.header.size = sizeof(rem_p);
        rem_p.header.type = PKT_TYPE::S_C_OBJECT_REMOVE;
        rem_p.id= _id;
        for (auto& u : g_users) {
			if (u.first != _id) // 나를 제외한 상대방에게 알리고
				u.second.do_send(&rem_p);
        }
		closesocket(_c_socket);
    }

    void recv_callback(int num_bytes) {
        // ----- 패킷 조립 시작 -----
        SIZE2* p = _recv_over._buffer;
        SIZE3 total = _remained + num_bytes;
        SIZE3 offset = 0;

        while (offset < total) {
            SIZE2 packetSize = *p;

            if (offset + packetSize > total) break; // 아직 패킷 완성이 안 됨

			DEBUG_LOG("[RECV][" << _id << "] packetSize = " << (SIZE3)packetSize << std::endl);

			process_packet(p);    // 패킷 처리
            p += (packetSize)/sizeof(SIZE2);      // 다음 패킷으로 이동
            offset += packetSize;
        }

        // 조립 안 된 데이터는 앞으로 당겨서 저장
        _remained = total - offset;

        if (_remained > 0)
            memmove(_recv_over._buffer, p, _remained);

        do_recv(); // 다음 수신
	}

    void do_send(void* buff) {
        OVER_EXP* send_ov = new OVER_EXP(OP_SEND);
        SIZE2 packet_size = reinterpret_cast<SIZE2*>(buff)[0];
        memcpy(send_ov->_buffer, buff, packet_size);
        send_ov->_wsabuf[0].buf = reinterpret_cast<CHAR*>(send_ov->_buffer);
        send_ov->_wsabuf[0].len = packet_size;
        DWORD size_sent;

		DEBUG_LOG("[do_send] ID = " << _id << ", size = " << packet_size << ", type = " << (int)reinterpret_cast<SIZE2*>(buff)[1] << std::endl);
        int ret = WSASend(_c_socket, send_ov->_wsabuf, 1, &size_sent, 0, &(send_ov->_over), g_send_callback);
        if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			std::cout << "[do_send] WSASend failed: " << WSAGetLastError() << "\n";
        }
    }

    void send_obj_info() {
		pkt_sc_obj_info packet;
		ZeroMemory(&packet, sizeof(packet));
		packet.header.size = sizeof(packet);
        packet.header.type = PKT_TYPE::S_C_OBJ_INFO;
		packet.id = _id;
		packet.obj_type = _obj_type;
		packet.skin_type = _skin_type;
		strcpy_s(packet.name, _name.c_str());
		packet.startposition = _position;
		packet.starthp = _hp;
        packet.velocity = _velocity;
        packet.look = _look;
        packet.pitch = _pitch;

		packet.act_type = _act_type;
		packet.gun_type = _gun_type;

		packet.level = _level;
		packet.score = _score;
		packet.damage = _damage;

        do_send(&packet);
    }

    void send_object_update() {
        pkt_sc_object_update p_update;
        p_update.header.size = sizeof(p_update);
        p_update.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
        p_update.id = _id;
        p_update.velocity = _velocity;
        p_update.look = _look;
        p_update.pitch = _pitch;
        p_update.hp = _hp;

        p_update.gun_type = _gun_type;
        p_update.level = _level;
        p_update.score = _score;
        p_update.damage = _damage;
        p_update.act_type = _act_type;
        do_send(&p_update);
    }

    // 총알 충돌 체크 및 데미지 적용
    void check_bullet_collision(SIZEID shooter_id, const Vec3& origin, const Vec3& direction) {
        constexpr float maxDistance = 100.0f;
        constexpr float hitRadius = 1.0f;

        Vec3 normDir = direction.Normalize();
        Vec3 endPos = origin + normDir * maxDistance;

        pkt_sc_hit_result hit_packet;
        hit_packet.header.size = sizeof(hit_packet);
        hit_packet.header.type = PKT_TYPE::S_C_HIT_RESULT;
        hit_packet.shooterId = shooter_id;
        for (auto& [id, session] : g_users)
            session.do_send(&hit_packet);


        // 좀비 충돌 체크
        for (auto zombie : g_zombies) {
            Vec3 toTarget = zombie->GetPosition() - origin;
            float t = toTarget.Dot(normDir);
            if (t < 0 || t > maxDistance) continue;

            Vec3 closest = origin + normDir * t;
            float distSqr = (closest - zombie->GetPosition()).LengthSquared();

            if (distSqr <= hitRadius * hitRadius) {
                zombie->AddDamage(10); 

				if (zombie->GetHP() <= 0) {
					// 좀비가 죽었을 때 처리
					// std::cout << "[Zombie] " << zombie->GetID() << " is dead.\n";
					zombie->ClearDirty(); // 좀비 상태 초기화 , 여기서 하는게 좋은가?
				}
            }
        }

        // 플레이어 충돌 체크
     //for (auto& [id, session] : g_users) {
     //    if (id == shooter_id) continue;
     //    Vec3 toTarget = session._position - origin;
     //    float t = toTarget.Dot(normDir);
     //    if (t < 0 || t > maxDistance) continue;
     //    Vec3 closest = origin + normDir * t;
     //    float distSqr = (closest - session._position).LengthSquared();
     //    if (distSqr <= hitRadius * hitRadius) {
     //        session._hp = (std::max)(0, session._hp - 10);
     //       // std::cout << "[Hit] " << shooter_id << " hit Player " << id << "\n";
     //        session.send_object_update();
     //    }
     //}
    }

	void process_packet(SIZE2* packet) {

		const unsigned char packet_type = packet[1];
        if (packet_type == 0) {
            std::cout << "[ERROR] Invalid Packet Type\n";
            return;
        }
		DEBUG_LOG("[process_packet] ID = " << _id << ", packet_type = " << (int)packet_type << "\n");

        switch (packet_type) {
        case ::PKT_TYPE::C_S_LOGIN:
        {
            pkt_cs_login* loginPacket = reinterpret_cast<pkt_cs_login*>(packet);
            _obj_type   = ObjectType::PLAYER;
            _skin_type  = loginPacket->skin_type;
            _name       = loginPacket->name;
            _position   = START_POSITIONS[IN_g_player_n % 3];
            _velocity  = { 0.0f,0.0f, 0.0f };
            _look      = { 0.0f,0.0f, 0.0f };
            _pitch      = 0.0f;
            _hp         = PLAYER_HP;
			_gun_type   = GunType::BULLET_PISTOL; // 총 종류
            _level      = 1;
            _score      = 0;
            _damage     = 0;
			_act_type   = ActionType::NONE;
            
            IN_g_player_n++;
			DEBUG_LOG("[process_packet][RECV][" << (int)_id << "] C_S_LOGIN: " << _name << "\n");
			DEBUG_LOG("[process_packet][RECV][" << (int)_id << "] C_S_LOGIN: " << _skin_type << "\n");
            send_obj_info();
            //send_object_update();

			pkt_sc_object_add p_Add_P;
			p_Add_P.header.size = sizeof(p_Add_P);
			p_Add_P.header.type = PKT_TYPE::S_C_OBJECT_ADD;
			p_Add_P.id = _id;
			p_Add_P.obj_type = ObjectType::PLAYER;
			p_Add_P.skin_type = _skin_type;
            strcpy_s(p_Add_P.name, _name.c_str());
			p_Add_P.startposition = _position;
			p_Add_P.starthp = _hp;
			p_Add_P.gun_type = BULLET_PISTOL;

            for (auto& u : g_users) {
                if (u.first != _id) // 나를 제외한 상대방에게 알리고
                    u.second.do_send(&p_Add_P);
            }
			for (auto& u : g_users) {
				if (u.first != _id) {// 나를 제외한 상대방의 정보를 나에게 알리고
                    pkt_sc_object_add p_Add_P;
                    p_Add_P.header.size = sizeof(p_Add_P);
                    p_Add_P.header.type = PKT_TYPE::S_C_OBJECT_ADD;

                    p_Add_P.id = u.first;
                    p_Add_P.obj_type = ObjectType::PLAYER;
                    p_Add_P.skin_type = u.second._skin_type;
                    strcpy_s(p_Add_P.name, u.second._name.c_str());
                    p_Add_P.startposition = u.second._position;
                    p_Add_P.starthp = u.second._hp;
					do_send(&p_Add_P);
				}
			}

            // 좀비 정보를 모든 플레이어에게 전송
            pkt_sc_object_add packet;
            for (auto zombie : g_zombies) {
                packet.header.size = sizeof(packet);
                packet.header.type = PKT_TYPE::S_C_OBJECT_ADD;
                packet.id = zombie->GetID();
                packet.obj_type = ObjectType::ZOMBIE;
                packet.skin_type = 0;
                strcpy_s(packet.name, "Zombie");
                packet.startposition = zombie->GetPosition();
                packet.starthp = zombie->GetHP();

                for (auto& [id, session] : g_users) {
                    session.do_send(&packet);
                }
            }

            break;
        }
        case PKT_TYPE::C_S_UPDATE:
        {
			pkt_cs_update* updatePacket = reinterpret_cast<pkt_cs_update*>(packet);


            float deltaTime = 1.0f / 60.0f; // 서버 틱 레이트 기준 (예: 60fps)
            _position = updatePacket->position;
            _velocity = updatePacket->velocity;
            _look = updatePacket->look;
            _pitch = updatePacket->pitch;
            _hp = updatePacket->hp;
            _level = updatePacket->level;
            _score = updatePacket->score;
            _damage = updatePacket->damage;
            _gun_type = updatePacket->gun_type;
            _act_type = updatePacket->act_type;

            // 로그
            //std::cout << "[process_packet][RECV][" << (int)_id << "] C_S_UPDATE: " << _name << "\n";
            //std::cout << "  position  = (" << _position.x << ", " << _position.y << ", " << _position.z << ")\n";
            //std::cout << "  direction = (" << _direction.x << ", " << _direction.y << ", " << _direction.z << ")\n";
            //std::cout << "  speed     = " << _speed << "\n";

            pkt_sc_object_update u_move_p;
            u_move_p.header.size = sizeof(u_move_p);
            u_move_p.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
            u_move_p.id = _id;
			u_move_p.position = _position;
			u_move_p.velocity = _velocity;
			u_move_p.look = _look;
			u_move_p.pitch = _pitch;
			u_move_p.hp = _hp;
			u_move_p.gun_type = _gun_type;
			u_move_p.level = _level;
			u_move_p.score = _score;
			u_move_p.damage = _damage;
			u_move_p.act_type = _act_type;

            for (auto& [id, session] : g_users) {
                if (id != _id)
                    session.do_send(&u_move_p);
            }
            break;
        }

        case PKT_TYPE::C_S_SCORE_INFO:
        {
            auto* p = reinterpret_cast<pkt_cs_score_info*>(packet);

            if (!validate_score_info(p)) {
                std::cout << "[SCORE_INFO] 유효하지 않은 점수 무시\n";
                break;
            }

            pkt_sc_score_info resp;
            resp.header.size = sizeof(resp);
            resp.header.type = PKT_TYPE::S_C_SCORE_INFO;
            resp.stage_score = p->stage_score;

            for (auto& [id, session] : g_users)
                session.do_send(&resp);

            break;
        }

        case PKT_TYPE::C_S_STAGE_INFO:
        {
            auto* p = reinterpret_cast<pkt_cs_stage_info*>(packet);

            if (!validate_stage_info(p)) {
                std::cout << "[STAGE_INFO] 유효하지 않은 값 무시\n";
                break;
            }

            pkt_sc_stage_info resp;
            resp.header.size = sizeof(resp);
            resp.header.type = PKT_TYPE::S_C_STAGE_INFO;
            resp.currentStage = p->currentStage;
            resp.totalStages = 1;
            resp.timeLeft = p->timeLeft;

            for (auto& [id, session] : g_users)
                session.do_send(&resp);

            break;
        }

        case PKT_TYPE::C_S_SHOOT:
        {
            auto* p = reinterpret_cast<pkt_cs_shoot*>(packet);
            Vec3 origin = { p->bulletPos[0], p->bulletPos[1], p->bulletPos[2] };
            Vec3 direction = { p->bulletDir[0], p->bulletDir[1], p->bulletDir[2] };

            // 레이캐스트 충돌 처리 함수 호출
            check_bullet_collision(_id, origin, direction);

            break;
        }


        default:
            std::cout << "[WARN] Unknown PacketType: " << packet_type << "\n";
            break;
        }
	}

};





void CALLBACK g_send_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    OVER_EXP* send_ov = reinterpret_cast<OVER_EXP*>(p_over);
    delete send_ov;
}

void CALLBACK g_recv_callback(DWORD err, DWORD num_bytes, LPWSAOVERLAPPED p_over, DWORD flag)
{
    auto my_id = reinterpret_cast<SIZEID>(p_over->hEvent);

    if (g_users.find(my_id) == g_users.end()) {
		std::cout << "[ERROR] Invalid session ID: " << my_id << "\n";
        return;
    }
    g_users[my_id].recv_callback(num_bytes);

}


auto lastTick = std::chrono::steady_clock::now();

void SpawnZombies(int count) {
    for (int i = 0; i < count; ++i) {
        auto [x, z] = GetRandomPosition(g_map);
        ZombieAI* zombie = new ZombieAI(g_map, 10000 + i);
        zombie->SetPosition((float)x, (float)z);
        zombie->SetHP(ZOMBIE_HP);
        g_zombies.push_back(zombie);

        // 좀비 정보를 모든 플레이어에게 전송
        pkt_sc_object_add p;
        p.header.size = sizeof(p);
        p.header.type = PKT_TYPE::S_C_OBJECT_ADD;
        p.id = zombie->GetID();
        p.obj_type = ObjectType::ZOMBIE;
        p.skin_type = 0;
        strcpy_s(p.name, "Zombie");
        p.startposition = zombie->GetPosition();
        p.starthp = zombie->GetHP();
        p.gun_type = GunType::BULLET_MAX;

        for (auto& [id, session] : g_users)
            session.do_send(&p);
    }
}


void ZombieAIThread() {
    while (serverRunning) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> dt = now - lastTick;
        lastTick = now;
        float deltaTime = dt.count();  // 초 단위

        std::vector<Vec3> playerPositions;
        for (auto& [id, session] : g_users) {
            if (session._obj_type == ObjectType::PLAYER)
                playerPositions.push_back(session._position);
        }

        for (auto& zombie : g_zombies) {
            zombie->Update(playerPositions, g_zombies, deltaTime);

            if (zombie->IsDirty()) {
                Object info = zombie->GetObjectinfo();

                pkt_sc_object_update p;
                p.header.size = sizeof(p);
                p.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
                p.id = zombie->GetID();
				p.act_type = info.act_type;
				p.position = info.position;
				p.velocity = info.velocity;
				p.look = info.look;
				p.pitch = info.pitch;
				p.hp = info.hp;
				p.gun_type = info.gun_type;
				p.level = info.level;
				p.score = info.score;
				p.damage = info.damage;
				p.act_type = info.act_type;

                for (auto& [id, session] : g_users)
                    session.do_send(&p);

                zombie->ClearDirty();
            }
        }

        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(FRAME_INTERVAL_MS));
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

    std::wcout.imbue(std::locale("korean")); 

    g_map = LoadMapBin("Node/ob_mask_te_1.bin");

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        error_display("WSAStartup failed", WSAGetLastError());
    else
        std::cout << "WSAStartup Success\n";

    SOCKET s_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
    if (s_socket == INVALID_SOCKET)
        error_display("Socket creation failed", WSAGetLastError()); \
    else
        std::cout << "Socket creation Success\n";

    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT_NUM);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind (s_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
        error_display("Bind failed", WSAGetLastError());
    else { std::cout << "Bind Success\n"; }

    if (listen (s_socket, SOMAXCONN) == SOCKET_ERROR)
        error_display("Listen failed", WSAGetLastError());
    else { std::cout << "Listen Success\n"; }   


    std::cout << "Zombie Strike 3D Server running on port: " << PORT_NUM << "\n";

    std::thread(serverControl).detach();
    std::thread(ZombieAIThread).detach();
    SpawnZombies(MAX_ZOMBIE_COUNT);

    SIZEID clientId = 0;
    INT serverAddr_size = sizeof(SOCKADDR_IN);

    while (serverRunning) {
        auto c_socket = WSAAccept(s_socket,reinterpret_cast<sockaddr*>(&serverAddr), &serverAddr_size, NULL, NULL);
        if (c_socket == INVALID_SOCKET) {
            std::cout << "Accept failed\n";
            continue;
        }

        g_users.try_emplace(clientId, clientId, c_socket);
        clientId++;
    }

    std::cout << "서버 종료 중...\n";
    closesocket(s_socket);
    WSACleanup();
}
