#pragma once

// Forward declares to avoid heavy includes
class Player;
class NavGrid;
class ZombieSystem;
class CameraSystem;

class WorldRenderer
{
public:
    void RenderFrame(
        const CameraSystem& camera,
        Player& player,
        const NavGrid& nav,
        const ZombieSystem& zombies,
        bool densityView);

private:
    void RenderWorld(
        float offX, float offY,
        Player& player,
        const NavGrid& nav,
        const ZombieSystem& zombies,
        bool densityView);

    void RenderZombies2D(float offX, float offY, const ZombieSystem& zombies, bool densityView);
    void RenderZombiesIso(float offX, float offY, const ZombieSystem& zombies, bool densityView, float playerX, float playerY);


    // NEW: UI now takes hp/maxHp
    void RenderUI(int simCount, int drawnCount, int step, bool densityView, int hp, int maxHp);

    void DrawZombieTri(float x, float y, float size, float r, float g, float b, bool cull);

    // NEW: 3D-ish iso wedge
    void WorldRenderer::DrawIsoWedge(float sx, float sy, float size, float height, float r, float g, float b, float angleRad);


    static float Clamp01(float v);
};
