#include "Player.h"
#include <cmath>

void Player::Init()
{
    sprite.reset(App::CreateSprite("./data/TestData/Test.bmp", 8, 4));
    sprite->SetPosition(400.0f, 400.0f);
    const float speed = 1.0f / 15.0f;
    sprite->CreateAnimation(ANIM_BACKWARDS, speed, { 0,1,2,3,4,5,6,7 });
    sprite->CreateAnimation(ANIM_LEFT, speed, { 8,9,10,11,12,13,14,15 });
    sprite->CreateAnimation(ANIM_RIGHT, speed, { 16,17,18,19,20,21,22,23 });
    sprite->CreateAnimation(ANIM_FORWARDS, speed, { 24,25,26,27,28,29,30,31 });
    sprite->SetScale(1.0f);
}

void Player::Update(float deltaTime, const InputState& in)
{
    if (!sprite)
        return;

    // Update animation timer
    sprite->Update(deltaTime);

    // Stop animation (edge-triggered)
    if (in.stopAnimPressed)
    {
        sprite->SetAnimation(-1);
        return;
    }

    // No movement input
    if (in.moveX == 0.0f && in.moveY == 0.0f)
        return;

    float x, y;
    sprite->GetPosition(x, y);

    const float dt = deltaTime / 1000.0f; // ms to seconds

    x += in.moveX * speedPixelsPerSec * dt;
    y += in.moveY * speedPixelsPerSec * dt;

    sprite->SetPosition(x, y);

    SetAnimFromMove(in.moveX, in.moveY);
}

void Player::SetAnimFromMove(float mx, float my)
{
    if (!sprite)
        return;

    if (std::fabs(mx) > std::fabs(my))
    {
        sprite->SetAnimation(mx > 0.0f ? ANIM_RIGHT : ANIM_LEFT);
    }
    else
    {
        sprite->SetAnimation(my > 0.0f ? ANIM_FORWARDS : ANIM_BACKWARDS);
    }
}

void Player::Render() const
{
    if (sprite)
        sprite->Draw();
}
