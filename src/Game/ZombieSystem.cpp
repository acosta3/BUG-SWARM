#include "ZombieSystem.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

static float Rand01()
{
    return (float)std::rand() / (float)RAND_MAX;
}

void ZombieSystem::Init(int maxZombies)
{
    maxCount = maxZombies;
    aliveCount = 0;

    posX.resize(maxCount);
    posY.resize(maxCount);
    velX.resize(maxCount);
    velY.resize(maxCount);

    type.resize(maxCount);
    state.resize(maxCount);

    fearTimerMs.resize(maxCount);
    attackCooldownMs.resize(maxCount);
    hp.resize(maxCount);

    InitTypeStats();

    gridW = (int)((worldMaxX - worldMinX) / cellSize) + 1;
    gridH = (int)((worldMaxY - worldMinY) / cellSize) + 1;

    cellStart.resize(gridW * gridH + 1);
    cellCount.resize(gridW * gridH);
    writeCursor.resize(gridW * gridH + 1);
    cellList.resize(maxCount);
}

void ZombieSystem::InitTypeStats()
{
    typeStats[GREEN] = { 60.f, 1.f, 1.f,  1, 1, 750.f,   0.f };
    typeStats[RED] = { 90.f, 1.f, 1.f,  1, 1, 650.f,   0.f };
    typeStats[BLUE] = { 45.f, 1.f, 1.f,  4, 2, 900.f,   0.f };
    typeStats[PURPLE_ELITE] = { 65.f, 1.f, 1.f, 12, 3, 850.f, 220.f };
}

uint8_t ZombieSystem::RollTypeWeighted() const
{
    float r = Rand01();
    if (r < 0.70f) return GREEN;
    if (r < 0.90f) return RED;
    if (r < 0.99f) return BLUE;
    return PURPLE_ELITE;
}

int ZombieSystem::CellIndex(float x, float y) const
{
    int cx = (int)((x - worldMinX) / cellSize);
    int cy = (int)((y - worldMinY) / cellSize);

    cx = std::clamp(cx, 0, gridW - 1);
    cy = std::clamp(cy, 0, gridH - 1);

    return cy * gridW + cx;
}

void ZombieSystem::BuildGrid()
{
    const int cellN = gridW * gridH;

    std::fill(cellCount.begin(), cellCount.end(), 0);

    for (int i = 0; i < aliveCount; i++)
    {
        int c = CellIndex(posX[i], posY[i]);
        cellCount[c]++;
    }

    cellStart[0] = 0;
    for (int c = 0; c < cellN; c++)
        cellStart[c + 1] = cellStart[c] + cellCount[c];

    // No allocation after Init()
    writeCursor = cellStart;

    for (int i = 0; i < aliveCount; i++)
    {
        int c = CellIndex(posX[i], posY[i]);
        int dst = writeCursor[c]++;
        cellList[dst] = i;
    }
}

void ZombieSystem::Clear()
{
    aliveCount = 0;
}

void ZombieSystem::Spawn(int count, float playerX, float playerY)
{
    const float minR = 250.f;
    const float maxR = 450.f;

    for (int k = 0; k < count && aliveCount < maxCount; k++)
    {
        int i = aliveCount++;
        uint8_t t = RollTypeWeighted();

        float ang = Rand01() * 6.2831853f;
        float r = minR + (maxR - minR) * Rand01();

        posX[i] = playerX + std::cos(ang) * r;
        posY[i] = playerY + std::sin(ang) * r;

        velX[i] = 0.f;
        velY[i] = 0.f;

        type[i] = t;
        state[i] = SEEK;

        fearTimerMs[i] = 0.f;
        attackCooldownMs[i] = 0.f;
        hp[i] = typeStats[t].maxHP;
    }
}

void ZombieSystem::Kill(int index)
{
    int last = aliveCount - 1;
    if (index != last)
    {
        posX[index] = posX[last];
        posY[index] = posY[last];
        velX[index] = velX[last];
        velY[index] = velY[last];

        type[index] = type[last];
        state[index] = state[last];
        fearTimerMs[index] = fearTimerMs[last];
        attackCooldownMs[index] = attackCooldownMs[last];
        hp[index] = hp[last];
    }
    aliveCount--;
}

void ZombieSystem::Update(float deltaTimeMs, float playerX, float playerY)
{
    const float dt = deltaTimeMs / 1000.0f;
    if (dt <= 0.0f) return;

    BuildGrid();

    // SIM LOD: only do separation near the player
    const float sepActiveRadius = 600.0f;
    const float sepActiveRadiusSq = sepActiveRadius * sepActiveRadius;

    // Fear despawn: if they flee far enough, remove them
    const float fleeDespawnRadius = 1200.0f;
    const float fleeDespawnRadiusSq = fleeDespawnRadius * fleeDespawnRadius;

    // Separation radius
    const float sepRadius = 18.0f;
    const float sepRadiusSq = sepRadius * sepRadius;

    // IMPORTANT: swap-remove safe loop
    for (int i = 0; i < aliveCount; )
    {
        // Timers (avoid std::max macro issues)
        if (fearTimerMs[i] > 0.f)
        {
            fearTimerMs[i] -= deltaTimeMs;
            if (fearTimerMs[i] < 0.f) fearTimerMs[i] = 0.f;
        }

        if (attackCooldownMs[i] > 0.f)
        {
            attackCooldownMs[i] -= deltaTimeMs;
            if (attackCooldownMs[i] < 0.f) attackCooldownMs[i] = 0.f;
        }

        const uint8_t t = type[i];
        const ZombieTypeStats& s = typeStats[t];

        // Vector to player + distance (compute once)
        float toPX = playerX - posX[i];
        float toPY = playerY - posY[i];
        float distSqToPlayer = toPX * toPX + toPY * toPY;

        const bool feared = (fearTimerMs[i] > 0.0f);

        // If feared and far enough, despawn
        if (feared && distSqToPlayer > fleeDespawnRadiusSq)
        {
            Kill(i);        // swap-remove
            continue;       // do NOT i++
        }

        // Direction: seek or flee
        float dx = toPX;
        float dy = toPY;

        if (feared)
        {
            dx = -dx;
            dy = -dy;
        }

        // Normalize direction
        float lenSq = dx * dx + dy * dy;
        if (lenSq > 0.0001f)
        {
            float invLen = 1.0f / std::sqrt(lenSq);
            dx *= invLen;
            dy *= invLen;
        }
        else
        {
            dx = 0.0f;
            dy = 0.0f;
        }

        // While feared: cheap panic flee (no separation)
        if (feared)
        {
            velX[i] = dx * s.maxSpeed;
            velY[i] = dy * s.maxSpeed;

            posX[i] += velX[i] * dt;
            posY[i] += velY[i] * dt;

            i++;
            continue;
        }

        // ---- SIM LOD EARLY-OUT (seek only when far) ----
        if (distSqToPlayer > sepActiveRadiusSq)
        {
            velX[i] = dx * s.maxSpeed;
            velY[i] = dy * s.maxSpeed;

            posX[i] += velX[i] * dt;
            posY[i] += velY[i] * dt;

            i++;
            continue;
        }
        // ------------------------------------------------

        // Separation (grid neighbors)
        float sepX = 0.0f;
        float sepY = 0.0f;

        int c = CellIndex(posX[i], posY[i]);
        int cx = c % gridW;
        int cy = c / gridW;

        for (int oy = -1; oy <= 1; oy++)
        {
            int ny = cy + oy;
            if (ny < 0 || ny >= gridH) continue;

            for (int ox = -1; ox <= 1; ox++)
            {
                int nx = cx + ox;
                if (nx < 0 || nx >= gridW) continue;

                int nc = ny * gridW + nx;

                int start = cellStart[nc];
                int end = cellStart[nc + 1];

                for (int k = start; k < end; k++)
                {
                    int j = cellList[k];
                    if (j == i) continue;

                    float ax = posX[i] - posX[j];
                    float ay = posY[i] - posY[j];
                    float d2 = ax * ax + ay * ay;

                    if (d2 > 0.0001f && d2 < sepRadiusSq)
                    {
                        float d = std::sqrt(d2);
                        float invD = 1.0f / d;

                        float push = (sepRadius - d) / sepRadius; // 0..1
                        sepX += ax * invD * push;
                        sepY += ay * invD * push;
                    }
                }
            }
        }

        // Combine seek + separation
        float vx = dx * s.seekWeight + sepX * s.sepWeight;
        float vy = dy * s.seekWeight + sepY * s.sepWeight;

        // Normalize final direction
        float v2 = vx * vx + vy * vy;
        if (v2 > 0.0001f)
        {
            float invV = 1.0f / std::sqrt(v2);
            vx *= invV;
            vy *= invV;
        }
        else
        {
            vx = 0.0f;
            vy = 0.0f;
        }

        // Apply speed + integrate
        velX[i] = vx * s.maxSpeed;
        velY[i] = vy * s.maxSpeed;

        posX[i] += velX[i] * dt;
        posY[i] += velY[i] * dt;

        i++;
    }
}

void ZombieSystem::TriggerFear(float sourceX, float sourceY, float radius, float durationMs)
{
    const float r2 = radius * radius;

    for (int i = 0; i < aliveCount; i++)
    {
        // Optional: elites don't get feared
        if (type[i] == PURPLE_ELITE)
            continue;

        float dx = posX[i] - sourceX;
        float dy = posY[i] - sourceY;
        float d2 = dx * dx + dy * dy;

        if (d2 <= r2)
        {
            state[i] = FLEE;
            fearTimerMs[i] = durationMs; // refresh/overwrite
        }
    }
}


void ZombieSystem::GetTypeCounts(int& g, int& r, int& b, int& p) const
{
    g = r = b = p = 0;
    for (int i = 0; i < aliveCount; i++)
    {
        switch (type[i])
        {
        case GREEN: g++; break;
        case RED: r++; break;
        case BLUE: b++; break;
        case PURPLE_ELITE: p++; break;
        }
    }
}
