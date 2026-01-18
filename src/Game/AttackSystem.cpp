// AttackSystem.cpp
#include "AttackSystem.h"
#include "ZombieSystem.h"
#include "HiveSystem.h"
#include "CameraSystem.h"
#include "../ContestAPI/app.h"

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

    if (s <= smallS) { outDmgMult = smallDmg; outRadMult = smallRad; return; }
    if (s >= bigS) { outDmgMult = bigDmg;   outRadMult = bigRad;   return; }

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

// ------------------- shared tuning -------------------
struct SlashParams { float range = 300.0f; float cosHalfAngle = 0.985f; };
static SlashParams GetSlashParams(float radMult)
{
    SlashParams p;
    p.range = 300.0f * radMult;     // tune slash reach
    p.cosHalfAngle = 0.985f;        // tune slash width
    return p;
}

struct PulseParams { float radius = 200.0f; };
static PulseParams GetPulseParams(float radMult)
{
    PulseParams p;
    p.radius = 200.0f * radMult;     // tune pulse radius
    return p;
}

struct MeteorParams { float targetDist = 260.0f; float radius = 120.0f; };
static MeteorParams GetMeteorParams(float radMult)
{
    MeteorParams p;
    p.targetDist = 260.0f;          // tune how far meteor lands
    p.radius = 120.0f * radMult;    // tune meteor blast radius
    return p;
}

// ------------------- draw helper (circle made of lines) -------------------
static void DrawCircleLinesApprox(float cx, float cy, float r, float cr, float cg, float cb, int segments = 24)
{
    if (segments < 8) segments = 8;
    const float step = 6.28318530718f / (float)segments;

    float prevX = cx + r;
    float prevY = cy;

    for (int i = 1; i <= segments; i++)
    {
        float a = step * (float)i;
        float x = cx + std::cos(a) * r;
        float y = cy + std::sin(a) * r;
        App::DrawLine(prevX, prevY, x, y, cr, cg, cb);
        prevX = x;
        prevY = y;
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

    slashFx = SlashFX{};
    pulseFx = PulseFX{};
    meteorFx = MeteorFX{};
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

    if (slashFx.active)
    {
        slashFx.timeMs += deltaTimeMs;
        if (slashFx.timeMs >= slashFx.durMs) slashFx.active = false;
    }

    if (pulseFx.active)
    {
        pulseFx.timeMs += deltaTimeMs;
        if (pulseFx.timeMs >= pulseFx.durMs) pulseFx.active = false;
    }

    if (meteorFx.active)
    {
        meteorFx.timeMs += deltaTimeMs;
        if (meteorFx.timeMs >= meteorFx.durMs) meteorFx.active = false;
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


static void TriggerFearAOE(float fx, float fy, ZombieSystem& zombies, CameraSystem& camera)
{
   
    const float fearRadius = 350.0f;
    const float fearDurationMs = 1200.0f;

    zombies.TriggerFear(fx, fy, fearRadius, fearDurationMs);
    camera.AddShake(10.0f, 0.12f);
}

void AttackSystem::Process(const AttackInput& in,
    float playerX, float playerY,
    float playerScale,
    ZombieSystem& zombies,
    HiveSystem& hives,
    CameraSystem& camera)
{
    lastPulseKills = 0;
    lastSlashKills = 0;
    lastMeteorKills = 0;

    if (in.pulsePressed && pulseCooldownMs <= 0.0f)
    {
        DoPulse(playerX, playerY, playerScale, zombies, hives, camera);
        pulseCooldownMs = 500.0f;
        App::PlayAudio("./Data/TestData/AOE.mp3", false);

        // start pulse FX (centered on player)
        float dmgDummy = 1.0f, radMult = 1.0f;
        GetAttackMultsFromScale(playerScale, dmgDummy, radMult);
        const PulseParams pp = GetPulseParams(radMult);

        pulseFx.active = true;
        pulseFx.timeMs = 0.0f;
        pulseFx.x = playerX;
        pulseFx.y = playerY;
        pulseFx.radMult = radMult;
        pulseFx.radius = pp.radius;
    }

    if (in.slashPressed && slashCooldownMs <= 0.0f)
    {
        DoSlash(playerX, playerY, playerScale, in.aimX, in.aimY, zombies, hives, camera);
        slashCooldownMs = 200.0f;
        App::PlayAudio("./Data/TestData/slash.mp3", false);

        // start slash FX
        slashFx.active = true;
        slashFx.timeMs = 0.0f;
        slashFx.x = playerX;
        slashFx.y = playerY;

        float ax = in.aimX, ay = in.aimY;
        float len2 = ax * ax + ay * ay;
        if (len2 > 0.0001f)
        {
            float inv = 1.0f / std::sqrt(len2);
            ax *= inv; ay *= inv;
        }
        else { ax = 0.0f; ay = 1.0f; }
        slashFx.ax = ax;
        slashFx.ay = ay;

        float dmgDummy = 1.0f, radMult = 1.0f;
        GetAttackMultsFromScale(playerScale, dmgDummy, radMult);
        slashFx.radMult = radMult;

        const SlashParams sp = GetSlashParams(radMult);
        slashFx.cosHalfAngle = sp.cosHalfAngle;
    }

    if (in.meteorPressed && meteorCooldownMs <= 0.0f)
    {
        DoMeteor(playerX, playerY, playerScale, in.aimX, in.aimY, zombies, hives, camera);
        meteorCooldownMs = 900.0f;
        App::PlayAudio("./Data/TestData/explode.mp3", false);

        // start meteor FX (at landing point)
        float ax = in.aimX, ay = in.aimY;
        float len2 = ax * ax + ay * ay;
        if (len2 > 0.0001f)
        {
            float inv = 1.0f / std::sqrt(len2);
            ax *= inv; ay *= inv;
        }
        else { ax = 0.0f; ay = 1.0f; }

        float dmgDummy = 1.0f, radMult = 1.0f;
        GetAttackMultsFromScale(playerScale, dmgDummy, radMult);

        const MeteorParams mp = GetMeteorParams(radMult);
        const float tx = playerX + ax * mp.targetDist;
        const float ty = playerY + ay * mp.targetDist;

        meteorFx.active = true;
        meteorFx.timeMs = 0.0f;
        meteorFx.x = tx;
        meteorFx.y = ty;
        meteorFx.radMult = radMult;
        meteorFx.radius = mp.radius;
    }
}

void AttackSystem::DoPulse(float px, float py, float playerScale, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    float dmgMult, radMult;
    GetAttackMultsFromScale(playerScale, dmgMult, radMult);

    const PulseParams pp = GetPulseParams(radMult);
    const float radius = pp.radius;
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
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE) eliteKilled = true;
            zombies.KillByPlayer(i);
            killed++;
            continue;
        }
        i++;
    }

    lastPulseKills = killed;

    const float hiveDamage = 20.0f * dmgMult;
    const bool hitHive = hives.DamageHiveAt(px, py, radius, hiveDamage);

    if (killed > 0) camera.AddShake(6.0f, 0.08f);
    if (hitHive)    camera.AddShake(4.0f, 0.05f);

    TriggerFearAOE(px, py, zombies, camera);
    
    //TriggerFearIfEliteKilled(eliteKilled, px, py, zombies, camera);
}

void AttackSystem::DoSlash(float px, float py, float playerScale, float aimX, float aimY,
    ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    float dmgMult, radMult;
    GetAttackMultsFromScale(playerScale, dmgMult, radMult);

    const SlashParams sp = GetSlashParams(radMult);
    const float range = sp.range;
    const float range2 = range * range;
    const float cosHalfAngle = sp.cosHalfAngle;

    float len2 = aimX * aimX + aimY * aimY;
    if (len2 > 0.0001f)
    {
        float inv = 1.0f / std::sqrt(len2);
        aimX *= inv; aimY *= inv;
    }
    else { aimX = 0.0f; aimY = 1.0f; }

    int killed = 0;
    bool eliteKilled = false;

    for (int i = 0; i < zombies.AliveCount(); )
    {
        const float zx = zombies.GetX(i) - px;
        const float zy = zombies.GetY(i) - py;

        const float d2 = zx * zx + zy * zy;
        if (d2 > range2) { i++; continue; }

        const float d = std::sqrt(d2);
        if (d < 0.0001f)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE) eliteKilled = true;
            zombies.KillByPlayer(i);
            killed++;
            continue;
        }

        const float nx = zx / d;
        const float ny = zy / d;

        const float dot = nx * aimX + ny * aimY;
        if (dot >= cosHalfAngle)
        {
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE) eliteKilled = true;
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

    if (killed > 0) camera.AddShake(5.0f, 0.06f);
    if (hitHive)    camera.AddShake(5.0f, 0.06f);

    //TriggerFearIfEliteKilled(eliteKilled, px, py, zombies, camera);
}

void AttackSystem::DoMeteor(float px, float py, float playerScale, float aimX, float aimY,
    ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    float dmgMult, radMult;
    GetAttackMultsFromScale(playerScale, dmgMult, radMult);

    float len2 = aimX * aimX + aimY * aimY;
    if (len2 > 0.0001f)
    {
        float inv = 1.0f / std::sqrt(len2);
        aimX *= inv; aimY *= inv;
    }
    else { aimX = 0.0f; aimY = 1.0f; }

    const MeteorParams mp = GetMeteorParams(radMult);
    const float tx = px + aimX * mp.targetDist;
    const float ty = py + aimY * mp.targetDist;

    const float radius = mp.radius;
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
            if (zombies.GetType(i) == ZombieSystem::PURPLE_ELITE) eliteKilled = true;
            zombies.KillByPlayer(i);
            killed++;
            continue;
        }

        i++;
    }

    lastMeteorKills = killed;

    const float hiveDamage = 50.0f * dmgMult;
    const bool hitHive = hives.DamageHiveAt(tx, ty, radius, hiveDamage);

    if (killed > 0) camera.AddShake(8.0f, 0.10f);
    if (hitHive)    camera.AddShake(9.0f, 0.12f);

    //TriggerFearIfEliteKilled(eliteKilled, tx, ty, zombies, camera);
}

void AttackSystem::RenderFX(float camOffX, float camOffY) const
{
    // SLASH
    if (slashFx.active)
    {
        float t = 1.0f - (slashFx.timeMs / slashFx.durMs);
        t = Clamp01(t);

        const SlashParams sp = GetSlashParams(slashFx.radMult);
        const float range = sp.range;

        float ax = slashFx.ax;
        float ay = slashFx.ay;

        float px = -ay;
        float py = ax;

        float theta = std::acos(slashFx.cosHalfAngle);
        float width = std::tan(theta);

        float e1x = ax + px * width;
        float e1y = ay + py * width;
        float e2x = ax - px * width;
        float e2y = ay - py * width;

        auto Norm = [](float& x, float& y)
            {
                float l2 = x * x + y * y;
                if (l2 > 0.0001f)
                {
                    float inv = 1.0f / std::sqrt(l2);
                    x *= inv; y *= inv;
                }
                else { x = 0.0f; y = 0.0f; }
            };

        Norm(e1x, e1y);
        Norm(e2x, e2y);

        float sx = slashFx.x - camOffX;
        float sy = slashFx.y - camOffY;

        float ex1 = sx + e1x * range;
        float ey1 = sy + e1y * range;
        float ex2 = sx + e2x * range;
        float ey2 = sy + e2y * range;

        float r = 0.35f * t;
        float g = 0.95f * t;
        float b = 1.00f * t;

        App::DrawLine(sx, sy, ex1, ey1, r, g, b);
        App::DrawLine(sx, sy, ex2, ey2, r, g, b);
        App::DrawLine(ex1, ey1, ex2, ey2, r, g, b);
    }

    // PULSE (electric blue ring)
    if (pulseFx.active)
    {
        float t = 1.0f - (pulseFx.timeMs / pulseFx.durMs);
        t = Clamp01(t);

        float sx = pulseFx.x - camOffX;
        float sy = pulseFx.y - camOffY;

        float r = pulseFx.radius;

        // electric blue that fades out
        float cr = 0.20f * t;
        float cg = 0.80f * t;
        float cb = 1.00f * t;

        DrawCircleLinesApprox(sx, sy, r, cr, cg, cb, 28);
        DrawCircleLinesApprox(sx, sy, r * 0.65f, cr, cg, cb, 28);
    }

    // METEOR (fire rings)
    if (meteorFx.active)
    {
        float t = 1.0f - (meteorFx.timeMs / meteorFx.durMs);
        t = Clamp01(t);

        float sx = meteorFx.x - camOffX;
        float sy = meteorFx.y - camOffY;

        float r = meteorFx.radius;

        // fire palette (all fade with t)
        // outer: orange
        DrawCircleLinesApprox(sx, sy, r, 1.00f * t, 0.45f * t, 0.05f * t, 32);
        // mid: yellow
        DrawCircleLinesApprox(sx, sy, r * 0.7f, 1.00f * t, 0.85f * t, 0.10f * t, 32);
        // inner: red
        DrawCircleLinesApprox(sx, sy, r * 0.4f, 1.00f * t, 0.15f * t, 0.02f * t, 32);
    }
}
