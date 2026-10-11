#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>

const std::string DB_FILE = "bptree.db";

// B+ Tree implementation with explicit tree structure
class BPTree {
private:
    struct BPTreeNode {
        bool is_leaf;
        std::vector<std::string> keys;
        std::vector<BPTreeNode*> children;
        std::vector<std::vector<int>> values;  // Only for leaf nodes
        BPTreeNode* next_leaf;
        
        BPTreeNode(bool leaf = false) : is_leaf(leaf), next_leaf(nullptr) {}
        
        ~BPTreeNode() {
            for (auto child : children) {
                delete child;
            }
        }
    };
    
    BPTreeNode* root;
    const int ORDER = 4;  // B+ Tree order
    
    // File operations
    void save_to_file() {
        std::ofstream file(DB_FILE, std::ios::binary);
        if (!file.is_open()) return;
        
        write_node(file, root);
        file.close();
    }
    
    void write_node(std::ofstream& file, BPTreeNode* node) {
        if (!node) {
            bool null = false;
            file.write(reinterpret_cast<const char*>(&null), sizeof(bool));
            return;
        }
        
        bool not_null = true;
        file.write(reinterpret_cast<const char*>(&not_null), sizeof(bool));
        file.write(reinterpret_cast<const char*>(&node->is_leaf), sizeof(bool));
        
        int key_count = node->keys.size();
        file.write(reinterpret_cast<const char*>(&key_count), sizeof(int));
        
        for (const auto& key : node->keys) {
            int key_len = key.length();
            file.write(reinterpret_cast<const char*>(&key_len), sizeof(int));
            file.write(key.c_str(), key_len);
        }
        
        if (node->is_leaf) {
            for (const auto& val_list : node->values) {
                int val_count = val_list.size();
                file.write(reinterpret_cast<const char*>(&val_count), sizeof(int));
                for (int val : val_list) {
                    file.write(reinterpret_cast<const char*>(&val), sizeof(int));
                }
            }
        } else {
            for (auto child : node->children) {
                write_node(file, child);
            }
        }
    }
    
    void load_from_file() {
        std::ifstream file(DB_FILE, std::ios::binary);
        if (!file.is_open()) return;
        
        root = read_node(file);
        file.close();
    }
    
    BPTreeNode* read_node(std::ifstream& file) {
        bool exists;
        file.read(reinterpret_cast<char*>(&exists), sizeof(bool));
        if (!exists) return nullptr;
        
        BPTreeNode* node = new BPTreeNode();
        file.read(reinterpret_cast<char*>(&node->is_leaf), sizeof(bool));
        
        int key_count;
        file.read(reinterpret_cast<char*>(&key_count), sizeof(int));
        
        for (int i = 0; i < key_count; i++) {
            int key_len;
            file.read(reinterpret_cast<char*>(&key_len), sizeof(int));
            std::string key(key_len, '\0');
            file.read(&key[0], key_len);
            node->keys.push_back(key);
        }
        
        if (node->is_leaf) {
            for (int i = 0; i < key_count; i++) {
                int val_count;
                file.read(reinterpret_cast<char*>(&val_count), sizeof(int));
                std::vector<int> vals;
                for (int j = 0; j < val_count; j++) {
                    int val;
                    file.read(reinterpret_cast<char*>(&val), sizeof(int));
                    vals.push_back(val);
                }
                node->values.push_back(vals);
            }
        } else {
            for (int i = 0; i <= key_count; i++) {
                BPTreeNode* child = read_node(file);
                node->children.push_back(child);
            }
        }
        
        return node;
    }
    
    // B+ Tree operations
    BPTreeNode* find_leaf(const std::string& key) {
        if (!root) return nullptr;
        
        BPTreeNode* current = root;
        while (!current->is_leaf) {
            int i = 0;
            while (i < (int)current->keys.size() && key >= current->keys[i]) {
                i++;
            }
            current = current->children[i];
        }
        
        return current;
    }
    
    void insert_into_leaf(BPTreeNode* leaf, const std::string& key, int value) {
        // Find position for key
        int pos = std::lower_bound(leaf->keys.begin(), leaf->keys.end(), key) - leaf->keys.begin();
        
        if (pos < (int)leaf->keys.size() && leaf->keys[pos] == key) {
            // Key exists, add value if not duplicate
            auto& vals = leaf->values[pos];
            if (std::find(vals.begin(), vals.end(), value) == vals.end()) {
                vals.insert(std::lower_bound(vals.begin(), vals.end(), value), value);
            }
        } else {
            // Insert new key
            leaf->keys.insert(leaf->keys.begin() + pos, key);
            leaf->values.insert(leaf->values.begin() + pos, std::vector<int>{value});
        }
    }
    
    void split_leaf(BPTreeNode* leaf) {
        BPTreeNode* new_leaf = new BPTreeNode(true);
        new_leaf->next_leaf = leaf->next_leaf;
        leaf->next_leaf = new_leaf;
        
        int split_pos = leaf->keys.size() / 2;
        
        // Move keys to new leaf
        new_leaf->keys.assign(leaf->keys.begin() + split_pos, leaf->keys.end());
        new_leaf->values.assign(leaf->values.begin() + split_pos, leaf->values.end());
        
        leaf->keys.resize(split_pos);
        leaf->values.resize(split_pos);
        
        // Insert separator into parent
        insert_into_parent(leaf, new_leaf, new_leaf->keys[0]);
    }
    
    void insert_into_parent(BPTreeNode* left, BPTreeNode* right, const std::string& key) {
        if (left == root) {
            // Create new root
            BPTreeNode* new_root = new BPTreeNode(false);
            new_root->keys.push_back(key);
            new_root->children.push_back(left);
            new_root->children.push_back(right);
            root = new_root;
            return;
        }
        
        // Find parent
        BPTreeNode* parent = find_parent(root, left);
        if (!parent) return;
        
        // Find insertion position
        int pos = 0;
        while (pos < (int)parent->children.size() && parent->children[pos] != left) {
            pos++;
        }
        
        // Insert key and child
        parent->keys.insert(parent->keys.begin() + pos, key);
        parent->children.insert(parent->children.begin() + pos + 1, right);
        
        // Split if necessary
        if (parent->keys.size() >= ORDER) {
            split_internal(parent);
        }
    }
    
    void split_internal(BPTreeNode* node) {
        BPTreeNode* new_node = new BPTreeNode(false);
        int split_pos = (ORDER - 1) / 2;
        std::string split_key = node->keys[split_pos];
        
        // Move keys and children to new node
        new_node->keys.assign(node->keys.begin() + split_pos + 1, node->keys.end());
        new_node->children.assign(node->children.begin() + split_pos + 1, node->children.end());
        
        // Resize original node
        node->keys.resize(split_pos);
        node->children.resize(split_pos + 1);
        
        // Insert split key into parent
        insert_into_parent(node, new_node, split_key);
    }
    
    BPTreeNode* find_parent(BPTreeNode* current, BPTreeNode* child) {
        if (current->is_leaf) return nullptr;
        
        for (auto c : current->children) {
            if (c == child) return current;
            BPTreeNode* parent = find_parent(c, child);
            if (parent) return parent;
        }
        return nullptr;
    }
    
public:
    BPTree() : root(nullptr) {
        load_from_file();
    }
    
    ~BPTree() {
        save_to_file();
        delete root;
    }
    
    void insert(const std::string& key, int value) {
        if (!root) {
            root = new BPTreeNode(true);
        }
        
        BPTreeNode* leaf = find_leaf(key);
        insert_into_leaf(leaf, key, value);
        
        if (leaf->keys.size() >= ORDER) {
            split_leaf(leaf);
        }
        
        save_to_file();
    }
    
    void remove(const std::string& key, int value) {
        if (!root) return;
        
        BPTreeNode* leaf = find_leaf(key);
        
        for (int i = 0; i < (int)leaf->keys.size(); i++) {
            if (leaf->keys[i] == key) {
                auto& vals = leaf->values[i];
                auto it = std::find(vals.begin(), vals.end(), value);
                if (it != vals.end()) {
                    vals.erase(it);
                    if (vals.empty()) {
                        leaf->keys.erase(leaf->keys.begin() + i);
                        leaf->values.erase(leaf->values.begin() + i);
                    }
                }
                break;
            }
        }
        
        save_to_file();
    }
    
    std::vector<int> find(const std::string& key) {
        std::vector<int> result;
        if (!root) return result;
        
        BPTreeNode* leaf = find_leaf(key);
        
        for (int i = 0; i < (int)leaf->keys.size(); i++) {
            if (leaf->keys[i] == key) {
                result = leaf->values[i];
                break;
            }
        }
        
        return result;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    BPTree tree;
    
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