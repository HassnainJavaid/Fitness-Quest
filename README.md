# 🎮 Fitness Quest Backend - Complete C++ REST API Server

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![C++ Version](https://img.shields.io/badge/C++-17-blue)]()
[![License](https://img.shields.io/badge/license-MIT-green)]()

A complete, production-ready RESTful API backend for a fitness gamification platform. Users log workouts, earn XP/gold, level up, complete quests, and progress their game character based on real fitness activities.

---

## 📋 Table of Contents

- [Features](#-features)
- [Architecture](#-architecture)
- [Project Structure](#-project-structure)
- [Prerequisites](#-prerequisites)
- [Quick Start](#-quick-start)
- [API Documentation](#-api-documentation)
- [Game Mechanics](#-game-mechanics)
- [Configuration](#-configuration)
- [Testing](#-testing)
- [Deployment](#-deployment)
- [Troubleshooting](#-troubleshooting)

---

## ✨ Features

### Core Features
- ✅ **User Management** - Registration, login, profiles
- ✅ **JWT Authentication** - Secure token-based auth
- ✅ **Workout Logging** - Track exercises with duration, intensity, form score
- ✅ **Game Progression** - XP, levels, gold, character stats
- ✅ **Quest System** - Daily/weekly challenges with rewards
- ✅ **Real-time Rewards** - Instant XP/gold calculation
- ✅ **Anti-Cheat** - Validation limits on workouts
- ✅ **Persistence** - Custom B-Tree file-based database

### Technical Features
- ✅ **RESTful API Design** - Standard HTTP methods
- ✅ **CORS Support** - Cross-origin requests
- ✅ **Error Handling** - Comprehensive error responses
- ✅ **Request Logging** - All requests logged
- ✅ **Thread Safety** - Mutex-protected database operations
- ✅ **Scalable Architecture** - Ready for cloud deployment

---

## 🏗️ Architecture

### System Design

```
┌─────────────────────────────────────────┐
│           Client Applications            │
│  (Web, Mobile, Desktop, Game Engines)   │
└───────────────┬─────────────────────────┘
                │ HTTP/HTTPS
                │
┌───────────────▼─────────────────────────┐
│          REST API Server (C++)           │
│                                          │
│  ┌────────────────────────────────────┐ │
│  │         HTTP Listener              │ │
│  │       (cpprestsdk/http)            │ │
│  └────────────┬───────────────────────┘ │
│               │                          │
│  ┌────────────▼───────────────────────┐ │
│  │          Router                    │ │
│  │    (URL Pattern Matching)          │ │
│  └────────────┬───────────────────────┘ │
│               │                          │
│  ┌────────────▼───────────────────────┐ │
│  │        Controllers                 │ │
│  │  • HealthController                │ │
│  │  • UserController                  │ │
│  │  • AuthController                  │ │
│  │  • WorkoutController               │ │
│  │  • QuestController                 │ │
│  └────────────┬───────────────────────┘ │
│               │                          │
│  ┌────────────▼───────────────────────┐ │
│  │         Services                   │ │
│  │  • GameService (stats, levels)     │ │
│  │  • RewardService (XP, gold)        │ │
│  └────────────┬───────────────────────┘ │
│               │                          │
│  ┌────────────▼───────────────────────┐ │
│  │      Database Wrapper              │ │
│  │   (Thread-safe operations)         │ │
│  └────────────┬───────────────────────┘ │
└───────────────┼─────────────────────────┘
                │
┌───────────────▼─────────────────────────┐
│     Custom B-Tree Database              │
│  ┌─────────────────────────────────┐   │
│  │  Users   │ Workouts │ Quests    │   │
│  │          │          │           │   │
│  │  B-Tree  │  B-Tree  │  B-Tree   │   │
│  └─────────────────────────────────┘   │
│                                          │
│     File-based Persistence               │
│  (./fitness_data/*.dat)                  │
└──────────────────────────────────────────┘
```

### Request Flow

1. **Client** sends HTTP request
2. **HTTP Listener** receives request
3. **Router** matches URL pattern
4. **Controller** handles request logic
5. **Service** executes business logic
6. **Database** persists/retrieves data
7. **Response** sent back to client

---

## 📁 Project Structure

```
fitness-quest-backend/
│
├── 📄 main.cpp                          # Server entry point
├── 📄 CMakeLists.txt                    # Build configuration
├── 📄 .env                              # Environment variables
├── 📄 README.md                         # This file
├── 📄 build.sh                          # Build script
├── 📄 run.sh                            # Run script
├── 📄 test-api.sh                       # API test script
│
├── 📂 config/                           # Configuration
│   ├── Database.hpp                     # Database wrapper
│   └── Environment.hpp                  # Environment loader
│
├── 📂 routes/                           # Routing
│   └── Router.hpp                       # HTTP router
│
├── 📂 controllers/                      # Request handlers
│   ├── HealthController.hpp             # Health check
│   ├── UserController.hpp               # User management
│   ├── AuthController.hpp               # Authentication
│   ├── WorkoutController.hpp            # Workout logging
│   └── QuestController.hpp              # Quest management
│
├── 📂 services/                         # Business logic
│   ├── GameService.hpp                  # Game logic
│   └── RewardService.hpp                # Reward calculation
│
├── 📂 middleware/                       # Middleware
│   ├── Logger.hpp                       # Request logging
│   └── ErrorHandler.hpp                 # Error handling
│
├── 📂 utils/                            # Utilities
│   ├── JWT.hpp                          # Token management
│   └── Validation.hpp                   # Input validation
│
├── 📂 database/                         # Database
│   └── complete_database.h              # Custom B-Tree DB
│
├── 📂 shared-models/                    # Data models
│   └── shared-models.hpp                # TypeScript/C++ models
│
├── 📂 build/                            # Build artifacts (generated)
│   └── bin/
│       └── fitness_quest_server         # Executable
│
└── 📂 fitness_data/                     # Database files (generated)
    ├── users.dat
    ├── workouts.dat
    ├── quests.dat
    └── ...
```

### File Descriptions

#### **Core System**

| File | Purpose | Key Functions |
|------|---------|---------------|
| `main.cpp` | Server entry point | HTTP listener setup, signal handling, main loop |
| `config/Database.hpp` | Database operations | CRUD operations, thread safety, connection management |
| `config/Environment.hpp` | Configuration | Load .env, get settings, manage environment vars |
| `routes/Router.hpp` | URL routing | Pattern matching, route registration, request dispatch |

#### **Controllers** (Handle HTTP Requests)

| File | Endpoints | Purpose |
|------|-----------|---------|
| `HealthController.hpp` | `GET /health` | Server health check, database status |
| `UserController.hpp` | `POST /api/users`, `GET /api/users/:id` | User registration, profile retrieval |
| `AuthController.hpp` | `POST /api/auth/login` | User authentication, token generation |
| `WorkoutController.hpp` | `POST /api/workouts`, `GET /api/workouts` | Log workouts, get history |
| `QuestController.hpp` | `GET /api/quests`, `POST /api/quests/complete` | Quest listing, completion |

#### **Services** (Business Logic)

| File | Purpose | Key Functions |
|------|---------|---------------|
| `GameService.hpp` | Game mechanics | Character initialization, stat calculation, level progression |
| `RewardService.hpp` | Reward system | XP/gold calculation, multipliers, bonuses, achievements |

#### **Utilities & Middleware**

| File | Purpose | Usage |
|------|---------|-------|
| `utils/JWT.hpp` | Token management | Generate JWT, verify JWT, extract user ID |
| `utils/Validation.hpp` | Input validation | Email, username, password validation |
| `middleware/Logger.hpp` | Request logging | Log all HTTP requests with timestamps |
| `middleware/ErrorHandler.hpp` | Error handling | Catch exceptions, format error responses |

#### **Data Layer**

| File | Purpose | Technology |
|------|---------|------------|
| `database/complete_database.h` | Custom database | B-Tree implementation, file persistence |
| `shared-models/shared-models.hpp` | Data models | Structs, enums, validation, serialization |

---

## 🔧 Prerequisites

### Required Software

| Software | Minimum Version | Purpose |
|----------|-----------------|---------|
| **C++ Compiler** | GCC 7+ or Clang 6+ | Compile C++17 code |
| **CMake** | 3.10+ | Build system |
| **cpprestsdk** | 2.10+ | HTTP server library |
| **OpenSSL** | 1.1+ | Cryptography (JWT) |
| **Boost** | 1.65+ | System libraries |

### Installation

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libcpprest-dev \
    libssl-dev \
    libboost-all-dev
```

**macOS:**
```bash
brew install cmake cpprestsdk openssl boost
```

**Verify Installation:**
```bash
g++ --version        # Should show 7.0+
cmake --version      # Should show 3.10+
```

---

## 🚀 Quick Start

### 1. Clone or Setup Project

```bash
# Create project directory
mkdir fitness-quest-backend
cd fitness-quest-backend

# Copy all files into this directory
# Ensure the structure matches the Project Structure above
```

### 2. Configure Environment

```bash
# Create .env file (or use the one provided)
cat > .env << 'EOF'
PORT=8080
DEBUG=true
DATA_DIR=./fitness_data
JWT_SECRET=your-secret-key-change-in-production
JWT_EXPIRATION_HOURS=24
RATE_LIMIT_WINDOW=900
RATE_LIMIT_MAX=100
EOF
```

### 3. Build

```bash
# Make scripts executable
chmod +x build.sh test-api.sh

# Build the server
./build.sh
```

**Expected Output:**
```
========================================
FITNESS QUEST - BUILD SCRIPT
========================================

[1/6] Checking dependencies...
   ✓ g++ found
   ✓ cmake found

[2/6] Checking required files...
   ✓ main.cpp
   ✓ CMakeLists.txt
   ✓ database/complete_database.h
   ✓ shared-models/shared-models.hpp

[3/6] Checking configuration...
   ✓ .env file exists

[4/6] Cleaning previous build...
   ✓ Build directory created

[5/6] Configuring with CMake...
   ✓ CMake configuration successful

[6/6] Compiling...
   Using 4 parallel jobs...
   ✓ Compilation successful

========================================
BUILD SUCCESSFUL!
========================================
```

### 4. Run Server

```bash
./build/bin/fitness_quest_server
```

**Expected Output:**
```
========================================
FITNESS QUEST API SERVER
========================================

✓ Environment variables loaded (7 variables from 7 lines)
Connecting to database...
  Database statistics:
    Users: 1
    Exercises: 2
    Workouts: 0
    Quests: 1
✓ Database connected

✓ Server listening on: http://0.0.0.0:8080

========================================
AVAILABLE ENDPOINTS
========================================

Health & Status:
  GET    /health                    - Health check

User Management:
  POST   /api/users                 - Create new user
  GET    /api/users/:id             - Get user profile

Authentication:
  POST   /api/auth/login            - User login

Workout System:
  POST   /api/workouts              - Log workout
  GET    /api/workouts              - Get workout history
  GET    /api/workouts/:id          - Get specific workout

Quest System:
  GET    /api/quests                - Get all quests
  GET    /api/quests/:id            - Get specific quest
  POST   /api/quests/complete       - Complete quest

========================================
Press CTRL+C to stop the server...
========================================
```

### 5. Test API

Open a new terminal and run:

```bash
# Health check
curl http://localhost:8080/health

# Or run the complete test suite
./test-api.sh
```

---

## 📡 API Documentation

### Base URL
```
http://localhost:8080
```

### Authentication
Most endpoints require a JWT token in the header:
```
Authorization: Bearer YOUR_JWT_TOKEN
```

### Response Format
All responses are JSON:
```json
{
  "success": true|false,
  "data": {...},
  "error": "error message" // only if success=false
}
```

---

### Endpoints

#### **1. Health Check**

**GET** `/health`

Check server and database status.

**Response:**
```json
{
  "status": "ok",
  "database": "connected",
  "timestamp": 1702345678,
  "stats": {
    "users": 5,
    "exercises": 10,
    "workouts": 25,
    "quests": 3
  }
}
```

---

#### **2. Create User**

**POST** `/api/users`

Register a new user.

**Request Body:**
```json
{
  "username": "fitnessking",
  "email": "king@fitness.com",
  "password": "secure123"
}
```

**Response:**
```json
{
  "success": true,
  "userId": "USER_1702345678_1234",
  "gameCharacterId": "USER_1702345678_1234",
  "token": "eyJhbGc...",
  "message": "User created successfully"
}
```

**Validation Rules:**
- Username: 3-20 characters, alphanumeric + underscore
- Email: Valid email format
- Password: Minimum 6 characters

---

#### **3. Login**

**POST** `/api/auth/login`

Authenticate user and get token.

**Request Body:**
```json
{
  "email": "king@fitness.com",
  "password": "secure123"
}
```

**Response:**
```json
{
  "success": true,
  "token": "eyJhbGc...",
  "userId": "USER_1702345678_1234",
  "user": {
    "id": "USER_1702345678_1234",
    "username": "fitnessking",
    "email": "king@fitness.com",
    "fitnessLevel": 5,
    "experiencePoints": 450
  },
  "message": "Login successful"
}
```

---

#### **4. Get User Profile**

**GET** `/api/users/:userId`

Get user profile information.

**Headers:**
```
Authorization: Bearer YOUR_TOKEN
```

**Response:**
```json
{
  "success": true,
  "user": {
    "id": "USER_1702345678_1234",
    "username": "fitnessking",
    "email": "king@fitness.com",
    "fitnessLevel": 5,
    "experiencePoints": 450,
    "createdAt": 1702345678
  }
}
```

---

#### **5. Log Workout**

**POST** `/api/workouts`

Log a workout and receive rewards.

**Headers:**
```
Authorization: Bearer YOUR_TOKEN
```

**Request Body:**
```json
{
  "type": "STRENGTH",
  "exerciseId": "pushup",
  "duration": 30,
  "intensity": 8,
  "formScore": 90,
  "notes": "Great session!"
}
```

**Parameters:**
- `type`: `STRENGTH` | `CARDIO` | `FLEXIBILITY` | `MEDITATION` | `BALANCE` | `CORE`
- `duration`: Minutes (1-240)
- `intensity`: Scale 1-10
- `formScore`: Optional, 0-100
- `notes`: Optional

**Response:**
```json
{
  "success": true,
  "workoutId": "WORKOUT_1702345700_456",
  "gameRewards": {
    "experience": 96,
    "gold": 48
  },
  "levelUp": true,
  "newLevel": 6,
  "message": "Congratulations! You reached level 6!"
}
```

---

#### **6. Get Quests**

**GET** `/api/quests`

Get all available quests.

**Headers:**
```
Authorization: Bearer YOUR_TOKEN
```

**Response:**
```json
{
  "success": true,
  "quests": [
    {
      "id": "Q001",
      "title": "Daily Challenge",
      "description": "Complete basic exercises",
      "priority": 1,
      "difficulty": 1,
      "completed": false
    }
  ],
  "dailyQuests": []
}
```

---

#### **7. Complete Quest**

**POST** `/api/quests/complete`

Mark a quest as completed and claim rewards.

**Headers:**
```
Authorization: Bearer YOUR_TOKEN
```

**Request Body:**
```json
{
  "questId": "Q001"
}
```

**Response:**
```json
{
  "success": true,
  "rewards": {
    "experience": 50,
    "gold": 25
  },
  "message": "Quest completed! Earned 50 XP and 25 gold!",
  "nextQuest": {
    "id": "Q002",
    "title": "Next Challenge"
  }
}
```

---

## 🎮 Game Mechanics

### XP & Leveling

**XP Calculation Formula:**
```
XP = duration × XP_RATE × (intensity / 5) × (1 + formBonus)
```

**XP Rates (per minute):**
- **STRENGTH**: 2 XP/min
- **CARDIO**: 3 XP/min
- **FLEXIBILITY**: 1.5 XP/min
- **MEDITATION**: 2 XP/min
- **BALANCE**: 1.8 XP/min
- **CORE**: 2.2 XP/min

**Form Score Bonus:**
- Form score 0-100 gives 0-20% bonus
- Example: 90/100 form score = +18% XP

**Level Progression:**
```
Formula: XP_required = 100 × 1.5^(level-1)

Level 1 → 2: 100 XP
Level 2 → 3: 150 XP
Level 3 → 4: 225 XP
Level 4 → 5: 337 XP
Level 5 → 6: 506 XP
```

### Example Calculation

**Workout:**
- Type: CARDIO
- Duration: 45 minutes
- Intensity: 8/10
- Form Score: 85/100

**Calculation:**
```
Base XP = 45 × 3 × (8/5) = 216 XP
Form Bonus = 85/100 × 0.2 = 17% bonus
Final XP = 216 × 1.17 = 252 XP

Base Gold = 45 × 1 × (8/5) = 72 gold
Final Gold = 72 × 1.17 = 84 gold
```

### Character Stats

**Core Stats** (from workouts):
- **Strength**: Weight training → Attack power
- **Stamina**: Cardio → Health pool
- **Agility**: Flexibility → Speed, dodge
- **Magic**: Meditation → Mana, special abilities

**Derived Stats** (automatic):
- **Attack Power** = strength × 2 + agility × 0.5
- **Defense** = strength × 0.5 + stamina
- **Magic Power** = magic × 2
- **Speed** = agility × 3
- **Max Health** = stamina × 10
- **Max Mana** = magic × 5

### Reward Multipliers

| Multiplier | Condition | Bonus |
|-----------|-----------|-------|
| High Intensity | intensity ≥ 8 | +20% |
| Long Workout | duration ≥ 60 min | +15% |
| Weekend | Saturday/Sunday | +10% |
| Random Bonus | 5% chance | +50 gold |

---

## ⚙️ Configuration

### Environment Variables (.env)

```bash
# Server Configuration
PORT=8080                    # Server port
DEBUG=true                   # Debug mode (verbose logging)

# Database
DATA_DIR=./fitness_data      # Database storage directory

# JWT Configuration
JWT_SECRET=your-secret-key   # JWT signing secret (CHANGE IN PRODUCTION!)
JWT_EXPIRATION_HOURS=24      # Token expiration time

# Security
RATE_LIMIT_WINDOW=900        # Rate limit window (seconds)
RATE_LIMIT_MAX=100           # Max requests per window
```

### Anti-Cheat Limits

Defined in `shared-models/shared-models.hpp`:

```cpp
MAX_WORKOUTS_PER_DAY = 10
MAX_DURATION_PER_WORKOUT = 240  // 4 hours
MIN_DURATION_PER_WORKOUT = 1    // 1 minute
MAX_INTENSITY = 10
MIN_INTENSITY = 1
MIN_REST_BETWEEN_WORKOUTS = 1800  // 30 minutes
```

---

## 🧪 Testing

### Automated Test Suite

```bash
./test-api.sh
```

**Tests:**
1. ✅ Health check
2. ✅ User registration
3. ✅ User login
4. ✅ Get user profile (with auth)
5. ✅ Log workout (with rewards)
6. ✅ Get quests
7. ✅ Complete quest
8. ✅ Invalid authentication (security)

### Manual Testing

```bash
# 1. Health check
curl http://localhost:8080/health

# 2. Create user
curl -X POST http://localhost:8080/api/users \
  -H "Content-Type: application/json" \
  -d '{"username":"testuser","email":"test@test.com","password":"password123"}'

# 3. Login
curl -X POST http://localhost:8080/api/auth/login \
  -H "Content-Type: application/json" \
  -d '{"email":"test@test.com","password":"password123"}'

# Save the token from response, then:

# 4. Log workout
curl -X POST http://localhost:8080/api/workouts \
  -H "Authorization: Bearer YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"type":"CARDIO","exerciseId":"running","duration":30,"intensity":7,"formScore":85}'
```

---

## 🚀 Deployment

### Docker (Recommended for Cloud)

Coming soon - Docker configuration files

### Manual Deployment

1. **Build release version:**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

2. **Set up systemd service:**
```bash
sudo nano /etc/systemd/system/fitness-quest.service
```

```ini
[Unit]
Description=Fitness Quest API Server
After=network.target

[Service]
Type=simple
User=www-data
WorkingDirectory=/opt/fitness-quest
ExecStart=/opt/fitness-quest/fitness_quest_server
Restart=always

[Install]
WantedBy=multi-user.target
```

3. **Enable and start:**
```bash
sudo systemctl enable fitness-quest
sudo systemctl start fitness-quest
sudo systemctl status fitness-quest
```

### Cloud Deployment (Azure/AWS)

See [DEPLOYMENT.md](DEPLOYMENT.md) for detailed cloud deployment guides.

---

## 🐛 Troubleshooting

### Build Errors

**Error: `cpprestsdk not found`**
```bash
# Ubuntu/Debian
sudo apt-get install libcpprest-dev

# macOS
brew install cpprestsdk
```

**Error: `OpenSSL not found`**
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# macOS
brew install openssl
export OPENSSL_ROOT_DIR=/usr/local/opt/openssl
```

**Error: `Boost not found`**
```bash
# Ubuntu/Debian
sudo apt-get install libboost-all-dev

# macOS
brew install boost
```

### Runtime Errors

**Error: `Port 8080 already in use`**
```bash
# Change port in .env
PORT=3000

# Or kill existing process
lsof -ti:8080 | xargs kill -9
```

**Error: `Database initialization failed`**
```bash
# Check permissions
chmod 755 fitness_data/

# Or specify different directory
DATA_DIR=/tmp/fitness_data
```

**Error: `401 Unauthorized`**
- Token might be expired (24 hour default)
- Token might be invalid
- Login again to get new token

### Performance Issues

**Slow response times:**
- Check database file size
- Monitor CPU/memory usage
- Consider migrating to PostgreSQL for production

---

## 📚 Additional Resources

- **API Testing Tool**: [Postman Collection](docs/postman_collection.json)
- **Database Schema**: [Database Documentation](docs/DATABASE.md)
- **Game Design**: [Game Mechanics Guide](docs/GAME_MECHANICS.md)
- **Contributing**: [Contribution Guidelines](CONTRIBUTING.md)

---

## 📄 License

MIT License - see [LICENSE](LICENSE) file

---

## 👥 Authors

Fitness Quest Development Team

---

## 🙏 Acknowledgments

- **cpprestsdk** - Microsoft's C++ REST SDK
- **OpenSSL** - Cryptography library
- **Boost** - C++ libraries

---

## 📞 Support

For issues, questions, or contributions:
- Open an issue on GitHub
- Check existing documentation
- Review troubleshooting section

---

**Happy Coding! 💪🎮**