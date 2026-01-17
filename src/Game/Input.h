#pragma once

struct InputState
{
    float moveX = 0.0f;
    float moveY = 0.0f;

    bool stopAnimPressed = false;

    bool toggleViewPressed = false;

    // Attacks (just-pressed)
    bool pulsePressed = false;   // Space / B
    bool slashPressed = false;   // Q (or Shift) / X
    bool meteorPressed = false;  // E / Y


    // Debug / test controls (held)
    bool scaleUpHeld = false;    // DPad Right
    bool scaleDownHeld = false;  // DPad Left


};

class InputSystem
{
public:
    void Update(float dt);
    const InputState& GetState() const { return state; }

private:
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
