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

// ---[Bullet ↔ Zombie Hit Test Only / 3D]-----------------------------

static inline void normalize3(float& x, float& y, float& z)
{
    const float m2 = x * x + y * y + z * z;
    if (m2 > 0.0f) {
        const float inv = 1.0f / sqrtf(m2);
        x *= inv; y *= inv; z *= inv;
    }
}

// Ray(ox,oy,oz; dx,dy,dz) vs Sphere(center cx,cy,cz; radius r)
// - out_t : 가장 이른 교차 파라미터 t(원점에서 거리)
// - 반환값 : 교차 여부
static bool ray3_vs_sphere(float ox, float oy, float oz,
    float dx, float dy, float dz,
    float cx, float cy, float cz, float r,
    float& out_t)
{
    // (o + t d - c)·(o + t d - c) = r^2
    const float vx = ox - cx;
    const float vy = oy - cy;
    const float vz = oz - cz;

    const float b = 2.0f * (dx * vx + dy * vy + dz * vz);
    const float c = (vx * vx + vy * vy + vz * vz) - r * r;

    const float disc = b * b - 4.0f * c;   // a = 1 (정규화)
    if (disc < 0.0f) return false;

    const float sqrt_disc = sqrtf(disc);
    const float t1 = (-b - sqrt_disc) * 0.5f;
    const float t2 = (-b + sqrt_disc) * 0.5f;

    if (t1 >= 0.0f) { out_t = t1; return true; }
    if (t2 >= 0.0f) { out_t = t2; return true; }
    return false;
}

// Ray vs Vertical Capsule(Y-axis)
// 캡슐 정의: 세로축(Y) 정렬 캡슐. 바닥점 y0, 머리점 y1, 반지름 r.
// 좀비는 (cx, cz) 수평 위치에 세움. 바닥 y0=0, 머리 y1=ZOMBIE_HEIGHT 가정.
// 반환: 교차 시 가장 작은 양의 t를 out_t에 기록.
static bool ray3_vs_capsule_y(
    float ox, float oy, float oz,
    float dx, float dy, float dz,   // d는 정규화 
    float cx, float cz,             // 캡슐 수직축의 x,z (y축 정렬)
    float y0, float y1,             // 바닥 y0, 머리 y1
    float r,
    float& out_t)
{
    //   1) 원기둥(무한 높이 아님, 유한 높이) 구간과 레이 교차
    //   평면투영(XZ)에서 원 반지름 r로 빗겨가는지 체크 후, 높이(y) 범위 교차인지 확인
    //   수학적으로: (o_perp + t d_perp - c_perp)^2 = r^2, 그리고 y(t) ∈ [y0, y1]
    const float vx = ox - cx;
    const float vz = oz - cz;

    // d_perp = (dx, dz)
    const float A = dx * dx + dz * dz;        // 원기둥 측면 교차용(A=0이면 수직 레이)
    const float B = 2.0f * (dx * vx + dz * vz);
    const float C = (vx * vx + vz * vz) - r * r;

    float best_t = FLT_MAX;
    bool  hit = false;

    if (A > 1e-6f) {
        const float disc = B * B - 4.0f * A * C;
        if (disc >= 0.0f) {
            const float s = sqrtf(disc);
            const float t1 = (-B - s) / (2.0f * A);
            const float t2 = (-B + s) / (2.0f * A);

            auto accept_cylinder_t = [&](float t) {
                if (t < 0.0f) return;
                float y = oy + t * dy;
                if (y >= y0 && y <= y1) {
                    if (t < best_t) { best_t = t; hit = true; }
                }
                };
            accept_cylinder_t(t1);
            accept_cylinder_t(t2);
        }
    }
    else {
        // 레이가 y축 거의 평행(수직) → 측면 원기둥 교차는 불능. 상/하 구에만 의존.
    }

    // 2) 끝구(hemisphere) 교차: 바닥/머리의 구체와 각각 레이 교차
    auto ray_sphere = [&](float scy) {
        float t;
        if (ray3_vs_sphere(ox, oy, oz, dx, dy, dz, cx, scy, cz, r, t)) {
            // 구체는 반구여야 하지만, 측면에서 이미 걸러줬으므로 그대로 사용 가능
            if (t >= 0.0f && t < best_t) { best_t = t; hit = true; }
        }
        };
    ray_sphere(y0); // 바닥 반구(센터 y=y0)
    ray_sphere(y1); // 머리 반구(센터 y=y1)

    if (hit) { out_t = best_t; }
    return hit;
}

// 레이와 가장 가까운 ‘살아있는’ 좀비(3D 스피어) 탐색
//  - 입력: 총알 원점/방향(정규화 내부 처리), 최대 사거리
//  - 출력: 맞은 좀비 ID와 t (없으면 false)
//  - 스피어 중심 Y(cy)는 일단 0.0f(맵 XZ 평면 기준). 필요시 모델 키값으로 보정 가능.
static bool find_nearest_hit_zombie3D(float ox, float oy, float oz,
    float dx, float dy, float dz,
    float max_range,
    /*out*/int& out_zid, /*out*/float& out_t)
{
    normalize3(dx, dy, dz);

    float best_t = max_range;
    int   best_id = -1;

    std::lock_guard<std::mutex> lock(zombiesMutex);
    for (auto* z : g_zombies) {
        if (!z) continue;
        if (z->IsDead()) continue;

        const float cx = z->GetX();
        //const float cy = 0.0f;      // 필요 시 높이 보정
        const float cz = z->GetZ();

        float t = 0.0f;
        if (ray3_vs_capsule_y(
            ox, oy, oz,
            dx, dy, dz,
            cx, cz,
            /*y0*/ 0.0f,
            /*y1*/ ZOMBIE_HEIGHT,
            /*r */ ZOMBIE_RADIUS,
            t))
        {
            if (t < best_t) {
                best_t = t;
                best_id = z->GetID();
            }
        }
        //if (ray3_vs_sphere(ox, oy, oz, dx, dy, dz, cx, cy, cz, ZOMBIE_HALF_SIZE, t)) {
        //    if (t < best_t) {
        //        best_t = t;
        //        best_id = z->GetID();
        //    }
        //}
    }

    if (best_id >= 0) {
        out_zid = best_id;
        out_t = best_t;
        return true;
    }
    return false;
}

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
    SIZE1           _move_input;

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
        packet.move_input = _move_input;

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
        p_update.move_input = _move_input;
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
                zombie->ApplyDamage(GUN_DAMAGE);

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
            _velocity   = { 0.0f,0.0f, 0.0f };
            _look       = { 0.0f,0.0f, 0.0f };
            _pitch      = 0.0f;
            _hp         = PLAYER_HP;
			_gun_type   = GunType::BULLET_PISTOL; // 총 종류
            _level      = 1;
            _score      = 0;
            _damage     = GUN_DAMAGE;
			_act_type   = ActionType::NONE;
            _move_input = 0;

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

            // [C_S_LOGIN] 기존 좀비 스냅샷은 "신규 접속자(나)"에게만 유니캐스트
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
                packet.gun_type = GunType::BULLET_MAX; // [추가] 누락 보정(좀비는 총 안씀)

                // 나(this 세션)에게만 전송
                this->do_send(&packet);
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
            _move_input = updatePacket->move_input;

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
			u_move_p.move_input = _move_input;

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

            // XYZ 그대로 사용
            float ox = p->bulletPos[0];
            float oy = p->bulletPos[1];
            float oz = p->bulletPos[2];

            float dx = p->bulletDir[0];
            float dy = p->bulletDir[1];
            float dz = p->bulletDir[2];

            // [SESSION::process_packet] 변경: 발사 브로드캐스트(S_C_SHOOT) 
            {
                pkt_sc_shoot b{};
                b.header.size = sizeof(b);
                b.header.type = PKT_TYPE::S_C_SHOOT;
                b.shooterId = _id;
                b.gun_type = _gun_type;            // 서버 관점에서 신뢰 가능한 총기 타입
                b.bulletPos[0] = ox; b.bulletPos[1] = oy; b.bulletPos[2] = oz;
                b.bulletDir[0] = dx; b.bulletDir[1] = dy; b.bulletDir[2] = dz;

                for (auto& [id, session] : g_users) {
                    if (id == _id) continue;         // 사수에게는 재전송 불필요
                    session.do_send(&b);
                }
            }

            constexpr float MAX_RANGE = 60.0f; // 임시 사거리

            int   hit_zid = -1;
            float hit_t = 0.0f;

            if (find_nearest_hit_zombie3D(ox, oy, oz, dx, dy, dz, MAX_RANGE, hit_zid, hit_t)) {
                DEBUG_LOG("[HIT-TEST/3D] shooter=" << _id
                    << " hit_zid=" << hit_zid << " t=" << hit_t);

                // !! 데미지 + 브로드캐스트
                constexpr SIZE2 DAMAGE = GUN_DAMAGE;   // 임시 고정 대미지(총기별 테이블은 이후에 연결)

                SIZE2 hp_after = 0;
                {
                    std::lock_guard<std::mutex> lock(zombiesMutex);
                    ZombieAI* hitZ = nullptr;
                    for (auto* z : g_zombies) {
                        if (z && z->GetID() == hit_zid) { hitZ = z; break; }
                    }
                    if (hitZ) {
                        hitZ->ApplyDamage(DAMAGE);
                        hp_after = hitZ->GetHP();
                    }
                }

                // (기존) 히트 결과 먼저 브로드캐스트
                pkt_sc_hit_result resp{};
                resp.header.size = sizeof(resp);
                resp.header.type = PKT_TYPE::S_C_HIT_RESULT;
                resp.shooterId = _id;
                resp.zombieId = hit_zid;
                resp.zombieHp = hp_after;
                for (auto& [id, session] : g_users) session.do_send(&resp);

                //  HP가 0이면 즉시 제거 패킷 (중복 방지: MarkRemoved)
                if (hp_after == 0) {
                    ZombieAI* hitZ = nullptr;
                    {
                        std::lock_guard<std::mutex> lock(zombiesMutex);
                        for (auto* z : g_zombies) {
                            if (z && z->GetID() == hit_zid) { hitZ = z; break; }
                        }
                    }
                    if (hitZ && !hitZ->IsRemoved()) {
                        hitZ->MarkRemoved(); // // ZombieAI::MarkRemoved - 서버 틱/충돌 제외

                        pkt_sc_object_remove rem{};
                        rem.header.size = sizeof(rem);
                        rem.header.type = PKT_TYPE::S_C_OBJECT_REMOVE;
                        rem.id = hit_zid;

                        for (auto& [id, session] : g_users)
                            session.do_send(&rem);
                    }
                }
            }
            else {
                DEBUG_LOG("[HIT-TEST/3D] shooter=" << _id << " miss");
            }
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

// 요구했던 두 포인트 + 예시 포인트 1~2개 더 (원하는 만큼 2~4개만 채워서 사용)
std::vector<std::pair<int, int>> spawnPoints = {
    {150, 180},  // 포인트 A
    {150, 100},  // 포인트 B
    // {200, 300},  // 포인트 C (원하면 활성화)
    // {400, 420},  // 포인트 D (원하면 활성화)
};


// // SpawnZombies: N등분 스폰 적용 (GetSpawnPointByIndexN)
void SpawnZombies(int count) {
    // // SpawnZombies: 방어 코드 – 스폰 포인트 미설정 시 기본 포인트 두 개로 세팅
    if (spawnPoints.empty()) {                                 
        spawnPoints = { {150,180}, {150,100} };                
    }

    for (int i = 0; i < count; ++i) {
        // // SpawnZombies: 균등 분할로 i번째 스폰 좌표 선택
        auto [sx, sz] = GetSpawnPointByIndexN(g_map, spawnPoints, i, count);  

        // // SpawnZombies: 좌표계 선택
        // 프로젝트가 '셀 인덱스' 좌표를 SetPosition에 기대하면 아래 1줄 사용:
        float zx = static_cast<float>(sx);                          
        float zz = static_cast<float>(sz);                         

        // 월드 좌표(CELL_SIZE 배수)를 SetPosition에 기대한다면 위 2줄 대신 아래 2줄 사용:
        // float zx = static_cast<float>(sx) * CELL_SIZE;          
        // float zz = static_cast<float>(sz) * CELL_SIZE;           

        ZombieAI* zombie = new ZombieAI(g_map, 10000 + i);
        zombie->SetPosition(zx, zz);                                
        zombie->SetHP(ZOMBIE_HP);
        g_zombies.push_back(zombie);

        // // SpawnZombies: 생성 즉시 대상 지정/경로 탐색을 원하면 필요 시 활성화
        // zombie->SetTargetPosition(player_x, player_z);          
        // zombie->FindPath();                                      

        // 좀비 정보를 모든 플레이어에게 전송 (기존 유지)
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

        // 플레이어 스냅샷: ID와 위치 동시 수집
        std::vector<Vec3> playerPositions;
        std::vector<std::pair<SIZEID, Vec3>> playerList;  // // ZombieAIThread - ID 포함
        for (auto& [id, session] : g_users) {
            if (session._obj_type == ObjectType::PLAYER) {
                playerPositions.push_back(session._position);
                playerList.emplace_back(id, session._position);
            }
        }

        for (auto& zombie : g_zombies) {
            if (zombie->IsRemoved()) continue;

            zombie->Update(playerPositions, g_zombies, deltaTime);
            // ZombieAIThread - 제거 플래그면 완전 스킵

            if (zombie->IsDirty()) {
                // 프레임 경합 방어: DEAD 상태면 업데이트 대신 제거 패킷
                if (zombie->IsDead()) {
                    zombie->MarkRemoved();

                    pkt_sc_object_remove rem{};
                    rem.header.size = sizeof(rem);
                    rem.header.type = PKT_TYPE::S_C_OBJECT_REMOVE;
                    rem.id = zombie->GetID();
                    for (auto& [id, session] : g_users) session.do_send(&rem);

                    zombie->ClearDirty();
                    continue;
                }

                // 일반 업데이트 브로드캐스트
                Object info = zombie->GetObjectinfo();
                pkt_sc_object_update p{};
                p.header.size = sizeof(p);
                p.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
                p.id = zombie->GetID();
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

                for (auto& [id, session] : g_users) session.do_send(&p);
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


        int flag = 1;
        setsockopt(c_socket, IPPROTO_TCP, TCP_NODELAY,
            reinterpret_cast<const char*>(&flag), sizeof(flag));

        g_users.try_emplace(clientId, clientId, c_socket);
        clientId++;
    }

    std::cout << "서버 종료 중...\n";
    closesocket(s_socket);
    WSACleanup();
}
