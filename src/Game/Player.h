// Player.h - AAA Quality Version
#pragma once
#include "../ContestAPI/app.h"
#include <memory>

class NavGrid;

class Player
{
public:
    // Initialization
    void Init();

    // Updates
    void Update(float deltaTime);

    // Rendering
    void Render(float camOffsetX, float camOffsetY) const;

    // Input
    void SetMoveInput(float x, float y) { moveX = x; moveY = y; }
    void SetStopAnimPressed(bool pressed) { stopAnimPressed = pressed; }
    void ApplyScaleInput(bool scaleUpHeld, bool scaleDownHeld, float deltaTime);

    // Position
    void GetWorldPosition(float& outX, float& outY) const;
    void SetWorldPosition(float x, float y);

    // Health & Combat
    void Heal(float amount);
    void TakeDamage(int amount);
    void Revive(bool fullHeal = true);
    void GiveInvulnerability(float ms) { if (ms > invulnMs) invulnMs = ms; }

    // State queries
    bool IsDead() const { return dead; }
    bool IsInvulnerable() const { return invulnMs > 0.0f; }
    int GetHealth() const { return health; }
    int GetMaxHealth() const { return maxHealth; }
    float GetInvulnMs() const { return invulnMs; }
    float GetScale() const;

    // Navigation
    void SetNavGrid(const NavGrid* g) { nav = g; }

private:
    // Animation IDs
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

private:
    void RecomputeStatsFromScale(float newScale);
    bool CircleHitsBlocked(float cx, float cy, float r) const;
    void MoveWithCollision(float& x, float& y, float dx, float dy);

private:
    // Health system
    int health = 200;
    int maxHealth = 200;
    bool dead = false;
    float invulnMs = 0.0f;

    // Rendering
    std::unique_ptr<CSimpleSprite> sprite;
    Facing facing = FACE_FWD;
    bool wasMovingLastFrame = false;

    // Movement
    float speedPixelsPerSec = 200.0f;
    float baseSpeedPixelsPerSec = 200.0f;
    float moveX = 0.0f;
    float moveY = 0.0f;
    bool stopAnimPressed = false;

    // Stats
    int baseMaxHealth = 200;

    // Navigation
    const NavGrid* nav = nullptr;
};