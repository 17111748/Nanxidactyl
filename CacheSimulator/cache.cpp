#include "cache.h"
#include "vector"
#include <cstdint>
#include <map>
#include <cmath>
#include <iostream>


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
    state = INVALID; 
    tag = 0;
    data = 0; 
    time_accessed = 0;  
}

// Constructor
Line::Line(cache_states state_param, uint64_t tag_param, uint32_t data_param, uint64_t time_accessed_param) {
    state = state_param;
    tag = tag_param;
    data = data_param;
    time_accessed = time_accessed_param; 
}

Set::Set() {
    setID = 0; 
}

// Constructor
Set::Set(std::vector<Line> lines_param, uint64_t num_lines_param, uint8_t setID_param) {
    lines = lines_param;	
    num_lines = num_lines_param; 
    setID = setID_param;
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
Cache::Cache(uint64_t set_associativity_param, uint64_t num_sets_param) {

    cache_stats = Cache_stat(); 
    sets = map<uint64_t, Set> {};// Map of index -> Set 
    for (uint64_t setID = 0; setID < num_sets_param; setID++) {
        // vector<Line> lines(set_associativity_param, Line());
        vector<Line> lines;
        uint64_t num_lines = set_associativity_param;
        Set s = Set(lines, num_lines, setID);
        sets.insert(pair<uint64_t, Set>(setID, s));    
    }

    // std::cout << "in cache " << unsigned(set_associativity) << endl; 
    set_associativity = set_associativity_param;
    num_sets = num_sets_param; 

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








