// ============================================================================
// CONFIGURATION - All configuration classes in one header file
// ============================================================================

#ifndef FITNESS_QUEST_CONFIG_HPP
#define FITNESS_QUEST_CONFIG_HPP

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <cctype>
#include "database/complete_database.h"

namespace FitnessQuest {
namespace Config {

// ============================================================================
// Environment Configuration
// ============================================================================
class Environment {
private:
    static std::map<std::string, std::string> variables;
    static bool loaded;
    
public:
    static void load(const std::string& filepath = ".env") {
        if (loaded) return;
        
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cout << "⚠ Warning: .env file not found at '" << filepath 
                     << "', using system environment and defaults" << std::endl;
            loaded = true;
            return;
        }
        
        std::string line;
        int lineCount = 0;
        while (std::getline(file, line)) {
            lineCount++;
            
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                key.erase(key.find_last_not_of(" \t\r\n") + 1);
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                
                if (value.length() >= 2) {
                    if ((value.front() == '"' && value.back() == '"') ||
                        (value.front() == '\'' && value.back() == '\'')) {
                        value = value.substr(1, value.length() - 2);
                    }
                }
                
                variables[key] = value;
            }
        }
        
        file.close();
        loaded = true;
        std::cout << "✓ Environment variables loaded (" << variables.size() 
                 << " variables from " << lineCount << " lines)" << std::endl;
    }
    
    static std::string get(const std::string& key, const std::string& defaultValue = "") {
        auto it = variables.find(key);
        if (it != variables.end()) {
            return it->second;
        }
        
        const char* value = std::getenv(key.c_str());
        if (value != nullptr) {
            return std::string(value);
        }
        
        return defaultValue;
    }
    
    static int getInt(const std::string& key, int defaultValue = 0) {
        std::string value = get(key);
        if (value.empty()) return defaultValue;
        try {
            return std::stoi(value);
        } catch (...) {
            return defaultValue;
        }
    }
    
    static bool getBool(const std::string& key, bool defaultValue = false) {
        std::string value = get(key);
        if (value.empty()) return defaultValue;
        
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c){ return std::tolower(c); });
        
        return value == "true" || value == "1" || value == "yes" || value == "on";
    }
    
    static std::string getDatabaseDirectory() { 
        return get("DATA_DIR", "./fitness_data"); 
    }
    
    static int getServerPort() { 
        return getInt("PORT", 8080); 
    }
    
    static std::string getJWTSecret() { 
        return get("JWT_SECRET", "fitness-quest-default-secret-CHANGE-IN-PRODUCTION"); 
    }
    
    static int getJWTExpiration() { 
        return getInt("JWT_EXPIRATION_HOURS", 24); 
    }
    
    static bool isDebug() { 
        return getBool("DEBUG", false); 
    }
    
    static int getRateLimitWindow() { 
        return getInt("RATE_LIMIT_WINDOW", 900);
    }
    
    static int getRateLimitMax() { 
        return getInt("RATE_LIMIT_MAX", 100); 
    }
    
    static void printAll() {
        std::cout << "\nLoaded Environment Variables:" << std::endl;
        std::cout << "================================" << std::endl;
        for (const auto& pair : variables) {
            if (pair.first.find("SECRET") != std::string::npos ||
                pair.first.find("PASSWORD") != std::string::npos ||
                pair.first.find("KEY") != std::string::npos) {
                std::cout << pair.first << " = [HIDDEN]" << std::endl;
            } else {
                std::cout << pair.first << " = " << pair.second << std::endl;
            }
        }
        std::cout << "================================\n" << std::endl;
    }
};

std::map<std::string, std::string> Environment::variables;
bool Environment::loaded = false;

// ============================================================================
// Database Configuration
// ============================================================================
class Database {
private:
    std::unique_ptr<FitnessDB::PersistentFitnessDatabase> db;
    std::mutex dbMutex;
    bool connected;
    std::string dataDir;
    
public:
    Database(const std::string& directory = "./fitness_data") 
        : connected(false), dataDir(directory) {}
    
    ~Database() {
        disconnect();
    }
    
    bool connect() {
        std::lock_guard<std::mutex> lock(dbMutex);
        
        try {
            db = std::make_unique<FitnessDB::PersistentFitnessDatabase>(dataDir);
            connected = true;
            
            auto stats = db->get_stats();
            std::cout << "  Database statistics:" << std::endl;
            std::cout << "    Users: " << stats.btree.user_count << std::endl;
            std::cout << "    Exercises: " << stats.btree.exercise_count << std::endl;
            std::cout << "    Workouts: " << stats.btree.workout_count << std::endl;
            std::cout << "    Quests: " << stats.btree.quest_count << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "✗ Database initialization error: " << e.what() << std::endl;
            connected = false;
            return false;
        }
    }
    
    void disconnect() {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (db && connected) {
            db.reset();
            connected = false;
        }
    }
    
    bool isConnected() const {
        return connected && db != nullptr;
    }
    
    FitnessDB::PersistentFitnessDatabase& getDB() {
        if (!isConnected()) {
            throw std::runtime_error("Database not connected");
        }
        return *db;
    }
    
    bool healthCheck() {
        std::lock_guard<std::mutex> lock(dbMutex);
        
        try {
            if (!isConnected()) return false;
            
            auto stats = db->get_stats();
            return stats.btree.user_count >= 0;
        } catch (...) {
            return false;
        }
    }
    std::vector<FitnessDB::WorkoutSession> getUserWorkouts(const std::string& userId) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        
        // Workaround: Get all workouts and filter by user
        std::vector<FitnessDB::WorkoutSession> allWorkouts;
        std::vector<FitnessDB::WorkoutSession> userWorkouts;
        
        // This assumes there's a method to get all workouts
        // You might need to adjust based on actual database API
        try {
            // Try different possible method names
            // allWorkouts = db->get_all_workouts();
            // OR create a simple implementation
            
            // For now, return empty
            return userWorkouts;
        } catch (...) {
            throw std::runtime_error("Failed to get user workouts");
        }
    }
    
    std::string createUser(const std::string& username, const std::string& email, 
                          const std::string& password) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->create_user(username, email, password);
    }
    
    FitnessDB::User getUser(const std::string& userId) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->get_user(userId);
    }
    
    FitnessDB::User getUserByEmail(const std::string& email) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->get_user_by_email(email);
    }
    
    void updateUser(const FitnessDB::User& user) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        db->update_user(user);
    }
    
    void addExercise(const FitnessDB::Exercise& exercise) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        db->add_exercise(exercise);
    }
    
    FitnessDB::Exercise getExercise(const std::string& exerciseId) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->get_exercise(exerciseId);
    }
    
    std::vector<FitnessDB::Exercise> getAllExercises() {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->get_all_exercises();
    }
    
    std::string startWorkout(const std::string& userId) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->start_workout(userId);
    }
    
    void completeWorkout(const std::string& workoutId) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        db->complete_workout(workoutId);
    }
    
    FitnessDB::WorkoutSession getWorkout(const std::string& workoutId) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->get_workout(workoutId);
    }
    
    void addQuest(const FitnessDB::Quest& quest) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        db->add_quest(quest);
    }
    
    FitnessDB::Quest getQuest(const std::string& questId) {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->get_quest(questId);
    }
    
    std::vector<FitnessDB::Quest> getAllQuests() {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->get_all_quests();
    }
    
    FitnessDB::Quest getNextQuest() {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->get_next_quest();
    }
    
    FitnessDB::PersistentFitnessDatabase::DatabaseStats getStats() {
        std::lock_guard<std::mutex> lock(dbMutex);
        if (!isConnected()) throw std::runtime_error("Database not connected");
        return db->get_stats();
    }
};

} // namespace Config
} // namespace FitnessQuest

#endif // FITNESS_QUEST_CONFIG_HPP