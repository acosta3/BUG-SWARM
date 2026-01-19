// MyGame.cpp
#include "MyGame.h"
#include "../ContestAPI/app.h"
#include "GameConfig.h"
#include "DifficultyManager.h"
#include "UIRenderer.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace GameConfig;

static void PlayRandomSquish()
{
    const int r = std::rand() % GameTuning::SQUISH_SOUND_COUNT;
    App::PlayAudio(AudioResources::GetSquishSound(r), false);
}

int MyGame::GetMaxZombiesForDifficulty() const
{
    return DifficultyManager::GetMaxZombies(selectedDifficulty);
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

    input.SetEnabled(true);
    input.Update(dtMs);

    const InputState& in = input.GetState();

    UpdateInput(dtMs);

    if (mode == GameMode::Menu)
    {
        // Difficulty selection with arrow keys or D-pad
        static bool diffUpPressed = false;
        static bool diffDownPressed = false;

        const bool upNow = in.moveY > 0.5f;
        const bool downNow = in.moveY < -0.5f;

        if (upNow && !diffUpPressed)
        {
            int current = static_cast<int>(selectedDifficulty);
            current = (current - 1 + 4) % 4;
            selectedDifficulty = static_cast<DifficultyLevel>(current);
        }

        if (downNow && !diffDownPressed)
        {
            int current = static_cast<int>(selectedDifficulty);
            current = (current + 1) % 4;
            selectedDifficulty = static_cast<DifficultyLevel>(current);
        }

        diffUpPressed = upNow;
        diffDownPressed = downNow;

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

    if (mode == GameMode::Paused)
        return;

    // From here: mode == Playing

    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);


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
        UIRenderer::RenderMenu(*this, hives);
        return;
    }

    renderer.RenderFrame(camera, player, nav, zombies, hives, attacks, lastDtMs, densityView);

    if (mode == GameMode::Paused)
        UIRenderer::RenderPauseOverlay(player, hives, zombies);

    if (mode == GameMode::Win)
        UIRenderer::RenderWinOverlay(player, zombies, GetMaxZombiesForDifficulty());
}

void MyGame::Shutdown()
{
    // Nothing required right now
}

// Initialization
void MyGame::InitWorld()
{
    player.Init();

    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    respawnX = px;
    respawnY = py;

    player.SetNavGrid(&nav);

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
    const float spread = GameTuning::OBSTACLE_SPREAD;
    const float half = GameTuning::OBSTACLE_HALF_SIZE;

    auto AddBlock = [&](float cx, float cy)
        {
            const float x = cx * spread;
            const float y = cy * spread;
            nav.AddObstacleRect(x - half, y - half, x + half, y + half);
        };



    const float bMin = BoundaryConfig::BOUNDARY_MIN;
    const float bMax = BoundaryConfig::BOUNDARY_MAX;
    const float thick = BoundaryConfig::WALL_THICKNESS;

    // Top wall
    nav.AddObstacleRect(bMin - thick, bMax, bMax + thick, bMax + thick);

    // Bottom wall
    nav.AddObstacleRect(bMin - thick, bMin - thick, bMax + thick, bMin);

    // Left wall
    nav.AddObstacleRect(bMin - thick, bMin, bMin, bMax);

    // Right wall
    nav.AddObstacleRect(bMax, bMin, bMax + thick, bMax);

    // ========== INTERIOR OBSTACLES ==========

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

    const int totalToSpawn = GetMaxZombiesForDifficulty();
    zombies.Init(totalToSpawn, nav);

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

// Update helpers
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

// Death and respawn
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

// Win and restart
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

    player.Revive(true);
    player.GiveInvulnerability(GameTuning::INVULNERABILITY_RESET_MS);
    player.SetWorldPosition(respawnX, respawnY);
    player.SetNavGrid(&nav);
    player.SetMoveInput(0.0f, 0.0f);

    // Reset camera
    camera.Follow(respawnX, respawnY);

    // Reset systems with selected difficulty
    hives.Init();
    const int totalToSpawn = GetMaxZombiesForDifficulty();
    zombies.Init(totalToSpawn, nav);

    // Spawn zombies distributed across alive hives
    {
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