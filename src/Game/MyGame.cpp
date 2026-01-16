#include "MyGame.h"
#include "../ContestAPI/app.h"

void MyGame::Init()
{
    player.Init();
    camera.Init(1024.0f, 768.0f); // matches APP_VIRTUAL_WIDTH/HEIGHT

}

void MyGame::Update(float deltaTime)
{
    input.Update(deltaTime);
    const InputState& in = input.GetState();

    player.SetMoveInput(in.moveX, in.moveY);
    player.SetStopAnimPressed(in.stopAnimPressed);

    player.Update(deltaTime);

    float px, py;
    player.GetWorldPosition(px, py);
    camera.Follow(px, py);
    camera.Update(deltaTime);

}

void MyGame::Render()
{
    player.Render(camera.GetOffsetX(), camera.GetOffsetY());
    App::Print(100, 100, "Sample Text");

    
}

void MyGame::Shutdown()
{
    // Nothing required right now:
    // - Player owns sprite via unique_ptr
    // - InputSystem has no heap allocations
}
