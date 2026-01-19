// HiveSystem.h - AAA Quality Version
#pragma once
#include <vector>

class ZombieSystem;
class NavGrid;

struct Hive
{
    float x = 0.0f;
    float y = 0.0f;
    float radius = 30.0f;

    float hp = 100.0f;
    float maxHp = 100.0f;

    bool alive = true;

    float spawnPerMin = 100.0f;
    float spawnAccum = 0.0f;
};

class HiveSystem
{
public:
    // Initialization
    void Init();

    // Updates
    void Update(float deltaTimeMs, ZombieSystem& zombies, const NavGrid& nav);

    // Rendering
    void Render(float camOffX, float camOffY) const;

    // Combat
    bool DamageHiveAt(float wx, float wy, float hitRadius, float damage);

    // State queries
    int AliveCount() const;
    int TotalCount() const { return static_cast<int>(hives.size()); }
    const std::vector<Hive>& GetHives() const { return hives; }

private:
    void AddHive(float x, float y, float radius, float hp);

private:
    std::vector<Hive> hives;
};