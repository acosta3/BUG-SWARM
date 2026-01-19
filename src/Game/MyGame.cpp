// MyGame.cpp
#include "MyGame.h"
#include "../ContestAPI/app.h"
#include "GameConfig.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace GameConfig;

// Optimized static audio function using GameConfig
static void PlayRandomSquish()
{
    const int r = std::rand() % GameTuning::SQUISH_SOUND_COUNT;
    App::PlayAudio(AudioResources::GetSquishSound(r), false);
}

void MyGame::Init()
{
    InitWorld();
    InitObstacles();
    InitSystems();

    mode = GameMode::Menu;

    App::PlayAudio(AudioResources::GAME_MUSIC, true);
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
            // Fresh run when leaving menu
            ResetRun();

            mode = GameMode::Playing;

            float px = 0.0f, py = 0.0f;
            player.GetWorldPosition(px, py);
            camera.Follow(px, py);
        }
        return;
    }

    // -----------------------------
    // Win screen
    // -----------------------------
    if (mode == GameMode::Win)
    {
        float px = 0.0f, py = 0.0f;
        player.GetWorldPosition(px, py);
        UpdateCamera(dtMs, px, py);

        // Return to menu on Start/Enter
        if (in.startPressed)
        {
            mode = GameMode::Menu;
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
    // Life-state handling first - using config constants
    // -----------------------------
    if (life != LifeState::Playing)
    {
        lifeTimerMs += dtMs;

        UpdateCamera(dtMs, px, py);

        hives.Update(dtMs, zombies, nav);
        UpdateZombies(dtMs, px, py);

        if (life == LifeState::DeathPause)
        {
            if (lifeTimerMs >= GameTuning::DEATH_PAUSE_MS)
            {
                RespawnNow();
                life = LifeState::RespawnGrace;
                lifeTimerMs = 0.0f;
            }
        }
        else
        {
            attacks.Update(dtMs);

            if (lifeTimerMs >= GameTuning::RESPAWN_GRACE_MS)
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

    // -----------------------------
    // WIN CONDITION: all hives dead
    // -----------------------------
    if (hives.AliveCount() == 0)
    {
        BeginWin();
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

    if (mode == GameMode::Win)
        RenderWinOverlay();
}

void MyGame::Shutdown()
{
    // Nothing required right now
}

// ------------------------------------------------------------
// Init - now using GameConfig constants
// ------------------------------------------------------------
void MyGame::InitWorld()
{
    player.Init();

    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    respawnX = px;
    respawnY = py;

    player.SetNavGrid(&nav);

    // Using GameConfig constants instead of hardcoded values
    camera.Init(GameTuning::SCREEN_WIDTH, GameTuning::SCREEN_HEIGHT);
    camera.Follow(px, py);

    nav.Init(
        GameTuning::WORLD_MIN_X, GameTuning::WORLD_MIN_Y,
        GameTuning::WORLD_MAX_X, GameTuning::WORLD_MAX_Y,
        GameTuning::NAV_CELL_SIZE
    );
    nav.ClearObstacles();

    life = LifeState::Playing;
    lifeTimerMs = 0.0f;
}

void MyGame::InitObstacles()
{
    // Using GameConfig constants
    const float spread = GameTuning::OBSTACLE_SPREAD;
    const float half = GameTuning::OBSTACLE_HALF_SIZE;

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
        const float barHalfH = GameTuning::BAR_HALF_HEIGHT;
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

    lastDtMs = GameTuning::DEFAULT_DT_MS;
}

// ------------------------------------------------------------
// Update helpers - using GameConfig constants
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
    if (len2 > GameTuning::MOVEMENT_THRESHOLD)
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
    if (len2 > GameTuning::MOVEMENT_THRESHOLD)
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
        // Using GameConfig constant instead of hardcoded 1.5f
        player.Heal(kills * GameTuning::HEAL_PER_KILL);
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
// Death / respawn - using GameConfig constants
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
    player.GiveInvulnerability(GameTuning::INVULNERABILITY_RESPAWN_MS);

    player.SetWorldPosition(respawnX, respawnY);
    player.SetNavGrid(&nav);

    lastTargetCell = -1;
    camera.Follow(respawnX, respawnY);

    player.SetMoveInput(0.0f, 0.0f);
}

// ------------------------------------------------------------
// Win + restart - using GameConfig constants
// ------------------------------------------------------------
void MyGame::BeginWin()
{
    mode = GameMode::Win;
    player.SetMoveInput(0.0f, 0.0f);
}

void MyGame::ResetRun()
{
    // Rebuild obstacles
    nav.ClearObstacles();
    InitObstacles();

    // Reset player - using GameConfig constants
    player.Revive(true);
    player.GiveInvulnerability(GameTuning::INVULNERABILITY_RESET_MS);
    player.SetWorldPosition(respawnX, respawnY);
    player.SetNavGrid(&nav);
    player.SetMoveInput(0.0f, 0.0f);

    // Reset camera
    camera.Follow(respawnX, respawnY);

    // Reset systems
    hives.Init();
    zombies.Init(kMaxZombies, nav);

    // Spawn zombies distributed across alive hives
    {
        const int totalToSpawn = kMaxZombies;
        const auto& hiveList = hives.GetHives();

        int aliveCount = 0;
        for (const auto& h : hiveList)
            if (h.alive) aliveCount++;

        if (aliveCount <= 0)
        {
            zombies.Spawn(totalToSpawn, respawnX, respawnY);
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
    }

    attacks.Init();

    lastAimX = 0.0f;
    lastAimY = 1.0f;
    lastTargetCell = -1;

    life = LifeState::Playing;
    lifeTimerMs = 0.0f;
}

void MyGame::RenderWinOverlay() const
{
    App::Print(470, 520, "YOU WIN");
    App::Print(280, 490, "Press Enter or Start to return to Menu");
    App::Print(310, 460, "Destroy all nests to win again");
}

// ------------------------------------------------------------
// Menu and Pause UI (unchanged - these are UI layout constants)
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