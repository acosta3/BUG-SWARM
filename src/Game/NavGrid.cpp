// NavGrid.cpp
#include "NavGrid.h"
#include <algorithm>
#include <cmath>
#include <queue>

#include "../ContestAPI/app.h"
#include "IsoProjector.h"

// -------------------------------------------------
// Helpers
// -------------------------------------------------
static inline void Shade(float r, float g, float b, float m, float& outR, float& outG, float& outB)
{
    outR = r * m;
    outG = g * m;
    outB = b * m;
}

// 2D quad helper (screen-space)
static void DrawFilledQuad(
    float x0, float y0, float x1, float y1,
    float r, float g, float b,
    float z = 0.0f, float w = 1.0f)
{
    App::DrawTriangle(
        x0, y0, z, w,
        x1, y0, z, w,
        x1, y1, z, w,
        r, g, b,
        r, g, b,
        r, g, b,
        false
    );

    App::DrawTriangle(
        x0, y0, z, w,
        x1, y1, z, w,
        x0, y1, z, w,
        r, g, b,
        r, g, b,
        r, g, b,
        false
    );
}

// Draw one iso quad given 4 screen points (two triangles)
static void DrawQuad4(
    float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy,
    float r, float g, float b)
{
    App::DrawTriangle(
        ax, ay, 0.0f, 1.0f,
        bx, by, 0.0f, 1.0f,
        cx, cy, 0.0f, 1.0f,
        r, g, b,
        r, g, b,
        r, g, b,
        false
    );

    App::DrawTriangle(
        ax, ay, 0.0f, 1.0f,
        cx, cy, 0.0f, 1.0f,
        dx, dy, 0.0f, 1.0f,
        r, g, b,
        r, g, b,
        r, g, b,
        false
    );
}

// World rect -> iso block (top + walls)
static void DrawIsoBlock(
    const IsoProjector& iso,
    float wx0, float wy0, float wx1, float wy1,
    float height,
    float r, float g, float b)
{
    // Base (z = 0)
    float b0x, b0y, b1x, b1y, b2x, b2y, b3x, b3y;
    iso.WorldToScreen(wx0, wy0, 0.0f, b0x, b0y);
    iso.WorldToScreen(wx1, wy0, 0.0f, b1x, b1y);
    iso.WorldToScreen(wx1, wy1, 0.0f, b2x, b2y);
    iso.WorldToScreen(wx0, wy1, 0.0f, b3x, b3y);

    // Top (z = height)
    float t0x, t0y, t1x, t1y, t2x, t2y, t3x, t3y;
    iso.WorldToScreen(wx0, wy0, -height, t0x, t0y);
    iso.WorldToScreen(wx1, wy0, -height, t1x, t1y);
    iso.WorldToScreen(wx1, wy1, -height, t2x, t2y);
    iso.WorldToScreen(wx0, wy1, -height, t3x, t3y);

    // Top color slightly brighter
    float rTop, gTop, bTop;
    Shade(r, g, b, 1.10f, rTop, gTop, bTop);

    // Wall shades
    float rWall1, gWall1, bWall1;
    float rWall2, gWall2, bWall2;
    Shade(r, g, b, 0.65f, rWall1, gWall1, bWall1);
    Shade(r, g, b, 0.80f, rWall2, gWall2, bWall2);

    // Draw walls (all 4, cheap and simple)
    // Edge 0-1
    DrawQuad4(b0x, b0y, b1x, b1y, t1x, t1y, t0x, t0y, rWall2, gWall2, bWall2);
    // Edge 1-2
    DrawQuad4(b1x, b1y, b2x, b2y, t2x, t2y, t1x, t1y, rWall1, gWall1, bWall1);
    // Edge 2-3
    DrawQuad4(b2x, b2y, b3x, b3y, t3x, t3y, t2x, t2y, rWall2, gWall2, bWall2);
    // Edge 3-0
    DrawQuad4(b3x, b3y, b0x, b0y, t0x, t0y, t3x, t3y, rWall1, gWall1, bWall1);

    // Draw top last
    DrawQuad4(t0x, t0y, t1x, t1y, t2x, t2y, t3x, t3y, rTop, gTop, bTop);
}

// -------------------------------------------------
// NavGrid
// -------------------------------------------------
void NavGrid::Init(float minX, float minY, float maxX, float maxY, float cs)
{
    worldMinX = minX;
    worldMinY = minY;
    worldMaxX = maxX;
    worldMaxY = maxY;
    cellSize = cs;

    gridW = (int)((worldMaxX - worldMinX) / cellSize) + 1;
    gridH = (int)((worldMaxY - worldMinY) / cellSize) + 1;

    const int cellN = gridW * gridH;

    blocked.assign(cellN, 0);
    cellHeight.assign(cellN, 0.0f);

    dist.resize(cellN);
    flowX.assign(cellN, 0.0f);
    flowY.assign(cellN, 0.0f);

    targetCell = -1;
}

int NavGrid::CellIndex(float x, float y) const
{
    int cx = (int)((x - worldMinX) / cellSize);
    int cy = (int)((y - worldMinY) / cellSize);

    cx = std::clamp(cx, 0, gridW - 1);
    cy = std::clamp(cy, 0, gridH - 1);

    return cy * gridW + cx;
}

void NavGrid::ClearObstacles()
{
    std::fill(blocked.begin(), blocked.end(), 0);
    std::fill(cellHeight.begin(), cellHeight.end(), 0.0f);
}

void NavGrid::AddObstacleRect(float x0, float y0, float x1, float y1, float height)
{
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    const float eps = 0.001f;

    int cx0 = (int)std::floor((x0 - worldMinX) / cellSize);
    int cy0 = (int)std::floor((y0 - worldMinY) / cellSize);
    int cx1 = (int)std::floor(((x1 - eps) - worldMinX) / cellSize);
    int cy1 = (int)std::floor(((y1 - eps) - worldMinY) / cellSize);

    cx0 = std::clamp(cx0, 0, gridW - 1);
    cy0 = std::clamp(cy0, 0, gridH - 1);
    cx1 = std::clamp(cx1, 0, gridW - 1);
    cy1 = std::clamp(cy1, 0, gridH - 1);

    for (int cy = cy0; cy <= cy1; cy++)
    {
        for (int cx = cx0; cx <= cx1; cx++)
        {
            const int idx = cy * gridW + cx;
            blocked[idx] = 1;

            // avoid max macro issues
            cellHeight[idx] = (cellHeight[idx] < height) ? height : cellHeight[idx];
        }
    }
}

void NavGrid::AddObstacleCircle(float cxW, float cyW, float radius, float height)
{
    const float r2 = radius * radius;

    int cx0 = (int)std::floor(((cxW - radius) - worldMinX) / cellSize);
    int cy0 = (int)std::floor(((cyW - radius) - worldMinY) / cellSize);
    int cx1 = (int)std::floor(((cxW + radius) - worldMinX) / cellSize);
    int cy1 = (int)std::floor(((cyW + radius) - worldMinY) / cellSize);

    cx0 = std::clamp(cx0, 0, gridW - 1);
    cy0 = std::clamp(cy0, 0, gridH - 1);
    cx1 = std::clamp(cx1, 0, gridW - 1);
    cy1 = std::clamp(cy1, 0, gridH - 1);

    for (int cy = cy0; cy <= cy1; cy++)
    {
        for (int cx = cx0; cx <= cx1; cx++)
        {
            const float centerX = worldMinX + (cx + 0.5f) * cellSize;
            const float centerY = worldMinY + (cy + 0.5f) * cellSize;

            const float dx = centerX - cxW;
            const float dy = centerY - cyW;

            if (dx * dx + dy * dy <= r2)
            {
                const int idx = cy * gridW + cx;
                blocked[idx] = 1;
                cellHeight[idx] = (cellHeight[idx] < height) ? height : cellHeight[idx];
            }
        }
    }
}

void NavGrid::BuildFlowField(float targetX, float targetY)
{
    const int cellN = gridW * gridH;
    const int INF = 1000000000;

    targetCell = CellIndex(targetX, targetY);

    for (int i = 0; i < cellN; i++)
        dist[i] = INF;

    std::queue<int> q;
    dist[targetCell] = 0;
    q.push(targetCell);

    while (!q.empty())
    {
        int c = q.front();
        q.pop();

        int cx = c % gridW;
        int cy = c / gridW;
        int base = dist[c];

        const int nx[4] = { cx + 1, cx - 1, cx, cx };
        const int ny[4] = { cy, cy, cy + 1, cy - 1 };

        for (int k = 0; k < 4; k++)
        {
            int x = nx[k], y = ny[k];
            if (x < 0 || x >= gridW || y < 0 || y >= gridH) continue;

            int nc = y * gridW + x;
            if (blocked[nc]) continue;

            if (dist[nc] > base + 1)
            {
                dist[nc] = base + 1;
                q.push(nc);
            }
        }
    }

    for (int c = 0; c < cellN; c++)
    {
        if (blocked[c] || dist[c] == INF)
        {
            flowX[c] = 0.0f;
            flowY[c] = 0.0f;
            continue;
        }

        int cx = c % gridW;
        int cy = c / gridW;

        int bestC = c;
        int bestD = dist[c];

        const int nx[4] = { cx + 1, cx - 1, cx, cx };
        const int ny[4] = { cy, cy, cy + 1, cy - 1 };

        for (int k = 0; k < 4; k++)
        {
            int x = nx[k], y = ny[k];
            if (x < 0 || x >= gridW || y < 0 || y >= gridH) continue;

            int nc = y * gridW + x;
            if (blocked[nc]) continue;

            if (dist[nc] < bestD)
            {
                bestD = dist[nc];
                bestC = nc;
            }
        }

        if (bestC == c)
        {
            flowX[c] = 0.0f;
            flowY[c] = 0.0f;
        }
        else
        {
            int bx = bestC % gridW;
            int by = bestC / gridW;

            float fromX = worldMinX + (cx + 0.5f) * cellSize;
            float fromY = worldMinY + (cy + 0.5f) * cellSize;

            float toX = worldMinX + (bx + 0.5f) * cellSize;
            float toY = worldMinY + (by + 0.5f) * cellSize;

            float dx = toX - fromX;
            float dy = toY - fromY;

            float l2 = dx * dx + dy * dy;
            if (l2 > 0.0001f)
            {
                float inv = 1.0f / std::sqrt(l2);
                flowX[c] = dx * inv;
                flowY[c] = dy * inv;
            }
            else
            {
                flowX[c] = 0.0f;
                flowY[c] = 0.0f;
            }
        }
    }
}

void NavGrid::DebugDrawBlocked(float offX, float offY) const
{
    const float screenW = 1024.0f;
    const float screenH = 768.0f;

    const float viewMinX = offX;
    const float viewMinY = offY;
    const float viewMaxX = offX + screenW;
    const float viewMaxY = offY + screenH;

    int cx0 = (int)((viewMinX - worldMinX) / cellSize) - 1;
    int cy0 = (int)((viewMinY - worldMinY) / cellSize) - 1;
    int cx1 = (int)((viewMaxX - worldMinX) / cellSize) + 1;
    int cy1 = (int)((viewMaxY - worldMinY) / cellSize) + 1;

    cx0 = std::clamp(cx0, 0, gridW - 1);
    cy0 = std::clamp(cy0, 0, gridH - 1);
    cx1 = std::clamp(cx1, 0, gridW - 1);
    cy1 = std::clamp(cy1, 0, gridH - 1);

    const float r = 1.0f, g = 0.1f, b = 0.1f;

    for (int cy = cy0; cy <= cy1; ++cy)
    {
        for (int cx = cx0; cx <= cx1; ++cx)
        {
            const int idx = cy * gridW + cx;
            if (blocked[idx] == 0) continue;

            const float wx0 = worldMinX + cx * cellSize;
            const float wy0 = worldMinY + cy * cellSize;
            const float wx1 = wx0 + cellSize;
            const float wy1 = wy0 + cellSize;

            const float x0 = wx0 - offX;
            const float y0 = wy0 - offY;
            const float x1 = wx1 - offX;
            const float y1 = wy1 - offY;

            DrawFilledQuad(x0, y0, x1, y1, r, g, b);
        }
    }
}

void NavGrid::DebugDrawBlockedIso(const IsoProjector& iso) const
{
    struct Col { float r, g, b; };
    static const Col palette[] =
    {
        {0.55f, 0.55f, 0.60f}, // steel
        {0.60f, 0.50f, 0.45f}, // brick
        {0.45f, 0.58f, 0.48f}, // greenish
        {0.48f, 0.52f, 0.62f}, // slate blue
        {0.62f, 0.56f, 0.40f}, // sand
    };

    for (int cy = 0; cy < gridH; ++cy)
    {
        for (int cx = 0; cx < gridW; ++cx)
        {
            const int idx = cy * gridW + cx;
            if (blocked[idx] == 0) continue;

            const float wx0 = worldMinX + cx * cellSize;
            const float wy0 = worldMinY + cy * cellSize;
            const float wx1 = wx0 + cellSize;
            const float wy1 = wy0 + cellSize;

            const float h = cellHeight[idx];

            const Col c = palette[idx % (sizeof(palette) / sizeof(palette[0]))];
            DrawIsoBlock(iso, wx0, wy0, wx1, wy1, h, c.r, c.g, c.b);
        }
    }
}
