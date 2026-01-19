// ZombieSystem.cpp
#include "ZombieSystem.h"
#include "NavGrid.h"
#include "GameConfig.h"

#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <cstdint>

using namespace GameConfig;

namespace
{
    float Rand01()
    {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    }

    void NormalizeSafe(float& x, float& y)
    {
        const float len2 = x * x + y * y;
        if (len2 > ZombieConfig::MOVEMENT_EPSILON)
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
}

bool ZombieSystem::PopOutIfStuck(float& x, float& y, float radius, const NavGrid& nav) const
{
    if (!nav.IsCircleBlocked(x, y, radius))
        return true;

    const int angles = ZombieConfig::UNSTUCK_SEARCH_ANGLES;
    const float step = radius * ZombieConfig::UNSTUCK_STEP_MULTIPLIER;
    const float maxR = radius * ZombieConfig::UNSTUCK_MAX_RADIUS_MULTIPLIER;

    for (float r = step; r <= maxR; r += step)
    {
        for (int a = 0; a < angles; a++)
        {
            const float t = (ZombieConfig::TWO_PI * a) / static_cast<float>(angles);
            const float nx = x + std::cos(t) * r;
            const float ny = y + std::sin(t) * r;

            if (!nav.IsCircleBlocked(nx, ny, radius))
            {
                x = nx;
                y = ny;
                return true;
            }
        }
    }

    return false;
}

bool ZombieSystem::ResolveMoveSlide(float& x, float& y, float vx, float vy, float dt,
    const NavGrid& nav, bool& outFullBlocked) const
{
    outFullBlocked = false;

    const float startX = x;
    const float startY = y;
    const float r = tuning.zombieRadius;

    // Attempt full move
    float nx = x + vx * dt;
    float ny = y + vy * dt;
    if (!nav.IsCircleBlocked(nx, ny, r))
    {
        x = nx;
        y = ny;
        return true;
    }

    outFullBlocked = true;

    // Try X-axis slide
    nx = x + vx * dt;
    ny = y;
    if (!nav.IsCircleBlocked(nx, ny, r))
    {
        x = nx;
        const float moved2 = (x - startX) * (x - startX) + (y - startY) * (y - startY);
        return moved2 > ZombieConfig::MOVEMENT_EPSILON;
    }

    // Try Y-axis slide
    nx = x;
    ny = y + vy * dt;
    if (!nav.IsCircleBlocked(nx, ny, r))
    {
        y = ny;
        const float moved2 = (x - startX) * (x - startX) + (y - startY) * (y - startY);
        return moved2 > ZombieConfig::MOVEMENT_EPSILON;
    }

    return false;
}

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

void ZombieSystem::Init(int maxZombies, const NavGrid& nav)
{
    maxCount = maxZombies;
    aliveCount = 0;

    // Allocate SoA storage
    posX.assign(maxCount, 0.0f);
    posY.assign(maxCount, 0.0f);
    velX.assign(maxCount, 0.0f);
    velY.assign(maxCount, 0.0f);
    type.assign(maxCount, 0);
    state.assign(maxCount, 0);
    fearTimerMs.assign(maxCount, 0.0f);
    attackCooldownMs.assign(maxCount, 0.0f);
    hp.assign(maxCount, 0);
    flowAssistMs.assign(maxCount, 0.0f);

    InitTypeStats();

    // Copy world bounds from NavGrid
    worldMinX = nav.WorldMinX();
    worldMinY = nav.WorldMinY();
    worldMaxX = nav.WorldMaxX();
    worldMaxY = nav.WorldMaxY();

    cellSize = tuning.sepRadius * 2.0f;

    gridW = static_cast<int>((worldMaxX - worldMinX) / cellSize) + 1;
    gridH = static_cast<int>((worldMaxY - worldMinY) / cellSize) + 1;

    const int totalCells = gridW * gridH;
    cellStart.assign(totalCells + 1, 0);
    cellCount.assign(totalCells, 0);
    cellList.assign(maxCount, 0);

    nearList.reserve(maxCount);

    killsThisFrame = 0;
    lastMoveKills = 0;
    gridDirty = true;
}

void ZombieSystem::Clear()
{
    aliveCount = 0;
    killsThisFrame = 0;
    lastMoveKills = 0;
    gridDirty = true;
}

bool ZombieSystem::SpawnAtWorld(float x, float y, uint8_t forcedType)
{
    if (aliveCount >= maxCount)
        return false;

    const int i = aliveCount++;
    const uint8_t t = (forcedType == 255) ? RollTypeWeighted() : forcedType;

    posX[i] = x;
    posY[i] = y;
    velX[i] = 0.0f;
    velY[i] = 0.0f;
    type[i] = t;
    state[i] = SEEK;
    fearTimerMs[i] = 0.0f;
    attackCooldownMs[i] = 0.0f;
    hp[i] = typeStats[t].maxHP;
    flowAssistMs[i] = 0.0f;

    gridDirty = true;
    return true;
}

void ZombieSystem::Spawn(int count, float playerX, float playerY)
{
    const float minR = ZombieConfig::SPAWN_MIN_RADIUS;
    const float maxR = ZombieConfig::SPAWN_MAX_RADIUS;

    for (int k = 0; k < count && aliveCount < maxCount; k++)
    {
        const int i = aliveCount++;
        const uint8_t t = RollTypeWeighted();

        const float ang = Rand01() * ZombieConfig::TWO_PI;
        const float r = minR + (maxR - minR) * Rand01();

        posX[i] = playerX + std::cos(ang) * r;
        posY[i] = playerY + std::sin(ang) * r;
        velX[i] = 0.0f;
        velY[i] = 0.0f;
        type[i] = t;
        state[i] = SEEK;
        fearTimerMs[i] = 0.0f;
        attackCooldownMs[i] = 0.0f;
        hp[i] = typeStats[t].maxHP;
        flowAssistMs[i] = 0.0f;
    }

    gridDirty = true;
}

void ZombieSystem::InitTypeStats()
{
    using ZC = GameConfig::ZombieConfig;

    typeStats[GREEN] = {
        ZC::GREEN_MAX_SPEED,
        ZC::ZOMBIE_SEEK_WEIGHT,
        ZC::ZOMBIE_SEP_WEIGHT,
        (uint16_t)ZC::GREEN_MAX_HP,
        (uint8_t)ZC::GREEN_TOUCH_DAMAGE,
        ZC::GREEN_ATTACK_COOLDOWN_MS,
        ZC::GREEN_FEAR_RADIUS
    };

    typeStats[RED] = {
        ZC::RED_MAX_SPEED,
        ZC::ZOMBIE_SEEK_WEIGHT,
        ZC::ZOMBIE_SEP_WEIGHT,
        (uint16_t)ZC::RED_MAX_HP,
        (uint8_t)ZC::RED_TOUCH_DAMAGE,
        ZC::RED_ATTACK_COOLDOWN_MS,
        ZC::RED_FEAR_RADIUS
    };

    typeStats[BLUE] = {
        ZC::BLUE_MAX_SPEED,
        ZC::ZOMBIE_SEEK_WEIGHT,
        ZC::ZOMBIE_SEP_WEIGHT,
        (uint16_t)ZC::BLUE_MAX_HP,
        (uint8_t)ZC::BLUE_TOUCH_DAMAGE,
        ZC::BLUE_ATTACK_COOLDOWN_MS,
        ZC::BLUE_FEAR_RADIUS
    };

    typeStats[PURPLE_ELITE] = {
        ZC::PURPLE_ELITE_MAX_SPEED,
        ZC::ZOMBIE_SEEK_WEIGHT,
        ZC::ZOMBIE_SEP_WEIGHT,
        (uint16_t)ZC::PURPLE_ELITE_MAX_HP,
        (uint8_t)ZC::PURPLE_ELITE_TOUCH_DAMAGE,
        ZC::PURPLE_ELITE_ATTACK_COOLDOWN_MS,
        ZC::PURPLE_ELITE_FEAR_RADIUS
    };
}

uint8_t ZombieSystem::RollTypeWeighted() const
{
    const float r = Rand01();

    if (r < ZombieConfig::GREEN_SPAWN_CHANCE)
        return GREEN;
    if (r < ZombieConfig::RED_SPAWN_CHANCE)
        return RED;
    if (r < ZombieConfig::BLUE_SPAWN_CHANCE)
        return BLUE;

    return PURPLE_ELITE;
}

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
    gridDirty = true;
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

int ZombieSystem::CellIndex(float x, float y) const
{
    int cx = static_cast<int>((x - worldMinX) / cellSize);
    int cy = static_cast<int>((y - worldMinY) / cellSize);

    cx = std::clamp(cx, 0, gridW - 1);
    cy = std::clamp(cy, 0, gridH - 1);

    return cy * gridW + cx;
}

void ZombieSystem::BuildSpatialGrid(float playerX, float playerY)
{
    const float sepActiveRadiusSq = tuning.sepActiveRadius * tuning.sepActiveRadius;
    const int cellN = gridW * gridH;

    // Build near zombie list
    nearList.clear();
    for (int i = 0; i < aliveCount; i++)
    {
        const float dx = playerX - posX[i];
        const float dy = playerY - posY[i];
        const float d2 = dx * dx + dy * dy;

        if (d2 <= sepActiveRadiusSq)
            nearList.push_back(i);
    }

    // Clear counts
    std::fill(cellCount.begin(), cellCount.end(), 0);

    // Count zombies per cell
    for (const int i : nearList)
    {
        const int c = CellIndex(posX[i], posY[i]);
        cellCount[c]++;
    }

    // Build prefix sum
    cellStart[0] = 0;
    for (int c = 0; c < cellN; c++)
    {
        cellStart[c + 1] = cellStart[c] + cellCount[c];
    }

    // Place zombies into grid (using cellStart as write cursor)
    for (const int i : nearList)
    {
        const int c = CellIndex(posX[i], posY[i]);
        const int dst = cellStart[c]++;
        cellList[dst] = i;
    }

    // Restore cellStart (shift back)
    for (int c = cellN; c > 0; c--)
    {
        cellStart[c] = cellStart[c - 1];
    }
    cellStart[0] = 0;
}

void ZombieSystem::TickTimers(int i, float deltaTimeMs)
{
    if (fearTimerMs[i] > 0.0f)
    {
        fearTimerMs[i] -= deltaTimeMs;
        if (fearTimerMs[i] < 0.0f)
            fearTimerMs[i] = 0.0f;
    }

    if (attackCooldownMs[i] > 0.0f)
    {
        attackCooldownMs[i] -= deltaTimeMs;
        if (attackCooldownMs[i] < 0.0f)
            attackCooldownMs[i] = 0.0f;
    }

    if (flowAssistMs[i] > 0.0f)
    {
        flowAssistMs[i] -= deltaTimeMs;
        if (flowAssistMs[i] < 0.0f)
            flowAssistMs[i] = 0.0f;
    }
}

void ZombieSystem::ApplyTouchDamage(int i, const ZombieTypeStats& s,
    float distSqToPlayer, float hitDistSq, int& outDamage)
{
    if (attackCooldownMs[i] <= 0.0f && distSqToPlayer <= hitDistSq)
    {
        outDamage += static_cast<int>(s.touchDamage);
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
    if (flowAssistMs[i] <= 0.0f)
        return;

    const float flowWeight = tuning.flowWeight;
    const int navCell = nav.CellIndex(posX[i], posY[i]);
    const float fx = nav.FlowXAtCell(navCell);
    const float fy = nav.FlowYAtCell(navCell);
    const float f2 = fx * fx + fy * fy;

    if (f2 <= ZombieConfig::MOVEMENT_EPSILON)
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

    int checked = 0;
    const int maxChecks = 32;

    float totalSepX = 0.0f;
    float totalSepY = 0.0f;

    // Check own cell first (most likely to have neighbors)
    {
        const int start = cellStart[c];
        const int end = cellStart[c + 1];

        for (int k = start; k < end && checked < maxChecks; k++)
        {
            const int j = cellList[k];
            if (j == i) continue;

            const float ax = posX[i] - posX[j];
            const float ay = posY[i] - posY[j];
            const float d2 = ax * ax + ay * ay;

            if (d2 > ZombieConfig::MOVEMENT_EPSILON && d2 < sepRadiusSq)
            {
                const float d = std::sqrt(d2);
                const float invD = 1.0f / d;
                const float push = (sepRadius - d) / sepRadius;

                totalSepX += ax * invD * push;
                totalSepY += ay * invD * push;
                checked++;
            }
        }
    }

    // Only check adjacent cells if needed
    if (checked < maxChecks)
    {
        for (int oy = -1; oy <= 1; oy++)
        {
            const int ny = cy + oy;
            if (ny < 0 || ny >= gridH) continue;

            for (int ox = -1; ox <= 1; ox++)
            {
                if (ox == 0 && oy == 0) continue;  // Already checked own cell

                const int nx = cx + ox;
                if (nx < 0 || nx >= gridW) continue;

                const int nc = ny * gridW + nx;
                const int start = cellStart[nc];
                const int end = cellStart[nc + 1];

                for (int k = start; k < end && checked < maxChecks; k++)
                {
                    const int j = cellList[k];

                    const float ax = posX[i] - posX[j];
                    const float ay = posY[i] - posY[j];
                    const float d2 = ax * ax + ay * ay;

                    if (d2 > ZombieConfig::MOVEMENT_EPSILON && d2 < sepRadiusSq)
                    {
                        const float d = std::sqrt(d2);
                        const float invD = 1.0f / d;
                        const float push = (sepRadius - d) / sepRadius;

                        totalSepX += ax * invD * push;
                        totalSepY += ay * invD * push;
                        checked++;
                    }
                }
            }
        }
    }

    outSepX = totalSepX;
    outSepY = totalSepY;
}

void ZombieSystem::LightweightUpdate(float deltaTimeMs)
{
    const float dt = std::min(deltaTimeMs, ZombieConfig::MAX_DELTA_TIME_MS) / 1000.0f;
    if (dt <= 0.0f)
        return;

    for (int i = 0; i < aliveCount; ++i)
    {
        TickTimers(i, deltaTimeMs);

        // Simple drift movement using last computed velocity
        posX[i] += velX[i] * dt * ZombieConfig::DRIFT_SPEED_MULTIPLIER;
        posY[i] += velY[i] * dt * ZombieConfig::DRIFT_SPEED_MULTIPLIER;
    }
}

int ZombieSystem::Update(float deltaTimeMs, float playerX, float playerY, const NavGrid& nav)
{
    deltaTimeMs = std::min(deltaTimeMs, ZombieConfig::MAX_DELTA_TIME_MS);
    const float dt = deltaTimeMs / 1000.0f;

    if (dt <= 0.0f)
        return 0;

    int damageThisFrame = 0;

    const float hitDist = tuning.playerRadius + tuning.zombieRadius;
    const float hitDistSq = hitDist * hitDist;
    const float sepActiveRadiusSq = tuning.sepActiveRadius * tuning.sepActiveRadius;
    const float fleeDespawnRadiusSq = tuning.fleeDespawnRadius * tuning.fleeDespawnRadius;

    // LOD radii
    const float noCollisionR = tuning.sepActiveRadius * ZombieConfig::NO_COLLISION_MULTIPLIER;
    const float noCollisionRSq = noCollisionR * noCollisionR;
    const float farCheapR = tuning.sepActiveRadius * ZombieConfig::FAR_CHEAP_MULTIPLIER;
    const float farCheapRSq = farCheapR * farCheapR;
    const float veryFarCheapRSq = farCheapRSq * ZombieConfig::VERY_FAR_CHEAP_MULTIPLIER;

    // Frame counter for staggering
    static uint32_t frameCounter = 0;
    frameCounter++;

    gridRebuildTimerMs += deltaTimeMs;
    if (gridRebuildTimerMs >= GRID_REBUILD_INTERVAL_MS || gridDirty)
    {
        gridRebuildTimerMs = 0.0f;
        gridDirty = false;
        BuildSpatialGrid(playerX, playerY);
    }

    // Process all zombies
    for (int i = 0; i < aliveCount; )
    {
        TickTimers(i, deltaTimeMs);

        // Unstuck if spawned in wall
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

        // Despawn feared zombies far from player
        if (feared && distSqToPlayer > fleeDespawnRadiusSq)
        {
            Despawn(i);
            continue;
        }

        // Feared zombies flee
        if (feared)
        {
            float dx, dy;
            ComputeFleeDir(i, playerX, playerY, dx, dy);

            velX[i] = dx * s.maxSpeed;
            velY[i] = dy * s.maxSpeed;

            bool fullBlocked = false;
            ResolveMoveSlide(posX[i], posY[i], velX[i], velY[i], dt, nav, fullBlocked);

            if (fullBlocked || nav.IsCircleBlocked(posX[i], posY[i], tuning.zombieRadius))
                PopOutIfStuck(posX[i], posY[i], tuning.zombieRadius, nav);

            i++;
            continue;
        }

        // Normal seeking behavior
        float dx, dy;
        ComputeSeekDir(i, playerX, playerY, dx, dy);

        // FAR LOD
        if (distSqToPlayer > sepActiveRadiusSq)
        {
            uint32_t rateMask = 1u;
            if (distSqToPlayer > farCheapRSq)
                rateMask = 3u;
            if (distSqToPlayer > veryFarCheapRSq)
                rateMask = 7u;

            if (((frameCounter + static_cast<uint32_t>(i)) & rateMask) != 0u)
            {
                // Cheap drift
                posX[i] += velX[i] * dt;
                posY[i] += velY[i] * dt;

                if (nav.IsCircleBlocked(posX[i], posY[i], tuning.zombieRadius))
                    PopOutIfStuck(posX[i], posY[i], tuning.zombieRadius, nav);

                i++;
                continue;
            }

            // Recompute velocity
            velX[i] = dx * s.maxSpeed;
            velY[i] = dy * s.maxSpeed;

            const bool skipCollision = (distSqToPlayer > noCollisionRSq);
            if (skipCollision)
            {
                posX[i] += velX[i] * dt;
                posY[i] += velY[i] * dt;

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

        // NEAR (staggered flow + separation)
        const bool doFlow = (((frameCounter + static_cast<uint32_t>(i)) & 1u) == 0u);
        if (doFlow)
            ApplyFlowAssistIfActive(i, nav, dx, dy);

        float sepX = 0.0f;
        float sepY = 0.0f;
        const bool doSep = (((frameCounter + static_cast<uint32_t>(i)) & 1u) == 0u);
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

        if (fullBlocked || nav.IsCircleBlocked(posX[i], posY[i], tuning.zombieRadius))
            PopOutIfStuck(posX[i], posY[i], tuning.zombieRadius, nav);

        i++;
    }

    return damageThisFrame;
}

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
        case GREEN:
            g++;
            break;
        case RED:
            r++;
            break;
        case BLUE:
            b++;
            break;
        case PURPLE_ELITE:
            p++;
            break;
        }
    }
}

float ZombieSystem::Clamp01(float v)
{
    return std::clamp(v, 0.0f, 1.0f);
}

void ZombieSystem::NormalizeSafe(float& x, float& y)
{
    ::NormalizeSafe(x, y);
}