#pragma once
#include <cmath>

class CameraSystem
{
public:
    void Init(float screenW, float screenH)
    {
        screenWidth = screenW;
        screenHeight = screenH;
        camX = 0.0f;
        camY = 0.0f;
        targetX = 0.0f;
        targetY = 0.0f;
        shakeTimeLeft = 0.0f;
        shakeStrength = 0.0f;
        seed = 1337;
    }

    void Follow(float worldX, float worldY)
    {
        targetX = worldX;
        targetY = worldY;
    }

    void Update(float deltaTimeMs)
    {
        const float dt = deltaTimeMs / 1000.0f;

        // Smooth follow (exponential)
        // Bigger = snappier camera
        const float followSpeed = 10.0f;
        const float t = 1.0f - std::exp(-followSpeed * dt);

        camX += (targetX - camX) * t;
        camY += (targetY - camY) * t;

        // Update shake
        if (shakeTimeLeft > 0.0f)
        {
            shakeTimeLeft -= dt;
            if (shakeTimeLeft <= 0.0f)
            {
                shakeTimeLeft = 0.0f;
                shakeStrength = 0.0f;
            }
        }
    }

    // Call this when something impactful happens
    void AddShake(float strengthPixels, float durationSec)
    {
        if (durationSec <= 0.0f) return;
        if (strengthPixels > shakeStrength) shakeStrength = strengthPixels;
        if (durationSec > shakeTimeLeft) shakeTimeLeft = durationSec;
    }

    float GetOffsetX() const { return camX - screenWidth * 0.5f + GetShakeX(); }
    float GetOffsetY() const { return camY - screenHeight * 0.5f + GetShakeY(); }

private:
    float screenWidth = 800.0f;
    float screenHeight = 600.0f;

    float camX = 0.0f;
    float camY = 0.0f;

    float targetX = 0.0f;
    float targetY = 0.0f;

    // Shake
    float shakeTimeLeft = 0.0f;
    float shakeStrength = 0.0f;

    mutable unsigned int seed = 0;

private:
    float Rand01() const
    {
        // Simple LCG, deterministic
        seed = 1664525u * seed + 1013904223u;
        return (seed & 0x00FFFFFF) / float(0x01000000);
    }

    float GetShakeX() const
    {
        if (shakeTimeLeft <= 0.0f) return 0.0f;
        return (Rand01() * 2.0f - 1.0f) * shakeStrength;
    }

    float GetShakeY() const
    {
        if (shakeTimeLeft <= 0.0f) return 0.0f;
        return (Rand01() * 2.0f - 1.0f) * shakeStrength;
    }
};
