#include "CollisionManager.h"
#include "ZombieAI.h"

extern std::vector<ZombieAI*> g_zombies;

std::vector<ZombieHit> CollisionManager::ProcessBulletCollisionAndDamage(
    const Vec3& origin, // 총알 발사 위치(origin)
	const Vec3& dir,    // 방향(dir)
	GunType gunType,    // 총 종류(gunType)을 기반으로
    std::vector<ZombieAI*>& zombies // 맞은 좀비 리스트 반환
) {
    std::vector<ZombieHit> hitResults; // 맞은 좀비 리스트

    const BulletInfo& bulletInfo = BULLET_TABLE[gunType];
    float radiusSq = bulletInfo.radius * bulletInfo.radius; // 총알의 충돌 반경 정보

    for (ZombieAI* zombie : zombies)
    {
        if (zombie->IsDead()) continue;

        Vec3 toZombie = zombie->GetPosition() - origin;
        float projection = toZombie.x * dir.x + toZombie.z * dir.z;
        if (projection < 0) continue; // 총알 뒤쪽

        Vec3 closestPoint = origin + dir * projection;
        float distSq = (zombie->GetPosition() - closestPoint).LengthSquared();

        if (distSq <= radiusSq) {
            zombie->ApplyDamage(bulletInfo.damage);

            ZombieHit hit;
            hit.zombieId = zombie->GetID();
            hit.hp = zombie->GetHP();
            hit.damage = bulletInfo.damage;
            hit.isDead = zombie->IsDead();

            hitResults.push_back(hit);
        }
    }

    return hitResults;
}

