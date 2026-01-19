#include "CameraSystem.h"
#include "GameConfig.h"
#include <cmath>

void CameraSystem::Init(float screenW, float screenH)
{
    screenWidth = screenW;
    screenHeight = screenH;
    camX = camY = 0.0f;
    targetX = targetY = 0.0f;
    shakeTimeLeft = 0.0f;
    shakeStrength = 0.0f;
    seed = GameConfig::CameraConfig::SHAKE_SEED;
}

void CameraSystem::Follow(float worldX, float worldY)
{
    targetX = worldX;
    targetY = worldY;
}

void CameraSystem::Update(float deltaTime)
{
    const float dt = deltaTime / 1000.0f;

    const float followSpeed = GameConfig::CameraConfig::FOLLOW_SPEED;
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

void CameraSystem::AddShake(float strengthPixels, float durationSec)
{
    if (durationSec <= 0.0f) return;
    if (strengthPixels > shakeStrength) shakeStrength = strengthPixels;
    if (durationSec > shakeTimeLeft) shakeTimeLeft = durationSec;
}

float CameraSystem::GetOffsetX() const
{
    return camX - screenWidth * GameConfig::CameraConfig::SCREEN_HALF_MULT + GetShakeX();
}

float CameraSystem::GetOffsetY() const
{
    return camY - screenHeight * GameConfig::CameraConfig::SCREEN_HALF_MULT + GetShakeY();
}

float CameraSystem::Rand01() const
{
    seed = GameConfig::CameraConfig::SHAKE_LCG_A * seed + GameConfig::CameraConfig::SHAKE_LCG_C;
    return (seed & GameConfig::CameraConfig::SHAKE_MASK) / float(GameConfig::CameraConfig::SHAKE_DIVISOR);
}

float CameraSystem::GetShakeX() const
{
    if (shakeTimeLeft <= 0.0f) return 0.0f;
    return (Rand01() * GameConfig::CameraConfig::SHAKE_RANGE - 1.0f) * shakeStrength;
}

float CameraSystem::GetShakeY() const
{
    if (shakeTimeLeft <= 0.0f) return 0.0f;
    return (Rand01() * GameConfig::CameraConfig::SHAKE_RANGE - 1.0f) * shakeStrength;
}