#pragma once

struct IsoProjector
{
    // Screen center for 1024x768
    float screenCX = 512.0f;
    float screenCY = 384.0f;

    // World scale -> screen scale
    float kx = 0.35f;
    float ky = 0.30f;

    // Camera center in world coords
    float camCenterX = 0.0f;
    float camCenterY = 0.0f;

    static IsoProjector FromCameraOffset(float offX, float offY)
    {
        IsoProjector iso;
        iso.camCenterX = offX + 512.0f;
        iso.camCenterY = offY + 384.0f;
        return iso;
    }

    void WorldToScreen(float wx, float wy, float wz, float& sx, float& sy) const
    {
        const float x = wx - camCenterX;
        const float y = wy - camCenterY;

        sx = (x - y) * kx + screenCX;
        sy = (x + y) * ky + screenCY - wz;
    }
};

inline bool IsBackFace2D(float x0, float y0, float x1, float y1, float x2, float y2)
{
    const float cross = (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
    return cross > 0.0f; // flip to > if it culls the wrong way
}
