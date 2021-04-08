#include "cache_system.h"
#include <vector>
#include <cstdint>
#include <map>




Cache_system::cache_write(uint8_t coreID, uint64_t addr, uint32_t data){
    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...>
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)

    // If address is in the cache (M), write normally
        // Remain in the same state 

    // If address is in the cache (S, I)
        // Send BusRdX. and change the state of other cores 
        // Upgrade to M state
        // write
        // Change the value in the LLC 

    // If address is not in the cache, 
        // If cache is full 
            // evict something (LRU policy)

        // Write to it
        // Change the value in the LLC 
        // Make it M state 
        // Send BusRdX, and change the state of other cores 

}




// Returns (Flag whether we speculated or not, Real Data, Invalid Data) 
Cache_system::cache_read(uint8_t coreID, uint64_t addr){
    
    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...>
    Cache cache = caches[coreID]; 

    std::map<uint64_t, uint64_t> addr_info = address_convert(addr); 
    uint64_t tag = addr_info.a; 
    uint64_t set_index = addr_info.b; 

    Set set = cache[set_index]; 

    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
    std::pair<bool, Line> line_info = lookup_line(addr, coreID); 
    
    access_llc = true; 
    if (line_info.a) {
        // If address is in the cache (M,S), Use valid data
        // Return the data 
        // Remain in the same state 
        Line line = line_info.b; 
        if (line.state == MODIFIED || line.state == SHARED) {
            return (0, line.data, 0) // TODO: CHANGE THIS 
        }
        // If address is in the cache (I)
            // (will always be allowed, only in this state if approximateble)
            // Remain in the same state 
            // Set flag
            // check for the valid data in other caches 
            // return the flag valid data, invalid data
        if (line.state == INVALID) {
            invalid_data = line.data;

            for (int core_index = 0; core_index < core_num, core_index++){
                if (core_index != coreID){
                    // look up helper function to find the line that tag matches
                    std::pair<bool, Line> other_line_info = lookup_line(addr, core_index); 
                    if (other_line_info.a) {
                        Line other_line = other_line_info.b; 
                        // if in M or S state
                        if (other_line.state == MODIFIED || other_line.state == SHARED) {
                            valid_data = other_line.data;
                            access_llc = false; 
                            break; 
                        }
                    }
                }
            }

            if (access_llc) {
                Line llc_line = access_llc(addr); 
                valid_data = llc_line.data; 
            }

            return (1, valid_data, invalid_data);
        }  
    }
    // If address is not in the cache, fetch data from llc, assume a write-through cache
        // return the valid data
        // insert fake latency -- entry into update buffer to update to S state after n cycles
    
    else {
        
    }
     

}

// Access 
Cache_system::access_llc(uint64_t addr) {

}

// return a flag to indicate if a line with matching address is found,
// also return the line if the line is found 
Cache_system::lookup_line(uint64_t addr, uint8_t core_index){

}



void main(){
    num_cores = 4; // Temporary 
    std::vector<std::pair<uint64_t, uint64_t>> addresses; 
    // We have to manually set the range of addresses  

    Magic_memory memory_system(addresses); 
    Cache_system cache_system(magic_memory, num_cores);
    
}