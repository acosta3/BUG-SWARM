// WorldRenderer.cpp
#include "WorldRenderer.h"

#include "../ContestAPI/app.h"
#include <cstdio>
#include <vector>
#include <algorithm>
#include <cmath>

// include your real headers here (not forward declares)
#include "Player.h"
#include "NavGrid.h"
#include "ZombieSystem.h"
#include "CameraSystem.h"
#include "IsoProjector.h"
#include "HiveSystem.h"

// --------------------------------------------
// Small helpers (local to this cpp)
// --------------------------------------------
struct ZombieDrawItem
{
    float sortKey;   // iso painter's algorithm (back-to-front)
    float sx, sy;    // projected screen position
    float size;
    float r, g, b;
    float angle;     // facing angle in SCREEN space (radians)
};

static inline void Shade(float r, float g, float b, float m, float& outR, float& outG, float& outB)
{
    outR = r * m;
    outG = g * m;
    outB = b * m;
}

static inline void Rotate2D(float lx, float ly, float c, float s, float& outX, float& outY)
{
    outX = lx * c - ly * s;
    outY = lx * s + ly * c;
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
    bool densityView)
{
    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    RenderWorld(offX, offY, player, nav, zombies, hives, densityView);
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
    bool densityView)
{
    float px, py;
    player.GetWorldPosition(px, py);

    // 1) Maze / obstacles (background debug)
    nav.DebugDrawBlocked(offX, offY);

    // 2) Hives (yellow circles) in world space
    hives.Render(offX, offY);

    // 3) Zombies
    //RenderZombiesIso(offX, offY, zombies, densityView, px, py);
    RenderZombies2D(offX, offY, zombies, densityView);

    // 4) Player on top so it stays readable
    player.Render(offX, offY);

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

    RenderUI(simCount, drawn, step, densityView, player.GetHealth(), player.GetMaxHealth());
}

// --------------------------------------------
// Zombies 2D (kept for debugging)
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

// --------------------------------------------
// Iso wedge (top + 2 side faces) in screen space
// --------------------------------------------
void WorldRenderer::DrawIsoWedge(float sx, float sy, float size, float height,
    float r, float g, float b, float angleRad)
{
    const float c = std::cos(angleRad);
    const float s = std::sin(angleRad);

    // Local upright triangle (around 0,0)
    // Tip is (0, -size) so "forward" is UP in screen space when angleRad = 0
    const float lx1 = 0.0f;    const float ly1 = -size;
    const float lx2 = -size;   const float ly2 = size;
    const float lx3 = size;    const float ly3 = size;

    float rx1, ry1, rx2, ry2, rx3, ry3;
    Rotate2D(lx1, ly1, c, s, rx1, ry1);
    Rotate2D(lx2, ly2, c, s, rx2, ry2);
    Rotate2D(lx3, ly3, c, s, rx3, ry3);

    // Base triangle in screen space
    const float b1x = sx + rx1; const float b1y = sy + ry1;
    const float b2x = sx + rx2; const float b2y = sy + ry2;
    const float b3x = sx + rx3; const float b3y = sy + ry3;

    // Lifted top triangle
    const float t1x = b1x; const float t1y = b1y - height;
    const float t2x = b2x; const float t2y = b2y - height;
    const float t3x = b3x; const float t3y = b3y - height;

    // Top face
    App::DrawTriangle(
        t1x, t1y, 0.0f, 1.0f,
        t2x, t2y, 0.0f, 1.0f,
        t3x, t3y, 0.0f, 1.0f,
        r, g, b,
        r, g, b,
        r, g, b,
        false
    );

    // Side shades
    float rDark, gDark, bDark;
    float rMid, gMid, bMid;
    Shade(r, g, b, 0.60f, rDark, gDark, bDark);
    Shade(r, g, b, 0.78f, rMid, gMid, bMid);

    // Side face 1: edge (b2-b3) to (t2-t3)
    App::DrawTriangle(
        b2x, b2y, 0.0f, 1.0f,
        b3x, b3y, 0.0f, 1.0f,
        t3x, t3y, 0.0f, 1.0f,
        rDark, gDark, bDark,
        rDark, gDark, bDark,
        rDark, gDark, bDark,
        false
    );
    App::DrawTriangle(
        b2x, b2y, 0.0f, 1.0f,
        t3x, t3y, 0.0f, 1.0f,
        t2x, t2y, 0.0f, 1.0f,
        rDark, gDark, bDark,
        rDark, gDark, bDark,
        rDark, gDark, bDark,
        false
    );

    // Side face 2: edge (b1-b3) to (t1-t3)
    App::DrawTriangle(
        b1x, b1y, 0.0f, 1.0f,
        b3x, b3y, 0.0f, 1.0f,
        t3x, t3y, 0.0f, 1.0f,
        rMid, gMid, bMid,
        rMid, gMid, bMid,
        rMid, gMid, bMid,
        false
    );
    App::DrawTriangle(
        b1x, b1y, 0.0f, 1.0f,
        t3x, t3y, 0.0f, 1.0f,
        t1x, t1y, 0.0f, 1.0f,
        rMid, gMid, bMid,
        rMid, gMid, bMid,
        rMid, gMid, bMid,
        false
    );
}

// --------------------------------------------
// Zombies ISO (depth sort + wedge draw)
// Facing is computed in SCREEN space.
// We add +PI/2 so the local tip (0,-1) points toward the player.
// --------------------------------------------
void WorldRenderer::RenderZombiesIso(
    float offX, float offY,
    const ZombieSystem& zombies,
    bool densityView,
    float playerX, float playerY)
{
    IsoProjector iso = IsoProjector::FromCameraOffset(offX, offY);

    static const float sizeByType[4] = { 3.0f, 3.5f, 5.0f, 7.0f };
    static const float rByType[4] = { 0.2f, 1.0f, 0.2f, 0.8f };
    static const float gByType[4] = { 1.0f, 0.2f, 0.4f, 0.2f };
    static const float bByType[4] = { 0.2f, 0.2f, 1.0f, 1.0f };

    const int count = zombies.AliveCount();

    // Project player once (screen space target)
    float psx, psy;
    iso.WorldToScreen(playerX, playerY, 0.0f, psx, psy);

    const float PI_2 = 1.57079632679f;

    // Density view
    if (densityView)
    {
        const int gw = zombies.GetGridW();
        const int gh = zombies.GetGridH();
        const float cs = zombies.GetCellSize();
        const float minX = zombies.GetWorldMinX();
        const float minY = zombies.GetWorldMinY();

        std::vector<ZombieDrawItem> items;
        items.reserve(4096);

        for (int cy = 0; cy < gh; cy++)
        {
            for (int cx = 0; cx < gw; cx++)
            {
                const int idx = cy * gw + cx;
                const int n = zombies.GetCellCountAt(idx);
                if (n <= 0) continue;

                const float wx = minX + (cx + 0.5f) * cs;
                const float wy = minY + (cy + 0.5f) * cs;

                float sx, sy;
                iso.WorldToScreen(wx, wy, 0.0f, sx, sy);

                if (sx < -50.0f || sx > 1074.0f || sy < -50.0f || sy > 818.0f)
                    continue;

                const float denom = 20.0f;
                const float intensity = Clamp01((float)n / denom);

                ZombieDrawItem it;
                it.sortKey = wx + wy;
                it.sx = sx;
                it.sy = sy;
                it.size = cs * 0.45f;
                it.r = intensity;
                it.g = 0.2f + 0.8f * (1.0f - 0.5f * intensity);
                it.b = 0.1f;

                // Screen-space facing, plus 90 degrees to match tip (0,-1)
                it.angle = std::atan2(psy - sy, psx - sx) + PI_2;

                items.push_back(it);
            }
        }

        std::sort(items.begin(), items.end(),
            [](const ZombieDrawItem& a, const ZombieDrawItem& b)
            {
                return a.sortKey < b.sortKey;
            });

        for (const auto& it : items)
        {
            const float h = it.size * 1.6f;
            DrawIsoWedge(it.sx, it.sy, it.size, h, it.r, it.g, it.b, it.angle);
        }

        return;
    }

    // Entity view: sample if huge, depth sort, draw wedges
    const int fullDrawThreshold = 50000;
    const int maxDraw = 10000;

    int step = 1;
    if (count > fullDrawThreshold)
        step = (count + maxDraw - 1) / maxDraw;

    std::vector<ZombieDrawItem> items;
    items.reserve(maxDraw);

    for (int i = 0; i < count; i += step)
    {
        const float wx = zombies.GetX(i);
        const float wy = zombies.GetY(i);

        float sx, sy;
        iso.WorldToScreen(wx, wy, 0.0f, sx, sy);

        if (sx < -50.0f || sx > 1074.0f || sy < -50.0f || sy > 818.0f)
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

        ZombieDrawItem it;
        it.sortKey = wx + wy;
        it.sx = sx;
        it.sy = sy;
        it.size = sizeByType[t];
        it.r = r; it.g = g; it.b = b;

        // Screen-space facing, plus 90 degrees to match tip (0,-1)
        it.angle = std::atan2(psy - sy, psx - sx) + PI_2;

        items.push_back(it);
    }

    std::sort(items.begin(), items.end(),
        [](const ZombieDrawItem& a, const ZombieDrawItem& b)
        {
            return a.sortKey < b.sortKey;
        });

    for (const auto& it : items)
    {
        const float h = it.size * 1.6f;
        DrawIsoWedge(it.sx, it.sy, it.size, h, it.r, it.g, it.b, it.angle);
    }
}

// --------------------------------------------
// UI
// --------------------------------------------
void WorldRenderer::RenderUI(int simCount, int drawnCount, int step, bool densityView, int hp, int maxHp)
{
    App::Print(20, 60, densityView ? "View: Density (V/X)" : "View: Entities (V/X)");

    char buf2[96];
    std::snprintf(buf2, sizeof(buf2), "Sim: %d  Draw: %d  Step: %d", simCount, drawnCount, step);
    App::Print(20, 40, buf2);

    char bufHP[96];
    std::snprintf(bufHP, sizeof(bufHP), "HP: %d/%d", hp, maxHp);
    App::Print(500, 40, bufHP);
}

// --------------------------------------------
// Basic triangle helper (still used by 2D mode)
// --------------------------------------------
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
