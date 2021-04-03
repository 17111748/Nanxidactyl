#include "cache.h"
#include <cstdint>
#include <vector>


// Non-Memory Operations
#define ADD_CYCLES 1
#define MULTIPLY_CYCLES 2
#define DIVIDE_CYCLES 3


// Memory Operations
#define READ_HIT_CYCLES 1
#define READ_MISS_CYCLES 2
#define WRITE_HIT_CYCLES 1
#define WRITE_MISS_CYCLES 2
#define READ_TO_MEMORY_CYCLES 100
#define WRITE_TO_MEMORY_CYCLES 100 

// L1 Cache parameters 
#define L1_SET_ASSOCIATIVITY 8
#define L1_NUM_SETS 32
// LLC Cache parameters
#define LLC_SET_ASSOCIATIVITY 8
#define LLC_NUM_SETS 32

// Different Cache States for the cache coherence protocol 
enum cache_states {INVALID, SHARED, VICTIMIZED, MODIFIED}; 

enum bus_actions {INVALIDATE, BUSRD, BUSRDOWN, SPECULATE}; 

enum cache_actions {READ, WRITE}; 


// Magic Memory Address Range index by 2
// Allowed Addresses to perform Victimized Protocol
// Address booking: keeps track of the address ranges that can tolerate approximation
class Magic_memory {
    public: 
        // Pair of start-inclusive, end-exclusive addresses which are allowed to perform protocol
        std::vector<std::pair<uint64_t, uint64_t>> addresses; 
        bool check_address(uint64_t address) {
            for (auto addr_pair = addresses.begin(); addr_pair != addresses.end(); ++addr_pair) {
                if(address >= (*addr_pair).a && address < (*addr_pair).b) {
                    return true;
                }
            }
            return false; // Address not allowed

            // This might also work... can try running it in make 
            // for (unsigned i = 0; i < addresses.size(); ++i) {
            //     if(address >= addresses[i].a && address < addresses[i].b) {
            //         return true; 
            //     }
            // }
            // return false; 
        }
        Magic_memory(std::vector<std::pair<uint64_t, uint64_t>> addresses) {addresses = addresses;} 
} ; 

// The cache system acts as the controller 
class Cache_system {
    public: 
        Magic_memory magic_memory; 
        uint8_t  num_cores; 
        std::map<uint8_t, Cache>    caches;
        std::map<uint8_t, uint64_t> cache_cycles; // To store the local clock of each core
        Cache      llc; 

        Cache_system(Magic_memory magic_memory, uint8_t num_cores) {
            magic_memory = magic_memory;
            num_cores = num_cores;
            
            caches = std::map<uint8_t, uint64_t> {};
            cache_cycles = std::map<uint8_t, uint64_t> {};
            for (unsigned int i = 0; i < num_cores; i++) {
                caches[i] = Cache(L1_SET_ASSOCIATIVITY, L1_NUM_SETS);
                cache_cycles[i] = 0;
            }
            llc = Cache(LLC_SET_ASSOCIATIVITY, LLC_NUM_SETS);
        }

        uint32_t cache_read(uint8_t coreID, uint64_t ADDR);
        void cache_write(uint8_t coreID, uint64_t ADDR, uint32_t data);
       
    private: 
        // Controller
        void initialize_system(); 
        void cache_bus_actions(Cache cache, cache_states new_cache_state, bus_actions bus_action);
};




// // TODO: FIGURE OUT WHICH FUNCTIONS BELONG WHERE ... COMMUNICATION BTWN SYSTEM
// // Actions 
// uint32_t cache_read(uint64_t ADDR);
// void cache_write(uint64_t ADDR, uint32_t data);
// uint32_t cache_evict(uint64_t ADDR);

// // Temporary Extra 
// uint8_t cache_search(uint64_t tag, uint64_t index, uint32_t *way_num);
// void cache_replace(uint64_t index, uint32_t way_num, block blk);

// // Speculative Buffer

// // MSI Protocol Buffer

// // Speculative Execution Functions (NEED EDIT)
// //bool speculative_compare(Cache c_original, Cache c_new, uint64_t threshold); // Compare the data and determine whether it is approximately close 
// //void speculative_rollback(Line cache_line, uint64_t addr, uint32_t data, uint64_t cur_cycles); // Just add a certain number of cycles and replace the cache line 
