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
}

void MyGame::Update(float deltaTimeMs)
{
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

    float px, py;
    player.GetWorldPosition(px, py);

    UpdateAttacks(deltaTimeMs);
    UpdateNavFlowField(px, py);
    UpdateCamera(deltaTimeMs, px, py);
    UpdateZombies(deltaTimeMs, px, py);
}

void MyGame::Render()
{
    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    RenderWorld(offX, offY);
}

void MyGame::Shutdown()
{
    // Nothing required right now:
    // - Player owns sprite via unique_ptr
    // - ZombieSystem uses vectors managed by RAII
    // - InputSystem has no heap allocations
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
    // Row 1
    nav.AddObstacleRect(-420.0f, -260.0f, -380.0f, -220.0f);
    nav.AddObstacleCircle(-300.0f, -240.0f, 22.0f);
    nav.AddObstacleRect(-220.0f, -270.0f, -180.0f, -230.0f);
    nav.AddObstacleCircle(-100.0f, -240.0f, 22.0f);
    nav.AddObstacleRect(0.0f, -260.0f, 40.0f, -220.0f);

    // Row 2
    nav.AddObstacleCircle(-380.0f, -120.0f, 22.0f);
    nav.AddObstacleRect(-280.0f, -140.0f, -240.0f, -100.0f);
    nav.AddObstacleCircle(-160.0f, -120.0f, 22.0f);
    nav.AddObstacleRect(-60.0f, -140.0f, -20.0f, -100.0f);
    nav.AddObstacleCircle(80.0f, -120.0f, 22.0f);

    // Row 3
    nav.AddObstacleRect(-420.0f, 0.0f, -380.0f, 40.0f);
    nav.AddObstacleCircle(-300.0f, 20.0f, 22.0f);
    nav.AddObstacleRect(-220.0f, -10.0f, -180.0f, 30.0f);
    nav.AddObstacleCircle(-100.0f, 20.0f, 22.0f);
    nav.AddObstacleRect(0.0f, 0.0f, 40.0f, 40.0f);

    // Row 4
    nav.AddObstacleCircle(-380.0f, 140.0f, 22.0f);
    nav.AddObstacleRect(-280.0f, 120.0f, -240.0f, 160.0f);
    nav.AddObstacleCircle(-160.0f, 140.0f, 22.0f);
    nav.AddObstacleRect(-60.0f, 120.0f, -20.0f, 160.0f);
    nav.AddObstacleCircle(80.0f, 140.0f, 22.0f);

    // Bar
    nav.AddObstacleRect(-300.0f, -200.0f, 100.0f, -175.0f);
}

void MyGame::InitSystems()
{
    float px, py;
    player.GetWorldPosition(px, py);

    // Zombies AFTER nav (so zombies can copy world bounds from nav)
    zombies.Init(50000, nav);
    zombies.Spawn(10'000, px, py);

    // Attacks last (depends on zombies + camera)
    attacks.Init();

    // Persistent state
    lastAimX = 0.0f;
    lastAimY = 1.0f;
    lastTargetCell = -1;
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
    player.ApplyScaleInput(in.scaleUpHeld, in.scaleDownHeld, deltaTimeMs);

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

    attacks.Process(a, px, py, zombies, camera);
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

// ------------------------------------------------------------
// Render helpers
// ------------------------------------------------------------
void MyGame::RenderWorld(float offX, float offY)
{
    player.Render(offX, offY);

    // Draw nav obstacles
    nav.DebugDrawBlocked(offX, offY);

    // Draw zombies
    RenderZombies(offX, offY);

    // UI
    const int simCount = zombies.AliveCount();

    // RenderZombies computes drawn + step, but UI wants them.
    // Easiest is to recompute here with same rules, or store members.
    // We'll recompute tiny values for UI by mirroring the logic.
    int step = 1;
    int drawn = 0;

    if (densityView)
    {
        // Density mode: drawn is how many occupied cells we rendered
        const int gw = zombies.GetGridW();
        const int gh = zombies.GetGridH();
        for (int cy = 0; cy < gh; cy++)
            for (int cx = 0; cx < gw; cx++)
                if (zombies.GetCellCountAt(cy * gw + cx) > 0) drawn++;
    }
    else
    {
        const int fullDrawThreshold = 50000;
        const int maxDraw = 10000;
        if (simCount > fullDrawThreshold)
            step = (simCount + maxDraw - 1) / maxDraw;

        // drawn in entity mode is approximate without culling; keep UI honest-ish:
        drawn = (simCount + step - 1) / step;
    }

    RenderUI(simCount, drawn, step);
}

void MyGame::RenderZombies(float offX, float offY)
{
    // Precomputed render data by type
    static const float sizeByType[4] = { 3.0f, 3.5f, 5.0f, 7.0f };
    static const float rByType[4] = { 0.2f, 1.0f, 0.2f, 0.8f };
    static const float gByType[4] = { 1.0f, 0.2f, 0.4f, 0.2f };
    static const float bByType[4] = { 0.2f, 0.2f, 1.0f, 1.0f };

    const int count = zombies.AliveCount();

    if (densityView)
    {
        const int gw = zombies.GetGridW();
        const int gh = zombies.GetGridH();
        const float cs = zombies.GetCellSize();
        const float minX = zombies.GetWorldMinX();
        const float minY = zombies.GetWorldMinY();

        for (int cy = 0; cy < gh; cy++)
        {
            for (int cx = 0; cx < gw; cx++)
            {
                const int idx = cy * gw + cx;
                const int n = zombies.GetCellCountAt(idx);
                if (n <= 0) continue;

                const float worldX = minX + (cx + 0.5f) * cs;
                const float worldY = minY + (cy + 0.5f) * cs;

                const float x = worldX - offX;
                const float y = worldY - offY;

                // Cull off-screen
                if (x < 0.0f || x > 1024.0f || y < 0.0f || y > 768.0f)
                    continue;

                const float denom = 20.0f;
                const float intensity = Clamp01((float)n / denom);

                const float r = intensity;
                const float g = 0.2f + 0.8f * (1.0f - 0.5f * intensity);
                const float b = 0.1f;

                const float size = cs * 0.45f;
                DrawZombieTri(x, y, size, r, g, b);
            }
        }
        return;
    }

    // Entity view
    const int fullDrawThreshold = 50000;
    const int maxDraw = 10000;

    int step = 1;
    if (count > fullDrawThreshold)
        step = (count + maxDraw - 1) / maxDraw;

    for (int i = 0; i < count; i += step)
    {
        float x = zombies.GetX(i) - offX;
        float y = zombies.GetY(i) - offY;

        if (x < 0.0f || x > 1024.0f || y < 0.0f || y > 768.0f)
            continue;

        const int t = zombies.GetType(i);

        float r = rByType[t];
        float g = gByType[t];
        float b = bByType[t];

        if (zombies.IsFeared(i))
        {
            r = 1.0f;
            if (g < 0.85f) g = 0.85f;
            b *= 0.25f;
        }

        // IMPORTANT: pass the modified (r,g,b), not the base arrays
        DrawZombieTri(x, y, sizeByType[t], r, g, b);
    }
}

void MyGame::RenderUI(int simCount, int drawnCount, int step)
{
    App::Print(20, 60, densityView ? "View: Density (V/X)" : "View: Entities (V/X)");

    char buf2[96];
    std::snprintf(buf2, sizeof(buf2), "Sim: %d  Draw: %d  Step: %d", simCount, drawnCount, step);
    App::Print(20, 40, buf2);

    char bufHP[96];
    std::snprintf(bufHP, sizeof(bufHP), "HP: %d/%d", player.GetHealth(), player.GetMaxHealth());
    App::Print(500, 40, bufHP);
}

// ------------------------------------------------------------
// Existing helpers you already had
// ------------------------------------------------------------
void MyGame::DrawZombieTri(float x, float y, float size, float r, float g, float b)
{
    const float p1x = x;
    const float p1y = y - size;

    const float p2x = x - size;
    const float p2y = y + size;

    const float p3x = x + size;
    const float p3y = y + size;

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

float MyGame::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
