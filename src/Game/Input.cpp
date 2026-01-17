// Input.cpp
#include "Input.h"
#include "../ContestAPI/app.h"

#include <algorithm>
#include <cmath>

static float AxisFromKeys(App::Key neg, App::Key pos)
{
    float v = 0.0f;
    if (App::IsKeyPressed(neg)) v -= 1.0f;
    if (App::IsKeyPressed(pos)) v += 1.0f;
    return v;
}

int InputSystem::FindActivePadIndex() const
{
    const float STICK_EPS = 0.15f;
    const float TRIG_EPS = 0.20f;

    for (int i = 0; i < 4; i++)
    {
        const CController& p = CSimpleControllers::GetInstance().GetController(i);

        const float lx = p.GetLeftThumbStickX();
        const float ly = p.GetLeftThumbStickY();
        const float rx = p.GetRightThumbStickX();
        const float ry = p.GetRightThumbStickY();

        const float lt = p.GetLeftTrigger();
        const float rt = p.GetRightTrigger();

        const bool anyStick =
            (std::fabs(lx) > STICK_EPS) || (std::fabs(ly) > STICK_EPS) ||
            (std::fabs(rx) > STICK_EPS) || (std::fabs(ry) > STICK_EPS);

        const bool anyTrig =
            (lt > TRIG_EPS) || (rt > TRIG_EPS);

        const bool anyButton =
            p.CheckButton(App::BTN_A, false) ||
            p.CheckButton(App::BTN_B, false) ||
            p.CheckButton(App::BTN_X, false) ||
            p.CheckButton(App::BTN_Y, false) ||
            p.CheckButton(App::BTN_START, false) ||
            p.CheckButton(App::BTN_BACK, false) ||
            p.CheckButton(App::BTN_DPAD_LEFT, false) ||
            p.CheckButton(App::BTN_DPAD_RIGHT, false) ||
            p.CheckButton(App::BTN_DPAD_DOWN, false) ||
            p.CheckButton(App::BTN_LBUMPER, false) ||
            p.CheckButton(App::BTN_RBUMPER, false);

        if (anyStick || anyTrig || anyButton)
            return i;
    }

    return padIndex;
}

void InputSystem::Update(float dt)
{
    state = {};

    // If input is disabled, return neutral state
    if (!inputEnabled)
    {
        // Reset edge detectors so inputs don't "pop" when re-enabled
        prevV = prevSpace = prevQ = prevE = false;
        return;
    }

    // Keyboard axes
    const float kx = AxisFromKeys(App::KEY_A, App::KEY_D);
    const float ky = AxisFromKeys(App::KEY_S, App::KEY_W);

    // Active controller
    padIndex = FindActivePadIndex();
    const CController& pad = CSimpleControllers::GetInstance().GetController(padIndex);

    // Controller axes (deadzone)
    float sx = pad.GetLeftThumbStickX();
    float sy = pad.GetLeftThumbStickY();

    const float DEAD = 0.5f;
    if (std::fabs(sx) < DEAD) sx = 0.0f;
    if (std::fabs(sy) < DEAD) sy = 0.0f;

    // Combine
    state.moveX = std::clamp(kx + sx, -1.0f, 1.0f);
    state.moveY = std::clamp(ky + sy, -1.0f, 1.0f);

    // Prevent faster diagonal
    float lenSq = state.moveX * state.moveX + state.moveY * state.moveY;
    if (lenSq > 1.0f)
    {
        float invLen = 1.0f / std::sqrt(lenSq);
        state.moveX *= invLen;
        state.moveY *= invLen;
    }

    // A: just pressed
    state.stopAnimPressed = pad.CheckButton(App::BTN_A, true);

    // Toggle view: V key just-pressed OR Dpad Down
    bool vNow = App::IsKeyPressed(App::KEY_V);
    state.toggleViewPressed = (vNow && !prevV);
    prevV = vNow;
    state.toggleViewPressed = state.toggleViewPressed || pad.CheckButton(App::BTN_DPAD_DOWN, true);

    // Pulse: Space just-pressed OR controller B
    bool spaceNow = App::IsKeyPressed(App::KEY_SPACE);
    state.pulsePressed = (spaceNow && !prevSpace);
    prevSpace = spaceNow;
    state.pulsePressed = state.pulsePressed || pad.CheckButton(App::BTN_B, true);

    // Slash: Q just-pressed OR controller X
    bool qNow = App::IsKeyPressed(App::KEY_Q);
    state.slashPressed = (qNow && !prevQ);
    prevQ = qNow;
    state.slashPressed = state.slashPressed || pad.CheckButton(App::BTN_X, true);

    // Meteor: E just-pressed OR controller Y
    bool eNow = App::IsKeyPressed(App::KEY_E);
    state.meteorPressed = (eNow && !prevE);
    prevE = eNow;
    state.meteorPressed = state.meteorPressed || pad.CheckButton(App::BTN_Y, true);

    // Scale controls: Right/Left Arrow OR controller bumpers (held)
    const bool rightNow = App::IsKeyPressed(App::KEY_RIGHT);
    const bool leftNow = App::IsKeyPressed(App::KEY_LEFT);

    state.scaleUpHeld = rightNow || pad.CheckButton(App::BTN_RBUMPER, false);
    state.scaleDownHeld = leftNow || pad.CheckButton(App::BTN_LBUMPER, false);
}
