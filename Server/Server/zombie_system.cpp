#include "zombie_system.h"
#include "../../protocol.h"
#include "session.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <iostream>

#define DEBUG_PRINT false
#define DEBUG_LOG(msg) do { if (DEBUG_PRINT) std::cout << msg << std::endl; } while (0)


extern std::vector<ZombieAI*> g_zombies;
extern std::unordered_map<SIZEID, std::shared_ptr<GameObject>> g_gameObjects;
extern std::vector<std::vector<int>> g_map;
extern bool serverRunning;

const double FRAME_INTERVAL_MS = 1000.0 / 60.0;

auto lastTick = std::chrono::steady_clock::now();

void SpawnZombies(int count) {
    for (int i = 0; i < count; ++i) {
        auto [x, z] = GetRandomPosition(g_map);
        ZombieAI* zombie = new ZombieAI(g_map, 10000 + i);
        zombie->SetPosition((float)x, (float)z);
        zombie->SetHP(ZOMBIE_HP);
        g_zombies.push_back(zombie);

        DEBUG_LOG("[Zombie] 생성 완료: ID = " << zombie->GetID()
            << ", Pos = (" << x << ", " << z << ")");

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

        for (auto& [id, session] : g_gameObjects)
            session->do_send(&p);
    }

    DEBUG_LOG("[SpawnZombies] 현재 총 좀비 수 = " << g_zombies.size());
}


void ZombieAIThread() {
    while (serverRunning) {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float> dt = now - lastTick;
        lastTick = now;
        float deltaTime = dt.count();

        std::vector<Vec3> playerPositions;
        for (auto& [id, session] : g_gameObjects) {
            if (session->_obj_type == ObjectType::PLAYER)
                playerPositions.push_back(session->_position);
        }
        //DEBUG_LOG("[ZombieAIThread] 틱 시작");
        for (auto& zombie : g_zombies) {

            zombie->Update(playerPositions, g_zombies, deltaTime);
            //DEBUG_LOG("[ZombieAIThread] 업데이트 대상 ID = " << zombie->GetID());
            if (zombie->IsDirty()) {
                Object info = zombie->GetObjectinfo();

                //DEBUG_LOG("[ZombieAIThread] 좀비 Dirty → 브로드캐스트 ID = " << zombie->GetID());

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

                for (auto& [id, session] : g_gameObjects)
                    session->do_send(&p);

                zombie->ClearDirty();
            }
        }

        std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(FRAME_INTERVAL_MS));
    }
}

