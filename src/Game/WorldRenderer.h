#pragma once
#include "../ContestAPI/app.h"
#include "IsoProjector.h"

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
    void RenderWorld(float offX, float offY, Player& player, const NavGrid& nav,
        const ZombieSystem& zombies, bool densityView);

    void RenderZombies2D(float offX, float offY, const ZombieSystem& zombies, bool densityView);
    void RenderZombiesIso(float offX, float offY, const ZombieSystem& zombies, bool densityView);

    void RenderUI(int simCount, int drawnCount, int step, bool densityView);

    void DrawZombieTri(float x, float y, float size, float r, float g, float b, bool cull);

    static float Clamp01(float v);
};
