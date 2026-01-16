#pragma once
#include "Player.h"
#include "Input.h"

class MyGame
{
public:
    void Init();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

private:
    InputSystem input;
    Player player;
};
