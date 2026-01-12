#include "Input.h"
#include <algorithm> 
#include <cmath>

static float AxisFromKeys(App::Key neg, App::Key pos)
{
    float v = 0.0f;
    if (App::IsKeyPressed(neg)) v -= 1.0f;
    if (App::IsKeyPressed(pos)) v += 1.0f;
    return v;
}

InputState Input::Poll()
{
    InputState in;

    const CController& pad = App::GetController();

    // Keyboard axes
    const float kx = AxisFromKeys(App::KEY_A, App::KEY_D);
    const float ky = AxisFromKeys(App::KEY_S, App::KEY_W);

    // Controller axes (with deadzone)
    float sx = pad.GetLeftThumbStickX();
    float sy = pad.GetLeftThumbStickY();

    const float DEAD = 0.5f;
    if (std::fabs(sx) < DEAD) sx = 0.0f;
    if (std::fabs(sy) < DEAD) sy = 0.0f;

    // Combine (no priority)
    in.moveX = std::clamp(kx + sx, -1.0f, 1.0f);
    in.moveY = std::clamp(ky + sy, -1.0f, 1.0f);

    // Prevent faster diagonal movement
    float lenSq = in.moveX * in.moveX + in.moveY * in.moveY;
    if (lenSq > 1.0f)
    {
        float invLen = 1.0f / std::sqrt(lenSq);
        in.moveX *= invLen;
        in.moveY *= invLen;
    }


    // “Stop animation” action from either device
    in.stopAnimPressed =
        pad.CheckButton(App::BTN_A, true) || App::IsKeyPressed(App::KEY_5);

    return in;
}
