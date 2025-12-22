// ============================================================================
// MIDDLEWARE - All middleware classes in one header file
// ============================================================================

#ifndef FITNESS_QUEST_MIDDLEWARE_HPP
#define FITNESS_QUEST_MIDDLEWARE_HPP

#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <iostream>
#include <ctime>
#include <exception>
#include <memory>
#include <mutex>
#include <map>
#include <string>
#include "config.hpp"

using namespace web;
using namespace web::http;

namespace FitnessQuest {
namespace Middleware {

// ============================================================================
// Logger
// ============================================================================
class Logger {
private:
    static std::mutex logMutex;
    
public:
    static void logRequest(const http_request& request) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        time_t now = time(nullptr);
        char timeStr[26];
        ctime_r(&now, timeStr);
        timeStr[24] = '\0';
        
        std::cout << "[REQUEST][" << timeStr << "] "
                  << request.method() << " "
                  << utility::conversions::to_utf8string(request.relative_uri().path())
                  << std::endl;
    }
    
    static void logResponse(const http_request& request, status_code status) {
        std::lock_guard<std::mutex> lock(logMutex);
        
        time_t now = time(nullptr);
        char timeStr[26];
        ctime_r(&now, timeStr);
        timeStr[24] = '\0';
        
        std::cout << "[RESPONSE][" << timeStr << "] "
                  << request.method() << " "
                  << utility::conversions::to_utf8string(request.relative_uri().path())
                  << " -> " << status << std::endl;
    }
    
    static void logInfo(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        logWithTime("[INFO]", message);
    }
    
    static void logWarning(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        logWithTime("[WARNING]", message);
    }
    
    static void logError(const std::string& message) {
        std::lock_guard<std::mutex> lock(logMutex);
        logWithTime("[ERROR]", message);
    }
    
    static void logDebug(const std::string& message) {
        if (Config::Environment::isDebug()) {
            std::lock_guard<std::mutex> lock(logMutex);
            logWithTime("[DEBUG]", message);
        }
    }
    
private:
    static void logWithTime(const std::string& level, const std::string& message) {
        time_t now = time(nullptr);
        char timeStr[26];
        ctime_r(&now, timeStr);
        timeStr[24] = '\0';
        
        std::cout << level << "[" << timeStr << "] " << message << std::endl;
    }
};

std::mutex Logger::logMutex;

// ============================================================================
// Error Handler
// ============================================================================
class ErrorHandler {
public:
    static void handleError(http_request request, const std::exception& e) {
        Logger::logError(e.what());
        
        json::value response = json::value::object();
        response[U("success")] = json::value::boolean(false);
        response[U("error")] = json::value::string(
            utility::conversions::to_string_t(e.what())
        );
        
        sendResponse(request, status_codes::InternalError, response);
    }
    
    static void sendJsonError(http_request request, status_code code, 
                            const std::string& message) {
        Logger::logError("HTTP " + std::to_string(code) + ": " + message);
        
        json::value response = json::value::object();
        response[U("success")] = json::value::boolean(false);
        response[U("error")] = json::value::string(
            utility::conversions::to_string_t(message)
        );
        
        sendResponse(request, code, response);
    }
    
    static void sendResponse(http_request request, status_code code, 
                            const json::value& body) {
        http_response http_resp(code);
        http_resp.headers().add(U("Content-Type"), U("application/json"));
        http_resp.headers().add(U("Access-Control-Allow-Origin"), U("*"));
        http_resp.headers().add(U("Access-Control-Allow-Methods"), 
            U("GET, POST, PUT, DELETE, PATCH, OPTIONS"));
        http_resp.headers().add(U("Access-Control-Allow-Headers"), 
            U("Content-Type, Authorization"));
        http_resp.set_body(body);
        
        Logger::logResponse(request, code);
        request.reply(http_resp);
    }
};

// ============================================================================
// Rate Limiter
// ============================================================================
class RateLimiter {
private:
    struct RateLimit {
        time_t lastRequest;
        int requestCount;
    };
    
    std::map<std::string, RateLimit> rateLimits;
    std::mutex rateLimitMutex;
    int maxRequests;
    int timeWindow; // in seconds
    
public:
    RateLimiter(int maxRequests = 100, int timeWindow = 60) 
        : maxRequests(maxRequests), timeWindow(timeWindow) {}
    
    bool checkLimit(const std::string& clientId) {
        std::lock_guard<std::mutex> lock(rateLimitMutex);
        
        time_t now = time(nullptr);
        auto it = rateLimits.find(clientId);
        
        if (it == rateLimits.end()) {
            rateLimits[clientId] = {now, 1};
            return true;
        }
        
        // Reset if window has passed
        if (now - it->second.lastRequest > timeWindow) {
            it->second = {now, 1};
            return true;
        }
        
        // Check if over limit
        if (it->second.requestCount >= maxRequests) {
            return false;
        }
        
        it->second.requestCount++;
        return true;
    }
    
    void cleanupExpired() {
        std::lock_guard<std::mutex> lock(rateLimitMutex);
        
        time_t now = time(nullptr);
        for (auto it = rateLimits.begin(); it != rateLimits.end();) {
            if (now - it->second.lastRequest > timeWindow) {
                it = rateLimits.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    std::string getClientId(const http_request& request) {
        auto headers = request.headers();
        if (headers.has("X-Forwarded-For")) {
            return utility::conversions::to_utf8string(headers["X-Forwarded-For"]);
        }
        if (headers.has("X-Real-IP")) {
            return utility::conversions::to_utf8string(headers["X-Real-IP"]);
        }
        return "unknown";
    }
};

// ============================================================================
// Authentication Middleware
// ============================================================================
class Authentication {
public:
    static std::string extractToken(const http_request& request) {
        auto headers = request.headers();
        if (!headers.has(U("Authorization"))) {
            throw std::runtime_error("Authorization header missing");
        }
        
        std::string authHeader = utility::conversions::to_utf8string(
            headers[U("Authorization")]
        );
        
        if (authHeader.substr(0, 7) != "Bearer ") {
            throw std::runtime_error("Invalid authorization format");
        }
        
        return authHeader.substr(7);
    }
    
    static bool isAuthenticated(const http_request& request) {
        try {
            std::string token = extractToken(request);
            // Validate token (this would call JWT verify)
            return true;
        } catch (...) {
            return false;
        }
    }
};

// ============================================================================
// CORS Middleware
// ============================================================================
class CORS {
public:
    static void addCORSHeaders(http_response& response) {
        response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
        response.headers().add(U("Access-Control-Allow-Methods"), 
            U("GET, POST, PUT, DELETE, PATCH, OPTIONS"));
        response.headers().add(U("Access-Control-Allow-Headers"), 
            U("Content-Type, Authorization, X-Requested-With"));
        response.headers().add(U("Access-Control-Allow-Credentials"), U("true"));
        response.headers().add(U("Access-Control-Max-Age"), U("3600"));
    }
    
    static void handlePreflight(http_request request) {
        http_response response(status_codes::OK);
        addCORSHeaders(response);
        request.reply(response);
    }
};

} // namespace Middleware
} // namespace FitnessQuest

#endif // FITNESS_QUEST_MIDDLEWARE_HPP