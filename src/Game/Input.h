#pragma once
#include "../ContestAPI/app.h"

struct InputState
{
    float moveX = 0.0f;
    float moveY = 0.0f;
    bool stopAnimPressed = false;
};

class InputSystem
{
public:
    void Update(float dt);
    const InputState& GetState() const { return state; }

private:
    int padIndex = 0;      // active controller slot (0-3)
    InputState state;

private:
    int FindActivePadIndex() const; // helper
};
