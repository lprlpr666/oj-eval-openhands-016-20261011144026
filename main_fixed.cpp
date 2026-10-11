#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>

const std::string DB_FILE = "bptree.db";

// B+ Tree implementation that actually uses tree structure
class BPTree {
private:
    struct TreeNode {
        bool is_leaf;
        std::vector<std::string> keys;
        std::vector<TreeNode*> children;
        std::vector<std::vector<int>> values;  // Only for leaf nodes
        TreeNode* next;
        TreeNode* parent;
        
        TreeNode(bool leaf = false) : is_leaf(leaf), next(nullptr), parent(nullptr) {}
        
        ~TreeNode() {
            for (auto child : children) {
                delete child;
            }
        }
    };
    
    TreeNode* root;
    std::string filename;
    const int MAX_KEYS = 3;  // Small for simplicity
    
    void save_to_file_recursive(std::ofstream& file, TreeNode* node) {
        if (!node) return;
        
        file << node->is_leaf << " " << node->keys.size();
        for (const auto& key : node->keys) {
            file << " " << key;
        }
        
        if (node->is_leaf) {
            for (const auto& val_list : node->values) {
                file << " " << val_list.size();
                for (int val : val_list) {
                    file << " " << val;
                }
            }
        }
        
        file << "\n";
        
        if (!node->is_leaf) {
            for (auto child : node->children) {
                save_to_file_recursive(file, child);
            }
        }
    }
    
    TreeNode* load_from_file_recursive(std::ifstream& file) {
        std::string line;
        if (!std::getline(file, line)) return nullptr;
        
        std::istringstream iss(line);
        TreeNode* node = new TreeNode();
        
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
                    int val;
                    iss >> val;
                    vals.push_back(val);
                }
                node->values.push_back(vals);
            }
        } else {
            for (int i = 0; i <= key_count; i++) {
                TreeNode* child = load_from_file_recursive(file);
                node->children.push_back(child);
                if (child) child->parent = node;
            }
        }
        
        return node;
    }
    
    TreeNode* find_leaf(const std::string& key) {
        if (!root) return nullptr;
        
        TreeNode* current = root;
        while (!current->is_leaf) {
            int i = 0;
            while (i < (int)current->keys.size() && key >= current->keys[i]) {
                i++;
            }
            current = current->children[i];
        }
        
        return current;
    }
    
    void insert_into_leaf(TreeNode* leaf, const std::string& key, int value) {
        // Find if key exists
        int pos = -1;
        for (int i = 0; i < (int)leaf->keys.size(); i++) {
            if (leaf->keys[i] == key) {
                pos = i;
                break;
            }
        }
        
        if (pos != -1) {
            // Key exists, add value if not duplicate
            for (int val : leaf->values[pos]) {
                if (val == value) return; // Duplicate
            }
            leaf->values[pos].push_back(value);
            std::sort(leaf->values[pos].begin(), leaf->values[pos].end());
        } else {
            // Find insertion position
            pos = 0;
            while (pos < (int)leaf->keys.size() && leaf->keys[pos] < key) {
                pos++;
            }
            
            // Insert new key
            leaf->keys.insert(leaf->keys.begin() + pos, key);
            std::vector<int> new_vals = {value};
            leaf->values.insert(leaf->values.begin() + pos, new_vals);
        }
    }
    
    void split_leaf(TreeNode* leaf) {
        TreeNode* new_leaf = new TreeNode(true);
        new_leaf->next = leaf->next;
        leaf->next = new_leaf;
        new_leaf->parent = leaf->parent;
        
        int split_point = leaf->keys.size() / 2;
        
        // Move keys to new leaf
        for (int i = split_point; i < (int)leaf->keys.size(); i++) {
            new_leaf->keys.push_back(leaf->keys[i]);
            new_leaf->values.push_back(leaf->values[i]);
        }
        
        leaf->keys.erase(leaf->keys.begin() + split_point, leaf->keys.end());
        leaf->values.erase(leaf->values.begin() + split_point, leaf->values.end());
        
        // Insert first key of new leaf into parent
        std::string split_key = new_leaf->keys[0];
        insert_into_parent(leaf->parent, leaf, new_leaf, split_key);
    }
    
    void insert_into_parent(TreeNode* parent, TreeNode* left, TreeNode* right, const std::string& key) {
        if (!parent) {
            // Create new root
            TreeNode* new_root = new TreeNode(false);
            new_root->keys.push_back(key);
            new_root->children.push_back(left);
            new_root->children.push_back(right);
            
            left->parent = new_root;
            right->parent = new_root;
            root = new_root;
            return;
        }
        
        // Find insertion position
        int pos = 0;
        while (pos < (int)parent->children.size() && parent->children[pos] != left) {
            pos++;
        }
        pos++;
        
        // Insert key and child
        parent->keys.insert(parent->keys.begin() + pos - 1, key);
        parent->children.insert(parent->children.begin() + pos, right);
        
        // Check if parent needs to split
        if (parent->keys.size() > MAX_KEYS) {
            split_internal(parent);
        }
    }
    
    void split_internal(TreeNode* node) {
        TreeNode* new_node = new TreeNode(false);
        new_node->parent = node->parent;
        
        int split_point = node->keys.size() / 2;
        std::string split_key = node->keys[split_point];
        
        // Move keys and children to new node
        for (int i = split_point + 1; i < (int)node->keys.size(); i++) {
            new_node->keys.push_back(node->keys[i]);
        }
        
        for (int i = split_point + 1; i < (int)node->children.size(); i++) {
            new_node->children.push_back(node->children[i]);
            node->children[i]->parent = new_node;
        }
        
        // Remove moved keys and children from original node
        node->keys.erase(node->keys.begin() + split_point, node->keys.end());
        node->children.erase(node->children.begin() + split_point + 1, node->children.end());
        
        // Insert split key into parent
        insert_into_parent(node->parent, node, new_node, split_key);
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
        
        save_to_file_recursive(file, root);
        file.close();
    }
    
    void load_from_file() {
        std::ifstream file(filename);
        if (!file.is_open()) return;
        
        delete root;
        root = load_from_file_recursive(file);
        file.close();
    }
    
    void insert(const std::string& key, int value) {
        if (!root) {
            root = new TreeNode(true);
        }
        
        TreeNode* leaf = find_leaf(key);
        insert_into_leaf(leaf, key, value);
        
        // Split leaf if necessary
        if (leaf->keys.size() > MAX_KEYS) {
            split_leaf(leaf);
        }
        
        save_to_file();
    }
    
    void remove(const std::string& key, int value) {
        if (!root) return;
        
        TreeNode* leaf = find_leaf(key);
        
        for (int i = 0; i < (int)leaf->keys.size(); i++) {
            if (leaf->keys[i] == key) {
                auto& vals = leaf->values[i];
                auto it = std::find(vals.begin(), vals.end(), value);
                if (it != vals.end()) {
                    vals.erase(it);
                    
                    // If no more values, remove key
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
        
        TreeNode* leaf = find_leaf(key);
        
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