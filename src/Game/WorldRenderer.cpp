// WorldRenderer.cpp
#include "WorldRenderer.h"

#include "../ContestAPI/app.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

#include "Player.h"
#include "NavGrid.h"
#include "ZombieSystem.h"
#include "CameraSystem.h"
#include "HiveSystem.h"
#include "AttackSystem.h"

// --------------------------------------------
// Public
// --------------------------------------------
void WorldRenderer::RenderFrame(
    const CameraSystem& camera,
    Player& player,
    const NavGrid& nav,
    const ZombieSystem& zombies,
    const HiveSystem& hives,
    const AttackSystem& attacks,
    bool densityView)
{
    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    RenderWorld(offX, offY, player, nav, zombies, hives, attacks, densityView);
}

// --------------------------------------------
// World
// --------------------------------------------
void WorldRenderer::RenderWorld(
    float offX, float offY,
    Player& player,
    const NavGrid& nav,
    const ZombieSystem& zombies,
    const HiveSystem& hives,
    const AttackSystem& attacks,
    bool densityView)
{
    float px, py;
    player.GetWorldPosition(px, py);

    // Background debug
    nav.DebugDrawBlocked(offX, offY);

    // Hives
    hives.Render(offX, offY);

    // Zombies
    RenderZombies2D(offX, offY, zombies, densityView);

    // Player
    player.Render(offX, offY);

    // UI numbers
    const int simCount = zombies.AliveCount();
    const int maxCount = zombies.MaxCount();

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

    RenderUI(
        simCount, maxCount,
        drawn, step,
        densityView,
        player.GetHealth(), player.GetMaxHealth(),
        attacks.GetPulseCooldownMs(),
        attacks.GetSlashCooldownMs(),
        attacks.GetMeteorCooldownMs());
}

// --------------------------------------------
// Zombies 2D
// --------------------------------------------
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
                DrawZombieTri(x, y, size, r, g, b);
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

        DrawZombieTri(x, y, sizeByType[t], r, g, b);
    }
}

// --------------------------------------------
// UI helpers (line fill like HiveSystem)
// --------------------------------------------
void WorldRenderer::DrawRectOutline(float x0, float y0, float x1, float y1, float r, float g, float b) const
{
    App::DrawLine(x0, y0, x1, y0, r, g, b);
    App::DrawLine(x1, y0, x1, y1, r, g, b);
    App::DrawLine(x1, y1, x0, y1, r, g, b);
    App::DrawLine(x0, y1, x0, y0, r, g, b);
}

void WorldRenderer::DrawBarLines(
    float x, float y, float w, float h, float t,
    float bgR, float bgG, float bgB,
    float fillR, float fillG, float fillB) const
{
    t = Clamp01(t);

    // Fill using stacked horizontal lines (works reliably in this API)
    const int lines = (int)std::lround(h);
    const float fillW = w * t;

    for (int i = 0; i < lines; i++)
    {
        const float yy = y + (float)i;

        // background line
        App::DrawLine(x, yy, x + w, yy, bgR, bgG, bgB);

        // fill line
        if (fillW > 0.5f)
            App::DrawLine(x, yy, x + fillW, yy, fillR, fillG, fillB);
    }

    // outline
    DrawRectOutline(x, y, x + w, y + h, 0.95f, 0.95f, 0.95f);
}

// --------------------------------------------
// UI
// --------------------------------------------
void WorldRenderer::RenderUI(
    int simCount, int maxCount,
    int drawnCount, int step,
    bool densityView,
    int hp, int maxHp,
    float pulseCdMs, float slashCdMs, float meteorCdMs)
{
    // Bottom-left anchor (easy to read)
    const float x = 18.0f;
    const float y = 740.0f;

    App::Print((int)x, (int)y, densityView ? "View: Density (V/X)" : "View: Entities (V/X)");

    char bufZ[128];
    std::snprintf(bufZ, sizeof(bufZ), "Zombies: %d/%d  Draw: %d  Step: %d", simCount, maxCount, drawnCount, step);
    App::Print((int)x, (int)(y - 18.0f), bufZ);

    // HP label
    char bufHP[64];
    std::snprintf(bufHP, sizeof(bufHP), "HP %d/%d", hp, maxHp);
    App::Print((int)x, (int)(y - 36.0f), bufHP);

    // HP bar (line fill like hive)
    const float hpT = (maxHp > 0) ? ((float)hp / (float)maxHp) : 0.0f;
    DrawBarLines(
        x + 120.0f, y - 34.0f,
        360.0f, 14.0f,
        hpT,
        0.20f, 0.20f, 0.20f,     // bg
        0.10f, 1.00f, 0.10f      // green fill
    );

    // Cooldown text
    char bufCD[160];
    std::snprintf(bufCD, sizeof(bufCD), "Cooldowns (ms)  Pulse: %.0f  Slash: %.0f  Meteor: %.0f",
        pulseCdMs, slashCdMs, meteorCdMs);
    App::Print((int)x, (int)(y - 54.0f), bufCD);

    // Optional: cooldown bars (also line fill)
    const float pulseT = 1.0f - Clamp01(pulseCdMs / 200.0f);
    const float slashT = 1.0f - Clamp01(slashCdMs / 350.0f);
    const float meteorT = 1.0f - Clamp01(meteorCdMs / 900.0f);

    DrawBarLines(x + 120.0f, y - 70.0f, 110.0f, 10.0f, pulseT, 0.15f, 0.15f, 0.15f, 0.95f, 0.95f, 0.20f);
    DrawBarLines(x + 240.0f, y - 70.0f, 110.0f, 10.0f, slashT, 0.15f, 0.15f, 0.15f, 0.95f, 0.50f, 0.20f);
    DrawBarLines(x + 360.0f, y - 70.0f, 110.0f, 10.0f, meteorT, 0.15f, 0.15f, 0.15f, 0.95f, 0.20f, 0.20f);
}

// --------------------------------------------
// Tri helper (NO face culling, no IsBackFace2D)
// --------------------------------------------
void WorldRenderer::DrawZombieTri(float x, float y, float size, float r, float g, float b)
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

float WorldRenderer::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
