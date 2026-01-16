#include "MyGame.h"
#include "../ContestAPI/app.h"
#include <cstdio>

void MyGame::Init()
{
    player.Init();
    camera.Init(1024.0f, 768.0f); // matches APP_VIRTUAL_WIDTH/HEIGHT

    // IMPORTANT: init pool before spawn
    zombies.Init(50000);

    // Spawn around the player's start position so it is always on-screen
    float px, py;
    player.GetWorldPosition(px, py);
    zombies.Spawn(20000, px, py);

    // Optional: set initial camera target so first frame is centered
    camera.Follow(px, py);
}

void MyGame::Update(float deltaTime)
{
    input.Update(deltaTime);
    const InputState& in = input.GetState();

    player.SetMoveInput(in.moveX, in.moveY);
    player.SetStopAnimPressed(in.stopAnimPressed);

    player.Update(deltaTime);

    float px, py;
    player.GetWorldPosition(px, py);

    camera.Follow(px, py);
    camera.Update(deltaTime);



    if (in.toggleViewPressed) 
    {
        densityView = !densityView;
    }
      
    zombies.Update(deltaTime, px, py);

    
}

void MyGame::Render()
{
    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    // World
    player.Render(offX, offY);

    // Precomputed render data by type
    static const float sizeByType[4] = { 3.0f, 3.5f, 5.0f, 7.0f };
    static const float rByType[4] = { 0.2f, 1.0f, 0.2f, 0.8f };
    static const float gByType[4] = { 1.0f, 0.2f, 0.4f, 0.2f };
    static const float bByType[4] = { 0.2f, 0.2f, 1.0f, 1.0f };

    const int count = zombies.AliveCount();

    const int fullDrawThreshold = 10000;
    const int maxDraw = 1000; // only used when count > threshold

    int step = 1;
    int drawn = 0;

    if (densityView)
    {
        const int gw = zombies.GetGridW();
        const int gh = zombies.GetGridH();
        const float cs = zombies.GetCellSize();
        const float minX = zombies.GetWorldMinX();
        const float minY = zombies.GetWorldMinY();

        // We only draw cells that are on-screen
        // Each occupied cell becomes ONE triangle (or you can draw 2 for a quad)
        for (int cy = 0; cy < gh; cy++)
        {
            for (int cx = 0; cx < gw; cx++)
            {
                const int idx = cy * gw + cx;
                const int n = zombies.GetCellCountAt(idx);
                if (n <= 0) continue;

                // Cell world center
                float worldX = minX + (cx + 0.5f) * cs;
                float worldY = minY + (cy + 0.5f) * cs;

                float x = worldX - offX;
                float y = worldY - offY;

                // Cull off-screen
                if (x < 0.0f || x > 1024.0f || y < 0.0f || y > 768.0f)
                    continue;

                // Intensity: map count -> 0..1
                // Tune denom depending on how dense your swarm gets
                const float denom = 20.0f; // 20 zombies in cell = full bright
                float intensity = Clamp01((float)n / denom);

                // Color: green to yellow to red style (simple)
                float r = intensity;
                float g = 0.2f + 0.8f * (1.0f - 0.5f * intensity);
                float b = 0.1f;

                // Size: proportional to cell size (visual)
                float size = cs * 0.45f;

                DrawZombieTri(x, y, size, r, g, b);
				drawn++;
            }
        }
    }
    else
    {
        // Your entity view loop (with maxDraw cap) goes here
        if (count > fullDrawThreshold)
        {
            // sample so we draw about maxDraw
            step = (count + maxDraw - 1) / maxDraw;
        }

        
        for (int i = 0; i < count; i += step)
        {
            float x = zombies.GetX(i) - offX;
            float y = zombies.GetY(i) - offY;

            if (x < 0.0f || x > 1024.0f || y < 0.0f || y > 768.0f)
                continue;

            int t = zombies.GetType(i);
            DrawZombieTri(x, y, sizeByType[t], rByType[t], gByType[t], bByType[t]);
            drawn++;
        }
    }

   

    // UI: show both sim and draw counts

    App::Print(20, 60, densityView ? "View: Density (V/X)" : "View: Entities (V/X)");

    char buf2[96];
    std::snprintf(buf2, sizeof(buf2), "Sim: %d  Draw: %d  Step: %d", count, drawn, step);
    App::Print(20, 40, buf2);
}


void MyGame::Shutdown()
{
    // Nothing required right now:
    // - Player owns sprite via unique_ptr
    // - ZombieSystem uses vectors managed by RAII
    // - InputSystem has no heap allocations
}

void MyGame::DrawZombieTri(float x, float y, float size, float r, float g, float b)
{
    float p1x = x;
    float p1y = y - size;

    float p2x = x - size;
    float p2y = y + size;

    float p3x = x + size;
    float p3y = y + size;

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
