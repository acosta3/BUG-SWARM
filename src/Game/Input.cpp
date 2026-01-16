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

int InputSystem::FindActivePadIndex() const
{
    // Pick the first controller that shows meaningful input
    const float STICK_EPS = 0.15f;
    const float TRIG_EPS = 0.20f;

    for (int i = 0; i < 10; i++)
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
            p.CheckButton(App::BTN_BACK, false);

        if (anyStick || anyTrig || anyButton)
            return i;
    }

    return padIndex; // keep last known if none active this frame
}

void InputSystem::Update(float dt)
{
    state = {};

    // Keyboard axes
    const float kx = AxisFromKeys(App::KEY_A, App::KEY_D);
    const float ky = AxisFromKeys(App::KEY_S, App::KEY_W);

    // Choose active controller slot dynamically
    padIndex = FindActivePadIndex();
    const CController& pad = CSimpleControllers::GetInstance().GetController(padIndex);

    // Controller axes (with deadzone)
    float sx = pad.GetLeftThumbStickX();
    float sy = pad.GetLeftThumbStickY();

    const float DEAD = 0.2f;
    if (std::fabs(sx) < DEAD) sx = 0.0f;
    if (std::fabs(sy) < DEAD) sy = 0.0f;

    // Combine keyboard + controller
    state.moveX = std::clamp(kx + sx, -1.0f, 1.0f);
    state.moveY = std::clamp(ky + sy, -1.0f, 1.0f);

    // Prevent faster diagonal movement
    float lenSq = state.moveX * state.moveX + state.moveY * state.moveY;
    if (lenSq > 1.0f)
    {
        float invLen = 1.0f / std::sqrt(lenSq);
        state.moveX *= invLen;
        state.moveY *= invLen;
    }

    // Stop anim: controller A just-pressed
    state.stopAnimPressed = pad.CheckButton(App::BTN_A, true);

    // Toggle view: V key (just-pressed) OR controller X (just-pressed)
    bool vNow = App::IsKeyPressed(App::KEY_V);
    state.toggleViewPressed = (vNow && !prevV);
    prevV = vNow;

    state.toggleViewPressed = state.toggleViewPressed || pad.CheckButton(App::BTN_X, true);

    // Optional debug overlay (remove later)
    // App::Print(20, 20, "ActivePad=%d", padIndex);
}
