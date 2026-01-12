#pragma once
#include "Player.h"

class MyGame
{
public:
    void Init();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

private:
    Player player;
};
