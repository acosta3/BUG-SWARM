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
#include "GameConfig.h"
#include "DifficultyManager.h"
#include <vector>

class MyGame
{
public:

    void Init();
    void Update(float deltaTimeMs);
    void Render();
    void Shutdown();

    DifficultyLevel GetSelectedDifficulty() const { return selectedDifficulty; }
    const Player& GetPlayer() const { return player; }
    const ZombieSystem& GetZombies() const { return zombies; }
    const HiveSystem& GetHives() const { return hives; }

private:
    int GetMaxZombiesForDifficulty() const;

    void BeginWin();
    void ResetRun();

    // Init
    void InitWorld();
    void InitObstacles();
    void InitSystems();

    // Update helpers - original methods
    void UpdateInput(float deltaTimeMs);
    void UpdatePlayer(float deltaTimeMs);
    void UpdateAttacks(float deltaTimeMs);
    void UpdateNavFlowField(float playerX, float playerY);
    void UpdateCamera(float deltaTimeMs, float playerX, float playerY);
    void UpdateZombies(float deltaTimeMs, float playerX, float playerY);

    // Optimized update methods
    void UpdateAttacksOptimized(float dtMs, float playerX, float playerY);
    void UpdateNavFlowFieldOptimized(float playerX, float playerY);
    void UpdateZombiesOptimized(float dtMs, float playerX, float playerY);
    void SpawnZombiesOptimized(float playerX, float playerY);

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

    // Menu / Pause game mode
    enum class GameMode
    {
        Menu,
        Playing,
        Paused,
        Win
    };

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

    // Tuning knobs (kept for compatibility, but using config values)
    float deathPauseMs = GameConfig::GameTuning::DEATH_PAUSE_MS;
    float respawnGraceMs = GameConfig::GameTuning::RESPAWN_GRACE_MS;

    // Game mode
    GameMode mode = GameMode::Menu;

    // Difficulty selection
    DifficultyLevel selectedDifficulty = DifficultyLevel::Easy;

    // Performance optimization caches
    mutable std::vector<int> tempCalculationBuffer;
    mutable AttackInput cachedAttackInput;

    // Cached state for optimization
    float lastPlayerPosX = 0.0f;
    float lastPlayerPosY = 0.0f;
    float lastUpdateTime = 0.0f;
};