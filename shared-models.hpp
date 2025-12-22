// ============================================================================
// SHARED MODELS - C++ Edition
// Must stay in sync with TypeScript version (shared-models.ts)
// ============================================================================

#ifndef FITNESS_QUEST_SHARED_MODELS_HPP
#define FITNESS_QUEST_SHARED_MODELS_HPP

#include <string>
#include <vector>
#include <ctime>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <cmath>
#include <cpprest/json.h>

namespace FitnessQuest {
namespace Models {

// ============================================================================
// ENUMS
// ============================================================================
enum class WorkoutType {
    STRENGTH = 0,
    CARDIO = 1,
    FLEXIBILITY = 2,
    MEDITATION = 3,
    BALANCE = 4,
    CORE = 5
};

enum class ExerciseDifficulty {
    BEGINNER = 0,
    INTERMEDIATE = 1,
    ADVANCED = 2,
    EXPERT = 3
};

enum class AbilityType {
    ATTACK = 0,
    DEFENSE = 1,
    UTILITY = 2,
    HEALING = 3
};

enum class QuestStatus {
    AVAILABLE = 0,
    ACTIVE = 1,
    COMPLETED = 2,
    FAILED = 3
};
// In shared-models/shared-models.hpp, add this function:
inline WorkoutType stringToWorkoutType(const std::string& str) {
    if (str == "STRENGTH") return WorkoutType::STRENGTH;
    if (str == "CARDIO") return WorkoutType::CARDIO;
    if (str == "FLEXIBILITY") return WorkoutType::FLEXIBILITY;
    if (str == "MEDITATION") return WorkoutType::MEDITATION;
    if (str == "BALANCE") return WorkoutType::BALANCE;
    if (str == "CORE") return WorkoutType::CORE;
    throw std::invalid_argument("Invalid WorkoutType string: " + str);
}
// ============================================================================
// ENUM CONVERSION HELPERS
// ============================================================================
inline std::string workoutTypeToString(WorkoutType type) {
    switch(type) {
        case WorkoutType::STRENGTH: return "STRENGTH";
        case WorkoutType::CARDIO: return "CARDIO";
        case WorkoutType::FLEXIBILITY: return "FLEXIBILITY";
        case WorkoutType::MEDITATION: return "MEDITATION";
        case WorkoutType::BALANCE: return "BALANCE";
        case WorkoutType::CORE: return "CORE";
        default: return "UNKNOWN";
    }
}



inline std::string questStatusToString(QuestStatus status) {
    switch(status) {
        case QuestStatus::AVAILABLE: return "AVAILABLE";
        case QuestStatus::ACTIVE: return "ACTIVE";
        case QuestStatus::COMPLETED: return "COMPLETED";
        case QuestStatus::FAILED: return "FAILED";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// CORE GAME STRUCTURES
// ============================================================================
struct StatBonus {
    std::string stat;  // "strength", "stamina", "agility", "magic"
    double amount;
    int64_t duration;  // seconds

    web::json::value to_json() const {
        web::json::value obj = web::json::value::object();
        obj[U("stat")] = web::json::value::string(utility::conversions::to_string_t(stat));
        obj[U("amount")] = web::json::value::number(amount);
        obj[U("duration")] = web::json::value::number(duration);
        return obj;
    }

    static StatBonus from_json(const web::json::value& json) {
        StatBonus bonus;
        bonus.stat = utility::conversions::to_utf8string(json.at(U("stat")).as_string());
        bonus.amount = json.at(U("amount")).as_double();
        bonus.duration = json.at(U("duration")).as_integer();
        return bonus;
    }
};

struct GameReward {
    int64_t experience;
    int64_t gold;
    std::vector<StatBonus> statBonuses;
    std::vector<std::string> unlockedAbilities;

    web::json::value to_json() const {
        web::json::value obj = web::json::value::object();
        obj[U("experience")] = web::json::value::number(experience);
        obj[U("gold")] = web::json::value::number(gold);
        
        web::json::value bonuses_array = web::json::value::array(statBonuses.size());
        for (size_t i = 0; i < statBonuses.size(); ++i) {
            bonuses_array[i] = statBonuses[i].to_json();
        }
        obj[U("statBonuses")] = bonuses_array;
        
        web::json::value abilities_array = web::json::value::array(unlockedAbilities.size());
        for (size_t i = 0; i < unlockedAbilities.size(); ++i) {
            abilities_array[i] = web::json::value::string(utility::conversions::to_string_t(unlockedAbilities[i]));
        }
        obj[U("unlockedAbilities")] = abilities_array;
        
        return obj;
    }

    static GameReward from_json(const web::json::value& json) {
        GameReward reward;
        reward.experience = json.at(U("experience")).as_integer();
        reward.gold = json.at(U("gold")).as_integer();
        
        if (json.has_field(U("statBonuses"))) {
            auto bonuses = json.at(U("statBonuses")).as_array();
            for (const auto& bonus_json : bonuses) {
                reward.statBonuses.push_back(StatBonus::from_json(bonus_json));
            }
        }
        
        if (json.has_field(U("unlockedAbilities"))) {
            auto abilities = json.at(U("unlockedAbilities")).as_array();
            for (const auto& ability : abilities) {
                reward.unlockedAbilities.push_back(utility::conversions::to_utf8string(ability.as_string()));
            }
        }
        
        return reward;
    }
};

struct TemporaryBonus {
    std::string statType;
    double amount;
    int64_t duration; // minutes
    time_t appliedAt;

    web::json::value to_json() const {
        web::json::value obj = web::json::value::object();
        obj[U("statType")] = web::json::value::string(utility::conversions::to_string_t(statType));
        obj[U("amount")] = web::json::value::number(amount);
        obj[U("duration")] = web::json::value::number(duration);
        obj[U("appliedAt")] = web::json::value::number(static_cast<int64_t>(appliedAt));
        return obj;
    }
};

// ============================================================================
// WORKOUT
// ============================================================================
struct Workout {
    std::string id;
    std::string userId;
    WorkoutType type;
    std::string exerciseId;
    double duration;        // minutes
    double intensity;       // 1-10
    double caloriesBurned;
    std::optional<double> formScore;  // 0-100
    std::string notes;
    time_t timestamp;
    GameReward gameRewards;
    
    web::json::value to_json() const {
        web::json::value obj = web::json::value::object();
        obj[U("id")] = web::json::value::string(utility::conversions::to_string_t(id));
        obj[U("userId")] = web::json::value::string(utility::conversions::to_string_t(userId));
        obj[U("type")] = web::json::value::string(utility::conversions::to_string_t(workoutTypeToString(type)));
        obj[U("exerciseId")] = web::json::value::string(utility::conversions::to_string_t(exerciseId));
        obj[U("duration")] = web::json::value::number(duration);
        obj[U("intensity")] = web::json::value::number(intensity);
        obj[U("caloriesBurned")] = web::json::value::number(caloriesBurned);
        
        if (formScore.has_value()) {
            obj[U("formScore")] = web::json::value::number(formScore.value());
        }
        
        obj[U("notes")] = web::json::value::string(utility::conversions::to_string_t(notes));
        obj[U("timestamp")] = web::json::value::number(static_cast<int64_t>(timestamp));
        obj[U("gameRewards")] = gameRewards.to_json();
        
        return obj;
    }
    
    static Workout from_json(const web::json::value& json) {
        Workout workout;
        workout.id = utility::conversions::to_utf8string(json.at(U("id")).as_string());
        workout.userId = utility::conversions::to_utf8string(json.at(U("userId")).as_string());
        workout.type = stringToWorkoutType(utility::conversions::to_utf8string(json.at(U("type")).as_string()));
        workout.exerciseId = utility::conversions::to_utf8string(json.at(U("exerciseId")).as_string());
        workout.duration = json.at(U("duration")).as_double();
        workout.intensity = json.at(U("intensity")).as_double();
        workout.caloriesBurned = json.at(U("caloriesBurned")).as_double();
        
        if (json.has_field(U("formScore")) && !json.at(U("formScore")).is_null()) {
            workout.formScore = json.at(U("formScore")).as_double();
        }
        
        workout.notes = utility::conversions::to_utf8string(json.at(U("notes")).as_string());
        workout.timestamp = static_cast<time_t>(json.at(U("timestamp")).as_integer());
        workout.gameRewards = GameReward::from_json(json.at(U("gameRewards")));
        
        return workout;
    }
};

// ============================================================================
// GAME CHARACTER
// ============================================================================
struct GameCharacter {
    std::string userId;
    std::string name;
    int32_t level;
    int64_t experience;
    int64_t nextLevelExperience;
    
    // Core stats (tied to real fitness)
    double strength;
    double stamina;
    double agility;
    double magic;
    
    // Combat stats
    double health;
    double maxHealth;
    double mana;
    double maxMana;
    
    // Progression
    int64_t gold;
    std::vector<std::string> unlockedAbilities;
    std::vector<std::string> equippedItems;
    std::vector<std::string> completedQuests;
    std::string currentLocation;
    
    std::vector<TemporaryBonus> temporaryBonuses;
    
    web::json::value to_json() const {
        web::json::value obj = web::json::value::object();
        obj[U("userId")] = web::json::value::string(utility::conversions::to_string_t(userId));
        obj[U("name")] = web::json::value::string(utility::conversions::to_string_t(name));
        obj[U("level")] = web::json::value::number(level);
        obj[U("experience")] = web::json::value::number(experience);
        obj[U("nextLevelExperience")] = web::json::value::number(nextLevelExperience);
        
        obj[U("strength")] = web::json::value::number(strength);
        obj[U("stamina")] = web::json::value::number(stamina);
        obj[U("agility")] = web::json::value::number(agility);
        obj[U("magic")] = web::json::value::number(magic);
        
        obj[U("health")] = web::json::value::number(health);
        obj[U("maxHealth")] = web::json::value::number(maxHealth);
        obj[U("mana")] = web::json::value::number(mana);
        obj[U("maxMana")] = web::json::value::number(maxMana);
        
        obj[U("gold")] = web::json::value::number(gold);
        obj[U("currentLocation")] = web::json::value::string(utility::conversions::to_string_t(currentLocation));
        
        // Arrays
        web::json::value abilities_array = web::json::value::array(unlockedAbilities.size());
        for (size_t i = 0; i < unlockedAbilities.size(); ++i) {
            abilities_array[i] = web::json::value::string(utility::conversions::to_string_t(unlockedAbilities[i]));
        }
        obj[U("unlockedAbilities")] = abilities_array;
        
        web::json::value items_array = web::json::value::array(equippedItems.size());
        for (size_t i = 0; i < equippedItems.size(); ++i) {
            items_array[i] = web::json::value::string(utility::conversions::to_string_t(equippedItems[i]));
        }
        obj[U("equippedItems")] = items_array;
        
        web::json::value quests_array = web::json::value::array(completedQuests.size());
        for (size_t i = 0; i < completedQuests.size(); ++i) {
            quests_array[i] = web::json::value::string(utility::conversions::to_string_t(completedQuests[i]));
        }
        obj[U("completedQuests")] = quests_array;
        
        web::json::value bonuses_array = web::json::value::array(temporaryBonuses.size());
        for (size_t i = 0; i < temporaryBonuses.size(); ++i) {
            bonuses_array[i] = temporaryBonuses[i].to_json();
        }
        obj[U("temporaryBonuses")] = bonuses_array;
        
        return obj;
    }
    
    static GameCharacter from_json(const web::json::value& json) {
        GameCharacter character;
        character.userId = utility::conversions::to_utf8string(json.at(U("userId")).as_string());
        character.name = utility::conversions::to_utf8string(json.at(U("name")).as_string());
        character.level = json.at(U("level")).as_integer();
        character.experience = json.at(U("experience")).as_integer();
        character.nextLevelExperience = json.at(U("nextLevelExperience")).as_integer();
        
        character.strength = json.at(U("strength")).as_double();
        character.stamina = json.at(U("stamina")).as_double();
        character.agility = json.at(U("agility")).as_double();
        character.magic = json.at(U("magic")).as_double();
        
        character.health = json.at(U("health")).as_double();
        character.maxHealth = json.at(U("maxHealth")).as_double();
        character.mana = json.at(U("mana")).as_double();
        character.maxMana = json.at(U("maxMana")).as_double();
        
        character.gold = json.at(U("gold")).as_integer();
        character.currentLocation = utility::conversions::to_utf8string(json.at(U("currentLocation")).as_string());
        
        // Parse arrays
        if (json.has_field(U("unlockedAbilities"))) {
            auto abilities = json.at(U("unlockedAbilities")).as_array();
            for (const auto& ability : abilities) {
                character.unlockedAbilities.push_back(utility::conversions::to_utf8string(ability.as_string()));
            }
        }
        
        if (json.has_field(U("equippedItems"))) {
            auto items = json.at(U("equippedItems")).as_array();
            for (const auto& item : items) {
                character.equippedItems.push_back(utility::conversions::to_utf8string(item.as_string()));
            }
        }
        
        if (json.has_field(U("completedQuests"))) {
            auto quests = json.at(U("completedQuests")).as_array();
            for (const auto& quest : quests) {
                character.completedQuests.push_back(utility::conversions::to_utf8string(quest.as_string()));
            }
        }
        
        return character;
    }
    
    // Calculate derived combat stats
    double getAttackPower() const { return strength * 2.0 + agility * 0.5; }
    double getDefense() const { return strength * 0.5 + stamina; }
    double getMagicPower() const { return magic * 2.0; }
    double getSpeed() const { return agility * 3.0; }
};

// ============================================================================
// GAME CONSTANTS
// ============================================================================
namespace Constants {
    // XP per minute by workout type
    inline double getXPPerMinute(WorkoutType type) {
        switch(type) {
            case WorkoutType::STRENGTH: return 2.0;
            case WorkoutType::CARDIO: return 3.0;
            case WorkoutType::FLEXIBILITY: return 1.5;
            case WorkoutType::MEDITATION: return 2.0;
            case WorkoutType::BALANCE: return 1.8;
            case WorkoutType::CORE: return 2.2;
            default: return 1.0;
        }
    }
    
    // Gold per minute by workout type
    inline double getGoldPerMinute(WorkoutType type) {
        switch(type) {
            case WorkoutType::STRENGTH: return 1.0;
            case WorkoutType::CARDIO: return 1.0;
            case WorkoutType::FLEXIBILITY: return 0.5;
            case WorkoutType::MEDITATION: return 0.5;
            case WorkoutType::BALANCE: return 0.7;
            case WorkoutType::CORE: return 0.8;
            default: return 0.5;
        }
    }
    
    // Level progression formula
    inline int64_t getXPForLevel(int32_t level) {
        return static_cast<int64_t>(100.0 * std::pow(1.5, level - 1));
    }
    
    // Anti-cheat limits
    constexpr int32_t MAX_WORKOUTS_PER_DAY = 10;
    constexpr int32_t MAX_DURATION_PER_WORKOUT = 240; // minutes
    constexpr int32_t MIN_DURATION_PER_WORKOUT = 1;
    constexpr int32_t MAX_INTENSITY = 10;
    constexpr int32_t MIN_INTENSITY = 1;
    constexpr int64_t MIN_REST_BETWEEN_WORKOUTS = 1800; // seconds
    constexpr double FORM_SCORE_MIN = 0.0;
    constexpr double FORM_SCORE_MAX = 100.0;
    
    // Health/Mana conversion
    constexpr double HEALTH_PER_STAMINA = 10.0;
    constexpr double MANA_PER_MAGIC = 5.0;
}

// ============================================================================
// VALIDATION
// ============================================================================
class ValidationException : public std::runtime_error {
public:
    ValidationException(const std::string& message, const std::string& field = "", const std::string& code = "")
        : std::runtime_error(message), field_(field), code_(code) {}
    
    const std::string& getField() const { return field_; }
    const std::string& getCode() const { return code_; }
    
private:
    std::string field_;
    std::string code_;
};

namespace Validation {
    inline void validateWorkoutDuration(double duration) {
        if (duration < Constants::MIN_DURATION_PER_WORKOUT) {
            throw ValidationException("Workout duration too short", "duration", "DURATION_TOO_SHORT");
        }
        if (duration > Constants::MAX_DURATION_PER_WORKOUT) {
            throw ValidationException("Workout duration exceeds maximum", "duration", "DURATION_TOO_LONG");
        }
    }
    
    inline void validateIntensity(double intensity) {
        if (intensity < Constants::MIN_INTENSITY || intensity > Constants::MAX_INTENSITY) {
            throw ValidationException("Invalid intensity value", "intensity", "INVALID_INTENSITY");
        }
    }
    
    inline void validateFormScore(double formScore) {
        if (formScore < Constants::FORM_SCORE_MIN || formScore > Constants::FORM_SCORE_MAX) {
            throw ValidationException("Invalid form score", "formScore", "INVALID_FORM_SCORE");
        }
    }
    
    inline void validateWorkout(const Workout& workout) {
        validateWorkoutDuration(workout.duration);
        validateIntensity(workout.intensity);
        if (workout.formScore.has_value()) {
            validateFormScore(workout.formScore.value());
        }
    }
}

// ============================================================================
// REWARD CALCULATION
// ============================================================================
namespace RewardCalculation {
    inline GameReward calculateWorkoutRewards(WorkoutType type, double duration, double intensity, std::optional<double> formScore) {
        double xpRate = Constants::getXPPerMinute(type);
        double goldRate = Constants::getGoldPerMinute(type);
        
        // Base rewards
        int64_t experience = static_cast<int64_t>(duration * xpRate * (intensity / 5.0));
        int64_t gold = static_cast<int64_t>(duration * goldRate * (intensity / 5.0));
        
        // Form score bonus (up to 20% extra)
        double formBonus = formScore.has_value() ? (formScore.value() / 100.0) * 0.2 : 0.0;
        
        GameReward reward;
        reward.experience = static_cast<int64_t>(experience * (1.0 + formBonus));
        reward.gold = static_cast<int64_t>(gold * (1.0 + formBonus));
        
        return reward;
    }
    
    inline int32_t calculateLevelFromXP(int64_t xp) {
        int32_t level = 1;
        int64_t requiredXP = Constants::getXPForLevel(level);
        
        while (xp >= requiredXP) {
            level++;
            requiredXP += Constants::getXPForLevel(level);
        }
        
        return level;
    }
}

} // namespace Models
} // namespace FitnessQuest

#endif // FITNESS_QUEST_SHARED_MODELS_HPP