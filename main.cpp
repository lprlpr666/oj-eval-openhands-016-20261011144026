#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <sstream>

const std::string DB_FILE = "bptree.db";

// Minimal B+ Tree implementation that meets requirements
class BPTree {
private:
    // B+ Tree node structure
    struct Node {
        bool is_leaf;
        std::vector<std::string> keys;
        std::vector<Node*> children;
        std::vector<std::vector<int>> values;  // Only for leaves
        Node* next;
        
        Node(bool leaf = false) : is_leaf(leaf), next(nullptr) {}
        ~Node() {
            for (auto child : children) delete child;
        }
    };
    
    Node* root;
    const int MAX_KEYS = 100;  // Large enough for test cases
    
    // Simple file persistence
    void save_tree() {
        std::ofstream file(DB_FILE);
        if (!file.is_open()) return;
        
        save_node(file, root);
        file.close();
    }
    
    void save_node(std::ofstream& file, Node* node) {
        if (!node) return;
        
        file << node->is_leaf << " " << node->keys.size();
        for (auto& key : node->keys) {
            file << " " << key;
        }
        
        if (node->is_leaf) {
            for (auto& vals : node->values) {
                file << " " << vals.size();
                for (int v : vals) file << " " << v;
            }
        }
        
        file << "\n";
        
        if (!node->is_leaf) {
            for (auto child : node->children) {
                save_node(file, child);
            }
        }
    }
    
    void load_tree() {
        std::ifstream file(DB_FILE);
        if (!file.is_open()) return;
        
        root = load_node(file);
        file.close();
    }
    
    Node* load_node(std::ifstream& file) {
        std::string line;
        if (!std::getline(file, line)) return nullptr;
        
        Node* node = new Node();
        std::istringstream iss(line);
        
        int key_count;
        iss >> node->is_leaf >> key_count;
        
        for (int i = 0; i < key_count; i++) {
            std::string key;
            iss >> key;
            node->keys.push_back(key);
        }
        
        if (node->is_leaf) {
            for (int i = 0; i < key_count; i++) {
                int val_count;
                iss >> val_count;
                std::vector<int> vals;
                for (int j = 0; j < val_count; j++) {
                    int v;
                    iss >> v;
                    vals.push_back(v);
                }
                node->values.push_back(vals);
            }
        } else {
            for (int i = 0; i <= key_count; i++) {
                Node* child = load_node(file);
                node->children.push_back(child);
            }
        }
        
        return node;
    }
    
    Node* find_leaf(const std::string& key) {
        if (!root) return nullptr;
        
        Node* curr = root;
        while (!curr->is_leaf) {
            int i = 0;
            while (i < (int)curr->keys.size() && key >= curr->keys[i]) i++;
            curr = curr->children[i];
        }
        return curr;
    }
    
    void insert_into_leaf(Node* leaf, const std::string& key, int value) {
        // Find key position
        int pos = 0;
        while (pos < (int)leaf->keys.size() && leaf->keys[pos] < key) pos++;
        
        if (pos < (int)leaf->keys.size() && leaf->keys[pos] == key) {
            // Key exists, add value
            auto& vals = leaf->values[pos];
            if (std::find(vals.begin(), vals.end(), value) == vals.end()) {
                vals.push_back(value);
                std::sort(vals.begin(), vals.end());
            }
        } else {
            // Insert new key
            leaf->keys.insert(leaf->keys.begin() + pos, key);
            leaf->values.insert(leaf->values.begin() + pos, std::vector<int>{value});
        }
    }
    
    void split_leaf(Node* leaf) {
        Node* new_leaf = new Node(true);
        new_leaf->next = leaf->next;
        leaf->next = new_leaf;
        
        int mid = leaf->keys.size() / 2;
        
        // Move half to new leaf
        for (int i = mid; i < (int)leaf->keys.size(); i++) {
            new_leaf->keys.push_back(leaf->keys[i]);
            new_leaf->values.push_back(leaf->values[i]);
        }
        
        leaf->keys.resize(mid);
        leaf->values.resize(mid);
        
        // Insert separator into parent
        insert_into_internal(leaf, new_leaf, new_leaf->keys[0]);
    }
    
    void insert_into_internal(Node* left, Node* right, const std::string& key) {
        if (left == root) {
            // New root
            Node* new_root = new Node(false);
            new_root->keys.push_back(key);
            new_root->children.push_back(left);
            new_root->children.push_back(right);
            root = new_root;
            return;
        }
        
        // Find parent and insert
        Node* parent = find_parent(root, left);
        if (!parent) return;
        
        int pos = 0;
        while (pos < (int)parent->children.size() && parent->children[pos] != left) pos++;
        pos++;
        
        parent->keys.insert(parent->keys.begin() + pos - 1, key);
        parent->children.insert(parent->children.begin() + pos, right);
        
        if (parent->keys.size() > MAX_KEYS) {
            split_internal(parent);
        }
    }
    
    void split_internal(Node* node) {
        Node* new_node = new Node(false);
        int mid = node->keys.size() / 2;
        std::string split_key = node->keys[mid];
        
        // Move keys and children
        for (int i = mid + 1; i < (int)node->keys.size(); i++) {
            new_node->keys.push_back(node->keys[i]);
        }
        for (int i = mid + 1; i < (int)node->children.size(); i++) {
            new_node->children.push_back(node->children[i]);
        }
        
        node->keys.resize(mid);
        node->children.resize(mid + 1);
        
        insert_into_internal(node, new_node, split_key);
    }
    
    Node* find_parent(Node* curr, Node* child) {
        if (curr->is_leaf) return nullptr;
        
        for (auto c : curr->children) {
            if (c == child) return curr;
            Node* parent = find_parent(c, child);
            if (parent) return parent;
        }
        return nullptr;
    }
    
public:
    BPTree() : root(nullptr) {
        load_tree();
    }
    
    ~BPTree() {
        save_tree();
        delete root;
    }
    
    void insert(const std::string& key, int value) {
        if (!root) {
            root = new Node(true);
        }
        
        Node* leaf = find_leaf(key);
        insert_into_leaf(leaf, key, value);
        
        if (leaf->keys.size() > MAX_KEYS) {
            split_leaf(leaf);
        }
        
        save_tree();
    }
    
    void remove(const std::string& key, int value) {
        if (!root) return;
        
        Node* leaf = find_leaf(key);
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
        
        save_tree();
    }
    
    std::vector<int> find(const std::string& key) {
        std::vector<int> result;
        if (!root) return result;
        
        Node* leaf = find_leaf(key);
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