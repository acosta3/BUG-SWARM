#include "AttackSystem.h"
#include "ZombieSystem.h"
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
    CameraSystem& camera)
{
    // Pulse
    if (in.pulsePressed && pulseCooldownMs <= 0.0f)
    {
        DoPulse(playerX, playerY, zombies, camera);
        pulseCooldownMs = 200.0f;
    }

    // Slash
    if (in.slashPressed && slashCooldownMs <= 0.0f)
    {
        DoSlash(playerX, playerY, in.aimX, in.aimY, zombies, camera);
        slashCooldownMs = 350.0f;
    }

    // Meteor
    if (in.meteorPressed && meteorCooldownMs <= 0.0f)
    {
        DoMeteor(playerX, playerY, in.aimX, in.aimY, zombies, camera);
        meteorCooldownMs = 900.0f;
    }
}

void AttackSystem::DoPulse(float px, float py, ZombieSystem& zombies, CameraSystem& camera)
{
    const float radius = 80.0f;
    const float r2 = radius * radius;

    int killed = 0;

    for (int i = 0; i < zombies.AliveCount(); )
    {
        float dx = zombies.GetX(i) - px;
        float dy = zombies.GetY(i) - py;
        float d2 = dx * dx + dy * dy;

        if (d2 <= r2)
        {
            zombies.Kill(i); // swap-remove
            killed++;
        }
        else
        {
            i++;
        }
    }

    if (killed > 0)
        camera.AddShake(6.0f, 0.08f);
}

void AttackSystem::DoSlash(float px, float py, float aimX, float aimY, ZombieSystem& zombies, CameraSystem& camera)
{
    // Cone attack in front of player
    // Range + angle are tunable
    const float range = 140.0f;
    const float range2 = range * range;

    // cos(halfAngle). 0.707 = 45 degrees half-angle
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
            zombies.Kill(i);
            killed++;
            continue;
        }

        float nx = zx / d;
        float ny = zy / d;

        float dot = nx * aimX + ny * aimY; // -1..1
        if (dot >= cosHalfAngle)
        {
            zombies.Kill(i);
            killed++;
        }
        else
        {
            i++;
        }
    }

    if (killed > 0)
        camera.AddShake(5.0f, 0.06f);
}

void AttackSystem::DoMeteor(float px, float py, float aimX, float aimY, ZombieSystem& zombies, CameraSystem& camera)
{
    // Simple target point in front of player.
    // Later you can target mouse position or right-stick direction.
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

    for (int i = 0; i < zombies.AliveCount(); )
    {
        float dx = zombies.GetX(i) - tx;
        float dy = zombies.GetY(i) - ty;
        float d2 = dx * dx + dy * dy;

        if (d2 <= r2)
        {
            zombies.Kill(i);
            killed++;
        }
        else
        {
            i++;
        }
    }

    if (killed > 0)
        camera.AddShake(8.0f, 0.10f);
}
