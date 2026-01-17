// Input.h
#pragma once

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
};

class InputSystem
{
public:
    void Update(float dt);
    const InputState& GetState() const { return state; }

    // NEW: enable/disable all input (returns neutral state when disabled)
    void SetEnabled(bool enabled) { inputEnabled = enabled; }

private:
    bool inputEnabled = true;

    int padIndex = 0;
    InputState state;

    // Edge detection for keyboard
    bool prevV = false;
    bool prevSpace = false;
    bool prevQ = false;
    bool prevE = false;

private:
    int FindActivePadIndex() const;
};
