// Player.cpp
#include "Player.h"
#include "NavGrid.h"
#include <cmath>
#include <algorithm>

static float Clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

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

    // Baselines (normal form)
    baseSpeedPixelsPerSec = speedPixelsPerSec;
    baseMaxHealth = maxHealth;

    // Ensure stats match starting scale
    RecomputeStatsFromScale(sprite->GetScale());
}

float Player::GetScale() const
{
    if (!sprite) return 1.0f;
    return sprite->GetScale();
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

    float mx = moveX;
    float my = moveY;

    const float deadZone = 0.15f;
    if (std::fabs(mx) < deadZone) mx = 0.0f;
    if (std::fabs(my) < deadZone) my = 0.0f;

    const bool moving = (mx != 0.0f || my != 0.0f);

    if (moving)
    {
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

        // Normalize so diagonals are not faster
        float lenSq = mx * mx + my * my;
        if (lenSq > 1.0f)
        {
            float invLen = 1.0f / std::sqrt(lenSq);
            mx *= invLen;
            my *= invLen;
        }

        float x, y;
        sprite->GetPosition(x, y);

        // ---- FIX: substep movement so dt spikes do not cause "speed bursts" or collision skipping ----
        float dt = deltaTime / 1000.0f;

        // Safety cap so huge stalls do not explode simulation
        if (dt > 0.20f) dt = 0.20f;

        const float maxStep = 1.0f / 60.0f; // ~16.67ms
        int steps = (int)std::ceil(dt / maxStep);
        if (steps < 1) steps = 1;
        if (steps > 8) steps = 8;           // keep it cheap

        const float stepDt = dt / (float)steps;

        for (int i = 0; i < steps; ++i)
        {
            const float dx = mx * speedPixelsPerSec * stepDt;
            const float dy = my * speedPixelsPerSec * stepDt;
            MoveWithCollision(x, y, dx, dy);
        }

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

    sprite->SetPosition(wx, wy); // restore world position
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

    // scale speed: units per second
    const float scalePerSec = 1.0f;

    // ---- FIX: cap scaling dt so frame spikes do not cause big size and speed jumps ----
    float dt = deltaTime / 1000.0f;
    if (dt > 0.0333f) dt = 0.0333f; // cap to ~30fps for scaling responsiveness

    if (scaleUpHeld)   s += scalePerSec * dt;
    if (scaleDownHeld) s -= scalePerSec * dt;

    // clamp to desired range
    s = std::clamp(s, 0.4f, 2.0f);

    sprite->SetScale(s);

    // Scale changes drive speed and maxHP fairly
    RecomputeStatsFromScale(s);
}

void Player::RecomputeStatsFromScale(float s)
{
    // Tier targets
    const float smallS = 0.7f;
    const float bigS = 1.3f;

    const float smallSpeed = 1.25f;
    const float smallHP = 0.60f;

    const float normalSpeed = 1.00f;
    const float normalHP = 1.00f;

    const float bigSpeed = 0.75f;
    const float bigHP = 1.60f;

    float speedMult = 1.0f;
    float hpMult = 1.0f;

    if (s <= smallS)
    {
        speedMult = smallSpeed;
        hpMult = smallHP;
    }
    else if (s >= bigS)
    {
        speedMult = bigSpeed;
        hpMult = bigHP;
    }
    else
    {
        // Smooth blend from small -> normal -> big
        if (s < 1.0f)
        {
            float t = (s - smallS) / (1.0f - smallS);
            t = Clamp01(t);
            speedMult = Lerp(smallSpeed, normalSpeed, t);
            hpMult = Lerp(smallHP, normalHP, t);
        }
        else
        {
            float t = (s - 1.0f) / (bigS - 1.0f);
            t = Clamp01(t);
            speedMult = Lerp(normalSpeed, bigSpeed, t);
            hpMult = Lerp(normalHP, bigHP, t);
        }
    }

    // Apply speed
    speedPixelsPerSec = baseSpeedPixelsPerSec * speedMult;

    // Apply maxHP fairly: preserve HP percentage (no infinite healing)
    const int oldMax = maxHealth;
    int newMax = (int)std::lround((float)baseMaxHealth * hpMult);

    if (newMax < 1) newMax = 1;
    if (newMax > 999) newMax = 999;

    maxHealth = newMax;

    if (!dead)
    {
        float hpPct = (oldMax > 0) ? ((float)health / (float)oldMax) : 1.0f;
        int newHealth = (int)std::lround(hpPct * (float)maxHealth);

        if (newHealth < 1) newHealth = 1;
        if (newHealth > maxHealth) newHealth = maxHealth;

        health = newHealth;
    }
    else
    {
        health = 0;
    }
}

void Player::Heal(float amount)
{
    if (amount <= 0.0f) return;
    health += amount;
    if (health > maxHealth) health = maxHealth;
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

    // X axis
    float nx = x + dx;
    if (!CircleHitsBlocked(nx, y, r))
        x = nx;

    // Y axis
    float ny = y + dy;
    if (!CircleHitsBlocked(x, ny, r))
        y = ny;
}
