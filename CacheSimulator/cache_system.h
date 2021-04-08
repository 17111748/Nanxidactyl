#include "cache.h"
#include <vector>
#include <cstdint>
#include <map>


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
        }
        Magic_memory(std::vector<std::pair<uint64_t, uint64_t>> addresses) {addresses = addresses;} 
} ; 

// The cache system acts as the controller 
class Cache_system {
    public: 
        uint64_t global_time; // For the LRU policy
        Magic_memory magic_memory; 
        uint8_t  num_cores; 
        std::map<uint8_t, Cache>    caches;
        Cache      llc; 

        Cache_system(Magic_memory magic_memory, uint8_t num_cores) {
            global_time = 0; 
            magic_memory = magic_memory;
            num_cores = num_cores;
            
            caches = std::map<uint8_t, uint64_t> {};
            for (unsigned int i = 0; i < num_cores; i++) {
                caches[i] = Cache(L1_SET_ASSOCIATIVITY, L1_NUM_SETS);
            }
            llc = Cache(LLC_SET_ASSOCIATIVITY, LLC_NUM_SETS);
        }

        std::tuple<bool, uint32_t, uint32_t> cache_read(uint8_t coreID, uint64_t addr);
        void cache_write(uint8_t coreID, uint64_t addr, uint32_t data);

        std::pair<bool, Line> lookup_line(uint64_t addr, uint8_t core_index, bool is_llc); 
        void update_llc(uint64_t addr, uint32_t data); 
};


