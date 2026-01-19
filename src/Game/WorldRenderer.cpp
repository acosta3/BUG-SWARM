// WorldRenderer.cpp
#include "WorldRenderer.h"
#include "GameConfig.h"

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
    const int band = GameConfig::RenderConfig::VIGNETTE_BAND_SIZE;

    const float r = 0.00f, g = 0.00f, b = 0.00f;

    // Top
    for (int i = 0; i < band; i++)
    {
        const float t = 1.0f - (float)i / (float)band;
        const float a = GameConfig::RenderConfig::VIGNETTE_TOP_STRENGTH * t;
        App::DrawLine(0.0f, (float)i, GameConfig::RenderConfig::SCREEN_W, (float)i, r + a, g + a, b + a);
    }

    // Bottom
    for (int i = 0; i < band; i++)
    {
        const float t = 1.0f - (float)i / (float)band;
        const float a = GameConfig::RenderConfig::VIGNETTE_BOTTOM_STRENGTH * t;
        const float y = GameConfig::RenderConfig::SCREEN_H - 1.0f - (float)i;
        App::DrawLine(0.0f, y, GameConfig::RenderConfig::SCREEN_W, y, r + a, g + a, b + a);
    }

    // Left
    for (int i = 0; i < band; i++)
    {
        const float t = 1.0f - (float)i / (float)band;
        const float a = GameConfig::RenderConfig::VIGNETTE_SIDE_STRENGTH * t;
        const float x = (float)i;
        App::DrawLine(x, 0.0f, x, GameConfig::RenderConfig::SCREEN_H, r + a, g + a, b + a);
    }

    // Right
    for (int i = 0; i < band; i++)
    {
        const float t = 1.0f - (float)i / (float)band;
        const float a = GameConfig::RenderConfig::VIGNETTE_SIDE_STRENGTH * t;
        const float x = GameConfig::RenderConfig::SCREEN_W - 1.0f - (float)i;
        App::DrawLine(x, 0.0f, x, GameConfig::RenderConfig::SCREEN_H, r + a, g + a, b + a);
    }
}

static void DrawSciFiLabBackground(float animTimeSec, float offX, float offY)
{
    // Scan lines (screen-space)
    for (int y = 0; y < (int)GameConfig::RenderConfig::SCREEN_H; y += GameConfig::RenderConfig::BG_SCANLINE_STEP)
    {
        const float t = (y % GameConfig::RenderConfig::BG_SCANLINE_MOD == 0)
            ? GameConfig::RenderConfig::BG_SCANLINE_THICK
            : GameConfig::RenderConfig::BG_SCANLINE_THIN;
        App::DrawLine(0.0f, (float)y, GameConfig::RenderConfig::SCREEN_W, (float)y,
            GameConfig::RenderConfig::BG_BASE_R + t,
            GameConfig::RenderConfig::BG_BASE_G + t,
            GameConfig::RenderConfig::BG_BASE_B + t);
    }

    const float screenW = GameConfig::RenderConfig::SCREEN_W;
    const float screenH = GameConfig::RenderConfig::SCREEN_H;
    const float grid = GameConfig::RenderConfig::BG_GRID_SIZE;
    const float majorPulse = GameConfig::RenderConfig::BG_MAJOR_PULSE_BASE +
        GameConfig::RenderConfig::BG_MAJOR_PULSE_AMP * std::sinf(animTimeSec * GameConfig::RenderConfig::BG_MAJOR_PULSE_FREQ);

    // World-anchoring
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

    const int cols = (int)(screenW / grid) + 3;
    const int rows = (int)(screenH / grid) + 3;

    // Vertical grid lines
    for (int i = -1; i < cols; i++)
    {
        const float x = ox + (float)i * grid;
        const int worldCol = (int)std::floor((offX + x) / grid);
        const bool major = (worldCol % GameConfig::RenderConfig::BG_GRID_MAJOR_EVERY) == 0;

        const int thick = major ? GameConfig::RenderConfig::BG_GRID_THICK_MAJOR : GameConfig::RenderConfig::BG_GRID_THICK_MINOR;
        const float a = major ? (GameConfig::RenderConfig::BG_GRID_ALPHA_MAJOR + majorPulse) : GameConfig::RenderConfig::BG_GRID_ALPHA_MINOR;

        DrawThickV(x, a, a + 0.01f, a + 0.03f, thick);
    }

    // Horizontal grid lines
    for (int j = -1; j < rows; j++)
    {
        const float y = oy + (float)j * grid;
        const int worldRow = (int)std::floor((offY + y) / grid);
        const bool major = (worldRow % GameConfig::RenderConfig::BG_GRID_MAJOR_EVERY) == 0;

        const int thick = major ? GameConfig::RenderConfig::BG_GRID_THICK_MAJOR : GameConfig::RenderConfig::BG_GRID_THICK_MINOR;
        const float a = major ? (GameConfig::RenderConfig::BG_GRID_ALPHA_MAJOR + majorPulse) : GameConfig::RenderConfig::BG_GRID_ALPHA_MINOR;

        DrawThickH(y, a, a + 0.01f, a + 0.03f, thick);
    }

    // Panel seams
    for (int j = -1; j < rows; j++)
    {
        for (int i = -1; i < cols; i++)
        {
            const float x0 = ox + (float)i * grid;
            const float y0 = oy + (float)j * grid;

            const int wc = (int)std::floor((offX + x0) / grid);
            const int wr = (int)std::floor((offY + y0) / grid);
            const bool alt = ((wc + wr) & 1) != 0;

            const float seam = alt ? GameConfig::RenderConfig::BG_SEAM_ALT_1 : GameConfig::RenderConfig::BG_SEAM_ALT_2;

            App::DrawLine(x0 + GameConfig::RenderConfig::BG_SEAM_OFFSET_1, y0 + GameConfig::RenderConfig::BG_SEAM_OFFSET_1,
                x0 + GameConfig::RenderConfig::BG_SEAM_OFFSET_2, y0 + GameConfig::RenderConfig::BG_SEAM_OFFSET_1, seam, seam + 0.01f, seam + 0.03f);
            App::DrawLine(x0 + GameConfig::RenderConfig::BG_SEAM_OFFSET_1, y0 + GameConfig::RenderConfig::BG_SEAM_OFFSET_1,
                x0 + GameConfig::RenderConfig::BG_SEAM_OFFSET_1, y0 + GameConfig::RenderConfig::BG_SEAM_OFFSET_2, seam, seam + 0.01f, seam + 0.03f);

            App::DrawLine(x0 + grid - GameConfig::RenderConfig::BG_SEAM_OFFSET_1, y0 + grid - GameConfig::RenderConfig::BG_SEAM_OFFSET_1,
                x0 + grid - GameConfig::RenderConfig::BG_SEAM_OFFSET_2, y0 + grid - GameConfig::RenderConfig::BG_SEAM_OFFSET_1, seam, seam + 0.01f, seam + 0.03f);
            App::DrawLine(x0 + grid - GameConfig::RenderConfig::BG_SEAM_OFFSET_1, y0 + grid - GameConfig::RenderConfig::BG_SEAM_OFFSET_1,
                x0 + grid - GameConfig::RenderConfig::BG_SEAM_OFFSET_1, y0 + grid - GameConfig::RenderConfig::BG_SEAM_OFFSET_2, seam, seam + 0.01f, seam + 0.03f);
        }
    }
}

static float Hash01(uint32_t v)
{
    v ^= v >> GameConfig::HashConfig::HASH_XOR_1;
    v *= GameConfig::HashConfig::HASH_MULT_1;
    v ^= v >> GameConfig::HashConfig::HASH_XOR_2;
    v *= GameConfig::HashConfig::HASH_MULT_2;
    v ^= v >> GameConfig::HashConfig::HASH_XOR_3;
    return (float)(v & GameConfig::HashConfig::HASH_MASK) / (float)GameConfig::HashConfig::HASH_DIVISOR;
}

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
    animTimeSec += (dtMs * 0.001f);
    if (animTimeSec > GameConfig::RenderConfig::WIGGLE_TIME_RESET) animTimeSec = 0.0f;

    const float offX = camera.GetOffsetX();
    const float offY = camera.GetOffsetY();

    RenderWorld(offX, offY, player, nav, zombies, hives, attacks, dtMs, densityView);
}

void WorldRenderer::NotifyKills(int kills)
{
    if (kills <= 0) return;

    if (killPopupTimeMs > 0.0f) killPopupCount += kills;
    else killPopupCount = kills;

    killPopupTimeMs = GameConfig::RenderConfig::KILL_POPUP_DURATION_MS;
}

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
        if (simCount > GameConfig::RenderConfig::FULL_DRAW_THRESHOLD)
            step = (simCount + GameConfig::RenderConfig::MAX_DRAW - 1) / GameConfig::RenderConfig::MAX_DRAW;

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

    RenderTacticalMinimap(player, hives);
}

void WorldRenderer::RenderZombies2D(float offX, float offY, const ZombieSystem& zombies, bool densityView)
{
    static const float sizeByType[4] = {
        GameConfig::RenderConfig::ZOMBIE_SIZE_GREEN,
        GameConfig::RenderConfig::ZOMBIE_SIZE_RED,
        GameConfig::RenderConfig::ZOMBIE_SIZE_BLUE,
        GameConfig::RenderConfig::ZOMBIE_SIZE_PURPLE
    };
    static const float rByType[4] = {
        GameConfig::RenderConfig::ZOMBIE_R_GREEN,
        GameConfig::RenderConfig::ZOMBIE_R_RED,
        GameConfig::RenderConfig::ZOMBIE_R_BLUE,
        GameConfig::RenderConfig::ZOMBIE_R_PURPLE
    };
    static const float gByType[4] = {
        GameConfig::RenderConfig::ZOMBIE_G_GREEN,
        GameConfig::RenderConfig::ZOMBIE_G_RED,
        GameConfig::RenderConfig::ZOMBIE_G_BLUE,
        GameConfig::RenderConfig::ZOMBIE_G_PURPLE
    };
    static const float bByType[4] = {
        GameConfig::RenderConfig::ZOMBIE_B_GREEN,
        GameConfig::RenderConfig::ZOMBIE_B_RED,
        GameConfig::RenderConfig::ZOMBIE_B_BLUE,
        GameConfig::RenderConfig::ZOMBIE_B_PURPLE
    };

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

                if (x < 0.0f || x > GameConfig::RenderConfig::SCREEN_W || y < 0.0f || y > GameConfig::RenderConfig::SCREEN_H)
                    continue;

                const float intensity = Clamp01((float)n / GameConfig::RenderConfig::DENSITY_INTENSITY_DIVISOR);

                const float r = intensity;
                const float g = GameConfig::RenderConfig::DENSITY_G_BASE +
                    GameConfig::RenderConfig::DENSITY_G_RANGE * (1.0f - GameConfig::RenderConfig::DENSITY_G_FACTOR * intensity);
                const float b = GameConfig::RenderConfig::DENSITY_B;

                DrawZombieTri(x, y, cs * GameConfig::RenderConfig::DENSITY_CELL_SCALE, 0.0f, r, g, b);
            }
        }
        return;
    }

    int step = 1;
    if (count > GameConfig::RenderConfig::FULL_DRAW_THRESHOLD)
        step = (count + GameConfig::RenderConfig::MAX_DRAW - 1) / GameConfig::RenderConfig::MAX_DRAW;

    for (int i = 0; i < count; i += step)
    {
        const float x = zombies.GetX(i) - offX;
        const float y = zombies.GetY(i) - offY;

        if (x < 0.0f || x > GameConfig::RenderConfig::SCREEN_W || y < 0.0f || y > GameConfig::RenderConfig::SCREEN_H)
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

        const uint32_t seed = (uint32_t)i * GameConfig::RenderConfig::WIGGLE_SEED_MULT ^ (uint32_t)(t * GameConfig::RenderConfig::WIGGLE_SEED_ADD);
        const float phase = Hash01(seed) * GameConfig::MathConstants::TWO_PI;
        const float freq = GameConfig::RenderConfig::WIGGLE_BASE_FREQ + Hash01(seed ^ GameConfig::RenderConfig::WIGGLE_SEED_XOR) * GameConfig::RenderConfig::WIGGLE_FREQ_JITTER;

        const float angle = std::sinf(animTimeSec * GameConfig::MathConstants::TWO_PI * freq + phase) * GameConfig::RenderConfig::WIGGLE_ANGLE_AMP;

        DrawZombieTri(x, y, sizeByType[t], angle, r, g, b);
    }
}

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
    const float fillW = (w * t) < GameConfig::RenderConfig::BAR_FILL_MIN_WIDTH ? 0.0f : (w * t);

    for (int i = 0; i < lines; i++)
    {
        const float yy = y + (float)i;

        App::DrawLine(x, yy, x + w, yy, bgR, bgG, bgB);
        if (fillW > GameConfig::RenderConfig::BAR_FILL_MIN_WIDTH)
            App::DrawLine(x, yy, x + fillW, yy, fillR, fillG, fillB);
    }

    DrawRectOutline(x, y, x + w, y + h,
        GameConfig::RenderConfig::BAR_OUTLINE_R,
        GameConfig::RenderConfig::BAR_OUTLINE_G,
        GameConfig::RenderConfig::BAR_OUTLINE_B);
}

void WorldRenderer::RenderUI(
    int simCount, int maxCount,
    int drawnCount, int step,
    bool densityView,
    int hp, int maxHp,
    int hivesAlive, int hivesTotal,
    float pulseCdMs, float slashCdMs, float meteorCdMs)
{
    const float x = GameConfig::RenderConfig::UI_HUD_X;
    const float y = GameConfig::RenderConfig::UI_HUD_Y;

    char bufZ[128];
    std::snprintf(bufZ, sizeof(bufZ), "Zombies: %d/%d  Draw: %d  Step: %d", simCount, maxCount, drawnCount, step);
    App::Print((int)x, (int)(y - 18.0f), bufZ);

    char bufHP[64];
    std::snprintf(bufHP, sizeof(bufHP), "HP %d/%d", hp, maxHp);
    App::Print((int)x, (int)(y - 36.0f), bufHP);

    const float hpT = (maxHp > 0) ? ((float)hp / (float)maxHp) : 0.0f;
    DrawBarLines(
        x + GameConfig::RenderConfig::UI_HP_BAR_X_OFFSET,
        y - GameConfig::RenderConfig::UI_HP_BAR_Y_OFFSET,
        GameConfig::RenderConfig::UI_HP_BAR_WIDTH,
        GameConfig::RenderConfig::UI_HP_BAR_HEIGHT,
        hpT,
        GameConfig::RenderConfig::BAR_BG_R,
        GameConfig::RenderConfig::BAR_BG_G,
        GameConfig::RenderConfig::BAR_BG_B,
        GameConfig::RenderConfig::HP_BAR_R,
        GameConfig::RenderConfig::HP_BAR_G,
        GameConfig::RenderConfig::HP_BAR_B
    );

    char bufCD[160];
    std::snprintf(
        bufCD, sizeof(bufCD),
        "Cooldowns (ms)  Pulse: %.0f  Slash: %.0f  Meteor: %.0f",
        pulseCdMs, slashCdMs, meteorCdMs
    );
    App::Print((int)x, (int)(y - 54.0f), bufCD);

    const float pulseT = 1.0f - Clamp01(pulseCdMs / GameConfig::RenderConfig::UI_PULSE_CD_MAX);
    const float slashT = 1.0f - Clamp01(slashCdMs / GameConfig::RenderConfig::UI_SLASH_CD_MAX);
    const float meteorT = 1.0f - Clamp01(meteorCdMs / GameConfig::RenderConfig::UI_METEOR_CD_MAX);

    DrawBarLines(x + GameConfig::RenderConfig::UI_HP_BAR_X_OFFSET, y - GameConfig::RenderConfig::UI_CD_BAR_Y_OFFSET,
        GameConfig::RenderConfig::UI_CD_BAR_WIDTH, GameConfig::RenderConfig::UI_CD_BAR_HEIGHT, pulseT,
        GameConfig::RenderConfig::CD_BG_R, GameConfig::RenderConfig::CD_BG_G, GameConfig::RenderConfig::CD_BG_B,
        GameConfig::RenderConfig::PULSE_CD_R, GameConfig::RenderConfig::PULSE_CD_G, GameConfig::RenderConfig::PULSE_CD_B);

    DrawBarLines(x + GameConfig::RenderConfig::UI_HP_BAR_X_OFFSET + GameConfig::RenderConfig::UI_CD_BAR_SPACING, y - GameConfig::RenderConfig::UI_CD_BAR_Y_OFFSET,
        GameConfig::RenderConfig::UI_CD_BAR_WIDTH, GameConfig::RenderConfig::UI_CD_BAR_HEIGHT, slashT,
        GameConfig::RenderConfig::CD_BG_R, GameConfig::RenderConfig::CD_BG_G, GameConfig::RenderConfig::CD_BG_B,
        GameConfig::RenderConfig::SLASH_CD_R, GameConfig::RenderConfig::SLASH_CD_G, GameConfig::RenderConfig::SLASH_CD_B);

    DrawBarLines(x + GameConfig::RenderConfig::UI_HP_BAR_X_OFFSET + GameConfig::RenderConfig::UI_CD_BAR_SPACING * 2.0f, y - GameConfig::RenderConfig::UI_CD_BAR_Y_OFFSET,
        GameConfig::RenderConfig::UI_CD_BAR_WIDTH, GameConfig::RenderConfig::UI_CD_BAR_HEIGHT, meteorT,
        GameConfig::RenderConfig::CD_BG_R, GameConfig::RenderConfig::CD_BG_G, GameConfig::RenderConfig::CD_BG_B,
        GameConfig::RenderConfig::METEOR_CD_R, GameConfig::RenderConfig::METEOR_CD_G, GameConfig::RenderConfig::METEOR_CD_B);

    char bufH[128];
    std::snprintf(bufH, sizeof(bufH), "Nests: %d/%d alive", hivesAlive, hivesTotal);
    App::Print(440, 700, bufH);
}

void WorldRenderer::RenderKillPopupOverPlayer(float playerScreenX, float playerScreenY, float dtMs)
{
    if (killPopupTimeMs <= 0.0f)
    {
        killPopupCount = 0;
        return;
    }

    killPopupTimeMs -= dtMs;
    if (killPopupTimeMs < 0.0f) killPopupTimeMs = 0.0f;

    float x = playerScreenX - GameConfig::RenderConfig::KILL_POPUP_OFFSET_X;
    float y = playerScreenY - GameConfig::RenderConfig::KILL_POPUP_OFFSET_Y;

    x = std::clamp(x, GameConfig::RenderConfig::KILL_POPUP_MIN_X, GameConfig::RenderConfig::SCREEN_W - GameConfig::RenderConfig::KILL_POPUP_MAX_X_OFFSET);
    y = std::clamp(y, GameConfig::RenderConfig::KILL_POPUP_MIN_Y, GameConfig::RenderConfig::SCREEN_H - GameConfig::RenderConfig::KILL_POPUP_MAX_Y_OFFSET);

    float r = GameConfig::RenderConfig::KILL_POPUP_NORMAL_R;
    float g = GameConfig::RenderConfig::KILL_POPUP_NORMAL_G;
    float b = GameConfig::RenderConfig::KILL_POPUP_NORMAL_B;
    int thicknessPasses = 2;
    float popY = 0.0f;

    const char* suffix = "";

    if (killPopupCount >= GameConfig::RenderConfig::KILL_POPUP_FRENZY_THRESHOLD && killPopupCount < GameConfig::RenderConfig::KILL_POPUP_UNSTOPPABLE_THRESHOLD)
    {
        r = GameConfig::RenderConfig::KILL_POPUP_FRENZY_R;
        g = GameConfig::RenderConfig::KILL_POPUP_FRENZY_G;
        b = GameConfig::RenderConfig::KILL_POPUP_FRENZY_B;
        thicknessPasses = 4;
        suffix = "  KILL FRENZY";
    }
    else if (killPopupCount >= GameConfig::RenderConfig::KILL_POPUP_UNSTOPPABLE_THRESHOLD)
    {
        r = GameConfig::RenderConfig::KILL_POPUP_UNSTOPPABLE_R;
        g = GameConfig::RenderConfig::KILL_POPUP_UNSTOPPABLE_G;
        b = GameConfig::RenderConfig::KILL_POPUP_UNSTOPPABLE_B;
        thicknessPasses = 8;
        suffix = "  UNSTOPPABLE ALL CHAOS";

        const float t01 = Clamp01(killPopupTimeMs / GameConfig::RenderConfig::KILL_POPUP_DURATION_MS);
        popY = (1.0f - t01) * GameConfig::RenderConfig::KILL_POPUP_POP_OFFSET;
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
        const int approxWidth = (int)std::strlen(kb) * GameConfig::RenderConfig::KILL_POPUP_CHAR_WIDTH;
        PrintThick(ix + approxWidth + GameConfig::RenderConfig::KILL_POPUP_TEXT_SPACING, iy, suffix);
    }

    const float t = Clamp01(killPopupTimeMs / GameConfig::RenderConfig::KILL_POPUP_DURATION_MS);
    DrawBarLines(
        x, (y - GameConfig::RenderConfig::KILL_POPUP_BAR_OFFSET_Y) + popY,
        GameConfig::RenderConfig::KILL_POPUP_BAR_WIDTH,
        GameConfig::RenderConfig::KILL_POPUP_BAR_HEIGHT,
        t,
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

    float ax, ay, bx, by, cx, cy;
    Rot(0.0f, -size, ax, ay);
    Rot(-size, size, bx, by);
    Rot(size, size, cx, cy);

    // Shadow/backplate
    {
        const float shadowScale = GameConfig::RenderConfig::ZOMBIE_SHADOW_SCALE;
        const float sr = r * GameConfig::RenderConfig::ZOMBIE_SHADOW_MULT;
        const float sg = g * GameConfig::RenderConfig::ZOMBIE_SHADOW_MULT;
        const float sb = b * GameConfig::RenderConfig::ZOMBIE_SHADOW_MULT;

        App::DrawTriangle(
            x + ax * shadowScale, y + ay * shadowScale, 0, 1,
            x + bx * shadowScale, y + by * shadowScale, 0, 1,
            x + cx * shadowScale, y + cy * shadowScale, 0, 1,
            sr, sg, sb, sr, sg, sb, sr, sg, sb, false
        );
    }

    // Main fill
    {
        const float fr = r * GameConfig::RenderConfig::ZOMBIE_FILL_MULT;
        const float fg = g * GameConfig::RenderConfig::ZOMBIE_FILL_MULT;
        const float fb = b * GameConfig::RenderConfig::ZOMBIE_FILL_MULT;

        App::DrawTriangle(
            x + ax, y + ay, 0, 1,
            x + bx, y + by, 0, 1,
            x + cx, y + cy, 0, 1,
            fr, fg, fb, fr, fg, fb, fr, fg, fb, false
        );

        // Outline (removed std::min)
        const float orr = (fr + GameConfig::RenderConfig::ZOMBIE_OUTLINE_ADD_R) > 1.0f ? 1.0f : (fr + GameConfig::RenderConfig::ZOMBIE_OUTLINE_ADD_R);
        const float org = (fg + GameConfig::RenderConfig::ZOMBIE_OUTLINE_ADD_G) > 1.0f ? 1.0f : (fg + GameConfig::RenderConfig::ZOMBIE_OUTLINE_ADD_G);
        const float orb = (fb + GameConfig::RenderConfig::ZOMBIE_OUTLINE_ADD_B) > 1.0f ? 1.0f : (fb + GameConfig::RenderConfig::ZOMBIE_OUTLINE_ADD_B);

        App::DrawLine(x + ax, y + ay, x + bx, y + by, orr, org, orb);
        App::DrawLine(x + bx, y + by, x + cx, y + cy, orr, org, orb);
        App::DrawLine(x + cx, y + cy, x + ax, y + ay, orr, org, orb);

        // Inner panel (removed std::min)
        const float inner = GameConfig::RenderConfig::ZOMBIE_INNER_SCALE;

        const float ir1 = (fr + 0.18f) > 1.0f ? 1.0f : (fr + 0.18f);
        const float ig1 = (fg + 0.18f) > 1.0f ? 1.0f : (fg + 0.18f);
        const float ib1 = (fb + 0.22f) > 1.0f ? 1.0f : (fb + 0.22f);

        const float ir2 = (fr + 0.10f) > 1.0f ? 1.0f : (fr + 0.10f);
        const float ig2 = (fg + 0.10f) > 1.0f ? 1.0f : (fg + 0.10f);
        const float ib2 = (fb + 0.12f) > 1.0f ? 1.0f : (fb + 0.12f);

        const float ir3 = (fr + 0.05f) > 1.0f ? 1.0f : (fr + 0.05f);
        const float ig3 = (fg + 0.05f) > 1.0f ? 1.0f : (fg + 0.05f);
        const float ib3 = (fb + 0.06f) > 1.0f ? 1.0f : (fb + 0.06f);

        App::DrawTriangle(
            x + ax * inner, y + ay * inner, 0, 1,
            x + bx * inner, y + by * inner, 0, 1,
            x + cx * inner, y + cy * inner, 0, 1,
            ir1, ig1, ib1,
            ir2, ig2, ib2,
            ir3, ig3, ib3,
            false
        );
    }

    // Eyes
    {
        float ex1, ey1, ex2, ey2;
        Rot(-size * GameConfig::RenderConfig::ZOMBIE_EYE_OFFSET_X, -size * GameConfig::RenderConfig::ZOMBIE_EYE_OFFSET_Y, ex1, ey1);
        Rot(size * GameConfig::RenderConfig::ZOMBIE_EYE_OFFSET_X, -size * GameConfig::RenderConfig::ZOMBIE_EYE_OFFSET_Y, ex2, ey2);

        // Removed std::max
        const float eyeSize = size * GameConfig::RenderConfig::ZOMBIE_EYE_SIZE;
        const float eye = eyeSize > 1.0f ? eyeSize : 1.0f;

        App::DrawLine(x + ex1 - eye, y + ey1, x + ex1 + eye, y + ey1,
            GameConfig::RenderConfig::ZOMBIE_EYE_R,
            GameConfig::RenderConfig::ZOMBIE_EYE_G,
            GameConfig::RenderConfig::ZOMBIE_EYE_B);
        App::DrawLine(x + ex1, y + ey1 - eye, x + ex1, y + ey1 + eye,
            GameConfig::RenderConfig::ZOMBIE_EYE_R,
            GameConfig::RenderConfig::ZOMBIE_EYE_G,
            GameConfig::RenderConfig::ZOMBIE_EYE_B);

        App::DrawLine(x + ex2 - eye, y + ey2, x + ex2 + eye, y + ey2,
            GameConfig::RenderConfig::ZOMBIE_EYE_R,
            GameConfig::RenderConfig::ZOMBIE_EYE_G,
            GameConfig::RenderConfig::ZOMBIE_EYE_B);
        App::DrawLine(x + ex2, y + ey2 - eye, x + ex2, y + ey2 + eye,
            GameConfig::RenderConfig::ZOMBIE_EYE_R,
            GameConfig::RenderConfig::ZOMBIE_EYE_G,
            GameConfig::RenderConfig::ZOMBIE_EYE_B);
    }

    // Legs
    {
        const float lx = size * GameConfig::RenderConfig::ZOMBIE_LEG_LX;
        const float ly = size * GameConfig::RenderConfig::ZOMBIE_LEG_LY;

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
}

void WorldRenderer::RenderTacticalMinimap(const Player& player, const HiveSystem& hives) const
{
    const float mapX = 20.0f;
    const float mapY = 20.0f;
    const float mapW = 180.0f;
    const float mapH = 180.0f;
    const float worldSize = 2600.0f;
    const float scale = mapW / worldSize;

    for (float y = mapY; y < mapY + mapH; y += 2.0f)
    {
        App::DrawLine(mapX, y, mapX + mapW, y, 0.0f, 0.0f, 0.0f);
    }

    App::DrawLine(mapX - 2, mapY - 2, mapX + mapW + 2, mapY - 2, 0.30f, 0.70f, 0.90f);
    App::DrawLine(mapX + mapW + 2, mapY - 2, mapX + mapW + 2, mapY + mapH + 2, 0.30f, 0.70f, 0.90f);
    App::DrawLine(mapX + mapW + 2, mapY + mapH + 2, mapX - 2, mapY + mapH + 2, 0.30f, 0.70f, 0.90f);
    App::DrawLine(mapX - 2, mapY + mapH + 2, mapX - 2, mapY - 2, 0.30f, 0.70f, 0.90f);

    DrawRectOutline(mapX, mapY, mapX + mapW, mapY + mapH, 1.0f, 0.95f, 0.20f);

    const float centerX = mapX + mapW * 0.5f;
    const float centerY = mapY + mapH * 0.5f;

    auto WorldToMap = [&](float wx, float wy, float& mx, float& my)
    {
        mx = centerX + wx * scale;
        my = centerY + wy * scale;
    };

    const float bMin = GameConfig::BoundaryConfig::BOUNDARY_MIN;
    const float bMax = GameConfig::BoundaryConfig::BOUNDARY_MAX;

    float x1, y1, x2, y2;
    WorldToMap(bMin, bMin, x1, y1);
    WorldToMap(bMax, bMax, x2, y2);

    const float wallAlpha = 0.4f;
    DrawRectOutline(x1, y1, x2, y2, 
        0.65f * wallAlpha, 0.55f * wallAlpha, 0.15f * wallAlpha);

    const auto& hiveList = hives.GetHives();

    for (const auto& h : hiveList)
    {
        float mx, my;
        WorldToMap(h.x, h.y, mx, my);
        const float r = h.radius * scale * 0.8f;

        if (h.alive)
        {
            const int segments = 8;
            float prevX = mx + r;
            float prevY = my;

            for (int i = 1; i <= segments; i++)
            {
                const float angle = (GameConfig::MathConstants::TWO_PI * i) / segments;
                const float x = mx + std::cos(angle) * r;
                const float y = my + std::sin(angle) * r;
                App::DrawLine(prevX, prevY, x, y, 1.0f, 0.85f, 0.10f);
                prevX = x;
                prevY = y;
            }
        }
        else
        {
            const float xSize = r * 1.2f;
            App::DrawLine(mx - xSize, my - xSize, mx + xSize, my + xSize, 0.8f, 0.1f, 0.1f);
            App::DrawLine(mx + xSize, my - xSize, mx - xSize, my + xSize, 0.8f, 0.1f, 0.1f);
        }
    }

    float px, py;
    player.GetWorldPosition(px, py);
    
    float playerMapX, playerMapY;
    WorldToMap(px, py, playerMapX, playerMapY);

    const float cursorSize = 4.0f;
    
    App::DrawLine(playerMapX, playerMapY - cursorSize, playerMapX, playerMapY + cursorSize, 0.0f, 1.0f, 0.0f);
    App::DrawLine(playerMapX - cursorSize, playerMapY, playerMapX + cursorSize, playerMapY, 0.0f, 1.0f, 0.0f);
    
    const int cursorSegments = 8;
    const float cursorRadius = 3.0f;
    float prevX = playerMapX + cursorRadius;
    float prevY = playerMapY;
    
    for (int i = 1; i <= cursorSegments; i++)
    {
        const float angle = (GameConfig::MathConstants::TWO_PI * i) / cursorSegments;
        const float x = playerMapX + std::cos(angle) * cursorRadius;
        const float y = playerMapY + std::sin(angle) * cursorRadius;
        App::DrawLine(prevX, prevY, x, y, 0.0f, 1.0f, 0.0f);
        prevX = x;
        prevY = y;
    }

    App::Print(static_cast<int>(mapX), static_cast<int>(mapY + mapH + 5), 
        "MAP", 0.30f, 0.70f, 0.90f, GLUT_BITMAP_HELVETICA_10);
}

float WorldRenderer::Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}