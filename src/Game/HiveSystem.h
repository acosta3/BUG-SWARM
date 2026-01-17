#pragma once
#include <vector>

struct Hive
{
    float x = 0.0f;
    float y = 0.0f;
    float radius = 30.0f;

    float hp = 100.0f;
    float maxHp = 100.0f;

    bool alive = true;
};

class HiveSystem
{
public:
    void Init();
    void Update(float deltaTimeMs);
    void Render(float camOffX, float camOffY) const;

    // Returns true if any hive was hit
    bool DamageHiveAt(float wx, float wy, float hitRadius, float damage);

    int AliveCount() const;

private:
    std::vector<Hive> hives;

private:
    void AddHive(float x, float y, float radius, float hp);
};
