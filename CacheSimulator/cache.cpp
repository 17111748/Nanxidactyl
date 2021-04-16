#include "cache.h"
#include "vector"
#include <cstdint>
#include <map>
#include <cmath>

// Buffer Sizes 
#define BLOCK_SIZE 5
#define ADDR_SIZE 64

// L1 Cache parameters 
#define L1_SET_ASSOCIATIVITY 8
#define L1_NUM_SETS 32
// LLC Cache parameters
#define LLC_SET_ASSOCIATIVITY 8
#define LLC_NUM_SETS 32

using namespace std; 

Line::Line() {
    tag = 0; 
}

// Constructor
Line::Line(cache_states state, uint8_t tag, uint32_t data, uint64_t time_accessed) {
    state = state;
    tag = tag;
    data = data;
    time_accessed = time_accessed; 
}

Set::Set() {
    setID = 0; 
}

// Constructor
Set::Set(std::vector<Line> lines, uint64_t num_lines, uint8_t setID) {
    lines = lines;	
    num_lines = num_lines; 
    setID = setID;
}

// Constructor 
Cache_stat::Cache_stat() {
    num_access = 0; 
    num_reads = 0; 
    num_writes = 0; 
    num_read_misses = 0; 
    num_write_misses = 0; 
    num_write_backs = 0; 
    num_blocks_transferred = 0; 
}

Cache::Cache() {
    num_sets = 0; 
}

// Constructor
Cache::Cache(uint64_t set_associativity, uint64_t num_sets) {

    cache_stats = Cache_stat(); 
    sets = map<uint64_t, Set> {};// Map of index -> Set 
    for (int setID = 0; setID < num_sets; setID++) {
        vector<Line> lines;
        uint64_t num_lines = set_associativity;
        Set s = Set(lines, num_lines, setID);
        sets.insert(pair<uint64_t, Set>(setID, s));    
    }

    set_associativity = set_associativity;
    num_sets = num_sets; 

}

// TODO: CHECK THESE FUNCTIONS 
// Cache Helper Functions 
std::pair<uint64_t, uint64_t> Cache::address_convert(uint64_t addr) {
    uint8_t index_length = log2(num_sets); 
    uint8_t tag_length = ADDR_SIZE - BLOCK_SIZE - index_length; 

    uint64_t tag = addr >> (ADDR_SIZE - tag_length); 
    uint64_t index = (addr << tag_length) >> (tag_length + BLOCK_SIZE); 

    return std::make_pair(tag, index); 
}


// int main() {
//     return 0; 
// }








