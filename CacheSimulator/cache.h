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

// Buffer Sizes 
#define SPEC_BUFFER_SIZE = 30
#define MSI_BUFFER_SIZE = 30


// Different Cache States for the cache coherence protocol 
enum cache_states {INVALID, SHARED, VICTIMIZED, MODIFIED}; 

enum bus_actions {INVALIDATE, BUSRD, BUSRDOWN, SPECULATE}; 

enum cache_actions {READ, WRITE}; 


class Line {

    public: 
        cache_states state;
        uint8_t tag;
        uint8_t valid;
        uint8_t dirty;
        uint32_t data; 
        uint64_t time_accessed; // This is for LRU Replacement Policy 
        Line(cache_states state, uint8_t tag, uint8_t valid, uint8_t dirty, uint32_t data, uint64_t time_inserted);

};

class Set {

    public: 
        std::vector<Line> lines;	
        uint64_t num_lines; 
        uint8_t setID; 
        Set(std::vector<Line> lines, uint64_t num_lines, uint8_t setID); 

}; 



// Cache is essentially a core 
class Cache {

    public: 
        Cache_stat cache_stats; 
        std::map<uint64_t, Set> sets;

        uint64_t set_associativity;
        uint64_t num_sets;

        Cache(uint64_t set_associativity, uint64_t num_sets); 


        // Methods 
        std::map<uint64_t, uint64_t> address_convert(uint64_t addr);
        uint64_t address_rebuild(uint64_t tag, uint64_t index);

};















