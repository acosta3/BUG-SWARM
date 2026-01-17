#include "Player.h"
#include "NavGrid.h"
#include "IsoProjector.h"
#include <cmath>

void Player::Init()
{
    sprite.reset(App::CreateSprite("./data/TestData/Test.bmp", 8, 4));
    sprite->SetPosition(400.0f, 400.0f);

    const float walkSpeed = 1.0f / 15.0f;
    const float idleSpeed = 1.0f;

    sprite->CreateAnimation(ANIM_WALK_BACK, walkSpeed, { 0,1,2,3,4,5,6,7 });
    sprite->CreateAnimation(ANIM_WALK_LEFT, walkSpeed, { 8,9,10,11,12,13,14,15 });
    sprite->CreateAnimation(ANIM_WALK_RIGHT, walkSpeed, { 16,17,18,19,20,21,22,23 });
    sprite->CreateAnimation(ANIM_WALK_FWD, walkSpeed, { 24,25,26,27,28,29,30,31 });

    sprite->CreateAnimation(ANIM_IDLE_BACK, idleSpeed, { 0 });
    sprite->CreateAnimation(ANIM_IDLE_LEFT, idleSpeed, { 8 });
    sprite->CreateAnimation(ANIM_IDLE_RIGHT, idleSpeed, { 16 });
    sprite->CreateAnimation(ANIM_IDLE_FWD, idleSpeed, { 24 });
}

void Player::Update(float deltaTime)
{
    if (!sprite) return;

    sprite->Update(deltaTime);

    if (stopAnimPressed)
    {
        sprite->SetAnimation(-1);
        return;
    }

    // movement uses WORLD input (already mapped in MyGame)
    float mx = moveX;
    float my = moveY;

    const float deadZone = 0.15f;
    if (std::fabs(mx) < deadZone) mx = 0.0f;
    if (std::fabs(my) < deadZone) my = 0.0f;

    const bool moving = (mx != 0.0f || my != 0.0f);

    // animation facing uses VIEW input (screen direction)
    float vx = viewX;
    float vy = viewY;
    if (std::fabs(vx) < deadZone) vx = 0.0f;
    if (std::fabs(vy) < deadZone) vy = 0.0f;

    const bool viewMoving = (vx != 0.0f || vy != 0.0f);

    if (viewMoving)
    {
        if (std::fabs(vx) > std::fabs(vy))
            facing = (vx > 0.0f) ? FACE_RIGHT : FACE_LEFT;
        else
            facing = (vy > 0.0f) ? FACE_FWD : FACE_BACK;
    }

    if (moving)
    {
        switch (facing)
        {
        case FACE_RIGHT: sprite->SetAnimation(ANIM_WALK_RIGHT); break;
        case FACE_LEFT:  sprite->SetAnimation(ANIM_WALK_LEFT);  break;
        case FACE_FWD:   sprite->SetAnimation(ANIM_WALK_FWD);   break;
        case FACE_BACK:  sprite->SetAnimation(ANIM_WALK_BACK);  break;
        }

        float lenSq = mx * mx + my * my;
        if (lenSq > 1.0f)
        {
            const float invLen = 1.0f / std::sqrt(lenSq);
            mx *= invLen;
            my *= invLen;
        }

        float x, y;
        sprite->GetPosition(x, y);

        const float dt = deltaTime / 1000.0f;
        const float dx = mx * speedPixelsPerSec * dt;
        const float dy = my * speedPixelsPerSec * dt;

        MoveWithCollision(x, y, dx, dy);
        sprite->SetPosition(x, y);

        wasMovingLastFrame = true;
        return;
    }

    if (wasMovingLastFrame)
    {
        switch (facing)
        {
        case FACE_RIGHT: sprite->SetAnimation(ANIM_IDLE_RIGHT, true); break;
        case FACE_LEFT:  sprite->SetAnimation(ANIM_IDLE_LEFT, true); break;
        case FACE_FWD:   sprite->SetAnimation(ANIM_IDLE_FWD, true); break;
        case FACE_BACK:  sprite->SetAnimation(ANIM_IDLE_BACK, true); break;
        }
    }

    wasMovingLastFrame = false;
}

void Player::Render(float camOffsetX, float camOffsetY) const
{
    if (!sprite) return;

    float wx, wy;
    sprite->GetPosition(wx, wy);

    sprite->SetPosition(wx - camOffsetX, wy - camOffsetY);
    sprite->Draw();

    sprite->SetPosition(wx, wy);
}

void Player::RenderIso(const IsoProjector& iso) const
{
    if (!sprite) return;

    float wx, wy;
    sprite->GetPosition(wx, wy);

    float sx, sy;
    iso.WorldToScreen(wx, wy, 0.0f, sx, sy);

    sprite->SetPosition(sx, sy);
    sprite->Draw();

    sprite->SetPosition(wx, wy);
}

void Player::GetWorldPosition(float& outX, float& outY) const
{
    if (!sprite) { outX = 0.0f; outY = 0.0f; return; }
    sprite->GetPosition(outX, outY);
}

void Player::ApplyScaleInput(bool scaleUpHeld, bool scaleDownHeld, float deltaTime)
{
    if (!sprite) return;

    float s = sprite->GetScale();

    const float scalePerSec = 1.0f;
    const float dt = deltaTime / 1000.0f;

    if (scaleUpHeld)   s += scalePerSec * dt;
    if (scaleDownHeld) s -= scalePerSec * dt;

    if (s < 0.1f) s = 0.1f;
    if (s > 5.0f) s = 5.0f;

    sprite->SetScale(s);
}

void Player::TakeDamage(int amount)
{
    if (dead) return;

    health -= amount;
    if (health < 0) health = 0;

    if (health == 0)
    {
        dead = true;
        if (sprite) sprite->SetAnimation(-1);
    }
}

bool Player::CircleHitsBlocked(float cx, float cy, float r) const
{
    if (!nav) return false;

    const float minX = cx - r;
    const float maxX = cx + r;
    const float minY = cy - r;
    const float maxY = cy + r;

    const float midX = cx;
    const float midY = cy;

    return nav->IsBlockedWorld(minX, minY) ||
        nav->IsBlockedWorld(midX, minY) ||
        nav->IsBlockedWorld(maxX, minY) ||
        nav->IsBlockedWorld(minX, midY) ||
        nav->IsBlockedWorld(maxX, midY) ||
        nav->IsBlockedWorld(minX, maxY) ||
        nav->IsBlockedWorld(midX, maxY) ||
        nav->IsBlockedWorld(maxX, maxY);
}

void Player::MoveWithCollision(float& x, float& y, float dx, float dy)
{
    const float s = sprite ? sprite->GetScale() : 1.0f;

    const float baseR = 40.0f;
    const float r = baseR * s;

    float nx = x + dx;
    if (!CircleHitsBlocked(nx, y, r))
        x = nx;

    float ny = y + dy;
    if (!CircleHitsBlocked(x, ny, r))
        y = ny;
}
