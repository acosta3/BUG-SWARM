#pragma once
#include <vector>
#include <cstdint>

class NavGrid; // forward declare (include NavGrid.h in .cpp)

struct ZombieTypeStats
{
    float maxSpeed;
    float seekWeight;
    float sepWeight;

    uint16_t maxHP;
    uint8_t  touchDamage;
    float    attackCooldownMs;

    float fearRadius;
};

class ZombieSystem
{
public:
    enum ZombieType : uint8_t
    {
        GREEN = 0,
        RED,
        BLUE,
        PURPLE_ELITE,
        ZTYPE_COUNT
    };

    enum ZombieState : uint8_t
    {
        SEEK = 0,
        FLEE
    };


    void Init(int maxZombies, const NavGrid& nav);
    void Clear();

    void Spawn(int count, float playerX, float playerY);
    void Kill(int index);

    // Uses nav flow-field for seek (obstacles), and ZombieSystem grid for separation
    void Update(float deltaTimeMs, float playerX, float playerY, const NavGrid& nav);

    // Fear API
    void TriggerFear(float sourceX, float sourceY, float radius, float durationMs);

    // Counts / accessors
    int AliveCount() const { return aliveCount; }

    float GetX(int i) const { return posX[i]; }
    float GetY(int i) const { return posY[i]; }
    uint8_t GetType(int i) const { return type[i]; }

    void GetTypeCounts(int& g, int& r, int& b, int& p) const;

    // Density view helpers
    int GetCellCountAt(int cellIndex) const { return cellCount[cellIndex]; }

    // Render tint helper
    bool IsFeared(int i) const { return fearTimerMs[i] > 0.0f; }

private:
    int maxCount = 0;
    int aliveCount = 0;

    // SoA storage
    std::vector<float> posX, posY;
    std::vector<float> velX, velY;

    std::vector<uint8_t> type;
    std::vector<uint8_t> state;

    std::vector<float> fearTimerMs;
    std::vector<float> attackCooldownMs;
    std::vector<uint16_t> hp;

    ZombieTypeStats typeStats[ZTYPE_COUNT];

private:
    void InitTypeStats();
    uint8_t RollTypeWeighted() const;

private:
    // --- Separation spatial grid (ZombieSystem-owned) ---
    float cellSize = 40.0f;
    int gridW = 0;
    int gridH = 0;

    // world bounds for separation grid
    float worldMinX = -5000.0f;
    float worldMinY = -5000.0f;
    float worldMaxX = 5000.0f;
    float worldMaxY = 5000.0f;

    std::vector<int> cellStart;   // size = gridW*gridH + 1
    std::vector<int> cellCount;   // size = gridW*gridH
    std::vector<int> cellList;    // size = maxCount
    std::vector<int> writeCursor; // size = gridW*gridH + 1

    int  CellIndex(float x, float y) const;
    void BuildGrid();
public:
    // --- Density view helpers (read-only) ---
    int   GetGridW() const { return gridW; }
    int   GetGridH() const { return gridH; }
    float GetCellSize() const { return cellSize; }
    float GetWorldMinX() const { return worldMinX; }
    float GetWorldMinY() const { return worldMinY; }

    std::vector<float> flowAssistMs;

};
