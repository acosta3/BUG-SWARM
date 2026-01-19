// MyGame.cpp
#include "MyGame.h"
#include "../ContestAPI/app.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

static void PlayRandomSquish()
{
    const int r = std::rand() % 3;

    switch (r)
    {
    case 0:
        App::PlayAudio("./Data/TestData/squish1.wav", false);
        break;
    case 1:
        App::PlayAudio("./Data/TestData/squish2.wav", false);
        break;
    default:
        App::PlayAudio("./Data/TestData/squish3.wav", false);
        break;
    }
}

void MyGame::Init()
{
    InitWorld();
    InitObstacles();
    InitSystems();

    mode = GameMode::Menu;

    App::PlayAudio("./Data/TestData/GameLoopMusic.wav", true);
}

void MyGame::Update(float dtMs)
{
    lastDtMs = dtMs;

    // IMPORTANT: Update input ONCE per frame
    input.SetEnabled(true);
    input.Update(dtMs);

    const InputState& in = input.GetState();

    // Handle any "global" input that just reads state (no re-update)
    UpdateInput(dtMs);

    // -----------------------------
    // Menu
    // -----------------------------
    if (mode == GameMode::Menu)
    {
        if (in.startPressed)
        {
            mode = GameMode::Playing;

            life = LifeState::Playing;
            lifeTimerMs = 0.0f;
            player.Revive(true);
            player.SetMoveInput(0.0f, 0.0f);

            float px = 0.0f, py = 0.0f;
            player.GetWorldPosition(px, py);
            camera.Follow(px, py);
        }
        return;
    }

    // -----------------------------
    // Pause toggle (only when not in menu)
    // -----------------------------
    if (in.pausePressed)
    {
        if (mode == GameMode::Playing)
        {
            mode = GameMode::Paused;
            player.SetMoveInput(0.0f, 0.0f);
            return;
        }
        else if (mode == GameMode::Paused)
        {
            mode = GameMode::Playing;
            return;
        }
    }

    // If paused, do not simulate gameplay
    if (mode == GameMode::Paused)
        return;

    // From here: mode == Playing

    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    // -----------------------------
    // Life-state handling first
    // -----------------------------
    if (life != LifeState::Playing)
    {
        lifeTimerMs += dtMs;

        UpdateCamera(dtMs, px, py);

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
        else
        {
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
    UpdatePlayer(dtMs);
    hives.Update(dtMs, zombies, nav);

    player.GetWorldPosition(px, py);

    UpdateAttacks(dtMs);
    UpdateNavFlowField(px, py);
    UpdateCamera(dtMs, px, py);

    UpdateZombies(dtMs, px, py);

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
    if (mode == GameMode::Menu)
    {
        RenderMenu();
        return;
    }

    renderer.RenderFrame(camera, player, nav, zombies, hives, attacks, lastDtMs, densityView);

    if (mode == GameMode::Paused)
        RenderPauseOverlay();
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
    player.Init();

    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

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

    AddBlock(-560.0f, -240.0f);
    AddBlock(-560.0f, -120.0f);
    AddBlock(-560.0f, 20.0f);
    AddBlock(-560.0f, 140.0f);

    AddBlock(180.0f, -240.0f);
    AddBlock(180.0f, -120.0f);
    AddBlock(180.0f, 20.0f);
    AddBlock(180.0f, 140.0f);

    AddBlock(-400.0f, -360.0f);
    AddBlock(-200.0f, -360.0f);
    AddBlock(20.0f, -360.0f);

    AddBlock(-400.0f, 260.0f);
    AddBlock(-200.0f, 260.0f);
    AddBlock(20.0f, 260.0f);

    AddBlock(-560.0f, -360.0f);
    AddBlock(180.0f, -360.0f);
    AddBlock(-560.0f, 260.0f);
    AddBlock(180.0f, 260.0f);

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

void MyGame::UpdateInput(float /*dtMs*/)
{
    // IMPORTANT: do NOT call input.Update() here
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

    const int kills = attacks.GetLastSlashKills();
    if (kills > 0)
    {
        const float healPerKill = 1.5f;
        player.Heal(kills * healPerKill);
        PlayRandomSquish();
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

// ------------------------------------------------------------
// Menu and Pause UI
// ------------------------------------------------------------
void MyGame::RenderMenu() const
{
    App::Print(420, 520, "BUG SWARM");

    App::Print(340, 490, "Start:  Enter   or   Start");

    App::Print(340, 450, "Keyboard");
    App::Print(340, 430, "Move:   W A S D");
    App::Print(340, 410, "View:   V");
    App::Print(340, 390, "Pulse:  Space");
    App::Print(340, 370, "Slash:  F");
    App::Print(340, 350, "Meteor: E");
    App::Print(340, 330, "Scale:  Left/Right Arrow");
    App::Print(340, 310, "Pause:  Esc");

    App::Print(580, 450, "Controller");
    App::Print(580, 430, "Move:   Left Stick");
    App::Print(580, 410, "View:   DPad Down");
    App::Print(580, 390, "Pulse:  B");
    App::Print(580, 370, "Slash:  X");
    App::Print(580, 350, "Meteor: Y");
    App::Print(580, 330, "Scale:  LB / RB");
    App::Print(580, 310, "Pause:  Start");
}

void MyGame::RenderPauseOverlay() const
{
    App::Print(470, 520, "PAUSED");
    App::Print(330, 490, "Resume:  Enter or Start");
    App::Print(330, 470, "Pause:   Esc   or Start");

    App::Print(340, 430, "Keyboard");
    App::Print(340, 410, "Move:   W A S D");
    App::Print(340, 390, "View:   V");
    App::Print(340, 370, "Pulse:  Space");
    App::Print(340, 350, "Slash:  F");
    App::Print(340, 330, "Meteor: E");

    App::Print(580, 430, "Controller");
    App::Print(580, 410, "Move:   Left Stick");
    App::Print(580, 390, "View:   DPad Down");
    App::Print(580, 370, "Pulse:  B");
    App::Print(580, 350, "Slash:  X");
    App::Print(580, 330, "Meteor: Y");
}
