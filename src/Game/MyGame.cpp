// MyGame.cpp
#include "MyGame.h"
#include "../ContestAPI/app.h"

#include <cmath>
#include <cstdio>

void MyGame::Init()
{
    InitWorld();
    InitObstacles();
    InitSystems();

    App::PlayAudio("./Data/TestData/GameLoopMusic.wav", true);
}

void MyGame::Update(float dtMs)
{
    lastDtMs = dtMs;

    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    // -----------------------------
    // Life-state handling first
    // -----------------------------
    if (life != LifeState::Playing)
    {
        lifeTimerMs += dtMs;

        // Camera stays smooth even while input is locked
        UpdateCamera(dtMs, px, py);

        // World can keep simulating while dead/locked
        hives.Update(dtMs, zombies, nav);
        UpdateZombies(dtMs, px, py);

        if (life == LifeState::DeathPause)
        {
            if (lifeTimerMs >= deathPauseMs)
            {
                RespawnNow();
                life = LifeState::RespawnGrace;
                lifeTimerMs = 0.0f;
            }
        }
        else // RespawnGrace
        {
            // Cooldowns can tick during grace
            attacks.Update(dtMs);

            if (lifeTimerMs >= respawnGraceMs)
            {
                life = LifeState::Playing;
                lifeTimerMs = 0.0f;
            }
        }

        renderer.NotifyKills(zombies.ConsumeKillsThisFrame());
        return;
    }

    // -----------------------------
    // Normal gameplay
    // -----------------------------
    UpdateInput(dtMs);

    UpdatePlayer(dtMs);
    hives.Update(dtMs, zombies, nav);

    player.GetWorldPosition(px, py);

    UpdateAttacks(dtMs);
    UpdateNavFlowField(px, py);
    UpdateCamera(dtMs, px, py);

    UpdateZombies(dtMs, px, py);

    // If the player died this frame, enter death state
    if (player.IsDead())
    {
        BeginDeath(px, py);
        renderer.NotifyKills(zombies.ConsumeKillsThisFrame());
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
// Init
// ------------------------------------------------------------
void MyGame::InitWorld()
{
    // Player
    player.Init();

    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    // Save spawn from initial position
    respawnX = px;
    respawnY = py;

    player.SetNavGrid(&nav);

    // Camera
    camera.Init(1024.0f, 768.0f);
    camera.Follow(px, py);

    // Nav
    nav.Init(-5000.0f, -5000.0f, 5000.0f, 5000.0f, 60.0f);
    nav.ClearObstacles();

    // Life state
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

    // -------------------------
    // Core pattern (yours)
    // -------------------------
    // Row 1
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

    // Thin bar (yours)
    {
        float x0 = -300.0f * spread;
        float x1 = 100.0f * spread;
        float y = -187.5f * spread;
        const float barHalfH = 6.0f;
        nav.AddObstacleRect(x0, y - barHalfH, x1, y + barHalfH);
    }

    // -------------------------
    // NEW: bigger "ring" around the play area
    // Keeps the same spacing feel, but adds more structure on screen
    // -------------------------

    // Outer columns (left/right)
    AddBlock(-560.0f, -240.0f);
    AddBlock(-560.0f, -120.0f);
    AddBlock(-560.0f, 20.0f);
    AddBlock(-560.0f, 140.0f);

    AddBlock(180.0f, -240.0f);
    AddBlock(180.0f, -120.0f);
    AddBlock(180.0f, 20.0f);
    AddBlock(180.0f, 140.0f);

    // Outer rows (top/bottom)
    AddBlock(-400.0f, -360.0f);
    AddBlock(-200.0f, -360.0f);
    AddBlock(20.0f, -360.0f);

    AddBlock(-400.0f, 260.0f);
    AddBlock(-200.0f, 260.0f);
    AddBlock(20.0f, 260.0f);

    // Corner accents (makes the screen edges feel “designed”)
    AddBlock(-560.0f, -360.0f);
    AddBlock(180.0f, -360.0f);
    AddBlock(-560.0f, 260.0f);
    AddBlock(180.0f, 260.0f);

    // Small inner accents to break empty space
    AddBlock(-120.0f, -40.0f);
    AddBlock(-120.0f, 90.0f);
    AddBlock(-320.0f, -40.0f);
    AddBlock(-320.0f, 90.0f);
}

void MyGame::InitSystems()
{
    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    hives.Init();

    zombies.Init(kMaxZombies, nav);

    const int totalToSpawn = kMaxZombies;
    const auto& hiveList = hives.GetHives();

    int aliveCount = 0;
    for (const auto& h : hiveList)
        if (h.alive) aliveCount++;

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

void MyGame::UpdateInput(float dtMs)
{
    input.SetEnabled(!InputLocked());
    input.Update(dtMs);

    const InputState& in = input.GetState();
    if (in.toggleViewPressed)
        densityView = !densityView;
}

void MyGame::UpdatePlayer(float dtMs)
{
    if (InputLocked())
    {
        player.SetMoveInput(0.0f, 0.0f);
        return;
    }

    const InputState& in = input.GetState();

    player.SetMoveInput(in.moveX, in.moveY);
    player.Update(dtMs);

    player.ApplyScaleInput(in.scaleUpHeld, in.scaleDownHeld, dtMs);

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

void MyGame::UpdateAttacks(float dtMs)
{
    attacks.Update(dtMs);

    if (InputLocked())
        return;

    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    const InputState& in = input.GetState();
    const AttackInput a = BuildAttackInput(in);

    attacks.Process(a, px, py, player.GetScale(), zombies, hives, camera);

    // Healing rule (keep as you had it)
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

void MyGame::UpdateCamera(float dtMs, float playerX, float playerY)
{
    camera.Follow(playerX, playerY);
    camera.Update(dtMs);
}

void MyGame::UpdateZombies(float dtMs, float playerX, float playerY)
{
    // During death/grace, zombies can move but cannot damage the player
    if (life != LifeState::Playing || player.IsDead())
    {
        (void)zombies.Update(dtMs, playerX, playerY, nav);
        return;
    }

    const int dmg = zombies.Update(dtMs, playerX, playerY, nav);
    if (dmg > 0)
        player.TakeDamage(dmg);
}

// ------------------------------------------------------------
// Death / respawn
// ------------------------------------------------------------
void MyGame::BeginDeath(float playerX, float playerY)
{
    life = LifeState::DeathPause;
    lifeTimerMs = 0.0f;

    player.SetMoveInput(0.0f, 0.0f);

    camera.Follow(playerX, playerY);

    lastAimX = 0.0f;
    lastAimY = 1.0f;
}

void MyGame::RespawnNow()
{
    player.Revive(true);
    player.GiveInvulnerability(2000.0f);

    player.SetWorldPosition(respawnX, respawnY);
    player.SetNavGrid(&nav);

    lastTargetCell = -1;
    camera.Follow(respawnX, respawnY);

    player.SetMoveInput(0.0f, 0.0f);
}
