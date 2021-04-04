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
Spec_buffer_entry::Spec_buffer_entry(uint64_t addr, Line cache_line, uint64_t time_of_speculation) {
    addr = addr;
    cache_line = cache_line;
    time_of_speculation = time_of_speculation;
}

// Constructor 
Spec_buffer::Spec_buffer(uint64_t size) {
    full = false;
    size = size; 
    spec_buffer_entries = std::vector<Spec_buffer_entry>; 
}

// Constructor 
Buffer_entry::Buffer_entry(uint8_t cache_dest, uint64_t addr, Line cache_line, bus_actions bus_action, uint64_t time_of_action) {
    cache_dest = cache_dest; 
    addr = addr; 
    cache_line = cache_line; 
    bus_action = bus_action; 
    time_of_action = time_of_action; 
}

// Constructor
MSI_buffer::MSI_buffer(uint64_t size) {
    full = false; 
    size = size; 
    msi_buffer_entries = std::vector<MSI_buffer_entry>; 
}


// Constructor
Cache::Cache(uint64_t set_associativity, uint64_t num_sets) {

    cycles = 0; 
    uint64_t spec_buffer_size = SPEC_BUFFER_SIZE; 
    uint64_t msi_buffer_size = MSI_BUFFER_SIZE; 

    cache_stats = Cache_stat(); 
    sets = std::map<uint64_t, Set> {};// Map of index -> Set 
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

}



// Spec_buffer Methods
void Spec_buffer::insert_entry(Spec_buffer_entry entry) {

}

Spec_buffer_entry Spec_buffer::buffer_search(uint64_t addr) {

}

// MSI_buffer Methods 
void MSI_buffer::insert_entry(MSI_buffer_entry entry) {

}

MSI_buffer_entry MSI_buffer::buffer_search(uint64_t addr) {

}

// Cache Helper Functions 
std::map<uint64_t, uint64_t> Cache::address_convert(uint64_t addr) {

}

uint64_t Cache::address_rebuild(uint64_t tag, uint64_t index) {

}

void Cache::cache_increment_cycle(uint64_t cycle) {
    cycles += cycle; 
}








// Cache Methods 
uint32_t Cache::cache_read(uint64_t addr) {

}

void Cache::cache_write(uint64_t addr, uint32_t data) {

}

uint32_t Cache::cache_evict(uint64_t addr) {
    
}

// Temporary Extra 
uint8_t Cache::cache_search(uint64_t tag, uint64_t index, uint32_t *way_num) {

}

void Cache::cache_replace(uint64_t index, uint32_t way_num, block blk) {

}