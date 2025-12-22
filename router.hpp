// ============================================================================
// ROUTER.HPP - Fully Updated & Synchronized with GameController
// ============================================================================

#ifndef FITNESS_QUEST_ROUTER_HPP
#define FITNESS_QUEST_ROUTER_HPP

#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <memory>
#include <regex>
#include <vector>
#include <functional>

#include "config.hpp"
#include "shared-models/shared-models.hpp"
#include "services.hpp"
#include "utils.hpp"
#include "GameSyncEngine.hpp"
#include "middleware.hpp"

using namespace web;
using namespace web::http;

namespace FitnessQuest {

// Forward declare controllers
namespace Controllers {
    class HealthController;
    class UserController;
    class AuthController;
    class WorkoutController;
    class QuestController;
    class GameController;
}

namespace Routes {

// ============================================================================
// ROUTER CLASS
// ============================================================================
class Router {
private:
    std::shared_ptr<Config::Database> database;
    std::shared_ptr<GameSync::GameSyncEngine> syncEngine;

    std::unique_ptr<Controllers::HealthController> healthController;
    std::unique_ptr<Controllers::UserController> userController;
    std::unique_ptr<Controllers::AuthController> authController;
    std::unique_ptr<Controllers::WorkoutController> workoutController;
    std::unique_ptr<Controllers::QuestController> questController;
    std::unique_ptr<Controllers::GameController> gameController;

    struct Route {
        std::string method;
        std::regex pathPattern;
        std::function<void(http_request, std::smatch)> handler;
    };

    std::vector<Route> routes;

public:
    Router(std::shared_ptr<Config::Database> db,
           std::shared_ptr<GameSync::GameSyncEngine> engine);

    void registerRoutes();

    void addRoute(const std::string& method, const std::string& pattern,
                  std::function<void(http_request, std::smatch)> handler) {
        routes.push_back({method, std::regex(pattern), handler});
    }

    void route(http_request request) {
        Middleware::Logger::logRequest(request);

        std::string method = request.method();
        std::string path = uri::decode(request.relative_uri().path());

        for (const auto& route : routes) {
            if (route.method == method) {
                std::smatch match;
                if (std::regex_match(path, match, route.pathPattern)) {
                    try {
                        route.handler(request, match);
                        return;
                    } catch (const std::exception& e) {
                        sendError(request, status_codes::InternalError, e.what());
                        return;
                    }
                }
            }
        }

        sendError(request, status_codes::NotFound,
                  "Endpoint not found: " + method + " " + path);
    }

    static void sendError(http_request request, status_code code,
                          const std::string& message) {
        Middleware::ErrorHandler::sendJsonError(request, code, message);
    }
};

} // namespace Routes
} // namespace FitnessQuest

// Include controllers
#include "controllers.hpp"

// ============================================================================
// ROUTER IMPLEMENTATION
// ============================================================================
namespace FitnessQuest {
namespace Routes {

inline Router::Router(std::shared_ptr<Config::Database> db,
                      std::shared_ptr<GameSync::GameSyncEngine> engine)
    : database(db), syncEngine(engine) {

    healthController = std::make_unique<Controllers::HealthController>(database);
    userController = std::make_unique<Controllers::UserController>(database);
    authController = std::make_unique<Controllers::AuthController>(database);
    workoutController = std::make_unique<Controllers::WorkoutController>(database);
    questController = std::make_unique<Controllers::QuestController>(database);
    gameController = std::make_unique<Controllers::GameController>(database, syncEngine);

    registerRoutes();
}

inline void Router::registerRoutes() {
    // -------------------------
    // HEALTH CHECK
    // -------------------------
    addRoute("GET", "^/health$", [this](http_request req, std::smatch) {
        healthController->getHealth(req);
    });

    // -------------------------
    // USER ROUTES
    // -------------------------
    addRoute("POST", "^/api/users$", [this](http_request req, std::smatch) {
        userController->createUser(req);
    });

    addRoute("GET", "^/api/users/([^/]+)$", [this](http_request req, std::smatch match) {
        userController->getUser(req, match[1].str());
    });

    // -------------------------
    // AUTH
    // -------------------------
    addRoute("POST", "^/api/auth/login$", [this](http_request req, std::smatch) {
        authController->login(req);
    });

    // -------------------------
    // WORKOUT
    // -------------------------
    addRoute("POST", "^/api/workouts$", [this](http_request req, std::smatch) {
        workoutController->logWorkout(req);
    });

    addRoute("GET", "^/api/workouts$", [this](http_request req, std::smatch) {
        workoutController->getWorkoutHistory(req);
    });

    addRoute("GET", "^/api/workouts/([^/]+)$", [this](http_request req, std::smatch match) {
        workoutController->getWorkout(req, match[1].str());
    });

    // -------------------------
    // QUEST ROUTES
    // -------------------------
    addRoute("GET", "^/api/quests$", [this](http_request req, std::smatch) {
        questController->getQuests(req);
    });

    addRoute("POST", "^/api/quests/complete$", [this](http_request req, std::smatch) {
        questController->completeQuest(req);
    });

    addRoute("GET", "^/api/quests/([^/]+)$", [this](http_request req, std::smatch match) {
        questController->getQuest(req, match[1].str());
    });

    // =====================================================
    //              GAME ENGINE ROUTES
    // =====================================================
    addRoute("GET", "^/api/game/state$", [this](http_request req, std::smatch) {
        gameController->syncGameState(req);
    });

    addRoute("GET", "^/api/game/stats$", [this](http_request req, std::smatch) {
        gameController->getPlayerStats(req);
    });

    addRoute("GET", "^/api/game/quests$", [this](http_request req, std::smatch) {
        gameController->getAvailableQuests(req);
    });

    addRoute("GET", "^/api/game/leaderboard$", [this](http_request req, std::smatch) {
        gameController->getLeaderboard(req);
    });

    addRoute("POST", "^/api/game/claim-reward$", [this](http_request req, std::smatch) {
        gameController->claimReward(req);
    });
}

} // namespace Routes
} // namespace FitnessQuest

#endif // FITNESS_QUEST_ROUTER_HPP
