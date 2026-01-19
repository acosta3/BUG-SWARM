#pragma once
#include <vector>
#include <memory>

template<typename T>
class ObjectPool
{
public:
    explicit ObjectPool(size_t initialSize = 32)
    {
        pool.reserve(initialSize);
        for (size_t i = 0; i < initialSize; ++i)
        {
            pool.emplace_back(std::make_unique<T>());
        }
    }

    std::unique_ptr<T> Get()
    {
        if (!pool.empty())
        {
            auto obj = std::move(pool.back());
            pool.pop_back();
            return obj;
        }
        return std::make_unique<T>();
    }

    void Return(std::unique_ptr<T> obj)
    {
        if (obj && pool.size() < maxSize)
        {
            pool.push_back(std::move(obj));
        }
    }

private:
    std::vector<std::unique_ptr<T>> pool;
    static constexpr size_t maxSize = 128;
};

namespace GamePools
{
    // Commonly used temporary objects
    struct TempVectors
    {
        std::vector<int> indices;
        std::vector<float> positions;

        void Clear()
        {
            indices.clear();
            positions.clear();
        }
    };

    class PoolManager
    {
    public:
        static PoolManager& Instance()
        {
            static PoolManager instance;
            return instance;
        }

        ObjectPool<TempVectors>& GetTempVectorPool() { return tempVectorPool; }

    private:
        ObjectPool<TempVectors> tempVectorPool{ 16 };
    };
}