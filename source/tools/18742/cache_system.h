#include "cache.h"
#include "llc.h"

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


// Different Cache States for the cache coherence protocol 
enum cache_states {INVALID, SHARED, VICTIMIZED, MODIFIED}; 

enum bus_actions {INVALIDATE, BUSRD, BUSRDOWN, SPECULATE}; 

enum cache_actions {READ, WRITE}; 


// Magic Memory Address Range index by 2
class Magic_memory {

    public: 
        uint64_t size; 
        uint64_t *addresses; 
        Magic_memory(uint64_t *addresses); 

} ; 

// The cache system acts as the controller 
class Cache_system {

    public: 
        Magic_memory magic_memory; 
        uint8_t  num_cores; 
        Cache    *caches;
        LLC      llc; 
        uint64_t *cache_cycles; // To store the local clock of each core
        Cache_system(Magic_memory magic_memory, uint8_t num_cores, Cache *caches, LLC llc); 

        void start_simulation(); 
        

       
    private: 
        // Controller
        void initialize_system(); 
        bool approximatable_address(uint64_t ADDR); // Checks magic memory 
        void cache_bus_actions(Cache cache, cache_states new_cache_state, bus_actions bus_action);


};