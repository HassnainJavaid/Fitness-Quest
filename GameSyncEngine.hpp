// ============================================================================
// GAME SYNC ENGINE - Real-time reality↔game synchronization
// ============================================================================

#ifndef FITNESS_QUEST_GAME_SYNC_HPP
#define FITNESS_QUEST_GAME_SYNC_HPP

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <memory>
#include "services.hpp"
#include "utils.hpp"
#include "shared-models/shared-models.hpp"

namespace FitnessQuest {
namespace GameSync {

// Forward declarations
class GameEvent;
class PlayerState;
class PlayerStats;

// ============================================================================
// PRIORITY QUEUE FOR SYNC JOBS
// ============================================================================
enum class SyncPriority {
    CRITICAL = 0,    // XP updates, level-ups (immediate)
    HIGH = 1,        // Quest completion, achievement unlocks
    MEDIUM = 2,      // Item rewards, stats updates  
    LOW = 3,         // Background sync, analytics
    BACKGROUND = 4   // Batch updates, cleanup
};

struct SyncJob {
    std::string jobId;
    std::string userId;
    SyncPriority priority;
    std::function<void()> task;
    time_t createdTime;
    
    bool operator<(const SyncJob& other) const {
        return static_cast<int>(priority) > static_cast<int>(other.priority);
    }
};

class SyncPriorityQueue {
private:
    std::priority_queue<SyncJob> queue;
    mutable std::mutex mtx;
    std::condition_variable cv;
    std::unordered_map<std::string, bool> processedJobs;
    
public:
    void push(const SyncJob& job) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(job);
        cv.notify_one();
    }
    
    SyncJob pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]() { return !queue.empty(); });
        SyncJob job = queue.top();
        queue.pop();
        return job;
    }
    
    bool isEmpty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }
    
    void markProcessed(const std::string& jobId) {
        std::lock_guard<std::mutex> lock(mtx);
        processedJobs[jobId] = true;
    }
    
    bool wasProcessed(const std::string& jobId) {
        std::lock_guard<std::mutex> lock(mtx);
        return processedJobs.find(jobId) != processedJobs.end();
    }
};

// ============================================================================
// SIMPLE GAME SYNC ENGINE (Minimal Version)
// ============================================================================
class GameSyncEngine {
private:
    std::shared_ptr<Config::Database> database;
    std::shared_ptr<Services::RewardService> rewardService;
    
    SyncPriorityQueue syncQueue;
    std::atomic<bool> running;
    std::thread workerThread;
    
    void workerLoop() {
        while (running) {
            if (!syncQueue.isEmpty()) {
                SyncJob job = syncQueue.pop();
                try {
                    job.task();
                    syncQueue.markProcessed(job.jobId);
                } catch (...) {
                    // Log error but continue
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
public:
    GameSyncEngine(std::shared_ptr<Config::Database> db,
                  std::shared_ptr<Services::RewardService> rewardSvc)
        : database(db), rewardService(rewardSvc), running(false) {}
    
    ~GameSyncEngine() {
        stop();
    }
    
    void start() {
        running = true;
        workerThread = std::thread(&GameSyncEngine::workerLoop, this);
    }
    
    void stop() {
        running = false;
        if (workerThread.joinable()) {
            workerThread.join();
        }
    }
    
    // Simple sync method
    void syncWorkout(const std::string& userId, const std::string& workoutId) {
        SyncJob job;
        job.jobId = "workout_" + userId + "_" + workoutId;
        job.userId = userId;
        job.priority = SyncPriority::CRITICAL;
        job.createdTime = time(nullptr);
        
        job.task = [this, userId, workoutId]() {
            // Simulate game sync - in real implementation, this would
            // send data to game server or update game state
            std::cout << "[GameSync] Syncing workout " << workoutId 
                      << " for user " << userId << std::endl;
        };
        
        syncQueue.push(job);
    }
    
    // Get player game state
    std::map<std::string, int> getPlayerGameState(const std::string& userId) {
        std::map<std::string, int> gameState;
        
        try {
            FitnessDB::User user = database->getUser(userId);
            
            // Basic game stats derived from fitness data
            gameState["level"] = user.fitness_level;
            gameState["xp"] = user.experience_points;
            gameState["strength"] = user.fitness_level * 10;
            gameState["stamina"] = user.fitness_level * 15;
            gameState["gold"] = user.experience_points / 10; // Convert XP to gold
            
            // Add workout-based stats
            auto workouts = database->getUserWorkouts(userId);
            gameState["workouts_completed"] = static_cast<int>(workouts.size());
            gameState["total_calories"] = 0;
            
            for (const auto& workout : workouts) {
                gameState["total_calories"] += static_cast<int>(workout.total_calories);
            }
            
        } catch (...) {
            // Return default state if user not found
            gameState["level"] = 1;
            gameState["xp"] = 0;
            gameState["strength"] = 10;
            gameState["stamina"] = 15;
            gameState["gold"] = 0;
            gameState["workouts_completed"] = 0;
            gameState["total_calories"] = 0;
        }
        
        return gameState;
    }
    
    // Get available quests for player
    std::vector<std::map<std::string, std::string>> getAvailableQuests(const std::string& userId) {
        std::vector<std::map<std::string, std::string>> quests;
        
        try {
            auto userQuests = database->getAllQuests();
            
            for (const auto& quest : userQuests) {
                if (!quest.completed) {
                    std::map<std::string, std::string> questInfo;
                    questInfo["id"] = quest.id;
                    questInfo["title"] = quest.title;
                    questInfo["description"] = quest.description;
                    questInfo["difficulty"] = std::to_string(quest.difficulty);
                    questInfo["priority"] = std::to_string(quest.priority);
                    quests.push_back(questInfo);
                }
            }
        } catch (...) {
            // Return some default quests
            std::map<std::string, std::string> defaultQuest;
            defaultQuest["id"] = "quest_1";
            defaultQuest["title"] = "First Workout";
            defaultQuest["description"] = "Complete your first workout";
            defaultQuest["difficulty"] = "1";
            defaultQuest["priority"] = "1";
            quests.push_back(defaultQuest);
        }
        
        return quests;
    }
};

} // namespace GameSync
} // namespace FitnessQuest

#endif // FITNESS_QUEST_GAME_SYNC_HPP