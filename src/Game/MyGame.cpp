#include "MyGame.h"
#include "../ContestAPI/app.h"
#include <cstdio>

void MyGame::Init()
{
    player.Init();
    camera.Init(1024.0f, 768.0f); // matches APP_VIRTUAL_WIDTH/HEIGHT

    // IMPORTANT: init pool before spawn
    zombies.Init(50000);

    // Spawn around the player's start position so it is always on-screen
    float px, py;
    player.GetWorldPosition(px, py);
    zombies.Spawn(10000, px, py);

    // Optional: set initial camera target so first frame is centered
    camera.Follow(px, py);
}

void MyGame::Update(float deltaTime)
{
    input.Update(deltaTime);
    const InputState& in = input.GetState();

    player.SetMoveInput(in.moveX, in.moveY);
    player.SetStopAnimPressed(in.stopAnimPressed);

    player.Update(deltaTime);

    float px, py;
    player.GetWorldPosition(px, py);

    camera.Follow(px, py);
    camera.Update(deltaTime);

    zombies.Update(deltaTime, px, py);

    // Optional stress test controls if you want
    // if (in.spawn1k)  zombies.Spawn(1000, px, py);
    // if (in.spawn10k) zombies.Spawn(10000, px, py);
    // if (in.clearAll) zombies.Clear();
}

void MyGame::Render()
{
    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    // World
    player.Render(offX, offY);

    // UI
    char buf[64];
    snprintf(buf, sizeof(buf), "Zombies: %d", zombies.AliveCount());
    App::Print(20, 20, buf, 1.0f, 1.0f, 0.0f, GLUT_BITMAP_9_BY_15);

    // Precomputed render data by type (faster than a switch per zombie)
    static const float sizeByType[4] = { 3.0f, 3.5f, 5.0f, 7.0f };
    static const float rByType[4] = { 0.2f, 1.0f, 0.2f, 0.8f };
    static const float gByType[4] = { 1.0f, 0.2f, 0.4f, 0.2f };
    static const float bByType[4] = { 0.2f, 0.2f, 1.0f, 1.0f };

    // Draw zombies as triangles
    for (int i = 0; i < zombies.AliveCount(); i++)
    {
        float x = zombies.GetX(i) - offX;
        float y = zombies.GetY(i) - offY;

        // Optional: simple screen cull (good when zombies spread out)
        // Virtual screen is 1024x768
        if (x < -10.0f || x > 1024.0f + 10.0f || y < -10.0f || y > 768.0f + 10.0f)
            continue;

        int t = zombies.GetType(i);
        DrawZombieTri(x, y, sizeByType[t], rByType[t], gByType[t], bByType[t]);
    }
}

void MyGame::Shutdown()
{
    // Nothing required right now:
    // - Player owns sprite via unique_ptr
    // - ZombieSystem uses vectors managed by RAII
    // - InputSystem has no heap allocations
}

void MyGame::DrawZombieTri(float x, float y, float size, float r, float g, float b)
{
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
        false
    );
}
