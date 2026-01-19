#include "UIRenderer.h"
#include "MyGame.h"
#include "Player.h"
#include "HiveSystem.h"
#include "ZombieSystem.h"
#include "DifficultyManager.h"
#include "GameConfig.h"
#include "../ContestAPI/app.h"
            
#include <cmath>
#include <cstdio>

void UIRenderer::DrawAnimatedBackground(float time)
{
    for (int y = 0; y < 768; y += 3)
    {
        const float intensity = (y % 6 == 0) ? 0.03f : 0.015f;
        App::DrawLine(0.0f, static_cast<float>(y), 1024.0f, static_cast<float>(y),
            0.02f + intensity, 0.03f + intensity, 0.05f + intensity);
    }

    const float gridSize = 64.0f;
    const float pulse = 0.02f + 0.01f * std::sin(time * 0.6f);

    for (float x = 0.0f; x < 1024.0f; x += gridSize)
    {
        const bool isMajor = (static_cast<int>(x / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(x, 0.0f, x, 768.0f, alpha, alpha + 0.01f, alpha + 0.03f);
    }

    for (float y = 0.0f; y < 768.0f; y += gridSize)
    {
        const bool isMajor = (static_cast<int>(y / gridSize) % 4 == 0);
        const float alpha = isMajor ? (0.08f + pulse) : 0.05f;
        App::DrawLine(0.0f, y, 1024.0f, y, alpha, alpha + 0.01f, alpha + 0.03f);
    }
}

void UIRenderer::DrawTacticalMap(const HiveSystem& hives, float mapX, float mapY, float mapW, float mapH, float time)
{
    const float worldSize = 2600.0f;
    const float scale = mapW / worldSize;

    App::DrawLine(mapX - 2, mapY - 2, mapX + mapW + 2, mapY - 2, 0.70f, 0.90f, 1.00f);
    App::DrawLine(mapX + mapW + 2, mapY - 2, mapX + mapW + 2, mapY + mapH + 2, 0.70f, 0.90f, 1.00f);
    App::DrawLine(mapX + mapW + 2, mapY + mapH + 2, mapX - 2, mapY + mapH + 2, 0.70f, 0.90f, 1.00f);
    App::DrawLine(mapX - 2, mapY + mapH + 2, mapX - 2, mapY - 2, 0.70f, 0.90f, 1.00f);

    App::DrawLine(mapX, mapY, mapX + mapW, mapY, 1.0f, 0.95f, 0.20f);
    App::DrawLine(mapX + mapW, mapY, mapX + mapW, mapY + mapH, 1.0f, 0.95f, 0.20f);
    App::DrawLine(mapX + mapW, mapY + mapH, mapX, mapY + mapH, 1.0f, 0.95f, 0.20f);
    App::DrawLine(mapX, mapY + mapH, mapX, mapY, 1.0f, 0.95f, 0.20f);

    const float bMin = GameConfig::BoundaryConfig::BOUNDARY_MIN;
    const float bMax = GameConfig::BoundaryConfig::BOUNDARY_MAX;
    const float centerX = mapX + mapW * 0.5f;
    const float centerY = mapY + mapH * 0.5f;

    auto WorldToMap = [&](float wx, float wy, float& mx, float& my)
    {
        mx = centerX + wx * scale;
        my = centerY + wy * scale;
    };

    float x1, y1, x2, y2;
    WorldToMap(bMin, bMin, x1, y1);
    WorldToMap(bMax, bMax, x2, y2);

    const float wallAlpha = 0.3f;
    App::DrawLine(x1, y1, x2, y1, 0.65f * wallAlpha, 0.55f * wallAlpha, 0.15f * wallAlpha);
    App::DrawLine(x1, y2, x2, y2, 0.65f * wallAlpha, 0.55f * wallAlpha, 0.15f * wallAlpha);
    App::DrawLine(x1, y1, x1, y2, 0.65f * wallAlpha, 0.55f * wallAlpha, 0.15f * wallAlpha);
    App::DrawLine(x2, y1, x2, y2, 0.65f * wallAlpha, 0.55f * wallAlpha, 0.15f * wallAlpha);

    const auto& hiveList = hives.GetHives();
    const float hivePulse = std::sin(time * 3.0f) * 0.3f + 0.7f;

    for (const auto& h : hiveList)
    {
        float mx, my;
        WorldToMap(h.x, h.y, mx, my);
        const float r = h.radius * scale;
        const int segments = 16;

        float prevX = mx + (r + 3) * hivePulse;
        float prevY = my;

        for (int i = 1; i <= segments; i++)
        {
            const float angle = (GameConfig::HiveConfig::TWO_PI * i) / segments;
            const float x = mx + std::cos(angle) * (r + 3) * hivePulse;
            const float y = my + std::sin(angle) * (r + 3) * hivePulse;
            App::DrawLine(prevX, prevY, x, y, 1.0f * hivePulse, 0.85f * hivePulse, 0.10f * hivePulse);
            prevX = x;
            prevY = y;
        }

        prevX = mx + r;
        prevY = my;

        for (int i = 1; i <= segments; i++)
        {
            const float angle = (GameConfig::HiveConfig::TWO_PI * i) / segments;
            const float x = mx + std::cos(angle) * r;
            const float y = my + std::sin(angle) * r;
            App::DrawLine(prevX, prevY, x, y, 1.0f, 0.95f, 0.20f);
            prevX = x;
            prevY = y;
        }
    }

    App::Print(mapX, mapY + mapH + 5, "3 HIVES DETECTED", 1.0f, 0.55f, 0.10f, GLUT_BITMAP_HELVETICA_10);
}

void UIRenderer::RenderMenu(const MyGame& game, const HiveSystem& hives)
{
    static float menuTime = 0.0f;
    menuTime += 0.016f;
    if (menuTime > 1000.0f) menuTime = 0.0f;

    DrawAnimatedBackground(menuTime);

    App::Print(387, 702, "BUG SWARM", 0.1f, 0.1f, 0.1f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(385, 700, "BUG SWARM", 1.0f, 0.95f, 0.20f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(280, 670, "TACTICAL ERADICATION PROTOCOL", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    const float mapX = 80.0f;
    const float mapY = 380.0f;
    const float mapW = 240.0f;
    const float mapH = 240.0f;

    App::Print(80, 635, "TACTICAL MAP", 0.70f, 0.90f, 1.00f);
    DrawTacticalMap(hives, mapX, mapY, mapW, mapH, menuTime);

    App::Print(380, 620, "SELECT DIFFICULTY", 0.70f, 0.90f, 1.00f);
    App::Print(380, 600, "Use UP/DOWN or D-Pad", 0.50f, 0.60f, 0.70f, GLUT_BITMAP_HELVETICA_10);

    const float optionY = 570.0f;
    const float optionSpacing = 20.0f;

    const DifficultyLevel selectedDifficulty = game.GetSelectedDifficulty();

    for (int i = 0; i < 4; i++)
    {
        const DifficultyLevel level = static_cast<DifficultyLevel>(i);
        const bool isSelected = (level == selectedDifficulty);
        const float yPos = optionY - i * optionSpacing;

        if (isSelected)
        {
            const float indicatorPulse = 0.7f + 0.3f * std::sin(menuTime * 5.0f);
            App::Print(375, yPos, ">>",
                1.0f * indicatorPulse, 0.95f * indicatorPulse, 0.20f * indicatorPulse);
        }

        float r, g, b;
        DifficultyManager::GetColor(level, r, g, b);
        const char* name = DifficultyManager::GetDisplayName(level);
        const char* desc = DifficultyManager::GetShortDescription(level);

        const float alpha = isSelected ? 1.0f : 0.5f;
        App::Print(400, yPos, name, r * alpha, g * alpha, b * alpha);
        App::Print(505, yPos, desc, 0.60f * alpha, 0.70f * alpha, 0.80f * alpha, GLUT_BITMAP_HELVETICA_10);
    }

    App::Print(415, 295, "MISSION OBJECTIVE", 1.0f, 0.55f, 0.10f);
    App::Print(380, 272, "- Eliminate all hive structures", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);
    App::Print(380, 242, "- Survive the swarm", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);
    App::Print(380, 212, "- Utilize tactical abilities", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    const float blinkAlpha = std::sin(menuTime * 4.0f) > 0.0f ? 1.0f : 0.3f;
    App::Print(320, 130, ">> PRESS ENTER OR START TO BEGIN <<",
        blinkAlpha, blinkAlpha * 0.95f, blinkAlpha * 0.20f);

    App::Print(80, 310, "KEYBOARD CONTROLS", 0.70f, 0.90f, 1.00f);
    App::Print(80, 285, "Move:   W A S D", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 265, "Pulse:  Space", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 245, "Slash:  F", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 225, "Meteor: E", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 205, "Scale:  Hold Left = Small", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 190, "        Hold Right = Big", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 170, "View:   V", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 140, "SCALE MECHANICS:", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 120, "Small: +Speed -Health -Damage", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(80, 105, "Large: -Speed +Health +Damage", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);

    App::Print(700, 310, "CONTROLLER", 0.70f, 0.90f, 1.00f);
    App::Print(700, 285, "Move:   L-Stick", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 265, "Pulse:  B", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 245, "Slash:  X", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 225, "Meteor: Y", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 205, "Scale:  Hold LB = Small", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 190, "        Hold RB = Big", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(700, 170, "View:   DPad Down", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);

    App::Print(260, 30, "CLASSIFIED - AUTHORIZATION LEVEL ALPHA REQUIRED",
        0.5f, 0.5f, 0.5f, GLUT_BITMAP_HELVETICA_10);
}

void UIRenderer::RenderPauseOverlay(const Player& player, const HiveSystem& hives, const ZombieSystem& zombies)
{
    static float pauseTime = 0.0f;
    pauseTime += 0.016f;
    if (pauseTime > 1000.0f) pauseTime = 0.0f;

    DrawAnimatedBackground(pauseTime);

    App::Print(437, 702, "PAUSED", 0.1f, 0.1f, 0.1f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(435, 700, "PAUSED", 1.0f, 0.95f, 0.20f, GLUT_BITMAP_TIMES_ROMAN_24);
    App::Print(340, 670, "MISSION SUSPENDED", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    const float panelX = 365.0f;
    const float panelY = 475.0f;

    App::DrawLine(panelX - 5, panelY - 5, panelX + 295, panelY - 5, 0.70f, 0.90f, 1.00f);
    App::DrawLine(panelX + 295, panelY - 5, panelX + 295, panelY + 130, 0.70f, 0.90f, 1.00f);
    App::DrawLine(panelX + 295, panelY + 130, panelX - 5, panelY + 130, 0.70f, 0.90f, 1.00f);
    App::DrawLine(panelX - 5, panelY + 130, panelX - 5, panelY - 5, 0.70f, 0.90f, 1.00f);

    App::Print(420, 580, "MISSION STATUS", 1.0f, 0.55f, 0.10f);

    char hpText[64];
    snprintf(hpText, sizeof(hpText), "- Agent HP: %d / %d", player.GetHealth(), player.GetMaxHealth());
    App::Print(375, 557, hpText, 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    char scaleText[64];
    snprintf(scaleText, sizeof(scaleText), "- Scale: %.2fx", player.GetScale());
    App::Print(375, 537, scaleText, 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    char enemyText[64];
    snprintf(enemyText, sizeof(enemyText), "- Hostiles: %d active", zombies.AliveCount());
    App::Print(375, 517, enemyText, 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    char hiveText[64];
    snprintf(hiveText, sizeof(hiveText), "- Hives: %d / %d remaining", hives.AliveCount(), hives.TotalCount());
    App::Print(375, 497, hiveText, 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_12);

    const float blinkAlpha = std::sin(pauseTime * 4.0f) > 0.0f ? 1.0f : 0.3f;
    App::Print(427, 482, "STATUS: PAUSED",
        blinkAlpha * 0.10f, blinkAlpha * 1.00f, blinkAlpha * 0.10f, GLUT_BITMAP_HELVETICA_12);

    App::Print(305, 410, ">> PRESS ESC OR START TO RESUME <<",
        blinkAlpha, blinkAlpha * 0.95f, blinkAlpha * 0.20f);

    App::Print(220, 340, "KEYBOARD CONTROLS", 0.70f, 0.90f, 1.00f);
    App::Print(220, 315, "Move:   W A S D", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 295, "Pulse:  Space", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 275, "Slash:  F", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 255, "Meteor: E", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 235, "Scale:  Left = Small", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 220, "        Right = Big", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 200, "View:   V", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 170, "SCALE MECHANICS:", 0.70f, 0.90f, 1.00f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 150, "Small: +Speed -Health -Damage", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(220, 135, "Large: -Speed +Health +Damage", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);

    App::Print(560, 340, "CONTROLLER", 0.70f, 0.90f, 1.00f);
    App::Print(560, 315, "Move:   L-Stick", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 295, "Pulse:  B", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 275, "Slash:  X", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 255, "Meteor: Y", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 235, "Scale:  LB = Small", 0.3f, 1.0f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 220, "        RB = Big", 1.0f, 0.5f, 0.3f, GLUT_BITMAP_HELVETICA_10);
    App::Print(560, 200, "View:   DPad Down", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_10);

    App::Print(300, 30, "MISSION PAUSED - AWAITING ORDERS",
        0.5f, 0.5f, 0.5f, GLUT_BITMAP_HELVETICA_10);
}

void UIRenderer::RenderWinOverlay(const Player& player, const ZombieSystem& zombies, int maxZombies)
{
    static float winTime = 0.0f;
    winTime += 0.016f;
    if (winTime > 1000.0f) winTime = 0.0f;

    for (int y = 0; y < 768; y += 2)
    {
        const float alpha = 0.05f + 0.02f * std::sin(winTime * 0.3f + y * 0.01f);
        App::DrawLine(0.0f, static_cast<float>(y), 1024.0f, static_cast<float>(y),
            alpha, alpha * 0.8f, alpha * 1.2f);
    }

    const float gridSize = 80.0f;
    const float gridPulse = 0.03f + 0.02f * std::sin(winTime * 0.5f);

    for (float x = 0.0f; x < 1024.0f; x += gridSize)
    {
        const bool isMajor = (static_cast<int>(x / gridSize) % 3 == 0);
        const float alpha = isMajor ? (0.12f + gridPulse) : 0.06f;
        App::DrawLine(x, 100.0f, x, 668.0f, alpha, alpha * 1.1f, alpha * 1.3f);
    }

    for (float y = 100.0f; y < 668.0f; y += gridSize)
    {
        const bool isMajor = (static_cast<int>(y / gridSize) % 3 == 0);
        const float alpha = isMajor ? (0.12f + gridPulse) : 0.06f;
        App::DrawLine(0.0f, y, 1024.0f, y, alpha, alpha * 1.1f, alpha * 1.3f);
    }

    const float titlePulse = 0.7f + 0.3f * std::sin(winTime * 1.5f);

    for (int offset = 8; offset > 0; offset -= 2)
    {
        const float glowAlpha = (1.0f - offset / 8.0f) * 0.15f * titlePulse;
        App::Print(312 - offset, 552 - offset, "MISSION COMPLETE",
            0.1f * glowAlpha, 1.0f * glowAlpha, 0.1f * glowAlpha, GLUT_BITMAP_TIMES_ROMAN_24);
        App::Print(312 + offset, 552 + offset, "MISSION COMPLETE",
            0.1f * glowAlpha, 1.0f * glowAlpha, 0.1f * glowAlpha, GLUT_BITMAP_TIMES_ROMAN_24);
    }

    App::Print(314, 554, "MISSION COMPLETE", 0.0f, 0.0f, 0.0f, GLUT_BITMAP_TIMES_ROMAN_24);

    const float colorShift = std::sin(winTime * 2.0f) * 0.15f;
    App::Print(312, 552, "MISSION COMPLETE",
        0.1f + colorShift, 1.0f, 0.1f + colorShift * 0.5f, GLUT_BITMAP_TIMES_ROMAN_24);

    const float bracketAlpha = 0.7f + 0.3f * std::sin(winTime * 3.0f);
    const float bracketSize = 30.0f;

    App::DrawLine(280, 520, 280 + bracketSize, 520, bracketAlpha, bracketAlpha * 0.95f, 0.2f * bracketAlpha);
    App::DrawLine(280, 520, 280, 520 + bracketSize, bracketAlpha, bracketAlpha * 0.95f, 0.2f * bracketAlpha);

    App::DrawLine(744 - bracketSize, 520, 744, 520, bracketAlpha, bracketAlpha * 0.95f, 0.2f * bracketAlpha);
    App::DrawLine(744, 520, 744, 520 + bracketSize, bracketAlpha, bracketAlpha * 0.95f, 0.2f * bracketAlpha);

    const float panelX = 262.0f;
    const float panelY = 360.0f;
    const float panelW = 500.0f;
    const float panelH = 140.0f;

    const float borderGlow = 0.5f + 0.5f * std::sin(winTime * 2.5f);
    for (int i = 0; i < 3; i++)
    {
        const float offset = static_cast<float>(i) * 2.0f;
        const float glowAlpha = (1.0f - i / 3.0f) * 0.3f * borderGlow;
        App::DrawLine(panelX - offset, panelY - offset, panelX + panelW + offset, panelY - offset,
            0.3f * glowAlpha, 0.7f * glowAlpha, 1.0f * glowAlpha);
        App::DrawLine(panelX + panelW + offset, panelY - offset, panelX + panelW + offset, panelY + panelH + offset,
            0.3f * glowAlpha, 0.7f * glowAlpha, 1.0f * glowAlpha);
        App::DrawLine(panelX + panelW + offset, panelY + panelH + offset, panelX - offset, panelY + panelH + offset,
            0.3f * glowAlpha, 0.7f * glowAlpha, 1.0f * glowAlpha);
        App::DrawLine(panelX - offset, panelY + panelH + offset, panelX - offset, panelY - offset,
            0.3f * glowAlpha, 0.7f * glowAlpha, 1.0f * glowAlpha);
    }

    App::Print(420, 485, "MISSION SUMMARY", 1.0f, 0.65f, 0.15f);

    App::Print(277, 455, "[STATUS]", 0.3f, 0.7f, 0.9f, GLUT_BITMAP_HELVETICA_12);
    App::Print(350, 455, "ALL HIVES DESTROYED", 0.15f, 1.0f, 0.15f, GLUT_BITMAP_HELVETICA_12);

    char finalHP[64];
    snprintf(finalHP, sizeof(finalHP), "[AGENT]  HP: %d / %d", player.GetHealth(), player.GetMaxHealth());
    App::Print(277, 435, finalHP, 0.3f, 0.7f, 0.9f, GLUT_BITMAP_HELVETICA_12);

    char enemiesText[64];
    snprintf(enemiesText, sizeof(enemiesText), "[KILLS]  HOSTILES: %d", maxZombies - zombies.AliveCount());
    App::Print(277, 415, enemiesText, 0.3f, 0.7f, 0.9f, GLUT_BITMAP_HELVETICA_12);

    const float threatPulse = 0.8f + 0.2f * std::sin(winTime * 4.0f);
    App::Print(320, 385, ">> THREAT NEUTRALIZED <<",
        0.15f * threatPulse, 1.0f * threatPulse, 0.15f * threatPulse, GLUT_BITMAP_HELVETICA_12);

    const float congratsX = 212.0f;
    const float congratsY = 240.0f;
    const float congratsW = 600.0f;
    const float congratsH = 90.0f;

    const float congratsBorderPulse = 0.6f + 0.4f * std::sin(winTime * 2.0f);
    App::DrawLine(congratsX - 3, congratsY - 3, congratsX + congratsW + 3, congratsY - 3,
        0.15f * congratsBorderPulse, 1.0f * congratsBorderPulse, 0.15f * congratsBorderPulse);
    App::DrawLine(congratsX + congratsW + 3, congratsY - 3, congratsX + congratsW + 3, congratsY + congratsH + 3,
        0.15f * congratsBorderPulse, 1.0f * congratsBorderPulse, 0.15f * congratsBorderPulse);
    App::DrawLine(congratsX + congratsW + 3, congratsY + congratsH + 3, congratsX - 3, congratsY + congratsH + 3,
        0.15f * congratsBorderPulse, 1.0f * congratsBorderPulse, 0.15f * congratsBorderPulse);
    App::DrawLine(congratsX - 3, congratsY + congratsH + 3, congratsX - 3, congratsY - 3,
        0.15f * congratsBorderPulse, 1.0f * congratsBorderPulse, 0.15f * congratsBorderPulse);

    App::Print(332, 315, "EXCELLENT WORK, AGENT!", 1.0f, 0.98f, 0.3f);
    App::Print(245, 285, "The swarm has been eradicated successfully", 0.7f, 0.9f, 1.0f, GLUT_BITMAP_HELVETICA_12);
    App::Print(265, 265, "All hive structures have been neutralized", 0.7f, 0.9f, 1.0f, GLUT_BITMAP_HELVETICA_12);

    const float promptBlink = std::sin(winTime * 5.0f) > 0.0f ? 1.0f : 0.4f;
    App::Print(240, 200, ">> PRESS ENTER OR START TO CONTINUE <<",
        promptBlink, promptBlink * 0.97f, promptBlink * 0.3f);

    for (int i = 0; i < 12; i++)
    {
        const float particleTime = winTime + i * 0.3f;
        const float particleAlpha = (std::sin(particleTime * 2.5f) + 1.0f) * 0.5f;
        const float particleX = 150.0f + i * 70.0f + std::sin(particleTime * 1.5f) * 15.0f;
        const float particleY = 560.0f + std::sin(particleTime * 2.0f + i) * 25.0f;
        const float size = 2.0f + particleAlpha * 2.0f;

        App::DrawLine(particleX - size, particleY, particleX + size, particleY,
            1.0f * particleAlpha, 0.98f * particleAlpha, 0.3f * particleAlpha);
        App::DrawLine(particleX, particleY - size, particleX, particleY + size,
            1.0f * particleAlpha, 0.98f * particleAlpha, 0.3f * particleAlpha);
        App::DrawLine(particleX - size * 0.7f, particleY - size * 0.7f, particleX + size * 0.7f, particleY + size * 0.7f,
            1.0f * particleAlpha, 0.98f * particleAlpha, 0.3f * particleAlpha);
        App::DrawLine(particleX + size * 0.7f, particleY - size * 0.7f, particleX - size * 0.7f, particleY + size * 0.7f,
            1.0f * particleAlpha, 0.98f * particleAlpha, 0.3f * particleAlpha);
    }

    const float statusBarAlpha = 0.3f + 0.1f * std::sin(winTime * 1.0f);
    App::Print(230, 40, "MISSION ACCOMPLISHED - AUTHORIZATION: ALPHA CLEARANCE",
        statusBarAlpha, statusBarAlpha * 0.9f, statusBarAlpha * 0.5f, GLUT_BITMAP_HELVETICA_10);

    for (int i = 0; i < 5; i++)
    {
        const float scanY = 100.0f + std::fmod(winTime * 80.0f + i * 120.0f, 568.0f);
        const float scanAlpha = 0.08f;
        App::DrawLine(0.0f, scanY, 1024.0f, scanY, scanAlpha, scanAlpha * 1.2f, scanAlpha * 1.5f);
    }
}
