// AttackSystem.h - AAA Quality Version
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
    bool active = false;
    float timeMs = 0.0f;
    float durMs = 80.0f;

    float x = 0.0f;
    float y = 0.0f;
    float ax = 0.0f;
    float ay = 1.0f;

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

class AttackSystem
{
public:
    // Initialization
    void Init();

    // Updates
    void Update(float deltaTimeMs);

    // Process attacks
    void Process(const AttackInput& in,
        float playerX, float playerY,
        float playerScale,
        ZombieSystem& zombies,
        HiveSystem& hives,
        CameraSystem& camera);

    // Rendering
    void RenderFX(float camOffX, float camOffY) const;

    // Cooldown queries
    float GetPulseCooldownMs() const { return pulseCooldownMs; }
    float GetSlashCooldownMs() const { return slashCooldownMs; }
    float GetMeteorCooldownMs() const { return meteorCooldownMs; }

    // Kill tracking
    int GetLastPulseKills() const { return lastPulseKills; }
    int GetLastSlashKills() const { return lastSlashKills; }
    int GetLastMeteorKills() const { return lastMeteorKills; }

private:
    void TickCooldown(float& cd, float dtMs);

    void DoPulse(float px, float py, float playerScale,
        ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);
    void DoSlash(float px, float py, float playerScale, float aimX, float aimY,
        ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);
    void DoMeteor(float px, float py, float playerScale, float aimX, float aimY,
        ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);

private:
    // Cooldown timers
    float pulseCooldownMs = 0.0f;
    float slashCooldownMs = 0.0f;
    float meteorCooldownMs = 0.0f;

    // Kill tracking
    int lastPulseKills = 0;
    int lastSlashKills = 0;
    int lastMeteorKills = 0;

    // Visual effects
    SlashFX slashFx;
    PulseFX pulseFx;
    MeteorFX meteorFx;
};