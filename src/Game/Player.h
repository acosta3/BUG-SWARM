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

    enum Anim { ANIM_FORWARDS, ANIM_BACKWARDS, ANIM_LEFT, ANIM_RIGHT };
    void SetAnimFromMove(float mx, float my);
};
