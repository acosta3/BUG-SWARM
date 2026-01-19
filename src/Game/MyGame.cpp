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

int MyGame::GetMaxZombiesForDifficulty() const
{
    switch (selectedDifficulty)
    {
    case DifficultyLevel::Easy:
        return SystemCapacity::MAX_ZOMBIES_EASY;
    case DifficultyLevel::Medium:
        return SystemCapacity::MAX_ZOMBIES_MEDIUM;
    case DifficultyLevel::Hard:
        return SystemCapacity::MAX_ZOMBIES_HARD;
    case DifficultyLevel::Extreme:
        return SystemCapacity::MAX_ZOMBIES_EXTREME;
    default:
        return SystemCapacity::MAX_ZOMBIES_EASY;
    }
}

const char* MyGame::GetDifficultyName() const
{
    switch (selectedDifficulty)
    {
    case DifficultyLevel::Easy:
        return "EASY";
    case DifficultyLevel::Medium:
        return "MEDIUM";
    case DifficultyLevel::Hard:
        return "HARD";
    case DifficultyLevel::Extreme:
        return "EXTREME";
    default:
        return "UNKNOWN";
    }
}

const char* MyGame::GetDifficultyDescription() const
{
    switch (selectedDifficulty)
    {
    case DifficultyLevel::Easy:
        return "20,000 Hostiles - Recommended for beginners";
    case DifficultyLevel::Medium:
        return "50,000 Hostiles - Balanced challenge";
    case DifficultyLevel::Hard:
        return "150,000 Hostiles - Intense combat";
    case DifficultyLevel::Extreme:
        return "200,000 Hostiles - Maximum threat level";
    default:
        return "Unknown difficulty";
    }
}

void MyGame::GetDifficultyColor(float& r, float& g, float& b) const
{
    switch (selectedDifficulty)
    {
    case DifficultyLevel::Easy:
        r = 0.10f;
        g = 1.00f;
        b = 0.10f;
        break;
    case DifficultyLevel::Medium:
        r = 1.00f;
        g = 0.95f;
        b = 0.20f;
        break;
    case DifficultyLevel::Hard:
        r = 1.00f;
        g = 0.55f;
        b = 0.10f;
        break;
    case DifficultyLevel::Extreme:
        r = 1.00f;
        g = 0.15f;
        b = 0.15f;
        break;
    default:
        r = 1.00f;
        g = 1.00f;
        b = 1.00f;
        break;
    }
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

void MyGame::RenderWinOverlay() const
{
    static float winTime = 0.0f;
    winTime += 0.016f;
    if (winTime > 1000.0f) winTime = 0.0f;

    // Semi-transparent dark overlay
    for (int y = 0; y < 768; y += 2)
    {
        const float alpha = 0.05f + 0.02f * std::sin(winTime * 0.3f + y * 0.01f);
        App::DrawLine(0.0f, static_cast<float>(y), 1024.0f, static_cast<float>(y),
            alpha, alpha * 0.8f, alpha * 1.2f);
    }

    // Animated grid background
    const float gridSize = 80.0f;
    const float gridPulse = 0.03f + 0.02f * std::sin(winTime * 0.5f);

    for (float x = 0.0f; x < 1024.0f; x += gridSize)
    {
        const bool isMajor = (static_cast<int>(x / gridSize) % 3 == 0);
        const float alpha = isMajor ? (0.12f + gridPulse) : 0.06f;
        App::DrawLine(x, 100.0f, x, 668.0f, alpha, alpha * 1.1f, alpha * 1.3f);
    }

    for (float y = 100.0f; y < 668.0f; y += gridSize)
    {
        const bool isMajor = (static_cast<int>(y / gridSize) % 3 == 0);
        const float alpha = isMajor ? (0.12f + gridPulse) : 0.06f;
        App::DrawLine(0.0f, y, 1024.0f, y, alpha, alpha * 1.1f, alpha * 1.3f);
    }

    // Main title with enhanced glow effect
    const float titlePulse = 0.7f + 0.3f * std::sin(winTime * 1.5f);

    // Outer glow layers
    for (int offset = 8; offset > 0; offset -= 2)
    {
        const float glowAlpha = (1.0f - offset / 8.0f) * 0.15f * titlePulse;
        App::Print(312 - offset, 552 - offset, "MISSION COMPLETE",
            0.1f * glowAlpha, 1.0f * glowAlpha, 0.1f * glowAlpha, GLUT_BITMAP_TIMES_ROMAN_24);
        App::Print(312 + offset, 552 + offset, "MISSION COMPLETE",
            0.1f * glowAlpha, 1.0f * glowAlpha, 0.1f * glowAlpha, GLUT_BITMAP_TIMES_ROMAN_24);
    }

    // Shadow
    App::Print(314, 554, "MISSION COMPLETE", 0.0f, 0.0f, 0.0f, GLUT_BITMAP_TIMES_ROMAN_24);

    // Main text with animated color
    const float colorShift = std::sin(winTime * 2.0f) * 0.15f;
    App::Print(312, 552, "MISSION COMPLETE",
        0.1f + colorShift, 1.0f, 0.1f + colorShift * 0.5f, GLUT_BITMAP_TIMES_ROMAN_24);

    // Decorative corner brackets
    const float bracketAlpha = 0.7f + 0.3f * std::sin(winTime * 3.0f);
    const float bracketSize = 30.0f;

    // Top-left corner
    App::DrawLine(280, 520, 280 + bracketSize, 520, bracketAlpha, bracketAlpha * 0.95f, 0.2f * bracketAlpha);
    App::DrawLine(280, 520, 280, 520 + bracketSize, bracketAlpha, bracketAlpha * 0.95f, 0.2f * bracketAlpha);

    // Top-right corner
    App::DrawLine(744 - bracketSize, 520, 744, 520, bracketAlpha, bracketAlpha * 0.95f, 0.2f * bracketAlpha);
    App::DrawLine(744, 520, 744, 520 + bracketSize, bracketAlpha, bracketAlpha * 0.95f, 0.2f * bracketAlpha);

    // Mission summary panel with glowing border
    const float panelX = 262.0f;
    const float panelY = 360.0f;
    const float panelW = 500.0f;
    const float panelH = 140.0f;

    // Panel glow
    const float borderGlow = 0.5f + 0.5f * std::sin(winTime * 2.5f);
    for (int i = 0; i < 3; i++)
    {
        const float offset = static_cast<float>(i) * 2.0f;
        const float glowAlpha = (1.0f - i / 3.0f) * 0.3f * borderGlow;
        App::DrawLine(panelX - offset, panelY - offset, panelX + panelW + offset, panelY - offset,
            0.3f * glowAlpha, 0.7f * glowAlpha, 1.0f * glowAlpha);
        App::DrawLine(panelX + panelW + offset, panelY - offset, panelX + panelW + offset, panelY + panelH + offset,
            0.3f * glowAlpha, 0.7f * glowAlpha, 1.0f * glowAlpha);
        App::DrawLine(panelX + panelW + offset, panelY + panelH + offset, panelX - offset, panelY + panelH + offset,
            0.3f * glowAlpha, 0.7f * glowAlpha, 1.0f * glowAlpha);
        App::DrawLine(panelX - offset, panelY + panelH + offset, panelX - offset, panelY - offset,
            0.3f * glowAlpha, 0.7f * glowAlpha, 1.0f * glowAlpha);
    }

    // Panel header
    App::Print(420, 485, "MISSION SUMMARY", 1.0f, 0.65f, 0.15f);

    // Victory stats with icons
    App::Print(277, 455, "[STATUS]", 0.3f, 0.7f, 0.9f, GLUT_BITMAP_HELVETICA_12);
    App::Print(350, 455, "ALL HIVES DESTROYED", 0.15f, 1.0f, 0.15f, GLUT_BITMAP_HELVETICA_12);

    char finalHP[64];
    snprintf(finalHP, sizeof(finalHP), "[AGENT]  HP: %d / %d", player.GetHealth(), player.GetMaxHealth());
    App::Print(277, 435, finalHP, 0.3f, 0.7f, 0.9f, GLUT_BITMAP_HELVETICA_12);

    char enemiesText[64];
    const int maxZombies = GetMaxZombiesForDifficulty();
    snprintf(enemiesText, sizeof(enemiesText), "[KILLS]  HOSTILES: %d",
        maxZombies - zombies.AliveCount());
    App::Print(277, 415, enemiesText, 0.3f, 0.7f, 0.9f, GLUT_BITMAP_HELVETICA_12);

    // Threat status with pulsing effect
    const float threatPulse = 0.8f + 0.2f * std::sin(winTime * 4.0f);
    App::Print(320, 385, ">> THREAT NEUTRALIZED <<",
        0.15f * threatPulse, 1.0f * threatPulse, 0.15f * threatPulse, GLUT_BITMAP_HELVETICA_12);

    // Congratulations panel
    const float congratsX = 212.0f;
    const float congratsY = 240.0f;
    const float congratsW = 600.0f;
    const float congratsH = 90.0f;

    // Animated border
    const float congratsBorderPulse = 0.6f + 0.4f * std::sin(winTime * 2.0f);
    App::DrawLine(congratsX - 3, congratsY - 3, congratsX + congratsW + 3, congratsY - 3,
        0.15f * congratsBorderPulse, 1.0f * congratsBorderPulse, 0.15f * congratsBorderPulse);
    App::DrawLine(congratsX + congratsW + 3, congratsY - 3, congratsX + congratsW + 3, congratsY + congratsH + 3,
        0.15f * congratsBorderPulse, 1.0f * congratsBorderPulse, 0.15f * congratsBorderPulse);
    App::DrawLine(congratsX + congratsW + 3, congratsY + congratsH + 3, congratsX - 3, congratsY + congratsH + 3,
        0.15f * congratsBorderPulse, 1.0f * congratsBorderPulse, 0.15f * congratsBorderPulse);
    App::DrawLine(congratsX - 3, congratsY + congratsH + 3, congratsX - 3, congratsY - 3,
        0.15f * congratsBorderPulse, 1.0f * congratsBorderPulse, 0.15f * congratsBorderPulse);

    // Main congratulations text
    App::Print(332, 315, "EXCELLENT WORK, AGENT!", 1.0f, 0.98f, 0.3f);
    App::Print(245, 285, "The swarm has been eradicated successfully", 0.7f, 0.9f, 1.0f, GLUT_BITMAP_HELVETICA_12);
    App::Print(265, 265, "All hive structures have been neutralized", 0.7f, 0.9f, 1.0f, GLUT_BITMAP_HELVETICA_12);

    // Continue prompt with blinking
    const float promptBlink = std::sin(winTime * 5.0f) > 0.0f ? 1.0f : 0.4f;
    App::Print(240, 200, ">> PRESS ENTER OR START TO CONTINUE <<",
        promptBlink, promptBlink * 0.97f, promptBlink * 0.3f);

    // Animated victory particles
    for (int i = 0; i < 12; i++)
    {
        const float particleTime = winTime + i * 0.3f;
        const float particleAlpha = (std::sin(particleTime * 2.5f) + 1.0f) * 0.5f;
        const float particleX = 150.0f + i * 70.0f + std::sin(particleTime * 1.5f) * 15.0f;
        const float particleY = 560.0f + std::sin(particleTime * 2.0f + i) * 25.0f;
        const float size = 2.0f + particleAlpha * 2.0f;

        // Star shape
        App::DrawLine(particleX - size, particleY, particleX + size, particleY,
            1.0f * particleAlpha, 0.98f * particleAlpha, 0.3f * particleAlpha);
        App::DrawLine(particleX, particleY - size, particleX, particleY + size,
            1.0f * particleAlpha, 0.98f * particleAlpha, 0.3f * particleAlpha);
        App::DrawLine(particleX - size * 0.7f, particleY - size * 0.7f, particleX + size * 0.7f, particleY + size * 0.7f,
            1.0f * particleAlpha, 0.98f * particleAlpha, 0.3f * particleAlpha);
        App::DrawLine(particleX + size * 0.7f, particleY - size * 0.7f, particleX - size * 0.7f, particleY + size * 0.7f,
            1.0f * particleAlpha, 0.98f * particleAlpha, 0.3f * particleAlpha);
    }

    // Bottom status bar
    const float statusBarAlpha = 0.3f + 0.1f * std::sin(winTime * 1.0f);
    App::Print(230, 40, "MISSION ACCOMPLISHED - AUTHORIZATION: ALPHA CLEARANCE",
        statusBarAlpha, statusBarAlpha * 0.9f, statusBarAlpha * 0.5f, GLUT_BITMAP_HELVETICA_10);

    // Scan lines for extra tech feel
    for (int i = 0; i < 5; i++)
    {
        const float scanY = 100.0f + std::fmod(winTime * 80.0f + i * 120.0f, 568.0f);
        const float scanAlpha = 0.08f;
        App::DrawLine(0.0f, scanY, 1024.0f, scanY, scanAlpha, scanAlpha * 1.2f, scanAlpha * 1.5f);
    }
}

void MyGame::RenderMenu() const
{

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

    for (float x = 0.0f; x < 1024.0f; x += gridSize)
    {
        const bool isMajor = (static_cast<int>(x / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(x, 0.0f, x, 768.0f, alpha, alpha + 0.01f, alpha + 0.03f);
    }

    for (float y = 0.0f; y < 768.0f; y += gridSize)
    {
        const bool isMajor = (static_cast<int>(y / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(0.0f, y, 1024.0f, y, alpha, alpha + 0.01f, alpha + 0.03f);
    }


    App::Print(387, 702, "BUG SWARM", 0.1f, 0.1f, 0.1f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(385, 700, "BUG SWARM", 1.0f, 0.95f, 0.20f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(280, 670, "TACTICAL ERADICATION PROTOCOL", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);


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

    // ========== DIFFICULTY SELECTION PANEL ==========
    const float diffPanelX = 270.0f;
    const float diffPanelY = 440.0f;
    const float diffPanelW = 80.0f;

    App::Print(380, 620, "SELECT DIFFICULTY", 0.70f, 0.90f, 1.00f);
    App::Print(380, 600, "Use UP/DOWN or D-Pad", 0.50f, 0.60f, 0.70f, GLUT_BITMAP_HELVETICA_10);

    // Difficulty options with selection indicator
    const float optionY = 570.0f;
    const float optionSpacing = 20.0f;

    for (int i = 0; i < 4; i++)
    {
        const DifficultyLevel level = static_cast<DifficultyLevel>(i);
        const bool isSelected = (level == selectedDifficulty);
        const float yPos = optionY - i * optionSpacing;

        // Selection indicator
        if (isSelected)
        {
            const float indicatorPulse = 0.7f + 0.3f * std::sin(menuTime * 5.0f);
            App::Print(375, yPos, ">>",
                1.0f * indicatorPulse, 0.95f * indicatorPulse, 0.20f * indicatorPulse);
        }

        // Difficulty name and description
        float r, g, b;
        const char* name = "";
        const char* desc = "";

        switch (level)
        {
        case DifficultyLevel::Easy:
            r = 0.10f; g = 1.00f; b = 0.10f;
            name = "EASY";
            desc = "20,000 Hostiles";
            break;
        case DifficultyLevel::Medium:
            r = 1.00f; g = 0.95f; b = 0.20f;
            name = "MEDIUM";
            desc = "50,000 Hostiles";
            break;
        case DifficultyLevel::Hard:
            r = 1.00f; g = 0.55f; b = 0.10f;
            name = "HARD";
            desc = "150,000 Hostiles";
            break;
        case DifficultyLevel::Extreme:
            r = 1.00f; g = 0.15f; b = 0.15f;
            name = "EXTREME";
            desc = "200,000 Hostiles";
            break;
        }

        const float alpha = isSelected ? 1.0f : 0.5f;
        App::Print(400, yPos, name, r * alpha, g * alpha, b * alpha);
        App::Print(505, yPos, desc, 0.60f * alpha, 0.70f * alpha, 0.80f * alpha, GLUT_BITMAP_HELVETICA_10);
    }

    // ========== MISSION OBJECTIVE PANEL ==========
    

    App::Print(415, 295, "MISSION OBJECTIVE", 1.0f, 0.55f, 0.10f);
    App::Print(380, 272, "- Eliminate all hive structures", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);
    App::Print(380, 242, "- Survive the swarm", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);
    App::Print(380, 212, "- Utilize tactical abilities", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    // Display selected difficulty's enemy count
    char enemyCountText[64];
    snprintf(enemyCountText, sizeof(enemyCountText), "Enemies: %d bugs", GetMaxZombiesForDifficulty());

    float diffR, diffG, diffB;
    GetDifficultyColor(diffR, diffG, diffB);
    App::Print(435, 380, enemyCountText, diffR, diffG, diffB, GLUT_BITMAP_HELVETICA_12);

    App::Print(435, 347, "STATUS: READY", 0.10f, 1.00f, 0.10f, GLUT_BITMAP_HELVETICA_12);


    const float blinkAlpha = std::sin(menuTime * 4.0f) > 0.0f ? 1.0f : 0.3f;
    App::Print(320, 130, ">> PRESS ENTER OR START TO BEGIN <<",
        blinkAlpha, blinkAlpha * 0.95f, blinkAlpha * 0.20f);


    App::Print(80, 310, "KEYBOARD CONTROLS", 0.70f, 0.90f, 1.00f);
    App::Print(80, 285, "Move:   W A S D", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 265, "Pulse:  Space", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 245, "Slash:  F", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 225, "Meteor: E", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 205, "Scale:  Hold Left = Small", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 190, "        Hold Right = Big", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 170, "View:   V", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);

    // Scale mechanics explanation
    App::Print(80, 140, "SCALE MECHANICS:", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 120, "Small: +Speed -Health -Damage", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 105, "Large: -Speed +Health +Damage", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);

    App::Print(700, 310, "CONTROLLER", 0.70f, 0.90f, 1.00f);
    App::Print(700, 285, "Move:   L-Stick", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 265, "Pulse:  B", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 245, "Slash:  X", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 225, "Meteor: Y", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 205, "Scale:  Hold LB = Small", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 190, "        Hold RB = Big", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 170, "View:   DPad Down", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);


    App::Print(260, 30, "CLASSIFIED - AUTHORIZATION LEVEL ALPHA REQUIRED",
        0.5f, 0.5f, 0.5f, GLUT_BITMAP_HELVETICA_10);

}

void MyGame::RenderPauseOverlay() const
{

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

    for (float x = 0.0f; x < 1024.0f; x += gridSize)
    {
        const bool isMajor = (static_cast<int>(x / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(x, 0.0f, x, 768.0f, alpha, alpha + 0.01f, alpha + 0.03f);
    }

    for (float y = 0.0f; y < 768.0f; y += gridSize)
    {
        const bool isMajor = (static_cast<int>(y / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(0.0f, y, 1024.0f, y, alpha, alpha + 0.01f, alpha + 0.03f);
    }


    App::Print(437, 702, "PAUSED", 0.1f, 0.1f, 0.1f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(435, 700, "PAUSED", 1.0f, 0.95f, 0.20f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(340, 670, "MISSION SUSPENDED", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);


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

    App::Print(305, 410, ">> PRESS ESC OR START TO RESUME <<",
        blinkAlpha, blinkAlpha * 0.95f, blinkAlpha * 0.20f);

    App::Print(220, 340, "KEYBOARD CONTROLS", 0.70f, 0.90f, 1.00f);
    App::Print(220, 315, "Move:   W A S D", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 295, "Pulse:  Space", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 275, "Slash:  F", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 255, "Meteor: E", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 235, "Scale:  Left = Small", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 220, "        Right = Big", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 200, "View:   V", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);

    // Scale mechanics explanation
    App::Print(220, 170, "SCALE MECHANICS:", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 150, "Small: +Speed -Health -Damage", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 135, "Large: -Speed +Health +Damage", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);

    App::Print(560, 340, "CONTROLLER", 0.70f, 0.90f, 1.00f);
    App::Print(560, 315, "Move:   L-Stick", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 295, "Pulse:  B", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 275, "Slash:  X", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 255, "Meteor: Y", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 235, "Scale:  LB = Small", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 220, "        RB = Big", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 200, "View:   DPad Down", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);


    App::Print(300, 30, "MISSION PAUSED - AWAITING ORDERS",
        0.5f, 0.5f, 0.5f, GLUT_BITMAP_HELVETICA_10);
}