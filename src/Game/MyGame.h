#pragma once

#include "Player.h"
#include "Input.h"
#include "CameraSystem.h"
#include "ZombieSystem.h"
#include "AttackSystem.h"
#include "NavGrid.h"
#include "HiveSystem.h"

// New
#include "WorldRenderer.h"

class MyGame
{
public:
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

    // New: all drawing lives here now
    WorldRenderer renderer;

    bool densityView = false;

    float lastAimX = 0.0f;
    float lastAimY = 1.0f;
    int lastTargetCell = -1;
};
