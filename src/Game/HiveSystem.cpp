

#include "HiveSystem.h"
#include "GameConfig.h"
#include "ZombieSystem.h"
#include "NavGrid.h"
#include "AttackSystem.h"  
#include "CameraSystem.h"
#include "MathUtils.h"
#include "RenderUtils.h"
#include "../ContestAPI/app.h"

#include <cmath>
#include <algorithm>

using namespace GameConfig;


namespace
{
    float gHiveAnimTimeSec = 0.0f;
}


void HiveSystem::Init()
{
    hives.clear();

    std::srand(HiveConfig::PLACEMENT_SEED);

    const float worldMin = HiveConfig::WORLD_MIN;
    const float worldMax = HiveConfig::WORLD_MAX;
    const float margin = HiveConfig::PLACEMENT_MARGIN;
    const float minDist = HiveConfig::MIN_HIVE_DISTANCE;
    const int hiveCount = HiveConfig::HIVE_COUNT;
    const int maxAttempts = HiveConfig::MAX_PLACEMENT_ATTEMPTS;

    float placedX[HiveConfig::HIVE_COUNT];
    float placedY[HiveConfig::HIVE_COUNT];
    int placed = 0;

    for (int i = 0; i < hiveCount; i++)
    {
        const bool isBossHive = (i == hiveCount - 1);
        const float radius = isBossHive ? HiveConfig::BOSS_HIVE_RADIUS : HiveConfig::NORMAL_HIVE_RADIUS;
        const float hp = isBossHive ? HiveConfig::BOSS_HIVE_HP : HiveConfig::NORMAL_HIVE_HP;

        bool found = false;

        for (int attempt = 0; attempt < maxAttempts; attempt++)
        {
            const float x = MathUtils::RandRange(worldMin + margin, worldMax - margin);
            const float y = MathUtils::RandRange(worldMin + margin, worldMax - margin);

            bool ok = true;
            for (int j = 0; j < placed; j++)
            {
                if (MathUtils::DistanceSq(x, y, placedX[j], placedY[j]) < (minDist * minDist))
                {
                    ok = false;
                    break;
                }
            }

            if (!ok)
                continue;

            AddHive(x, y, radius, hp);
            placedX[placed] = x;
            placedY[placed] = y;
            placed++;
            found = true;
            break;
        }

        if (!found)
        {
            const float x = MathUtils::RandRange(worldMin + margin, worldMax - margin);
            const float y = MathUtils::RandRange(worldMin + margin, worldMax - margin);
            AddHive(x, y, radius, hp);
            placedX[placed] = x;
            placedY[placed] = y;
            placed++;
        }
    }
}

void HiveSystem::AddHive(float x, float y, float radius, float hp)
{
    Hive h;
    h.x = x;
    h.y = y;
    h.radius = radius;
    h.hp = hp;
    h.maxHp = hp;
    h.alive = true;
    h.spawnPerMin = HiveConfig::SPAWN_PER_MINUTE;
    h.spawnAccum = 0.0f;

    hives.push_back(h);
}

int HiveSystem::AliveCount() const
{
    int count = 0;
    for (const Hive& h : hives)
    {
        if (h.alive)
            count++;
    }
    return count;
}

void HiveSystem::Update(float deltaTimeMs, ZombieSystem& zombies, const NavGrid& nav)
{
    const float dt = deltaTimeMs * HiveConfig::MS_TO_SEC;

    gHiveAnimTimeSec += deltaTimeMs * HiveConfig::MS_TO_SEC;
    if (gHiveAnimTimeSec > HiveConfig::ANIM_TIME_RESET)
        gHiveAnimTimeSec = 0.0f;

    if (dt <= 0.0f)
        return;

    for (Hive& h : hives)
    {
        if (!h.alive)
            continue;

        const float spawnPerSec = h.spawnPerMin / HiveConfig::SECONDS_PER_MINUTE;
        h.spawnAccum += spawnPerSec * dt;

        if (h.spawnAccum > HiveConfig::MAX_SPAWN_ACCUM)
            h.spawnAccum = HiveConfig::MAX_SPAWN_ACCUM;

        while (h.spawnAccum >= 1.0f)
        {
            if (!zombies.CanSpawnMore(1))
                return;

            bool spawned = false;

            for (int tries = 0; tries < HiveConfig::SPAWN_PLACEMENT_ATTEMPTS; tries++)
            {
                const float ang = MathUtils::Rand01() * HiveConfig::TWO_PI;
                const float rMin = h.radius + HiveConfig::SPAWN_RADIUS_MIN_OFFSET;
                const float rMax = h.radius + HiveConfig::SPAWN_RADIUS_MAX_OFFSET;
                const float rr = rMin + (rMax - rMin) * MathUtils::Rand01();

                const float sx = h.x + std::cos(ang) * rr;
                const float sy = h.y + std::sin(ang) * rr;

                if (nav.IsBlockedWorld(sx, sy))
                    continue;

                if (zombies.SpawnAtWorld(sx, sy))
                {
                    spawned = true;
                    break;
                }
            }

            if (!spawned)
                break;

            h.spawnAccum -= 1.0f;
        }
    }
}
bool HiveSystem::DamageHiveAt(float wx, float wy, float hitRadius, float damage, 
                               AttackSystem* attacks, CameraSystem* camera)
{
    bool hitAny = false;

    for (Hive& h : hives)
    {
        if (!h.alive)
            continue;

        const float combinedRadius = h.radius + hitRadius;
        if (MathUtils::DistanceSq(wx, wy, h.x, h.y) <= combinedRadius * combinedRadius)
        {
            const bool wasAlive = h.alive;
            h.hp -= damage;
            hitAny = true;

            if (h.hp <= 0.0f)
            {
                h.hp = 0.0f;
                h.alive = false;

                if (wasAlive && attacks && camera)
                {
                    attacks->TriggerHiveExplosion(h.x, h.y, h.radius, *camera);
                }
            }
        }
    }

    return hitAny;
}


void HiveSystem::Render(float camOffX, float camOffY) const
{
    const float time = gHiveAnimTimeSec;

    for (const Hive& h : hives)
    {
        if (!h.alive)
            continue;

        const float sx = h.x - camOffX;
        const float sy = h.y - camOffY;
        const float R = h.radius;

        const float pulse = HiveConfig::PULSE_BASE +
            HiveConfig::PULSE_AMP * std::sinf(time * HiveConfig::PULSE_FREQUENCY);

        RenderUtils::DrawCircleLines(sx, sy, R, 1.0f, 0.95f, 0.20f);
        RenderUtils::DrawCircleLines(sx, sy, R + HiveConfig::PULSE_RING_OFFSET + pulse * HiveConfig::PULSE_RING_SIZE,
            1.0f, 0.85f, 0.10f);

        RenderUtils::DrawSpokeRing(sx, sy, R * HiveConfig::SPOKE_RADIUS_MULT, HiveConfig::SPOKE_LENGTH,
            1.0f, 0.55f, 0.10f, HiveConfig::SPOKE_COUNT,
            time * HiveConfig::SPOKE_ROTATION_SPEED);

        RenderUtils::DrawCircleLines(sx, sy, R * HiveConfig::INNER_RING_1_MULT, 1.0f, 0.85f, 0.10f);
        RenderUtils::DrawCircleLines(sx, sy, R * HiveConfig::INNER_RING_2_MULT, 1.0f, 0.70f, 0.05f);

        RenderUtils::DrawArc(sx, sy, R * HiveConfig::INNER_RING_1_MULT,
            time * HiveConfig::ARC_1_SPEED,
            time * HiveConfig::ARC_1_SPEED + HiveConfig::ARC_1_LENGTH,
            1.0f, 0.95f, 0.20f);

        RenderUtils::DrawArc(sx, sy, R * HiveConfig::INNER_RING_2_MULT,
            -time * HiveConfig::ARC_2_SPEED,
            -time * HiveConfig::ARC_2_SPEED + HiveConfig::ARC_2_LENGTH,
            1.0f, 0.70f, 0.10f);

        const float hpT = (h.maxHp > 0.0f) ? std::clamp(h.hp / h.maxHp, 0.0f, 1.0f) : 0.0f;
        const float barW = HiveConfig::HP_BAR_WIDTH;
        const float barY = sy - R - HiveConfig::HP_BAR_OFFSET_Y;

        App::DrawLine(sx - barW * 0.5f, barY, sx + barW * 0.5f, barY,
            0.05f, 0.07f, 0.10f);

        App::DrawLine(sx - barW * 0.5f, barY, sx - barW * 0.5f + barW * hpT, barY,
            0.10f, 1.00f, 0.10f);

        App::DrawLine(sx - barW * 0.5f, barY - 1.0f, sx + barW * 0.5f, barY - 1.0f,
            0.70f, 0.90f, 1.00f);
    }
}