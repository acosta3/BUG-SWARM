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

    void BeginWin();
    void ResetRun();              // resets gameplay state (hives/zombies/player/attacks)
    void RenderWinOverlay() const;



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

    // NEW: Menu / Pause game mode
    enum class GameMode
    {
        Menu,
        Playing,
        Paused,
        Win
    };

    void RenderMenu() const;
    void RenderPauseOverlay() const;

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
    float deathPauseMs = 900.0f;
    float respawnGraceMs = 650.0f;

    // NEW
    GameMode mode = GameMode::Menu;
};
