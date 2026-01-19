#pragma once
#include <vector>
#include <cassert>

// Simple, cache-friendly object pool for AAA performance
template<typename T, int MaxSize>
class ObjectPool
{
public:
    ObjectPool()
    {
        objects.reserve(MaxSize);
        activeFlags.reserve(MaxSize);

        // Pre-allocate all objects
        for (int i = 0; i < MaxSize; i++)
        {
            objects.emplace_back();
            activeFlags.push_back(false);
        }
    }

    // Acquire an object from the pool
    T* Acquire()
    {
        for (int i = 0; i < MaxSize; i++)
        {
            if (!activeFlags[i])
            {
                activeFlags[i] = true;
                return &objects[i];
            }
        }

        // Pool exhausted - this should be tuned in GameConfig
        assert(false && "Object pool exhausted!");
        return nullptr;
    }

    // Release an object back to the pool
    void Release(T* obj)
    {
        if (!obj) return;

        // Find the object in our pool
        for (int i = 0; i < MaxSize; i++)
        {
            if (&objects[i] == obj)
            {
                activeFlags[i] = false;
                return;
            }
        }
    }

    // Get active object by index (for iteration) - NON-CONST version
    T* GetActive(int index)
    {
        if (index >= 0 && index < MaxSize && activeFlags[index])
            return &objects[index];
        return nullptr;
    }

    // Get active object by index (for iteration) - CONST version
    const T* GetActive(int index) const
    {
        if (index >= 0 && index < MaxSize && activeFlags[index])
            return &objects[index];
        return nullptr;
    }

    // Count active objects
    int ActiveCount() const
    {
        int count = 0;
        for (bool active : activeFlags)
            if (active) count++;
        return count;
    }

    // Reset all objects (useful for game restart)
    void Clear()
    {
        for (int i = 0; i < MaxSize; i++)
            activeFlags[i] = false;
    }

private:
    std::vector<T> objects;        // Pre-allocated objects
    std::vector<bool> activeFlags; // Track which are in use
};