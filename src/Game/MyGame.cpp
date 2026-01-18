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

    App::PlayAudio("./Data/TestData/GameLoopMusic.wav", true);
}

void MyGame::Update(float deltaTimeMs)
{
    lastDtMs = deltaTimeMs;

    float px, py;
    player.GetWorldPosition(px, py);

    // Handle life-state timers first
    if (life != LifeState::Playing)
    {
        lifeTimerMs += deltaTimeMs;

        // Keep camera smooth even while input is locked / dead
        UpdateCamera(deltaTimeMs, px, py);

        // Optional: keep world sim running while dead (hives evolve, zombies move)
        // If you want the whole world to freeze, comment these two lines.
        hives.Update(deltaTimeMs, zombies, nav);
        UpdateZombies(deltaTimeMs, px, py); // damage will be ignored while dead via IsDead() check inside BeginDeath logic

        if (life == LifeState::DeathPause)
        {
            if (lifeTimerMs >= deathPauseMs)
            {
                RespawnNow();
                life = LifeState::RespawnGrace;
                lifeTimerMs = 0.0f;
            }
        }
        else if (life == LifeState::RespawnGrace)
        {
            // During grace, you can still update attacks cooldowns visually
            attacks.Update(deltaTimeMs);

            if (lifeTimerMs >= respawnGraceMs)
            {
                life = LifeState::Playing;
                lifeTimerMs = 0.0f;
            }
        }

        // Render popups still
        renderer.NotifyKills(zombies.ConsumeKillsThisFrame());
        return;
    }

    // Normal gameplay
    UpdateInput(deltaTimeMs);

    UpdatePlayer(deltaTimeMs);
    hives.Update(deltaTimeMs, zombies, nav);

    player.GetWorldPosition(px, py);

    UpdateAttacks(deltaTimeMs);
    UpdateNavFlowField(px, py);
    UpdateCamera(deltaTimeMs, px, py);

    UpdateZombies(deltaTimeMs, px, py);

    // If player died this frame, enter death state
    if (player.IsDead())
    {
        BeginDeath(px, py);
        return;
    }

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
    player.Init();

    float px, py;
    player.GetWorldPosition(px, py);

    // Save spawn from initial position (so respawn is stable)
    respawnX = px;
    respawnY = py;

    player.SetNavGrid(&nav);

    camera.Init(1024.0f, 768.0f);
    camera.Follow(px, py);

    nav.Init(-5000.0f, -5000.0f, 5000.0f, 5000.0f, 60.0f);
    nav.ClearObstacles();

    life = LifeState::Playing;
    lifeTimerMs = 0.0f;
}

void MyGame::InitObstacles()
{
    const float spread = 1.8f;
    const float half = 9.0f;

    auto AddBlock = [&](float cx, float cy)
        {
            const float x = cx * spread;
            const float y = cy * spread;
            nav.AddObstacleRect(x - half, y - half, x + half, y + half);
        };

    AddBlock(-400.0f, -240.0f);
    AddBlock(-200.0f, -250.0f);
    AddBlock(20.0f, -240.0f);

    AddBlock(-260.0f, -120.0f);
    AddBlock(-40.0f, -120.0f);

    AddBlock(-400.0f, 20.0f);
    AddBlock(-200.0f, 10.0f);
    AddBlock(20.0f, 20.0f);

    AddBlock(-260.0f, 140.0f);
    AddBlock(-40.0f, 140.0f);

    {
        float x0 = -300.0f * spread;
        float x1 = 100.0f * spread;
        float y = -187.5f * spread;
        const float barHalfH = 6.0f;

        nav.AddObstacleRect(x0, y - barHalfH, x1, y + barHalfH);
    }
}

void MyGame::InitSystems()
{
    float px, py;
    player.GetWorldPosition(px, py);

    hives.Init();
    zombies.Init(kMaxZombies, nav);

    const int totalToSpawn = kMaxZombies / 2;
    const auto& hiveList = hives.GetHives();

    int aliveCount = 0;
    for (const auto& h : hiveList) if (h.alive) aliveCount++;

    if (aliveCount <= 0)
    {
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

    lastAimX = 0.0f;
    lastAimY = 1.0f;
    lastTargetCell = -1;
    lastDtMs = 16.0f;
}

// ------------------------------------------------------------
// Update helpers
// ------------------------------------------------------------
bool MyGame::InputLocked() const
{
    return (life != LifeState::Playing);
}

void MyGame::UpdateInput(float deltaTimeMs)
{
    input.SetEnabled(!InputLocked());
    input.Update(deltaTimeMs);

    const InputState& in = input.GetState();
    if (in.toggleViewPressed)
        densityView = !densityView;
}

void MyGame::UpdatePlayer(float deltaTimeMs)
{
    if (InputLocked())
    {
        player.SetMoveInput(0.0f, 0.0f);
        return;
    }

    const InputState& in = input.GetState();

    player.SetMoveInput(in.moveX, in.moveY);
    player.Update(deltaTimeMs);

    player.ApplyScaleInput(in.scaleUpHeld, in.scaleDownHeld, deltaTimeMs);

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

    if (InputLocked())
        return;

    float px, py;
    player.GetWorldPosition(px, py);

    const InputState& in = input.GetState();
    const AttackInput a = BuildAttackInput(in);

    attacks.Process(a, px, py, player.GetScale(), zombies, hives, camera);

    const int kills = attacks.GetLastSlashKills();
    if (kills > 0)
    {
        const float healPerKill = 1.5f;
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
    // If player is dead or input-locked states, we do not apply damage
    // (they can still move, but cannot keep killing you on the respawn pause).
    if (life != LifeState::Playing || player.IsDead())
    {
        zombies.Update(deltaTimeMs, playerX, playerY, nav);
        return;
    }

    const int dmg = zombies.Update(deltaTimeMs, playerX, playerY, nav);
    if (dmg > 0)
        player.TakeDamage(dmg);
}

// ------------------------------------------------------------
// Death / respawn
// ------------------------------------------------------------
void MyGame::BeginDeath(float playerX, float playerY)
{
    // Lock input and stop any accidental movement
    life = LifeState::DeathPause;
    lifeTimerMs = 0.0f;

    player.SetMoveInput(0.0f, 0.0f);

    // Optional: set camera to keep following the last location
    camera.Follow(playerX, playerY);

    // Reset aim so respawn attacks feel consistent
    lastAimX = 0.0f;
    lastAimY = 1.0f;
}

void MyGame::RespawnNow()
{
    player.Revive();
    player.GiveInvulnerability(2000.0f);
    player.SetWorldPosition(respawnX, respawnY);
    player.SetNavGrid(&nav);

    lastTargetCell = -1; // force flow field rebuild
    camera.Follow(respawnX, respawnY);

    player.SetMoveInput(0.0f, 0.0f);
}
