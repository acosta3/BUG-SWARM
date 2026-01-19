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

    //nav.ClearObstacles(); // delete after 
}

void MyGame::InitSystems()
{
    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    hives.Init();
    zombies.Init(GameConfig::SystemCapacity::MAX_ZOMBIES, nav);

    const int totalToSpawn = GameConfig::SystemCapacity::MAX_ZOMBIES;
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
    zombies.Init(GameConfig::SystemCapacity::MAX_ZOMBIES, nav);

    // Spawn zombies distributed across alive hives
    {
        const int totalToSpawn = GameConfig::SystemCapacity::MAX_ZOMBIES;
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
    // ========== SCI-FI BACKGROUND ==========
    static float menuTime = 0.0f;
    menuTime += 0.016f;
    if (menuTime > 1000.0f) menuTime = 0.0f;

    // Scanlines
    for (int y = 0; y < 768; y += 3)
    {
        const float intensity = (y % 6 == 0) ? 0.03f : 0.015f;
        App::DrawLine(0.0f, static_cast<float>(y), 1024.0f, static_cast<float>(y),
            0.02f + intensity, 0.03f + intensity, 0.05f + intensity);
    }

    // Animated grid
    const float gridSize = 64.0f;
    const float pulse = 0.02f + 0.01f * std::sin(menuTime * 0.6f);

    for (float x = 0; x < 1024.0f; x += gridSize)
    {
        const bool isMajor = (static_cast<int>(x / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(x, 0.0f, x, 768.0f, alpha, alpha + 0.01f, alpha + 0.03f);
    }

    for (float y = 0; y < 768.0f; y += gridSize)
    {
        const bool isMajor = (static_cast<int>(y / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(0.0f, y, 1024.0f, y, alpha, alpha + 0.01f, alpha + 0.03f);
    }

    // ========== TITLE ==========
    App::Print(387, 702, "BUG SWARM", 0.1f, 0.1f, 0.1f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(385, 700, "BUG SWARM", 1.0f, 0.95f, 0.20f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(280, 670, "TACTICAL ERADICATION PROTOCOL", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    // ========== MAP PREVIEW ==========
    const float mapX = 80.0f;
    const float mapY = 380.0f;
    const float mapW = 240.0f;
    const float mapH = 240.0f;
    const float worldSize = 2600.0f;
    const float scale = mapW / worldSize;

    App::Print(80, 635, "TACTICAL MAP", 0.70f, 0.90f, 1.00f);

    // Map borders
    App::DrawLine(mapX - 2, mapY - 2, mapX + mapW + 2, mapY - 2, 0.70f, 0.90f, 1.00f);
    App::DrawLine(mapX + mapW + 2, mapY - 2, mapX + mapW + 2, mapY + mapH + 2, 0.70f, 0.90f, 1.00f);
    App::DrawLine(mapX + mapW + 2, mapY + mapH + 2, mapX - 2, mapY + mapH + 2, 0.70f, 0.90f, 1.00f);
    App::DrawLine(mapX - 2, mapY + mapH + 2, mapX - 2, mapY - 2, 0.70f, 0.90f, 1.00f);

    App::DrawLine(mapX, mapY, mapX + mapW, mapY, 1.0f, 0.95f, 0.20f);
    App::DrawLine(mapX + mapW, mapY, mapX + mapW, mapY + mapH, 1.0f, 0.95f, 0.20f);
    App::DrawLine(mapX + mapW, mapY + mapH, mapX, mapY + mapH, 1.0f, 0.95f, 0.20f);
    App::DrawLine(mapX, mapY + mapH, mapX, mapY, 1.0f, 0.95f, 0.20f);

    // Draw walls and hives
    const float bMin = BoundaryConfig::BOUNDARY_MIN;
    const float bMax = BoundaryConfig::BOUNDARY_MAX;
    const float centerX = mapX + mapW * 0.5f;
    const float centerY = mapY + mapH * 0.5f;

    auto WorldToMap = [&](float wx, float wy, float& mx, float& my)
        {
            mx = centerX + wx * scale;
            my = centerY + wy * scale;
        };

    float x1, y1, x2, y2;
    WorldToMap(bMin, bMin, x1, y1);
    WorldToMap(bMax, bMax, x2, y2);

    const float wallAlpha = 0.3f;
    App::DrawLine(x1, y1, x2, y1, 0.65f * wallAlpha, 0.55f * wallAlpha, 0.15f * wallAlpha);
    App::DrawLine(x1, y2, x2, y2, 0.65f * wallAlpha, 0.55f * wallAlpha, 0.15f * wallAlpha);
    App::DrawLine(x1, y1, x1, y2, 0.65f * wallAlpha, 0.55f * wallAlpha, 0.15f * wallAlpha);
    App::DrawLine(x2, y1, x2, y2, 0.65f * wallAlpha, 0.55f * wallAlpha, 0.15f * wallAlpha);

    const auto& hiveList = hives.GetHives();
    const float hivePulse = std::sin(menuTime * 3.0f) * 0.3f + 0.7f;

    for (const auto& h : hiveList)
    {
        float mx, my;
        WorldToMap(h.x, h.y, mx, my);
        const float r = h.radius * scale;
        const int segments = 16;

        float prevX = mx + (r + 3) * hivePulse;
        float prevY = my;

        for (int i = 1; i <= segments; i++)
        {
            const float angle = (GameConfig::HiveConfig::TWO_PI * i) / segments;
            const float x = mx + std::cos(angle) * (r + 3) * hivePulse;
            const float y = my + std::sin(angle) * (r + 3) * hivePulse;
            App::DrawLine(prevX, prevY, x, y, 1.0f * hivePulse, 0.85f * hivePulse, 0.10f * hivePulse);
            prevX = x;
            prevY = y;
        }

        prevX = mx + r;
        prevY = my;

        for (int i = 1; i <= segments; i++)
        {
            const float angle = (GameConfig::HiveConfig::TWO_PI * i) / segments;
            const float x = mx + std::cos(angle) * r;
            const float y = my + std::sin(angle) * r;
            App::DrawLine(prevX, prevY, x, y, 1.0f, 0.95f, 0.20f);
            prevX = x;
            prevY = y;
        }
    }

    App::Print(80, 365, "3 HIVES DETECTED", 1.0f, 0.55f, 0.10f, GLUT_BITMAP_HELVETICA_10);

    // ========== MISSION BRIEFING - BETTER CENTERED ==========
    const float panelX = 370.0f;
    const float panelY = 475.0f;

    App::DrawLine(panelX - 5, panelY - 5, panelX + 295, panelY - 5, 0.70f, 0.90f, 1.00f);
    App::DrawLine(panelX + 295, panelY - 5, panelX + 295, panelY + 130, 0.70f, 0.90f, 1.00f);
    App::DrawLine(panelX + 295, panelY + 130, panelX - 5, panelY + 130, 0.70f, 0.90f, 1.00f);
    App::DrawLine(panelX - 5, panelY + 130, panelX - 5, panelY - 5, 0.70f, 0.90f, 1.00f);

    App::Print(415, 580, "MISSION OBJECTIVE", 1.0f, 0.55f, 0.10f);
    App::Print(380, 567, "- Eliminate all hive structures", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);
    App::Print(380, 547, "- Survive the swarm", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);
    App::Print(380, 527, "- Utilize tactical abilities", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);
    App::Print(380, 502, "Enemies: 100,000 bugs", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);
    App::Print(435, 482, "STATUS: READY", 0.10f, 1.00f, 0.10f, GLUT_BITMAP_HELVETICA_12);

    // ========== START PROMPT ==========
    const float blinkAlpha = std::sin(menuTime * 4.0f) > 0.0f ? 1.0f : 0.3f;
    App::Print(320, 330, ">> PRESS ENTER OR START TO BEGIN <<",
        blinkAlpha, blinkAlpha * 0.95f, blinkAlpha * 0.20f);

    // ========== CONTROLS ==========
    App::Print(80, 310, "KEYBOARD CONTROLS", 0.70f, 0.90f, 1.00f);
    App::Print(80, 285, "Move:   W A S D", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 265, "Pulse:  Space", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 245, "Slash:  F", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 225, "Meteor: E", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 205, "Scale:  Arrows", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 185, "View:   V", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);

    App::Print(700, 310, "CONTROLLER", 0.70f, 0.90f, 1.00f);
    App::Print(700, 285, "Move:   L-Stick", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 265, "Pulse:  B", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 245, "Slash:  X", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 225, "Meteor: Y", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 205, "Scale:  LB/RB", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 185, "View:   DPad Down", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);

    // ========== FOOTER ==========
    App::Print(260, 30, "CLASSIFIED - AUTHORIZATION LEVEL ALPHA REQUIRED",
        0.5f, 0.5f, 0.5f, GLUT_BITMAP_HELVETICA_10);
    
}

void MyGame::RenderPauseOverlay() const
{
    // ========== SCI-FI BACKGROUND (SAME AS MENU) ==========
    static float pauseTime = 0.0f;
    pauseTime += 0.016f;
    if (pauseTime > 1000.0f) pauseTime = 0.0f;

    // Scanlines
    for (int y = 0; y < 768; y += 3)
    {
        const float intensity = (y % 6 == 0) ? 0.03f : 0.015f;
        App::DrawLine(0.0f, static_cast<float>(y), 1024.0f, static_cast<float>(y),
            0.02f + intensity, 0.03f + intensity, 0.05f + intensity);
    }

    // Animated grid
    const float gridSize = 64.0f;
    const float pulse = 0.02f + 0.01f * std::sin(pauseTime * 0.6f);

    for (float x = 0; x < 1024.0f; x += gridSize)
    {
        const bool isMajor = (static_cast<int>(x / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(x, 0.0f, x, 768.0f, alpha, alpha + 0.01f, alpha + 0.03f);
    }

    for (float y = 0; y < 768.0f; y += gridSize)
    {
        const bool isMajor = (static_cast<int>(y / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(0.0f, y, 1024.0f, y, alpha, alpha + 0.01f, alpha + 0.03f);
    }

    // ========== TITLE ==========
    App::Print(437, 702, "PAUSED", 0.1f, 0.1f, 0.1f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(435, 700, "PAUSED", 1.0f, 0.95f, 0.20f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(340, 670, "MISSION SUSPENDED", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    // ========== MISSION STATUS PANEL (CENTERED) ==========
    const float panelX = 365.0f;
    const float panelY = 475.0f;

    App::DrawLine(panelX - 5, panelY - 5, panelX + 295, panelY - 5, 0.70f, 0.90f, 1.00f);
    App::DrawLine(panelX + 295, panelY - 5, panelX + 295, panelY + 130, 0.70f, 0.90f, 1.00f);
    App::DrawLine(panelX + 295, panelY + 130, panelX - 5, panelY + 130, 0.70f, 0.90f, 1.00f);
    App::DrawLine(panelX - 5, panelY + 130, panelX - 5, panelY - 5, 0.70f, 0.90f, 1.00f);

    App::Print(420, 580, "MISSION STATUS", 1.0f, 0.55f, 0.10f);

    // Live player stats
    char hpText[64];
    snprintf(hpText, sizeof(hpText), "- Agent HP: %d / %d", player.GetHealth(), player.GetMaxHealth());
    App::Print(375, 557, hpText, 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    char scaleText[64];
    snprintf(scaleText, sizeof(scaleText), "- Scale: %.2fx", player.GetScale());
    App::Print(375, 537, scaleText, 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    char enemyText[64];
    snprintf(enemyText, sizeof(enemyText), "- Hostiles: %d active", zombies.AliveCount());
    App::Print(375, 517, enemyText, 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    // Hive status
    char hiveText[64];
    snprintf(hiveText, sizeof(hiveText), "- Hives: %d / %d remaining", hives.AliveCount(), hives.TotalCount());
    App::Print(375, 497, hiveText, 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    // Blinking resume prompt
    const float blinkAlpha = std::sin(pauseTime * 4.0f) > 0.0f ? 1.0f : 0.3f;
    App::Print(427, 482, "STATUS: PAUSED",
        blinkAlpha * 0.10f, blinkAlpha * 1.00f, blinkAlpha * 0.10f, GLUT_BITMAP_HELVETICA_12);

    // ========== RESUME PROMPT ==========
    App::Print(305, 410, ">> PRESS ESC OR START TO RESUME <<",
        blinkAlpha, blinkAlpha * 0.95f, blinkAlpha * 0.20f);

    // ========== CONTROLS ==========
    App::Print(220, 340, "KEYBOARD CONTROLS", 0.70f, 0.90f, 1.00f);
    App::Print(220, 315, "Move:   W A S D", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 295, "Pulse:  Space", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 275, "Slash:  F", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 255, "Meteor: E", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 235, "Scale:  Arrows", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 215, "View:   V", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);

    App::Print(560, 340, "CONTROLLER", 0.70f, 0.90f, 1.00f);
    App::Print(560, 315, "Move:   L-Stick", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 295, "Pulse:  B", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 275, "Slash:  X", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 255, "Meteor: Y", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 235, "Scale:  LB/RB", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 215, "View:   DPad Down", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);

    // ========== FOOTER ==========
    App::Print(300, 30, "MISSION PAUSED - AWAITING ORDERS",
        0.5f, 0.5f, 0.5f, GLUT_BITMAP_HELVETICA_10);
}