#pragma once

#include "Player.h"
#include "Input.h"
#include "CameraSystem.h"
#include "ZombieSystem.h"
#include "AttackSystem.h"
#include "NavGrid.h"

class MyGame
{
public:
    void Init();
    void Update(float deltaTimeMs);
    void Render();
    void Shutdown();

    static void DrawZombieTri(float x, float y, float size, float r, float g, float b);
    static float Clamp01(float v);

private:
    void InitWorld();
    void InitObstacles();
    void InitSystems();

    void UpdateInput(float deltaTimeMs);
    void UpdatePlayer(float deltaTimeMs);
    void UpdateAttacks(float deltaTimeMs);
    void UpdateNavFlowField(float playerX, float playerY);
    void UpdateCamera(float deltaTimeMs, float playerX, float playerY);
    void UpdateZombies(float deltaTimeMs, float playerX, float playerY);

    void RenderWorld(float offX, float offY);
    void RenderZombies(float offX, float offY);
    void RenderUI(int simCount, int drawnCount, int step);

    

    AttackInput BuildAttackInput(const InputState& in);

private:
    InputSystem input;
    Player player;
    CameraSystem camera;
    ZombieSystem zombies;
    AttackSystem attacks;
    NavGrid nav;

    bool densityView = false;

    // Persistent per game state (no statics inside Update)
    float lastAimX = 0.0f;
    float lastAimY = 1.0f;
    int lastTargetCell = -1;
};
