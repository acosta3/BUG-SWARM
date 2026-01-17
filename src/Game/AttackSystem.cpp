#include "AttackSystem.h"
#include "ZombieSystem.h"
#include "HiveSystem.h"
#include "CameraSystem.h"
#include <cmath>

void AttackSystem::Init()
{
    pulseCooldownMs = 0.0f;
    slashCooldownMs = 0.0f;
    meteorCooldownMs = 0.0f;
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
    ZombieSystem& zombies,
    HiveSystem& hives,
    CameraSystem& camera)
{
    // Pulse
    if (in.pulsePressed && pulseCooldownMs <= 0.0f)
    {
        DoPulse(playerX, playerY, zombies, hives, camera);
        pulseCooldownMs = 200.0f;
    }

    // Slash
    if (in.slashPressed && slashCooldownMs <= 0.0f)
    {
        DoSlash(playerX, playerY, in.aimX, in.aimY, zombies, hives, camera);
        slashCooldownMs = 350.0f;
    }

    // Meteor
    if (in.meteorPressed && meteorCooldownMs <= 0.0f)
    {
        DoMeteor(playerX, playerY, in.aimX, in.aimY, zombies, hives, camera);
        meteorCooldownMs = 900.0f;
    }
}

static void TriggerFearIfEliteKilled(bool eliteKilled, float fx, float fy, ZombieSystem& zombies, CameraSystem& camera)
{
    if (!eliteKilled) return;

    // Tunables
    const float fearRadius = 750.0f;
    const float fearDurationMs = 1200.0f;

    zombies.TriggerFear(fx, fy, fearRadius, fearDurationMs);

    // extra feedback
    camera.AddShake(10.0f, 0.12f);
}

void AttackSystem::DoPulse(float px, float py, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    const float radius = 80.0f;
    const float r2 = radius * radius;

    int killed = 0;
    bool eliteKilled = false;

    for (int i = 0; i < zombies.AliveCount(); )
    {
        float dx = zombies.GetX(i) - px;
        float dy = zombies.GetY(i) - py;
        float d2 = dx * dx + dy * dy;

        if (d2 <= r2)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE)
                eliteKilled = true;

            zombies.Kill(i); // swap-remove
            killed++;
        }
        else
        {
            i++;
        }
    }

    // ALSO damage hives in the pulse radius
    const float hiveDamage = 20.0f;
    const bool hitHive = hives.DamageHiveAt(px, py, radius, hiveDamage);
    if (hitHive)
        camera.AddShake(4.0f, 0.05f);

    if (killed > 0)
        camera.AddShake(6.0f, 0.08f);

    TriggerFearIfEliteKilled(eliteKilled, px, py, zombies, camera);
}

void AttackSystem::DoSlash(float px, float py, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    const float range = 140.0f;
    const float range2 = range * range;
    const float cosHalfAngle = 0.707f;

    // Normalize aim
    float len2 = aimX * aimX + aimY * aimY;
    if (len2 > 0.0001f)
    {
        float inv = 1.0f / std::sqrt(len2);
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
        float zx = zombies.GetX(i) - px;
        float zy = zombies.GetY(i) - py;

        float d2 = zx * zx + zy * zy;
        if (d2 > range2)
        {
            i++;
            continue;
        }

        float d = std::sqrt(d2);
        if (d < 0.0001f)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE)
                eliteKilled = true;

            zombies.Kill(i);
            killed++;
            continue;
        }

        float nx = zx / d;
        float ny = zy / d;

        float dot = nx * aimX + ny * aimY;
        if (dot >= cosHalfAngle)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE)
                eliteKilled = true;

            zombies.Kill(i);
            killed++;
        }
        else
        {
            i++;
        }
    }

    // ALSO damage hives with a slash hit-circle in front of player
    // (cheap + feels like a melee cone)
    const float slashCenterDist = 90.0f;
    const float hx = px + aimX * slashCenterDist;
    const float hy = py + aimY * slashCenterDist;
    const float slashHitRadius = 70.0f;
    const float hiveDamage = 30.0f;

    const bool hitHive = hives.DamageHiveAt(hx, hy, slashHitRadius, hiveDamage);
    if (hitHive)
        camera.AddShake(5.0f, 0.06f);

    if (killed > 0)
        camera.AddShake(5.0f, 0.06f);

    TriggerFearIfEliteKilled(eliteKilled, px, py, zombies, camera);
}

void AttackSystem::DoMeteor(float px, float py, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    // Normalize aim
    float len2 = aimX * aimX + aimY * aimY;
    if (len2 > 0.0001f)
    {
        float inv = 1.0f / std::sqrt(len2);
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

    const float radius = 120.0f;
    const float r2 = radius * radius;

    int killed = 0;
    bool eliteKilled = false;

    for (int i = 0; i < zombies.AliveCount(); )
    {
        float dx = zombies.GetX(i) - tx;
        float dy = zombies.GetY(i) - ty;
        float d2 = dx * dx + dy * dy;

        if (d2 <= r2)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE)
                eliteKilled = true;

            zombies.Kill(i);
            killed++;
        }
        else
        {
            i++;
        }
    }

    // ALSO damage hives at meteor impact
    const float hiveDamage = 50.0f;
    const bool hitHive = hives.DamageHiveAt(tx, ty, radius, hiveDamage);
    if (hitHive)
        camera.AddShake(9.0f, 0.12f);

    if (killed > 0)
        camera.AddShake(8.0f, 0.10f);

    TriggerFearIfEliteKilled(eliteKilled, tx, ty, zombies, camera);
}
