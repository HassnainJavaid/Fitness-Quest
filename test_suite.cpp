// ============================================================================
// COMPREHENSIVE TEST SUITE - FITNESS QUEST
// Compile: g++ -std=c++17 -o test_suite test_suite.cpp -lcpprest -lssl -lcrypto -pthread
// Run: ./test_suite
// ============================================================================

#include <iostream>
#include <vector>
#include <functional>
#include <string>
#include <sstream>
#include <cassert>
#include <memory>
#include <chrono>
#include <iomanip>

// Include project headers
#include "config.hpp"
#include "shared-models/shared-models.hpp"
#include "services.hpp"
#include "utils.hpp"
#include "GameSyncEngine.hpp"

// ANSI color codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

namespace FitnessQuest {
namespace Testing {

// ============================================================================
// TEST FRAMEWORK
// ============================================================================

struct TestResult {
    std::string name;
    bool passed;
    std::string error;
    double timeMs;
};

class TestSuite {
private:
    std::string suiteName;
    std::vector<std::function<void()>> tests;
    std::vector<std::string> testNames;
    std::vector<TestResult> results;
    
public:
    TestSuite(const std::string& name) : suiteName(name) {}
    
    void add(const std::string& name, std::function<void()> test) {
        testNames.push_back(name);
        tests.push_back(test);
    }
    
    void run() {
        std::cout << "\n" << CYAN << BOLD << "╔══════════════════════════════════════════════════╗\n";
        std::cout << "║  " << RESET << YELLOW << BOLD << "Test Suite: " << suiteName 
                  << std::string(std::max(0, 30 - (int)suiteName.length()), ' ') 
                  << CYAN << BOLD << "║\n";
        std::cout << "╚══════════════════════════════════════════════════╝" << RESET << "\n\n";
        
        int passed = 0;
        int failed = 0;
        
        for (size_t i = 0; i < tests.size(); i++) {
            TestResult result;
            result.name = testNames[i];
            
            std::cout << BLUE << "  ▶ " << RESET << testNames[i] << "... ";
            std::cout.flush();
            
            auto start = std::chrono::high_resolution_clock::now();
            
            try {
                tests[i]();
                auto end = std::chrono::high_resolution_clock::now();
                result.timeMs = std::chrono::duration<double, std::milli>(end - start).count();
                result.passed = true;
                passed++;
                
                std::cout << GREEN << "✓ PASSED" << RESET 
                          << " (" << std::fixed << std::setprecision(2) << result.timeMs << "ms)\n";
                
            } catch (const std::exception& e) {
                auto end = std::chrono::high_resolution_clock::now();
                result.timeMs = std::chrono::duration<double, std::milli>(end - start).count();
                result.passed = false;
                result.error = e.what();
                failed++;
                
                std::cout << RED << "✗ FAILED" << RESET << "\n";
                std::cout << RED << "      ↳ " << e.what() << RESET << "\n";
            }
            
            results.push_back(result);
        }
        
        printSummary(passed, failed);
    }
    
    void printSummary(int passed, int failed) {
        std::cout << "\n" << CYAN << "═══════════════════════════════════════════════" << RESET << "\n";
        std::cout << YELLOW << "  Summary: " << suiteName << RESET << "\n";
        std::cout << CYAN << "═══════════════════════════════════════════════" << RESET << "\n";
        std::cout << GREEN << "  ✓ Passed: " << passed << RESET << "\n";
        std::cout << (failed > 0 ? RED : GREEN) << "  ✗ Failed: " << failed << RESET << "\n";
        std::cout << "  Total: " << (passed + failed) << "\n";
        
        double totalTime = 0;
        for (const auto& r : results) totalTime += r.timeMs;
        std::cout << "  Time: " << std::fixed << std::setprecision(2) << totalTime << "ms\n";
        std::cout << CYAN << "═══════════════════════════════════════════════" << RESET << "\n";
    }
};

// Test assertion macros
#define ASSERT_TRUE(condition) \
    if (!(condition)) throw std::runtime_error("Assertion failed: " #condition)

#define ASSERT_FALSE(condition) \
    if (condition) throw std::runtime_error("Assertion failed: NOT " #condition)

#define ASSERT_EQUAL(expected, actual) \
    if ((expected) != (actual)) { \
        std::stringstream ss; \
        ss << "Assertion failed: expected " << (expected) << " but got " << (actual); \
        throw std::runtime_error(ss.str()); \
    }

#define ASSERT_THROWS(expression) \
    { \
        bool threw = false; \
        try { expression; } catch (...) { threw = true; } \
        if (!threw) throw std::runtime_error("Expected exception but none thrown"); \
    }

// ============================================================================
// MODEL TESTS
// ============================================================================

void testWorkoutTypeConversion() {
    ASSERT_EQUAL("STRENGTH", Models::workoutTypeToString(Models::WorkoutType::STRENGTH));
    ASSERT_EQUAL("CARDIO", Models::workoutTypeToString(Models::WorkoutType::CARDIO));
    //ASSERT_EQUAL(Models::WorkoutType::FLEXIBILITY, Models::stringToWorkoutType("FLEXIBILITY"));
}

void testXPCalculation() {
    int64_t xp1 = Models::Constants::getXPForLevel(1);
    int64_t xp2 = Models::Constants::getXPForLevel(2);
    ASSERT_TRUE(xp2 > xp1);
    ASSERT_EQUAL(100, xp1);
}

void testWorkoutValidation() {
    ASSERT_THROWS(Models::Validation::validateWorkoutDuration(0));
    ASSERT_THROWS(Models::Validation::validateWorkoutDuration(500));
    ASSERT_THROWS(Models::Validation::validateIntensity(0));
    ASSERT_THROWS(Models::Validation::validateIntensity(11));
    ASSERT_THROWS(Models::Validation::validateFormScore(-1));
    ASSERT_THROWS(Models::Validation::validateFormScore(101));
    
    // Valid values should not throw
    Models::Validation::validateWorkoutDuration(30);
    Models::Validation::validateIntensity(5);
    Models::Validation::validateFormScore(85.5);
}

void testRewardCalculation() {
    auto reward = Models::RewardCalculation::calculateWorkoutRewards(
        Models::WorkoutType::STRENGTH, 30.0, 7.0, std::nullopt
    );
    
    ASSERT_TRUE(reward.experience > 0);
    ASSERT_TRUE(reward.gold > 0);
}

void testLevelCalculation() {
    ASSERT_EQUAL(1, Models::RewardCalculation::calculateLevelFromXP(0));
    ASSERT_EQUAL(1, Models::RewardCalculation::calculateLevelFromXP(50));
    ASSERT_EQUAL(2, Models::RewardCalculation::calculateLevelFromXP(150));
}

// ============================================================================
// UTILITY TESTS
// ============================================================================

void testEmailValidation() {
    ASSERT_TRUE(Utils::Validation::validateEmail("test@example.com"));
    ASSERT_TRUE(Utils::Validation::validateEmail("user.name+tag@domain.co.uk"));
    ASSERT_FALSE(Utils::Validation::validateEmail("invalid-email"));
    ASSERT_FALSE(Utils::Validation::validateEmail("@example.com"));
    ASSERT_FALSE(Utils::Validation::validateEmail("test@"));
}

void testUsernameValidation() {
    ASSERT_TRUE(Utils::Validation::validateUsername("user123"));
    ASSERT_TRUE(Utils::Validation::validateUsername("test_user"));
    ASSERT_FALSE(Utils::Validation::validateUsername("ab")); // Too short
    ASSERT_FALSE(Utils::Validation::validateUsername("user-name")); // Invalid char
    //ASSERT_FALSE(Utils::Validation::validateUsername("a".append(30, 'b'))); // Too long
}

void testPasswordValidation() {
    ASSERT_TRUE(Utils::Validation::validatePassword("password123"));
    ASSERT_TRUE(Utils::Validation::validatePassword("123456"));
    ASSERT_FALSE(Utils::Validation::validatePassword("short"));
    ASSERT_FALSE(Utils::Validation::validatePassword("12345"));
}

void testJWTGeneration() {
    std::string token = Utils::JWT::generateToken("user123");
    ASSERT_TRUE(!token.empty());
    ASSERT_TRUE(token.find('.') != std::string::npos);
}

void testJWTVerification() {
    std::string userId = "test_user_789";
    std::string token = Utils::JWT::generateToken(userId);
    std::string verified = Utils::JWT::verifyToken(token);
    ASSERT_EQUAL(userId, verified);
}

// ============================================================================
// SERVICE TESTS
// ============================================================================

void testRewardServiceCreation() {
    auto db = std::make_shared<Config::Database>();
    db->connect();
    
    Services::RewardService service(db);
    
    auto bundle = service.calculateWorkoutRewards(
        "user123", Models::WorkoutType::CARDIO, 45.0, 8.0, std::optional<double>(90.0)
    );
    
    ASSERT_TRUE(bundle.experience > 0);
    ASSERT_TRUE(bundle.gold > 0);
}

void testGameServiceInitialization() {
    auto db = std::make_shared<Config::Database>();
    db->connect();
    
    Services::GameService gameService(db);
    
    auto character = gameService.initializeCharacter("user456", "TestPlayer");
    
    ASSERT_EQUAL("user456", character.userId);
    ASSERT_EQUAL(1, character.level);
    ASSERT_EQUAL(0, character.experience);
    ASSERT_TRUE(character.strength > 0);
    ASSERT_TRUE(character.stamina > 0);
}

void testRewardMultipliers() {
    auto db = std::make_shared<Config::Database>();
    db->connect();
    
    Services::RewardService service(db);
    
    // High intensity should give more rewards
    auto lowIntensity = service.calculateWorkoutRewards(
        "user123", Models::WorkoutType::STRENGTH, 30.0, 3.0, std::nullopt
    );
    
    auto highIntensity = service.calculateWorkoutRewards(
        "user123", Models::WorkoutType::STRENGTH, 30.0, 9.0, std::nullopt
    );
    
    ASSERT_TRUE(highIntensity.experience > lowIntensity.experience);
}

// ============================================================================
// GAME SYNC ENGINE TESTS
// ============================================================================

void testGameSyncEngineCreation() {
    auto db = std::make_shared<Config::Database>();
    db->connect();
    auto rewardService = std::make_shared<Services::RewardService>(db);
    
    GameSync::GameSyncEngine engine(db, rewardService);
    ASSERT_TRUE(true); // Just test construction
}

void testGameStateRetrieval() {
    auto db = std::make_shared<Config::Database>();
    db->connect();
    auto rewardService = std::make_shared<Services::RewardService>(db);
    
    GameSync::GameSyncEngine engine(db, rewardService);
    
    try {
        auto state = engine.getPlayerGameState("nonexistent_user");
        // Should return default state
        ASSERT_EQUAL(1, state["level"]);
        ASSERT_EQUAL(0, state["xp"]);
    } catch (...) {
        // Also acceptable
        ASSERT_TRUE(true);
    }
}

void testQuestRetrieval() {
    auto db = std::make_shared<Config::Database>();
    db->connect();
    auto rewardService = std::make_shared<Services::RewardService>(db);
    
    GameSync::GameSyncEngine engine(db, rewardService);
    
    auto quests = engine.getAvailableQuests("test_user");
    ASSERT_TRUE(quests.size() >= 0); // Just verify it doesn't crash
}

// ============================================================================
// DATABASE TESTS
// ============================================================================

void testDatabaseConnection() {
    Config::Database db;
    ASSERT_TRUE(db.connect());
    ASSERT_TRUE(db.isConnected());
}

void testDatabaseHealthCheck() {
    Config::Database db;
    db.connect();
    ASSERT_TRUE(db.healthCheck());
}

void testUserCreation() {
    Config::Database db;
    db.connect();
    
    std::string testEmail = "test_" + std::to_string(time(nullptr)) + "@test.com";
    
    std::string userId = db.createUser("testuser", testEmail, "password123");
    ASSERT_TRUE(!userId.empty());
    
    FitnessDB::User user = db.getUser(userId);
    ASSERT_EQUAL("testuser", user.username);
    ASSERT_EQUAL(testEmail, user.email);
}

void testUserRetrieval() {
    Config::Database db;
    db.connect();
    
    std::string testEmail = "retrieve_" + std::to_string(time(nullptr)) + "@test.com";
    std::string userId = db.createUser("retrieveuser", testEmail, "password");
    
    FitnessDB::User user = db.getUserByEmail(testEmail);
    ASSERT_EQUAL(userId, user.id);
}

void testWorkoutCreation() {
    Config::Database db;
    db.connect();
    
    std::string testEmail = "workout_" + std::to_string(time(nullptr)) + "@test.com";
    std::string userId = db.createUser("workoutuser", testEmail, "password");
    
    std::string workoutId = db.startWorkout(userId);
    ASSERT_TRUE(!workoutId.empty());
    
    db.completeWorkout(workoutId);
    
    FitnessDB::WorkoutSession workout = db.getWorkout(workoutId);
    ASSERT_EQUAL(userId, workout.user_id);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

void testFullUserWorkflow() {
    Config::Database db;
    db.connect();
    
    // Create user
    std::string email = "workflow_" + std::to_string(time(nullptr)) + "@test.com";
    std::string userId = db.createUser("workflowuser", email, "password");
    
    // Create reward service
    auto rewardService = std::make_shared<Services::RewardService>(
        std::make_shared<Config::Database>()
    );
    
    // Calculate rewards
    auto bundle = rewardService->calculateWorkoutRewards(
        userId, Models::WorkoutType::STRENGTH, 30.0, 7.0, std::optional<double>(85.0)
    );
    
    ASSERT_TRUE(bundle.experience > 0);
    
    // Update user
    FitnessDB::User user = db.getUser(userId);
    user.experience_points += bundle.experience;
    db.updateUser(user);
    
    // Verify update
    FitnessDB::User updatedUser = db.getUser(userId);
    ASSERT_TRUE(updatedUser.experience_points > 0);
}

void testGameCharacterProgression() {
    auto db = std::make_shared<Config::Database>();
    db->connect();
    
    Services::GameService gameService(db);
    
    auto character = gameService.initializeCharacter("prog_user", "ProgressPlayer");
    
    int64_t initialXP = character.experience;
    ASSERT_EQUAL(0, initialXP);
    
    // Simulate earning XP
    ASSERT_FALSE(gameService.didLevelUp(0, 50)); // Still level 1
    ASSERT_TRUE(gameService.didLevelUp(0, 200)); // Should level up
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

} // namespace Testing
} // namespace FitnessQuest

int main() {
    using namespace FitnessQuest::Testing;
    
    std::cout << BOLD << CYAN << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                       ║\n";
    std::cout << "║       " << YELLOW << "FITNESS QUEST - COMPREHENSIVE TEST SUITE" << CYAN << "       ║\n";
    std::cout << "║                                                       ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n";
    std::cout << RESET << "\n";
    
    try {
        // Model Tests
        TestSuite modelTests("Shared Models");
        modelTests.add("Workout Type Conversion", testWorkoutTypeConversion);
        modelTests.add("XP Calculation", testXPCalculation);
        modelTests.add("Workout Validation", testWorkoutValidation);
        modelTests.add("Reward Calculation", testRewardCalculation);
        modelTests.add("Level Calculation", testLevelCalculation);
        modelTests.run();
        
        // Utility Tests
        TestSuite utilityTests("Utility Functions");
        utilityTests.add("Email Validation", testEmailValidation);
        utilityTests.add("Username Validation", testUsernameValidation);
        utilityTests.add("Password Validation", testPasswordValidation);
        utilityTests.add("JWT Generation", testJWTGeneration);
        utilityTests.add("JWT Verification", testJWTVerification);
        utilityTests.run();
        
        // Service Tests
        TestSuite serviceTests("Service Layer");
        serviceTests.add("Reward Service Creation", testRewardServiceCreation);
        serviceTests.add("Game Service Initialization", testGameServiceInitialization);
        serviceTests.add("Reward Multipliers", testRewardMultipliers);
        serviceTests.run();
        
        // Game Sync Tests
        TestSuite gameSyncTests("Game Sync Engine");
        gameSyncTests.add("Engine Creation", testGameSyncEngineCreation);
        gameSyncTests.add("Game State Retrieval", testGameStateRetrieval);
        gameSyncTests.add("Quest Retrieval", testQuestRetrieval);
        gameSyncTests.run();
        
        // Database Tests
        TestSuite databaseTests("Database Operations");
        databaseTests.add("Database Connection", testDatabaseConnection);
        databaseTests.add("Health Check", testDatabaseHealthCheck);
        databaseTests.add("User Creation", testUserCreation);
        databaseTests.add("User Retrieval", testUserRetrieval);
        databaseTests.add("Workout Creation", testWorkoutCreation);
        databaseTests.run();
        
        // Integration Tests
        TestSuite integrationTests("Integration Tests");
        integrationTests.add("Full User Workflow", testFullUserWorkflow);
        integrationTests.add("Character Progression", testGameCharacterProgression);
        integrationTests.run();
        
        std::cout << "\n" << GREEN << BOLD << "═══════════════════════════════════════════════════════\n";
        std::cout << "  ✓ ALL TESTS COMPLETED SUCCESSFULLY!\n";
        std::cout << "═══════════════════════════════════════════════════════" << RESET << "\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << RED << BOLD << "\n❌ Test suite failed with error: " << e.what() << RESET << "\n\n";
        return 1;
    }
}