// Player.cpp - AAA Quality Version
#include "Player.h"
#include "NavGrid.h"
#include "GameConfig.h"

#include <cmath>
#include <algorithm>

using namespace GameConfig;

// -------------------- Utilities --------------------
namespace
{
    float Clamp01(float v)
    {
        return std::clamp(v, 0.0f, 1.0f);  // ✅ FIXED: Was ".0f" instead of "1.0f"
    }

    float Lerp(float a, float b, float t)
    {
        return a + (b - a) * t;
    }
}

// -------------------- Initialization --------------------
void Player::Init()
{
    sprite.reset(App::CreateSprite(
        PlayerConfig::SPRITE_PATH,
        PlayerConfig::SPRITE_COLUMNS,
        PlayerConfig::SPRITE_ROWS
    ));

    sprite->SetPosition(PlayerConfig::INITIAL_X, PlayerConfig::INITIAL_Y);

    // Create walk animations
    sprite->CreateAnimation(ANIM_WALK_BACK, PlayerConfig::WALK_ANIM_SPEED, { 0,1,2,3,4,5,6,7 });
    sprite->CreateAnimation(ANIM_WALK_LEFT, PlayerConfig::WALK_ANIM_SPEED, { 8,9,10,11,12,13,14,15 });
    sprite->CreateAnimation(ANIM_WALK_RIGHT, PlayerConfig::WALK_ANIM_SPEED, { 16,17,18,19,20,21,22,23 });
    sprite->CreateAnimation(ANIM_WALK_FWD, PlayerConfig::WALK_ANIM_SPEED, { 24,25,26,27,28,29,30,31 });

    // Create idle animations
    sprite->CreateAnimation(ANIM_IDLE_BACK, PlayerConfig::IDLE_ANIM_SPEED, { 0 });
    sprite->CreateAnimation(ANIM_IDLE_LEFT, PlayerConfig::IDLE_ANIM_SPEED, { 8 });
    sprite->CreateAnimation(ANIM_IDLE_RIGHT, PlayerConfig::IDLE_ANIM_SPEED, { 16 });
    sprite->CreateAnimation(ANIM_IDLE_FWD, PlayerConfig::IDLE_ANIM_SPEED, { 24 });

    baseSpeedPixelsPerSec = speedPixelsPerSec;
    baseMaxHealth = maxHealth;

    RecomputeStatsFromScale(sprite->GetScale());
}

// -------------------- Position --------------------
float Player::GetScale() const
{
    if (!sprite)
        return 1.0f;
    return sprite->GetScale();
}

void Player::SetWorldPosition(float x, float y)
{
    if (!sprite)
        return;
    sprite->SetPosition(x, y);
}

void Player::GetWorldPosition(float& outX, float& outY) const
{
    if (!sprite)
    {
        outX = 0.0f;
        outY = 0.0f;
        return;
    }
    sprite->GetPosition(outX, outY);
}

// -------------------- Health & Combat --------------------
void Player::Revive(bool fullHeal)
{
    dead = false;
    stopAnimPressed = false;
    wasMovingLastFrame = false;
    invulnMs = 0.0f;

    if (fullHeal)
        health = maxHealth;
    else
        health = std::clamp(health, 1, maxHealth);

    if (sprite)
        sprite->SetAnimation(ANIM_IDLE_FWD, true);
}

void Player::Heal(float amount)
{
    if (amount <= 0.0f)
        return;

    health += static_cast<int>(std::lround(amount));
    if (health > maxHealth)
        health = maxHealth;
}

void Player::TakeDamage(int amount)
{
    if (dead)
        return;

    // Invulnerability frames
    if (invulnMs > 0.0f)
        return;

    health -= amount;
    if (health < 0)
        health = 0;

    if (health == 0)
    {
        dead = true;
        if (sprite)
            sprite->SetAnimation(-1);
    }
}

// -------------------- Update --------------------
void Player::Update(float deltaTime)
{
    if (!sprite)
        return;

    // Tick invulnerability timer
    if (invulnMs > 0.0f)
    {
        invulnMs -= deltaTime;
        if (invulnMs < 0.0f)
            invulnMs = 0.0f;
    }

    sprite->Update(deltaTime);

    if (stopAnimPressed)
    {
        sprite->SetAnimation(-1);
        return;
    }

    // Apply deadzone to input
    float mx = moveX;
    float my = moveY;

    if (std::fabs(mx) < PlayerConfig::INPUT_DEADZONE)
        mx = 0.0f;
    if (std::fabs(my) < PlayerConfig::INPUT_DEADZONE)
        my = 0.0f;

    const bool moving = (mx != 0.0f || my != 0.0f);

    if (moving)
    {
        // Determine facing direction
        if (std::fabs(mx) > std::fabs(my))
            facing = (mx > 0.0f) ? FACE_RIGHT : FACE_LEFT;
        else
            facing = (my > 0.0f) ? FACE_FWD : FACE_BACK;

        // Set walk animation
        switch (facing)
        {
        case FACE_RIGHT: sprite->SetAnimation(ANIM_WALK_RIGHT); break;
        case FACE_LEFT:  sprite->SetAnimation(ANIM_WALK_LEFT);  break;
        case FACE_FWD:   sprite->SetAnimation(ANIM_WALK_FWD);   break;
        case FACE_BACK:  sprite->SetAnimation(ANIM_WALK_BACK);  break;
        }

        // Normalize movement vector
        const float lenSq = mx * mx + my * my;
        if (lenSq > 1.0f)
        {
            const float invLen = 1.0f / std::sqrt(lenSq);
            mx *= invLen;
            my *= invLen;
        }

        // Get current position
        float x, y;
        sprite->GetPosition(x, y);

        // Clamp delta time to prevent tunneling
        float dt = deltaTime / 1000.0f;
        if (dt > PlayerConfig::MAX_DELTA_TIME)
            dt = PlayerConfig::MAX_DELTA_TIME;

        // Substep movement for smooth collision
        int steps = static_cast<int>(std::ceil(dt / PlayerConfig::MAX_SUBSTEP));
        steps = std::clamp(steps, PlayerConfig::MIN_SUBSTEPS, PlayerConfig::MAX_SUBSTEPS);

        const float stepDt = dt / static_cast<float>(steps);

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

    // Not moving - set idle animation if just stopped
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

// -------------------- Rendering --------------------
void Player::Render(float camOffsetX, float camOffsetY) const
{
    if (!sprite)
        return;

    float wx, wy;
    sprite->GetPosition(wx, wy);

    sprite->SetPosition(wx - camOffsetX, wy - camOffsetY);
    sprite->Draw();
    sprite->SetPosition(wx, wy);
}

// -------------------- Scale System --------------------
void Player::ApplyScaleInput(bool scaleUpHeld, bool scaleDownHeld, float deltaTime)
{
    if (!sprite)
        return;

    float s = sprite->GetScale();

    float dt = deltaTime / 1000.0f;
    if (dt > PlayerConfig::SCALE_DT_MAX)
        dt = PlayerConfig::SCALE_DT_MAX;

    if (scaleUpHeld)
        s += PlayerConfig::SCALE_PER_SECOND * dt;
    if (scaleDownHeld)
        s -= PlayerConfig::SCALE_PER_SECOND * dt;

    s = std::clamp(s, PlayerConfig::SCALE_MIN, PlayerConfig::SCALE_MAX);

    sprite->SetScale(s);
    RecomputeStatsFromScale(s);
}

void Player::RecomputeStatsFromScale(float s)
{
    float speedMult = 1.0f;
    float hpMult = 1.0f;

    if (s <= PlayerConfig::SMALL_SCALE)
    {
        speedMult = PlayerConfig::SMALL_SPEED_MULT;
        hpMult = PlayerConfig::SMALL_HP_MULT;
    }
    else if (s >= PlayerConfig::BIG_SCALE)
    {
        speedMult = PlayerConfig::BIG_SPEED_MULT;
        hpMult = PlayerConfig::BIG_HP_MULT;
    }
    else
    {
        if (s < 1.0f)
        {
            float t = (s - PlayerConfig::SMALL_SCALE) / (1.0f - PlayerConfig::SMALL_SCALE);
            t = Clamp01(t);
            speedMult = Lerp(PlayerConfig::SMALL_SPEED_MULT, PlayerConfig::NORMAL_SPEED_MULT, t);
            hpMult = Lerp(PlayerConfig::SMALL_HP_MULT, PlayerConfig::NORMAL_HP_MULT, t);
        }
        else
        {
            float t = (s - 1.0f) / (PlayerConfig::BIG_SCALE - 1.0f);
            t = Clamp01(t);
            speedMult = Lerp(PlayerConfig::NORMAL_SPEED_MULT, PlayerConfig::BIG_SPEED_MULT, t);
            hpMult = Lerp(PlayerConfig::NORMAL_HP_MULT, PlayerConfig::BIG_HP_MULT, t);
        }
    }

    speedPixelsPerSec = baseSpeedPixelsPerSec * speedMult;

    const int oldMax = maxHealth;
    int newMax = static_cast<int>(std::lround(static_cast<float>(baseMaxHealth) * hpMult));

    newMax = std::clamp(newMax, PlayerConfig::MIN_HEALTH, PlayerConfig::MAX_HEALTH_CAP);
    maxHealth = newMax;

    if (!dead)
    {
        const float hpPct = (oldMax > 0) ? (static_cast<float>(health) / static_cast<float>(oldMax)) : 1.0f;
        int newHealth = static_cast<int>(std::lround(hpPct * static_cast<float>(maxHealth)));

        newHealth = std::clamp(newHealth, PlayerConfig::MIN_HEALTH, maxHealth);
        health = newHealth;
    }
    else
    {
        health = 0;
    }
}

// -------------------- Collision --------------------
bool Player::CircleHitsBlocked(float cx, float cy, float r) const
{
    if (!nav)
        return false;

    // Check 8 points around circle AABB
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
    const float r = PlayerConfig::BASE_COLLISION_RADIUS * s;

    // Try X movement
    const float nx = x + dx;
    if (!CircleHitsBlocked(nx, y, r))
        x = nx;

    // Try Y movement
    const float ny = y + dy;
    if (!CircleHitsBlocked(x, ny, r))
        y = ny;
}