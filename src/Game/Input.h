#pragma once
#include "../ContestAPI/app.h"

struct InputState
{
    float moveX = 0.0f;   // -1..1
    float moveY = 0.0f;   // -1..1

    bool stopAnimPressed = false; // “just pressed”
};
namespace Input
{
    InputState Poll();
}
