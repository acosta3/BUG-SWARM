#include "WorldRenderer.h"

#include "../ContestAPI/app.h"
#include <cstdio>

// include your real headers here (not forward declares)
#include "Player.h"
#include "NavGrid.h"
#include "ZombieSystem.h"
#include "CameraSystem.h"
#include "IsoProjector.h"

void WorldRenderer::RenderFrame(
    const CameraSystem& camera,
    Player& player,
    const NavGrid& nav,
    const ZombieSystem& zombies,
    bool densityView)
{
    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    RenderWorld(offX, offY, player, nav, zombies, densityView);
}

void WorldRenderer::RenderWorld(float offX, float offY,
    Player& player,
    const NavGrid& nav,
    const ZombieSystem& zombies,
    bool densityView)
{
    // Player is still 2D for now (we will iso-billboard it next)
    player.Render(offX, offY);

    // Maze / obstacles (still 2D debug draw for now)
    nav.DebugDrawBlocked(offX, offY);

    // Zombies (2.5D iso)
    RenderZombiesIso(offX, offY, zombies, densityView);

    // UI counts
    const int simCount = zombies.AliveCount();

    int step = 1;
    int drawn = 0;

    if (densityView)
    {
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

        drawn = (simCount + step - 1) / step;
    }

    RenderUI(simCount, drawn, step, densityView);
}

void WorldRenderer::RenderZombies2D(float offX, float offY, const ZombieSystem& zombies, bool densityView)
{
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

                if (x < 0.0f || x > 1024.0f || y < 0.0f || y > 768.0f)
                    continue;

                const float denom = 20.0f;
                const float intensity = Clamp01((float)n / denom);

                const float r = intensity;
                const float g = 0.2f + 0.8f * (1.0f - 0.5f * intensity);
                const float b = 0.1f;

                const float size = cs * 0.45f;
                DrawZombieTri(x, y, size, r, g, b, false);
            }
        }
        return;
    }

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

        DrawZombieTri(x, y, sizeByType[t], r, g, b, false);
    }
}

void WorldRenderer::RenderZombiesIso(float offX, float offY, const ZombieSystem& zombies, bool densityView)
{
    IsoProjector iso = IsoProjector::FromCameraOffset(offX, offY);

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

                float x, y;
                iso.WorldToScreen(worldX, worldY, 0.0f, x, y);

                if (x < 0.0f || x > 1024.0f || y < 0.0f || y > 768.0f)
                    continue;

                const float denom = 20.0f;
                const float intensity = Clamp01((float)n / denom);

                const float r = intensity;
                const float g = 0.2f + 0.8f * (1.0f - 0.5f * intensity);
                const float b = 0.1f;

                const float size = cs * 0.45f;
                DrawZombieTri(x, y, size, r, g, b, true);
            }
        }
        return;
    }

    const int fullDrawThreshold = 50000;
    const int maxDraw = 10000;

    int step = 1;
    if (count > fullDrawThreshold)
        step = (count + maxDraw - 1) / maxDraw;

    for (int i = 0; i < count; i += step)
    {
        const float wx = zombies.GetX(i);
        const float wy = zombies.GetY(i);

        float x, y;
        iso.WorldToScreen(wx, wy, 0.0f, x, y);

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

        DrawZombieTri(x, y, sizeByType[t], r, g, b, true);
    }
}

void WorldRenderer::RenderUI(int simCount, int drawnCount, int step, bool densityView)
{
    App::Print(20, 60, densityView ? "View: Density (V/X)" : "View: Entities (V/X)");

    char buf2[96];
    std::snprintf(buf2, sizeof(buf2), "Sim: %d  Draw: %d  Step: %d", simCount, drawnCount, step);
    App::Print(20, 40, buf2);
}

void WorldRenderer::DrawZombieTri(float x, float y, float size, float r, float g, float b, bool cull)
{
    const float p1x = x;
    const float p1y = y - size;

    const float p2x = x - size;
    const float p2y = y + size;

    const float p3x = x + size;
    const float p3y = y + size;

    if (cull && IsBackFace2D(p1x, p1y, p2x, p2y, p3x, p3y))
        return;

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

float WorldRenderer::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
