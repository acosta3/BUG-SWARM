// AttackSystem.h
#pragma once

class ZombieSystem;
class CameraSystem;
class HiveSystem;

struct AttackInput
{
    bool pulsePressed = false;
    bool slashPressed = false;
    bool meteorPressed = false;

    float aimX = 0.0f;
    float aimY = 1.0f;
};

struct SlashFX
{
    bool  active = false;
    float timeMs = 0.0f;
    float durMs = 80.0f;

    float x = 0.0f, y = 0.0f;
    float ax = 0.0f, ay = 1.0f;

    float radMult = 1.0f;
    float cosHalfAngle = 0.707f; // matches gameplay
};

struct PulseFX
{
    bool  active = false;
    float timeMs = 0.0f;
    float durMs = 140.0f;

    float x = 0.0f, y = 0.0f;
    float radMult = 1.0f;

    float radius = 80.0f; // matches gameplay (already scaled by radMult in cpp)
};

struct MeteorFX
{
    bool  active = false;
    float timeMs = 0.0f;
    float durMs = 220.0f;

    float x = 0.0f, y = 0.0f;     // landing point
    float radMult = 1.0f;

    float radius = 120.0f;        // matches gameplay (already scaled by radMult in cpp)
};

class AttackSystem
{
public:
    void Init();
    void Update(float deltaTimeMs);

    void Process(const AttackInput& in,
        float playerX, float playerY,
        float playerScale,
        ZombieSystem& zombies,
        HiveSystem& hives,
        CameraSystem& camera);

    float GetPulseCooldownMs()  const { return pulseCooldownMs; }
    float GetSlashCooldownMs()  const { return slashCooldownMs; }
    float GetMeteorCooldownMs() const { return meteorCooldownMs; }

    int GetLastPulseKills()  const { return lastPulseKills; }
    int GetLastSlashKills()  const { return lastSlashKills; }
    int GetLastMeteorKills() const { return lastMeteorKills; }

    // Draw all attack FX (slash wedge + pulse ring + meteor ring)
    void RenderFX(float camOffX, float camOffY) const;

private:
    void TickCooldown(float& cd, float dtMs);

    void DoPulse(float px, float py, float playerScale, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);
    void DoSlash(float px, float py, float playerScale, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);
    void DoMeteor(float px, float py, float playerScale, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);

private:
    float pulseCooldownMs = 0.0f;
    float slashCooldownMs = 0.0f;
    float meteorCooldownMs = 0.0f;

    int lastPulseKills = 0;
    int lastSlashKills = 0;
    int lastMeteorKills = 0;

    SlashFX slashFx;
    PulseFX pulseFx;
    MeteorFX meteorFx;
};
