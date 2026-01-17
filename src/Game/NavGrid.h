#pragma once
#include <vector>
#include <cstdint>

class NavGrid
{
public:
    void Init(float worldMinX, float worldMinY, float worldMaxX, float worldMaxY, float cellSize);

    // Obstacles
    void ClearObstacles();
    void AddObstacleRect(float x0, float y0, float x1, float y1);     // world coords
    void AddObstacleCircle(float cx, float cy, float radius);         // world coords

    // Flow field toward target
    void BuildFlowField(float targetX, float targetY);

    void DebugDrawBlocked(float offX, float offY) const;

    // Queries
    int   CellIndex(float x, float y) const;
    bool  IsBlockedCell(int cellIndex) const { return blocked[cellIndex] != 0; }
    bool IsBlockedWorld(float x, float y) const
    {
        return IsBlockedCell(CellIndex(x, y));
    }

    float FlowXAtCell(int cellIndex) const { return flowX[cellIndex]; }
    float FlowYAtCell(int cellIndex) const { return flowY[cellIndex]; }

    // Debug / density view
    int   GridW() const { return gridW; }
    int   GridH() const { return gridH; }
    float CellSize() const { return cellSize; }
    float WorldMinX() const { return worldMinX; }
    float WorldMinY() const { return worldMinY; }
    int   CellCount() const { return gridW * gridH; }

    float WorldMaxX() const { return worldMaxX; }
    float WorldMaxY() const { return worldMaxY; }
    

private:
    float worldMinX = -5000.0f;
    float worldMinY = -5000.0f;
    float worldMaxX = 5000.0f;
    float worldMaxY = 5000.0f;

    float cellSize = 40.0f;
    int gridW = 0;
    int gridH = 0;

    std::vector<uint8_t> blocked;  // 0/1
    std::vector<int>     dist;     // BFS distance
    std::vector<float>   flowX;    // direction per cell
    std::vector<float>   flowY;

    int targetCell = -1;
};
