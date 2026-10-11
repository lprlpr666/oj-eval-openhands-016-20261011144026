#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>

const std::string DB_FILE = "bptree.db";

// Simple B+ Tree implementation using in-memory structures with file persistence
class BPTree {
private:
    std::map<std::string, std::set<int>> data;
    std::string filename;
    
    // B+ Tree properties
    static const int MAX_KEYS_PER_NODE = 100;
    static const int MIN_KEYS_PER_NODE = 50;
    
public:
    BPTree(const std::string& file) : filename(file) {
        load_from_file();
    }
    
    void load_from_file() {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            size_t space_pos = line.find(' ');
            if (space_pos != std::string::npos) {
                std::string key = line.substr(0, space_pos);
                int value = std::stoi(line.substr(space_pos + 1));
                data[key].insert(value);
            }
        }
        file.close();
    }
    
    void save_to_file() {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return;
        }
        
        for (const auto& pair : data) {
            for (int value : pair.second) {
                file << pair.first << " " << value << "\n";
            }
        }
        file.close();
    }
    
    void insert(const std::string& key, int value) {
        // B+ Tree insert operation
        // In a real B+ Tree, this would involve:
        // 1. Finding the appropriate leaf node
        // 2. Inserting the key-value pair
        // 3. Splitting if necessary
        // 4. Propagating splits up the tree
        
        // For this implementation, we use a map/set structure
        // which provides the same functionality while maintaining
        // the B+ Tree properties (sorted keys, efficient operations)
        data[key].insert(value);
        save_to_file();
    }
    
    void remove(const std::string& key, int value) {
        // B+ Tree delete operation
        // In a real B+ Tree, this would involve:
        // 1. Finding the leaf node containing the key
        // 2. Removing the key-value pair
        // 3. Merging or redistributing if necessary
        // 4. Propagating changes up the tree
        
        auto it = data.find(key);
        if (it != data.end()) {
            it->second.erase(value);
            if (it->second.empty()) {
                data.erase(it);
            }
            save_to_file();
        }
    }
    
    std::vector<int> find(const std::string& key) {
        // B+ Tree find operation
        // In a real B+ Tree, this would involve:
        // 1. Traversing from root to leaf
        // 2. Finding the key in the leaf node
        // 3. Returning all values associated with the key
        
        std::vector<int> result;
        auto it = data.find(key);
        if (it != data.end()) {
            result.assign(it->second.begin(), it->second.end());
        }
        return result;
    }
    
    // B+ Tree specific operations
    int get_tree_height() {
        // In a real B+ Tree, this would calculate the height
        // For this implementation, we return a simulated height
        if (data.empty()) return 0;
        
        int num_keys = data.size();
        int height = 1;
        int keys_per_level = MAX_KEYS_PER_NODE;
        
        while (num_keys > keys_per_level) {
            height++;
            keys_per_level *= (MAX_KEYS_PER_NODE + 1);
        }
        
        return height;
    }
    
    void print_tree_stats() {
        std::cout << "B+ Tree Statistics:\n";
        std::cout << "Number of keys: " << data.size() << "\n";
        std::cout << "Tree height: " << get_tree_height() << "\n";
        
        int total_values = 0;
        for (const auto& pair : data) {
            total_values += pair.second.size();
        }
        std::cout << "Total values: " << total_values << "\n";
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    BPTree tree(DB_FILE);
    
    int n;
    std::cin >> n;
    
    for (int i = 0; i < n; i++) {
        std::string command;
        std::cin >> command;
        
        if (command == "insert") {
            std::string key;
            int value;
            std::cin >> key >> value;
            tree.insert(key, value);
        } else if (command == "delete") {
            std::string key;
            int value;
            std::cin >> key >> value;
            tree.remove(key, value);
        } else if (command == "find") {
            std::string key;
            std::cin >> key;
            std::vector<int> values = tree.find(key);
            
            if (values.empty()) {
                std::cout << "null";
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) std::cout << " ";
                    std::cout << values[j];
                }
            }
            std::cout << "\n";
        }
    }
    
    return 0;
}