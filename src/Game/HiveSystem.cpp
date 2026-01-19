
#include "HiveSystem.h"
#include "GameConfig.h"
#include "ZombieSystem.h"
#include "NavGrid.h"
#include "AttackSystem.h"  
#include "CameraSystem.h"   
#include "../ContestAPI/app.h"

#include <cmath>
#include <algorithm>
#include <cstdlib>

using namespace GameConfig;


namespace
{
    float gHiveAnimTimeSec = 0.0f;
}

namespace
{
    float Rand01()
    {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    }

    float RandRange(float a, float b)
    {
        return a + (b - a) * Rand01();
    }

    float DistSq(float ax, float ay, float bx, float by)
    {
        const float dx = ax - bx;
        const float dy = ay - by;
        return dx * dx + dy * dy;
    }
}
namespace
{
    void DrawCircleLines(float cx, float cy, float r, float red, float green, float blue)
    {
        const int segments = HiveConfig::CIRCLE_SEGMENTS;
        const float twoPi = HiveConfig::TWO_PI;

        float prevX = cx + r;
        float prevY = cy;

        for (int i = 1; i <= segments; i++)
        {
            const float a = (static_cast<float>(i) / static_cast<float>(segments)) * twoPi;
            const float x = cx + std::cos(a) * r;
            const float y = cy + std::sin(a) * r;

            App::DrawLine(prevX, prevY, x, y, red, green, blue);
            prevX = x;
            prevY = y;
        }
    }

    void DrawSpokeRing(float cx, float cy, float radius, float spokeLen,
        float r, float g, float b, int spokes, float phase)
    {
        const float twoPi = HiveConfig::TWO_PI;

        for (int i = 0; i < spokes; i++)
        {
            const float a = twoPi * (static_cast<float>(i) / static_cast<float>(spokes)) + phase;

            const float x0 = cx + std::cos(a) * (radius - spokeLen);
            const float y0 = cy + std::sin(a) * (radius - spokeLen);
            const float x1 = cx + std::cos(a) * (radius + spokeLen);
            const float y1 = cy + std::sin(a) * (radius + spokeLen);

            App::DrawLine(x0, y0, x1, y1, r, g, b);
        }
    }

    void DrawArc(float cx, float cy, float radius, float a0, float a1,
        float r, float g, float b, int seg = HiveConfig::ARC_SEGMENTS)
    {
        const float twoPi = HiveConfig::TWO_PI;
        while (a1 < a0)
            a1 += twoPi;

        float px = cx + std::cos(a0) * radius;
        float py = cy + std::sin(a0) * radius;

        for (int i = 1; i <= seg; i++)
        {
            const float tt = static_cast<float>(i) / static_cast<float>(seg);
            const float a = a0 + (a1 - a0) * tt;

            const float x = cx + std::cos(a) * radius;
            const float y = cy + std::sin(a) * radius;

            App::DrawLine(px, py, x, y, r, g, b);
            px = x;
            py = y;
        }
    }
}


void HiveSystem::Init()
{
    hives.clear();

    // Fixed seed for deterministic hive placement
    std::srand(HiveConfig::PLACEMENT_SEED);

    const float worldMin = HiveConfig::WORLD_MIN;
    const float worldMax = HiveConfig::WORLD_MAX;
    const float margin = HiveConfig::PLACEMENT_MARGIN;
    const float minDist = HiveConfig::MIN_HIVE_DISTANCE;
    const int hiveCount = HiveConfig::HIVE_COUNT;
    const int maxAttempts = HiveConfig::MAX_PLACEMENT_ATTEMPTS;

    // Track placed hive positions for minimum distance enforcement
    float placedX[HiveConfig::HIVE_COUNT];
    float placedY[HiveConfig::HIVE_COUNT];
    int placed = 0;

    for (int i = 0; i < hiveCount; i++)
    {
        const bool isBossHive = (i == hiveCount - 1);
        const float radius = isBossHive ? HiveConfig::BOSS_HIVE_RADIUS : HiveConfig::NORMAL_HIVE_RADIUS;
        const float hp = isBossHive ? HiveConfig::BOSS_HIVE_HP : HiveConfig::NORMAL_HIVE_HP;

        bool found = false;

        // Try to find valid placement position
        for (int attempt = 0; attempt < maxAttempts; attempt++)
        {
            const float x = RandRange(worldMin + margin, worldMax - margin);
            const float y = RandRange(worldMin + margin, worldMax - margin);

            // Check minimum distance from all already-placed hives
            bool ok = true;
            for (int j = 0; j < placed; j++)
            {
                if (DistSq(x, y, placedX[j], placedY[j]) < (minDist * minDist))
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

        // Fallback: if no valid position found after all attempts, place anyway
        if (!found)
        {
            const float x = RandRange(worldMin + margin, worldMax - margin);
            const float y = RandRange(worldMin + margin, worldMax - margin);
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

    // Update global animation timer
    gHiveAnimTimeSec += deltaTimeMs * HiveConfig::MS_TO_SEC;
    if (gHiveAnimTimeSec > HiveConfig::ANIM_TIME_RESET)
        gHiveAnimTimeSec = 0.0f;

    if (dt <= 0.0f)
        return;

    for (Hive& h : hives)
    {
        if (!h.alive)
            continue;

        // Accumulate spawn timer
        const float spawnPerSec = h.spawnPerMin / HiveConfig::SECONDS_PER_MINUTE;
        h.spawnAccum += spawnPerSec * dt;

        // Cap accumulator to prevent excessive buildup
        if (h.spawnAccum > HiveConfig::MAX_SPAWN_ACCUM)
            h.spawnAccum = HiveConfig::MAX_SPAWN_ACCUM;

        // Spawn zombies when accumulator reaches 1.0
        while (h.spawnAccum >= 1.0f)
        {
            if (!zombies.CanSpawnMore(1))
                return;

            bool spawned = false;

            // Try multiple times to find valid spawn position
            for (int tries = 0; tries < HiveConfig::SPAWN_PLACEMENT_ATTEMPTS; tries++)
            {
                const float ang = Rand01() * HiveConfig::TWO_PI;
                const float rMin = h.radius + HiveConfig::SPAWN_RADIUS_MIN_OFFSET;
                const float rMax = h.radius + HiveConfig::SPAWN_RADIUS_MAX_OFFSET;
                const float rr = rMin + (rMax - rMin) * Rand01();

                const float sx = h.x + std::cos(ang) * rr;
                const float sy = h.y + std::sin(ang) * rr;

                // Skip if spawn position is blocked
                if (nav.IsBlockedWorld(sx, sy))
                    continue;

                // Attempt spawn
                if (zombies.SpawnAtWorld(sx, sy))
                {
                    spawned = true;
                    break;
                }
            }

            // If couldn't spawn after all attempts, stop trying this frame
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

        // Combined radius check for hit detection
        const float combinedRadius = h.radius + hitRadius;
        if (DistSq(wx, wy, h.x, h.y) <= combinedRadius * combinedRadius)
        {
            const bool wasAlive = h.alive;  // ✅ NEW: Track if it was alive before damage
            h.hp -= damage;
            hitAny = true;

            // Clamp HP and mark as dead if needed
            if (h.hp <= 0.0f)
            {
                h.hp = 0.0f;
                h.alive = false;

                // ✅ NEW: Trigger explosion VFX when hive dies
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

        // Pulsing animation
        const float pulse = HiveConfig::PULSE_BASE +
            HiveConfig::PULSE_AMP * std::sinf(time * HiveConfig::PULSE_FREQUENCY);

        // Outer reactor rings
        DrawCircleLines(sx, sy, R, 1.0f, 0.95f, 0.20f);
        DrawCircleLines(sx, sy, R + HiveConfig::PULSE_RING_OFFSET + pulse * HiveConfig::PULSE_RING_SIZE,
            1.0f, 0.85f, 0.10f);

        // Rotating spokes
        DrawSpokeRing(sx, sy, R * HiveConfig::SPOKE_RADIUS_MULT, HiveConfig::SPOKE_LENGTH,
            1.0f, 0.55f, 0.10f, HiveConfig::SPOKE_COUNT,
            time * HiveConfig::SPOKE_ROTATION_SPEED);

        // Inner reactor rings
        DrawCircleLines(sx, sy, R * HiveConfig::INNER_RING_1_MULT, 1.0f, 0.85f, 0.10f);
        DrawCircleLines(sx, sy, R * HiveConfig::INNER_RING_2_MULT, 1.0f, 0.70f, 0.05f);

        // Animated arcs
        DrawArc(sx, sy, R * HiveConfig::INNER_RING_1_MULT,
            time * HiveConfig::ARC_1_SPEED,
            time * HiveConfig::ARC_1_SPEED + HiveConfig::ARC_1_LENGTH,
            1.0f, 0.95f, 0.20f);

        DrawArc(sx, sy, R * HiveConfig::INNER_RING_2_MULT,
            -time * HiveConfig::ARC_2_SPEED,
            -time * HiveConfig::ARC_2_SPEED + HiveConfig::ARC_2_LENGTH,
            1.0f, 0.70f, 0.10f);

        // HP bar
        const float hpT = (h.maxHp > 0.0f) ? std::clamp(h.hp / h.maxHp, 0.0f, 1.0f) : 0.0f;
        const float barW = HiveConfig::HP_BAR_WIDTH;
        const float barY = sy - R - HiveConfig::HP_BAR_OFFSET_Y;

        // HP bar background
        App::DrawLine(sx - barW * 0.5f, barY, sx + barW * 0.5f, barY,
            0.05f, 0.07f, 0.10f);

        // HP bar fill
        App::DrawLine(sx - barW * 0.5f, barY, sx - barW * 0.5f + barW * hpT, barY,
            0.10f, 1.00f, 0.10f);

        // HP bar tech highlight
        App::DrawLine(sx - barW * 0.5f, barY - 1.0f, sx + barW * 0.5f, barY - 1.0f,
            0.70f, 0.90f, 1.00f);
    }
}