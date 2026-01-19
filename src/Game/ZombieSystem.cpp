// ZombieSystem.cpp (FULL + ULTRA SCALE + STAGGERED SEEK + POP-OUT FIX)
#include "ZombieSystem.h"
#include "NavGrid.h"

#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdint>

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

// -------------------- pop-out (unstuck) --------------------
bool ZombieSystem::PopOutIfStuck(float& x, float& y, float radius, const NavGrid& nav) const
{
    if (!nav.IsCircleBlocked(x, y, radius))
        return true;

    const int angles = 16;
    const float step = radius * 0.75f;   // how fast we expand
    const float maxR = radius * 12.0f;   // how far we search

    for (float r = step; r <= maxR; r += step)
    {
        for (int a = 0; a < angles; a++)
        {
            const float t = (6.2831853f * a) / (float)angles;
            const float nx = x + std::cos(t) * r;
            const float ny = y + std::sin(t) * r;

            if (!nav.IsCircleBlocked(nx, ny, radius))
            {
                x = nx; y = ny;
                return true;
            }
        }
    }

    return false;
}

// -------------------- movement (axis slide) --------------------
bool ZombieSystem::ResolveMoveSlide(float& x, float& y, float vx, float vy, float dt,
    const NavGrid& nav, bool& outFullBlocked) const
{
    outFullBlocked = false;

    const float startX = x;
    const float startY = y;

    const float r = tuning.zombieRadius;

    // Full move
    float nx = x + vx * dt;
    float ny = y + vy * dt;
    if (!nav.IsCircleBlocked(nx, ny, r))
    {
        x = nx; y = ny;
        return true;
    }

    outFullBlocked = true;

    // Slide X only
    nx = x + vx * dt;
    ny = y;
    if (!nav.IsCircleBlocked(nx, ny, r))
    {
        x = nx;
        const float moved2 = (x - startX) * (x - startX) + (y - startY) * (y - startY);
        return moved2 > 0.0001f;
    }

    // Slide Y only
    nx = x;
    ny = y + vy * dt;
    if (!nav.IsCircleBlocked(nx, ny, r))
    {
        y = ny;
        const float moved2 = (x - startX) * (x - startX) + (y - startY) * (y - startY);
        return moved2 > 0.0001f;
    }

    return false;
}

// -------------------- kill counter --------------------
void ZombieSystem::BeginFrame()
{
    killsThisFrame = 0;
}

int ZombieSystem::ConsumeKillsThisFrame()
{
    const int k = killsThisFrame;
    killsThisFrame = 0;
    lastMoveKills = k;
    return k;
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

    killsThisFrame = 0;
    lastMoveKills = 0;
}

void ZombieSystem::Clear()
{
    aliveCount = 0;
    killsThisFrame = 0;
    lastMoveKills = 0;
}

bool ZombieSystem::SpawnAtWorld(float x, float y, uint8_t forcedType)
{
    if (aliveCount >= maxCount) return false;

    const int i = aliveCount++;
    const uint8_t t = (forcedType == 255) ? RollTypeWeighted() : forcedType;

    posX[i] = x;
    posY[i] = y;
    velX[i] = 0.f;
    velY[i] = 0.f;

    type[i] = t;
    state[i] = SEEK;

    fearTimerMs[i] = 0.f;
    attackCooldownMs[i] = 0.f;
    hp[i] = typeStats[t].maxHP;

    flowAssistMs[i] = 0.0f;
    return true;
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

// -------------------- types --------------------
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

// -------------------- swap remove --------------------
void ZombieSystem::KillSwapRemove(int index)
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

void ZombieSystem::Despawn(int index)
{
    KillSwapRemove(index);
}

void ZombieSystem::KillByPlayer(int index)
{
    killsThisFrame++;
    KillSwapRemove(index);
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

// Build separation grid using ONLY indices in 'indices'
static void BuildGridNear_Internal(
    int gridW, int gridH,
    float worldMinX, float worldMinY,
    float cellSize,
    const std::vector<float>& posX,
    const std::vector<float>& posY,
    const std::vector<int>& indices,
    std::vector<int>& cellCount,
    std::vector<int>& cellStart,
    std::vector<int>& writeCursor,
    std::vector<int>& cellList)
{
    const int cellN = gridW * gridH;

    std::fill(cellCount.begin(), cellCount.end(), 0);

    auto CellIndexLocal = [&](float x, float y) -> int
        {
            int cx = (int)((x - worldMinX) / cellSize);
            int cy = (int)((y - worldMinY) / cellSize);

            cx = std::clamp(cx, 0, gridW - 1);
            cy = std::clamp(cy, 0, gridH - 1);

            return cy * gridW + cx;
        };

    for (int idx = 0; idx < (int)indices.size(); idx++)
    {
        const int i = indices[idx];
        const int c = CellIndexLocal(posX[i], posY[i]);
        cellCount[c]++;
    }

    cellStart[0] = 0;
    for (int c = 0; c < cellN; c++)
        cellStart[c + 1] = cellStart[c] + cellCount[c];

    writeCursor = cellStart;

    for (int idx = 0; idx < (int)indices.size(); idx++)
    {
        const int i = indices[idx];
        const int c = CellIndexLocal(posX[i], posY[i]);
        const int dst = writeCursor[c]++;
        cellList[dst] = i;
    }
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
    if (f2 <= 0.0001f) return;

    ioDX = ioDX * (1.0f - flowWeight) + fx * flowWeight;
    ioDY = ioDY * (1.0f - flowWeight) + fy * flowWeight;
    NormalizeSafe(ioDX, ioDY);
}

// capped separation
void ZombieSystem::ComputeSeparation(int i, float& outSepX, float& outSepY) const
{
    outSepX = 0.0f;
    outSepY = 0.0f;

    const float sepRadius = tuning.sepRadius;
    const float sepRadiusSq = sepRadius * sepRadius;

    const int c = CellIndex(posX[i], posY[i]);
    const int cx = c % gridW;
    const int cy = c / gridW;

    int checked = 0;
    const int maxChecks = 32;

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

                    checked++;
                    if (checked >= maxChecks)
                        return;
                }
            }
        }
    }
}

// -------------------- update --------------------
int ZombieSystem::Update(float deltaTimeMs, float playerX, float playerY, const NavGrid& nav)
{
    // dt clamp helps prevent tunneling on frame spikes
    deltaTimeMs = std::min(deltaTimeMs, 33.0f);

    const float dt = deltaTimeMs / 1000.0f;
    if (dt <= 0.0f) return 0;

    int damageThisFrame = 0;

    const float hitDist = tuning.playerRadius + tuning.zombieRadius;
    const float hitDistSq = hitDist * hitDist;

    const float sepActiveRadiusSq = tuning.sepActiveRadius * tuning.sepActiveRadius;
    const float fleeDespawnRadiusSq = tuning.fleeDespawnRadius * tuning.fleeDespawnRadius;

    // Far LOD radii (tune)
    const float noCollisionR = tuning.sepActiveRadius * 1.25f;
    const float noCollisionRSq = noCollisionR * noCollisionR;

    const float farCheapR = tuning.sepActiveRadius * 2.5f;
    const float farCheapRSq = farCheapR * farCheapR;

    const float veryFarCheapRSq = farCheapRSq * 2.25f;

    // frame counter (stagger)
    static uint32_t frameCounter = 0;
    frameCounter++;

    // Build near list once
    static std::vector<int> nearList;
    nearList.clear();
    nearList.reserve(aliveCount);

    for (int i = 0; i < aliveCount; i++)
    {
        const float dx = playerX - posX[i];
        const float dy = playerY - posY[i];
        const float d2 = dx * dx + dy * dy;
        if (d2 <= sepActiveRadiusSq)
            nearList.push_back(i);
    }

    // Build separation grid using nearList only
    BuildGridNear_Internal(
        gridW, gridH,
        worldMinX, worldMinY,
        cellSize,
        posX, posY,
        nearList,
        cellCount,
        cellStart,
        writeCursor,
        cellList);

    for (int i = 0; i < aliveCount; )
    {
        TickTimers(i, deltaTimeMs);

        // If a zombie is already overlapping a wall (spawned there or dt spike),
        // pop it out before doing anything.
        if (nav.IsCircleBlocked(posX[i], posY[i], tuning.zombieRadius))
        {
            PopOutIfStuck(posX[i], posY[i], tuning.zombieRadius, nav);
        }

        const uint8_t zt = type[i];
        const ZombieTypeStats& s = typeStats[zt];

        const float toPX = playerX - posX[i];
        const float toPY = playerY - posY[i];
        const float distSqToPlayer = toPX * toPX + toPY * toPY;

        const bool feared = (fearTimerMs[i] > 0.0f);

        ApplyTouchDamage(i, s, distSqToPlayer, hitDistSq, damageThisFrame);

        // Feared despawn far away
        if (feared && distSqToPlayer > fleeDespawnRadiusSq)
        {
            Despawn(i);
            continue;
        }

        // ---- feared: flee ----
        if (feared)
        {
            float dx, dy;
            ComputeFleeDir(i, playerX, playerY, dx, dy);

            velX[i] = dx * s.maxSpeed;
            velY[i] = dy * s.maxSpeed;

            bool fullBlocked = false;
            (void)ResolveMoveSlide(posX[i], posY[i], velX[i], velY[i], dt, nav, fullBlocked);

            // If still overlapping, pop out
            if (fullBlocked || nav.IsCircleBlocked(posX[i], posY[i], tuning.zombieRadius))
                PopOutIfStuck(posX[i], posY[i], tuning.zombieRadius, nav);

            i++;
            continue;
        }

        // ---- seek base dir ----
        float dx, dy;
        ComputeSeekDir(i, playerX, playerY, dx, dy);

        // -------------------- FAR LOD --------------------
        if (distSqToPlayer > sepActiveRadiusSq)
        {
            uint32_t rateMask = 1u;
            if (distSqToPlayer > farCheapRSq)      rateMask = 3u;
            if (distSqToPlayer > veryFarCheapRSq)  rateMask = 7u;

            if (((frameCounter + (uint32_t)i) & rateMask) != 0u)
            {
                // cheap drift
                posX[i] += velX[i] * dt;
                posY[i] += velY[i] * dt;

                // if drift pushed us into a wall (rare), pop out
                if (nav.IsCircleBlocked(posX[i], posY[i], tuning.zombieRadius))
                    PopOutIfStuck(posX[i], posY[i], tuning.zombieRadius, nav);

                i++;
                continue;
            }

            // recompute velocity this tick
            velX[i] = dx * s.maxSpeed;
            velY[i] = dy * s.maxSpeed;

            const bool skipCollision = (distSqToPlayer > noCollisionRSq);
            if (skipCollision)
            {
                posX[i] += velX[i] * dt;
                posY[i] += velY[i] * dt;

                // even when skipping collision, don't allow being stuck forever
                if (nav.IsCircleBlocked(posX[i], posY[i], tuning.zombieRadius))
                    PopOutIfStuck(posX[i], posY[i], tuning.zombieRadius, nav);
            }
            else
            {
                bool fullBlocked = false;
                const bool moved = ResolveMoveSlide(posX[i], posY[i], velX[i], velY[i], dt, nav, fullBlocked);
                if (fullBlocked || !moved)
                    flowAssistMs[i] = tuning.flowAssistBurstMs;

                if (fullBlocked || nav.IsCircleBlocked(posX[i], posY[i], tuning.zombieRadius))
                    PopOutIfStuck(posX[i], posY[i], tuning.zombieRadius, nav);
            }

            i++;
            continue;
        }

        // -------------------- NEAR (staggered flow + staggered separation) --------------------
        const bool doFlow = (((frameCounter + (uint32_t)i) & 1u) == 0u);
        if (doFlow)
            ApplyFlowAssistIfActive(i, nav, dx, dy);

        float sepX = 0.0f;
        float sepY = 0.0f;

        const uint32_t sepMask = 1u;
        const bool doSep = (((frameCounter + (uint32_t)i) & sepMask) == 0u);
        if (doSep)
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

        // Final safety: if we still overlap a blocked tile, pop out
        if (fullBlocked || nav.IsCircleBlocked(posX[i], posY[i], tuning.zombieRadius))
            PopOutIfStuck(posX[i], posY[i], tuning.zombieRadius, nav);

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
