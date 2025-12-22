// ============================================================================
// SERVICES - All business logic services in one header file
// ============================================================================

#ifndef FITNESS_QUEST_SERVICES_HPP
#define FITNESS_QUEST_SERVICES_HPP

#include <memory>
#include <string>
#include <cmath>
#include <random>
#include <vector>
#include <optional>
#include "config.hpp"
#include "shared-models/shared-models.hpp"

namespace FitnessQuest {
namespace Services {

using namespace Models;

// ============================================================================
// Reward Bundle Structure
// ============================================================================
struct RewardBundle {
    int64_t experience;
    int64_t gold;
    std::vector<std::string> items;
    std::vector<std::string> achievements;
    bool levelUp;
    int32_t newLevel;
    std::string message;
};

// ============================================================================
// Game Character Data Structure
// ============================================================================
struct GameCharacterData {
    std::string userId;
    std::string name;
    int32_t level;
    int64_t experience;
    int64_t nextLevelXP;
    
    double strength;
    double stamina;
    double agility;
    double magic;
    
    double health;
    double maxHealth;
    double mana;
    double maxMana;
    
    int64_t gold;
    std::vector<std::string> unlockedAbilities;
    
    double attackPower;
    double defense;
    double magicPower;
    double speed;
};

// ============================================================================
// Game Service
// ============================================================================
class GameService {
private:
    std::shared_ptr<Config::Database> database;
    
    const double STRENGTH_GROWTH = 2.0;
    const double STAMINA_GROWTH = 3.0;
    const double AGILITY_GROWTH = 1.5;
    const double MAGIC_GROWTH = 1.0;
    
    const double HEALTH_PER_STAMINA = 10.0;
    const double MANA_PER_MAGIC = 5.0;
    
public:
    GameService(std::shared_ptr<Config::Database> db) : database(db) {}
    
    GameCharacterData initializeCharacter(const std::string& userId, 
                                         const std::string& username) {
        GameCharacterData character;
        character.userId = userId;
        character.name = username + "'s Hero";
        character.level = 1;
        character.experience = 0;
        character.nextLevelXP = Constants::getXPForLevel(1);
        
        character.strength = 10.0;
        character.stamina = 10.0;
        character.agility = 10.0;
        character.magic = 10.0;
        
        updateDerivedStats(character);
        character.gold = 100;
        
        return character;
    }
    
    GameCharacterData applyWorkoutRewards(const std::string& userId,
                                         WorkoutType workoutType,
                                         double duration,
                                         double intensity,
                                         const GameReward& rewards) {
        FitnessDB::User user = database->getUser(userId);
        int32_t oldLevel = RewardCalculation::calculateLevelFromXP(user.experience_points);
        
        user.experience_points += rewards.experience;
        int32_t newLevel = RewardCalculation::calculateLevelFromXP(user.experience_points);
        
        if (newLevel > oldLevel) {
            user.fitness_level = newLevel;
        }
        
        database->updateUser(user);
        
        GameCharacterData character = buildCharacterFromUser(user);
        applyWorkoutStatBonus(character, workoutType, duration, intensity);
        
        return character;
    }
    
    GameCharacterData getCharacter(const std::string& userId) {
        FitnessDB::User user = database->getUser(userId);
        return buildCharacterFromUser(user);
    }
    
    bool didLevelUp(int64_t oldXP, int64_t newXP) {
        int32_t oldLevel = RewardCalculation::calculateLevelFromXP(oldXP);
        int32_t newLevel = RewardCalculation::calculateLevelFromXP(newXP);
        return newLevel > oldLevel;
    }
    
    std::vector<std::string> getAbilitiesForLevel(int32_t level) {
        std::vector<std::string> abilities;
        
        if (level >= 5) abilities.push_back("Power Strike");
        if (level >= 10) abilities.push_back("Sprint Boost");
        if (level >= 15) abilities.push_back("Flexibility Enhancement");
        if (level >= 20) abilities.push_back("Meditation Focus");
        if (level >= 25) abilities.push_back("Balance Master");
        if (level >= 30) abilities.push_back("Core Strength");
        
        return abilities;
    }
    
private:
    GameCharacterData buildCharacterFromUser(const FitnessDB::User& user) {
        GameCharacterData character;
        character.userId = user.id;
        character.name = user.username + "'s Hero";
        character.level = user.fitness_level;
        character.experience = user.experience_points;
        character.nextLevelXP = Constants::getXPForLevel(character.level + 1);
        
        calculateStatsFromLevel(character);
        updateDerivedStats(character);
        
        for (int i = 1; i <= character.level; i++) {
            auto levelAbilities = getAbilitiesForLevel(i);
            character.unlockedAbilities.insert(
                character.unlockedAbilities.end(),
                levelAbilities.begin(),
                levelAbilities.end()
            );
        }
        
        character.gold = character.level * 50;
        return character;
    }
    
    void calculateStatsFromLevel(GameCharacterData& character) {
        character.strength = 10.0 + (character.level * STRENGTH_GROWTH);
        character.stamina = 10.0 + (character.level * STAMINA_GROWTH);
        character.agility = 10.0 + (character.level * AGILITY_GROWTH);
        character.magic = 10.0 + (character.level * MAGIC_GROWTH);
    }
    
    void updateDerivedStats(GameCharacterData& character) {
        character.maxHealth = character.stamina * HEALTH_PER_STAMINA;
        character.health = character.maxHealth;
        
        character.maxMana = character.magic * MANA_PER_MAGIC;
        character.mana = character.maxMana;
        
        character.attackPower = character.strength * 2.0 + character.agility * 0.5;
        character.defense = character.strength * 0.5 + character.stamina;
        character.magicPower = character.magic * 2.0;
        character.speed = character.agility * 3.0;
    }
    
    void applyWorkoutStatBonus(GameCharacterData& character,
                               WorkoutType type,
                               double duration,
                               double intensity) {
        double bonus = (duration / 60.0) * (intensity / 10.0);
        
        switch (type) {
            case WorkoutType::STRENGTH:
                character.strength += bonus * 0.5;
                break;
            case WorkoutType::CARDIO:
                character.stamina += bonus * 0.5;
                break;
            case WorkoutType::FLEXIBILITY:
                character.agility += bonus * 0.5;
                break;
            case WorkoutType::MEDITATION:
                character.magic += bonus * 0.5;
                break;
            case WorkoutType::BALANCE:
                character.agility += bonus * 0.3;
                break;
            case WorkoutType::CORE:
                character.strength += bonus * 0.3;
                break;
        }
        
        updateDerivedStats(character);
    }
};

// ============================================================================
// Reward Service
// ============================================================================
class RewardService {
private:
    std::shared_ptr<Config::Database> database;
    std::random_device rd;
    std::mt19937 gen;
    
public:
    RewardService(std::shared_ptr<Config::Database> db) 
        : database(db), gen(rd()) {}
    
    RewardBundle calculateWorkoutRewards(const std::string& userId,
                                        WorkoutType type,
                                        double duration,
                                        double intensity,
                                        std::optional<double> formScore) {
        RewardBundle bundle;
        
        GameReward baseRewards = RewardCalculation::calculateWorkoutRewards(
            type, duration, intensity, formScore
        );
        
        bundle.experience = baseRewards.experience;
        bundle.gold = baseRewards.gold;
        
        applyMultipliers(bundle, userId, type, duration, intensity);
        checkBonusRewards(bundle, userId, type);
        checkAchievements(bundle, userId, type, duration);
        
        FitnessDB::User user = database->getUser(userId);
        int32_t oldLevel = RewardCalculation::calculateLevelFromXP(user.experience_points);
        int32_t newLevel = RewardCalculation::calculateLevelFromXP(
            user.experience_points + bundle.experience
        );
        
        bundle.levelUp = newLevel > oldLevel;
        bundle.newLevel = newLevel;
        bundle.message = generateRewardMessage(bundle);
        
        return bundle;
    }
    
    RewardBundle calculateQuestRewards(const std::string& userId,
                                      const FitnessDB::Quest& quest) {
        RewardBundle bundle;
        
        bundle.experience = quest.difficulty * 50;
        bundle.gold = quest.difficulty * 25;
        
        if (quest.title.find("Daily") != std::string::npos) {
            bundle.experience = static_cast<int64_t>(bundle.experience * 1.5);
        }
        
        if (shouldDropItem(quest.difficulty)) {
            bundle.items.push_back(generateRandomItem(quest.difficulty));
        }
        
        FitnessDB::User user = database->getUser(userId);
        int32_t oldLevel = RewardCalculation::calculateLevelFromXP(user.experience_points);
        int32_t newLevel = RewardCalculation::calculateLevelFromXP(
            user.experience_points + bundle.experience
        );
        
        bundle.levelUp = newLevel > oldLevel;
        bundle.newLevel = newLevel;
        
        bundle.message = "Quest completed! Earned " + 
                        std::to_string(bundle.experience) + " XP and " +
                        std::to_string(bundle.gold) + " gold!";
        
        return bundle;
    }
    
    RewardBundle getDailyLoginBonus(const std::string& userId) {
        RewardBundle bundle;
        
        FitnessDB::User user = database->getUser(userId);
        time_t now = time(nullptr);
        time_t daysSinceLogin = (now - user.last_login) / 86400;
        
        if (daysSinceLogin >= 1) {
            bundle.experience = 50;
            bundle.gold = 25;
            
            int32_t streak = 1;
            bundle.experience += streak * 10;
            bundle.gold += streak * 5;
            
            bundle.message = "Daily login bonus! +" + 
                           std::to_string(bundle.experience) + " XP, +" +
                           std::to_string(bundle.gold) + " gold";
        }
        
        return bundle;
    }
    
    RewardBundle getAchievementReward(const std::string& achievementId) {
        RewardBundle bundle;
        
        if (achievementId.find("legendary") != std::string::npos) {
            bundle.experience = 1000;
            bundle.gold = 500;
        } else if (achievementId.find("epic") != std::string::npos) {
            bundle.experience = 500;
            bundle.gold = 250;
        } else if (achievementId.find("rare") != std::string::npos) {
            bundle.experience = 200;
            bundle.gold = 100;
        } else {
            bundle.experience = 100;
            bundle.gold = 50;
        }
        
        bundle.achievements.push_back(achievementId);
        bundle.message = "Achievement unlocked: " + achievementId;
        
        return bundle;
    }
    
private:
    void applyMultipliers(RewardBundle& bundle, 
                         const std::string& userId,
                         WorkoutType type,
                         double duration,
                         double intensity) {
        double multiplier = 1.0;
        
        if (intensity >= 8.0) {
            multiplier += 0.2;
        }
        
        if (duration >= 60.0) {
            multiplier += 0.15;
        }
        
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        if (timeinfo->tm_wday == 0 || timeinfo->tm_wday == 6) {
            multiplier += 0.1;
        }
        
        bundle.experience = static_cast<int64_t>(bundle.experience * multiplier);
        bundle.gold = static_cast<int64_t>(bundle.gold * multiplier);
    }
    
    void checkBonusRewards(RewardBundle& bundle,
                          const std::string& userId,
                          WorkoutType type) {
        std::uniform_real_distribution<> dis(0.0, 1.0);
        if (dis(gen) < 0.05) {
            bundle.gold += 50;
            bundle.message += " BONUS: +50 gold!";
        }
    }
    
    void checkAchievements(RewardBundle& bundle,
                          const std::string& userId,
                          WorkoutType type,
                          double duration) {
        if (duration >= 120.0) {
            bundle.achievements.push_back("marathon_session");
        }
    }
    
    bool shouldDropItem(int difficulty) {
        std::uniform_real_distribution<> dis(0.0, 1.0);
        double dropChance = 0.1 * difficulty;
        return dis(gen) < dropChance;
    }
    
    std::string generateRandomItem(int difficulty) {
        std::vector<std::string> items;
        
        if (difficulty <= 2) {
            items = {"Health Potion", "Stamina Drink", "Protein Bar"};
        } else if (difficulty <= 5) {
            items = {"Magic Scroll", "Power Gem", "Speed Boots"};
        } else {
            items = {"Legendary Sword", "Ancient Armor", "Dragon Scale"};
        }
        
        std::uniform_int_distribution<> dis(0, items.size() - 1);
        return items[dis(gen)];
    }
    
    std::string generateRewardMessage(const RewardBundle& bundle) {
        std::string msg = "Earned " + std::to_string(bundle.experience) + " XP";
        msg += " and " + std::to_string(bundle.gold) + " gold!";
        
        if (bundle.levelUp) {
            msg += " 🎉 LEVEL UP! You are now level " + 
                   std::to_string(bundle.newLevel) + "!";
        }
        
        if (!bundle.items.empty()) {
            msg += " Found item: " + bundle.items[0] + "!";
        }
        
        if (!bundle.achievements.empty()) {
            msg += " New achievement: " + bundle.achievements[0] + "!";
        }
        
        return msg;
    }
};

} // namespace Services
} // namespace FitnessQuest

#endif // FITNESS_QUEST_SERVICES_HPP