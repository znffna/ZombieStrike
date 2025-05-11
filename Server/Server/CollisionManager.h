#pragma once
#include <vector>
#include "../../protocol.h"

class ZombieAI;

class CollisionManager {
public:
    // 총알 충돌 + 데미지 적용을 한 번에 처리하고,
    // 결과(ZombieHit)만 리턴 (패킷 전송용)
    static std::vector<ZombieHit> ProcessBulletCollisionAndDamage(
        const Vec3& origin,
        const Vec3& dir,
        GunType gunType,
        std::vector<ZombieAI*>& zombies
    );
};


