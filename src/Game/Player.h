#pragma once
#include "../ContestAPI/app.h"
#include <memory>

class NavGrid;

class Player
{
public:
    void Init();
    void Update(float deltaTime);
    void Render(float camOffsetX, float camOffsetY) const;

    // Input intent (fed by MyGame)
    void SetMoveInput(float x, float y) { moveX = x; moveY = y; }
    void SetStopAnimPressed(bool pressed) { stopAnimPressed = pressed; }

    void GetWorldPosition(float& outX, float& outY) const;

    void ApplyScaleInput(bool scaleUpHeld, bool scaleDownHeld, float deltaTime);

    void SetNavGrid(const NavGrid* g) { nav = g; }

private:
    std::unique_ptr<CSimpleSprite> sprite;
    float speedPixelsPerSec = 200.0f;

    float moveX = 0.0f;
    float moveY = 0.0f;
    bool stopAnimPressed = false;

    const NavGrid* nav = nullptr;

    bool CircleHitsBlocked(float cx, float cy, float r) const;
    void MoveWithCollision(float& x, float& y, float dx, float dy);

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
