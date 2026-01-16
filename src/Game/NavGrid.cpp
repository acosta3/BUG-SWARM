#include "NavGrid.h"
#include <algorithm>
#include <cmath>
#include <queue>

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
}

void NavGrid::AddObstacleRect(float x0, float y0, float x1, float y1)
{
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    int cx0 = (int)((x0 - worldMinX) / cellSize);
    int cy0 = (int)((y0 - worldMinY) / cellSize);
    int cx1 = (int)((x1 - worldMinX) / cellSize);
    int cy1 = (int)((y1 - worldMinY) / cellSize);

    cx0 = std::clamp(cx0, 0, gridW - 1);
    cy0 = std::clamp(cy0, 0, gridH - 1);
    cx1 = std::clamp(cx1, 0, gridW - 1);
    cy1 = std::clamp(cy1, 0, gridH - 1);

    for (int cy = cy0; cy <= cy1; cy++)
        for (int cx = cx0; cx <= cx1; cx++)
            blocked[cy * gridW + cx] = 1;
}

void NavGrid::AddObstacleCircle(float cxW, float cyW, float radius)
{
    const float r2 = radius * radius;

    int cx0 = (int)((cxW - radius - worldMinX) / cellSize);
    int cy0 = (int)((cyW - radius - worldMinY) / cellSize);
    int cx1 = (int)((cxW + radius - worldMinX) / cellSize);
    int cy1 = (int)((cyW + radius - worldMinY) / cellSize);

    cx0 = std::clamp(cx0, 0, gridW - 1);
    cy0 = std::clamp(cy0, 0, gridH - 1);
    cx1 = std::clamp(cx1, 0, gridW - 1);
    cy1 = std::clamp(cy1, 0, gridH - 1);

    for (int cy = cy0; cy <= cy1; cy++)
    {
        for (int cx = cx0; cx <= cx1; cx++)
        {
            float centerX = worldMinX + (cx + 0.5f) * cellSize;
            float centerY = worldMinY + (cy + 0.5f) * cellSize;

            float dx = centerX - cxW;
            float dy = centerY - cyW;

            if (dx * dx + dy * dy <= r2)
                blocked[cy * gridW + cx] = 1;
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
