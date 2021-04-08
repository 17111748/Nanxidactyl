#include "cache_system.h"
#include <vector>
#include <cstdint>
#include <map>


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
    for (int i = 0; i < set.lines.size(); i++) {
        Line line = set.lines[i]; 
        if (line.tag == tag) {
            return std::make_pair(true, line); 
        }
    }
    return std::make_pair(false, NULL); 
}

// Create copy in LLC because it is a write through cache. 
Cache_system::update_llc(uint64_t addr, uint32_t data) {
    std::map<uint64_t, uint64_t> addr_info = llc.address_convert(addr); 
    uint64_t tag = addr_info.first; 
    uint64_t set_index = addr_info.second; 
    Set set = llc[set_index]; 

    // If there is a match in the LLC then update it. 
    for (int i = 0; i < set.lines.size(); i++) {
        Line line = set.lines[i]; 
        if (line.tag == tag) {
            line.data = data; 
            return; 
        }
    }
    // If no match found in the LLC then create a new line and insert
    Line LLC_line(INVALID, tag, data, global_time); 
    set.lines.push_back(LLC_line); 
}

Cache_system::cache_write(uint8_t coreID, uint64_t addr, uint32_t data){
    global_time += 1; 

    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...>
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
    std::pair<bool, Line> line_info = lookup_line(addr, coreID, false); 

    if (line_info.first) {
        Line line = line_info.second; 
        line.time_accessed = global_time; 
        // If address is in the cache (M), write normally
            // Remain in the same state 
            // Update in the LLC 
        if (line.state == MODIFIED) {
            line.data = data; 
            update_llc(addr, data); 
        }

        // If address is in the cache (S, I, V)
            // Send BusRdX. and change the state of other cores 
            // Upgrade to M state
            // write
            // Change the value in the LLC 
        if (line.state == SHARED || line.state == INVALID || line.state == VICTIMIZED) {
            line.state = MODIFIED; 
            for (int core_index = 0; core_index < core_num, core_index++){
                if (core_index != coreID){
                    // look up helper function to find the line that tag matches
                    std::pair<bool, Line> other_line_info = lookup_line(addr, core_index, false); 
                    if (other_line_info.first) {
                        Line other_line = other_line_info.second; 
                        // if in M or S state ... don't need to flush because of write through cache
                        if (other_line.state == MODIFIED || other_line.state == SHARED) {
                            other_line.state = VICTIMIZED; 
                        }
                    }
                }
            }
            line.data = data; 
            update_llc(addr, data); 
        }

    }
    // If address is not in the cache, 
        // If cache is full 
            // evict something (LRU policy)

        // Write to it
        // Change the value in the LLC 
        // Make it M state 
        // Send BusRdX, and change the state of other cores 
    else {
        // Find the Value in the LLC 

        addr_info = cache.address_convert(addr); 
        tag = addr_info.first; 
        set_index = addr_info.second; 
        set = cache[coreID][set_index]; 

        Line new_line(MODIFIED, tag, data, global_time); 

        // If the cache isn't full. 
        if(set.lines.size() < L1_SET_ASSOCIATIVITY) {
            set.lines.push_back(new_line); 
            for (int core_index = 0; core_index < core_num, core_index++){
                if (core_index != coreID){
                    // look up helper function to find the line that tag matches
                    std::pair<bool, Line> other_line_info = lookup_line(addr, core_index, false); 
                    if (other_line_info.first) {
                        Line other_line = other_line_info.second; 
                        // if in M or S state ... don't need to flush because of write through cache
                        if (other_line.state == MODIFIED || other_line.state == SHARED) {
                            other_line.state = VICTIMIZED; 
                        }
                    }
                }
            }
            update_llc(addr, data); 
        }
        // If the cache is full... Evict a cache line and replace it with LLC Line. 
        else {
            int LRU_index = 0; 
            uint64_t LRU_time = set.lines[0].time_accessed
            for (int i = 1; i < set.lines.size(); i++) {
                Line line = set.lines[i]; 
                if(line.time_accessed < LRU_time) {
                    LRU_time = line.time_accessed; 
                    LRU_index = i; 
                }
            }
            set.lines.replace(LRU_index, new_line); // TODO: CHECK SYNTAX FOR THIS 
            update_llc(addr, data); 
        }
    }
    return; 
}




// Returns (Flag whether we speculated or not, Real Data, Invalid Data) 
Cache_system::cache_read(uint8_t coreID, uint64_t addr){
    global_time += 1; 
    
    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
    std::pair<bool, Line> line_info = lookup_line(addr, coreID, false); 
    
    // If the address matches the L1 Cache 
    access_llc_flag = true; 
    if (line_info.first) {
        Line line = line_info.second; 
        line.time_accessed = global_time; 
        // If address is in the cache (M,S), Use valid data
        // Return the data 
        // Remain in the same state 
        if (line.state == MODIFIED || line.state == SHARED) {
            return std::make_tuple(false, line.data, 0)
        }
        // If address is in the cache (V)
            // Go to Shared state 
            // Set Flag 
            // Find valid data in other caches 
        if (line.state == VICTIMIZED) {
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
                            other_line.state = SHARED; 
                        }
                    }
                }
            }
            line.state = SHARED;
            return std::make_tuple(true, valid_data, invalid_data); 
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
                            other_line.state = SHARED; 
                            access_llc_flag = false; 
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
            line.state = SHARED; 
            return std::make_tuple(false, valid_data, invalid_data);
        }  
    }
    // If address is not in the cache, fetch data from llc, assume a write-through cache
        // return the valid data
        // insert fake latency -- entry into update buffer to update to S state after n cycles
    else {
        // Find the Value in the LLC 
        Line LLC_line; 
        std::map<uint64_t, uint64_t> addr_info = llc.address_convert(addr); 
        uint64_t tag = addr_info.first; 
        uint64_t set_index = addr_info.second; 
        Set set = llc[set_index]; 

        for (int i = 0; i < set.lines.size(); i++) {
            Line line = set.lines[i]; 
            if (line.tag == tag) {
                LLC_line = line; 
                break; 
            }
        }

        // Put line into the L1 Cache 
        addr_info = cache.address_convert(addr); 
        tag = addr_info.first; 
        set_index = addr_info.second; 
        set = cache[coreID][set_index]; 

        LLC_line.time_accessed = global_time; 
        // If there isn't a capacity miss 
        if(set.lines.size() < L1_SET_ASSOCIATIVITY) {
            set.lines.push_back(LLC_line); 
        }
        // Evict a cache line and replace it with LLC Line. 
        else {
            int LRU_index = 0; 
            uint64_t LRU_time = set.lines[0].time_accessed
            for (int i = 1; i < set.lines.size(); i++) {
                Line line = set.lines[i]; 
                if(line.time_accessed < LRU_time) {
                    LRU_time = line.time_accessed; 
                    LRU_index = i; 
                }
            }
            set.lines.replace(LRU_index, LLC_line); // TODO: CHECK SYNTAX FOR THIS 
        }
        
        return std::make_tuple(false, LLC_line.data , 0); 
    }
     

}


void main(){
    num_cores = 4; // Temporary 
    std::vector<std::pair<uint64_t, uint64_t>> addresses; 
    // We have to manually set the range of addresses  

    Magic_memory memory_system(addresses); 
    Cache_system cache_system(magic_memory, num_cores);
    
}