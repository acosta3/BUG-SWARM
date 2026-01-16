#pragma once
#include "Player.h"
#include "Input.h"
#include "CameraSystem.h"
#include "ZombieSystem.h"

class MyGame
{
public:
    void Init();

    void Update(float deltaTime);

    void Render();

    void Shutdown();

    static void DrawZombieTri(float x, float y, float size,
        float r, float g, float b);

private:
    InputSystem input;

    Player player;

    CameraSystem camera;

	ZombieSystem zombies;
private:
    bool densityView = false;
    static float Clamp01(float v);
};
