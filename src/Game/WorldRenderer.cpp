// WorldRenderer.cpp
#include "WorldRenderer.h"

#include "../ContestAPI/app.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdint>

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

static constexpr float kPi = 3.14159265358979323846f;


static float Clamp01f(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float WrapMod(float v, float m)
{
    float r = std::fmod(v, m);
    if (r < 0.0f) r += m;
    return r;
}

static void DrawVignette()
{
    // Thickness in pixels
    const int band = 70;

    // Dark edge color (keep it subtle)
    const float r = 0.00f, g = 0.00f, b = 0.00f;

    // Top
    for (int i = 0; i < band; i++)
    {
        const float t = 1.0f - (float)i / (float)band;  // 1 -> 0
        const float a = 0.18f * t;                      // strength
        App::DrawLine(0.0f, (float)i, 1024.0f, (float)i, r + a, g + a, b + a);
    }

    // Bottom
    for (int i = 0; i < band; i++)
    {
        const float t = 1.0f - (float)i / (float)band;
        const float a = 0.18f * t;
        const float y = 768.0f - 1.0f - (float)i;
        App::DrawLine(0.0f, y, 1024.0f, y, r + a, g + a, b + a);
    }

    // Left
    for (int i = 0; i < band; i++)
    {
        const float t = 1.0f - (float)i / (float)band;
        const float a = 0.16f * t;
        const float x = (float)i;
        App::DrawLine(x, 0.0f, x, 768.0f, r + a, g + a, b + a);
    }

    // Right
    for (int i = 0; i < band; i++)
    {
        const float t = 1.0f - (float)i / (float)band;
        const float a = 0.16f * t;
        const float x = 1024.0f - 1.0f - (float)i;
        App::DrawLine(x, 0.0f, x, 768.0f, r + a, g + a, b + a);
    }
}



static void DrawSciFiLabBackground(float animTimeSec, float offX, float offY)
{
    // Base floor tint (not pure black)
    const float baseR = 0.02f;
    const float baseG = 0.03f;
    const float baseB = 0.05f;

    // Scan lines (screen-space is fine, keeps the look crisp)
    for (int y = 0; y < 768; y += 2)
    {
        const float t = (y % 8 == 0) ? 0.014f : 0.008f;
        App::DrawLine(0.0f, (float)y, 1024.0f, (float)y, baseR + t, baseG + t, baseB + t);
    }

    const float screenW = 1024.0f;
    const float screenH = 768.0f;

    const float grid = 64.0f;
    const float majorPulse = 0.02f + 0.01f * std::sinf(animTimeSec * 0.6f);

    // World-anchoring: shift pattern by camera offset
    const float ox = -WrapMod(offX, grid);
    const float oy = -WrapMod(offY, grid);

    auto DrawThickV = [&](float x, float r, float g, float b, int thick)
        {
            for (int i = 0; i < thick; i++)
                App::DrawLine(x + (float)i, 0.0f, x + (float)i, screenH, r, g, b);
        };

    auto DrawThickH = [&](float y, float r, float g, float b, int thick)
        {
            for (int i = 0; i < thick; i++)
                App::DrawLine(0.0f, y + (float)i, screenW, y + (float)i, r, g, b);
        };

    // Draw enough lines to cover screen (+1 cell margin)
    const int cols = (int)(screenW / grid) + 3;
    const int rows = (int)(screenH / grid) + 3;

    // Vertical grid lines (world anchored)
    for (int i = -1; i < cols; i++)
    {
        const float x = ox + (float)i * grid;

        // Major every 4 cells (based on world cell index, not screen)
        const int worldCol = (int)std::floor((offX + x) / grid);
        const bool major = (worldCol % 4) == 0;

        const int thick = major ? 3 : 2;
        const float a = major ? (0.085f + majorPulse) : 0.055f;

        DrawThickV(x, a, a + 0.01f, a + 0.03f, thick);
    }

    // Horizontal grid lines (world anchored)
    for (int j = -1; j < rows; j++)
    {
        const float y = oy + (float)j * grid;

        const int worldRow = (int)std::floor((offY + y) / grid);
        const bool major = (worldRow % 4) == 0;

        const int thick = major ? 3 : 2;
        const float a = major ? (0.085f + majorPulse) : 0.055f;

        DrawThickH(y, a, a + 0.01f, a + 0.03f, thick);
    }

    // Panel seams (world anchored)
    for (int j = -1; j < rows; j++)
    {
        for (int i = -1; i < cols; i++)
        {
            const float x0 = ox + (float)i * grid;
            const float y0 = oy + (float)j * grid;

            // Alternate using world indices so it doesn't "swim"
            const int wc = (int)std::floor((offX + x0) / grid);
            const int wr = (int)std::floor((offY + y0) / grid);
            const bool alt = ((wc + wr) & 1) != 0;

            const float seam = alt ? 0.075f : 0.060f;

            // top-left notch
            App::DrawLine(x0 + 6.0f, y0 + 6.0f, x0 + 20.0f, y0 + 6.0f, seam, seam + 0.01f, seam + 0.03f);
            App::DrawLine(x0 + 6.0f, y0 + 6.0f, x0 + 6.0f, y0 + 20.0f, seam, seam + 0.01f, seam + 0.03f);

            // bottom-right notch
            App::DrawLine(x0 + grid - 6.0f, y0 + grid - 6.0f, x0 + grid - 20.0f, y0 + grid - 6.0f, seam, seam + 0.01f, seam + 0.03f);
            App::DrawLine(x0 + grid - 6.0f, y0 + grid - 6.0f, x0 + grid - 6.0f, y0 + grid - 20.0f, seam, seam + 0.01f, seam + 0.03f);
        }
    }
}


// Cheap deterministic hash -> [0,1)
static float Hash01(uint32_t v)
{
    v ^= v >> 16;
    v *= 0x7feb352dU;
    v ^= v >> 15;
    v *= 0x846ca68bU;
    v ^= v >> 16;
    return (float)(v & 0x00FFFFFFu) / (float)0x01000000u;
}

// Minimal controls block (pure text, no boxes)
static void PrintControlsMinimal(int x, int y, bool densityView)
{
    App::Print(x, y, "Controls");
    App::Print(x, y - 16, "Move: WASD or Left Stick");
    App::Print(x, y - 32, "View: V or DPad Down");
    App::Print(x, y - 48, "Stop Anim: A");
    App::Print(x, y - 64, "Pulse: Space or B");
    App::Print(x, y - 80, "Slash: Q or X");
    App::Print(x, y - 96, "Meteor: E or Y");
    App::Print(x, y - 112, "Scale: Left/Right Arrow or LB/RB");
    App::Print(x, y - 128, densityView ? "Mode: Density" : "Mode: Entities");
}

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
    // advance wiggle time
    animTimeSec += (dtMs * 0.001f);
    if (animTimeSec > 100000.0f) animTimeSec = 0.0f;

    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    RenderWorld(offX, offY, player, nav, zombies, hives, attacks, dtMs, densityView);
}

void WorldRenderer::NotifyKills(int kills)
{
    if (kills <= 0) return;

    if (killPopupTimeMs > 0.0f) killPopupCount += kills;
    else killPopupCount = kills;

    killPopupTimeMs = 1200.0f;
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
    DrawVignette();


    

    float px = 0.0f, py = 0.0f;
    player.GetWorldPosition(px, py);

    DrawSciFiLabBackground(dtMs, offX, offY);

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

                // no wiggle in density view (cleaner)
                DrawZombieTri(x, y, cs * 0.45f, 0.0f, r, g, b);
            }
        }
        return;
    }

    int step = 1;
    if (count > kFullDrawThreshold)
        step = (count + kMaxDraw - 1) / kMaxDraw;

    // Wiggle tuning
    const float baseFreq = 2.2f;
    const float freqJitter = 1.1f;
    const float angleAmp = 0.22f;

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

        // per-zombie phase and slightly different speed
        const uint32_t seed = (uint32_t)i * 2654435761u ^ (uint32_t)(t * 97u);
        const float phase = Hash01(seed) * (2.0f * kPi);
        const float freq = baseFreq + Hash01(seed ^ 0xA53A9E3Du) * freqJitter;

        const float angle = std::sinf(animTimeSec * (2.0f * kPi) * freq + phase) * angleAmp;

        DrawZombieTri(x, y, sizeByType[t], angle, r, g, b);
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
    // Minimal controls (top-left)
    //PrintControlsMinimal(18, 740, densityView);

    // Stats HUD (top-right-ish)
    const float x = 500.0f;
    const float y = 80.0f;

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
    int thicknessPasses =2;
    float popY = 0.0f;

    const char* suffix = "";

    if (killPopupCount >= 100 && killPopupCount < 1000)
    {
        r = 1.0f; g = 0.55f; b = 0.10f;
        thicknessPasses = 4;
        suffix = "  KILL FRENZY";
    }
    else if (killPopupCount >= 1000)
    {
        r = 1.0f; g = 0.15f; b = 0.15f;
        thicknessPasses = 8;
        suffix = "  UNSTOPPABLE ALL CHAOS";

        const float t01 = Clamp01(killPopupTimeMs / 1200.0f);
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

    const float t = Clamp01(killPopupTimeMs / 1200.0f);
    DrawBarLines(
        x, (y - 12.0f) + popY, 180.0f, 6.0f, t,
        0.10f, 0.10f, 0.10f,
        r, g, b
    );
}

void WorldRenderer::DrawZombieTri(float x, float y, float size, float angleRad, float r, float g, float b)
{
    const float c = std::cos(angleRad);
    const float s = std::sin(angleRad);

    auto Rot = [&](float px, float py, float& ox, float& oy)
        {
            ox = px * c - py * s;
            oy = px * s + py * c;
        };

    // Triangle points in local space
    float ax, ay, bx, by, cx, cy;
    Rot(0.0f, -size, ax, ay);
    Rot(-size, size, bx, by);
    Rot(size, size, cx, cy);

    App::DrawTriangle(
        x + ax, y + ay, 0, 1,
        x + bx, y + by, 0, 1,
        x + cx, y + cy, 0, 1,
        r, g, b, r, g, b, r, g, b, false
    );

    // Legs in local space, then rotate them too
    const float lx = size * 1.3f;
    const float ly = size * 0.7f;

    float p1x, p1y, p2x, p2y;

    Rot(-size * 0.6f, 0.0f, p1x, p1y); Rot(-lx, -ly, p2x, p2y);
    App::DrawLine(x + p1x, y + p1y, x + p2x, y + p2y, r, g, b);

    Rot(-size * 0.7f, size * 0.4f, p1x, p1y); Rot(-lx, 0.0f, p2x, p2y);
    App::DrawLine(x + p1x, y + p1y, x + p2x, y + p2y, r, g, b);

    Rot(-size * 0.5f, size * 0.8f, p1x, p1y); Rot(-lx, ly, p2x, p2y);
    App::DrawLine(x + p1x, y + p1y, x + p2x, y + p2y, r, g, b);

    Rot(size * 0.6f, 0.0f, p1x, p1y); Rot(lx, -ly, p2x, p2y);
    App::DrawLine(x + p1x, y + p1y, x + p2x, y + p2y, r, g, b);

    Rot(size * 0.7f, size * 0.4f, p1x, p1y); Rot(lx, 0.0f, p2x, p2y);
    App::DrawLine(x + p1x, y + p1y, x + p2x, y + p2y, r, g, b);

    Rot(size * 0.5f, size * 0.8f, p1x, p1y); Rot(lx, ly, p2x, p2y);
    App::DrawLine(x + p1x, y + p1y, x + p2x, y + p2y, r, g, b);
}

float WorldRenderer::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
