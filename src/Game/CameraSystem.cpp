#include "CameraSystem.h"

void CameraSystem::Init(float screenW, float screenH)
{
    screenWidth = screenW;
    screenHeight = screenH;
    camX = camY = 0.0f;
    targetX = targetY = 0.0f;
    shakeTimeLeft = 0.0f;
    shakeStrength = 0.0f;
    seed = 1337;
}

void CameraSystem::Follow(float worldX, float worldY)
{
    targetX = worldX;
    targetY = worldY;
}

void CameraSystem::Update(float deltaTime)
{
    const float dt = deltaTime / 1000.0f;

    const float followSpeed = 10.0f;
    const float t = 1.0f - std::exp(-followSpeed * dt);

    camX += (targetX - camX) * t;
    camY += (targetY - camY) * t;

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
//

void CameraSystem::AddShake(float strengthPixels, float durationSec)
{
    if (durationSec <= 0.0f) return;
    if (strengthPixels > shakeStrength) shakeStrength = strengthPixels;
    if (durationSec > shakeTimeLeft) shakeTimeLeft = durationSec;
}

float CameraSystem::GetOffsetX() const
{
    return camX - screenWidth * 0.5f + GetShakeX();
}

float CameraSystem::GetOffsetY() const
{
    return camY - screenHeight * 0.5f + GetShakeY();
}

float CameraSystem::Rand01() const
{
    seed = 1664525u * seed + 1013904223u;
    return (seed & 0x00FFFFFF) / float(0x01000000);
}

float CameraSystem::GetShakeX() const
{
    if (shakeTimeLeft <= 0.0f) return 0.0f;
    return (Rand01() * 2.0f - 1.0f) * shakeStrength;
}

float CameraSystem::GetShakeY() const
{
    if (shakeTimeLeft <= 0.0f) return 0.0f;
    return (Rand01() * 2.0f - 1.0f) * shakeStrength;
}
