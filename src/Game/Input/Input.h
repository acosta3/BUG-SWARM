// Input.h
#pragma once
#include "../ContestAPI/app.h"

struct InputState
{
    float moveX = 0.0f;
    float moveY = 0.0f;

    bool stopAnimPressed = false;
    bool toggleViewPressed = false;

    // Attacks (just-pressed)
    bool pulsePressed = false;   // Space / B
    bool slashPressed = false;   // Q / X
    bool meteorPressed = false;  // E / Y

    // Debug / test controls (held)
    bool scaleUpHeld = false;    // Right Arrow / RBumper
    bool scaleDownHeld = false;  // Left Arrow / LBumper

    // NEW: Menu / Pause (just-pressed)
    bool startPressed = false;   // Enter / Start button
    bool pausePressed = false;   // Esc / Start button
};

class InputSystem
{
public:
    void Update(float dt);
    const InputState& GetState() const { return state; }

    void SetEnabled(bool enabled) { inputEnabled = enabled; }

private:
    bool inputEnabled = true;

    int padIndex = 0;
    InputState state;

    // Edge detection for keyboard
    bool prevV = false;
    bool prevSpace = false;
    bool prevF = false;
    bool prevE = false;

    // NEW edge detection
    bool prevEnter = false;
    bool prevEsc = false;

private:
    int FindActivePadIndex() const;
};
