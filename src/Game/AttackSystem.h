#pragma once
#include <cstdint>

class ZombieSystem;
class CameraSystem;

struct AttackInput
{
    bool pulsePressed = false;       // Space / B
    bool slashPressed = false;       // e.g. Left Shift / X
    bool meteorPressed = false;      // e.g. M / Y

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

    // Fire whichever attacks are pressed this frame
    void Process(const AttackInput& in,
        float playerX, float playerY,
        ZombieSystem& zombies,
        CameraSystem& camera);

    // Debug UI
    float GetPulseCooldownMs() const { return pulseCooldownMs; }
    float GetSlashCooldownMs() const { return slashCooldownMs; }
    float GetMeteorCooldownMs() const { return meteorCooldownMs; }

private:
    // Cooldowns
    float pulseCooldownMs = 0.0f;
    float slashCooldownMs = 0.0f;
    float meteorCooldownMs = 0.0f;

private:
    void TickCooldown(float& cd, float dtMs);

    void DoPulse(float playerX, float playerY, ZombieSystem& zombies, CameraSystem& camera);
    void DoSlash(float playerX, float playerY, float aimX, float aimY, ZombieSystem& zombies, CameraSystem& camera);
    void DoMeteor(float playerX, float playerY, float aimX, float aimY, ZombieSystem& zombies, CameraSystem& camera);
};
