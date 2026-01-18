// MyGame.cpp
#include "MyGame.h"
#include "../ContestAPI/app.h"
#include <cstdio>
#include <cmath>

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
void MyGame::Init()
{
    InitWorld();
    InitObstacles();
    InitSystems();
    App::PlayAudio("./Data/TestData/GameLoopMusic.wav", true); // game music
}

void MyGame::Update(float deltaTimeMs)
{
    lastDtMs = deltaTimeMs;

    UpdateInput(deltaTimeMs);

    // If dead, keep camera stable (optional) and stop gameplay
    if (player.IsDead())
    {
        float px, py;
        player.GetWorldPosition(px, py);
        UpdateCamera(deltaTimeMs, px, py);
        return;
    }

    UpdatePlayer(deltaTimeMs);
    hives.Update(deltaTimeMs, zombies, nav);

    float px, py;
    player.GetWorldPosition(px, py);

    UpdateAttacks(deltaTimeMs);
    UpdateNavFlowField(px, py);
    UpdateCamera(deltaTimeMs, px, py);
    UpdateZombies(deltaTimeMs, px, py);

    // NEW: send kill count to renderer popup
    renderer.NotifyKills(zombies.ConsumeKillsThisFrame());
}

void MyGame::Render()
{
    renderer.RenderFrame(camera, player, nav, zombies, hives, attacks, lastDtMs, densityView);
}

void MyGame::Shutdown()
{
    // Nothing required right now
}

// ------------------------------------------------------------
// Init helpers
// ------------------------------------------------------------
void MyGame::InitWorld()
{
    // Player first
    player.Init();

    float px, py;
    player.GetWorldPosition(px, py);
    player.SetNavGrid(&nav);

    // Camera next
    camera.Init(1024.0f, 768.0f);
    camera.Follow(px, py);

    // NavGrid before zombies
    nav.Init(-5000.0f, -5000.0f, 5000.0f, 5000.0f, 60.0f);
    nav.ClearObstacles();
}

void MyGame::InitObstacles()
{
    // Thin + spread version of your same pattern.
 // Change these two knobs:
    const float spread = 1.8f;   // >1 spreads out positions
    const float half = 9.0f;   // half-size of each block (thickness/size)

    auto AddBlock = [&](float cx, float cy)
        {
            // scale positions outward
            const float x = cx * spread;
            const float y = cy * spread;

            // thin small square (you can make it a skinny wall by changing width/height separately)
            nav.AddObstacleRect(x - half, y - half, x + half, y + half);
        };

    // Row 1 (centers from your original rects)
    AddBlock(-400.0f, -240.0f);
    AddBlock(-200.0f, -250.0f);
    AddBlock(20.0f, -240.0f);

    // Row 2
    AddBlock(-260.0f, -120.0f);
    AddBlock(-40.0f, -120.0f);

    // Row 3
    AddBlock(-400.0f, 20.0f);
    AddBlock(-200.0f, 10.0f);
    AddBlock(20.0f, 20.0f);

    // Row 4
    AddBlock(-260.0f, 140.0f);
    AddBlock(-40.0f, 140.0f);

    // Bar (make it thinner + spread in position too)
    {
        float x0 = -300.0f * spread;
        float x1 = 100.0f * spread;
        float y = -187.5f * spread;   // center of -200..-175 is -187.5
        const float barHalfH = 6.0f;   // thin bar thickness

        nav.AddObstacleRect(x0, y - barHalfH, x1, y + barHalfH);
    }

    // nav.ClearObstacles(); // keep this OFF

}

void MyGame::InitSystems()
{
    float px, py;
    player.GetWorldPosition(px, py);

    hives.Init();

    zombies.Init(kMaxZombies, nav);
    

    const int totalToSpawn = kMaxZombies / 2;

    // Get alive hives (however your HiveSystem exposes them)
    // Option A: hives.GetHives() returns a vector/list of Hive with fields {x,y,alive}
    const auto& hiveList = hives.GetHives();

    int aliveCount = 0;
    for (const auto& h : hiveList) if (h.alive) aliveCount++;

    if (aliveCount <= 0)
    {
        // fallback: spawn near player if no hives
        zombies.Spawn(totalToSpawn, px, py);
    }
    else
    {
        const int base = totalToSpawn / aliveCount;
        int rem = totalToSpawn % aliveCount;

        for (const auto& h : hiveList)
        {
            if (!h.alive) continue;

            int n = base + (rem > 0 ? 1 : 0);
            if (rem > 0) rem--;

            if (n > 0)
                zombies.Spawn(n, h.x, h.y);
        }
    }


    attacks.Init();

    // Persistent state
    lastAimX = 0.0f;
    lastAimY = 1.0f;
    lastTargetCell = -1;

    lastDtMs = 16.0f;
}

// ------------------------------------------------------------
// Update helpers
// ------------------------------------------------------------
void MyGame::UpdateInput(float deltaTimeMs)
{
    input.SetEnabled(!player.IsDead());
    input.Update(deltaTimeMs);

    const InputState& in = input.GetState();
    if (in.toggleViewPressed)
        densityView = !densityView;
}

void MyGame::UpdatePlayer(float deltaTimeMs)
{
    const InputState& in = input.GetState();

    player.SetMoveInput(in.moveX, in.moveY);
    player.Update(deltaTimeMs);
    player.ApplyScaleInput(input.GetState().scaleUpHeld, input.GetState().scaleDownHeld, deltaTimeMs);

    // Update last aim when moving
    const float len2 = in.moveX * in.moveX + in.moveY * in.moveY;
    if (len2 > 0.0001f)
    {
        lastAimX = in.moveX;
        lastAimY = in.moveY;
    }
}

AttackInput MyGame::BuildAttackInput(const InputState& in)
{
    AttackInput a;
    a.pulsePressed = in.pulsePressed;
    a.slashPressed = in.slashPressed;
    a.meteorPressed = in.meteorPressed;

    // Aim: movement dir, but keep last aim when idle
    const float len2 = in.moveX * in.moveX + in.moveY * in.moveY;
    if (len2 > 0.0001f)
    {
        a.aimX = in.moveX;
        a.aimY = in.moveY;
    }
    else
    {
        a.aimX = lastAimX;
        a.aimY = lastAimY;
    }

    return a;
}

void MyGame::UpdateAttacks(float deltaTimeMs)
{
    attacks.Update(deltaTimeMs);

    float px, py;
    player.GetWorldPosition(px, py);

    const InputState& in = input.GetState();
    const AttackInput a = BuildAttackInput(in);

    attacks.Process(a, px, py, player.GetScale(), zombies, hives, camera);
    const int kills = attacks.GetLastSlashKills(); // only pulse kills heal player for now
    if (kills > 0)
    {
        const float healPerKill = 1.5f; // tune this
        player.Heal(kills * healPerKill);
    }
}

void MyGame::UpdateNavFlowField(float playerX, float playerY)
{
    const int curTargetCell = nav.CellIndex(playerX, playerY);
    if (curTargetCell != lastTargetCell)
    {
        nav.BuildFlowField(playerX, playerY);
        lastTargetCell = curTargetCell;
    }
}

void MyGame::UpdateCamera(float deltaTimeMs, float playerX, float playerY)
{
    camera.Follow(playerX, playerY);
    camera.Update(deltaTimeMs);
}

void MyGame::UpdateZombies(float deltaTimeMs, float playerX, float playerY)
{
    const int dmg = zombies.Update(deltaTimeMs, playerX, playerY, nav);
    if (dmg > 0)
        player.TakeDamage(dmg);
}
