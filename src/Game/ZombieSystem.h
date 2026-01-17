#pragma once
#include <vector>
#include <cstdint>

class NavGrid;

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

// Central place for all "magic numbers"
struct ZombieTuning
{
    // Touch damage collision sizes
    float playerRadius = 16.0f;
    float zombieRadius = 14.0f;

    // Separation LOD (only separate when close to player)
    float sepActiveRadius = 600.0f;

    // Separation force radius
    float sepRadius = 18.0f;

    // Fear behavior (despawn when far away)
    float fleeDespawnRadius = 1200.0f;

    // Flow assist behavior
    float flowAssistBurstMs = 300.0f;
    float flowWeight = 0.75f;

    // Density render tuning (visual only)
    float densityDenom = 20.0f;
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

public:
    void Init(int maxZombies, const NavGrid& nav);
    void Clear();

    void Spawn(int count, float playerX, float playerY);
    void Kill(int index);

    // Returns damage dealt to player this frame
    int Update(float deltaTimeMs, float playerX, float playerY, const NavGrid& nav);

    void TriggerFear(float sourceX, float sourceY, float radius, float durationMs);

public:
    // Accessors
    int AliveCount() const { return aliveCount; }

    float   GetX(int i) const { return posX[i]; }
    float   GetY(int i) const { return posY[i]; }
    uint8_t GetType(int i) const { return type[i]; }

    bool IsFeared(int i) const { return fearTimerMs[i] > 0.0f; }

    void GetTypeCounts(int& g, int& r, int& b, int& p) const;

public:
    // Density view helpers
    int GetCellCountAt(int cellIndex) const { return cellCount[cellIndex]; }
    int GetGridW() const { return gridW; }
    int GetGridH() const { return gridH; }
    float GetCellSize() const { return cellSize; }
    float GetWorldMinX() const { return worldMinX; }
    float GetWorldMinY() const { return worldMinY; }

public:
    // Optional: tweak tuning at runtime (nice for balancing)
    ZombieTuning& GetTuning() { return tuning; }
    const ZombieTuning& GetTuning() const { return tuning; }

private:
    // ---- Init helpers ----
    void InitTypeStats();
    uint8_t RollTypeWeighted() const;

    // ---- Grid helpers ----
    int  CellIndex(float x, float y) const;
    void BuildGrid();

    // ---- Per-frame helpers ----
    void TickTimers(int i, float deltaTimeMs);
    void ApplyTouchDamage(int i, const ZombieTypeStats& s, float distSqToPlayer, float hitDistSq, int& outDamage);

    // Steering
    void ComputeSeekDir(int i, float playerX, float playerY, float& outDX, float& outDY) const;
    void ComputeFleeDir(int i, float playerX, float playerY, float& outDX, float& outDY) const;
    void ApplyFlowAssistIfActive(int i, const NavGrid& nav, float& ioDX, float& ioDY) const;

    // Separation
    void ComputeSeparation(int i, float& outSepX, float& outSepY) const;

    // Movement
    bool ResolveMoveSlide(float& x, float& y, float vx, float vy, float dt, const NavGrid& nav, bool& outFullBlocked) const;

    // Utility
    static void NormalizeSafe(float& x, float& y);
    static float Clamp01(float v);

private:
    ZombieTuning tuning;

private:
    int maxCount = 0;
    int aliveCount = 0;

    // ---- SoA storage ----
    std::vector<float> posX, posY;
    std::vector<float> velX, velY;

    std::vector<uint8_t> type;
    std::vector<uint8_t> state;

    std::vector<float> fearTimerMs;
    std::vector<float> attackCooldownMs;
    std::vector<uint16_t> hp;

    std::vector<float> flowAssistMs;

    ZombieTypeStats typeStats[ZTYPE_COUNT];

private:
    // ---- Separation spatial grid ----
    float cellSize = 40.0f;
    int gridW = 0;
    int gridH = 0;

    float worldMinX = -5000.0f;
    float worldMinY = -5000.0f;
    float worldMaxX = 5000.0f;
    float worldMaxY = 5000.0f;

    std::vector<int> cellStart;   // size = gridW*gridH + 1
    std::vector<int> cellCount;   // size = gridW*gridH
    std::vector<int> cellList;    // size = maxCount
    std::vector<int> writeCursor; // size = gridW*gridH + 1
};
