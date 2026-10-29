#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <chrono>
#include <cstdint>

using namespace std;

// bit mask for 192 bits (up to 192 fragments)
struct Mask {
    uint64_t m[3] = {0, 0, 0};
    
    void set(int i) { m[i >> 6] |= (1ULL << (i & 63)); }
    bool test(int i) const { return (m[i >> 6] >> (i & 63)) & 1; }
    
    bool operator==(const Mask& o) const {
        return m[0] == o.m[0] && m[1] == o.m[1] && m[2] == o.m[2];
    }
};

// used for memoization: current node + mask
struct State {
    int node;
    Mask mask;
    
    bool operator==(const State& o) const {
        return node == o.node && mask == o.mask;
    }
};

// Xorshift hash function
struct StateHash {
    size_t operator()(const State& s) const {
        size_t h = s.node;
        for (int i = 0; i < 3; ++i) {
            h ^= s.mask.m[i] + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        }
        return h;
    }
};

// Fixed cache entry (occupies exactly 32 bytes)
struct CacheEntry {
    State state;
    uint16_t res;
    bool valid = false;
};

// Cache size per thread: 2^22 = 4,194,304 elements (~134 МБ RAM). Used constexpr.
constexpr size_t CACHE_BITS = 22; 
constexpr size_t CACHE_SIZE = 1ULL << CACHE_BITS;
constexpr size_t CACHE_MASK = CACHE_SIZE - 1;

// Worker thread class
struct Worker {
    vector<CacheEntry> cache;
    const vector<vector<int>>& adj;

    Worker(const vector<vector<int>>& adjacency) : adj(adjacency) {
        cache.resize(CACHE_SIZE);
    }

    // Recursive DFS, returns: (path_length << 8) | next_node
    // Pass mask by const reference for efficiency
    uint16_t dfs(int curr, const Mask& mask) {
        State state{curr, mask};
        size_t idx = StateHash()(state) & CACHE_MASK;
        
        if (cache[idx].valid && cache[idx].state == state) {
            return cache[idx].res;
        }

        uint8_t max_len = 0;
        uint8_t best_next = 255; // 255 means path end of path

        for (int nxt : adj[curr]) {
            if (!mask.test(nxt)) {
                Mask next_mask = mask; // Copying only when going deeper
                next_mask.set(nxt);
                uint16_t res = dfs(nxt, next_mask);
                uint8_t l = res >> 8;
                if (l > max_len) {
                    max_len = l;
                    best_next = nxt;
                }
            }
        }

        uint16_t final_res = ((max_len + 1) << 8) | best_next;
        cache[idx].state = state;
        cache[idx].res = final_res;
        cache[idx].valid = true;
        return final_res;
    }

    // Path reconstruction (cache-overwrite safe)
    vector<int> reconstruct(int start_node) {
        vector<int> path;
        int curr = start_node;
        Mask mask;
        mask.set(curr);
        path.push_back(curr);

        while (true) {
            State state{curr, mask};
            size_t idx = StateHash()(state) & CACHE_MASK;
            uint16_t res;
            
            if (cache[idx].valid && cache[idx].state == state) {
                res = cache[idx].res;
            } else {
                res = dfs(curr, mask);
            }

            int nxt = res & 0xFF;
            if (nxt == 255) break;

            curr = nxt;
            mask.set(curr);
            path.push_back(curr);
        }
        return path;
    }
};

int main() {
    ifstream file("data.txt");
    if (!file.is_open()) {
        cerr << "Error, file 'data.txt' not found" << endl;
        return 1;
    }

    vector<string> fragments;
    string line;
    while (file >> line) {
        // Guard against empty or too short strings
        if (line.length() >= 2) {
            fragments.push_back(line);
        }
    }
    file.close();

    int n = fragments.size();
    if (n == 0) return 0;
    if (n > 192) {
        cerr << "Error: algorithm was optimised for max. 192 fragments." << endl;
        return 1;
    }

    // Graph construction
    vector<vector<int>> adj(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j && fragments[i].substr(fragments[i].length() - 2) == fragments[j].substr(0, 2)) {
                adj[i].push_back(j);
            }
        }
    }

    int num_threads = thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 8;
    cout << "C++ Multi-Thread Engine launched (" << num_threads << " threads)\n";
    cout << "Graph size: " << n << " fragments\n\n";

    atomic<int> current_task(0);
    atomic<int> completed_tasks(0);
    mutex result_mtx;
    vector<int> global_best_path;

    auto start_time = chrono::high_resolution_clock::now();

    // Function executed by every thread
    auto thread_func = [&]() {
        Worker worker(adj);
        
        while (true) {
            int start_idx = current_task.fetch_add(1);
            if (start_idx >= n) break;

            Mask initial_mask;
            initial_mask.set(start_idx);
            
            // 1. Longest path search
            worker.dfs(start_idx, initial_mask);
            
            // 2. Specific index sequence reconstruction
            vector<int> path = worker.reconstruct(start_idx);
            
            // 3. Result and progress synchronization
            {
                lock_guard<mutex> lock(result_mtx);
                if (path.size() > global_best_path.size()) {
                    global_best_path = std::move(path);
                }
                
                completed_tasks++;
                int comp = completed_tasks.load();
                int total_stars = 30;
                int filled = (comp * total_stars) / n;
                int percent = (comp * 100) / n;
                
                cout << "\rProgress: [";
                for (int i = 0; i < filled; ++i) cout << "*";
                for (int i = filled; i < total_stars; ++i) cout << ".";
                cout << "] " << percent << "% (" << comp << "/" << n << ")" << flush;
            }
        }
    };

    // Launching thread pool
    vector<thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_func);
    }
    for (auto& t : threads) {
        t.join();
    }

    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double> diff = end_time - start_time;

    // String concatenation
    cout << "\n\nComputation completed in " << diff.count() << " seconds\n";
    
    string result = fragments[global_best_path[0]];
    for (size_t i = 1; i < global_best_path.size(); ++i) {
        result += fragments[global_best_path[i]].substr(2);
    }

    cout << "Fragments used: " << global_best_path.size() << " out of " << n << "\n";
    cout << "Final string length: " << result.length() << "\n\n";
    cout << "Result:\n" << result << endl;

    return 0;
}
