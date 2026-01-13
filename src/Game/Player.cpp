#include "Player.h"
#include <cmath>

void Player::Init()
{
    sprite.reset(App::CreateSprite("./data/TestData/Test.bmp", 8, 4));
    sprite->SetPosition(400.0f, 400.0f);
   
    const float walkSpeed = 1.0f / 15.0f;
    const float idleSpeed = 1.0f; // irrelevant for 1 frame

    sprite->CreateAnimation(ANIM_WALK_BACK, walkSpeed, { 0,1,2,3,4,5,6,7 });
    sprite->CreateAnimation(ANIM_WALK_LEFT, walkSpeed, { 8,9,10,11,12,13,14,15 });
    sprite->CreateAnimation(ANIM_WALK_RIGHT, walkSpeed, { 16,17,18,19,20,21,22,23 });
    sprite->CreateAnimation(ANIM_WALK_FWD, walkSpeed, { 24,25,26,27,28,29,30,31 });

    // Idle = 1 frame (choose the standing pose per direction)
    sprite->CreateAnimation(ANIM_IDLE_BACK, idleSpeed, { 0 });
    sprite->CreateAnimation(ANIM_IDLE_LEFT, idleSpeed, { 8 });
    sprite->CreateAnimation(ANIM_IDLE_RIGHT, idleSpeed, { 16 });
    sprite->CreateAnimation(ANIM_IDLE_FWD, idleSpeed, { 24 });

}

void Player::Update(float deltaTime, const InputState& in)
{
    if (!sprite) return;

    // Advance sprite animation time
    sprite->Update(deltaTime);

    // Optional debug: hard stop animation
    if (in.stopAnimPressed)
    {
        sprite->SetAnimation(-1);
        return;
    }

    float mx = in.moveX;
    float my = in.moveY;

    // Deadzone (important for controllers)
    const float dead = 0.15f;
    if (std::fabs(mx) < dead) mx = 0.0f;
    if (std::fabs(my) < dead) my = 0.0f;

    const bool moving = (mx != 0.0f || my != 0.0f);

    // If moving: pick facing + play walk anim
    if (moving)
    {
        // Determine facing from dominant axis
        if (std::fabs(mx) > std::fabs(my))
            facing = (mx > 0.0f) ? FACE_RIGHT : FACE_LEFT;
        else
            facing = (my > 0.0f) ? FACE_FWD : FACE_BACK;

        switch (facing)
        {
        case FACE_RIGHT: sprite->SetAnimation(ANIM_WALK_RIGHT); break;
        case FACE_LEFT:  sprite->SetAnimation(ANIM_WALK_LEFT);  break;
        case FACE_FWD:   sprite->SetAnimation(ANIM_WALK_FWD);   break;
        case FACE_BACK:  sprite->SetAnimation(ANIM_WALK_BACK);  break;
        }

        // Normalize so diagonal isn't faster
        float lenSq = mx * mx + my * my;
        if (lenSq > 1.0f)
        {
            float invLen = 1.0f / std::sqrt(lenSq);
            mx *= invLen;
            my *= invLen;
        }

        // Move
        float x, y;
        sprite->GetPosition(x, y);

        const float dt = deltaTime / 1000.0f; // ms -> seconds
        x += mx * speedPixelsPerSec * dt;
        y += my * speedPixelsPerSec * dt;

        sprite->SetPosition(x, y);

        wasMovingLastFrame = true;
        return;
    }

    // Not moving: switch to idle only once when we STOP
    if (wasMovingLastFrame)
    {
        switch (facing)
        {
        case FACE_RIGHT: sprite->SetAnimation(ANIM_IDLE_RIGHT, true); break;
        case FACE_LEFT:  sprite->SetAnimation(ANIM_IDLE_LEFT, true); break;
        case FACE_FWD:   sprite->SetAnimation(ANIM_IDLE_FWD, true);  break;
        case FACE_BACK:  sprite->SetAnimation(ANIM_IDLE_BACK, true);  break;
        }

        
    }

    wasMovingLastFrame = false;
}



void Player::Render() const
{
    if (sprite)
        sprite->Draw();
}
