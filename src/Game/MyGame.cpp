#include "MyGame.h"
#include "../ContestAPI/app.h"

void MyGame::Init()
{
    player.Init();
}

void MyGame::Update(float deltaTime)
{
    input.Update(deltaTime);
    const InputState& in = input.GetState();

    player.SetMoveInput(in.moveX, in.moveY);
    player.SetStopAnimPressed(in.stopAnimPressed);

    player.Update(deltaTime);
}

void MyGame::Render()
{
    player.Render();
    App::Print(100, 100, "Sample Text");
}

void MyGame::Shutdown()
{
    // Nothing required right now:
    // - Player owns sprite via unique_ptr
    // - InputSystem has no heap allocations
}
