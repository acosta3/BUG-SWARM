// AttackSystem.cpp
#include "AttackSystem.h"
#include "ZombieSystem.h"
#include "HiveSystem.h"
#include "CameraSystem.h"
#include <cmath>
#include <algorithm>

static float Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

// Small (<=0.7): dmg 0.70x, radius 0.60x
// Normal (~1.0):  dmg 1.00x, radius 1.00x
// Big (>=1.3):    dmg 1.45x, radius 1.80x
static void GetAttackMultsFromScale(float s, float& outDmgMult, float& outRadMult)
{
    const float smallS = 0.7f;
    const float bigS = 1.3f;

    const float smallDmg = 0.70f;
    const float smallRad = 0.60f;

    const float normDmg = 1.00f;
    const float normRad = 1.00f;

    const float bigDmg = 1.45f;
    const float bigRad = 1.80f;

    if (s <= smallS)
    {
        outDmgMult = smallDmg;
        outRadMult = smallRad;
        return;
    }

    if (s >= bigS)
    {
        outDmgMult = bigDmg;
        outRadMult = bigRad;
        return;
    }

    if (s < 1.0f)
    {
        float t = (s - smallS) / (1.0f - smallS);
        t = Clamp01(t);
        outDmgMult = Lerp(smallDmg, normDmg, t);
        outRadMult = Lerp(smallRad, normRad, t);
    }
    else
    {
        float t = (s - 1.0f) / (bigS - 1.0f);
        t = Clamp01(t);
        outDmgMult = Lerp(normDmg, bigDmg, t);
        outRadMult = Lerp(normRad, bigRad, t);
    }
}

void AttackSystem::Init()
{
    pulseCooldownMs = 0.0f;
    slashCooldownMs = 0.0f;
    meteorCooldownMs = 0.0f;

    lastPulseKills = 0;
    lastSlashKills = 0;
    lastMeteorKills = 0;
}

void AttackSystem::TickCooldown(float& cd, float dtMs)
{
    if (cd <= 0.0f) return;
    cd -= dtMs;
    if (cd < 0.0f) cd = 0.0f;
}

void AttackSystem::Update(float deltaTimeMs)
{
    TickCooldown(pulseCooldownMs, deltaTimeMs);
    TickCooldown(slashCooldownMs, deltaTimeMs);
    TickCooldown(meteorCooldownMs, deltaTimeMs);
}

void AttackSystem::Process(const AttackInput& in,
    float playerX, float playerY,
    float playerScale,
    ZombieSystem& zombies,
    HiveSystem& hives,
    CameraSystem& camera)
{
    // Reset "last kills" each frame (only set when that move is used)
    lastPulseKills = 0;
    lastSlashKills = 0;
    lastMeteorKills = 0;

    if (in.pulsePressed && pulseCooldownMs <= 0.0f)
    {
        DoPulse(playerX, playerY, playerScale, zombies, hives, camera);
        pulseCooldownMs = 200.0f;
    }

    if (in.slashPressed && slashCooldownMs <= 0.0f)
    {
        DoSlash(playerX, playerY, playerScale, in.aimX, in.aimY, zombies, hives, camera);
        slashCooldownMs = 350.0f;
    }

    if (in.meteorPressed && meteorCooldownMs <= 0.0f)
    {
        DoMeteor(playerX, playerY, playerScale, in.aimX, in.aimY, zombies, hives, camera);
        meteorCooldownMs = 900.0f;
    }
}

static void TriggerFearIfEliteKilled(bool eliteKilled, float fx, float fy, ZombieSystem& zombies, CameraSystem& camera)
{
    if (!eliteKilled) return;

    const float fearRadius = 750.0f;
    const float fearDurationMs = 1200.0f;

    zombies.TriggerFear(fx, fy, fearRadius, fearDurationMs);
    camera.AddShake(10.0f, 0.12f);
}

void AttackSystem::DoPulse(float px, float py, float playerScale, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    float dmgMult, radMult;
    GetAttackMultsFromScale(playerScale, dmgMult, radMult);

    const float radius = 80.0f * radMult;
    const float r2 = radius * radius;

    int killed = 0;
    bool eliteKilled = false;

    for (int i = 0; i < zombies.AliveCount(); )
    {
        const float dx = zombies.GetX(i) - px;
        const float dy = zombies.GetY(i) - py;
        const float d2 = dx * dx + dy * dy;

        if (d2 <= r2)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE)
                eliteKilled = true;

            zombies.KillByPlayer(i); // IMPORTANT: no i++ (swap remove)
            killed++;
            continue;
        }

        i++;
    }

    lastPulseKills = killed;

    const float hiveDamage = 20.0f * dmgMult;
    const bool hitHive = hives.DamageHiveAt(px, py, radius, hiveDamage);

    if (killed > 0)
        camera.AddShake(6.0f, 0.08f);
    if (hitHive)
        camera.AddShake(4.0f, 0.05f);

    TriggerFearIfEliteKilled(eliteKilled, px, py, zombies, camera);
}

void AttackSystem::DoSlash(float px, float py, float playerScale, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    float dmgMult, radMult;
    GetAttackMultsFromScale(playerScale, dmgMult, radMult);

    const float range = 140.0f * radMult;
    const float range2 = range * range;
    const float cosHalfAngle = 0.707f;

    float len2 = aimX * aimX + aimY * aimY;
    if (len2 > 0.0001f)
    {
        const float inv = 1.0f / std::sqrt(len2);
        aimX *= inv;
        aimY *= inv;
    }
    else
    {
        aimX = 0.0f;
        aimY = 1.0f;
    }

    int killed = 0;
    bool eliteKilled = false;

    for (int i = 0; i < zombies.AliveCount(); )
    {
        const float zx = zombies.GetX(i) - px;
        const float zy = zombies.GetY(i) - py;

        const float d2 = zx * zx + zy * zy;
        if (d2 > range2)
        {
            i++;
            continue;
        }

        const float d = std::sqrt(d2);
        if (d < 0.0001f)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE)
                eliteKilled = true;

            zombies.KillByPlayer(i);
            killed++;
            continue;
        }

        const float nx = zx / d;
        const float ny = zy / d;

        const float dot = nx * aimX + ny * aimY;
        if (dot >= cosHalfAngle)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE)
                eliteKilled = true;

            zombies.KillByPlayer(i);
            killed++;
            continue;
        }

        i++;
    }

    lastSlashKills = killed;

    const float slashCenterDist = 90.0f * radMult;
    const float hx = px + aimX * slashCenterDist;
    const float hy = py + aimY * slashCenterDist;
    const float slashHitRadius = 70.0f * radMult;

    const float hiveDamage = 30.0f * dmgMult;
    const bool hitHive = hives.DamageHiveAt(hx, hy, slashHitRadius, hiveDamage);

    if (killed > 0)
        
        camera.AddShake(5.0f, 0.06f);
    if (hitHive)
        camera.AddShake(5.0f, 0.06f);

    TriggerFearIfEliteKilled(eliteKilled, px, py, zombies, camera);
}

void AttackSystem::DoMeteor(float px, float py, float playerScale, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    float dmgMult, radMult;
    GetAttackMultsFromScale(playerScale, dmgMult, radMult);

    float len2 = aimX * aimX + aimY * aimY;
    if (len2 > 0.0001f)
    {
        const float inv = 1.0f / std::sqrt(len2);
        aimX *= inv;
        aimY *= inv;
    }
    else
    {
        aimX = 0.0f;
        aimY = 1.0f;
    }

    const float targetDist = 260.0f;
    const float tx = px + aimX * targetDist;
    const float ty = py + aimY * targetDist;

    const float radius = 120.0f * radMult;
    const float r2 = radius * radius;

    int killed = 0;
    bool eliteKilled = false;

    for (int i = 0; i < zombies.AliveCount(); )
    {
        const float dx = zombies.GetX(i) - tx;
        const float dy = zombies.GetY(i) - ty;
        const float d2 = dx * dx + dy * dy;

        if (d2 <= r2)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE)
                eliteKilled = true;

            zombies.KillByPlayer(i);
            killed++;
            continue;
        }

        i++;
    }

    lastMeteorKills = killed;

    const float hiveDamage = 50.0f * dmgMult;
    const bool hitHive = hives.DamageHiveAt(tx, ty, radius, hiveDamage);

    if (killed > 0)
        camera.AddShake(8.0f, 0.10f);
    if (hitHive)
        camera.AddShake(9.0f, 0.12f);

    TriggerFearIfEliteKilled(eliteKilled, tx, ty, zombies, camera);
}
