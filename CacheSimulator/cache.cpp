#include "cache.h"
#include "vector"
#include <cstdint>

// Constructor
Line::Line(cache_states state, uint8_t tag, uint8_t valid, uint8_t dirty, uint32_t data) {
    state = state;
    tag = tag;
    valid = valid;
    dirty = dirty;
    data = data;
}


// Constructor
Set::Set(Line *lines, uint8_t num_lines) {
    lines = lines;	
    num_lines = num_lines;	
}

Cache_stat::Cache_stat() {
    num_access = 0; 
    num_reads = 0; 
    num_writes = 0; 
    num_read_misses = 0; 
    num_write_misses = 0; 
    num_write_backs = 0; 
    num_blocks_transferred = 0; 
}

Spec_buffer_entry::Spec_buffer_entry(uint64_t addr, Line cache_line, uint64_t time_of_speculation) {
    addr = addr;
    cache_line = cache_line;
    time_of_speculation = time_of_speculation;
}

Spec_buffer::Spec_buffer(uint64_t size) {
    size = size;
    full = false;
    spec_buffer_entries = null; //???
    
    

}

// Constructor
Cache::Cache(uint64_t set_associativity, uint64_t num_sets, uint64_t num_bytes) {

    cycles = 0; 

    cache_stats = Cache_stat(); // Map of index -> Set
    sets = std::map<uint64_t, Set> {};// Map of index -> Set ;
    for (int setID = 0; setID < num_sets; setID++) {
        lines = std::vector<Line>;
        num_lines = set_associativity;
        s = Set(lines, num_lines, setID);
        sets.insert(std::pair<uint64_t, Set>(i, s));    
    }
    

    spec_buffer = Spec_buffer(spec_buffer_size); 
    msi_buffer = MSI_buffer(msi_buffer_size); 

    set_associativity = set_associativity;
    num_sets = num_sets; 
    num_bytes = num_bytes; 

}


// Cache Methods 
uint32_t Cache::cache_read(uint64_t ADDR) {

}

void Cache::cache_write(uint64_t ADDR, uint32_t data) {

}

uint32_t Cache::cache_evict(uint64_t ADDR) {
    
}

// Temporary Extra 
uint8_t Cache::cache_search(uint64_t tag, uint64_t index, uint32_t *way_num) {

}

void Cache::cache_replace(uint64_t index, uint32_t way_num, block blk) {

}