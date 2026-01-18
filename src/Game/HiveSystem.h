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

    float spawnPerMin = 1000.0f;
    float spawnAccum = 0.0f;
};

class HiveSystem
{
public:
    void Init();
    void Update(float deltaTimeMs, ZombieSystem& zombies, const NavGrid& nav);
    void Render(float camOffX, float camOffY) const;

    bool DamageHiveAt(float wx, float wy, float hitRadius, float damage);
    int AliveCount() const;
    const std::vector<Hive>& GetHives() const { return hives; }


private:
    std::vector<Hive> hives;

private:
    void AddHive(float x, float y, float radius, float hp);
};
