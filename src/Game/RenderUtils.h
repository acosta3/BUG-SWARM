#pragma once

#include "../ContestAPI/app.h"
#include "GameConfig.h"
#include <cmath>

namespace RenderUtils
{
    inline void DrawCircleLines(float cx, float cy, float r, float red, float green, float blue, int segments = 28)
    {
        const float twoPi = GameConfig::MathConstants::TWO_PI;

        float prevX = cx + r;
        float prevY = cy;

        for (int i = 1; i <= segments; i++)
        {
            const float a = (static_cast<float>(i) / static_cast<float>(segments)) * twoPi;
            const float x = cx + std::cos(a) * r;
            const float y = cy + std::sin(a) * r;

            App::DrawLine(prevX, prevY, x, y, red, green, blue);
            prevX = x;
            prevY = y;
        }
    }

    inline void DrawArc(float cx, float cy, float radius, float a0, float a1, 
                       float r, float g, float b, int seg = 16)
    {
        const float twoPi = GameConfig::MathConstants::TWO_PI;
        while (a1 < a0)
            a1 += twoPi;

        float px = cx + std::cos(a0) * radius;
        float py = cy + std::sin(a0) * radius;

        for (int i = 1; i <= seg; i++)
        {
            const float tt = static_cast<float>(i) / static_cast<float>(seg);
            const float a = a0 + (a1 - a0) * tt;

            const float x = cx + std::cos(a) * radius;
            const float y = cy + std::sin(a) * radius;

            App::DrawLine(px, py, x, y, r, g, b);
            px = x;
            py = y;
        }
    }

    inline void DrawSpokeRing(float cx, float cy, float radius, float spokeLen,
                             float r, float g, float b, int spokes, float phase)
    {
        const float twoPi = GameConfig::MathConstants::TWO_PI;

        for (int i = 0; i < spokes; i++)
        {
            const float a = twoPi * (static_cast<float>(i) / static_cast<float>(spokes)) + phase;

            const float x0 = cx + std::cos(a) * (radius - spokeLen);
            const float y0 = cy + std::sin(a) * (radius - spokeLen);
            const float x1 = cx + std::cos(a) * (radius + spokeLen);
            const float y1 = cy + std::sin(a) * (radius + spokeLen);

            App::DrawLine(x0, y0, x1, y1, r, g, b);
        }
    }

    inline void DrawRectOutline(float x0, float y0, float x1, float y1, float r, float g, float b)
    {
        App::DrawLine(x0, y0, x1, y0, r, g, b);
        App::DrawLine(x1, y0, x1, y1, r, g, b);
        App::DrawLine(x1, y1, x0, y1, r, g, b);
        App::DrawLine(x0, y1, x0, y0, r, g, b);
    }
}
