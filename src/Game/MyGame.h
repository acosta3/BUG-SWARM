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
    static constexpr int kMaxZombies = 50'000;

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

    // NEW: store dt so RenderFrame can animate popups
    float lastDtMs = 16.0f;
};
