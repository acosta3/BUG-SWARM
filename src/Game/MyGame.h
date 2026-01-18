// MyGame.h
#pragma once

#include "AttackSystem.h"
#include "CameraSystem.h"
#include "HiveSystem.h"
#include "Input.h"
#include "NavGrid.h"
#include "Player.h"
#include "WorldRenderer.h"
#include "ZombieSystem.h"

class MyGame
{
public:
    static constexpr int kMaxZombies = 50'000;

    void Init();
    void Update(float dtMs);
    void Render();
    void Shutdown();

private:
    // Init
    void InitWorld();
    void InitObstacles();
    void InitSystems();

    // Update
    void UpdateInput(float dtMs);
    void UpdatePlayer(float dtMs);
    void UpdateAttacks(float dtMs);
    void UpdateNavFlowField(float playerX, float playerY);
    void UpdateCamera(float dtMs, float playerX, float playerY);
    void UpdateZombies(float dtMs, float playerX, float playerY);

    AttackInput BuildAttackInput(const InputState& in);

    // Life state
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
    InputSystem   input;
    Player        player;
    CameraSystem  camera;
    ZombieSystem  zombies;
    AttackSystem  attacks;
    NavGrid       nav;
    HiveSystem    hives;
    WorldRenderer renderer;

    bool  densityView = false;

    float lastAimX = 0.0f;
    float lastAimY = 1.0f;
    int   lastTargetCell = -1;

    float lastDtMs = 16.0f;

    // Respawn location
    float respawnX = 400.0f;
    float respawnY = 400.0f;

    // Life-state machine
    LifeState life = LifeState::Playing;
    float     lifeTimerMs = 0.0f;

    // Tuning
    float deathPauseMs = 900.0f;
    float respawnGraceMs = 650.0f;
};
