///////////////////////////////////////////////////////////////////////////////
// Filename: GameTest.cpp
// Entry points called by the framework.
// Keep this file thin: forward into your Game class.
///////////////////////////////////////////////////////////////////////////////

#if BUILD_PLATFORM_WINDOWS
#include <windows.h>
#endif

#include "MyGame.h"          // or "MyGame.h" if you named it that
#include "../ContestAPI/app.h"

// File-private, lifetime = whole program
namespace
{
    MyGame g_game;
}

void Init()
{
    g_game.Init();
}

void Update(const float deltaTime)
{
    g_game.Update(deltaTime);
}

void Render()
{
    g_game.Render();
}

void Shutdown()
{
    g_game.Shutdown();
}
