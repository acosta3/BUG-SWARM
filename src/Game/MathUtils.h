#pragma once

#include <cmath>
#include <cstdlib>

namespace MathUtils
{
    inline float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    inline float DistanceSq(float ax, float ay, float bx, float by)
    {
        const float dx = ax - bx;
        const float dy = ay - by;
        return dx * dx + dy * dy;
    }

    inline void NormalizeSafe(float& x, float& y)
    {
        const float lenSq = x * x + y * y;
        if (lenSq > 0.0001f)
        {
            const float invLen = 1.0f / std::sqrt(lenSq);
            x *= invLen;
            y *= invLen;
        }
    }

    inline float Rand01()
    {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    }

    inline float RandRange(float a, float b)
    {
        return a + (b - a) * Rand01();  
    }
            
    inline float WrapMod(float v, float m)
    {
        float r = std::fmod(v, m);
        if (r < 0.0f) r += m;
        return r;
    }
}
                