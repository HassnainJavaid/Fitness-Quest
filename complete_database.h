#ifndef COMPLETE_FITNESS_DATABASE_H
#define COMPLETE_FITNESS_DATABASE_H

// ============================================================
// COMPLETE FITNESS QUEST DATABASE - WITH FILE PERSISTENCE
// ============================================================

#include <iostream>
#include <string>
#include <climits>
#include <vector>
#include <map>
#include <ctime>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <queue>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <list>
#include <set>
#include <cstring>
#include <sys/stat.h>
using namespace std;

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define mkdir(dir, mode) _mkdir(dir)
#define access _access
#define F_OK 0
#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace FitnessDB {

// ============================================================
// 0. FILE UTILITIES
// ============================================================

inline bool directory_exists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
}

inline bool create_directory(const std::string& path) {
    #ifdef _WIN32
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
    #else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
    #endif
}

inline bool file_exists(const std::string& filename) {
    #ifdef _WIN32
    return _access(filename.c_str(), F_OK) == 0;
    #else
    return access(filename.c_str(), F_OK) == 0;
    #endif
}

// ============================================================
// 1. SERIALIZATION HELPER FUNCTIONS
// ============================================================

inline void write_string(std::ofstream& os, const std::string& str) {
    size_t len = str.size();
    os.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) {
        os.write(str.c_str(), len);
    }
}

inline void read_string(std::ifstream& is, std::string& str) {
    size_t len;
    is.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len > 0 && len < 1000000) { // Sanity check
        str.resize(len);
        is.read(&str[0], len);
    } else {
        str.clear();
    }
}

inline void write_vector_string(std::ofstream& os, const std::vector<std::string>& vec) {
    size_t count = vec.size();
    os.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& s : vec) {
        write_string(os, s);
    }
}

inline void read_vector_string(std::ifstream& is, std::vector<std::string>& vec) {
    size_t count;
    is.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (count < 10000) { // Sanity check
        vec.resize(count);
        for (size_t i = 0; i < count; i++) {
            read_string(is, vec[i]);
        }
    } else {
        vec.clear();
    }
}

// ============================================================
// 2. COMPLETE B-TREE IMPLEMENTATION WITH PERSISTENCE
// ============================================================

template<typename K, typename V>
class CompleteBTree {
private:
    struct BTreeNode {
        bool is_leaf;
        vector<std::pair<K, V>> keys;
        std::vector<std::shared_ptr<BTreeNode>> children;
        
        BTreeNode(bool leaf = true) : is_leaf(leaf) {}
        
        int find_key_position(const K& key) const {
            for (size_t i = 0; i < keys.size(); i++) {
                if (key <= keys[i].first) {
                    return static_cast<int>(i);
                }
            }
            return static_cast<int>(keys.size());
        }
        
        void insert_key(const K& key, const V& value) {
            int pos = find_key_position(key);
            // Update if exists, insert if new
            if (pos < static_cast<int>(keys.size()) && keys[pos].first == key) {
                keys[pos].second = value;
            } else {
                keys.insert(keys.begin() + pos, {key, value});
            }
        }
        
        bool search_key(const K& key, V& value) const {
            for (const auto& kv : keys) {
                if (kv.first == key) {
                    value = kv.second;
                    return true;
                }
            }
            return false;
        }
    };
    
    std::shared_ptr<BTreeNode> root;
    const int ORDER = 3;
    const int MIN_KEYS = ORDER - 1;
    const int MAX_KEYS = 2 * ORDER - 1;
    
    void split_child(std::shared_ptr<BTreeNode> parent, int child_index) {
        auto child = parent->children[child_index];
        auto new_child = std::make_shared<BTreeNode>(child->is_leaf);
        
        int mid_index = MIN_KEYS;
        auto mid_key = child->keys[mid_index];
        
        for (size_t i = mid_index + 1; i < child->keys.size(); i++) {
            new_child->keys.push_back(child->keys[i]);
        }
        child->keys.resize(mid_index);
        
        if (!child->is_leaf) {
            for (size_t i = mid_index + 1; i < child->children.size(); i++) {
                new_child->children.push_back(child->children[i]);
            }
            child->children.resize(mid_index + 1);
        }
        
        parent->insert_key(mid_key.first, mid_key.second);
        parent->children.insert(parent->children.begin() + child_index + 1, new_child);
    }
    
    void insert_non_full(std::shared_ptr<BTreeNode> node, const K& key, const V& value) {
        if (node->is_leaf) {
            node->insert_key(key, value);
            return;
        }
        
        int i = static_cast<int>(node->keys.size()) - 1;
        while (i >= 0 && key < node->keys[i].first) {
            i--;
        }
        i++;
        
        if (static_cast<size_t>(i) < node->children.size() && 
            node->children[i]->keys.size() == static_cast<size_t>(MAX_KEYS)) {
            split_child(node, i);
            if (key > node->keys[i].first) {
                i++;
            }
        }
        
        if (static_cast<size_t>(i) < node->children.size()) {
            insert_non_full(node->children[i], key, value);
        }
    }
    
    bool search_node(std::shared_ptr<BTreeNode> node, const K& key, V& value) const {
        if (!node) return false;
        
        int i = 0;
        while (i < static_cast<int>(node->keys.size()) && key > node->keys[i].first) {
            i++;
        }
        
        if (i < static_cast<int>(node->keys.size()) && key == node->keys[i].first) {
            value = node->keys[i].second;
            return true;
        }
        
        if (node->is_leaf) {
            return false;
        }
        
        if (static_cast<size_t>(i) < node->children.size()) {
            return search_node(node->children[i], key, value);
        }
        
        return false;
    }
    
public:
    CompleteBTree() : root(std::make_shared<BTreeNode>(true)) {}
    
    void insert(const K& key, const V& value) {
        if (root->keys.size() == static_cast<size_t>(MAX_KEYS)) {
            auto new_root = std::make_shared<BTreeNode>(false);
            new_root->children.push_back(root);
            split_child(new_root, 0);
            root = new_root;
        }
        insert_non_full(root, key, value);
    }
    
    V search(const K& key) const {
        V value;
        if (search_node(root, key, value)) {
            return value;
        }
        throw std::runtime_error("Key not found in B-Tree");
    }
    
    bool exists(const K& key) const {
        V value;
        return search_node(root, key, value);
    }
    
    std::vector<K> get_all_keys() const {
        std::vector<K> keys;
        std::function<void(std::shared_ptr<BTreeNode>)> traverse = [&](std::shared_ptr<BTreeNode> node) {
            if (!node) return;
            
            for (size_t i = 0; i < node->keys.size(); i++) {
                if (!node->is_leaf && i < node->children.size()) {
                    traverse(node->children[i]);
                }
                keys.push_back(node->keys[i].first);
            }
            
            if (!node->is_leaf && node->keys.size() < node->children.size()) {
                traverse(node->children[node->keys.size()]);
            }
        };
        
        traverse(root);
        return keys;
    }
    
    int get_height() const {
        std::function<int(std::shared_ptr<BTreeNode>)> height = [&](std::shared_ptr<BTreeNode> node) -> int {
            if (!node) return 0;
            int h = 1;
            while (!node->is_leaf && !node->children.empty()) {
                node = node->children[0];
                h++;
            }
            return h;
        };
        return height(root);
    }
    
    size_t get_size() const {
        return get_all_keys().size();
    }
    
    std::vector<V> range_query(const K& start, const K& end) const {
        std::vector<V> results;
        std::function<void(std::shared_ptr<BTreeNode>)> traverse_range = 
            [&](std::shared_ptr<BTreeNode> node) {
                if (!node) return;
                
                for (const auto& kv : node->keys) {
                    if (kv.first >= start && kv.first <= end) {
                        results.push_back(kv.second);
                    }
                }
                
                if (!node->is_leaf) {
                    for (const auto& child : node->children) {
                        traverse_range(child);
                    }
                }
            };
        
        traverse_range(root);
        return results;
    }
    
    void save_to_file(const std::string& filename, 
                     std::function<void(std::ofstream&, const K&, const V&)> save_func) const {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file for writing: " + filename);
        }
        
        auto keys = get_all_keys();
        size_t count = keys.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        for (const auto& key : keys) {
            try {
                V value = search(key);
                save_func(file, key, value);
            } catch (...) {
                // Skip if not found
            }
        }
        
        file.close();
    }
    
    void load_from_file(const std::string& filename,
                       std::function<void(std::ifstream&, K&, V&)> load_func) {
        if (!file_exists(filename)) {
            return;
        }
        
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            return;
        }
        
        size_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        if (count > 1000000) { // Sanity check
            file.close();
            return;
        }
        
        for (size_t i = 0; i < count; i++) {
            K key;
            V value;
            try {
                load_func(file, key, value);
                insert(key, value);
            } catch (...) {
                break;
            }
        }
        
        file.close();
    }
    
    void clear() {
        root = std::make_shared<BTreeNode>(true);
    }
};

// ============================================================
// 3. UPDATED DATA MODELS WITH SERIALIZATION
// ============================================================

enum class ExerciseDifficulty { BEGINNER = 0, INTERMEDIATE = 1, ADVANCED = 2, EXPERT = 3 };
enum class ExerciseType { STRENGTH = 0, CARDIO = 1, FLEXIBILITY = 2, BALANCE = 3, CORE = 4 };

struct Exercise {
    std::string id;
    std::string name;
    ExerciseType type;
    ExerciseDifficulty difficulty;
    std::string description;
    std::vector<std::string> target_muscles;
    int calories_per_minute;
    std::vector<std::string> prerequisites;
    std::vector<std::string> next_exercises;
    time_t created_at;
    
    Exercise() : type(ExerciseType::STRENGTH), 
                difficulty(ExerciseDifficulty::BEGINNER),
                calories_per_minute(0),
                created_at(time(nullptr)) {}
    
    bool operator<(const Exercise& other) const { return id < other.id; }
    
    void serialize(std::ofstream& os) const {
        write_string(os, id);
        write_string(os, name);
        os.write(reinterpret_cast<const char*>(&type), sizeof(type));
        os.write(reinterpret_cast<const char*>(&difficulty), sizeof(difficulty));
        write_string(os, description);
        write_vector_string(os, target_muscles);
        os.write(reinterpret_cast<const char*>(&calories_per_minute), sizeof(calories_per_minute));
        write_vector_string(os, prerequisites);
        write_vector_string(os, next_exercises);
        os.write(reinterpret_cast<const char*>(&created_at), sizeof(created_at));
    }
    
    void deserialize(std::ifstream& is) {
        read_string(is, id);
        read_string(is, name);
        is.read(reinterpret_cast<char*>(&type), sizeof(type));
        is.read(reinterpret_cast<char*>(&difficulty), sizeof(difficulty));
        read_string(is, description);
        read_vector_string(is, target_muscles);
        is.read(reinterpret_cast<char*>(&calories_per_minute), sizeof(calories_per_minute));
        read_vector_string(is, prerequisites);
        read_vector_string(is, next_exercises);
        is.read(reinterpret_cast<char*>(&created_at), sizeof(created_at));
    }
};

struct User {
    std::string id;
    std::string username;
    std::string email;
    std::string password_hash;
    int fitness_level;
    int experience_points;
    std::vector<std::string> completed_exercises;
    std::vector<std::string> achievements;
    time_t created_at;
    time_t last_login;
    
    User() : fitness_level(1), experience_points(0), 
            created_at(time(nullptr)), last_login(time(nullptr)) {}
    
    bool operator<(const User& other) const { return id < other.id; }
    
    void serialize(std::ofstream& os) const {
        write_string(os, id);
        write_string(os, username);
        write_string(os, email);
        write_string(os, password_hash);
        os.write(reinterpret_cast<const char*>(&fitness_level), sizeof(fitness_level));
        os.write(reinterpret_cast<const char*>(&experience_points), sizeof(experience_points));
        write_vector_string(os, completed_exercises);
        write_vector_string(os, achievements);
        os.write(reinterpret_cast<const char*>(&created_at), sizeof(created_at));
        os.write(reinterpret_cast<const char*>(&last_login), sizeof(last_login));
    }
    
    void deserialize(std::ifstream& is) {
        read_string(is, id);
        read_string(is, username);
        read_string(is, email);
        read_string(is, password_hash);
        is.read(reinterpret_cast<char*>(&fitness_level), sizeof(fitness_level));
        is.read(reinterpret_cast<char*>(&experience_points), sizeof(experience_points));
        read_vector_string(is, completed_exercises);
        read_vector_string(is, achievements);
        is.read(reinterpret_cast<char*>(&created_at), sizeof(created_at));
        is.read(reinterpret_cast<char*>(&last_login), sizeof(last_login));
    }
};

struct Quest {
    std::string id;
    std::string title;
    std::string description;
    int priority;
    int difficulty;
    std::vector<std::string> required_exercises;
    std::vector<std::string> rewards;
    time_t deadline;
    bool completed;
    
    Quest() : priority(1), difficulty(1), completed(false), deadline(0) {}
    
    bool operator<(const Quest& other) const { return id < other.id; }
    
    void serialize(std::ofstream& os) const {
        write_string(os, id);
        write_string(os, title);
        write_string(os, description);
        os.write(reinterpret_cast<const char*>(&priority), sizeof(priority));
        os.write(reinterpret_cast<const char*>(&difficulty), sizeof(difficulty));
        write_vector_string(os, required_exercises);
        write_vector_string(os, rewards);
        os.write(reinterpret_cast<const char*>(&deadline), sizeof(deadline));
        os.write(reinterpret_cast<const char*>(&completed), sizeof(completed));
    }
    
    void deserialize(std::ifstream& is) {
        read_string(is, id);
        read_string(is, title);
        read_string(is, description);
        is.read(reinterpret_cast<char*>(&priority), sizeof(priority));
        is.read(reinterpret_cast<char*>(&difficulty), sizeof(difficulty));
        read_vector_string(is, required_exercises);
        read_vector_string(is, rewards);
        is.read(reinterpret_cast<char*>(&deadline), sizeof(deadline));
        is.read(reinterpret_cast<char*>(&completed), sizeof(completed));
    }
};

struct WorkoutSession {
    std::string id;
    std::string user_id;
    time_t start_time;
    time_t end_time;
    std::vector<std::string> exercises;
    int total_calories;
    bool validated;
    float form_score;
    
    WorkoutSession() : total_calories(0), validated(false), form_score(0.0f), 
                       start_time(time(nullptr)), end_time(0) {
        id = "WORKOUT_" + std::to_string(time(nullptr)) + "_" + std::to_string(rand() % 1000);
    }
    
    bool operator<(const WorkoutSession& other) const { return id < other.id; }
    
    void serialize(std::ofstream& os) const {
        write_string(os, id);
        write_string(os, user_id);
        os.write(reinterpret_cast<const char*>(&start_time), sizeof(start_time));
        os.write(reinterpret_cast<const char*>(&end_time), sizeof(end_time));
        write_vector_string(os, exercises);
        os.write(reinterpret_cast<const char*>(&total_calories), sizeof(total_calories));
        os.write(reinterpret_cast<const char*>(&validated), sizeof(validated));
        os.write(reinterpret_cast<const char*>(&form_score), sizeof(form_score));
    }
    
    void deserialize(std::ifstream& is) {
        read_string(is, id);
        read_string(is, user_id);
        is.read(reinterpret_cast<char*>(&start_time), sizeof(start_time));
        is.read(reinterpret_cast<char*>(&end_time), sizeof(end_time));
        read_vector_string(is, exercises);
        is.read(reinterpret_cast<char*>(&total_calories), sizeof(total_calories));
        is.read(reinterpret_cast<char*>(&validated), sizeof(validated));
        is.read(reinterpret_cast<char*>(&form_score), sizeof(form_score));
    }
};

// ============================================================
// 4. PERSISTENT FITNESS DATABASE
// ============================================================

class PersistentFitnessDatabase {
private:
    CompleteBTree<std::string, Exercise> exercise_btree;
    CompleteBTree<std::string, User> user_btree;
    CompleteBTree<std::string, WorkoutSession> workout_btree;
    CompleteBTree<std::string, Quest> quest_btree;
    
    static void save_exercise_pair(std::ofstream& os, const std::string& key, const Exercise& value) {
        write_string(os, key);
        value.serialize(os);
    }
    
    static void load_exercise_pair(std::ifstream& is, std::string& key, Exercise& value) {
        read_string(is, key);
        value.deserialize(is);
    }
    
    static void save_user_pair(std::ofstream& os, const std::string& key, const User& value) {
        write_string(os, key);
        value.serialize(os);
    }
    
    static void load_user_pair(std::ifstream& is, std::string& key, User& value) {
        read_string(is, key);
        value.deserialize(is);
    }
    
    static void save_workout_pair(std::ofstream& os, const std::string& key, const WorkoutSession& value) {
        write_string(os, key);
        value.serialize(os);
    }
    
    static void load_workout_pair(std::ifstream& is, std::string& key, WorkoutSession& value) {
        read_string(is, key);
        value.deserialize(is);
    }
    
    static void save_quest_pair(std::ofstream& os, const std::string& key, const Quest& value) {
        write_string(os, key);
        value.serialize(os);
    }
    
    static void load_quest_pair(std::ifstream& is, std::string& key, Quest& value) {
        read_string(is, key);
        value.deserialize(is);
    }
    
    struct HashTableEntry {
        std::string key;
        std::string value;
        
        void serialize(std::ofstream& os) const {
            write_string(os, key);
            write_string(os, value);
        }
        
        void deserialize(std::ifstream& is) {
            read_string(is, key);
            read_string(is, value);
        }
    };
    
    std::vector<HashTableEntry> email_index;
    
    struct GraphEdge {
        std::string from;
        std::string to;
        int weight;
        
        void serialize(std::ofstream& os) const {
            write_string(os, from);
            write_string(os, to);
            os.write(reinterpret_cast<const char*>(&weight), sizeof(weight));
        }
        
        void deserialize(std::ifstream& is) {
            read_string(is, from);
            read_string(is, to);
            is.read(reinterpret_cast<char*>(&weight), sizeof(weight));
        }
    };
    
    std::vector<GraphEdge> graph_edges;
    
    struct PriorityQueueEntry {
        Quest quest;
        int priority;
        time_t timestamp;
        
        void serialize(std::ofstream& os) const {
            quest.serialize(os);
            os.write(reinterpret_cast<const char*>(&priority), sizeof(priority));
            os.write(reinterpret_cast<const char*>(&timestamp), sizeof(timestamp));
        }
        
        void deserialize(std::ifstream& is) {
            quest.deserialize(is);
            is.read(reinterpret_cast<char*>(&priority), sizeof(priority));
            is.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
        }
    };
    
    std::vector<PriorityQueueEntry> pq_entries;
    std::string data_dir;
    
    void ensure_data_dir() {
        if (!directory_exists(data_dir)) {
            if (!create_directory(data_dir)) {
                std::cerr << "Warning: Could not create directory: " << data_dir << std::endl;
            }
        }
    }
    
    std::string get_file_path(const std::string& filename) const {
        return data_dir + "/" + filename;
    }
    
public:
    PersistentFitnessDatabase(const std::string& directory = "./fitness_data") 
        : data_dir(directory) {
        
        ensure_data_dir();
        load_all_data();
        
        if (user_btree.get_size() == 0) {
            initialize_sample_data();
        }
    }
    
    ~PersistentFitnessDatabase() {
        save_all_data();
    }
    
    void save_all_data() {
        try {
            exercise_btree.save_to_file(get_file_path("exercises.dat"), save_exercise_pair);
            user_btree.save_to_file(get_file_path("users.dat"), save_user_pair);
            workout_btree.save_to_file(get_file_path("workouts.dat"), save_workout_pair);
            quest_btree.save_to_file(get_file_path("quests.dat"), save_quest_pair);
            
            save_hash_table();
            save_graph();
            save_priority_queue();
            
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to save data: " << e.what() << std::endl;
        }
    }
    
    void load_all_data() {
        try {
            exercise_btree.load_from_file(get_file_path("exercises.dat"), load_exercise_pair);
            user_btree.load_from_file(get_file_path("users.dat"), load_user_pair);
            workout_btree.load_from_file(get_file_path("workouts.dat"), load_workout_pair);
            quest_btree.load_from_file(get_file_path("quests.dat"), load_quest_pair);
            
            load_hash_table();
            load_graph();
            load_priority_queue();
            
        } catch (const std::exception& e) {
            // Silent fail for first run
        }
    }
    
    void save_hash_table() {
        std::ofstream file(get_file_path("email_index.dat"), std::ios::binary);
        if (!file) return;
        
        size_t count = email_index.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        for (const auto& entry : email_index) {
            entry.serialize(file);
        }
        
        file.close();
    }
    
    void load_hash_table() {
        if (!file_exists(get_file_path("email_index.dat"))) return;
        
        std::ifstream file(get_file_path("email_index.dat"), std::ios::binary);
        if (!file) return;
        
        size_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        if (count < 100000) {
            email_index.resize(count);
            for (size_t i = 0; i < count; i++) {
                email_index[i].deserialize(file);
            }
        }
        
        file.close();
    }
    
    void save_graph() {
        std::ofstream file(get_file_path("graph.dat"), std::ios::binary);
        if (!file) return;
        
        size_t count = graph_edges.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        for (const auto& edge : graph_edges) {
            edge.serialize(file);
        }
        
        file.close();
    }
    
    void load_graph() {
        if (!file_exists(get_file_path("graph.dat"))) return;
        
        std::ifstream file(get_file_path("graph.dat"), std::ios::binary);
        if (!file) return;
        
        size_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        if (count < 100000) {
            graph_edges.resize(count);
            for (size_t i = 0; i < count; i++) {
                graph_edges[i].deserialize(file);
            }
        }
        
        file.close();
    }
    
    void save_priority_queue() {
        std::ofstream file(get_file_path("priority_queue.dat"), std::ios::binary);
        if (!file) return;
        
        size_t count = pq_entries.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(count));
        
        for (const auto& entry : pq_entries) {
            entry.serialize(file);
        }
        
        file.close();
    }
    
    void load_priority_queue() {
        if (!file_exists(get_file_path("priority_queue.dat"))) return;
        
        std::ifstream file(get_file_path("priority_queue.dat"), std::ios::binary);
        if (!file) return;
        
        size_t count;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        if (count < 100000) {
            pq_entries.resize(count);
            for (size_t i = 0; i < count; i++) {
                pq_entries[i].deserialize(file);
            }
        }
        
        file.close();
    }
    
    void initialize_sample_data() {
        Exercise pushup;
        pushup.id = "EX001";
        pushup.name = "Push-up";
        pushup.type = ExerciseType::STRENGTH;
        pushup.difficulty = ExerciseDifficulty::BEGINNER;
        pushup.calories_per_minute = 8;
        pushup.prerequisites = {};
        pushup.next_exercises = {"EX002"};
        exercise_btree.insert(pushup.id, pushup);
        
        Exercise squat;
        squat.id = "EX002";
        squat.name = "Squat";
        squat.type = ExerciseType::STRENGTH;
        squat.difficulty = ExerciseDifficulty::BEGINNER;
        squat.calories_per_minute = 7;
        squat.prerequisites = {"EX001"};
        exercise_btree.insert(squat.id, squat);
        
        User admin;
        admin.id = "ADMIN001";
        admin.username = "Admin";
        admin.email = "admin@fitnessquest.com";
        admin.password_hash = "hashed_password";
        admin.fitness_level = 10;
        user_btree.insert(admin.id, admin);
        
        email_index.push_back({admin.email, admin.id});
        
        graph_edges.push_back({"EX001", "EX002", 1});
        
        Quest daily;
        daily.id = "Q001";
        daily.title = "Daily Challenge";
        daily.description = "Complete basic exercises";
        daily.priority = 1;
        daily.required_exercises = {"EX001", "EX002"};
        daily.rewards = {"100 XP"};
        quest_btree.insert(daily.id, daily);
        pq_entries.push_back({daily, daily.priority, time(nullptr)});
        
        save_all_data();
    }
    
    std::string create_user(const std::string& username, const std::string& email, 
                           const std::string& password) {
        for (const auto& entry : email_index) {
            if (entry.key == email) {
                throw std::runtime_error("Email already registered");
            }
        }
        
        User user;
        user.id = "USER_" + std::to_string(time(nullptr)) + "_" + std::to_string(rand() % 10000);
        user.username = username;
        user.email = email;
        user.password_hash = std::to_string(std::hash<std::string>{}(password));
        
        user_btree.insert(user.id, user);
        email_index.push_back({email, user.id});
        
        save_all_data();
        
        return user.id;
    }
    
    User get_user(const std::string& user_id) {
        return user_btree.search(user_id);
    }
    
    User get_user_by_email(const std::string& email) {
        for (const auto& entry : email_index) {
            if (entry.key == email) {
                return user_btree.search(entry.value);
            }
        }
        throw std::runtime_error("User not found with email: " + email);
    }
    
    void update_user(const User& user) {
        user_btree.insert(user.id, user);
        save_all_data();
    }
    
    void add_exercise(const Exercise& exercise) {
        exercise_btree.insert(exercise.id, exercise);
        
        for (const auto& prereq : exercise.prerequisites) {
            graph_edges.push_back({prereq, exercise.id, 1});
        }
        
        save_all_data();
    }
    
    Exercise get_exercise(const std::string& exercise_id) {
        return exercise_btree.search(exercise_id);
    }
    
    std::vector<Exercise> get_all_exercises() {
        std::vector<Exercise> exercises;
        auto keys = exercise_btree.get_all_keys();
        for (const auto& key : keys) {
            try {
                exercises.push_back(exercise_btree.search(key));
            } catch (...) {}
        }
        return exercises;
    }
    
    std::string start_workout(const std::string& user_id) {
        WorkoutSession session;
        session.user_id = user_id;
        workout_btree.insert(session.id, session);
        
        save_all_data();
        return session.id;
    }
    
    void complete_workout(const std::string& workout_id) {
        WorkoutSession session = workout_btree.search(workout_id);
        session.end_time = time(nullptr);
        workout_btree.insert(workout_id, session);
        
        save_all_data();
    }
    
    WorkoutSession get_workout(const std::string& workout_id) {
        return workout_btree.search(workout_id);
    }
    
    void add_quest(const Quest& quest) {
        quest_btree.insert(quest.id, quest);
        pq_entries.push_back({quest, quest.priority, time(nullptr)});
        
        std::sort(pq_entries.begin(), pq_entries.end(), 
                 [](const PriorityQueueEntry& a, const PriorityQueueEntry& b) {
                     return a.priority > b.priority;
                 });
        
        save_all_data();
    }
    
    Quest get_next_quest() {
        if (pq_entries.empty()) {
            throw std::runtime_error("No quests available");
        }
        
        Quest quest = pq_entries.back().quest;
        pq_entries.pop_back();
        
        save_all_data();
        return quest;
    }
    
    Quest get_quest(const std::string& quest_id) {
        return quest_btree.search(quest_id);
    }
    
    std::vector<Quest> get_all_quests() {
        std::vector<Quest> quests;
        auto keys = quest_btree.get_all_keys();
        for (const auto& key : keys) {
            try {
                quests.push_back(quest_btree.search(key));
            } catch (...) {}
        }
        return quests;
    }
    
    std::vector<GraphEdge> get_exercise_graph() const {
        return graph_edges;
    }
    
    struct DatabaseStats {
        struct BTreeStats {
            size_t exercise_count;
            size_t user_count;
            size_t workout_count;
            size_t quest_count;
        } btree;
        
        struct OtherStats {
            size_t email_index_size;
            size_t graph_edges;
            size_t priority_queue_size;
        } other;
    };
    
    DatabaseStats get_stats() const {
        DatabaseStats stats;
        
        stats.btree.exercise_count = exercise_btree.get_size();
        stats.btree.user_count = user_btree.get_size();
        stats.btree.workout_count = workout_btree.get_size();
        stats.btree.quest_count = quest_btree.get_size();
        
        stats.other.email_index_size = email_index.size();
        stats.other.graph_edges = graph_edges.size();
        stats.other.priority_queue_size = pq_entries.size();
        
        return stats;
    }
    
    bool test_persistence() {
        try {
            std::string user_id = create_user("TestUser", "test@test.com", "password");
            
            PersistentFitnessDatabase new_db(data_dir);
            
            User loaded_user = new_db.get_user_by_email("test@test.com");
            
            return loaded_user.username == "TestUser";
            
        } catch (const std::exception& e) {
            std::cerr << "Persistence test failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    void clear_all_data() {
        email_index.clear();
        graph_edges.clear();
        pq_entries.clear();
        
        exercise_btree.clear();
        user_btree.clear();
        workout_btree.clear();
        quest_btree.clear();
        
        std::vector<std::string> files = {
            "exercises.dat", "users.dat", "workouts.dat", "quests.dat",
            "email_index.dat", "graph.dat", "priority_queue.dat"
        };
        
        for (const auto& file : files) {
            std::string path = get_file_path(file);
            if (file_exists(path)) {
                std::remove(path.c_str());
            }
        }
        
        initialize_sample_data();
    }
};

} // namespace FitnessDB

#endif // COMPLETE_FITNESS_DATABASE_H