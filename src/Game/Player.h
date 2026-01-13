#pragma once
#include "../ContestAPI/app.h"
#include "Input.h"
#include <memory>

class Player
{
public:
    void Init();
    void Update(float deltaTime, const InputState& in);
    void Render() const;

private:
    std::unique_ptr<CSimpleSprite> sprite;
    float speedPixelsPerSec = 200.0f;

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
