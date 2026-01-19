#pragma once
#include <vector>
#include <cassert>
#include <cstdint>

template<typename T, int MaxSize>
class ObjectPool
{
public:
    ObjectPool()
    {
        objects.reserve(MaxSize);
        activeFlags.reserve(MaxSize);
        freeList.reserve(MaxSize);

        // Pre-allocate all objects
        for (int i = 0; i < MaxSize; i++)
        {
            objects.emplace_back();
            activeFlags.push_back(0);  // Using uint8_t now
            freeList.push_back(MaxSize - 1 - i);  // Fill free list in reverse
        }
    }

   
    T* Acquire()
    {
        if (freeList.empty())
        {
            assert(false && "Object pool exhausted!");
            return nullptr;
        }

        const int index = freeList.back();
        freeList.pop_back();

        activeFlags[index] = 1;
        return &objects[index];
    }

    
    void Release(T* obj)
    {
        if (!obj) return;

        // Fast pointer arithmetic instead of linear search
        const ptrdiff_t index = obj - &objects[0];

        if (index >= 0 && index < MaxSize && activeFlags[index])
        {
            activeFlags[index] = 0;
            freeList.push_back(static_cast<int>(index));
        }
    }

    // Iterate only ACTIVE objects - much faster!
    template<typename Func>
    void ForEach(Func&& func)
    {
        for (int i = 0; i < MaxSize; i++)
        {
            if (activeFlags[i])
                func(&objects[i]);
        }
    }

    template<typename Func>
    void ForEach(Func&& func) const
    {
        for (int i = 0; i < MaxSize; i++)
        {
            if (activeFlags[i])
                func(&objects[i]);
        }
    }

    // Get active object by index (legacy support)
    T* GetActive(int index)
    {
        if (index >= 0 && index < MaxSize && activeFlags[index])
            return &objects[index];
        return nullptr;
    }

    const T* GetActive(int index) const
    {
        if (index >= 0 && index < MaxSize && activeFlags[index])
            return &objects[index];
        return nullptr;
    }

    // Count active objects
    int ActiveCount() const
    {
        return MaxSize - static_cast<int>(freeList.size());
    }

    // Reset all objects
    void Clear()
    {
        freeList.clear();
        for (int i = 0; i < MaxSize; i++)
        {
            activeFlags[i] = 0;
            freeList.push_back(MaxSize - 1 - i);
        }
    }

private:
    std::vector<T> objects;              
    std::vector<uint8_t> activeFlags;    
    std::vector<int> freeList;           
};