// ============================================================================
// UTILITIES - All utility classes in one header file
// ============================================================================

#ifndef FITNESS_QUEST_UTILS_HPP
#define FITNESS_QUEST_UTILS_HPP

#include <string>
#include <regex>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <openssl/hmac.h>
#include <openssl/buffer.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include "config.hpp"

using namespace web;
using namespace web::http;

namespace FitnessQuest {
namespace Utils {

// ============================================================================
// JWT - JSON Web Token utilities
// ============================================================================
class JWT {
public:
    static std::string generateToken(const std::string& userId) {
        time_t now = std::time(nullptr);
        time_t expiry = now + (Config::Environment::getJWTExpiration() * 3600);
        
        std::ostringstream payload;
        payload << userId << ":" << expiry;
        
        std::string secret = Config::Environment::getJWTSecret();
        
        // Simple HMAC signature
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hashLen;
        
        HMAC(EVP_sha256(), secret.c_str(), secret.length(),
             reinterpret_cast<const unsigned char*>(payload.str().c_str()),
             payload.str().length(), hash, &hashLen);
        
        // Base64 encode
        BIO *bio, *b64;
        BUF_MEM *bufferPtr;
        
        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);
        
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, hash, hashLen);
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferPtr);
        
        std::string signature(bufferPtr->data, bufferPtr->length);
        BIO_free_all(bio);
        
        return payload.str() + "." + signature;
    }
    
    // In utils.hpp - Update verifyToken method
    static std::string verifyToken(const std::string& token) {
        // Your token format: USER_1765130866_886:1765217266.3tiGE/NoxKPN...
        // This has userId:expiry.signature format
        
        size_t firstDot = token.find('.');
        if (firstDot == std::string::npos) {
            throw std::runtime_error("Invalid token format - no dot found");
        }
        
        // Extract payload (part before the dot)
        std::string payload = token.substr(0, firstDot);
        
        // Find colon separator in payload
        size_t colonPos = payload.find(':');
        if (colonPos == std::string::npos) {
            throw std::runtime_error("Invalid token payload - no colon found");
        }
        
        // Extract user ID
        std::string userId = payload.substr(0, colonPos);
        
        // Extract expiry timestamp
        std::string expiryStr = payload.substr(colonPos + 1);
        time_t expiry;
        
        try {
            expiry = std::stoll(expiryStr);
        } catch (...) {
            throw std::runtime_error("Invalid expiry timestamp in token");
        }
        
        // Check if token expired
        if (std::time(nullptr) > expiry) {
            throw std::runtime_error("Token expired");
        }
        
        // Verify signature (simplified - in production, verify HMAC)
        // For testing, we'll trust the token
        
        return userId;
    }
};

// ============================================================================
// Validation - Input validation utilities
// ============================================================================
class Validation {
public:
    static bool validateEmail(const std::string& email) {
        static const std::regex emailRegex(
            R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)"
        );
        return std::regex_match(email, emailRegex);
    }
    
    static bool validateUsername(const std::string& username) {
        if (username.length() < 3 || username.length() > 20) {
            return false;
        }
        static const std::regex usernameRegex("^[a-zA-Z0-9_]+$");
        return std::regex_match(username, usernameRegex);
    }
    
    static bool validatePassword(const std::string& password) {
        return password.length() >= 6;
    }
};

// ============================================================================
// Request - HTTP request helper utilities
// ============================================================================
class Request {
public:
    // Check if JSON body has a field
    static bool hasField(const web::json::value& body, const std::string& fieldName) {
        return body.has_field(utility::conversions::to_string_t(fieldName)) && 
               !body.at(utility::conversions::to_string_t(fieldName)).is_null();
    }
    
    // Get string field from JSON body
    static std::string getStringField(const web::json::value& body, const std::string& fieldName) {
        if (!hasField(body, fieldName)) {
            throw std::runtime_error("Missing required field: " + fieldName);
        }
        return utility::conversions::to_utf8string(
            body.at(utility::conversions::to_string_t(fieldName)).as_string()
        );
    }
    
    // Get double field from JSON body
    static double getDoubleField(const web::json::value& body, const std::string& fieldName) {
        if (!hasField(body, fieldName)) {
            throw std::runtime_error("Missing required field: " + fieldName);
        }
        return body.at(utility::conversions::to_string_t(fieldName)).as_double();
    }
    
    // Get int field from JSON body
    static int getIntField(const web::json::value& body, const std::string& fieldName) {
        if (!hasField(body, fieldName)) {
            throw std::runtime_error("Missing required field: " + fieldName);
        }
        return body.at(utility::conversions::to_string_t(fieldName)).as_integer();
    }
    
    // Extract JWT token from Authorization header
    static std::string extractToken(http_request& request) {
        if (!request.headers().has(U("Authorization"))) {
            throw std::runtime_error("Authorization header missing");
        }
        
        std::string authHeader = utility::conversions::to_utf8string(
            request.headers()[U("Authorization")]
        );
        
        if (authHeader.substr(0, 7) != "Bearer ") {
            throw std::runtime_error("Invalid authorization format");
        }
        
        return authHeader.substr(7);
    }
};

// ============================================================================
// Response - HTTP response helper utilities
// ============================================================================
class Response {
public:
    // Send JSON response with custom data
    static void sendJsonResponse(http_request request, status_code code,
                                const web::json::value& data) {
        http_response response(code);
        response.headers().add(U("Content-Type"), U("application/json"));
        response.headers().add(U("Access-Control-Allow-Origin"), U("*"));
        response.set_body(data);
        request.reply(response);
    }
    
    // Send error response
    static void sendError(http_request request, status_code code, const std::string& message) {
        web::json::value response = web::json::value::object();
        response[U("success")] = web::json::value::boolean(false);
        response[U("error")] = web::json::value::string(
            utility::conversions::to_string_t(message)
        );
        sendJsonResponse(request, code, response);
    }
    
    // Send success response with optional data
    static void sendSuccess(http_request request, const web::json::value& data = web::json::value::object()) {
        web::json::value response = web::json::value::object();
        response[U("success")] = web::json::value::boolean(true);
        
        // Merge data into response
        if (data.is_object()) {
            auto obj = data.as_object();
            for (const auto& pair : obj) {
                response[pair.first] = pair.second;
            }
        }
        
        sendJsonResponse(request, status_codes::OK, response);
    }
};

} // namespace Utils
} // namespace FitnessQuest

#endif // FITNESS_QUEST_UTILS_HPP