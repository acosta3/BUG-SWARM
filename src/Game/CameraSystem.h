#pragma once
#include <cmath>

class CameraSystem
{
public:
    void Init(float screenW, float screenH);
    void Follow(float worldX, float worldY);
    void Update(float deltaTime);

    void AddShake(float strengthPixels, float durationSec);

    float GetOffsetX() const;
    float GetOffsetY() const;

private:
    float screenWidth = 800.0f;
    float screenHeight = 600.0f;

    float camX = 0.0f;
    float camY = 0.0f;

    float targetX = 0.0f;
    float targetY = 0.0f;

    float shakeTimeLeft = 0.0f;
    float shakeStrength = 0.0f;

    mutable unsigned int seed = 0;

private:
    float Rand01() const;
    float GetShakeX() const;
    float GetShakeY() const;
};
