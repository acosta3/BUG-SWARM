// WorldRenderer.h
#pragma once

class CameraSystem;
class Player;
class NavGrid;
class ZombieSystem;
class HiveSystem;
class AttackSystem;

class WorldRenderer
{
public:
    void RenderFrame(
        const CameraSystem& camera,
        Player& player,
        const NavGrid& nav,
        const ZombieSystem& zombies,
        const HiveSystem& hives,
        const AttackSystem& attacks,
        float dtMs,
        bool densityView);

    void NotifyKills(int kills);

private:
    void RenderWorld(
        float offX, float offY,
        Player& player,
        const NavGrid& nav,
        const ZombieSystem& zombies,
        const HiveSystem& hives,
        const AttackSystem& attacks,
        float dtMs,
        bool densityView);

    void RenderZombies2D(float offX, float offY, const ZombieSystem& zombies, bool densityView);

    void RenderUI(
        int simCount, int maxCount,
        int drawnCount, int step,
        bool densityView,
        int hp, int maxHp,
        int hivesAlive, int hivesTotal,
        float pulseCdMs, float slashCdMs, float meteorCdMs);

    void RenderKillPopupOverPlayer(float playerScreenX, float playerScreenY, float dtMs);

    // ✅ NEW: Render live tactical minimap
    void RenderTacticalMinimap(const Player& player, const HiveSystem& hives) const;

    void DrawRectOutline(float x0, float y0, float x1, float y1, float r, float g, float b) const;

    void DrawBarLines(
        float x, float y, float w, float h, float t,
        float bgR, float bgG, float bgB,
        float fillR, float fillG, float fillB) const;

    static void DrawZombieTri(float x, float y, float size, float angleRad, float r, float g, float b);
    static float Clamp01(float v);

private:
    int   killPopupCount = 0;
    float killPopupTimeMs = 0.0f;

    // NEW: global render time for wiggle
    float animTimeSec = 0.0f;
};