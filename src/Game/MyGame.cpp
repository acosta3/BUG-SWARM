#include "MyGame.h"
#include "../ContestAPI/app.h"
#include <cstdio>


void MyGame::Init()
{
    player.Init();
    camera.Init(1024.0f, 768.0f); // matches APP_VIRTUAL_WIDTH/HEIGHT
    zombies.Init(50000);
    zombies.Spawn(50000, 512.0f, 384.0f);

    // created once in Init()
    //CSimpleSprite* zombieSprite[4]; // or whatever your sprite type is // want to reuse the sprites

    




}

void MyGame::Update(float deltaTime)
{
    input.Update(deltaTime);
    const InputState& in = input.GetState();

    player.SetMoveInput(in.moveX, in.moveY);
    player.SetStopAnimPressed(in.stopAnimPressed);

    player.Update(deltaTime);

	float px, py; // store the position of the player
    player.GetWorldPosition(px, py);
    camera.Follow(px, py);
    camera.Update(deltaTime);
    zombies.Update(deltaTime, px, py);


    


}

void MyGame::Render()
{
   

    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    player.Render(offX, offY);

    // print statement
    char buf[64];
    snprintf(buf, sizeof(buf), "Zombies: %d", zombies.AliveCount());
    App::Print(200, 20, buf, 1.0f, 1.0f, 0.0f, GLUT_BITMAP_9_BY_15);
   

	// Want to draw zombies here
    // use sprite and reuse them
    for (int i = 0; i < zombies.AliveCount(); i++)
    {
        float worldX = zombies.GetX(i);
        float worldY = zombies.GetY(i);

        float x = worldX - offX;
        float y = worldY - offY;

        float size = 3.0f;
        float r = 0.2f, g = 1.0f, b = 0.2f;

        switch (zombies.GetType(i))
        {
        case ZombieSystem::GREEN:        r = 0.2f; g = 1.0f; b = 0.2f; size = 3.0f; break;
        case ZombieSystem::RED:          r = 1.0f; g = 0.2f; b = 0.2f; size = 3.5f; break;
        case ZombieSystem::BLUE:         r = 0.2f; g = 0.4f; b = 1.0f; size = 5.0f; break;
        case ZombieSystem::PURPLE_ELITE: r = 0.8f; g = 0.2f; b = 1.0f; size = 7.0f; break;
        }

        DrawZombieTri(x, y, size, r, g, b);
    }
    
}

void MyGame::Shutdown()
{
    // Nothing required right now:
    // - Player owns sprite via unique_ptr
    // - InputSystem has no heap allocations
}

void MyGame::DrawZombieTri(float x, float y, float size, float r, float g, float b)
{
    // Triangle points (2D), z=0, w=1
    float p1x = x;
    float p1y = y - size;

    float p2x = x - size;
    float p2y = y + size;

    float p3x = x + size;
    float p3y = y + size;

    App::DrawTriangle(
        p1x, p1y, 0.0f, 1.0f,
        p2x, p2y, 0.0f, 1.0f,
        p3x, p3y, 0.0f, 1.0f,
        r, g, b,
        r, g, b,
        r, g, b,
        false // filled, not wireframe
    );
}
