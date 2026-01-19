#pragma once

#include "GameConfig.h"

enum class DifficultyLevel
{
    Easy = 0,
    Medium = 1,
    Hard = 2,
    Extreme = 3
};

class DifficultyManager
{
public:
    static int GetMaxZombies(DifficultyLevel level)
    {
        switch (level)
        {
        case DifficultyLevel::Easy:
            return GameConfig::SystemCapacity::MAX_ZOMBIES_EASY;
        case DifficultyLevel::Medium:
            return GameConfig::SystemCapacity::MAX_ZOMBIES_MEDIUM;
        case DifficultyLevel::Hard:
            return GameConfig::SystemCapacity::MAX_ZOMBIES_HARD;
        case DifficultyLevel::Extreme:
            return GameConfig::SystemCapacity::MAX_ZOMBIES_EXTREME;
        default:
            return GameConfig::SystemCapacity::MAX_ZOMBIES_EASY;
        }
    }

    static const char* GetName(DifficultyLevel level)
    {
        switch (level)
        {
        case DifficultyLevel::Easy:
            return "EASY";
        case DifficultyLevel::Medium:
            return "MEDIUM";
        case DifficultyLevel::Hard:
            return "HARD";
        case DifficultyLevel::Extreme:
            return "EXTREME";
        default:
            return "UNKNOWN";
        }
    }

    static const char* GetDescription(DifficultyLevel level)
    {
        switch (level)
        {
        case DifficultyLevel::Easy:
            return "20,000 Hostiles - Recommended for beginners";
        case DifficultyLevel::Medium:
            return "50,000 Hostiles - Balanced challenge";
        case DifficultyLevel::Hard:
            return "150,000 Hostiles - Intense combat";
        case DifficultyLevel::Extreme:
            return "200,000 Hostiles - Maximum threat level";
        default:
            return "Unknown difficulty";
        }   
    }

    static void GetColor(DifficultyLevel level, float& r, float& g, float& b)
    {
        switch (level)
        {
        case DifficultyLevel::Easy:
            r = 0.10f; g = 1.00f; b = 0.10f;
            break;
        case DifficultyLevel::Medium:
            r = 1.00f; g = 0.95f; b = 0.20f;
            break;
        case DifficultyLevel::Hard:
            r = 1.00f; g = 0.55f; b = 0.10f;
            break;
        case DifficultyLevel::Extreme:
            r = 1.00f; g = 0.15f; b = 0.15f;
            break;
        default:
            r = 1.00f; g = 1.00f; b = 1.00f;
            break;
        }
    }

    static const char* GetDisplayName(DifficultyLevel level)
    {
        switch (level)
        {
        case DifficultyLevel::Easy:
            return "LIGHT";
        case DifficultyLevel::Medium:
            return "MODERATE";
        case DifficultyLevel::Hard:
            return "HEAVY";
        case DifficultyLevel::Extreme:
            return "INFESTATION";
        default:
            return "UNKNOWN";
        }
    }

    static const char* GetShortDescription(DifficultyLevel level)
    {
        switch (level)
        {
        case DifficultyLevel::Easy:
            return "20,000 Bugs";
        case DifficultyLevel::Medium:
            return "50,000 Bugs";
        case DifficultyLevel::Hard:
            return "150,000 Bugs";
        case DifficultyLevel::Extreme:
            return "200,000 Bugs";
        default:
            return "Unknown";
        }
    }
};
