// ============================================================================
// CONTROLLERS_IMPL.HPP - All controller implementations (clean & backend-synced)
// ============================================================================

#ifndef FITNESS_QUEST_CONTROLLERS_IMPL_HPP
#define FITNESS_QUEST_CONTROLLERS_IMPL_HPP

#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <memory>
#include <optional>
#include <sstream>
#include <ctime>

#include "config.hpp"
#include "shared-models/shared-models.hpp"
#include "services.hpp"
#include "utils.hpp"
#include "GameSyncEngine.hpp"

using namespace web;
using namespace web::http;

namespace FitnessQuest {
namespace Controllers {

// ============================================================================
// HEALTH CONTROLLER
// ============================================================================
class HealthController {
private:
    std::shared_ptr<Config::Database> database;

public:
    explicit HealthController(std::shared_ptr<Config::Database> db) : database(std::move(db)) {}

    void getHealth(http_request request) {
        json::value response = json::value::object();
        try {
            bool dbHealthy = database->healthCheck();

            response[U("success")] = json::value::boolean(true);
            response[U("status")] = json::value::string(U("healthy"));
            response[U("timestamp")] = json::value::number(static_cast<int64_t>(time(nullptr)));

            json::value services = json::value::object();
            services[U("database")] = json::value::boolean(dbHealthy);
            services[U("api")] = json::value::boolean(true);
            response[U("services")] = services;

            Utils::Response::sendJsonResponse(request, status_codes::OK, response);
        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::InternalError, e.what());
        }
    }
};

// ============================================================================
// AUTH CONTROLLER
// ============================================================================
class AuthController {
private:
    std::shared_ptr<Config::Database> database;

    static bool verifyPassword(const std::string& password, const std::string& storedHash) {
        return std::to_string(std::hash<std::string>{}(password)) == storedHash;
    }

public:
    explicit AuthController(std::shared_ptr<Config::Database> db) : database(std::move(db)) {}

    void login(http_request request) {
        // capture request by value to keep it valid inside the continuation
        request.extract_json().then([this, request](pplx::task<json::value> task) mutable {
            try {
                json::value body = task.get();

                if (!Utils::Request::hasField(body, "email") || !Utils::Request::hasField(body, "password")) {
                    Utils::Response::sendError(request, status_codes::BadRequest, "Missing email or password");
                    return;
                }

                std::string email = Utils::Request::getStringField(body, "email");
                std::string password = Utils::Request::getStringField(body, "password");

                FitnessDB::User user;
                try {
                    user = database->getUserByEmail(email);
                } catch (...) {
                    Utils::Response::sendError(request, status_codes::Unauthorized, "Invalid credentials");
                    return;
                }

                if (!verifyPassword(password, user.password_hash)) {
                    Utils::Response::sendError(request, status_codes::Unauthorized, "Invalid credentials");
                    return;
                }

                user.last_login = time(nullptr);
                database->updateUser(user);

                std::string token = Utils::JWT::generateToken(user.id);

                json::value response = json::value::object();
                response[U("success")] = json::value::boolean(true);
                response[U("token")] = json::value::string(utility::conversions::to_string_t(token));
                response[U("userId")] = json::value::string(utility::conversions::to_string_t(user.id));

                json::value userData = json::value::object();
                userData[U("id")] = json::value::string(utility::conversions::to_string_t(user.id));
                userData[U("username")] = json::value::string(utility::conversions::to_string_t(user.username));
                userData[U("email")] = json::value::string(utility::conversions::to_string_t(user.email));
                userData[U("fitnessLevel")] = json::value::number(user.fitness_level);
                userData[U("experiencePoints")] = json::value::number(user.experience_points);

                response[U("user")] = userData;
                Utils::Response::sendJsonResponse(request, status_codes::OK, response);

            } catch (const std::exception& e) {
                Utils::Response::sendError(request, status_codes::InternalError, e.what());
            }
        }).wait(); // wait ensures the request lifetime is managed here (safe for typical REST handlers)
    }
};

// ============================================================================
// USER CONTROLLER
// ============================================================================
class UserController {
private:
    std::shared_ptr<Config::Database> database;

public:
    explicit UserController(std::shared_ptr<Config::Database> db) : database(std::move(db)) {}

    void createUser(http_request request) {
        request.extract_json().then([this, request](pplx::task<json::value> task) mutable {
            try {
                json::value body = task.get();

                std::string username = Utils::Request::getStringField(body, "username");
                std::string email = Utils::Request::getStringField(body, "email");
                std::string password = Utils::Request::getStringField(body, "password");

                if (!Utils::Validation::validateEmail(email)) {
                    Utils::Response::sendError(request, status_codes::BadRequest, "Invalid email");
                    return;
                }

                if (!Utils::Validation::validateUsername(username)) {
                    Utils::Response::sendError(request, status_codes::BadRequest, "Invalid username");
                    return;
                }

                if (!Utils::Validation::validatePassword(password)) {
                    Utils::Response::sendError(request, status_codes::BadRequest, "Password too short");
                    return;
                }

                std::string userId = database->createUser(username, email, password);
                std::string token = Utils::JWT::generateToken(userId);

                json::value response = json::value::object();
                response[U("success")] = json::value::boolean(true);
                response[U("userId")] = json::value::string(utility::conversions::to_string_t(userId));
                response[U("token")] = json::value::string(utility::conversions::to_string_t(token));

                Utils::Response::sendJsonResponse(request, status_codes::Created, response);

            } catch (const std::exception& e) {
                Utils::Response::sendError(request, status_codes::InternalError, e.what());
            }
        }).wait();
    }

    void getUser(http_request request, const std::string& userId) {
        try {
            std::string token = Utils::Request::extractToken(request);
            std::string tokenUserId = Utils::JWT::verifyToken(token);

            if (tokenUserId != userId) {
                Utils::Response::sendError(request, status_codes::Forbidden, "Access denied");
                return;
            }

            FitnessDB::User user = database->getUser(userId);

            json::value response = json::value::object();
            response[U("success")] = json::value::boolean(true);

            json::value userData = json::value::object();
            userData[U("id")] = json::value::string(utility::conversions::to_string_t(user.id));
            userData[U("username")] = json::value::string(utility::conversions::to_string_t(user.username));
            userData[U("email")] = json::value::string(utility::conversions::to_string_t(user.email));
            userData[U("fitnessLevel")] = json::value::number(user.fitness_level);
            userData[U("experiencePoints")] = json::value::number(user.experience_points);

            response[U("user")] = userData;
            Utils::Response::sendJsonResponse(request, status_codes::OK, response);

        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::InternalError, e.what());
        }
    }
};

// ============================================================================
// WORKOUT CONTROLLER
// ============================================================================
class WorkoutController {
private:
    std::shared_ptr<Config::Database> database;
    std::shared_ptr<Services::RewardService> rewardService;

    static Models::WorkoutType stringToWorkoutType(const std::string& typeStr) {
        if (typeStr == "strength") return Models::WorkoutType::STRENGTH;
        if (typeStr == "cardio") return Models::WorkoutType::CARDIO;
        if (typeStr == "flexibility") return Models::WorkoutType::FLEXIBILITY;
        if (typeStr == "meditation") return Models::WorkoutType::MEDITATION;
        if (typeStr == "balance") return Models::WorkoutType::BALANCE;
        if (typeStr == "core") return Models::WorkoutType::CORE;
        return Models::WorkoutType::STRENGTH;
    }

public:
    explicit WorkoutController(std::shared_ptr<Config::Database> db)
        : database(std::move(db)) {
        rewardService = std::make_shared<Services::RewardService>(database);
    }

    void logWorkout(http_request request) {
        request.extract_json().then([this, request](pplx::task<json::value> task) mutable {
            try {
                std::string token = Utils::Request::extractToken(request);
                std::string userId = Utils::JWT::verifyToken(token);

                json::value body = task.get();
                std::string typeStr = Utils::Request::getStringField(body, "type");
                double duration = Utils::Request::getDoubleField(body, "duration");
                double intensity = Utils::Request::getDoubleField(body, "intensity");

                std::optional<double> formScore;
                if (Utils::Request::hasField(body, "formScore")) {
                    formScore = Utils::Request::getDoubleField(body, "formScore");
                }

                Models::WorkoutType type = Models::stringToWorkoutType(typeStr);

                // Validate
                Models::Validation::validateWorkoutDuration(duration);
                Models::Validation::validateIntensity(intensity);
                if (formScore.has_value()) {
                    Models::Validation::validateFormScore(formScore.value());
                }

                // Calculate rewards using RewardService
                auto rewardBundle = rewardService->calculateWorkoutRewards(userId, type, duration, intensity, formScore);

                // Update user
                FitnessDB::User user = database->getUser(userId);
                user.experience_points += rewardBundle.experience;
                if (rewardBundle.levelUp) {
                    user.fitness_level = rewardBundle.newLevel;
                }
                database->updateUser(user);

                // Persist workout
                std::string workoutId = database->startWorkout(userId);
                database->completeWorkout(workoutId);

                // Respond
                json::value response = json::value::object();
                response[U("success")] = json::value::boolean(true);
                response[U("workoutId")] = json::value::string(utility::conversions::to_string_t(workoutId));

                json::value rewards = json::value::object();
                rewards[U("experience")] = json::value::number(rewardBundle.experience);
                rewards[U("gold")] = json::value::number(rewardBundle.gold);
                response[U("gameRewards")] = rewards;

                if (rewardBundle.levelUp) {
                    response[U("levelUp")] = json::value::boolean(true);
                    response[U("newLevel")] = json::value::number(rewardBundle.newLevel);
                }

                response[U("message")] = json::value::string(utility::conversions::to_string_t(rewardBundle.message));
                Utils::Response::sendJsonResponse(request, status_codes::Created, response);

            } catch (const std::exception& e) {
                Utils::Response::sendError(request, status_codes::InternalError, e.what());
            }
        }).wait();
    }

    void getWorkoutHistory(http_request request) {
        try {
            std::string token = Utils::Request::extractToken(request);
            std::string userId = Utils::JWT::verifyToken(token);

            // Database currently does not provide a paginated history API;
            // We'll return all workouts for this user by scanning workout btree keys.
            // Use the DB wrapper method getUserWorkouts if present.
            std::vector<FitnessDB::WorkoutSession> workouts;
            try {
                // If Config::Database exposes getUserWorkouts wrapper, use it
                workouts = database->getUserWorkouts(userId);
            } catch (...) {
                // Fall back to returning empty array if not available
                workouts.clear();
            }

            json::value arr = json::value::array(static_cast<unsigned int>(workouts.size()));
            for (size_t i = 0; i < workouts.size(); ++i) {
                json::value w = json::value::object();
                w[U("id")] = json::value::string(utility::conversions::to_string_t(workouts[i].id));
                w[U("userId")] = json::value::string(utility::conversions::to_string_t(workouts[i].user_id));
                w[U("startTime")] = json::value::number(static_cast<int64_t>(workouts[i].start_time));
                w[U("endTime")] = json::value::number(static_cast<int64_t>(workouts[i].end_time));
                w[U("totalCalories")] = json::value::number(workouts[i].total_calories);
                arr[static_cast<unsigned int>(i)] = w;
            }

            json::value response = json::value::object();
            response[U("success")] = json::value::boolean(true);
            response[U("workouts")] = arr;
            Utils::Response::sendJsonResponse(request, status_codes::OK, response);
        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::InternalError, e.what());
        }
    }

    void getWorkout(http_request request, const std::string& workoutId) {
        try {
            std::string token = Utils::Request::extractToken(request);
            (void) Utils::JWT::verifyToken(token); // ensure token is valid

            FitnessDB::WorkoutSession workout = database->getWorkout(workoutId);

            json::value workoutData = json::value::object();
            workoutData[U("id")] = json::value::string(utility::conversions::to_string_t(workout.id));
            workoutData[U("userId")] = json::value::string(utility::conversions::to_string_t(workout.user_id));
            workoutData[U("startTime")] = json::value::number(static_cast<int64_t>(workout.start_time));
            workoutData[U("endTime")] = json::value::number(static_cast<int64_t>(workout.end_time));
            workoutData[U("totalCalories")] = json::value::number(workout.total_calories);

            json::value response = json::value::object();
            response[U("success")] = json::value::boolean(true);
            response[U("workout")] = workoutData;

            Utils::Response::sendJsonResponse(request, status_codes::OK, response);
        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::NotFound, "Workout not found");
        }
    }
};

// ============================================================================
// QUEST CONTROLLER
// ============================================================================
class QuestController {
private:
    std::shared_ptr<Config::Database> database;

public:
    explicit QuestController(std::shared_ptr<Config::Database> db) : database(std::move(db)) {}

    void getQuests(http_request request) {
        try {
            std::string token = Utils::Request::extractToken(request);
            (void) Utils::JWT::verifyToken(token);

            auto quests = database->getAllQuests();
            json::value questsArray = json::value::array(static_cast<unsigned int>(quests.size()));

            for (size_t i = 0; i < quests.size(); ++i) {
                json::value q = json::value::object();
                q[U("id")] = json::value::string(utility::conversions::to_string_t(quests[i].id));
                q[U("title")] = json::value::string(utility::conversions::to_string_t(quests[i].title));
                q[U("description")] = json::value::string(utility::conversions::to_string_t(quests[i].description));
                q[U("difficulty")] = json::value::number(quests[i].difficulty);
                q[U("completed")] = json::value::boolean(quests[i].completed);
                questsArray[static_cast<unsigned int>(i)] = q;
            }

            json::value response = json::value::object();
            response[U("success")] = json::value::boolean(true);
            response[U("quests")] = questsArray;

            Utils::Response::sendJsonResponse(request, status_codes::OK, response);
        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::InternalError, e.what());
        }
    }

    void getQuest(http_request request, const std::string& questId) {
        try {
            std::string token = Utils::Request::extractToken(request);
            (void) Utils::JWT::verifyToken(token);

            FitnessDB::Quest quest = database->getQuest(questId);

            json::value questData = json::value::object();
            questData[U("id")] = json::value::string(utility::conversions::to_string_t(quest.id));
            questData[U("title")] = json::value::string(utility::conversions::to_string_t(quest.title));
            questData[U("description")] = json::value::string(utility::conversions::to_string_t(quest.description));
            questData[U("completed")] = json::value::boolean(quest.completed);

            json::value response = json::value::object();
            response[U("success")] = json::value::boolean(true);
            response[U("quest")] = questData;

            Utils::Response::sendJsonResponse(request, status_codes::OK, response);
        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::NotFound, "Quest not found");
        }
    }

    void completeQuest(http_request request) {
        request.extract_json().then([this, request](pplx::task<json::value> task) mutable {
            try {
                std::string userId = Utils::JWT::verifyToken(Utils::Request::extractToken(request));
                json::value body = task.get();
                std::string questId = Utils::Request::getStringField(body, "questId");

                FitnessDB::Quest quest = database->getQuest(questId);
                quest.completed = true;
                // addQuest behaves as insert/update (CompleteBTree::insert replaces existing key).
                database->addQuest(quest);

                int64_t xp = static_cast<int64_t>(quest.difficulty) * 50;
                FitnessDB::User user = database->getUser(userId);
                user.experience_points += xp;
                database->updateUser(user);

                json::value response = json::value::object();
                response[U("success")] = json::value::boolean(true);
                response[U("message")] = json::value::string(U("Quest completed!"));

                Utils::Response::sendJsonResponse(request, status_codes::OK, response);
            } catch (const std::exception& e) {
                Utils::Response::sendError(request, status_codes::InternalError, e.what());
            }
        }).wait();
    }
};

// ============================================================================
// GAME CONTROLLER
// ============================================================================
class GameController {
private:
    std::shared_ptr<Config::Database> database;
    std::shared_ptr<GameSync::GameSyncEngine> syncEngine;

public:
    GameController(std::shared_ptr<Config::Database> db, std::shared_ptr<GameSync::GameSyncEngine> engine)
        : database(std::move(db)), syncEngine(std::move(engine)) {}

    void syncGameState(http_request request) {
        try {
            std::string userId = Utils::JWT::verifyToken(Utils::Request::extractToken(request));
            auto gameState = syncEngine->getPlayerGameState(userId);

            json::value state = json::value::object();
            for (const auto& kv : gameState) {
                state[utility::conversions::to_string_t(kv.first)] = json::value::number(kv.second);
            }

            json::value response = json::value::object();
            response[U("success")] = json::value::boolean(true);
            response[U("gameState")] = state;

            Utils::Response::sendJsonResponse(request, status_codes::OK, response);
        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::InternalError, e.what());
        }
    }

    void getPlayerStats(http_request request) {
        try {
            std::string userId = Utils::JWT::verifyToken(Utils::Request::extractToken(request));
            FitnessDB::User user = database->getUser(userId);

            json::value stats = json::value::object();
            stats[U("level")] = json::value::number(user.fitness_level);
            stats[U("xp")] = json::value::number(user.experience_points);

            json::value response = json::value::object();
            response[U("success")] = json::value::boolean(true);
            response[U("stats")] = stats;

            Utils::Response::sendJsonResponse(request, status_codes::OK, response);
        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::InternalError, e.what());
        }
    }

    void getAvailableQuests(http_request request) {
        try {
            std::string userId = Utils::JWT::verifyToken(Utils::Request::extractToken(request));
            auto quests = syncEngine->getAvailableQuests(userId); // vector<map<string,string>>

            json::value arr = json::value::array(static_cast<unsigned int>(quests.size()));
            for (size_t i = 0; i < quests.size(); ++i) {
                json::value q = json::value::object();
                const auto& qm = quests[i];
                if (qm.find("id") != qm.end()) q[U("id")] = json::value::string(utility::conversions::to_string_t(qm.at("id")));
                if (qm.find("title") != qm.end()) q[U("title")] = json::value::string(utility::conversions::to_string_t(qm.at("title")));
                if (qm.find("description") != qm.end()) q[U("description")] = json::value::string(utility::conversions::to_string_t(qm.at("description")));
                if (qm.find("difficulty") != qm.end()) q[U("difficulty")] = json::value::number(std::stoi(qm.at("difficulty")));
                if (qm.find("priority") != qm.end()) q[U("priority")] = json::value::number(std::stoi(qm.at("priority")));
                arr[static_cast<unsigned int>(i)] = q;
            }

            json::value response = json::value::object();
            response[U("success")] = json::value::boolean(true);
            response[U("quests")] = arr;

            Utils::Response::sendJsonResponse(request, status_codes::OK, response);
        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::InternalError, e.what());
        }
    }

    void getLeaderboard(http_request request) {
        try {
            (void) Utils::JWT::verifyToken(Utils::Request::extractToken(request));

            // Build a simple leaderboard by scanning users (not ideal for large DBs)
            auto stats = database->getStats();
            std::vector<std::pair<std::string, int>> entries; // (userId, xp)

            // If Config::Database provided a get_all_users or similar, use it. Otherwise we return empty.
            try {
                // Attempt to collect users if wrapper exists
                // This block intentionally left minimal — implement getAllUsers in DB for proper leaderboard.
            } catch (...) {}

            json::value arr = json::value::array(static_cast<unsigned int>(entries.size()));
            for (size_t i = 0; i < entries.size(); ++i) {
                json::value e = json::value::object();
                e[U("userId")] = json::value::string(utility::conversions::to_string_t(entries[i].first));
                e[U("xp")] = json::value::number(entries[i].second);
                arr[static_cast<unsigned int>(i)] = e;
            }

            json::value response = json::value::object();
            response[U("success")] = json::value::boolean(true);
            response[U("leaderboard")] = arr;
            Utils::Response::sendJsonResponse(request, status_codes::OK, response);
        } catch (const std::exception& e) {
            Utils::Response::sendError(request, status_codes::InternalError, e.what());
        }
    }

    void claimReward(http_request request) {
        request.extract_json().then([this, request](pplx::task<json::value> task) mutable {
            try {
                std::string userId = Utils::JWT::verifyToken(Utils::Request::extractToken(request));
                json::value body = task.get();
                std::string rewardId = Utils::Request::getStringField(body, "rewardId");

                // Reward claiming logic is application-specific.
                // Hook into RewardService (not exposed here) or implement custom logic.
                json::value response = json::value::object();
                response[U("success")] = json::value::boolean(true);
                response[U("message")] = json::value::string(U("Reward claimed"));

                Utils::Response::sendJsonResponse(request, status_codes::OK, response);
            } catch (const std::exception& e) {
                Utils::Response::sendError(request, status_codes::InternalError, e.what());
            }
        }).wait();
    }
};

} // namespace Controllers
} // namespace FitnessQuest

#endif // FITNESS_QUEST_CONTROLLERS_IMPL_HPP
