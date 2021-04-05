#include "cache_system.h"
#include <vector>
#include <cstdint>
#include <map>



Cache_system::cache_read(uint8_t coreID, uint64_t addr){
    // check if possible for approximation

    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...>
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state>
    // If ~valid, approximate/speculate with the old data if allowed
}

Cache_system::cache_write(uint8_t coreID, uint64_t addr){
    
}





Cache_system::cache_read(uint8_t coreID, uint64_t addr){

    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...>
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)

    // If address is in the cache (M,S), Use valid data
        // Return the data 
        // Remain in the same state 

    // If address is in the cache (V), approximate/speculate with the old data if allowed 
        // (will always be allowed, only in this state if approximateble)
        // Return the invalid data and create a speculative entry in the speculative buffer that stores the time_of_speculation
        // Remain in the same state 

    // If address is in the cache (I), fetch data from llc, assume a write-through cache
        // If llc contains the 
        // insert fake latency -- entry into update buffer to update to S state after n cycles

    // If address is not in the cache, fetch data from llc, assume a write-through cache
        // insert fake latency -- entry into update buffer to update to S state after n cycles
    

}

bool Cache_system::speculative_compare(uint32_t original_data, uint32_t new_data, uint64_t threshold){

}

void Cache_system::speculative_rollback(Line cache_line, uint64_t addr, uint32_t data, uint64_t cur_cycles){

}