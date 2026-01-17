// NavGrid.h
#pragma once
#include <vector>
#include <cstdint>

class IsoProjector;

class NavGrid
{
public:
    void Init(float worldMinX, float worldMinY, float worldMaxX, float worldMaxY, float cellSize);

    // Obstacles (height is visual only)
    void ClearObstacles();
    void AddObstacleRect(float x0, float y0, float x1, float y1, float height = 60.0f);
    void AddObstacleCircle(float cx, float cy, float radius, float height = 60.0f);

    // Flow field toward target
    void BuildFlowField(float targetX, float targetY);

    // Debug draws
    void DebugDrawBlocked(float offX, float offY) const;
    void DebugDrawBlockedIso(const IsoProjector& iso) const;

    // Queries
    int   CellIndex(float x, float y) const;

    bool IsBlockedCell(int cx, int cy) const
    {
        if (cx < 0 || cy < 0 || cx >= gridW || cy >= gridH) return true;
        return blocked[cy * gridW + cx] != 0;
    }

    bool IsBlockedWorld(float wx, float wy) const
    {
        const int cx = (int)((wx - worldMinX) / cellSize);
        const int cy = (int)((wy - worldMinY) / cellSize);
        return IsBlockedCell(cx, cy);
    }

    float FlowXAtCell(int cellIndex) const { return flowX[cellIndex]; }
    float FlowYAtCell(int cellIndex) const { return flowY[cellIndex]; }

    // Debug / info
    int   GridW() const { return gridW; }
    int   GridH() const { return gridH; }
    float CellSize() const { return cellSize; }
    float WorldMinX() const { return worldMinX; }
    float WorldMinY() const { return worldMinY; }
    float WorldMaxX() const { return worldMaxX; }
    float WorldMaxY() const { return worldMaxY; }

    float HeightAtCell(int cellIndex) const { return cellHeight[cellIndex]; }

private:
    float worldMinX = -5000.0f;
    float worldMinY = -5000.0f;
    float worldMaxX = 5000.0f;
    float worldMaxY = 5000.0f;

    float cellSize = 40.0f;
    int gridW = 0;
    int gridH = 0;

    std::vector<uint8_t> blocked;      // 0/1
    std::vector<float>   cellHeight;   // visual height per cell (pixels in iso Z)

    std::vector<int>     dist;         // BFS distance
    std::vector<float>   flowX;        // direction per cell
    std::vector<float>   flowY;

    int targetCell = -1;
};
