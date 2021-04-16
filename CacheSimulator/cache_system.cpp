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
    
    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
    std::pair<bool, Line> line_info = lookup_line(addr, coreID, false); 
    
    // If the address matches the L1 Cache 
    access_llc_flag = true; 
    if (line_info.first) {
        // If address is in the cache (M,S), Use valid data
        // Return the data 
        // Remain in the same state 
        Line line = line_info.second; 
        if (line.state == MODIFIED || line.state == SHARED) {
            return std::make_tuple(false, line.data, 0)
        }
        // If address is in the cache (I)
            // (will always be allowed, only in this state if approximateble)
            // Remain in the same state 
            // Set flag
            // check for the valid data in other caches 
            // return the flag valid data, invalid data
        if (line.state == INVALID) {
            valid_data = 0; 
            invalid_data = line.data;

            for (int core_index = 0; core_index < core_num, core_index++){
                if (core_index != coreID){
                    // look up helper function to find the line that tag matches
                    std::pair<bool, Line> other_line_info = lookup_line(addr, core_index, false); 
                    if (other_line_info.first) {
                        Line other_line = other_line_info.second; 
                        // if in M or S state
                        if (other_line.state == MODIFIED || other_line.state == SHARED) {
                            valid_data = other_line.data;
                            access_llc_flag = false; 
                            break; 
                        }
                    }
                }
            }
            // If not found in the other cores then fetch from the LLC 
            if (access_llc_flag) {
                std::pair<bool, Line> llc_line_info = lookup_line(addr, 0, true); 
                // TODO: Check the LLC 
                if(llc_line_info.first == false) {
                    cout << "There shouldn't be lines not found on the LLC" << endl;
                }
                valid_data = llc_line.second.data; 
            }

            return std::make_tuple(true, valid_data, invalid_data);
        }  
    }
    // If address is not in the cache, fetch data from llc, assume a write-through cache
        // return the valid data
        // insert fake latency -- entry into update buffer to update to S state after n cycles
    else {
        
    }
     

}


// return a flag to indicate if a line with matching address is found,
// also return the line if the line is found 
Cache_system::lookup_line(uint64_t addr, uint8_t coreID, bool is_llc){

    Cache cache; 
    if (is_llc) {
        cache = llc; 
    }
    else {
        cache = caches[coreID]; 
    }

    std::map<uint64_t, uint64_t> addr_info = cache.address_convert(addr); 
    uint64_t tag = addr_info.first; 
    uint64_t set_index = addr_info.second; 

    Set set = cache[set_index]; 
    for (int i = 0; i < set.num_lines; i++) {
        Line line = set.lines[i]; 
        if (line.tag == tag) {
            return std::make_pair(true, line); 
        }
    }
    return std::make_pair(false, NULL); 
}



void main(){
    num_cores = 4; // Temporary 
    std::vector<std::pair<uint64_t, uint64_t>> addresses; 
    // We have to manually set the range of addresses  

    Magic_memory memory_system(addresses); 
    Cache_system cache_system(magic_memory, num_cores);
    
}