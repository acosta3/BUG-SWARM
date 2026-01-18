// WorldRenderer.h
#pragma once

class Player;
class NavGrid;
class ZombieSystem;
class CameraSystem;
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
        bool densityView);

private:
    void RenderWorld(
        float offX, float offY,
        Player& player,
        const NavGrid& nav,
        const ZombieSystem& zombies,
        const HiveSystem& hives,
        const AttackSystem& attacks,
        bool densityView);

    void RenderZombies2D(float offX, float offY, const ZombieSystem& zombies, bool densityView);

    void RenderUI(
        int simCount, int maxCount,
        int drawnCount, int step,
        bool densityView,
        int hp, int maxHp,
        float pulseCdMs, float slashCdMs, float meteorCdMs);

private:
    // UI helpers (line-based, like HiveSystem::Render)
    void DrawBarLines(float x, float y, float w, float h, float t,
        float bgR, float bgG, float bgB,
        float fillR, float fillG, float fillB) const;

    void DrawRectOutline(float x0, float y0, float x1, float y1, float r, float g, float b) const;

private:
    void DrawZombieTri(float x, float y, float size, float r, float g, float b);

    static float Clamp01(float v);
};
