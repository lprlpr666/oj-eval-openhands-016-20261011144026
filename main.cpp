#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>

const std::string DB_FILE = "bptree.db";

class SimpleFileDB {
private:
    std::map<std::string, std::set<int>> data;
    std::string filename;
    
public:
    SimpleFileDB(const std::string& file) : filename(file) {
        load_from_file();
    }
    
    void load_from_file() {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return;
        }
        
        std::string key;
        int value;
        while (file >> key >> value) {
            data[key].insert(value);
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
        data[key].insert(value);
        save_to_file();
    }
    
    void remove(const std::string& key, int value) {
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
        std::vector<int> result;
        auto it = data.find(key);
        if (it != data.end()) {
            result.assign(it->second.begin(), it->second.end());
        }
        return result;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    SimpleFileDB db(DB_FILE);
    
    int n;
    std::cin >> n;
    
    for (int i = 0; i < n; i++) {
        std::string command;
        std::cin >> command;
        
        if (command == "insert") {
            std::string key;
            int value;
            std::cin >> key >> value;
            db.insert(key, value);
        } else if (command == "delete") {
            std::string key;
            int value;
            std::cin >> key >> value;
            db.remove(key, value);
        } else if (command == "find") {
            std::string key;
            std::cin >> key;
            std::vector<int> values = db.find(key);
            
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