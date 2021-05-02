#include <vector>
#include <cstdint>
#include <iostream> 
#include <cmath>


// Memory Operations
#define READ_HIT_CYCLES 1
#define READ_MISS_CYCLES 2
#define WRITE_HIT_CYCLES 1
#define WRITE_MISS_CYCLES 2
#define READ_TO_MEMORY_CYCLES 100
#define WRITE_TO_MEMORY_CYCLES 100

// // Buffer Sizes 
#define NUM_BLOCKS 32
#define BLOCK_SIZE 5
#define ADDR_SIZE 64

// // L1 Cache parameters 
// #define L1_SET_ASSOCIATIVITY 8
// #define L1_NUM_SETS 32
// // LLC Cache parameters
// #define LLC_SET_ASSOCIATIVITY 8
// #define LLC_NUM_SETS 64

// #define NUM_BLOCKS 32
// #define BLOCK_SIZE 5
// #define ADDR_SIZE 64

#define L1_SET_ASSOCIATIVITY 8
#define L1_NUM_SETS 64
#define LLC_SET_ASSOCIATIVITY 8
#define LLC_NUM_SETS 256

// Different Cache States for the cache coherence protocol 
enum cache_states {INVALID, SHARED, VICTIMIZED, MODIFIED}; 



class Line {

    public: 
        cache_states state;
        uint64_t tag;
        std::vector<uint64_t> data; 
        uint64_t time_accessed; // This is for LRU Replacement Policy 
        Line() {
            state = INVALID; 
            tag = 0;
            data = std::vector<uint64_t>(); 
            time_accessed = 0; 
        }; 
        Line(cache_states state, uint64_t tag, uint64_t data, uint64_t time_accessed, uint8_t block_index) {
            this->state = state; 
            this->tag = tag; 
            this->data = std::vector<uint64_t>(NUM_BLOCKS/8, 0); 
            this->data[block_index] = data;
            this->time_accessed = time_accessed;
        };

};

class Set {

    public: 
        std::vector<Line> lines;	
        uint64_t num_lines; 
        Set() {
            this->num_lines = 0;
            this->lines = std::vector<Line>();
        }; 
        Set(std::vector<Line> lines, uint64_t num_lines) {
            this->lines = lines;	
            this->num_lines = num_lines; 
        }; 

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
        Cache_stat() {
            this->num_access = 0; 
            this->num_reads_hits = 0; 
            this->num_writes_hits = 0; 
            this->num_read_misses = 0; 
            this->num_write_misses = 0;
            this->num_read_from_llc = 0; // read: line not in cache or no M/S version in other caches
            this->num_write_to_llc = 0;
        }; 
};


// Cache is essentially a core 
class Cache {

    public: 
        Cache_stat cache_stats; 
        std::vector<Set> sets;

        uint64_t set_associativity;
        uint64_t num_sets;

        Cache() {
            this->num_sets = 0; 
            this->set_associativity = 1;
        }; 
        Cache(uint64_t set_associativity, uint64_t num_sets){
            this->cache_stats = Cache_stat(); 
            this->sets = std::vector<Set>();// Map of index -> Set 
            for (uint64_t setID = 0; setID < num_sets; setID++) {
                // vector<Line> lines(set_associativity_param, Line());
                std::vector<Line> lines;
                uint64_t num_lines = set_associativity;
                Set s = Set(lines, num_lines);
                this->sets.push_back(s);    
            }

            this->set_associativity = set_associativity;
            this->num_sets = num_sets; 
        }; 


        // Methods: < Tag, Index > 
        std::vector<uint64_t> address_convert(uint64_t addr) {
            // printf("Address Convert: Start\n"); 
            uint8_t index_length = log2(num_sets); 
            uint8_t tag_length = ADDR_SIZE - BLOCK_SIZE - index_length; 

            uint64_t tag = addr >> (ADDR_SIZE - tag_length); 
            uint64_t index = (addr << tag_length) >> (tag_length + BLOCK_SIZE); 
            uint64_t block = addr << (ADDR_SIZE - BLOCK_SIZE) >> (ADDR_SIZE - BLOCK_SIZE);

            uint64_t block_index = block/8; 

            std::vector<uint64_t> result; 
            result.push_back(tag); 
            result.push_back(index); 
            result.push_back(block_index); 
            // printf("Address Convert: Tag: %li, Set: %li, Block: %li\n", tag, index, block);
            return result; 

        };

};















