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

    AddHive(220.0f, 220.0f, 30.0f, 120.0f);
    AddHive(900.0f, 250.0f, 30.0f, 120.0f);
    AddHive(320.0f, 720.0f, 30.0f, 120.0f);
    AddHive(860.0f, 650.0f, 30.0f, 120.0f);
    AddHive(560.0f, 430.0f, 34.0f, 160.0f);
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
