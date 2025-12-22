// ============================================================================
// GAME ENGINE - Simple, Fast, Database-Synced Game Engine
// Single header file - No .cpp files, no complex structure
// FIXED: Now uses web::json::value (cpprest) instead of nlohmann::json
// ============================================================================

#ifndef FITNESS_QUEST_GAME_ENGINE_HPP
#define FITNESS_QUEST_GAME_ENGINE_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <ctime>
#include <random>
#include <cpprest/json.h>
#include "config.hpp"
#include "services.hpp"
#include "GameSyncEngine.hpp"

namespace FitnessQuest {
namespace Game {

using namespace web;

// ============================================================================
// SIMPLE GAME MODELS
// ============================================================================

struct GamePlayer {
    std::string userId;
    std::string username;
    std::string characterName;
    
    // Core stats
    int level = 1;
    int experience = 0;
    int nextLevelExp = 100;
    
    // Fitness stats
    int totalSteps = 0;
    int totalCalories = 0;
    int totalWorkouts = 0;
    int streak = 0;
    
    // Game resources
    int gold = 100;
    int gems = 0;
    int energy = 100;
    
    // Game stats
    int strength = 10;
    int stamina = 10;
    int agility = 10;
    int intelligence = 10;
    
    // Progress
    std::vector<std::string> completedQuests;
    std::vector<std::string> unlockedAchievements;
    std::vector<std::string> inventory;
    
    // Timestamps
    std::string lastLoginDate;
    std::string lastWorkoutDate;
    
    // Serialization using web::json::value
    json::value toJson() const {
        json::value j = json::value::object();
        j[U("userId")] = json::value::string(utility::conversions::to_string_t(userId));
        j[U("username")] = json::value::string(utility::conversions::to_string_t(username));
        j[U("characterName")] = json::value::string(utility::conversions::to_string_t(characterName));
        j[U("level")] = json::value::number(level);
        j[U("experience")] = json::value::number(experience);
        j[U("nextLevelExp")] = json::value::number(nextLevelExp);
        j[U("totalSteps")] = json::value::number(totalSteps);
        j[U("totalCalories")] = json::value::number(totalCalories);
        j[U("totalWorkouts")] = json::value::number(totalWorkouts);
        j[U("streak")] = json::value::number(streak);
        j[U("gold")] = json::value::number(gold);
        j[U("gems")] = json::value::number(gems);
        j[U("energy")] = json::value::number(energy);
        j[U("strength")] = json::value::number(strength);
        j[U("stamina")] = json::value::number(stamina);
        j[U("agility")] = json::value::number(agility);
        j[U("intelligence")] = json::value::number(intelligence);
        j[U("lastLoginDate")] = json::value::string(utility::conversions::to_string_t(lastLoginDate));
        j[U("lastWorkoutDate")] = json::value::string(utility::conversions::to_string_t(lastWorkoutDate));
        
        // Array conversions
        json::value questsArr = json::value::array(completedQuests.size());
        for (size_t i = 0; i < completedQuests.size(); ++i) {
            questsArr[i] = json::value::string(utility::conversions::to_string_t(completedQuests[i]));
        }
        j[U("completedQuests")] = questsArr;
        
        json::value achArr = json::value::array(unlockedAchievements.size());
        for (size_t i = 0; i < unlockedAchievements.size(); ++i) {
            achArr[i] = json::value::string(utility::conversions::to_string_t(unlockedAchievements[i]));
        }
        j[U("unlockedAchievements")] = achArr;
        
        json::value invArr = json::value::array(inventory.size());
        for (size_t i = 0; i < inventory.size(); ++i) {
            invArr[i] = json::value::string(utility::conversions::to_string_t(inventory[i]));
        }
        j[U("inventory")] = invArr;
        
        return j;
    }
    
    static GamePlayer fromJson(const json::value& j) {
        GamePlayer player;
        
        if (j.has_field(U("userId"))) 
            player.userId = utility::conversions::to_utf8string(j.at(U("userId")).as_string());
        if (j.has_field(U("username"))) 
            player.username = utility::conversions::to_utf8string(j.at(U("username")).as_string());
        if (j.has_field(U("characterName"))) 
            player.characterName = utility::conversions::to_utf8string(j.at(U("characterName")).as_string());
        if (j.has_field(U("level"))) 
            player.level = j.at(U("level")).as_integer();
        if (j.has_field(U("experience"))) 
            player.experience = j.at(U("experience")).as_integer();
        if (j.has_field(U("nextLevelExp"))) 
            player.nextLevelExp = j.at(U("nextLevelExp")).as_integer();
        if (j.has_field(U("totalSteps"))) 
            player.totalSteps = j.at(U("totalSteps")).as_integer();
        if (j.has_field(U("totalCalories"))) 
            player.totalCalories = j.at(U("totalCalories")).as_integer();
        if (j.has_field(U("totalWorkouts"))) 
            player.totalWorkouts = j.at(U("totalWorkouts")).as_integer();
        if (j.has_field(U("streak"))) 
            player.streak = j.at(U("streak")).as_integer();
        if (j.has_field(U("gold"))) 
            player.gold = j.at(U("gold")).as_integer();
        if (j.has_field(U("gems"))) 
            player.gems = j.at(U("gems")).as_integer();
        if (j.has_field(U("energy"))) 
            player.energy = j.at(U("energy")).as_integer();
        if (j.has_field(U("strength"))) 
            player.strength = j.at(U("strength")).as_integer();
        if (j.has_field(U("stamina"))) 
            player.stamina = j.at(U("stamina")).as_integer();
        if (j.has_field(U("agility"))) 
            player.agility = j.at(U("agility")).as_integer();
        if (j.has_field(U("intelligence"))) 
            player.intelligence = j.at(U("intelligence")).as_integer();
        if (j.has_field(U("lastLoginDate"))) 
            player.lastLoginDate = utility::conversions::to_utf8string(j.at(U("lastLoginDate")).as_string());
        if (j.has_field(U("lastWorkoutDate"))) 
            player.lastWorkoutDate = utility::conversions::to_utf8string(j.at(U("lastWorkoutDate")).as_string());
        
        // Parse arrays
        if (j.has_field(U("completedQuests")) && j.at(U("completedQuests")).is_array()) {
            auto arr = j.at(U("completedQuests")).as_array();
            for (const auto& item : arr) {
                player.completedQuests.push_back(utility::conversions::to_utf8string(item.as_string()));
            }
        }
        
        if (j.has_field(U("unlockedAchievements")) && j.at(U("unlockedAchievements")).is_array()) {
            auto arr = j.at(U("unlockedAchievements")).as_array();
            for (const auto& item : arr) {
                player.unlockedAchievements.push_back(utility::conversions::to_utf8string(item.as_string()));
            }
        }
        
        if (j.has_field(U("inventory")) && j.at(U("inventory")).is_array()) {
            auto arr = j.at(U("inventory")).as_array();
            for (const auto& item : arr) {
                player.inventory.push_back(utility::conversions::to_utf8string(item.as_string()));
            }
        }
        
        return player;
    }
};

struct GameQuest {
    std::string id;
    std::string title;
    std::string description;
    std::string type;
    int targetValue;
    int rewardExp;
    int rewardGold;
    int rewardGems;
    std::vector<std::string> rewardItems;
    bool repeatable = false;
    
    json::value toJson() const {
        json::value j = json::value::object();
        j[U("id")] = json::value::string(utility::conversions::to_string_t(id));
        j[U("title")] = json::value::string(utility::conversions::to_string_t(title));
        j[U("description")] = json::value::string(utility::conversions::to_string_t(description));
        j[U("type")] = json::value::string(utility::conversions::to_string_t(type));
        j[U("targetValue")] = json::value::number(targetValue);
        j[U("rewardExp")] = json::value::number(rewardExp);
        j[U("rewardGold")] = json::value::number(rewardGold);
        j[U("rewardGems")] = json::value::number(rewardGems);
        j[U("repeatable")] = json::value::boolean(repeatable);
        
        json::value itemsArr = json::value::array(rewardItems.size());
        for (size_t i = 0; i < rewardItems.size(); ++i) {
            itemsArr[i] = json::value::string(utility::conversions::to_string_t(rewardItems[i]));
        }
        j[U("rewardItems")] = itemsArr;
        
        return j;
    }
};

struct GameAchievement {
    std::string id;
    std::string title;
    std::string description;
    std::string icon;
    std::string condition;
    int conditionValue;
    int rewardGems;
    
    json::value toJson() const {
        json::value j = json::value::object();
        j[U("id")] = json::value::string(utility::conversions::to_string_t(id));
        j[U("title")] = json::value::string(utility::conversions::to_string_t(title));
        j[U("description")] = json::value::string(utility::conversions::to_string_t(description));
        j[U("icon")] = json::value::string(utility::conversions::to_string_t(icon));
        j[U("condition")] = json::value::string(utility::conversions::to_string_t(condition));
        j[U("conditionValue")] = json::value::number(conditionValue);
        j[U("rewardGems")] = json::value::number(rewardGems);
        return j;
    }
};

// ============================================================================
// GAME ENGINE CORE
// ============================================================================

class GameEngine {
private:
    std::shared_ptr<Config::Database> database;
    std::shared_ptr<GameSync::GameSyncEngine> syncEngine;
    
    std::unordered_map<std::string, GamePlayer> activePlayers;
    std::unordered_map<std::string, GameQuest> quests;
    std::unordered_map<std::string, GameAchievement> achievements;
    
    time_t lastDailyReset = 0;
    
    std::string getCurrentDate() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time);
        std::stringstream ss;
        ss << std::put_time(&tm, "%Y-%m-%d");
        return ss.str();
    }
    
    void initializeDefaultContent() {
        quests["daily_steps_5000"] = {"daily_steps_5000", "Morning Walk", "Walk 5,000 steps today", "steps", 5000, 50, 25, 0, {}, true};
        quests["daily_calories_200"] = {"daily_calories_200", "Calorie Burn", "Burn 200 calories today", "calories", 200, 75, 35, 0, {}, true};
        quests["daily_login"] = {"daily_login", "Daily Login", "Login to the game", "login", 1, 30, 10, 1, {}, true};
        quests["weekly_workouts_3"] = {"weekly_workouts_3", "Weekly Warrior", "Complete 3 workouts this week", "workouts", 3, 200, 100, 5, {"energy_potion"}, false};
        quests["weekly_streak_5"] = {"weekly_streak_5", "Consistency King", "Maintain a 5-day streak", "streak", 5, 150, 75, 3, {}, false};
        
        achievements["first_10k_steps"] = {"first_10k_steps", "Marathon Walker", "Walk 10,000 steps in one day", "🏃", "steps", 10000, 20};
        achievements["level_10"] = {"level_10", "Fitness Pro", "Reach level 10", "⭐", "level", 10, 50};
        achievements["streak_30"] = {"streak_30", "Monthly Champion", "30-day streak", "🔥", "streak", 30, 100};
        achievements["quest_master"] = {"quest_master", "Quest Master", "Complete 50 quests", "🏆", "quests", 50, 200};
    }
    
    void checkAndUpdateStreak(GamePlayer& player) {
        std::string today = getCurrentDate();
        
        if (player.lastLoginDate.empty()) {
            player.streak = 1;
        } else if (player.lastLoginDate == today) {
            return;
        } else {
            std::tm lastDate = {}, todayDate = {};
            std::istringstream lastStream(player.lastLoginDate);
            std::istringstream todayStream(today);
            
            lastStream >> std::get_time(&lastDate, "%Y-%m-%d");
            todayStream >> std::get_time(&todayDate, "%Y-%m-%d");
            
            std::time_t lastTime = std::mktime(&lastDate);
            std::time_t todayTime = std::mktime(&todayDate);
            
            double diffDays = std::difftime(todayTime, lastTime) / (60 * 60 * 24);
            
            if (diffDays == 1) {
                player.streak++;
            } else if (diffDays > 1) {
                player.streak = 1;
            }
        }
        
        player.lastLoginDate = today;
    }
    
    void checkLevelUp(GamePlayer& player) {
        while (player.experience >= player.nextLevelExp) {
            player.level++;
            player.experience -= player.nextLevelExp;
            player.strength += 2;
            player.stamina += 3;
            player.agility += 1;
            player.intelligence += 1;
            player.nextLevelExp = static_cast<int>(player.nextLevelExp * 1.5);
            player.gold += player.level * 50;
            player.gems += player.level;
            checkAchievement(player, "level", player.level);
        }
    }
    
    void checkAchievement(GamePlayer& player, const std::string& type, int value) {
        for (const auto& [id, achievement] : achievements) {
            if (achievement.condition == type && value >= achievement.conditionValue) {
                if (std::find(player.unlockedAchievements.begin(), 
                              player.unlockedAchievements.end(), id) == player.unlockedAchievements.end()) {
                    player.unlockedAchievements.push_back(id);
                    player.gems += achievement.rewardGems;
                }
            }
        }
    }
    
    std::vector<json::value> checkQuests(GamePlayer& player) {
        std::vector<json::value> completed;
        
        for (const auto& [id, quest] : quests) {
            if (!quest.repeatable && 
                std::find(player.completedQuests.begin(), 
                         player.completedQuests.end(), id) != player.completedQuests.end()) {
                continue;
            }
            
            bool questComplete = false;
            
            if (quest.type == "steps" && player.totalSteps >= quest.targetValue) {
                questComplete = true;
            } else if (quest.type == "calories" && player.totalCalories >= quest.targetValue) {
                questComplete = true;
            } else if (quest.type == "workouts" && player.totalWorkouts >= quest.targetValue) {
                questComplete = true;
            } else if (quest.type == "streak" && player.streak >= quest.targetValue) {
                questComplete = true;
            } else if (quest.type == "login" && !player.lastLoginDate.empty()) {
                questComplete = true;
            }
            
            if (questComplete) {
                player.experience += quest.rewardExp;
                player.gold += quest.rewardGold;
                player.gems += quest.rewardGems;
                
                for (const auto& item : quest.rewardItems) {
                    player.inventory.push_back(item);
                }
                
                player.completedQuests.push_back(id);
                
                json::value questJson = quest.toJson();
                json::value rewards = json::value::object();
                rewards[U("experience")] = json::value::number(quest.rewardExp);
                rewards[U("gold")] = json::value::number(quest.rewardGold);
                rewards[U("gems")] = json::value::number(quest.rewardGems);
                
                json::value itemsArr = json::value::array(quest.rewardItems.size());
                for (size_t i = 0; i < quest.rewardItems.size(); ++i) {
                    itemsArr[i] = json::value::string(utility::conversions::to_string_t(quest.rewardItems[i]));
                }
                rewards[U("items")] = itemsArr;
                questJson[U("rewards")] = rewards;
                
                completed.push_back(questJson);
            }
        }
        
        return completed;
    }
    
public:
    GameEngine(std::shared_ptr<Config::Database> db, 
               std::shared_ptr<GameSync::GameSyncEngine> sync)
        : database(db), syncEngine(sync) {
        initializeDefaultContent();
        lastDailyReset = time(nullptr);
    }
    
    GamePlayer initializePlayer(const std::string& userId) {
        try {
            FitnessDB::User dbUser = database->getUser(userId);
            
            GamePlayer player;
            player.userId = userId;
            player.username = dbUser.username;
            player.characterName = dbUser.username + "'s Hero";
            player.level = dbUser.fitness_level;
            player.experience = dbUser.experience_points;
            player.nextLevelExp = 100 * player.level * player.level;
            
            auto workouts = database->getUserWorkouts(userId);
            player.totalWorkouts = static_cast<int>(workouts.size());
            
            for (const auto& workout : workouts) {
                player.totalCalories += static_cast<int>(workout.total_calories);
                player.totalSteps += 1000;
            }
            
            checkAndUpdateStreak(player);
            activePlayers[userId] = player;
            
            return player;
            
        } catch (const std::exception& e) {
            GamePlayer player;
            player.userId = userId;
            player.username = "Player";
            player.characterName = "Fitness Hero";
            player.level = 1;
            player.experience = 0;
            player.nextLevelExp = 100;
            player.gold = 100;
            player.energy = 100;
            player.lastLoginDate = getCurrentDate();
            
            activePlayers[userId] = player;
            return player;
        }
    }
    
    json::value updateFromFitnessData(const std::string& userId, const json::value& fitnessData) {
        if (activePlayers.find(userId) == activePlayers.end()) {
            initializePlayer(userId);
        }
        
        GamePlayer& player = activePlayers[userId];
        
        int steps = 0, calories = 0, workoutDuration = 0;
        std::string workoutType;
        
        if (fitnessData.has_field(U("steps"))) steps = fitnessData.at(U("steps")).as_integer();
        if (fitnessData.has_field(U("calories"))) calories = fitnessData.at(U("calories")).as_integer();
        if (fitnessData.has_field(U("duration"))) workoutDuration = fitnessData.at(U("duration")).as_integer();
        if (fitnessData.has_field(U("type"))) workoutType = utility::conversions::to_utf8string(fitnessData.at(U("type")).as_string());
        
        player.totalSteps += steps;
        player.totalCalories += calories;
        
        if (workoutDuration > 0) {
            player.totalWorkouts++;
            player.lastWorkoutDate = getCurrentDate();
        }
        
        int expEarned = (steps / 100) + (calories / 10) + (workoutDuration * 2);
        int goldEarned = (steps / 200) + (calories / 20) + (workoutDuration);
        
        player.experience += expEarned;
        player.gold += goldEarned;
        
        checkLevelUp(player);
        
        std::vector<json::value> completedQuests = checkQuests(player);
        
        checkAchievement(player, "steps", player.totalSteps);
        checkAchievement(player, "calories", player.totalCalories);
        checkAchievement(player, "workouts", player.totalWorkouts);
        checkAchievement(player, "streak", player.streak);
        
        syncEngine->syncWorkout(userId, "workout_" + std::to_string(time(nullptr)));
        
        json::value response = json::value::object();
        response[U("success")] = json::value::boolean(true);
        response[U("player")] = player.toJson();
        
        json::value rewards = json::value::object();
        rewards[U("experience")] = json::value::number(expEarned);
        rewards[U("gold")] = json::value::number(goldEarned);
        response[U("rewards")] = rewards;
        
        json::value questsArr = json::value::array(completedQuests.size());
        for (size_t i = 0; i < completedQuests.size(); ++i) {
            questsArr[i] = completedQuests[i];
        }
        response[U("completedQuests")] = questsArr;
        
        response[U("levelUp")] = json::value::boolean(player.experience >= player.nextLevelExp);
        
        return response;
    }
    
    json::value getPlayerState(const std::string& userId) {
        if (activePlayers.find(userId) == activePlayers.end()) {
            initializePlayer(userId);
        }
        
        json::value response = json::value::object();
        response[U("success")] = json::value::boolean(true);
        response[U("player")] = activePlayers[userId].toJson();
        
        auto availableQuests = getAvailableQuests(userId);
        json::value questsArr = json::value::array(availableQuests.size());
        for (size_t i = 0; i < availableQuests.size(); ++i) {
            questsArr[i] = availableQuests[i];
        }
        response[U("availableQuests")] = questsArr;
        
        auto achievements = getUnlockedAchievements(userId);
        json::value achArr = json::value::array(achievements.size());
        for (size_t i = 0; i < achievements.size(); ++i) {
            achArr[i] = achievements[i];
        }
        response[U("unlockedAchievements")] = achArr;
        
        return response;
    }
    
    std::vector<json::value> getAvailableQuests(const std::string& userId) {
        std::vector<json::value> availableQuests;
        
        if (activePlayers.find(userId) == activePlayers.end()) {
            return availableQuests;
        }
        
        const GamePlayer& player = activePlayers[userId];
        
        for (const auto& [id, quest] : quests) {
            if (!quest.repeatable && 
                std::find(player.completedQuests.begin(), 
                         player.completedQuests.end(), id) != player.completedQuests.end()) {
                continue;
            }
            
            if (player.level >= 1) {
                availableQuests.push_back(quest.toJson());
            }
        }
        
        return availableQuests;
    }
    
    std::vector<json::value> getUnlockedAchievements(const std::string& userId) {
        std::vector<json::value> unlocked;
        
        if (activePlayers.find(userId) == activePlayers.end()) {
            return unlocked;
        }
        
        const GamePlayer& player = activePlayers[userId];
        
        for (const auto& achId : player.unlockedAchievements) {
            if (achievements.find(achId) != achievements.end()) {
                unlocked.push_back(achievements[achId].toJson());
            }
        }
        
        return unlocked;
    }
    
    json::value completeQuest(const std::string& userId, const std::string& questId) {
        json::value response = json::value::object();
        
        if (activePlayers.find(userId) == activePlayers.end() || 
            quests.find(questId) == quests.end()) {
            response[U("success")] = json::value::boolean(false);
            response[U("error")] = json::value::string(U("Invalid player or quest"));
            return response;
        }
        
        GamePlayer& player = activePlayers[userId];
        const GameQuest& quest = quests[questId];
        
        if (!quest.repeatable && 
            std::find(player.completedQuests.begin(), 
                     player.completedQuests.end(), questId) != player.completedQuests.end()) {
            response[U("success")] = json::value::boolean(false);
            response[U("error")] = json::value::string(U("Quest already completed"));
            return response;
        }
        
        player.experience += quest.rewardExp;
        player.gold += quest.rewardGold;
        player.gems += quest.rewardGems;
        
        for (const auto& item : quest.rewardItems) {
            player.inventory.push_back(item);
        }
        
        player.completedQuests.push_back(questId);
        checkLevelUp(player);
        checkAchievement(player, "quests", static_cast<int>(player.completedQuests.size()));
        
        response[U("success")] = json::value::boolean(true);
        
        json::value rewards = json::value::object();
        rewards[U("experience")] = json::value::number(quest.rewardExp);
        rewards[U("gold")] = json::value::number(quest.rewardGold);
        rewards[U("gems")] = json::value::number(quest.rewardGems);
        
        json::value itemsArr = json::value::array(quest.rewardItems.size());
        for (size_t i = 0; i < quest.rewardItems.size(); ++i) {
            itemsArr[i] = json::value::string(utility::conversions::to_string_t(quest.rewardItems[i]));
        }
        rewards[U("items")] = itemsArr;
        
        response[U("rewards")] = rewards;
        response[U("player")] = player.toJson();
        
        return response;
    }
    
    json::value getLeaderboard(const std::string& type = "level", int limit = 10) {
        json::value response = json::value::object();
        response[U("success")] = json::value::boolean(true);
        response[U("type")] = json::value::string(utility::conversions::to_string_t(type));
        
        json::value playersArr = json::value::array(std::min(limit, 10));
        
        for (int i = 1; i <= std::min(limit, 10); i++) {
            json::value player = json::value::object();
            player[U("rank")] = json::value::number(i);
            player[U("username")] = json::value::string(U("Player_") + utility::conversions::to_string_t(std::to_string(i)));
            player[U("level")] = json::value::number(10 + i);
            player[U("experience")] = json::value::number(i * 1000);
            player[U("totalSteps")] = json::value::number(i * 10000);
            player[U("totalWorkouts")] = json::value::number(i * 5);
            playersArr[i - 1] = player;
        }
        
        response[U("players")] = playersArr;
        return response;
    }
    
    json::value claimDailyReward(const std::string& userId) {
        json::value response = json::value::object();
        
        if (activePlayers.find(userId) == activePlayers.end()) {
            initializePlayer(userId);
        }
        
        GamePlayer& player = activePlayers[userId];
        std::string today = getCurrentDate();
        
        if (player.lastLoginDate == today) {
            response[U("success")] = json::value::boolean(false);
            response[U("error")] = json::value::string(U("Daily reward already claimed today"));
            return response;
        }
        
        int baseReward = 50;
        int streakBonus = player.streak * 10;
        int totalGold = baseReward + streakBonus;
        
        player.gold += totalGold;
        player.gems += 1;
        player.lastLoginDate = today;
        
        response[U("success")] = json::value::boolean(true);
        
        json::value rewards = json::value::object();
        rewards[U("gold")] = json::value::number(totalGold);
        rewards[U("gems")] = json::value::number(1);
        rewards[U("streak")] = json::value::number(player.streak);
        rewards[U("streakBonus")] = json::value::number(streakBonus);
        response[U("rewards")] = rewards;
        
        response[U("player")] = player.toJson();
        
        return response;
    }
};

} // namespace Game
} // namespace FitnessQuest

#endif // FITNESS_QUEST_GAME_ENGINE_HPP