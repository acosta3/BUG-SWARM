// AttackSystem.h
#pragma once

class ZombieSystem;
class CameraSystem;
class HiveSystem;

struct AttackInput
{
    bool pulsePressed = false;       // Space / B
    bool slashPressed = false;       // Left Shift / X
    bool meteorPressed = false;      // M / Y

    // Aim direction in world space (normalized if possible)
    float aimX = 0.0f;
    float aimY = 1.0f;
};

class AttackSystem
{
public:
    void Init();

    // Call once per frame
    void Update(float deltaTimeMs);

    // Pass playerScale in
    void Process(const AttackInput& in,
        float playerX, float playerY,
        float playerScale,
        ZombieSystem& zombies,
        HiveSystem& hives,
        CameraSystem& camera);

    // Debug UI
    float GetPulseCooldownMs() const { return pulseCooldownMs; }
    float GetSlashCooldownMs() const { return slashCooldownMs; }
    float GetMeteorCooldownMs() const { return meteorCooldownMs; }

    // Optional: expose last move kills per attack type (nice for UI/debug)
    int GetLastPulseKills() const { return lastPulseKills; }
    int GetLastSlashKills() const { return lastSlashKills; }
    int GetLastMeteorKills() const { return lastMeteorKills; }

private:
    float pulseCooldownMs = 0.0f;
    float slashCooldownMs = 0.0f;
    float meteorCooldownMs = 0.0f;

private:
    void TickCooldown(float& cd, float dtMs);

    void DoPulse(float px, float py, float playerScale, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);
    void DoSlash(float px, float py, float playerScale, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);
    void DoMeteor(float px, float py, float playerScale, float aimX, float aimY, ZombieSystem& zombies, HiveSystem& hives, CameraSystem& camera);

private:
    int lastPulseKills = 0;
    int lastSlashKills = 0;
    int lastMeteorKills = 0;
};
