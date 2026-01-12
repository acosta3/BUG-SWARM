#include "MyGame.h"
#include "Input.h"
#include "../ContestAPI/app.h"

void MyGame::Init()
{
    player.Init();
}

void MyGame::Update(float deltaTime)
{
    InputState in = Input::Poll();
    player.Update(deltaTime, in);
}

void MyGame::Render()
{
    player.Render();
    App::Print(100, 100, "Sample Text");
}

void MyGame::Shutdown()
{
    // If Player uses unique_ptr, nothing required.
    // 
    // 
    // If Player uses raw pointer, call player.Shutdown().
}
