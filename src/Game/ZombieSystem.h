#pragma once
#include <vector>
#include <cstdint>

struct ZombieTypeStats
{
    float maxSpeed;
    float seekWeight;
    float sepWeight;

    uint16_t maxHP;
    uint8_t touchDamage;
    float attackCooldownMs;

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

    void Init(int maxZombies);
    void Clear();

    void Spawn(int count, float playerX, float playerY);
    void Kill(int index);

    void Update(float deltaTimeMs, float playerX, float playerY);


    int AliveCount() const { return aliveCount; }

    float GetX(int i) const { return posX[i]; }
    float GetY(int i) const { return posY[i]; }
    uint8_t GetType(int i) const { return type[i]; }

    void GetTypeCounts(int& g, int& r, int& b, int& p) const;

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
	// spatial hashing for separation
   
    
    float cellSize = 40.0f;      // tweak later
    int gridW = 0;
    int gridH = 0;

    std::vector<int> cellStart;  // size = gridW*gridH + 1
    std::vector<int> cellCount;  // size = gridW*gridH
    std::vector<int> cellList;   // size = aliveCount (indices)
    std::vector<int> writeCursor;


    // world bounds for grid (simple, can be big) 
    float worldMinX = -5000.0f; // have to decide how big later
    float worldMinY = -5000.0f;
    float worldMaxX = 5000.0f;
    float worldMaxY = 5000.0f;

    int  CellIndex(float x, float y) const;
    void BuildGrid();

public:

    // --- Density view helpers (read-only) ---
    int GetGridW() const { return gridW; }
    int GetGridH() const { return gridH; }
    float GetCellSize() const { return cellSize; }
    float GetWorldMinX() const { return worldMinX; }
    float GetWorldMinY() const { return worldMinY; }
    int GetCellCountAt(int cellIndex) const { return cellCount[cellIndex]; }



};
