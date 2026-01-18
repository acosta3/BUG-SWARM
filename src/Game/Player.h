// Player.h
#pragma once
#include "../ContestAPI/app.h"

#include <memory>

class NavGrid;

class Player
{
public:
    void Init();
    void Update(float dtMs);
    void Render(float camOffsetX, float camOffsetY) const;

    // Input intent (fed by MyGame)
    void SetMoveInput(float x, float y) { moveX = x; moveY = y; }
    void SetStopAnimPressed(bool pressed) { stopAnimPressed = pressed; }

    void GetWorldPosition(float& outX, float& outY) const;

    // Respawn helpers
    void SetWorldPosition(float x, float y);
    void Revive(bool fullHeal = true);

    // Scale
    void ApplyScaleInput(bool scaleUpHeld, bool scaleDownHeld, float dtMs);

    void SetNavGrid(const NavGrid* g) { nav = g; }

    // Health
    void Heal(float amount);
    void TakeDamage(int amount);

    bool IsDead() const { return dead; }
    int  GetHealth() const { return health; }
    int  GetMaxHealth() const { return maxHealth; }

    float GetScale() const;

    // Invulnerability (ms)
    void GiveInvulnerability(float ms);
    bool IsInvulnerable() const { return invulnMs > 0.0f; }
    float GetInvulnMs() const { return invulnMs; }

private:
    void RecomputeStatsFromScale(float newScale);

    bool CircleHitsBlocked(float cx, float cy, float r) const;
    void MoveWithCollision(float& x, float& y, float dx, float dy);

private:
    // Health
    int  health = 200;
    int  maxHealth = 200;
    bool dead = false;

    // i-frames
    float invulnMs = 0.0f;

    std::unique_ptr<CSimpleSprite> sprite;

    // Movement
    float speedPixelsPerSec = 200.0f;
    float baseSpeedPixelsPerSec = 200.0f;

    // Must match initial maxHealth (so scaling math is consistent)
    int baseMaxHealth = 200;

    float moveX = 0.0f;
    float moveY = 0.0f;
    bool  stopAnimPressed = false;

    const NavGrid* nav = nullptr;

    enum Anim
    {
        ANIM_IDLE_BACK = 100,
        ANIM_IDLE_LEFT,
        ANIM_IDLE_RIGHT,
        ANIM_IDLE_FWD,

        ANIM_WALK_BACK,
        ANIM_WALK_LEFT,
        ANIM_WALK_RIGHT,
        ANIM_WALK_FWD,
    };

    enum Facing { FACE_BACK, FACE_LEFT, FACE_RIGHT, FACE_FWD };
    Facing facing = FACE_FWD;

    bool wasMovingLastFrame = false;
};
