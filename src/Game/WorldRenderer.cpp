// WorldRenderer.cpp
#include "WorldRenderer.h"

#include "../ContestAPI/app.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "AttackSystem.h"
#include "CameraSystem.h"
#include "HiveSystem.h"
#include "NavGrid.h"
#include "Player.h"
#include "ZombieSystem.h"

static constexpr float kScreenW = 1024.0f;
static constexpr float kScreenH = 768.0f;

static constexpr int kFullDrawThreshold = 50000;
static constexpr int kMaxDraw = 10000;

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
    float dtMs,
    bool densityView)
{
    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    RenderWorld(offX, offY, player, nav, zombies, hives, attacks, dtMs, densityView);
}

void WorldRenderer::NotifyKills(int kills)
{
    if (kills <= 0) return;

    if (killPopupTimeMs > 0.0f) killPopupCount += kills;
    else killPopupCount = kills;

    killPopupTimeMs = 900.0f;
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
    float dtMs,
    bool densityView)
{
    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    const float playerScreenX = px - offX;
    const float playerScreenY = py - offY;

    nav.DebugDrawBlocked(offX, offY);
    hives.Render(offX, offY);
    RenderZombies2D(offX, offY, zombies, densityView);

    player.Render(offX, offY);
    attacks.RenderFX(offX, offY);

    RenderKillPopupOverPlayer(playerScreenX, playerScreenY, dtMs);

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
        if (simCount > kFullDrawThreshold)
            step = (simCount + kMaxDraw - 1) / kMaxDraw;

        drawn = (simCount + step - 1) / step;
    }

    const int hAlive = hives.AliveCount();
    const int hTotal = hives.TotalCount();

    RenderUI(
        simCount, maxCount,
        drawn, step,
        densityView,
        player.GetHealth(), player.GetMaxHealth(),
        hAlive, hTotal,
        attacks.GetPulseCooldownMs(),
        attacks.GetSlashCooldownMs(),
        attacks.GetMeteorCooldownMs()
    );
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

                if (x < 0.0f || x > kScreenW || y < 0.0f || y > kScreenH)
                    continue;

                const float intensity = Clamp01((float)n / 20.0f);

                const float r = intensity;
                const float g = 0.2f + 0.8f * (1.0f - 0.5f * intensity);
                const float b = 0.1f;

                DrawZombieTri(x, y, cs * 0.45f, r, g, b);
            }
        }
        return;
    }

    int step = 1;
    if (count > kFullDrawThreshold)
        step = (count + kMaxDraw - 1) / kMaxDraw;

    for (int i = 0; i < count; i += step)
    {
        const float x = zombies.GetX(i) - offX;
        const float y = zombies.GetY(i) - offY;

        if (x < 0.0f || x > kScreenW || y < 0.0f || y > kScreenH)
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
// UI helpers
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

    const int lines = (int)std::lround(h);
    const float fillW = w * t;

    for (int i = 0; i < lines; i++)
    {
        const float yy = y + (float)i;

        App::DrawLine(x, yy, x + w, yy, bgR, bgG, bgB);
        if (fillW > 0.5f)
            App::DrawLine(x, yy, x + fillW, yy, fillR, fillG, fillB);
    }

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
    int hivesAlive, int hivesTotal,
    float pulseCdMs, float slashCdMs, float meteorCdMs)
{
    const float x = 500.0f;
    const float y = 80.0f;

    App::Print((int)x, (int)y, densityView ? "View: Density (V/X)" : "View: Entities (V/X)");

    char bufZ[128];
    std::snprintf(bufZ, sizeof(bufZ), "Zombies: %d/%d  Draw: %d  Step: %d", simCount, maxCount, drawnCount, step);
    App::Print((int)x, (int)(y - 18.0f), bufZ);

    char bufHP[64];
    std::snprintf(bufHP, sizeof(bufHP), "HP %d/%d", hp, maxHp);
    App::Print((int)x, (int)(y - 36.0f), bufHP);

    const float hpT = (maxHp > 0) ? ((float)hp / (float)maxHp) : 0.0f;
    DrawBarLines(
        x + 120.0f, y - 34.0f,
        360.0f, 14.0f,
        hpT,
        0.20f, 0.20f, 0.20f,
        0.10f, 1.00f, 0.10f
    );

    char bufCD[160];
    std::snprintf(
        bufCD, sizeof(bufCD),
        "Cooldowns (ms)  Pulse: %.0f  Slash: %.0f  Meteor: %.0f",
        pulseCdMs, slashCdMs, meteorCdMs
    );
    App::Print((int)x, (int)(y - 54.0f), bufCD);

    const float pulseT = 1.0f - Clamp01(pulseCdMs / 200.0f);
    const float slashT = 1.0f - Clamp01(slashCdMs / 350.0f);
    const float meteorT = 1.0f - Clamp01(meteorCdMs / 900.0f);

    DrawBarLines(x + 120.0f, y - 70.0f, 110.0f, 10.0f, pulseT, 0.15f, 0.15f, 0.15f, 0.95f, 0.95f, 0.20f);
    DrawBarLines(x + 240.0f, y - 70.0f, 110.0f, 10.0f, slashT, 0.15f, 0.15f, 0.15f, 0.95f, 0.50f, 0.20f);
    DrawBarLines(x + 360.0f, y - 70.0f, 110.0f, 10.0f, meteorT, 0.15f, 0.15f, 0.15f, 0.95f, 0.20f, 0.20f);

    char bufH[128];
    std::snprintf(bufH, sizeof(bufH), "Nests: %d/%d alive", hivesAlive, hivesTotal);
    App::Print(440, 700, bufH);
}

// --------------------------------------------
// Kill popup above player
// --------------------------------------------
void WorldRenderer::RenderKillPopupOverPlayer(float playerScreenX, float playerScreenY, float dtMs)
{
    if (killPopupTimeMs <= 0.0f)
    {
        killPopupCount = 0;
        return;
    }

    killPopupTimeMs -= dtMs;
    if (killPopupTimeMs < 0.0f) killPopupTimeMs = 0.0f;

    float x = playerScreenX - 40.0f;
    float y = playerScreenY - 60.0f;

    x = std::clamp(x, 10.0f, kScreenW - 260.0f);
    y = std::clamp(y, 10.0f, kScreenH - 30.0f);

    float r = 1.0f, g = 0.90f, b = 0.20f;
    int thicknessPasses = 1;
    float popY = 0.0f;

    const char* suffix = "";

    if (killPopupCount >= 100 && killPopupCount < 1000)
    {
        r = 1.0f; g = 0.55f; b = 0.10f;
        thicknessPasses = 2;
        suffix = "  KILL FRENZY";
    }
    else if (killPopupCount >= 1000)
    {
        r = 1.0f; g = 0.15f; b = 0.15f;
        thicknessPasses = 4;
        suffix = "  UNSTOPPABLE ALL CHAOS";

        const float t01 = Clamp01(killPopupTimeMs / 900.0f);
        popY = (1.0f - t01) * -10.0f;
    }

    char kb[64];
    std::snprintf(kb, sizeof(kb), "+%d KILLS", killPopupCount);

    const int ix = (int)x;
    const int iy = (int)(y + popY);

    auto PrintThick = [&](int px, int py, const char* text)
        {
            if (thicknessPasses <= 1)
            {
                App::Print(px, py, text, r, g, b);
            }
            else if (thicknessPasses == 2)
            {
                App::Print(px, py, text, r, g, b);
                App::Print(px + 1, py + 1, text, r, g, b);
            }
            else
            {
                App::Print(px, py, text, r, g, b);
                App::Print(px + 1, py, text, r, g, b);
                App::Print(px, py + 1, text, r, g, b);
                App::Print(px + 1, py + 1, text, r, g, b);
            }
        };

    PrintThick(ix, iy, kb);

    if (suffix[0] != '\0')
    {
        const int approxWidth = (int)std::strlen(kb) * 10;
        PrintThick(ix + approxWidth + 10, iy, suffix);
    }

    const float t = Clamp01(killPopupTimeMs / 900.0f);
    DrawBarLines(
        x, (y - 12.0f) + popY, 180.0f, 6.0f, t,
        0.10f, 0.10f, 0.10f,
        r, g, b
    );
}

void WorldRenderer::DrawZombieTri(float x, float y, float size, float r, float g, float b)
{
    App::DrawTriangle(
        x, y - size, 0, 1,
        x - size, y + size, 0, 1,
        x + size, y + size, 0, 1,
        r, g, b, r, g, b, r, g, b, false
    );

    const float lx = size * 1.3f;
    const float ly = size * 0.7f;

    App::DrawLine(x - size * 0.6f, y, x - lx, y - ly, r, g, b);
    App::DrawLine(x - size * 0.7f, y + size * 0.4f, x - lx, y + 0.0f, r, g, b);
    App::DrawLine(x - size * 0.5f, y + size * 0.8f, x - lx, y + ly, r, g, b);

    App::DrawLine(x + size * 0.6f, y, x + lx, y - ly, r, g, b);
    App::DrawLine(x + size * 0.7f, y + size * 0.4f, x + lx, y + 0.0f, r, g, b);
    App::DrawLine(x + size * 0.5f, y + size * 0.8f, x + lx, y + ly, r, g, b);
}

float WorldRenderer::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
