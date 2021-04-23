#include <vector>
#include <cstdint>
#include <map>

// // Non-Memory Operations
// #define ADD_CYCLES 1
// #define MULTIPLY_CYCLES 2
// #define DIVIDE_CYCLES 3


// // Memory Operations
// #define READ_HIT_CYCLES 1
// #define READ_MISS_CYCLES 2
// #define WRITE_HIT_CYCLES 1
// #define WRITE_MISS_CYCLES 2
// #define READ_TO_MEMORY_CYCLES 100
// #define WRITE_TO_MEMORY_CYCLES 100


// Different Cache States for the cache coherence protocol 
enum cache_states {INVALID, SHARED, VICTIMIZED, MODIFIED}; 



class Line {

    public: 
        cache_states state;
        uint64_t tag;
        uint32_t data; 
        uint64_t time_accessed; // This is for LRU Replacement Policy 
        Line(); 
        Line(cache_states state, uint64_t tag, uint32_t data, uint64_t time_accessed);

};

class Set {

    public: 
        std::vector<Line> lines;	
        uint64_t num_lines; 
        uint8_t setID; 
        Set(); 
        Set(std::vector<Line> lines, uint64_t num_lines, uint8_t setID); 

}; 

class Cache_stat {

    public: 
        uint64_t num_access; 
        uint64_t num_reads_hits; 
        uint64_t num_writes_hits; 
        uint64_t num_read_misses; 
        uint64_t num_write_misses;
        uint64_t num_read_from_llc; // read: line not in cache or no M/S version in other caches
        uint64_t num_write_to_llc; 
        Cache_stat(); 
};


// Cache is essentially a core 
class Cache {

    public: 
        Cache_stat cache_stats; 
        std::map<uint64_t, Set> sets;

        uint64_t set_associativity;
        uint64_t num_sets;

        Cache(); 
        Cache(uint64_t set_associativity, uint64_t num_sets); 


        // Methods: < Tag, Index > 
        std::vector<uint64_t> address_convert(uint64_t addr);

};















