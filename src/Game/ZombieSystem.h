// ZombieSystem.h - AAA Quality Version
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

// Runtime tuning parameters (can be modified during gameplay for balancing)
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
    // Initialization
    void Init(int maxZombies, const NavGrid& nav);
    void Clear();

    // Spawning
    void Spawn(int count, float playerX, float playerY);
    bool SpawnAtWorld(float x, float y, uint8_t forcedType = 255);

    // Removal
    void Despawn(int index);      // Non-player removals (fear, cleanup, etc.)
    void KillByPlayer(int index); // Player attack kills (tracked for UI)

    // Updates
    int Update(float deltaTimeMs, float playerX, float playerY, const NavGrid& nav);
    void LightweightUpdate(float deltaTimeMs); // Performance-optimized drift update

    // Behavior
    void TriggerFear(float sourceX, float sourceY, float radius, float durationMs);

public:
    // Kill tracking for UI
    void BeginFrame();
    int  ConsumeKillsThisFrame();
    int  LastMoveKills() const { return lastMoveKills; }

public:
    // State queries
    int AliveCount() const { return aliveCount; }
    int MaxCount() const { return maxCount; }
    bool CanSpawnMore(int n = 1) const { return (aliveCount + n) <= maxCount; }

    // Zombie data accessors
    float   GetX(int i) const { return posX[i]; }
    float   GetY(int i) const { return posY[i]; }
    uint8_t GetType(int i) const { return type[i]; }
    bool    IsFeared(int i) const { return fearTimerMs[i] > 0.0f; }

    void GetTypeCounts(int& g, int& r, int& b, int& p) const;

public:
    // Spatial grid accessors (for density view rendering)
    int   GetCellCountAt(int cellIndex) const { return cellCount[cellIndex]; }
    int   GetGridW() const { return gridW; }
    int   GetGridH() const { return gridH; }
    float GetCellSize() const { return cellSize; }
    float GetWorldMinX() const { return worldMinX; }
    float GetWorldMinY() const { return worldMinY; }

public:
    // Runtime tuning access
    ZombieTuning& GetTuning() { return tuning; }
    const ZombieTuning& GetTuning() const { return tuning; }

private:
    // Initialization helpers
    void InitTypeStats();
    uint8_t RollTypeWeighted() const;

    // Spatial grid helpers
    int CellIndex(float x, float y) const;
    void BuildSpatialGrid(float playerX, float playerY);  // ✅ NEW: Optimized grid builder

    // Per-frame update helpers
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
    bool PopOutIfStuck(float& x, float& y, float radius, const NavGrid& nav) const;

    // Utility
    static void NormalizeSafe(float& x, float& y);
    static float Clamp01(float v);

    // Removal core
    void KillSwapRemove(int index);

private:
    // Runtime tuning
    ZombieTuning tuning;

private:
    // Capacity and count
    int maxCount = 0;
    int aliveCount = 0;

    // SoA (Structure of Arrays) for cache-friendly data layout
    std::vector<float> posX, posY;
    std::vector<float> velX, velY;

    std::vector<uint8_t> type;
    std::vector<uint8_t> state;

    std::vector<float> fearTimerMs;
    std::vector<float> attackCooldownMs;
    std::vector<uint16_t> hp;
    std::vector<float> flowAssistMs;

    // Type configurations (initialized from GameConfig)
    ZombieTypeStats typeStats[ZTYPE_COUNT];

private:
    // ✅ OPTIMIZATION: Spatial grid improvements
    float cellSize = 40.0f;
    int gridW = 0;
    int gridH = 0;

    float worldMinX = -5000.0f;
    float worldMinY = -5000.0f;
    float worldMaxX = 5000.0f;
    float worldMaxY = 5000.0f;

    std::vector<int> cellStart;   // Prefix sum array (size = gridW*gridH + 1)
    std::vector<int> cellCount;   // Count per cell (size = gridW*gridH)
    std::vector<int> cellList;    // Zombie indices sorted by cell (size = maxCount)

    // ✅ NEW: Grid optimization members
    std::vector<int> nearList;              // Pre-allocated near zombie list
    bool gridDirty = true;                  // Flag to rebuild grid
    float gridRebuildTimerMs = 0.0f;        // Timer for periodic rebuild
    static constexpr float GRID_REBUILD_INTERVAL_MS = 50.0f;  // Rebuild every 50ms

private:
    // Kill tracking
    int killsThisFrame = 0;
    int lastMoveKills = 0;
};