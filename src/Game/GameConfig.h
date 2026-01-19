#pragma once

namespace GameConfig
{
    // Mathematical Constants - Shared across all systems
    // Note: PI is already defined in AppSettings.h as a macro
    struct MathConstants
    {
        // Use the API's PI definition (defined in AppSettings.h)
        static constexpr float TWO_PI = 6.28318530718f;
        static constexpr float EPSILON = 0.0001f;
    };

    // Audio Resources - centralized file paths
    struct AudioResources
    {
        static constexpr const char* SQUISH_1 = "./Data/TestData/squish1.wav";
        static constexpr const char* SQUISH_2 = "./Data/TestData/squish2.wav";
        static constexpr const char* SQUISH_3 = "./Data/TestData/squish3.wav";
        static constexpr const char* GAME_MUSIC = "./Data/TestData/GameLoopMusic.wav";

        static constexpr const char* GetSquishSound(int index)
        {
            switch (index)
            {
            case 0: return SQUISH_1;
            case 1: return SQUISH_2;
            default: return SQUISH_3;
            }
        }
    };

    // Game Tuning - centralized magic numbers
    struct GameTuning
    {
        // Life state timers
        static constexpr float DEATH_PAUSE_MS = 900.0f;
        static constexpr float RESPAWN_GRACE_MS = 650.0f;
        static constexpr float INVULNERABILITY_RESPAWN_MS = 2000.0f;
        static constexpr float INVULNERABILITY_RESET_MS = 1000.0f;

        // Combat
        static constexpr float HEAL_PER_KILL = 1.5f;
        static constexpr int SQUISH_SOUND_COUNT = 3;

        // World dimensions
        static constexpr float WORLD_MIN_X = -5000.0f;
        static constexpr float WORLD_MIN_Y = -5000.0f;
        static constexpr float WORLD_MAX_X = 5000.0f;
        static constexpr float WORLD_MAX_Y = 5000.0f;
        static constexpr float NAV_CELL_SIZE = 60.0f;

        // Screen dimensions
        static constexpr float SCREEN_WIDTH = 1024.0f;
        static constexpr float SCREEN_HEIGHT = 768.0f;

        // Default timing
        static constexpr float DEFAULT_DT_MS = 16.0f;
        static constexpr float MOVEMENT_THRESHOLD = 0.0001f;

        // Obstacle generation
        static constexpr float OBSTACLE_SPREAD = 1.8f;
        static constexpr float OBSTACLE_HALF_SIZE = 9.0f;
        static constexpr float BAR_HALF_HEIGHT = 6.0f;

        // Performance optimization thresholds
        static constexpr float PLAYER_MOVE_CACHE_THRESHOLD = 1.0f;
        static constexpr float UPDATE_CACHE_TIME_MS = 16.0f;
    };

    // Zombie System Configuration
    struct ZombieConfig
    {
        // Spawn distances
        static constexpr float SPAWN_MIN_RADIUS = 250.0f;
        static constexpr float SPAWN_MAX_RADIUS = 450.0f;

        // Performance limits
        static constexpr float MAX_DELTA_TIME_MS = 33.0f;

        // Pop-out search parameters
        static constexpr int UNSTUCK_SEARCH_ANGLES = 16;
        static constexpr float UNSTUCK_STEP_MULTIPLIER = 0.75f;
        static constexpr float UNSTUCK_MAX_RADIUS_MULTIPLIER = 12.0f;

        // Separation performance limits
        static constexpr int MAX_SEPARATION_CHECKS = 32;

        // Type distribution probabilities
        static constexpr float GREEN_SPAWN_CHANCE = 0.70f;
        static constexpr float RED_SPAWN_CHANCE = 0.90f;
        static constexpr float BLUE_SPAWN_CHANCE = 0.99f;

        // LOD performance multipliers
        static constexpr float NO_COLLISION_MULTIPLIER = 1.25f;
        static constexpr float FAR_CHEAP_MULTIPLIER = 2.5f;
        static constexpr float VERY_FAR_CHEAP_MULTIPLIER = 2.25f;

        // Lightweight update drift speed
        static constexpr float DRIFT_SPEED_MULTIPLIER = 0.5f;

        // Zombie type stats
        static constexpr float GREEN_MAX_SPEED = 60.0f;
        static constexpr float RED_MAX_SPEED = 90.0f;
        static constexpr float BLUE_MAX_SPEED = 45.0f;
        static constexpr float PURPLE_ELITE_MAX_SPEED = 65.0f;

        static constexpr float ZOMBIE_SEEK_WEIGHT = 1.0f;
        static constexpr float ZOMBIE_SEP_WEIGHT = 1.0f;

        static constexpr int GREEN_MAX_HP = 1;
        static constexpr int RED_MAX_HP = 1;
        static constexpr int BLUE_MAX_HP = 4;
        static constexpr int PURPLE_ELITE_MAX_HP = 12;

        static constexpr int GREEN_TOUCH_DAMAGE = 1;
        static constexpr int RED_TOUCH_DAMAGE = 1;
        static constexpr int BLUE_TOUCH_DAMAGE = 2;
        static constexpr int PURPLE_ELITE_TOUCH_DAMAGE = 3;

        static constexpr float GREEN_ATTACK_COOLDOWN_MS = 750.0f;
        static constexpr float RED_ATTACK_COOLDOWN_MS = 650.0f;
        static constexpr float BLUE_ATTACK_COOLDOWN_MS = 900.0f;
        static constexpr float PURPLE_ELITE_ATTACK_COOLDOWN_MS = 850.0f;

        static constexpr float GREEN_FEAR_RADIUS = 0.0f;
        static constexpr float RED_FEAR_RADIUS = 0.0f;
        static constexpr float BLUE_FEAR_RADIUS = 0.0f;
        static constexpr float PURPLE_ELITE_FEAR_RADIUS = 220.0f;

        // Spatial grid
        static constexpr float ZOMBIE_CELL_SIZE = 40.0f;

        // Use shared math constants
        static constexpr float TWO_PI = MathConstants::TWO_PI;
        static constexpr float MOVEMENT_EPSILON = MathConstants::EPSILON;
    };

    // Renderer Configuration
    struct RenderConfig
    {
        static constexpr float SCREEN_W = 1024.0f;
        static constexpr float SCREEN_H = 768.0f;
        static constexpr int FULL_DRAW_THRESHOLD = 50000;
        static constexpr int MAX_DRAW = 10000;

        // Vignette settings
        static constexpr int VIGNETTE_BAND_SIZE = 70;
        static constexpr float VIGNETTE_TOP_STRENGTH = 0.18f;
        static constexpr float VIGNETTE_BOTTOM_STRENGTH = 0.18f;
        static constexpr float VIGNETTE_SIDE_STRENGTH = 0.16f;

        // Background settings
        static constexpr float BG_BASE_R = 0.02f;
        static constexpr float BG_BASE_G = 0.03f;
        static constexpr float BG_BASE_B = 0.05f;
        static constexpr int BG_SCANLINE_STEP = 2;
        static constexpr int BG_SCANLINE_MOD = 8;
        static constexpr float BG_SCANLINE_THIN = 0.008f;
        static constexpr float BG_SCANLINE_THICK = 0.014f;
        static constexpr float BG_GRID_SIZE = 64.0f;
        static constexpr float BG_MAJOR_PULSE_BASE = 0.02f;
        static constexpr float BG_MAJOR_PULSE_AMP = 0.01f;
        static constexpr float BG_MAJOR_PULSE_FREQ = 0.6f;
        static constexpr int BG_GRID_MAJOR_EVERY = 4;
        static constexpr int BG_GRID_THICK_MAJOR = 3;
        static constexpr int BG_GRID_THICK_MINOR = 2;
        static constexpr float BG_GRID_ALPHA_MAJOR = 0.085f;
        static constexpr float BG_GRID_ALPHA_MINOR = 0.055f;
        static constexpr float BG_GRID_MARGIN = 3.0f;
        static constexpr float BG_SEAM_ALT_1 = 0.075f;
        static constexpr float BG_SEAM_ALT_2 = 0.060f;
        static constexpr float BG_SEAM_OFFSET_1 = 6.0f;
        static constexpr float BG_SEAM_OFFSET_2 = 20.0f;

        // Zombie rendering
        static constexpr float ZOMBIE_SIZE_GREEN = 3.0f;
        static constexpr float ZOMBIE_SIZE_RED = 3.5f;
        static constexpr float ZOMBIE_SIZE_BLUE = 5.0f;
        static constexpr float ZOMBIE_SIZE_PURPLE = 7.0f;

        // Zombie colors
        static constexpr float ZOMBIE_R_GREEN = 0.2f;
        static constexpr float ZOMBIE_G_GREEN = 1.0f;
        static constexpr float ZOMBIE_B_GREEN = 0.2f;

        static constexpr float ZOMBIE_R_RED = 1.0f;
        static constexpr float ZOMBIE_G_RED = 0.2f;
        static constexpr float ZOMBIE_B_RED = 0.2f;

        static constexpr float ZOMBIE_R_BLUE = 0.2f;
        static constexpr float ZOMBIE_G_BLUE = 0.4f;
        static constexpr float ZOMBIE_B_BLUE = 1.0f;

        static constexpr float ZOMBIE_R_PURPLE = 0.8f;
        static constexpr float ZOMBIE_G_PURPLE = 0.2f;
        static constexpr float ZOMBIE_B_PURPLE = 1.0f;

        // Density view
        static constexpr float DENSITY_INTENSITY_DIVISOR = 20.0f;
        static constexpr float DENSITY_CELL_SCALE = 0.45f;
        static constexpr float DENSITY_G_BASE = 0.2f;
        static constexpr float DENSITY_G_RANGE = 0.8f;
        static constexpr float DENSITY_G_FACTOR = 0.5f;
        static constexpr float DENSITY_B = 0.1f;

        // Wiggle animation
        static constexpr float WIGGLE_BASE_FREQ = 2.2f;
        static constexpr float WIGGLE_FREQ_JITTER = 1.1f;
        static constexpr float WIGGLE_ANGLE_AMP = 0.22f;
        static constexpr float WIGGLE_TIME_RESET = 100000.0f;
        static constexpr uint32_t WIGGLE_SEED_MULT = 2654435761u;
        static constexpr uint32_t WIGGLE_SEED_ADD = 97u;
        static constexpr uint32_t WIGGLE_SEED_XOR = 0xA53A9E3Du;

        // Zombie triangle rendering
        static constexpr float ZOMBIE_SHADOW_SCALE = 1.18f;
        static constexpr float ZOMBIE_SHADOW_MULT = 0.10f;
        static constexpr float ZOMBIE_FILL_MULT = 0.85f;
        static constexpr float ZOMBIE_OUTLINE_ADD_R = 0.20f;
        static constexpr float ZOMBIE_OUTLINE_ADD_G = 0.20f;
        static constexpr float ZOMBIE_OUTLINE_ADD_B = 0.25f;
        static constexpr float ZOMBIE_INNER_SCALE = 0.55f;
        static constexpr float ZOMBIE_EYE_OFFSET_X = 0.22f;
        static constexpr float ZOMBIE_EYE_OFFSET_Y = 0.18f;
        static constexpr float ZOMBIE_EYE_SIZE = 0.18f;
        static constexpr float ZOMBIE_EYE_R = 0.70f;
        static constexpr float ZOMBIE_EYE_G = 0.95f;
        static constexpr float ZOMBIE_EYE_B = 1.00f;
        static constexpr float ZOMBIE_LEG_LX = 1.3f;
        static constexpr float ZOMBIE_LEG_LY = 0.7f;

        // Kill popup
        static constexpr float KILL_POPUP_DURATION_MS = 1200.0f;
        static constexpr float KILL_POPUP_OFFSET_X = 40.0f;
        static constexpr float KILL_POPUP_OFFSET_Y = 60.0f;
        static constexpr float KILL_POPUP_MIN_X = 10.0f;
        static constexpr float KILL_POPUP_MAX_X_OFFSET = 260.0f;
        static constexpr float KILL_POPUP_MIN_Y = 10.0f;
        static constexpr float KILL_POPUP_MAX_Y_OFFSET = 30.0f;
        static constexpr int KILL_POPUP_FRENZY_THRESHOLD = 100;
        static constexpr int KILL_POPUP_UNSTOPPABLE_THRESHOLD = 1000;
        static constexpr float KILL_POPUP_POP_OFFSET = -10.0f;
        static constexpr int KILL_POPUP_TEXT_SPACING = 10;
        static constexpr int KILL_POPUP_CHAR_WIDTH = 10;
        static constexpr float KILL_POPUP_BAR_OFFSET_Y = 12.0f;
        static constexpr float KILL_POPUP_BAR_WIDTH = 180.0f;
        static constexpr float KILL_POPUP_BAR_HEIGHT = 6.0f;

        // Kill popup colors
        static constexpr float KILL_POPUP_NORMAL_R = 1.0f;
        static constexpr float KILL_POPUP_NORMAL_G = 0.90f;
        static constexpr float KILL_POPUP_NORMAL_B = 0.20f;

        static constexpr float KILL_POPUP_FRENZY_R = 1.0f;
        static constexpr float KILL_POPUP_FRENZY_G = 0.55f;
        static constexpr float KILL_POPUP_FRENZY_B = 0.10f;

        static constexpr float KILL_POPUP_UNSTOPPABLE_R = 1.0f;
        static constexpr float KILL_POPUP_UNSTOPPABLE_G = 0.15f;
        static constexpr float KILL_POPUP_UNSTOPPABLE_B = 0.15f;

        // UI positions and colors
        static constexpr float UI_HUD_X = 500.0f;
        static constexpr float UI_HUD_Y = 80.0f;
        static constexpr float UI_HP_BAR_X_OFFSET = 120.0f;
        static constexpr float UI_HP_BAR_Y_OFFSET = 34.0f;
        static constexpr float UI_HP_BAR_WIDTH = 360.0f;
        static constexpr float UI_HP_BAR_HEIGHT = 14.0f;
        static constexpr float UI_CD_BAR_Y_OFFSET = 70.0f;
        static constexpr float UI_CD_BAR_WIDTH = 110.0f;
        static constexpr float UI_CD_BAR_HEIGHT = 10.0f;
        static constexpr float UI_CD_BAR_SPACING = 120.0f;
        static constexpr float UI_PULSE_CD_MAX = 200.0f;
        static constexpr float UI_SLASH_CD_MAX = 350.0f;
        static constexpr float UI_METEOR_CD_MAX = 900.0f;

        // Bar colors
        static constexpr float BAR_BG_R = 0.20f;
        static constexpr float BAR_BG_G = 0.20f;
        static constexpr float BAR_BG_B = 0.20f;

        static constexpr float HP_BAR_R = 0.10f;
        static constexpr float HP_BAR_G = 1.00f;
        static constexpr float HP_BAR_B = 0.10f;

        static constexpr float CD_BG_R = 0.15f;
        static constexpr float CD_BG_G = 0.15f;
        static constexpr float CD_BG_B = 0.15f;

        static constexpr float PULSE_CD_R = 0.95f;
        static constexpr float PULSE_CD_G = 0.95f;
        static constexpr float PULSE_CD_B = 0.20f;

        static constexpr float SLASH_CD_R = 0.95f;
        static constexpr float SLASH_CD_G = 0.50f;
        static constexpr float SLASH_CD_B = 0.20f;

        static constexpr float METEOR_CD_R = 0.95f;
        static constexpr float METEOR_CD_G = 0.20f;
        static constexpr float METEOR_CD_B = 0.20f;

        static constexpr float BAR_OUTLINE_R = 0.95f;
        static constexpr float BAR_OUTLINE_G = 0.95f;
        static constexpr float BAR_OUTLINE_B = 0.95f;

        static constexpr float BAR_FILL_MIN_WIDTH = 0.5f;
    };

    // Camera Configuration
    struct CameraConfig
    {
        static constexpr float DEFAULT_SCREEN_WIDTH = 800.0f;
        static constexpr float DEFAULT_SCREEN_HEIGHT = 600.0f;
        static constexpr float FOLLOW_SPEED = 10.0f;
        static constexpr unsigned int SHAKE_SEED = 1337;
        static constexpr unsigned int SHAKE_LCG_A = 1664525u;
        static constexpr unsigned int SHAKE_LCG_C = 1013904223u;
        static constexpr unsigned int SHAKE_MASK = 0x00FFFFFF;
        static constexpr unsigned int SHAKE_DIVISOR = 0x01000000;
        static constexpr float SHAKE_RANGE = 2.0f;
        static constexpr float SCREEN_HALF_MULT = 0.5f;
    };

    // Hash Configuration
    struct HashConfig
    {
        static constexpr uint32_t HASH_XOR_1 = 16;
        static constexpr uint32_t HASH_MULT_1 = 0x7feb352dU;
        static constexpr uint32_t HASH_XOR_2 = 15;
        static constexpr uint32_t HASH_MULT_2 = 0x846ca68bU;
        static constexpr uint32_t HASH_XOR_3 = 16;
        static constexpr uint32_t HASH_MASK = 0x00FFFFFFu;
        static constexpr uint32_t HASH_DIVISOR = 0x01000000u;
    };

    // Hive System Configuration
    struct HiveConfig
    {
        // Hive placement
        static constexpr float WORLD_MIN = -1500.0f;
        static constexpr float WORLD_MAX = 1500.0f;
        static constexpr float PLACEMENT_MARGIN = 220.0f;
        static constexpr float MIN_HIVE_DISTANCE = 900.0f;
        static constexpr int HIVE_COUNT = 5;
        static constexpr int MAX_PLACEMENT_ATTEMPTS = 700;
        static constexpr int PLACEMENT_SEED = 1337;

        // Hive stats
        static constexpr float NORMAL_HIVE_RADIUS = 30.0f;
        static constexpr float BOSS_HIVE_RADIUS = 34.0f;
        static constexpr float NORMAL_HIVE_HP = 120.0f;
        static constexpr float BOSS_HIVE_HP = 160.0f;
        static constexpr float SPAWN_PER_MINUTE = 100.0f;
        static constexpr float MAX_SPAWN_ACCUM = 10.0f;

        // Spawn zone
        static constexpr float SPAWN_RADIUS_MIN_OFFSET = 10.0f;
        static constexpr float SPAWN_RADIUS_MAX_OFFSET = 55.0f;
        static constexpr int SPAWN_PLACEMENT_ATTEMPTS = 10;

        // Rendering
        static constexpr int CIRCLE_SEGMENTS = 28;
        static constexpr float PULSE_FREQUENCY = 3.0f;
        static constexpr float PULSE_BASE = 0.5f;
        static constexpr float PULSE_AMP = 0.5f;
        static constexpr float PULSE_RING_OFFSET = 2.0f;
        static constexpr float PULSE_RING_SIZE = 2.0f;
        static constexpr int SPOKE_COUNT = 20;
        static constexpr float SPOKE_ROTATION_SPEED = 1.2f;
        static constexpr float SPOKE_LENGTH = 6.0f;
        static constexpr float SPOKE_RADIUS_MULT = 0.82f;
        static constexpr float INNER_RING_1_MULT = 0.55f;
        static constexpr float INNER_RING_2_MULT = 0.35f;
        static constexpr float ARC_1_SPEED = 1.5f;
        static constexpr float ARC_1_LENGTH = 1.3f;
        static constexpr float ARC_2_SPEED = 1.8f;
        static constexpr float ARC_2_LENGTH = 1.1f;
        static constexpr int ARC_SEGMENTS = 18;
        static constexpr float HP_BAR_WIDTH = 64.0f;
        static constexpr float HP_BAR_OFFSET_Y = 12.0f;
        static constexpr float ANIM_TIME_RESET = 100000.0f;

        // Use shared math constants
        static constexpr float TWO_PI = MathConstants::TWO_PI;
        static constexpr float SECONDS_PER_MINUTE = 60.0f;
        static constexpr float MS_TO_SEC = 0.001f;
    };

    // Player Configuration
    struct PlayerConfig
    {
        // Sprite settings
        static constexpr const char* SPRITE_PATH = "./data/TestData/Test.bmp";
        static constexpr int SPRITE_COLUMNS = 8;
        static constexpr int SPRITE_ROWS = 4;
        static constexpr float INITIAL_X = 400.0f;
        static constexpr float INITIAL_Y = 400.0f;

        // Animation speeds
        static constexpr float WALK_ANIM_SPEED = 1.0f / 15.0f;
        static constexpr float IDLE_ANIM_SPEED = 1.0f;

        // Base stats
        static constexpr float BASE_SPEED = 200.0f;
        static constexpr int BASE_MAX_HEALTH = 200;
        static constexpr int INITIAL_HEALTH = 200;

        // Input deadzone
        static constexpr float INPUT_DEADZONE = 0.15f;

        // Movement substep tuning
        static constexpr float MAX_DELTA_TIME = 0.20f;
        static constexpr float MAX_SUBSTEP = 1.0f / 60.0f;
        static constexpr int MIN_SUBSTEPS = 1;
        static constexpr int MAX_SUBSTEPS = 8;

        // Scale system
        static constexpr float SCALE_PER_SECOND = 1.0f;
        static constexpr float SCALE_DT_MAX = 0.0333f;
        static constexpr float SCALE_MIN = 0.4f;
        static constexpr float SCALE_MAX = 2.0f;

        // Scale breakpoints
        static constexpr float SMALL_SCALE = 0.7f;
        static constexpr float BIG_SCALE = 1.3f;

        // Small scale multipliers
        static constexpr float SMALL_SPEED_MULT = 2.00f;
        static constexpr float SMALL_HP_MULT = 0.60f;

        // Normal scale multipliers
        static constexpr float NORMAL_SPEED_MULT = 1.00f;
        static constexpr float NORMAL_HP_MULT = 1.00f;

        // Big scale multipliers
        static constexpr float BIG_SPEED_MULT = 0.50f;
        static constexpr float BIG_HP_MULT = 1.60f;

        // Health bounds
        static constexpr int MIN_HEALTH = 1;
        static constexpr int MAX_HEALTH_CAP = 999;

        // Collision
        static constexpr float BASE_COLLISION_RADIUS = 40.0f;
        static constexpr int COLLISION_CHECK_POINTS = 8;

        // Use shared math constants
        static constexpr float EPSILON = MathConstants::EPSILON;
    };

    // Attack System Configuration  
    struct AttackConfig
    {
        // Cooldowns (ms)
        static constexpr float PULSE_COOLDOWN_MS = 500.0f;
        static constexpr float SLASH_COOLDOWN_MS = 200.0f;
        static constexpr float METEOR_COOLDOWN_MS = 900.0f;

        // Audio paths
        static constexpr const char* PULSE_SOUND = "./Data/TestData/AOE.mp3";
        static constexpr const char* SLASH_SOUND = "./Data/TestData/slash.mp3";
        static constexpr const char* METEOR_SOUND = "./Data/TestData/explode.mp3";

        // Scale-based damage/radius multipliers
        static constexpr float SMALL_SCALE = 0.7f;
        static constexpr float BIG_SCALE = 1.3f;

        static constexpr float SMALL_DMG_MULT = 0.70f;
        static constexpr float SMALL_RAD_MULT = 0.60f;

        static constexpr float NORMAL_DMG_MULT = 1.00f;
        static constexpr float NORMAL_RAD_MULT = 1.00f;

        static constexpr float BIG_DMG_MULT = 1.45f;
        static constexpr float BIG_RAD_MULT = 1.80f;

        // Pulse attack
        static constexpr float PULSE_BASE_RADIUS = 200.0f;
        static constexpr float PULSE_HIVE_DAMAGE = 20.0f;
        static constexpr float PULSE_SHAKE_STRENGTH = 6.0f;
        static constexpr float PULSE_SHAKE_DURATION = 0.08f;
        static constexpr float PULSE_HIVE_SHAKE_STRENGTH = 4.0f;
        static constexpr float PULSE_HIVE_SHAKE_DURATION = 0.05f;

        // Slash attack
        static constexpr float SLASH_BASE_RANGE = 300.0f;
        static constexpr float SLASH_COS_HALF_ANGLE = 0.985f;
        static constexpr float SLASH_CENTER_DIST = 90.0f;
        static constexpr float SLASH_HIT_RADIUS = 70.0f;
        static constexpr float SLASH_HIVE_DAMAGE = 30.0f;
        static constexpr float SLASH_SHAKE_STRENGTH = 5.0f;
        static constexpr float SLASH_SHAKE_DURATION = 0.06f;

        // Meteor attack
        static constexpr float METEOR_TARGET_DIST = 260.0f;
        static constexpr float METEOR_BASE_RADIUS = 120.0f;
        static constexpr float METEOR_HIVE_DAMAGE = 50.0f;
        static constexpr float METEOR_SHAKE_STRENGTH = 8.0f;
        static constexpr float METEOR_SHAKE_DURATION = 0.10f;
        static constexpr float METEOR_HIVE_SHAKE_STRENGTH = 9.0f;
        static constexpr float METEOR_HIVE_SHAKE_DURATION = 0.12f;

        // Fear mechanics
        static constexpr float ELITE_FEAR_RADIUS = 750.0f;
        static constexpr float ELITE_FEAR_DURATION_MS = 1200.0f;
        static constexpr float ELITE_FEAR_SHAKE_STRENGTH = 10.0f;
        static constexpr float ELITE_FEAR_SHAKE_DURATION = 0.12f;

        static constexpr float AOE_FEAR_RADIUS = 350.0f;
        static constexpr float AOE_FEAR_DURATION_MS = 1200.0f;
        static constexpr float AOE_FEAR_SHAKE_STRENGTH = 10.0f;
        static constexpr float AOE_FEAR_SHAKE_DURATION = 0.12f;

        // VFX durations
        static constexpr float SLASH_FX_DURATION_MS = 80.0f;
        static constexpr float PULSE_FX_DURATION_MS = 140.0f;
        static constexpr float METEOR_FX_DURATION_MS = 220.0f;

        // VFX rendering
        static constexpr int CIRCLE_SEGMENTS_LOW = 24;
        static constexpr int CIRCLE_SEGMENTS_MED = 28;
        static constexpr int CIRCLE_SEGMENTS_HIGH = 32;
        static constexpr float PULSE_INNER_RADIUS_MULT = 0.65f;
        static constexpr float METEOR_MID_RADIUS_MULT = 0.7f;
        static constexpr float METEOR_INNER_RADIUS_MULT = 0.4f;

        // Use shared math constants
        static constexpr float TWO_PI = MathConstants::TWO_PI;
        static constexpr float EPSILON = MathConstants::EPSILON;
    };
}