// ============================================================================
// CORRECTED MAIN.CPP - FITNESS QUEST BACKEND
// Compile: g++ -std=c++17 -o fitness_quest main.cpp -lcpprest -lssl -lcrypto -pthread
// ============================================================================

#include <iostream>
#include <memory>
#include <signal.h>
#include <thread>
#include <chrono>

// Include headers in correct order
#include "config.hpp"
#include "shared-models/shared-models.hpp"
#include "services.hpp"
#include "utils.hpp"
#include "GameSyncEngine.hpp"
#include "middleware.hpp"
#include "router.hpp"

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

// Global variables
std::unique_ptr<FitnessQuest::Routes::Router> router;
std::unique_ptr<http_listener> listener;

void signalHandler(int signal) {
    std::cout << "\n⚠  Received signal " << signal << ", shutting down gracefully...\n";
    if (listener) {
        listener->close().wait();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    std::cout << "==========================================\n";
    std::cout << "🏋️  FITNESS QUEST BACKEND SERVER\n";
    std::cout << "==========================================\n\n";
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Load environment configuration
        std::cout << "📋 Loading configuration...\n";
        FitnessQuest::Config::Environment::load();
        
        // Initialize database
        std::cout << "🗄️  Initializing database...\n";
        auto database = std::make_shared<FitnessQuest::Config::Database>();
        
        if (!database->connect()) {
            std::cerr << "❌ Failed to connect to database\n";
            return 1;
        }
        
        // Initialize reward service
        std::cout << "🎮 Initializing game services...\n";
        auto rewardService = std::make_shared<FitnessQuest::Services::RewardService>(database);
        
        // Initialize game sync engine
        std::cout << "🔄 Initializing game sync engine...\n";
        auto syncEngine = std::make_shared<FitnessQuest::GameSync::GameSyncEngine>(
            database, rewardService
        );
        syncEngine->start();
        
        // Initialize router with all controllers
        std::cout << "🚦 Setting up routes...\n";
        router = std::make_unique<FitnessQuest::Routes::Router>(database, syncEngine);
        
        // Set up HTTP listener
        auto env = std::make_shared<FitnessQuest::Config::Environment>();
        std::string host = "http://0.0.0.0";
        int port = env->getServerPort();
        std::string address = host + ":" + std::to_string(port);
        
        listener = std::make_unique<http_listener>(address);
        
        // Main request handler
        listener->support([&](http_request request) {
            try {
                router->route(request);
            } catch (const std::exception& e) {
                FitnessQuest::Middleware::ErrorHandler::handleError(request, e);
            }
        });
        
        // CORS preflight handler
        listener->support(methods::OPTIONS, [](http_request request) {
            FitnessQuest::Middleware::CORS::handlePreflight(request);
        });
        
        // Start the server
        listener->open().wait();
        
        std::cout << "\n✅ Initialization complete!\n";
        std::cout << "🌐 Server running on: " << address << "\n";
        std::cout << "📊 Health check: " << address << "/health\n\n";
        
        std::cout << "📍 Available endpoints:\n";
        std::cout << "  POST /api/users              - Register new user\n";
        std::cout << "  POST /api/auth/login         - User login\n";
        std::cout << "  GET  /api/users/{id}         - Get user profile\n";
        std::cout << "  POST /api/workouts           - Log workout\n";
        std::cout << "  GET  /api/workouts           - Get workout history\n";
        std::cout << "  GET  /api/workouts/{id}      - Get specific workout\n";
        std::cout << "  GET  /api/quests             - Get all quests\n";
        std::cout << "  GET  /api/quests/{id}        - Get specific quest\n";
        std::cout << "  POST /api/quests/complete    - Complete a quest\n";
        std::cout << "  GET  /api/game/state         - Get game state\n";
        std::cout << "  GET  /api/game/stats         - Get player stats\n";
        std::cout << "  GET  /api/game/quests        - Get available quests\n";
        std::cout << "  GET  /api/game/leaderboard   - Get leaderboard\n";
        std::cout << "  POST /api/game/claim-reward  - Claim reward\n";
        
        std::cout << "\n🚀 Server is ready! Press Ctrl+C to stop\n";
        std::cout << "==========================================\n\n";
        
        // Keep server running
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ FATAL ERROR: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}