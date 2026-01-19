#pragma once

class MyGame;
class Player;
class HiveSystem;
class ZombieSystem;

class UIRenderer
{
public:
    static void RenderMenu(const MyGame& game, const HiveSystem& hives);
    static void RenderPauseOverlay(const Player& player, const HiveSystem& hives, const ZombieSystem& zombies);
    static void RenderWinOverlay(const Player& player, const ZombieSystem& zombies, int maxZombies);

private:
    static void DrawAnimatedBackground(float time);
    static void DrawTacticalMap(const HiveSystem& hives, float mapX, float mapY, float mapW, float mapH, float time);
};
                    