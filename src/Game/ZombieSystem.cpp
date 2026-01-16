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
}

void ZombieSystem::InitTypeStats()
{
    typeStats[GREEN] = { 60.f, 1.f, 1.f, 1, 1, 750.f, 0.f };
    typeStats[RED] = { 90.f, 1.f, 1.f, 1, 1, 650.f, 0.f };
    typeStats[BLUE] = { 45.f, 1.f, 1.f, 4, 2, 900.f, 0.f };
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

void ZombieSystem::Update(float deltaTimeMs)
{
    for (int i = 0; i < aliveCount; i++)
    {
        if (fearTimerMs[i] > 0.f)
            fearTimerMs[i] = std::max(0.f, fearTimerMs[i] - deltaTimeMs);

        if (attackCooldownMs[i] > 0.f)
            attackCooldownMs[i] = std::max(0.f, attackCooldownMs[i] - deltaTimeMs);
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
