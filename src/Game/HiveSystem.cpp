// HiveSystem.cpp
#include "HiveSystem.h"

#include "../ContestAPI/app.h"
#include "ZombieSystem.h"
#include "NavGrid.h"

#include <cmath>
#include <algorithm>
#include <cstdlib>

static float gHiveAnimTimeSec = 0.0f;

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

// Circle using line segments
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

static void DrawSpokeRing(float cx, float cy, float radius, float spokeLen, float r, float g, float b, int spokes, float phase)
{
    const float twoPi = 6.28318530718f;

    for (int i = 0; i < spokes; i++)
    {
        const float a = twoPi * ((float)i / (float)spokes) + phase;

        const float x0 = cx + std::cos(a) * (radius - spokeLen);
        const float y0 = cy + std::sin(a) * (radius - spokeLen);
        const float x1 = cx + std::cos(a) * (radius + spokeLen);
        const float y1 = cy + std::sin(a) * (radius + spokeLen);

        App::DrawLine(x0, y0, x1, y1, r, g, b);
    }
}

static void DrawArc(float cx, float cy, float radius, float a0, float a1, float r, float g, float b, int seg = 18)
{
    const float twoPi = 6.28318530718f;
    while (a1 < a0) a1 += twoPi;

    float px = cx + std::cos(a0) * radius;
    float py = cy + std::sin(a0) * radius;

    for (int i = 1; i <= seg; i++)
    {
        const float tt = (float)i / (float)seg;
        const float a = a0 + (a1 - a0) * tt;

        const float x = cx + std::cos(a) * radius;
        const float y = cy + std::sin(a) * radius;

        App::DrawLine(px, py, x, y, r, g, b);
        px = x;
        py = y;
    }
}

void HiveSystem::Init()
{
    hives.clear();

    // Fixed seed so hive layout is the same every run
    std::srand(1337);

    const float worldMin = -1500.0f;
    const float worldMax = 1500.0f;

    const float margin = 220.0f;
    const float minDist = 900.0f;
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

    gHiveAnimTimeSec += deltaTimeMs * 0.001f;
    if (gHiveAnimTimeSec > 100000.0f) gHiveAnimTimeSec = 0.0f;

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
    const float time = gHiveAnimTimeSec;

    for (const Hive& h : hives)
    {
        if (!h.alive) continue;

        const float sx = h.x - camOffX;
        const float sy = h.y - camOffY;

        const float pulse = 0.5f + 0.5f * std::sinf(time * 3.0f);
        const float R = h.radius;

        // Reactor look
        DrawCircleLines(sx, sy, R, 1.0f, 0.95f, 0.20f);
        DrawCircleLines(sx, sy, R + 2.0f + pulse * 2.0f, 1.0f, 0.85f, 0.10f);

        DrawSpokeRing(sx, sy, R * 0.82f, 6.0f, 1.0f, 0.55f, 0.10f, 20, time * 1.2f);

        DrawCircleLines(sx, sy, R * 0.55f, 1.0f, 0.85f, 0.10f);
        DrawCircleLines(sx, sy, R * 0.35f, 1.0f, 0.70f, 0.05f);

        DrawArc(sx, sy, R * 0.55f, time * 1.5f, time * 1.5f + 1.3f, 1.0f, 0.95f, 0.20f);
        DrawArc(sx, sy, R * 0.35f, -time * 1.8f, -time * 1.8f + 1.1f, 1.0f, 0.70f, 0.10f);

        // HP bar (restored)
        float hpT = (h.maxHp > 0.0f) ? (h.hp / h.maxHp) : 0.0f;
        hpT = std::clamp(hpT, 0.0f, 1.0f);

        const float barW = 64.0f;
        const float barY = sy - R - 12.0f;

        // background
        App::DrawLine(sx - barW * 0.5f, barY, sx + barW * 0.5f, barY, 0.05f, 0.07f, 0.10f);

        // fill
        App::DrawLine(sx - barW * 0.5f, barY, sx - barW * 0.5f + barW * hpT, barY, 0.10f, 1.00f, 0.10f);

        // thin tech highlight
        App::DrawLine(sx - barW * 0.5f, barY - 1.0f, sx + barW * 0.5f, barY - 1.0f, 0.70f, 0.90f, 1.00f);
    }
}
