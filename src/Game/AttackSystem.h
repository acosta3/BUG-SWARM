#pragma once
#include "ObjectPool.h"

struct AttackInput
{
    bool pulsePressed = false;
    bool slashPressed = false;
    bool meteorPressed = false;
    float aimX = 0.0f;
    float aimY = 0.0f;
};

// VFX structs now pooled
struct SlashFX
{
    bool active = false;
    float timeMs = 0.0f;
    float durMs = 80.0f;
    float x = 0.0f;
    float y = 0.0f;
    float ax = 0.0f;
    float ay = 0.0f;
    float radMult = 1.0f;
    float cosHalfAngle = 0.985f;
};

struct PulseFX
{
    bool active = false;
    float timeMs = 0.0f;
    float durMs = 140.0f;
    float x = 0.0f;
    float y = 0.0f;
    float radMult = 1.0f;
    float radius = 200.0f;
};

struct MeteorFX
{
    bool active = false;
    float timeMs = 0.0f;
    float durMs = 220.0f;
    float x = 0.0f;
    float y = 0.0f;
    float radMult = 1.0f;
    float radius = 120.0f;
};

// ✅ NEW: Hive Explosion FX
struct HiveExplosionFX
{
    bool active = false;
    float timeMs = 0.0f;
    float durMs = 800.0f;
    float x = 0.0f;
    float y = 0.0f;
    float baseRadius = 30.0f;
};

class ZombieSystem;
class HiveSystem;
class CameraSystem;

class AttackSystem
{
public:
    void Init();
    void Update(float deltaTimeMs);

    void Process(
        const AttackInput& in,
        float playerX, float playerY,
        float playerScale,
        ZombieSystem& zombies,
        HiveSystem& hives,
        CameraSystem& camera);

    void RenderFX(float camOffX, float camOffY) const;

    // ✅ NEW: Trigger hive explosion
    void TriggerHiveExplosion(float x, float y, float hiveRadius, CameraSystem& camera);

    float GetPulseCooldownMs() const { return pulseCooldownMs; }
    float GetSlashCooldownMs() const { return slashCooldownMs; }
    float GetMeteorCooldownMs() const { return meteorCooldownMs; }

    int GetLastPulseKills() const { return lastPulseKills; }
    int GetLastSlashKills() const { return lastSlashKills; }
    int GetLastMeteorKills() const { return lastMeteorKills; }

private:
    void DoPulse(float px, float py, float playerScale, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);
    void DoSlash(float px, float py, float playerScale, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);
    void DoMeteor(float px, float py, float playerScale, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);

    void TickCooldown(float& cd, float dtMs);

    float pulseCooldownMs = 0.0f;
    float slashCooldownMs = 0.0f;
    float meteorCooldownMs = 0.0f;

    int lastPulseKills = 0;
    int lastSlashKills = 0;
    int lastMeteorKills = 0;

    // OBJECT POOLS - AAA style!
    ObjectPool<SlashFX, 16> slashFxPool;
    ObjectPool<PulseFX, 16> pulseFxPool;
    ObjectPool<MeteorFX, 16> meteorFxPool;
    ObjectPool<HiveExplosionFX, 8> hiveExplosionPool; // ✅ NEW: Max 8 hive explosions
};