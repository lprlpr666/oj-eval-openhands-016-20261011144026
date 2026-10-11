#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <memory>

const std::string DB_FILE = "bptree.db";
const int MAX_KEY_LENGTH = 64;
const int ORDER = 4;  // Small order for simplicity and testing
const int MAX_KEYS_PER_NODE = ORDER - 1;
const int MAX_VALUES_PER_KEY = 10;

// B+ Tree Node structure
struct BPTreeNode {
    bool is_leaf;
    int key_count;
    std::string keys[MAX_KEYS_PER_NODE];
    
    // For leaf nodes
    std::vector<int> values[MAX_KEYS_PER_NODE];
    
    // For internal nodes
    BPTreeNode* children[MAX_KEYS_PER_NODE + 1];
    BPTreeNode* next;  // For leaf node linking
    
    BPTreeNode* parent;
    
    BPTreeNode() : is_leaf(false), key_count(0), next(nullptr), parent(nullptr) {
        for (int i = 0; i < MAX_KEYS_PER_NODE + 1; i++) {
            children[i] = nullptr;
        }
    }
    
    ~BPTreeNode() {
        if (!is_leaf) {
            for (int i = 0; i <= key_count; i++) {
                delete children[i];
            }
        }
    }
};

class BPTree {
private:
    BPTreeNode* root;
    std::string filename;
    
    void save_node_to_file(std::ofstream& file, BPTreeNode* node) {
        if (!node) return;
        
        file << node->is_leaf << " " << node->key_count;
        
        for (int i = 0; i < node->key_count; i++) {
            file << " " << node->keys[i];
        }
        
        if (node->is_leaf) {
            for (int i = 0; i < node->key_count; i++) {
                file << " " << node->values[i].size();
                for (int val : node->values[i]) {
                    file << " " << val;
                }
            }
        }
        
        file << "\n";
        
        if (!node->is_leaf) {
            for (int i = 0; i <= node->key_count; i++) {
                save_node_to_file(file, node->children[i]);
            }
        }
    }
    
    void load_node_from_file(std::ifstream& file, BPTreeNode*& node) {
        std::string line;
        if (!std::getline(file, line)) {
            node = nullptr;
            return;
        }
        
        node = new BPTreeNode();
        std::istringstream iss(line);
        
        iss >> node->is_leaf >> node->key_count;
        
        for (int i = 0; i < node->key_count; i++) {
            iss >> node->keys[i];
        }
        
        if (node->is_leaf) {
            for (int i = 0; i < node->key_count; i++) {
                int val_count;
                iss >> val_count;
                for (int j = 0; j < val_count; j++) {
                    int val;
                    iss >> val;
                    node->values[i].push_back(val);
                }
            }
        } else {
            for (int i = 0; i <= node->key_count; i++) {
                load_node_from_file(file, node->children[i]);
                if (node->children[i]) {
                    node->children[i]->parent = node;
                }
            }
        }
    }
    
    BPTreeNode* find_leaf(const std::string& key) {
        if (!root) return nullptr;
        
        BPTreeNode* current = root;
        while (!current->is_leaf) {
            int i = 0;
            while (i < current->key_count && key >= current->keys[i]) {
                i++;
            }
            current = current->children[i];
        }
        
        return current;
    }
    
    void insert_into_leaf(BPTreeNode* leaf, const std::string& key, int value) {
        // Check if key already exists
        int pos = 0;
        while (pos < leaf->key_count && leaf->keys[pos] != key) {
            pos++;
        }
        
        if (pos < leaf->key_count) {
            // Key exists, add value if not duplicate
            for (int val : leaf->values[pos]) {
                if (val == value) return; // Duplicate
            }
            
            // Insert value in sorted order
            auto& vals = leaf->values[pos];
            auto it = std::lower_bound(vals.begin(), vals.end(), value);
            vals.insert(it, value);
        } else {
            // Find insertion position for new key
            pos = 0;
            while (pos < leaf->key_count && leaf->keys[pos] < key) {
                pos++;
            }
            
            // Shift keys to make room
            for (int i = leaf->key_count; i > pos; i--) {
                leaf->keys[i] = leaf->keys[i-1];
                leaf->values[i] = leaf->values[i-1];
            }
            
            // Insert new key
            leaf->keys[pos] = key;
            leaf->values[pos].push_back(value);
            leaf->key_count++;
        }
    }
    
    void split_leaf(BPTreeNode* leaf) {
        BPTreeNode* new_leaf = new BPTreeNode();
        new_leaf->is_leaf = true;
        new_leaf->next = leaf->next;
        leaf->next = new_leaf;
        new_leaf->parent = leaf->parent;
        
        int split_point = leaf->key_count / 2;
        
        // Move keys to new leaf
        for (int i = split_point; i < leaf->key_count; i++) {
            new_leaf->keys[i - split_point] = leaf->keys[i];
            new_leaf->values[i - split_point] = leaf->values[i];
        }
        
        new_leaf->key_count = leaf->key_count - split_point;
        leaf->key_count = split_point;
        
        // Insert first key of new leaf into parent
        std::string split_key = new_leaf->keys[0];
        insert_into_parent(leaf->parent, leaf, new_leaf, split_key);
    }
    
    void insert_into_parent(BPTreeNode* parent, BPTreeNode* left, BPTreeNode* right, const std::string& key) {
        if (!parent) {
            // Create new root
            BPTreeNode* new_root = new BPTreeNode();
            new_root->keys[0] = key;
            new_root->children[0] = left;
            new_root->children[1] = right;
            new_root->key_count = 1;
            new_root->is_leaf = false;
            
            left->parent = new_root;
            right->parent = new_root;
            
            root = new_root;
            return;
        }
        
        // Find insertion position
        int pos = 0;
        while (pos < parent->key_count && parent->children[pos] != left) {
            pos++;
        }
        pos++;
        
        // Shift to make room
        for (int i = parent->key_count; i > pos; i--) {
            parent->keys[i] = parent->keys[i-1];
            parent->children[i+1] = parent->children[i];
        }
        
        // Insert key and child
        parent->keys[pos] = key;
        parent->children[pos] = right;
        parent->key_count++;
        
        // Check if parent needs to split
        if (parent->key_count >= MAX_KEYS_PER_NODE) {
            split_internal(parent);
        }
    }
    
    void split_internal(BPTreeNode* node) {
        BPTreeNode* new_node = new BPTreeNode();
        new_node->is_leaf = false;
        new_node->parent = node->parent;
        
        int split_point = node->key_count / 2;
        std::string split_key = node->keys[split_point];
        
        // Move keys and children to new node
        for (int i = split_point + 1; i < node->key_count; i++) {
            new_node->keys[i - split_point - 1] = node->keys[i];
            new_node->children[i - split_point - 1] = node->children[i];
            node->children[i]->parent = new_node;
        }
        new_node->children[node->key_count - split_point - 1] = node->children[node->key_count];
        node->children[node->key_count]->parent = new_node;
        
        new_node->key_count = node->key_count - split_point - 1;
        node->key_count = split_point;
        
        // Insert split key into parent
        insert_into_parent(node->parent, node, new_node, split_key);
    }
    
    void delete_from_leaf(BPTreeNode* leaf, const std::string& key, int value) {
        int pos = 0;
        while (pos < leaf->key_count && leaf->keys[pos] != key) {
            pos++;
        }
        
        if (pos < leaf->key_count) {
            auto& vals = leaf->values[pos];
            auto it = std::find(vals.begin(), vals.end(), value);
            if (it != vals.end()) {
                vals.erase(it);
                
                // If no more values, remove key
                if (vals.empty()) {
                    for (int i = pos; i < leaf->key_count - 1; i++) {
                        leaf->keys[i] = leaf->keys[i+1];
                        leaf->values[i] = leaf->values[i+1];
                    }
                    leaf->key_count--;
                }
            }
        }
    }
    
public:
    BPTree(const std::string& file) : root(nullptr), filename(file) {
        load_from_file();
    }
    
    ~BPTree() {
        delete root;
    }
    
    void save_to_file() {
        std::ofstream file(filename);
        if (!file.is_open()) return;
        
        save_node_to_file(file, root);
        file.close();
    }
    
    void load_from_file() {
        std::ifstream file(filename);
        if (!file.is_open()) return;
        
        load_node_from_file(file, root);
        file.close();
    }
    
    void insert(const std::string& key, int value) {
        if (!root) {
            root = new BPTreeNode();
            root->is_leaf = true;
        }
        
        BPTreeNode* leaf = find_leaf(key);
        insert_into_leaf(leaf, key, value);
        
        // Split leaf if necessary
        if (leaf->key_count >= MAX_KEYS_PER_NODE) {
            split_leaf(leaf);
        }
        
        save_to_file();
    }
    
    void remove(const std::string& key, int value) {
        if (!root) return;
        
        BPTreeNode* leaf = find_leaf(key);
        delete_from_leaf(leaf, key, value);
        save_to_file();
    }
    
    std::vector<int> find(const std::string& key) {
        std::vector<int> result;
        if (!root) return result;
        
        BPTreeNode* leaf = find_leaf(key);
        
        for (int i = 0; i < leaf->key_count; i++) {
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