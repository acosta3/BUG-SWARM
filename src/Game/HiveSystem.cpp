// HiveSystem.cpp
#include "HiveSystem.h"

#include "../ContestAPI/app.h"
#include "ZombieSystem.h"
#include "NavGrid.h"

#include <cmath>
#include <algorithm>
#include <cstdlib>

static float Rand01()
{
    return (float)std::rand() / (float)RAND_MAX;
}

static float RandRange(float a, float b)
{
    return a + (b - a) * Rand01();
}

static float DistSq(float ax, float ay, float bx, float by)
{
    const float dx = ax - bx;
    const float dy = ay - by;
    return dx * dx + dy * dy;
}

// Yellow circle using line segments
static void DrawCircleLines(float cx, float cy, float r, float red, float green, float blue)
{
    static constexpr float kPi = 3.14159265358979323846f;

    const int segments = 28;
    const float twoPi = 2.0f * kPi;

    float prevX = cx + r;
    float prevY = cy;

    for (int i = 1; i <= segments; i++)
    {
        const float a = (float)i / (float)segments * twoPi;
        const float x = cx + std::cos(a) * r;
        const float y = cy + std::sin(a) * r;

        App::DrawLine(prevX, prevY, x, y, red, green, blue);
        prevX = x;
        prevY = y;
    }
}

void HiveSystem::Init()
{
    hives.clear();

    // If you want the SAME hive layout every run, keep this fixed seed.
    // If you want different layout each run, remove this line and seed once in main with time.
    std::srand(1337);

    // World is roughly -1500..1500 (your note)
    const float worldMin = -1500.0f;
    const float worldMax = 1500.0f;

    // Tuning knobs
    const float margin = 220.0f;     // keep away from edges
    const float minDist = 900.0f;    // spread out distance between hives
    const int hiveCount = 5;
    const int maxAttemptsPerHive = 700;

    float placedX[hiveCount];
    float placedY[hiveCount];
    int placed = 0;

    for (int i = 0; i < hiveCount; i++)
    {
        const bool isBossHive = (i == hiveCount - 1);
        const float radius = isBossHive ? 34.0f : 30.0f;
        const float hp = isBossHive ? 160.0f : 120.0f;

        bool found = false;

        for (int attempt = 0; attempt < maxAttemptsPerHive; attempt++)
        {
            const float x = RandRange(worldMin + margin, worldMax - margin);
            const float y = RandRange(worldMin + margin, worldMax - margin);

            bool ok = true;
            for (int j = 0; j < placed; j++)
            {
                if (DistSq(x, y, placedX[j], placedY[j]) < (minDist * minDist))
                {
                    ok = false;
                    break;
                }
            }

            if (!ok) continue;

            AddHive(x, y, radius, hp);
            placedX[placed] = x;
            placedY[placed] = y;
            placed++;
            found = true;
            break;
        }

        // Fallback: if it couldn't find a far-enough spot, still place it somewhere valid.
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

    h.spawnPerMin = 100.0f;
    h.spawnAccum = 0.0f;

    hives.push_back(h);
}

int HiveSystem::AliveCount() const
{
    int c = 0;
    for (const Hive& h : hives)
        if (h.alive) c++;
    return c;
}

void HiveSystem::Update(float deltaTimeMs, ZombieSystem& zombies, const NavGrid& nav)
{
    const float dt = deltaTimeMs / 1000.0f;
    if (dt <= 0.0f) return;

    for (Hive& h : hives)
    {
        if (!h.alive) continue;

        const float spawnPerSec = h.spawnPerMin / 60.0f;
        h.spawnAccum += spawnPerSec * dt;

        if (h.spawnAccum > 10.0f) h.spawnAccum = 10.0f;

        while (h.spawnAccum >= 1.0f)
        {
            if (!zombies.CanSpawnMore(1))
                return;

            bool spawned = false;

            for (int tries = 0; tries < 10; tries++)
            {
                const float ang = Rand01() * 6.2831853f;
                const float rMin = h.radius + 10.0f;
                const float rMax = h.radius + 55.0f;
                const float rr = rMin + (rMax - rMin) * Rand01();

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

bool HiveSystem::DamageHiveAt(float wx, float wy, float hitRadius, float damage)
{
    bool hitAny = false;

    for (Hive& h : hives)
    {
        if (!h.alive) continue;

        const float rr = h.radius + hitRadius;
        if (DistSq(wx, wy, h.x, h.y) <= rr * rr)
        {
            h.hp -= damage;
            hitAny = true;

            if (h.hp <= 0.0f)
            {
                h.hp = 0.0f;
                h.alive = false;
            }
        }
    }

    return hitAny;
}

void HiveSystem::Render(float camOffX, float camOffY) const
{
    for (const Hive& h : hives)
    {
        if (!h.alive) continue;

        const float sx = h.x - camOffX;
        const float sy = h.y - camOffY;

        DrawCircleLines(sx, sy, h.radius, 1.0f, 1.0f, 0.0f);
        DrawCircleLines(sx, sy, h.radius * 0.7f, 1.0f, 0.9f, 0.0f);
        DrawCircleLines(sx, sy, h.radius * 0.4f, 1.0f, 0.8f, 0.0f);

        float t = (h.maxHp > 0.0f) ? (h.hp / h.maxHp) : 0.0f;
        t = std::clamp(t, 0.0f, 1.0f);

        const float barW = 60.0f;
        const float barY = sy - h.radius - 12.0f;

        App::DrawLine(sx - barW * 0.5f, barY, sx + barW * 0.5f, barY, 0.2f, 0.2f, 0.2f);
        App::DrawLine(sx - barW * 0.5f, barY, sx - barW * 0.5f + barW * t, barY, 0.1f, 1.0f, 0.1f);
    }
}
