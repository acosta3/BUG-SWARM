// AttackSystem.cpp
#include "AttackSystem.h"
#include "ZombieSystem.h"
#include "HiveSystem.h"
#include "CameraSystem.h"
#include "GameConfig.h"
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

// Scale-based damage/radius multipliers using centralized config
static void GetAttackMultsFromScale(float s, float& outDmgMult, float& outRadMult)
{
    if (s <= GameConfig::AttackConfig::SMALL_SCALE)
    {
        outDmgMult = GameConfig::AttackConfig::SMALL_DMG_MULT;
        outRadMult = GameConfig::AttackConfig::SMALL_RAD_MULT;
        return;
    }
    if (s >= GameConfig::AttackConfig::BIG_SCALE)
    {
        outDmgMult = GameConfig::AttackConfig::BIG_DMG_MULT;
        outRadMult = GameConfig::AttackConfig::BIG_RAD_MULT;
        return;
    }

    if (s < 1.0f)
    {
        float t = (s - GameConfig::AttackConfig::SMALL_SCALE) / (1.0f - GameConfig::AttackConfig::SMALL_SCALE);
        t = Clamp01(t);
        outDmgMult = Lerp(GameConfig::AttackConfig::SMALL_DMG_MULT, GameConfig::AttackConfig::NORMAL_DMG_MULT, t);
        outRadMult = Lerp(GameConfig::AttackConfig::SMALL_RAD_MULT, GameConfig::AttackConfig::NORMAL_RAD_MULT, t);
    }
    else
    {
        float t = (s - 1.0f) / (GameConfig::AttackConfig::BIG_SCALE - 1.0f);
        t = Clamp01(t);
        outDmgMult = Lerp(GameConfig::AttackConfig::NORMAL_DMG_MULT, GameConfig::AttackConfig::BIG_DMG_MULT, t);
        outRadMult = Lerp(GameConfig::AttackConfig::NORMAL_RAD_MULT, GameConfig::AttackConfig::BIG_RAD_MULT, t);
    }
}

// ------------------- shared tuning -------------------
struct SlashParams { float range; float cosHalfAngle; };
static SlashParams GetSlashParams(float radMult)
{
    SlashParams p;
    p.range = GameConfig::AttackConfig::SLASH_BASE_RANGE * radMult;
    p.cosHalfAngle = GameConfig::AttackConfig::SLASH_COS_HALF_ANGLE;
    return p;
}

struct PulseParams { float radius; };
static PulseParams GetPulseParams(float radMult)
{
    PulseParams p;
    p.radius = GameConfig::AttackConfig::PULSE_BASE_RADIUS * radMult;
    return p;
}

struct MeteorParams { float targetDist; float radius; };
static MeteorParams GetMeteorParams(float radMult)
{
    MeteorParams p;
    p.targetDist = GameConfig::AttackConfig::METEOR_TARGET_DIST;
    p.radius = GameConfig::AttackConfig::METEOR_BASE_RADIUS * radMult;
    return p;
}

// ------------------- draw helper (circle made of lines) -------------------
static void DrawCircleLinesApprox(float cx, float cy, float r, float cr, float cg, float cb, int segments = GameConfig::AttackConfig::CIRCLE_SEGMENTS_LOW)
{
    if (segments < 8) segments = 8;
    const float step = GameConfig::AttackConfig::TWO_PI / (float)segments;

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

    // ✅ OPTIMIZED: Use ForEach instead of manual loop checking all slots
    slashFxPool.ForEach([deltaTimeMs, this](SlashFX* fx) {
        if (fx && fx->active)
        {
            fx->timeMs += deltaTimeMs;
            if (fx->timeMs >= fx->durMs)
            {
                fx->active = false;
                slashFxPool.Release(fx);
            }
        }
        });

    pulseFxPool.ForEach([deltaTimeMs, this](PulseFX* fx) {
        if (fx && fx->active)
        {
            fx->timeMs += deltaTimeMs;
            if (fx->timeMs >= fx->durMs)
            {
                fx->active = false;
                pulseFxPool.Release(fx);
            }
        }
        });

    meteorFxPool.ForEach([deltaTimeMs, this](MeteorFX* fx) {
        if (fx && fx->active)
        {
            fx->timeMs += deltaTimeMs;
            if (fx->timeMs >= fx->durMs)
            {
                fx->active = false;
                meteorFxPool.Release(fx);
            }
        }
        });

    // ✅ NEW: Update hive explosions
    hiveExplosionPool.ForEach([deltaTimeMs, this](HiveExplosionFX* fx) {
        if (fx && fx->active)
        {
            fx->timeMs += deltaTimeMs;
            if (fx->timeMs >= fx->durMs)
            {
                fx->active = false;
                hiveExplosionPool.Release(fx);
            }
        }
        });
}

static void TriggerFearIfEliteKilled(bool eliteKilled, float fx, float fy, ZombieSystem& zombies, CameraSystem& camera)
{
    if (!eliteKilled) return;

    zombies.TriggerFear(fx, fy, GameConfig::AttackConfig::ELITE_FEAR_RADIUS, GameConfig::AttackConfig::ELITE_FEAR_DURATION_MS);
    camera.AddShake(GameConfig::AttackConfig::ELITE_FEAR_SHAKE_STRENGTH, GameConfig::AttackConfig::ELITE_FEAR_SHAKE_DURATION);
}

static void TriggerFearAOE(float fx, float fy, ZombieSystem& zombies, CameraSystem& camera)
{
    zombies.TriggerFear(fx, fy, GameConfig::AttackConfig::AOE_FEAR_RADIUS, GameConfig::AttackConfig::AOE_FEAR_DURATION_MS);
    camera.AddShake(GameConfig::AttackConfig::AOE_FEAR_SHAKE_STRENGTH, GameConfig::AttackConfig::AOE_FEAR_SHAKE_DURATION);
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
        pulseCooldownMs = GameConfig::AttackConfig::PULSE_COOLDOWN_MS;
        App::PlayAudio(GameConfig::AttackConfig::PULSE_SOUND, false);

        PulseFX* fx = pulseFxPool.Acquire();
        if (fx)
        {
            float dmgDummy = 1.0f, radMult = 1.0f;
            GetAttackMultsFromScale(playerScale, dmgDummy, radMult);
            const PulseParams pp = GetPulseParams(radMult);

            fx->active = true;
            fx->timeMs = 0.0f;
            fx->durMs = GameConfig::AttackConfig::PULSE_FX_DURATION_MS;
            fx->x = playerX;
            fx->y = playerY;
            fx->radMult = radMult;
            fx->radius = pp.radius;
        }
    }

    if (in.slashPressed && slashCooldownMs <= 0.0f)
    {
        DoSlash(playerX, playerY, playerScale, in.aimX, in.aimY, zombies, hives, camera);
        slashCooldownMs = GameConfig::AttackConfig::SLASH_COOLDOWN_MS;
        App::PlayAudio(GameConfig::AttackConfig::SLASH_SOUND, false);

        SlashFX* fx = slashFxPool.Acquire();
        if (fx)
        {
            fx->active = true;
            fx->timeMs = 0.0f;
            fx->durMs = GameConfig::AttackConfig::SLASH_FX_DURATION_MS;
            fx->x = playerX;
            fx->y = playerY;

            float ax = in.aimX, ay = in.aimY;
            float len2 = ax * ax + ay * ay;
            if (len2 > GameConfig::AttackConfig::EPSILON)
            {
                float inv = 1.0f / std::sqrt(len2);
                ax *= inv; ay *= inv;
            }
            else { ax = 0.0f; ay = 1.0f; }
            fx->ax = ax;
            fx->ay = ay;

            float dmgDummy = 1.0f, radMult = 1.0f;
            GetAttackMultsFromScale(playerScale, dmgDummy, radMult);
            fx->radMult = radMult;

            const SlashParams sp = GetSlashParams(radMult);
            fx->cosHalfAngle = sp.cosHalfAngle;
        }
    }

    if (in.meteorPressed && meteorCooldownMs <= 0.0f)
    {
        DoMeteor(playerX, playerY, playerScale, in.aimX, in.aimY, zombies, hives, camera);
        meteorCooldownMs = GameConfig::AttackConfig::METEOR_COOLDOWN_MS;
        App::PlayAudio(GameConfig::AttackConfig::METEOR_SOUND, false);

        MeteorFX* fx = meteorFxPool.Acquire();
        if (fx)
        {
            float ax = in.aimX, ay = in.aimY;
            float len2 = ax * ax + ay * ay;
            if (len2 > GameConfig::AttackConfig::EPSILON)
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

            fx->active = true;
            fx->timeMs = 0.0f;
            fx->durMs = GameConfig::AttackConfig::METEOR_FX_DURATION_MS;
            fx->x = tx;
            fx->y = ty;
            fx->radMult = radMult;
            fx->radius = mp.radius;
        }
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

    const float hiveDamage = GameConfig::AttackConfig::PULSE_HIVE_DAMAGE * dmgMult;
    const bool hitHive = hives.DamageHiveAt(px, py, radius, hiveDamage, this, &camera);

    if (killed > 0) camera.AddShake(GameConfig::AttackConfig::PULSE_SHAKE_STRENGTH, GameConfig::AttackConfig::PULSE_SHAKE_DURATION);
    if (hitHive)    camera.AddShake(GameConfig::AttackConfig::PULSE_HIVE_SHAKE_STRENGTH, GameConfig::AttackConfig::PULSE_HIVE_SHAKE_DURATION);

    TriggerFearAOE(px, py, zombies, camera);
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
    if (len2 > GameConfig::AttackConfig::EPSILON)
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
        if (d < GameConfig::AttackConfig::EPSILON)
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

    const float slashCenterDist = GameConfig::AttackConfig::SLASH_CENTER_DIST * radMult;
    const float hx = px + aimX * slashCenterDist;
    const float hy = py + aimY * slashCenterDist;
    const float slashHitRadius = GameConfig::AttackConfig::SLASH_HIT_RADIUS * radMult;

    const float hiveDamage = GameConfig::AttackConfig::SLASH_HIVE_DAMAGE * dmgMult;
    const bool hitHive = hives.DamageHiveAt(hx, hy, slashHitRadius, hiveDamage, this, &camera);

    if (killed > 0) camera.AddShake(GameConfig::AttackConfig::SLASH_SHAKE_STRENGTH, GameConfig::AttackConfig::SLASH_SHAKE_DURATION);
    if (hitHive)    camera.AddShake(GameConfig::AttackConfig::SLASH_SHAKE_STRENGTH, GameConfig::AttackConfig::SLASH_SHAKE_DURATION);
}

void AttackSystem::DoMeteor(float px, float py, float playerScale, float aimX, float aimY,
    ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera)
{
    float dmgMult, radMult;
    GetAttackMultsFromScale(playerScale, dmgMult, radMult);

    float len2 = aimX * aimX + aimY * aimY;
    if (len2 > GameConfig::AttackConfig::EPSILON)
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

    const float hiveDamage = GameConfig::AttackConfig::METEOR_HIVE_DAMAGE * dmgMult;
    const bool hitHive = hives.DamageHiveAt(tx, ty, radius, hiveDamage, this, &camera);

    if (killed > 0) camera.AddShake(GameConfig::AttackConfig::METEOR_SHAKE_STRENGTH, GameConfig::AttackConfig::METEOR_SHAKE_DURATION);
    if (hitHive)    camera.AddShake(GameConfig::AttackConfig::METEOR_HIVE_SHAKE_STRENGTH, GameConfig::AttackConfig::METEOR_HIVE_SHAKE_DURATION);
}

void AttackSystem::TriggerHiveExplosion(float x, float y, float hiveRadius, CameraSystem& camera)
{
    HiveExplosionFX* fx = hiveExplosionPool.Acquire();
    if (fx)
    {
        fx->active = true;
        fx->timeMs = 0.0f;
        fx->durMs = GameConfig::HiveConfig::EXPLOSION_DURATION_MS;
        fx->x = x;
        fx->y = y;
        fx->baseRadius = hiveRadius;
    }

    // Big camera shake for hive destruction
    camera.AddShake(15.0f, 0.3f);

    // Play explosion sound
    App::PlayAudio(GameConfig::AttackConfig::METEOR_SOUND, false);
}

void AttackSystem::RenderFX(float camOffX, float camOffY) const
{
    // ✅ OPTIMIZED: Use ForEach for rendering
    slashFxPool.ForEach([&](const SlashFX* fx) {
        if (!fx->active) return;

        float t = 1.0f - (fx->timeMs / fx->durMs);
        t = Clamp01(t);

        const SlashParams sp = GetSlashParams(fx->radMult);
        const float range = sp.range;

        float ax = fx->ax;
        float ay = fx->ay;

        float px = -ay;
        float py = ax;

        float theta = std::acos(fx->cosHalfAngle);
        float width = std::tan(theta);

        float e1x = ax + px * width;
        float e1y = ay + py * width;
        float e2x = ax - px * width;
        float e2y = ay - py * width;

        auto Norm = [](float& x, float& y)
            {
                float l2 = x * x + y * y;
                if (l2 > GameConfig::AttackConfig::EPSILON)
                {
                    float inv = 1.0f / std::sqrt(l2);
                    x *= inv; y *= inv;
                }
                else { x = 0.0f; y = 0.0f; }
            };

        Norm(e1x, e1y);
        Norm(e2x, e2y);

        float sx = fx->x - camOffX;
        float sy = fx->y - camOffY;

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
        });

    pulseFxPool.ForEach([&](const PulseFX* fx) {
        if (!fx->active) return;

        float t = 1.0f - (fx->timeMs / fx->durMs);
        t = Clamp01(t);

        float sx = fx->x - camOffX;
        float sy = fx->y - camOffY;

        float r = fx->radius;

        float cr = 0.20f * t;
        float cg = 0.80f * t;
        float cb = 1.00f * t;

        DrawCircleLinesApprox(sx, sy, r, cr, cg, cb, GameConfig::AttackConfig::CIRCLE_SEGMENTS_MED);
        DrawCircleLinesApprox(sx, sy, r * GameConfig::AttackConfig::PULSE_INNER_RADIUS_MULT, cr, cg, cb, GameConfig::AttackConfig::CIRCLE_SEGMENTS_MED);
        });

    meteorFxPool.ForEach([&](const MeteorFX* fx) {
        if (!fx->active) return;

        float t = 1.0f - (fx->timeMs / fx->durMs);
        t = Clamp01(t);

        float sx = fx->x - camOffX;
        float sy = fx->y - camOffY;

        float r = fx->radius;

        DrawCircleLinesApprox(sx, sy, r, 1.00f * t, 0.45f * t, 0.05f * t, GameConfig::AttackConfig::CIRCLE_SEGMENTS_HIGH);
        DrawCircleLinesApprox(sx, sy, r * GameConfig::AttackConfig::METEOR_MID_RADIUS_MULT, 1.00f * t, 0.85f * t, 0.10f * t, GameConfig::AttackConfig::CIRCLE_SEGMENTS_HIGH);
        DrawCircleLinesApprox(sx, sy, r * GameConfig::AttackConfig::METEOR_INNER_RADIUS_MULT, 1.00f * t, 0.15f * t, 0.02f * t, GameConfig::AttackConfig::CIRCLE_SEGMENTS_HIGH);
        });

    // ✅ NEW: Render hive explosions
    hiveExplosionPool.ForEach([&](const HiveExplosionFX* fx) {
        if (!fx->active) return;

        const float t = fx->timeMs / fx->durMs;
        const float fadeIn = min(t * 4.0f, 1.0f);
        const float fadeOut = 1.0f - t;
        const float alpha = fadeIn * fadeOut;

        const float sx = fx->x - camOffX;
        const float sy = fx->y - camOffY;

        const float maxRadius = fx->baseRadius * GameConfig::HiveConfig::EXPLOSION_MAX_RADIUS_MULT;

        // Draw expanding explosion rings
        const int ringCount = static_cast<int>(GameConfig::HiveConfig::EXPLOSION_RINGS);
        for (int i = 0; i < ringCount; i++)
        {
            const float ringT = t + (float)i / (float)ringCount * 0.3f;
            if (ringT > 1.0f) continue;

            const float radius = maxRadius * ringT;
            const float ringAlpha = alpha * (1.0f - ringT);

            const float r = GameConfig::HiveConfig::EXPLOSION_R * ringAlpha;
            const float g = GameConfig::HiveConfig::EXPLOSION_G * ringAlpha;
            const float b = GameConfig::HiveConfig::EXPLOSION_B * ringAlpha;

            DrawCircleLinesApprox(sx, sy, radius, r, g, b, GameConfig::AttackConfig::CIRCLE_SEGMENTS_HIGH);
        }

        // Draw debris particles flying outward
        const int debrisCount = static_cast<int>(GameConfig::HiveConfig::DEBRIS_COUNT);
        for (int i = 0; i < debrisCount; i++)
        {
            const float angle = (GameConfig::HiveConfig::TWO_PI / (float)debrisCount) * (float)i;
            const float speed = maxRadius * 1.5f * t;
            const float dx = std::cos(angle) * speed;
            const float dy = std::sin(angle) * speed;

            const float debrisAlpha = alpha * 0.8f;
            const float r = GameConfig::HiveConfig::METEOR_R * debrisAlpha;
            const float g = GameConfig::HiveConfig::METEOR_G * debrisAlpha;
            const float b = GameConfig::HiveConfig::METEOR_B * debrisAlpha;

            const float size = 4.0f * (1.0f - t);
            App::DrawLine(sx + dx - size, sy + dy - size,
                sx + dx + size, sy + dy + size, r, g, b);
            App::DrawLine(sx + dx + size, sy + dy - size,
                sx + dx - size, sy + dy + size, r, g, b);
        }

        // Draw central flash
        if (t < 0.2f)
        {
            const float flashT = t / 0.2f;
            const float flashAlpha = (1.0f - flashT) * 1.5f;
            const float flashRadius = fx->baseRadius * (1.0f + flashT * 2.0f);

            DrawCircleLinesApprox(sx, sy, flashRadius,
                1.0f * flashAlpha, 0.9f * flashAlpha, 0.8f * flashAlpha,
                GameConfig::AttackConfig::CIRCLE_SEGMENTS_MED);
        }
        });
}