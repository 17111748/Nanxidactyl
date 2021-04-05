#include "cache.h"
#include "vector"
#include <cstdint>
#include <map>

// Buffer Sizes 
#define SPEC_BUFFER_SIZE = 30
#define MSI_BUFFER_SIZE = 30

// Constructor
Line::Line(cache_states state, uint8_t tag, uint8_t valid, uint8_t dirty, uint32_t data, uint64_t time_inserted) {
    state = state;
    tag = tag;
    valid = valid;
    dirty = dirty;
    data = data;
    time_inserted = time_inserted; 
}

// Constructor
Set::Set(std::vector<Line> lines, uint64_t num_lines, uint8_t setID) {
    lines = lines;	
    num_lines = num_lines; 
    setID = setID: 
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


// Constructor
Cache::Cache(uint64_t set_associativity, uint64_t num_sets) {

    cache_stats = Cache_stat(); 
    sets = std::map<uint64_t, Set> {};// Map of index -> Set 
    for (int setID = 0; setID < num_sets; setID++) {
        lines = std::vector<Line>;
        num_lines = set_associativity;
        s = Set(lines, num_lines, setID);
        sets.insert(std::pair<uint64_t, Set>(i, s));    
    }

    set_associativity = set_associativity;
    num_sets = num_sets; 

}


// Cache Helper Functions 
std::map<uint64_t, uint64_t> Cache::address_convert(uint64_t addr) {

}

uint64_t Cache::address_rebuild(uint64_t tag, uint64_t index) {

}










