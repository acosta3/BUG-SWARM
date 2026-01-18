// MyGame.h
#pragma once

#include "Player.h"
#include "Input.h"
#include "CameraSystem.h"
#include "ZombieSystem.h"
#include "AttackSystem.h"
#include "NavGrid.h"
#include "HiveSystem.h"
#include "WorldRenderer.h"

class MyGame
{
public:
    static constexpr int kMaxZombies = 10'000;

    void Init();
    void Update(float deltaTimeMs);
    void Render();
    void Shutdown();

private:
    // Init
    void InitWorld();
    void InitObstacles();
    void InitSystems();

    // Update
    void UpdateInput(float deltaTimeMs);
    void UpdatePlayer(float deltaTimeMs);
    void UpdateAttacks(float deltaTimeMs);
    void UpdateNavFlowField(float playerX, float playerY);
    void UpdateCamera(float deltaTimeMs, float playerX, float playerY);
    void UpdateZombies(float deltaTimeMs, float playerX, float playerY);

    AttackInput BuildAttackInput(const InputState& in);

    // Death / respawn flow
    enum class LifeState
    {
        Playing,
        DeathPause,
        RespawnGrace
    };

    void BeginDeath(float playerX, float playerY);
    void RespawnNow();
    bool InputLocked() const;

private:
    InputSystem input;
    Player player;
    CameraSystem camera;
    ZombieSystem zombies;
    AttackSystem attacks;
    NavGrid nav;
    HiveSystem hives;
    WorldRenderer renderer;

    bool densityView = false;

    float lastAimX = 0.0f;
    float lastAimY = 1.0f;
    int lastTargetCell = -1;

    float lastDtMs = 16.0f;

    // Respawn location (same level)
    float respawnX = 400.0f;
    float respawnY = 400.0f;

    // Life-state machine timers
    LifeState life = LifeState::Playing;
    float lifeTimerMs = 0.0f;

    // Tuning knobs
    float deathPauseMs = 900.0f;      // how long you stay dead with frozen input
    float respawnGraceMs = 650.0f;    // how long input stays frozen after respawn
};
