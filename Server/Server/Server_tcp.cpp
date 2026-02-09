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
#include <random>
#include <atomic>
#include <chrono> 

#include "../../protocol.h"
#include "ZombieAI.h" 

#pragma comment(lib, "ws2_32.lib")
constexpr double FRAME_INTERVAL_MS = 1000.0 / 60.0;

constexpr bool DEBUG_PRINT = false;
#define DEBUG_LOG(msg) \
    do { if (DEBUG_PRINT) std::cout << msg << std::endl; } while (0)

// // [GLOBAL] - 리스폰 체력(기존 상수 재사용)
static constexpr SIZE2 PLAYER_RESPAWN_HP = PLAYER_HP;      
static constexpr int  PLAYER_RESPAWN_MS = 3000;        


constexpr int ZOMBIE_SKIN_COUNT = 3; 
static std::mt19937 g_rng{ std::random_device{}() }; 
static std::uniform_int_distribution<int> g_zombieSkinDist(0, ZOMBIE_SKIN_COUNT - 1); 
static std::unordered_map<SIZEID, SIZE1> g_zombieSkin;  // 좀비 id -> skin_type (늦게 접속한 유저 스냅샷 일관성 유지용)

static std::unordered_map<SIZEID, ZombieType> g_zombieType;                 // 좀비 타입 저장(늦게 접속한 유저 스냅샷 일관성 유지용)
static std::discrete_distribution<int> g_zombieTypeDist({ 50, 40, 10 });    // 타입 가중치 랜덤(예: NORMAL 70%, RUNNER 20%, TANKER 10%)

static std::uniform_real_distribution<float> g_speedMulNormal(0.85f, 1.15f); // // NORMAL 범위
static std::uniform_real_distribution<float> g_speedMulRunner(0.95f, 1.10f); // // RUNNER 범위(폭 작게)
static std::uniform_real_distribution<float> g_speedMulTanker(0.85f, 1.05f); // // TANKER 범위(폭 작게)

// // [GLOBAL] - Stage1 스코어/좀비 카운터(서버 권위)
static std::atomic<SIZE2> g_stage_score{ 0 };
static std::atomic<SIZE2> g_total_zombies{ 0 };
static std::atomic<SIZE2> g_killed_zombies{ 0 };

// // [GLOBAL] - Stage1 진행 상태(필요 시 확장)
static std::atomic<SIZE2> g_current_stage{ 1 };
static std::atomic_bool   g_stage1_cleared{ false };

// // [GLOBAL] - Wave 시스템(3단계) 추가
static constexpr SIZE1  WAVE_TOTAL = 3;
static constexpr SIZE2  WAVE_PLAN[WAVE_TOTAL] = { 1, 20, 30 }; // Wave: 웨이브별 스폰 수(원하는 값으로 변경)

static std::atomic<SIZE1> g_current_wave{ 1 };       // Wave: 현재 웨이브(1~3)
static std::atomic<SIZE2> g_wave_total_zombies{ 0 }; // Wave: 현재 웨이브 총 수
static std::atomic<SIZE2> g_wave_killed_zombies{ 0 };// Wave: 현재 웨이브 킬 수

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

static SIZE2 GetMaxAmmo(GunType gt) // 총 타입별 최대 탄 수
{
    switch (gt) {
    //case GunType::BULLET_PISTOL:  return 12;
    case GunType::BULLET_RIFLE:   return 3000;
    //case GunType::BULLET_SHOTGUN: return 8;
    default:                      return 3000;
    }
}


static SIZE2 GetReloadMs(GunType gt)    // 총 타입별 리로드 시간(ms)
{
    switch (gt) {
    //case GunType::BULLET_PISTOL:  return 1200;
    case GunType::BULLET_RIFLE:   return 1500;
    //case GunType::BULLET_SHOTGUN: return 2000;
    default:                      return 1500;
    }
}


std::vector<ZombieAI*> g_zombies;
static std::atomic<SIZEID> g_nextZombieId{ 10000 }; // 웨이브마다 좀비 ID 재사용 방지(유니크)
std::vector<std::vector<int>> g_map; 
std::mutex zombiesMutex;

static bool Server_KillZombie(SIZEID zombieId);


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
std::mutex g_usersMutex; // g_users 보호
// std::unordered_map<SIZEID, std::shared_ptr<SESSION>>로 교체

void CALLBACK g_recv_callback(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void CALLBACK g_send_callback(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);


bool validate_score_info(const pkt_cs_score_info* p) {
    return (p->stage_score <= 10000);
}

bool validate_stage_info(const pkt_cs_stage_info* p) {
    return (p->currentStage >= 1 && p->currentStage <= 10 && p->timeLeft <= 60000);

}

static void SendAmmoInfoToSelf(SESSION& s);
static void BroadcastScoreInfoToAll();
static void BroadcastStageInfoToAll(SIZE2 timeLeft);
void SpawnZombies(int count);
static void GatherUserTargets(std::vector<SESSION*>& out);

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

    SIZE2 _ammo_cur = 0;
    SIZE2 _ammo_max = 0;
    bool  _reloading = false;
    std::chrono::steady_clock::time_point _reload_end_tp{};

    // ===============================
    // Respawn (Server Authority)
    // ===============================
    SIZE1  _spawn_index = 0;  // 로그인 때 배정된 스폰 인덱스 보관(리스폰 위치용)
    bool   _respawning = false; // 죽음/리스폰 대기 상태
    std::chrono::steady_clock::time_point _respawn_end_tp{}; // 리스폰 예정 시각

    std::atomic_bool _is_loaded{ false };
    std::atomic_bool _alive{ true };        // - 소켓 생존 플래그

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
                _alive.store(false, std::memory_order_release);
				std::cout << "[do_recv] WSARecv failed: " << err << "\n";
                closesocket(_c_socket);
                { std::lock_guard<std::mutex> lk(g_usersMutex); g_users.erase(_id); }
            }
            else {
				//DEBUG_LOG("[do_recv] WSARecv: IO_PENDING (정상)\n");
            }
        }
        else {
			//DEBUG_LOG("[do_recv] WSARecv: 즉시 수신 완료 (ret == 0)\n");
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

			//DEBUG_LOG("[RECV][" << _id << "] packetSize = " << (SIZE3)packetSize << std::endl);

			process_packet(p);    // 패킷 처리
            p += (packetSize)/sizeof(SIZE2);    // 다음 패킷으로 이동
            offset += packetSize;
        }

        _remained = total - offset; // 조립 안 된 데이터는 앞으로 당겨서 저장

        if (_remained > 0)
            memmove(_recv_over._buffer, p, _remained);

        do_recv(); // 다음 수신
	}

    void do_send(void* buff) {
        if (!_alive.load(std::memory_order_acquire)) return;

        OVER_EXP* send_ov = new OVER_EXP(OP_SEND);
        SIZE2 packet_size = reinterpret_cast<SIZE2*>(buff)[0];
        memcpy(send_ov->_buffer, buff, packet_size);
        send_ov->_wsabuf[0].buf = reinterpret_cast<CHAR*>(send_ov->_buffer);
        send_ov->_wsabuf[0].len = packet_size;
        DWORD size_sent;

		//DEBUG_LOG("[do_send] ID = " << _id << ", size = " << packet_size << ", type = " << (int)reinterpret_cast<SIZE2*>(buff)[1] << std::endl);
        int ret = WSASend(_c_socket, send_ov->_wsabuf, 1, &size_sent, 0, &(send_ov->_over), g_send_callback);
        if (ret == SOCKET_ERROR) {
            int e = WSAGetLastError();
            if (e != WSA_IO_PENDING) {
                // // [SESSION::do_send] - 소켓 죽음 처리(10054 포함)
                _alive.store(false, std::memory_order_release);
            }
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
        ZeroMemory(&p_update, sizeof(p_update));
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
                zombie->AddPendingDamage(GUN_DAMAGE);

    //            zombie->ApplyDamage(10);

				//if (zombie->GetHP() <= 0) {
				//	// 좀비가 죽었을 때 처리
				//	// std::cout << "[Zombie] " << zombie->GetID() << " is dead.\n";
				//	zombie->ClearDirty(); // 좀비 상태 초기화 , 여기서 하는게 좋은가?
				//}
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
    void SendSceneSnapshot()
    {
        std::vector<SESSION*> targets;
        GatherUserTargets(targets);

        // 1) 나에게: 이미 로딩 완료된 플레이어들만 ADD
        for (SESSION* pu : targets) {
            SESSION& u = *pu;

            if (u._id == _id) continue;
            if (u._obj_type != ObjectType::PLAYER) continue;
            if (!u._is_loaded.load(std::memory_order_acquire)) continue;

            pkt_sc_object_add add{};
            add.header.size = sizeof(add);
            add.header.type = PKT_TYPE::S_C_OBJECT_ADD;
            add.id = u._id;
            add.obj_type = ObjectType::PLAYER;
            add.skin_type = u._skin_type;
            strcpy_s(add.name, u._name.c_str());
            add.startposition = u._position;
            add.starthp = u._hp;
            add.gun_type = u._gun_type;
            add.act_type = u._act_type;
            add.move_input = u._move_input;

            this->do_send(&add);
        }

        // 2) 나에게: 기존 좀비들 ADD (스킨 테이블 일관성 유지)
        for (auto* zombie : g_zombies) {
            if (!zombie) continue;
            if (zombie->IsRemoved()) continue;

            pkt_sc_object_add add{};
            add.header.size = sizeof(add);
            add.header.type = PKT_TYPE::S_C_OBJECT_ADD;
            add.id = zombie->GetID();
            add.obj_type = ObjectType::ZOMBIE;

            auto sit = g_zombieSkin.find(zombie->GetID());
            add.skin_type = (sit != g_zombieSkin.end()) ? sit->second : 0;

            auto tit = g_zombieType.find(zombie->GetID());
            ZombieType zType = (tit != g_zombieType.end()) ? tit->second : ZombieType::NORMAL;

            if (zType == ZombieType::RUNNER)      strcpy_s(add.name, "Zombie_Runner"); 
            else if (zType == ZombieType::TANKER) strcpy_s(add.name, "Zombie_Tanker"); 
            else                                  strcpy_s(add.name, "Zombie");        

            add.startposition = zombie->GetPosition();
            add.starthp = zombie->GetHP();
            add.gun_type = static_cast<GunType>(0);

            this->do_send(&add);
        }
    }

    // // [SESSION::BroadcastAddMe] - 로딩 완료된 다른 유저에게만 "나" ADD 브로드캐스트
    void BroadcastAddMe()
    {
        pkt_sc_object_add me{};
        me.header.size = sizeof(me);
        me.header.type = PKT_TYPE::S_C_OBJECT_ADD;
        me.id = _id;
        me.obj_type = ObjectType::PLAYER;
        me.skin_type = _skin_type;
        strcpy_s(me.name, _name.c_str());
        me.startposition = _position;
        me.starthp = _hp;
        me.gun_type = _gun_type;
        me.act_type = _act_type;
        me.move_input = _move_input;

        std::vector<SESSION*> targets;
        GatherUserTargets(targets);


        for (SESSION* pu : targets) {
            SESSION& u = *pu;
            if (u._id == _id) continue;
            if (!u._is_loaded.load(std::memory_order_acquire)) continue; // 로딩중에게는 보내지 않음
            u.do_send(&me);
        }
    }

	void process_packet(SIZE2* packet) {

		const unsigned char packet_type = packet[1];
        if (packet_type == 0) {
            std::cout << "[ERROR] Invalid Packet Type\n";
            return;
        }
		//DEBUG_LOG("[process_packet] ID = " << _id << ", packet_type = " << (int)packet_type << "\n");

        switch (packet_type) {
        case ::PKT_TYPE::C_S_LOGIN:
        {
            pkt_cs_login* loginPacket = reinterpret_cast<pkt_cs_login*>(packet);
            _obj_type   = ObjectType::PLAYER;
            const int assigned = (IN_g_player_n % 3);
            _spawn_index = static_cast<SIZE1>(assigned);

            _skin_type = static_cast<SIZE1>(assigned);
            _name       = loginPacket->name;
            _position = START_POSITIONS[assigned];
            _velocity   = { 0.0f,0.0f, 0.0f };
            _look       = { 0.0f,0.0f, 0.0f };
            _pitch      = 0.0f;
            _hp         = PLAYER_HP;
			_gun_type   = GunType::BULLET_PISTOL; // 총 종류
            _level      = 1;
            _score      = 0;
            _damage     = 0;
			_act_type   = ActionType::IDLE;
            _move_input = 0;
            
            _ammo_max = GetMaxAmmo(_gun_type);  //  로그인 시 탄/리로드 상태 초기화(서버 권위)
            _ammo_cur = _ammo_max;
            _reloading = false;
            _reload_end_tp = std::chrono::steady_clock::time_point{};

            IN_g_player_n++;

			//DEBUG_LOG("[process_packet][RECV][" << (int)_id << "] C_S_LOGIN: " << _name << "\n");
            //DEBUG_LOG("[process_packet][RECV][" << (int)_id << "] C_S_LOGIN: " << _skin_type << "\n");
            
            _is_loaded.store(false, std::memory_order_release); // 로딩 완료 전
            send_obj_info();

            //send_object_update();
            break;
         
        }

        case PKT_TYPE::C_S_LOADING_FINISH:
        {
            _is_loaded.store(true); // 핵심

            SendSceneSnapshot();    // ← 이 시점부터만
            BroadcastAddMe();       // 다른 플레이어에게 나 add

            SendAmmoInfoToSelf(*this);

            BroadcastScoreInfoToAll();
            BroadcastStageInfoToAll(/*timeLeft=*/60000);
        }
        break;

        case PKT_TYPE::C_S_UPDATE:
        {
            if (!_is_loaded.load(std::memory_order_acquire)) break; // 로딩중엔 월드 영향 패킷 무시(텔포/꼬임 방지)

            {   // 죽음/리스폰 대기 중엔 이동/상태 업데이트 무시(텔포/부활 꼬임 방지)
                std::lock_guard<std::mutex> lk(g_usersMutex);
                if (_hp == 0 || _respawning) break;
            }

			pkt_cs_update* updatePacket = reinterpret_cast<pkt_cs_update*>(packet);

            {
                std::lock_guard<std::mutex> ulk(g_usersMutex); // C_S_UPDATE write lock

                _position = updatePacket->position;
                _velocity = updatePacket->velocity;
                _look = updatePacket->look;
                _pitch = updatePacket->pitch;
                //_hp = updatePacket->hp;
                _level = updatePacket->level;
                _score = updatePacket->score;
                _damage = updatePacket->damage;
                _gun_type = updatePacket->gun_type;
                _act_type = updatePacket->act_type;
                _move_input = updatePacket->move_input;
            }

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

            BroadcastScoreInfoToAll();

            //pkt_sc_score_info resp;
            //resp.header.size = sizeof(resp);
            //resp.header.type = PKT_TYPE::S_C_SCORE_INFO;
            //resp.stage_score = p->stage_score;
            //
            //for (auto& [id, session] : g_users)
            //    session.do_send(&resp);

            break;
        }

        case PKT_TYPE::C_S_STAGE_INFO:
        {
            auto* p = reinterpret_cast<pkt_cs_stage_info*>(packet);

            if (!validate_stage_info(p)) {
                std::cout << "[STAGE_INFO] 유효하지 않은 값 무시\n";
                break;
            }

            BroadcastStageInfoToAll(p->timeLeft);    //서버 권위 스테이지 정보 브로드캐스트

           /* pkt_sc_stage_info resp;
            resp.header.size = sizeof(resp);
            resp.header.type = PKT_TYPE::S_C_STAGE_INFO;
            resp.currentStage = p->currentStage;
            resp.totalStages = 1;
            resp.timeLeft = p->timeLeft;

            for (auto& [id, session] : g_users)
                session.do_send(&resp);*/

            break;
        }

        case PKT_TYPE::C_S_SHOOT:
        {
            auto* p = reinterpret_cast<pkt_cs_shoot*>(packet);

            
            if (_reloading) {   //  리로드 중 발사 금지
                SendAmmoInfoToSelf(*this);
                break;
            }

            if (_ammo_cur <= 0) {   // 탄 없으면 발사 금지
                SendAmmoInfoToSelf(*this);
                break;
            }

            --_ammo_cur;    // 발사 승인: 탄 감소(서버 권위)
            SendAmmoInfoToSelf(*this);

            float ox = p->bulletPos[0];  // XYZ 그대로 사용
            float oy = p->bulletPos[1];
            float oz = p->bulletPos[2];

            float dx = p->bulletDir[0];
            float dy = p->bulletDir[1];
            float dz = p->bulletDir[2];
            
            {   // [SESSION::process_packet] - DEBUG: 좌표계 검증
                std::lock_guard<std::mutex> lock(zombiesMutex);

                if (!g_zombies.empty() && g_zombies[0]) {
                    ZombieAI* z = g_zombies[0];

                    //std::cout
                    //    << "[COORD-DBG] shooter=" << _id
                    //    << " bulletPos=(" << ox << "," << oy << "," << oz << ")"
                    //    << " bulletDir=(" << dx << "," << dy << "," << dz << ")"
                    //    << " zombie0=(" << z->GetX() << "," << z->GetZ() << ")"
                    //    << "\n";
                }
            }

            {   // 발사 브로드캐스트(S_C_SHOOT) 
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

            constexpr float MAX_RANGE = 200.0f; // 임시 사거리

            int   hit_zid = -1;
            float hit_t = 0.0f;

            if (find_nearest_hit_zombie3D(ox, oy, oz, dx, dy, dz, MAX_RANGE, hit_zid, hit_t)) {
                //DEBUG_LOG("[HIT-TEST/3D] shooter=" << _id
                //   << " hit_zid=" << hit_zid << " t=" << hit_t);

                constexpr SIZE2 DAMAGE = GUN_DAMAGE;   // 임시 고정 대미지(총기별 테이블은 이후에 연결)

                /*std::cout << "[HIT-DBG] shooter=" << _id
                    << " hit_zid=" << hit_zid
                    << " t=" << hit_t
                    << " dmg=" << DAMAGE
                    << "\n";*/


                SIZE2 hp_after = 0;
                bool  died_now = false; 

                {
                    std::lock_guard<std::mutex> lock(zombiesMutex);
                    ZombieAI* hitZ = nullptr;
                    for (auto* z : g_zombies) {
                        if (z && z->GetID() == hit_zid) { hitZ = z; break; }
                    }

                    if (hitZ && !hitZ->IsRemoved()) {
                        const SIZE2 hp_before = hitZ->GetHP();               
                        hitZ->ApplyDamage(DAMAGE);                           
                        hp_after = hitZ->GetHP();                           
                        died_now = (hp_before > 0 && hp_after == 0);         
                    }
                }


                pkt_sc_hit_result resp{};    // (기존) 히트 결과 먼저 브로드캐스트
                resp.header.size = sizeof(resp);
                resp.header.type = PKT_TYPE::S_C_HIT_RESULT;
                resp.shooterId = _id;
                resp.zombieId = hit_zid;
                resp.zombieHp = hp_after;
                for (auto& [id, session] : g_users) session.do_send(&resp);

                if (died_now) {   
                    Server_KillZombie(hit_zid); 
                //    ZombieAI* hitZ = nullptr;
                //    {
                //        std::lock_guard<std::mutex> lock(zombiesMutex);
                //        for (auto* z : g_zombies) {
                //            if (z && z->GetID() == hit_zid) { hitZ = z; break; }
                //        }
                //    }
                //    if (hitZ && !hitZ->IsRemoved()) {
                //        hitZ->MarkRemoved(); // 서버 틱/충돌 제외

                //        // 좀비 킬 카운트/스코어 갱신(서버 권위)
                //        const SIZE2 killed_now = static_cast<SIZE2>(g_killed_zombies.fetch_add(1, std::memory_order_acq_rel) + 1);
                //        g_stage_score.fetch_add(1, std::memory_order_acq_rel); // 1킬 = 1점(필요 시 테이블화)

                //        BroadcastScoreInfoToAll();

                //        // Stage1 클리어 조건: killed == total
                //        const SIZE2 total = g_total_zombies.load(std::memory_order_acquire);
                //        if (total > 0 && killed_now >= total) {
                //            const bool first_clear = !g_stage1_cleared.exchange(true, std::memory_order_acq_rel);
                //            if (first_clear) {
                //                std::cout << "[STAGE_CLEAR] Stage1 cleared. killed=" << killed_now << " total=" << total << "\n";
                //                BroadcastStageInfoToAll(/*timeLeft=*/0);
                //            }
                //        }

                //        g_zombieSkin.erase(hit_zid); // erase zombie skin
                //        g_zombieType.erase(hit_zid);

                //        pkt_sc_object_remove rem{};
                //        rem.header.size = sizeof(rem);
                //        rem.header.type = PKT_TYPE::S_C_OBJECT_REMOVE;
                //        rem.id = hit_zid;

                //        for (auto& [id, session] : g_users)
                //            session.do_send(&rem);
                //    }
                }
            }
            else {
                //DEBUG_LOG("[HIT-TEST/3D] shooter=" << _id << " miss");
                // std::cout << "[MISS] shooter=" << _id << "\n";
            }
            break;
        }

        case PKT_TYPE::C_S_RELOAD:
        {
            auto* p = reinterpret_cast<pkt_cs_reload*>(packet);

            if (_reloading) {   // 리로드 시작(서버 권위)
                SendAmmoInfoToSelf(*this);
                break;
            }

            _ammo_max = GetMaxAmmo(_gun_type);
            if (_ammo_cur >= _ammo_max) {
                SendAmmoInfoToSelf(*this);
                break;
            }

            _reloading = true;
            _reload_end_tp = std::chrono::steady_clock::now()
                + std::chrono::milliseconds(GetReloadMs(_gun_type));

            SendAmmoInfoToSelf(*this);
            break;
        }

        case PKT_TYPE::C_S_RELOAD_FINISH:
        {
            auto* p = reinterpret_cast<pkt_cs_reload_finish*>(packet);

            if (!_reloading) {  // 리로드 종료 알림(서버 시간 검증)
                SendAmmoInfoToSelf(*this);
                break;
            }

            auto now = std::chrono::steady_clock::now();
            if (now < _reload_end_tp) {
                SendAmmoInfoToSelf(*this);  //조기 finish 방지
                break;
            }

            _ammo_max = GetMaxAmmo(_gun_type);
            _ammo_cur = _ammo_max;
            _reloading = false;

            SendAmmoInfoToSelf(*this);
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

    SESSION* s = nullptr;
    {
        std::lock_guard<std::mutex> lk(g_usersMutex); // // [g_recv_callback] - g_users 보호
        auto it = g_users.find(my_id);
        if (it == g_users.end()) {
            std::cout << "[ERROR] Invalid session ID: " << my_id << "\n";
            return;
        }
        s = &it->second;
    }
    s->recv_callback(num_bytes); 

}

static void GatherUserTargets(std::vector<SESSION*>& out)
{
    out.clear();
    std::lock_guard<std::mutex> lk(g_usersMutex);
    out.reserve(g_users.size());
    for (auto& [id, s] : g_users) out.push_back(&s);
}


static void Server_StartWave(SIZE1 wave) // 웨이브 시작(카운터 세팅 + 스폰 + 브로드캐스트)
{
    if (wave < 1 || wave > WAVE_TOTAL) return;

    g_current_wave.store(wave, std::memory_order_release);

    const SIZE2 count = WAVE_PLAN[wave - 1];
    g_wave_total_zombies.store(count, std::memory_order_release);
    g_wave_killed_zombies.store(0, std::memory_order_release);

    SpawnZombies(count); // 현재 웨이브 수만큼 스폰(스폰 분산 로직은 아래 C에서 교체)

    // // [Server_StartWave] - 웨이브 시작 디버그 출력
    std::cout << "[WAVE-START] wave=" << (int)wave
        << "/" << (int)WAVE_TOTAL
        << " spawn=" << count
        << "\n";

    BroadcastStageInfoToAll(/*timeLeft=*/60000);
    BroadcastScoreInfoToAll(); // 웨이브/스코어 스냅샷 전송
}

//====================================
// 서버 권위 본인에게만 탄/리로드 상태 전송
//====================================
static void SendAmmoInfoToSelf(SESSION& s)
{
    pkt_sc_ammo_info a{};
    a.header.size = sizeof(a);
    a.header.type = PKT_TYPE::S_C_AMMO_INFO;

    a.playerId = s._id;
    a.gun_type = s._gun_type;
    a.cur_ammo = s._ammo_cur;
    a.max_ammo = s._ammo_max;
    a.reloading = s._reloading ? 1 : 0;

    s.do_send(&a);
}

//====================================
// 서버 권위 스코어/좀비 카운터 전송
//====================================
static void BroadcastScoreInfoToAll()
{
    pkt_sc_score_info resp{};
    resp.header.size = sizeof(resp);
    resp.header.type = PKT_TYPE::S_C_SCORE_INFO;

    const SIZE2 wave_total = g_wave_total_zombies.load(std::memory_order_acquire);
    const SIZE2 wave_killed = g_wave_killed_zombies.load(std::memory_order_acquire);
    const SIZE2 wave_alive = (wave_killed <= wave_total) ? static_cast<SIZE2>(wave_total - wave_killed) : 0;


    resp.stage_score = g_stage_score.load(std::memory_order_acquire);

    resp.total_zombies = wave_total;
    resp.killed_zombies = wave_killed;
    resp.alive_zombies = wave_alive;

    // 추가 웨이브 필드
    resp.current_wave = g_current_wave.load(std::memory_order_acquire);
    resp.total_waves = WAVE_TOTAL;
    resp.wave_total_zombies = wave_total;
    resp.wave_killed_zombies = wave_killed;
    resp.wave_alive_zombies = wave_alive;

    std::vector<SESSION*> targets;
    GatherUserTargets(targets);
    for (SESSION* s : targets) s->do_send(&resp);
}
//====================================
// 스테이지 정보(현재는 Stage1 only)
//====================================
static void BroadcastStageInfoToAll(SIZE2 timeLeft)
{
    pkt_sc_stage_info resp{};
    resp.header.size = sizeof(resp);
    resp.header.type = PKT_TYPE::S_C_STAGE_INFO;

    resp.currentStage = g_current_stage.load(std::memory_order_acquire);
    resp.totalStages = 1;
    resp.timeLeft = timeLeft;

    std::vector<SESSION*> targets;
    GatherUserTargets(targets);
    for (SESSION* s : targets) s->do_send(&resp);
}

static void BroadcastPlayerFullUpdate(SIZEID victimId)
{
    pkt_sc_object_update u{};
    u.header.size = sizeof(u);
    u.header.type = PKT_TYPE::S_C_OBJECT_UPDATE;
    u.id = victimId;

    {   // // [BroadcastPlayerFullUpdate] - 피해/리스폰 시 0 덮어쓰기 방지: 전체 필드 채움
        std::lock_guard<std::mutex> lk(g_usersMutex);
        auto it = g_users.find(victimId);
        if (it == g_users.end()) return;
        SESSION& s = it->second;

        u.position = s._position;
        u.velocity = s._velocity;
        u.look = s._look;
        u.pitch = s._pitch;
        u.hp = s._hp;

        u.gun_type = s._gun_type;
        u.level = s._level;
        u.score = s._score;
        u.damage = s._damage;
        u.act_type = s._act_type;
        u.move_input = s._move_input;
    }

    std::vector<SESSION*> targets;
    GatherUserTargets(targets);
    for (SESSION* ps : targets) ps->do_send(&u);
}

static void BroadcastPlayerHpOnly(SIZEID pid) // HP만 브로드캐스트(방향/상태 불변)
{
    pkt_sc_player_hp_only p{};
    p.header.size = sizeof(p);
    p.header.type = PKT_TYPE::S_C_PLAYER_HP_ONLY;
    p.id = pid;

    {
        std::lock_guard<std::mutex> lk(g_usersMutex);
        auto it = g_users.find(pid);
        if (it == g_users.end()) return;
        p.hp = it->second._hp;
    }

    std::vector<SESSION*> targets;
    GatherUserTargets(targets);
    for (SESSION* s : targets) s->do_send(&p);
}


// ===============================
// Respawn (Server Authority)
// ===============================

// 죽음 처리(HP 0 확정 시 1회만 호출)
static void Server_OnPlayerDead(SIZEID pid) // 플레이어 죽음 처리 + 리스폰 예약
{
    std::lock_guard<std::mutex> lk(g_usersMutex);
    auto it = g_users.find(pid);
    if (it == g_users.end()) return;

    SESSION& s = it->second;
    if (!s._alive.load(std::memory_order_acquire)) return;
    if (!s._is_loaded.load(std::memory_order_acquire)) return;

    if (s._respawning) return;         // 중복 죽음 처리 방지
    if (s._hp != 0) return;            // HP 0에서만

    s._respawning = true;              // 리스폰 대기 시작
    s._respawn_end_tp = std::chrono::steady_clock::now()
        + std::chrono::milliseconds(PLAYER_RESPAWN_MS);


    //s._act_type = ActionType::DEATH;   // 상태를 죽음으로
    //s._velocity = { 0,0,0 };           // 정지

    auto remain = std::chrono::duration_cast<std::chrono::milliseconds>(s._respawn_end_tp - std::chrono::steady_clock::now()).count();
    std::cout << "[RESPAWN-SET] pid=" << pid << " remain_ms=" << remain << "\n";
}

// 주기 체크(ZombieAIThread에서 매 프레임 호출)
static void Server_TickRespawn() // 리스폰 타이밍 체크/처리
{
    const auto now = std::chrono::steady_clock::now();

    std::vector<SIZEID> toRespawn;
    {
        std::lock_guard<std::mutex> lk(g_usersMutex);
        for (auto& [id, s] : g_users) {
            if (s._obj_type != ObjectType::PLAYER) continue;
            if (!s._alive.load(std::memory_order_acquire)) continue;
            if (!s._is_loaded.load(std::memory_order_acquire)) continue;

            //if (s._respawn_end_tp == std::chrono::steady_clock::time_point{}) {
            //    //std::cout << "[RESPAWN-WARN] pid=" << id << " end_tp is ZERO (blocked)\n";
            //    continue;
            //}

            if (s._respawning && now >= s._respawn_end_tp) {
                toRespawn.push_back(id);
            }
        }
    }

    for (SIZEID pid : toRespawn) {
        {   // 상태 복구는 락 안에서
            std::lock_guard<std::mutex> lk(g_usersMutex);
            auto it = g_users.find(pid);
            if (it == g_users.end()) continue;

            SESSION& s = it->second;
            if (!s._respawning) continue;

            s._hp = PLAYER_RESPAWN_HP;                           // HP 복구
            //s._position = START_POSITIONS[s._spawn_index];     // 시작 위치로
            //s._velocity = { 0,0,0 };                           // 정지
            //s._act_type = ActionType::IDLE;                    // 기본 상태
            //s._move_input = 0;                                 // 입력 초기화

            s._respawning = false;                             // 리스폰 완료
            s._respawn_end_tp = std::chrono::steady_clock::time_point{};
        }

        BroadcastPlayerHpOnly(pid); // HP만
        std::cout << "[RESPAWN-DONE] pid=" << pid << "\n";
    }
}

// ===============================
// Zombie Kill / Remove (Single Authority)
// ===============================
static bool Server_KillZombie(SIZEID zombieId)  // 좀비 제거/킬카운트/스테이지 종료를 단일 처리
{
    ZombieAI* hitZ = nullptr;

    {
        std::lock_guard<std::mutex> lock(zombiesMutex);

        for (auto* z : g_zombies) {
            if (z && z->GetID() == zombieId) { hitZ = z; break; }
        }
        if (!hitZ) return false;
        if (hitZ->IsRemoved()) return false; // 이미 제거 처리된 좀비면 중복 금지
        if (!hitZ->IsDead()) return false;   // hp가 0이 아닐 수도 있으니 “죽은 상태에서만” 제거 처리 (안전)

        hitZ->MarkRemoved();  // 서버 틱/충돌 제외
    }

    // 여기부터는 “한 번만” 실행되는 영역이어야 함(위에서 IsRemoved로 가드됨)

    g_zombieSkin.erase(zombieId); // - 스킨/타입 테이블 정리
    g_zombieType.erase(zombieId);

    g_stage_score.fetch_add(1, std::memory_order_acq_rel);

    const SIZE2 wave_killed_now = static_cast<SIZE2>(g_wave_killed_zombies.fetch_add(1, std::memory_order_acq_rel) + 1);    //웨이브 킬 카운트 증가


    BroadcastScoreInfoToAll();

    // // Server_KillZombie: 웨이브 클리어 체크
    const SIZE2 wave_total = g_wave_total_zombies.load(std::memory_order_acquire);
    if (wave_total > 0 && wave_killed_now >= wave_total)
    {
        const SIZE1 wave = g_current_wave.load(std::memory_order_acquire);

        if (wave < WAVE_TOTAL) {
            Server_StartWave(static_cast<SIZE1>(wave + 1)); // 다음 웨이브 시작
        }
        else {
            // 마지막 웨이브 클리어 → 스테이지 클리어 처리
            const bool first_clear = !g_stage1_cleared.exchange(true, std::memory_order_acq_rel);
            if (first_clear) {
                std::cout << "[STAGE_CLEAR] All waves cleared.\n";
                BroadcastStageInfoToAll(/*timeLeft=*/0);
            }
        }
    }

    pkt_sc_object_remove rem{};
    rem.header.size = sizeof(rem);
    rem.header.type = PKT_TYPE::S_C_OBJECT_REMOVE;
    rem.id = zombieId;

    std::vector<SESSION*> targets;
    GatherUserTargets(targets);
    for (SESSION* s : targets) s->do_send(&rem);

    return true;
}


auto lastTick = std::chrono::steady_clock::now();

std::vector<std::pair<int, int>> spawnPoints = {    // 요구했던 두 포인트 + 예시 포인트 1~2개 더 (원하는 만큼 2~4개만 채워서 사용)
    {150, 180},  // 포인트 A
    {150, 100},  // 포인트 B
    // {200, 300},  // 포인트 C (원하면 활성화)
    // {400, 420},  // 포인트 D (원하면 활성화)
};

// ===============================
// Center Flat Spawn Utilities
// ===============================

// PrintMap2 기준: 0=평지(이동가능), 1=벽(막힘)
static inline bool IsWalkableCell(int v)
{
    return (v == 0);
}

static inline bool InRangeCell(int x, int z)
{
    const int H = static_cast<int>(g_map.size());
    const int W = (H > 0) ? static_cast<int>(g_map[0].size()) : 0;
    return (x >= 0 && z >= 0 && z < H && x < W);
}
// g_map 기준 중앙 셀(x,z) 반환
static inline std::pair<int, int> GetMapCenterCell()
{
    const int H = static_cast<int>(g_map.size());
    const int W = (H > 0) ? static_cast<int>(g_map[0].size()) : 0;
    return { W / 2, H / 2 }; // (x,z)
}

// 중앙 기준 도넛(내/외반경) 영역에서 평지 후보 셀 수집
static std::vector<std::pair<int, int>> CollectCenterCandidates(
    int cx, int cz,
    int minR, int maxR,
    int limit)
{
    std::vector<std::pair<int, int>> out;
    out.reserve((std::min)(limit, 20000));

    const int H = static_cast<int>(g_map.size());
    const int W = (H > 0) ? static_cast<int>(g_map[0].size()) : 0;
    if (H <= 0 || W <= 0) return out;

    const int minR2 = minR * minR;
    const int maxR2 = maxR * maxR;

    for (int dz = -maxR; dz <= maxR; ++dz) {
        for (int dx = -maxR; dx <= maxR; ++dx) {
            const int x = cx + dx;
            const int z = cz + dz;
            if (!InRangeCell(x, z)) continue;

            const int r2 = dx * dx + dz * dz;
            if (r2 < minR2 || r2 > maxR2) continue;

            if (!IsWalkableCell(g_map[z][x])) continue;

            out.emplace_back(x, z);
            if (static_cast<int>(out.size()) >= limit) return out;
        }
    }
    return out;
}

//  후보에서 최소거리(minDistCell) 유지하며 분산 선택
static std::vector<std::pair<int, int>> PickSpreadPoints(
    std::vector<std::pair<int, int>>& candidates,
    int need,
    int minDistCell)
{
    std::shuffle(candidates.begin(), candidates.end(), g_rng);

    std::vector<std::pair<int, int>> picked;
    picked.reserve(need);

    const int md2 = minDistCell * minDistCell;

    auto dist2 = [](int x1, int z1, int x2, int z2) {
        const int dx = x1 - x2;
        const int dz = z1 - z2;
        return dx * dx + dz * dz;
        };

    for (auto& p : candidates) {
        bool ok = true;
        for (auto& q : picked) {
            if (dist2(p.first, p.second, q.first, q.second) < md2) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;

        picked.push_back(p);
        if (static_cast<int>(picked.size()) >= need) break;
    }

    return picked;
}


void SpawnZombies(int count) {  // N등분 스폰 적용 (GetSpawnPointByIndexN)
        
    //if (spawnPoints.empty()) {  // 스폰 포인트 미설정 시 기본 포인트 두 개로 세팅                                       
    //    spawnPoints = { {150,180}, {150,100} };          
    //}
    //  Stage1 카운터 초기화(서버 권위)
    /*g_current_stage.store(1, std::memory_order_release);
    g_stage1_cleared.store(false, std::memory_order_release);
    g_stage_score.store(0, std::memory_order_release);
    g_total_zombies.store(static_cast<SIZE2>(count), std::memory_order_release);
    g_killed_zombies.store(0, std::memory_order_release);*/

    // -------------------------------
    // // [SpawnZombies] - 중앙 평지 후보 수집 + 분산 스폰(몰림 방지)
    // -------------------------------
    auto [ccx, ccz] = GetMapCenterCell(); //  맵 중앙 셀

    // 튜닝 포인트 (셀 단위)
    // CELL_SIZE ≈ 0.488 기준 튜닝(셀 단위)
    // - 중앙에서 20m~70m 사이에 뿌림 + 최소 4m 간격
    constexpr int CAND_LIMIT = 50000;   // 후보 최대
    constexpr int MIN_R = 40;           // 중앙에서 너무 붙지 않게(도넛 내반경)
    constexpr int MAX_R = 140;          // 중앙 기준 외반경
    constexpr int MIN_DIST_CELL = 8;    // 서로 최소 거리(셀) - 몰림 방지 핵심

    auto candidates = CollectCenterCandidates(ccx, ccz, MIN_R, MAX_R, CAND_LIMIT);


    std::vector<std::pair<int, int>> spawnCells;        // 후보가 너무 적으면: 거리조건 완화 폴백
    if (static_cast<int>(candidates.size()) >= count) {
        spawnCells = PickSpreadPoints(candidates, count, MIN_DIST_CELL); // 분산 선택
    }
    else {
        spawnCells = candidates; // 후보 자체가 적음 → 있는 만큼이라도 사용
    }

    if (static_cast<int>(spawnCells.size()) < count) {  // 그래도 부족하면: MIN_DIST를 완화해서 다시 시도
        auto candidates2 = CollectCenterCandidates(ccx, ccz, /*minR*/10, /*maxR*/200, CAND_LIMIT); // 폴백 후보 확장
        spawnCells = PickSpreadPoints(candidates2, count, /*minDistCell*/2); //  폴백 거리완화
    }

    while (static_cast<int>(spawnCells.size()) < count) {   // 최종 폴백: 그래도 부족하면 중앙에라도 채움(절대 터지지 않게)
        spawnCells.push_back({ ccx, ccz }); // 최후 폴백(중앙)
    }


    for (int i = 0; i < count; ++i) {

        //auto [sx, sz] = GetSpawnPointByIndexN(g_map, spawnPoints, i, count);  //균등 분할로 i번째 스폰 좌표 선택

        // SpawnZombies: 좌표계 선택 , 프로젝트가 '셀 인덱스' 좌표를 SetPosition에 기대하면 아래 1줄 사용:
        //float zx = static_cast<float>(sx);                          
        //float zz = static_cast<float>(sz);                         

        // 월드 좌표(CELL_SIZE 배수)를 SetPosition에 기대한다면 위 2줄 대신 아래 2줄 사용:
        // float zx = static_cast<float>(sx) * CELL_SIZE;          
        // float zz = static_cast<float>(sz) * CELL_SIZE;           

        const auto [sx, sz] = spawnCells[i]; // 분산 스폰 셀 좌표

        const float zx = static_cast<float>(sx) * CELL_SIZE; // 셀→월드 변환
        const float zz = static_cast<float>(sz) * CELL_SIZE; // 

        const SIZEID zid = g_nextZombieId.fetch_add(1, std::memory_order_relaxed); // 유니크 좀비 ID 발급
        ZombieAI* zombie = new ZombieAI(g_map, zid);
        zombie->SetPosition(zx, zz);

        ZombieType zType = static_cast<ZombieType>(g_zombieTypeDist(g_rng));    // 타입 랜덤 결정 + 스탯 적용(HP/이속/쿨/데미지)
        
        float speedMul = 1.0f;
        if (zType == ZombieType::NORMAL)      speedMul = g_speedMulNormal(g_rng); // NORMAL
        else if (zType == ZombieType::RUNNER) speedMul = g_speedMulRunner(g_rng); // RUNNER
        else if (zType == ZombieType::TANKER) speedMul = g_speedMulTanker(g_rng); // TANKER

        zombie->ApplySpeedRandomMul(speedMul); // 개체 속도 배율 저장(랜덤은 여기서만)
        zombie->SetType(zType);                // 타입 기본 스탯 적용 + 배율 반영(SetType 내부)

        {
            std::lock_guard<std::mutex> lock(zombiesMutex); // g_zombies push_back 동기화(웨이브 전환 안전)
            g_zombies.push_back(zombie);
        }
        g_zombieType[zombie->GetID()] = zType; // 타입 테이블 저장(늦접 스냅샷 일관성)

        const SIZE1 Z_Random_skin = static_cast<SIZE1>(g_zombieSkinDist(g_rng));    // - random skin
        g_zombieSkin[zombie->GetID()] = Z_Random_skin;                              // - save skin by id

        // // SpawnZombies: 생성 즉시 대상 지정/경로 탐색을 원하면 필요 시 활성화
        // zombie->SetTargetPosition(player_x, player_z);          
        // zombie->FindPath();                                      

        // 좀비 정보를 모든 플레이어에게 전송 (기존 유지)
        pkt_sc_object_add p;
        p.header.size = sizeof(p);
        p.header.type = PKT_TYPE::S_C_OBJECT_ADD;
        p.id = zombie->GetID();
        p.obj_type = ObjectType::ZOMBIE;
        p.skin_type = Z_Random_skin;

        if (zType == ZombieType::RUNNER)      strcpy_s(p.name, "Zombie_Runner"); 
        else if (zType == ZombieType::TANKER) strcpy_s(p.name, "Zombie_Tanker"); 
        else                                  strcpy_s(p.name, "Zombie");        

        p.startposition = zombie->GetPosition();
        p.starthp = zombie->GetHP();
        //p.gun_type = GunType::BULLET_MAX; 
        p.gun_type = static_cast<GunType>(0);

        for (auto& [id, session] : g_users) {
            if (!session._is_loaded.load(std::memory_order_acquire)) continue;
            session.do_send(&p);
        }
            
    }

    BroadcastStageInfoToAll(/*timeLeft=*/60000); // Stage / Score 스냅샷 최초 전송
    BroadcastScoreInfoToAll();
}


void ZombieAIThread() {
    while (serverRunning) {
        //----------
        static auto s_last = std::chrono::steady_clock::now(); // DEBUG(루프 간격)
        auto now2 = std::chrono::steady_clock::now();
        auto gap = std::chrono::duration_cast<std::chrono::milliseconds>(now2 - s_last).count();
        s_last = now2;
        if (gap > 200) { // DEBUG(루프 자체가 200ms 이상 멈춤)
            std::cout << "[ZDBG][LoopGap] gap_ms=" << gap << "\n";
        }
        //----------

        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> dt = now - lastTick;
        lastTick = now;
        float deltaTime = dt.count();  // 초 단위
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // // [ZombieAIThread] - Wave 남은 좀비 수 주기 디버그(1초마다)
        static float s_wave_dbg_acc = 0.0f;
        s_wave_dbg_acc += deltaTime;
        if (s_wave_dbg_acc >= 5.0f) {
            s_wave_dbg_acc = 0.0f;

            const SIZE1 wave = g_current_wave.load(std::memory_order_acquire);
            const SIZE2 total = g_wave_total_zombies.load(std::memory_order_acquire);
            const SIZE2 killed = g_wave_killed_zombies.load(std::memory_order_acquire);
            const SIZE2 alive = (killed <= total) ? (total - killed) : 0;

            std::cout << "[WAVE-DBG] wave=" << (int)wave
                << "/" << (int)WAVE_TOTAL
                << " total=" << total
                << " killed=" << killed
                << " alive=" << alive
                << " stageCleared=" << (g_stage1_cleared.load(std::memory_order_acquire) ? 1 : 0)
                << "\n";
        }

        // 플레이어 스냅샷: ID와 위치 동시 수집
        std::vector<Vec3> playerPositions;
        std::vector<std::pair<SIZEID, Vec3>> playerList;  // ID 포함

        //for (auto& [id, session] : g_users) {
        //    if (session._obj_type != ObjectType::PLAYER) continue;

        //    if (!session._is_loaded.load(std::memory_order_acquire)) continue;  // 로딩중 플레이어는 좀비 인지 대상에서 제외

        //    playerPositions.push_back(session._position);
        //    playerList.emplace_back(id, session._position);
        //}

        Server_TickRespawn(); // 리스폰 주기 처리

        {
            std::lock_guard<std::mutex> ulk(g_usersMutex); // 플레이어 스냅샷은 락 걸고 수집(데이터 레이스 방지)
            for (auto& [id, session] : g_users) {
                if (session._obj_type != ObjectType::PLAYER) continue;
                if (!session._is_loaded.load(std::memory_order_acquire)) continue; // 로딩중 제외
                if (session._respawning || session._hp == 0) continue;
                playerPositions.push_back(session._position);
                playerList.emplace_back(id, session._position);
                
            }
        }
        
        //  플레이어 스냅샷이 갱신되는지 확인(2초마다)
        static float s_player_dbg = 0.0f; // DEBUG 누적
        s_player_dbg += deltaTime;        // DEBUG 누적
        if (s_player_dbg >= 2.0f) {       // DEBUG 출력 주기
            s_player_dbg = 0.0f;
            if (!playerPositions.empty()) {
                /*std::cout << "[ZDBG][Players] n=" << playerPositions.size()
                     << " p0=(" << playerPositions[0].x << "," << playerPositions[0].z << ")\n";*/
            }
            else {
                //std::cout << "[ZDBG][Players] n=0 (ALL FILTERED?)\n";
            }
        }

        // ==========================================================
        // 스냅샷 생성(락 최소화: 여기서만 잠깐)
        // ==========================================================
        std::vector<ZombieAI*> zombiesSnapshot; 
        {
            std::lock_guard<std::mutex> lock(zombiesMutex); 
            zombiesSnapshot = g_zombies;                    
        }
        
        for (auto* zombie : zombiesSnapshot) {

            if (zombie->IsRemoved()) continue;

            //zombie->Update(playerPositions, g_zombies, deltaTime);
            // ZombieAIThread - 제거 플래그면 완전 스킵

            //----------
            auto t0 = std::chrono::steady_clock::now(); // DEBUG(Update 시간 측정)
            zombie->Update(playerPositions, zombiesSnapshot, deltaTime);
            auto t1 = std::chrono::steady_clock::now(); // DEBUG(Update 시간 측정)

            if (zombie->ConsumeAttackHit())
            {
                if (!playerList.empty() && !zombie->IsDead() && !zombie->IsRemoved())
                {
                    const float zx = zombie->GetX();
                    const float zz = zombie->GetZ();

                    // 1) 가장 가까운 플레이어 선정(스냅샷 기반)
                    SIZEID bestPid = 0;
                    float bestD2 = FLT_MAX;

                    for (auto& [pid, pos] : playerList) {
                        const float dx = pos.x - zx;
                        const float dz = pos.z - zz;
                        const float d2 = dx * dx + dz * dz;
                        if (d2 < bestD2) { bestD2 = d2; bestPid = pid; }
                    }

                    // 2) 사거리 체크(좀비쪽 effectiveAttackRange와 동일한 취지)
                    const float attackRange = (std::max)(Z_ATTACK_RANGE, CELL_SIZE * 2.0f);
                    if (bestD2 <= attackRange * attackRange)
                    {
                        const SIZE2 dmg = zombie->GetDamage();

                        bool applied = false;
                        bool diedNow = false;

                        {   // 피해 적용(HP 감소는 1회만) + 죽음 감지
                            std::lock_guard<std::mutex> lk(g_usersMutex);
                            auto it = g_users.find(bestPid);
                            if (it != g_users.end()) {
                                SESSION& v = it->second;

                                if (v._alive.load(std::memory_order_acquire) &&
                                    v._is_loaded.load(std::memory_order_acquire) &&
                                    !v._respawning && v._hp > 0) // 죽은/리스폰 대기 중이면 추가 피해 금지
                                {
                                    const SIZE2 before = v._hp;
                                    v._hp = (before > dmg) ? (before - dmg) : 0;

                                    if (before > 0 && v._hp == 0) {
                                        v._act_type = ActionType::DEATH; // DEATH 반영
                                        diedNow = true;                  // 죽음 감지
                                    }
                                    else {
                                        // v._act_type = ActionType::HIT; // 원하면 피격 상태
                                    }

                                    applied = true;
                                }
                            }
                        }

                        // 3) 변경 브로드캐스트(HP-only 금지 → 풀 업데이트)
                        if (applied) {
                            BroadcastPlayerHpOnly(bestPid);
                        }

                        // 죽음이면 리스폰 예약(락 밖에서)
                        if (diedNow) {
                            Server_OnPlayerDead(bestPid); // 리스폰 예약
                        }
                    }
                }
            }

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count(); // // ZombieAIThread - DEBUG(Update ms)
            if (ms > 30) { // /DEBUG(한 좀비 Update가 30ms 이상이면 경고)
                std::cout << "[ZDBG][SlowUpdate] zid=" << zombie->GetID()
                    << " ms=" << ms
                    << " playerN=" << playerPositions.size()
                    << "\n";
            }
            //----------

            if (zombie->IsDirty()) {
                // 프레임 경합 방어: DEAD 상태면 업데이트 대신 제거 패킷
                if (zombie->IsDead()) {
                    const SIZEID zid = zombie->GetID();

                    zombie->ClearDirty();                  
                    Server_KillZombie(zid);                
                    continue;
                }
                //if (zombie->IsDead()) {
                //    zombie->MarkRemoved();

                //    // // ZombieAIThread - remove 시 스킨 테이블 정리
                //    g_zombieSkin.erase(zombie->GetID());
                //    g_zombieType.erase(zombie->GetID());

                //    pkt_sc_object_remove rem{};
                //    rem.header.size = sizeof(rem);
                //    rem.header.type = PKT_TYPE::S_C_OBJECT_REMOVE;
                //    rem.id = zombie->GetID();
                //    for (auto& [id, session] : g_users) session.do_send(&rem);

                //    zombie->ClearDirty();
                //    continue;
                //}

                // 일반 업데이트 브로드캐스트
                Object info = zombie->GetObjectinfo();

                // // ZombieAIThread - DEBUG(송신값 확인): 좀비0만 0.5초마다 출력
                //static float s_net_dbg_accum = 0.0f;                     // // ZombieAIThread - DEBUG 누적
                //s_net_dbg_accum += deltaTime;                            // // ZombieAIThread - DEBUG 누적
                //static int s_watch_id = -1;                           // // ZombieAIThread - DEBUG(감시할 좀비 id 1마리)
                //if (s_watch_id == -1) s_watch_id = zombie->GetID();  // // ZombieAIThread - DEBUG(처음 만난 좀비 id로 고정)

                //if (zombie->GetID() == s_watch_id && s_net_dbg_accum >= 0.5f) {   // // ZombieAIThread - DEBUG(좀비0만)
                //    s_net_dbg_accum = 0.0f;
                //    std::cout
                //        << "[ZDBG][Send ] id=" << zombie->GetID()
                //        << " pos=(" << info.position.x << "," << info.position.z << ")"
                //        << " act=" << (int)info.act_type
                //        << "\n";
                //}

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
                

               /* std::cout << "[ZOMBIE INFO] name=Zombie_" << p.id
                    << " act_type=" << p.act_type
                    << "(" << ToString((ActionType)p.act_type) << ")"
                    << "\n";*/


                std::vector<SESSION*> targets;
                GatherUserTargets(targets);
                for (SESSION* ps : targets) ps->do_send(&p);

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

    Server_StartWave(1);
    //SpawnZombies(MAX_ZOMBIE_COUNT);

    SIZEID clientId = 1;
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
