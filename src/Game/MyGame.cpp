#include "MyGame.h"
#include "../ContestAPI/app.h"
#include <cstdio>
#include <cmath>


static inline void ScreenDirToWorldDir(float sxDir, float syDir, float kx, float ky, float& outWx, float& outWy)
{
    // Inverse of:
    // dsx = (dx - dy) * kx
    // dsy = (dx + dy) * ky
    const float inv2kx = (kx != 0.0f) ? (1.0f / (2.0f * kx)) : 0.0f;
    const float inv2ky = (ky != 0.0f) ? (1.0f / (2.0f * ky)) : 0.0f;

    float wx = sxDir * inv2kx + syDir * inv2ky;
    float wy = -sxDir * inv2kx + syDir * inv2ky;

    // normalize so diagonals are not faster
    const float len2 = wx * wx + wy * wy;
    if (len2 > 0.0001f)
    {
        const float invLen = 1.0f / std::sqrt(len2);
        wx *= invLen;
        wy *= invLen;
    }

    outWx = wx;
    outWy = wy;
}


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
   
    renderer.RenderFrame(camera, player, nav, zombies, densityView);
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
   
    nav.AddObstacleRect(-220.0f, -270.0f, -180.0f, -230.0f);
    
    nav.AddObstacleRect(0.0f, -260.0f, 40.0f, -220.0f);

    // Row 2
    nav.AddObstacleCircle(-380.0f, -120.0f, 22.0f);
    nav.AddObstacleRect(-280.0f, -140.0f, -240.0f, -100.0f);
  
    nav.AddObstacleRect(-60.0f, -140.0f, -20.0f, -100.0f);
    

    // Row 3
    nav.AddObstacleRect(-420.0f, 0.0f, -380.0f, 40.0f);
   
    nav.AddObstacleRect(-220.0f, -10.0f, -180.0f, 30.0f);
 
    nav.AddObstacleRect(0.0f, 0.0f, 40.0f, 40.0f);

    // Row 4
    nav.AddObstacleRect(-280.0f, 120.0f, -240.0f, 160.0f);
 
    nav.AddObstacleRect(-60.0f, 120.0f, -20.0f, 160.0f);
   
    // Bar
    nav.AddObstacleRect(-300.0f, -200.0f, 100.0f, -175.0f);

	nav.ClearObstacles(); // issue is how my rectangle are colliding 

    nav.AddObstacleRect(-420.0f, -260.0f, -380.0f, -220.0f);

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

    // screen input (what the player feels as left/right/up/down)
    const float sx = in.moveX;
    const float sy = in.moveY;

    // convert to world-space movement so "left" feels left in iso
    const float wx = 0.5f * (sx + sy);
    const float wy = 0.5f * (sy - sx);

    player.SetMoveInput(wx, wy);

    // NEW: keep view input for animations/facing
    player.SetViewInput(sx, sy);

    player.Update(deltaTimeMs);
    player.ApplyScaleInput(in.scaleUpHeld, in.scaleDownHeld, deltaTimeMs);

    // last aim should match world movement (for attacks)
    const float len2 = wx * wx + wy * wy;
    if (len2 > 0.0001f)
    {
        lastAimX = wx;
        lastAimY = wy;
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








