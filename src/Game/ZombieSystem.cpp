#include "ZombieSystem.h"
#include "NavGrid.h"

#include <cstdlib>
#include <cmath>
#include <algorithm>

// -------------------- small utilities --------------------
static float Rand01()
{
    return (float)std::rand() / (float)RAND_MAX;
}

float ZombieSystem::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

void ZombieSystem::NormalizeSafe(float& x, float& y)
{
    const float len2 = x * x + y * y;
    if (len2 > 0.0001f)
    {
        const float inv = 1.0f / std::sqrt(len2);
        x *= inv;
        y *= inv;
    }
    else
    {
        x = 0.0f;
        y = 0.0f;
    }
}

// -------------------- movement (axis slide) --------------------
bool ZombieSystem::ResolveMoveSlide(float& x, float& y, float vx, float vy, float dt,
    const NavGrid& nav, bool& outFullBlocked) const
{
    outFullBlocked = false;

    const float startX = x;
    const float startY = y;

    // Full move
    float nx = x + vx * dt;
    float ny = y + vy * dt;
    if (!nav.IsBlockedWorld(nx, ny))
    {
        x = nx; y = ny;
        return true;
    }

    outFullBlocked = true;

    // Slide X only
    nx = x + vx * dt;
    ny = y;
    if (!nav.IsBlockedWorld(nx, ny))
    {
        x = nx;
        const float moved2 = (x - startX) * (x - startX) + (y - startY) * (y - startY);
        return moved2 > 0.0001f;
    }

    // Slide Y only
    nx = x;
    ny = y + vy * dt;
    if (!nav.IsBlockedWorld(nx, ny))
    {
        y = ny;
        const float moved2 = (x - startX) * (x - startX) + (y - startY) * (y - startY);
        return moved2 > 0.0001f;
    }

    return false;
}

// -------------------- init --------------------
void ZombieSystem::Init(int maxZombies, const NavGrid& nav)
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

    flowAssistMs.resize(maxCount);

    InitTypeStats();

    // Copy world bounds from NavGrid (single source of truth)
    worldMinX = nav.WorldMinX();
    worldMinY = nav.WorldMinY();
    worldMaxX = nav.WorldMaxX();
    worldMaxY = nav.WorldMaxY();

    // Separation grid uses ZombieSystem cellSize (independent from nav cellSize)
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
    const float r = Rand01();
    if (r < 0.70f) return GREEN;
    if (r < 0.90f) return RED;
    if (r < 0.99f) return BLUE;
    return PURPLE_ELITE;
}

// -------------------- grid --------------------
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
        const int c = CellIndex(posX[i], posY[i]);
        cellCount[c]++;
    }

    cellStart[0] = 0;
    for (int c = 0; c < cellN; c++)
        cellStart[c + 1] = cellStart[c] + cellCount[c];

    // No allocation after Init()
    writeCursor = cellStart;

    for (int i = 0; i < aliveCount; i++)
    {
        const int c = CellIndex(posX[i], posY[i]);
        const int dst = writeCursor[c]++;
        cellList[dst] = i;
    }
}

// -------------------- lifecycle --------------------
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
        const int i = aliveCount++;
        const uint8_t t = RollTypeWeighted();

        const float ang = Rand01() * 6.2831853f;
        const float r = minR + (maxR - minR) * Rand01();

        posX[i] = playerX + std::cos(ang) * r;
        posY[i] = playerY + std::sin(ang) * r;

        velX[i] = 0.f;
        velY[i] = 0.f;

        type[i] = t;
        state[i] = SEEK;

        fearTimerMs[i] = 0.f;
        attackCooldownMs[i] = 0.f;
        hp[i] = typeStats[t].maxHP;

        flowAssistMs[i] = 0.0f;
    }
}

void ZombieSystem::Kill(int index)
{
    const int last = aliveCount - 1;
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

        flowAssistMs[index] = flowAssistMs[last];
    }
    aliveCount--;
}

// -------------------- per-frame helpers --------------------
void ZombieSystem::TickTimers(int i, float deltaTimeMs)
{
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

    if (flowAssistMs[i] > 0.f)
    {
        flowAssistMs[i] -= deltaTimeMs;
        if (flowAssistMs[i] < 0.f) flowAssistMs[i] = 0.f;
    }
}

void ZombieSystem::ApplyTouchDamage(int i, const ZombieTypeStats& s,
    float distSqToPlayer, float hitDistSq, int& outDamage)
{
    if (attackCooldownMs[i] <= 0.0f && distSqToPlayer <= hitDistSq)
    {
        outDamage += (int)s.touchDamage;
        attackCooldownMs[i] = s.attackCooldownMs;
    }
}

void ZombieSystem::ComputeSeekDir(int i, float playerX, float playerY, float& outDX, float& outDY) const
{
    outDX = playerX - posX[i];
    outDY = playerY - posY[i];
    NormalizeSafe(outDX, outDY);
}

void ZombieSystem::ComputeFleeDir(int i, float playerX, float playerY, float& outDX, float& outDY) const
{
    outDX = posX[i] - playerX;
    outDY = posY[i] - playerY;
    NormalizeSafe(outDX, outDY);
}

void ZombieSystem::ApplyFlowAssistIfActive(int i, const NavGrid& nav, float& ioDX, float& ioDY) const
{
    if (flowAssistMs[i] <= 0.0f) return;

    const float flowWeight = tuning.flowWeight;

    const int navCell = nav.CellIndex(posX[i], posY[i]);
    const float fx = nav.FlowXAtCell(navCell);
    const float fy = nav.FlowYAtCell(navCell);

    const float f2 = fx * fx + fy * fy;
    if (f2 <= 0.0001f)
        return;

    ioDX = ioDX * (1.0f - flowWeight) + fx * flowWeight;
    ioDY = ioDY * (1.0f - flowWeight) + fy * flowWeight;
    NormalizeSafe(ioDX, ioDY);
}

void ZombieSystem::ComputeSeparation(int i, float& outSepX, float& outSepY) const
{
    outSepX = 0.0f;
    outSepY = 0.0f;

    const float sepRadius = tuning.sepRadius;
    const float sepRadiusSq = sepRadius * sepRadius;

    const int c = CellIndex(posX[i], posY[i]);
    const int cx = c % gridW;
    const int cy = c / gridW;

    for (int oy = -1; oy <= 1; oy++)
    {
        const int ny = cy + oy;
        if (ny < 0 || ny >= gridH) continue;

        for (int ox = -1; ox <= 1; ox++)
        {
            const int nx = cx + ox;
            if (nx < 0 || nx >= gridW) continue;

            const int nc = ny * gridW + nx;
            const int start = cellStart[nc];
            const int end = cellStart[nc + 1];

            for (int k = start; k < end; k++)
            {
                const int j = cellList[k];
                if (j == i) continue;

                const float ax = posX[i] - posX[j];
                const float ay = posY[i] - posY[j];
                const float d2 = ax * ax + ay * ay;

                if (d2 > 0.0001f && d2 < sepRadiusSq)
                {
                    const float d = std::sqrt(d2);
                    const float invD = 1.0f / d;

                    const float push = (sepRadius - d) / sepRadius;
                    outSepX += ax * invD * push;
                    outSepY += ay * invD * push;
                }
            }
        }
    }
}

// -------------------- update --------------------
int ZombieSystem::Update(float deltaTimeMs, float playerX, float playerY, const NavGrid& nav)
{
    const float dt = deltaTimeMs / 1000.0f;
    if (dt <= 0.0f) return 0;

    int damageThisFrame = 0;

    const float hitDist = tuning.playerRadius + tuning.zombieRadius;
    const float hitDistSq = hitDist * hitDist;

    const float sepActiveRadiusSq = tuning.sepActiveRadius * tuning.sepActiveRadius;
    const float fleeDespawnRadiusSq = tuning.fleeDespawnRadius * tuning.fleeDespawnRadius;

    BuildGrid();

    for (int i = 0; i < aliveCount; )
    {
        TickTimers(i, deltaTimeMs);

        const uint8_t zt = type[i];
        const ZombieTypeStats& s = typeStats[zt];

        const float toPX = playerX - posX[i];
        const float toPY = playerY - posY[i];
        const float distSqToPlayer = toPX * toPX + toPY * toPY;

        const bool feared = (fearTimerMs[i] > 0.0f);

        ApplyTouchDamage(i, s, distSqToPlayer, hitDistSq, damageThisFrame);

        if (feared && distSqToPlayer > fleeDespawnRadiusSq)
        {
            Kill(i);
            continue;
        }

        // ---- feared: flee (no separation) ----
        if (feared)
        {
            float dx, dy;
            ComputeFleeDir(i, playerX, playerY, dx, dy);

            velX[i] = dx * s.maxSpeed;
            velY[i] = dy * s.maxSpeed;

            bool fullBlocked = false;
            (void)ResolveMoveSlide(posX[i], posY[i], velX[i], velY[i], dt, nav, fullBlocked);

            i++;
            continue;
        }

        // ---- seek + optional flow assist ----
        float dx, dy;
        ComputeSeekDir(i, playerX, playerY, dx, dy);
        ApplyFlowAssistIfActive(i, nav, dx, dy);

        // ---- LOD: far away = no separation ----
        if (distSqToPlayer > sepActiveRadiusSq)
        {
            velX[i] = dx * s.maxSpeed;
            velY[i] = dy * s.maxSpeed;

            bool fullBlocked = false;
            const bool moved = ResolveMoveSlide(posX[i], posY[i], velX[i], velY[i], dt, nav, fullBlocked);

            if (fullBlocked || !moved)
                flowAssistMs[i] = tuning.flowAssistBurstMs;

            i++;
            continue;
        }

        // ---- separation ----
        float sepX, sepY;
        ComputeSeparation(i, sepX, sepY);

        float vx = dx * s.seekWeight + sepX * s.sepWeight;
        float vy = dy * s.seekWeight + sepY * s.sepWeight;
        NormalizeSafe(vx, vy);

        velX[i] = vx * s.maxSpeed;
        velY[i] = vy * s.maxSpeed;

        bool fullBlocked = false;
        const bool moved = ResolveMoveSlide(posX[i], posY[i], velX[i], velY[i], dt, nav, fullBlocked);

        if (fullBlocked || !moved)
            flowAssistMs[i] = tuning.flowAssistBurstMs;

        i++;
    }

    return damageThisFrame;
}

// -------------------- fear + counts --------------------
void ZombieSystem::TriggerFear(float sourceX, float sourceY, float radius, float durationMs)
{
    const float r2 = radius * radius;

    for (int i = 0; i < aliveCount; i++)
    {
        if (type[i] == PURPLE_ELITE)
            continue;

        const float dx = posX[i] - sourceX;
        const float dy = posY[i] - sourceY;
        const float d2 = dx * dx + dy * dy;

        if (d2 <= r2)
        {
            state[i] = FLEE;
            fearTimerMs[i] = durationMs;
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
        case RED:   r++; break;
        case BLUE:  b++; break;
        case PURPLE_ELITE: p++; break;
        }
    }
}
