#include "cache_system.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <map>

using namespace std; 


bool Magic_memory::check_address(uint64_t address) {

    printf("Check address: %lx\n", address);
    if (this->addresses.size() % 2 != 0) {
        printf("Magic Memory not given 2n values (paired ranges)\n");
    }

    for(int i = 0; i < addresses.size(); i+=2) {
        if (this->addresses[i] > this->addresses[i+1]) {
            printf("Magic memory pair a is not less than or equal to b\n");
        }

        if(address >= this->addresses[i] && address < this->addresses[i]) {
            return true; 
        }
    }
    return false; // Address not allowed
}

bool Cache_system::within_threshold(uint32_t valid, uint32_t invalid){
    
    if(valid == 0 && invalid == 0) {
        return true; 
    }

    double percentDiff = ((((double)invalid - (double)valid) / (double)valid) * 100); 

    return percentDiff <= margin_of_error;  
}

// return a flag to indicate if a line with matching address is found,
// also return the line if the line is found 
Line_result Cache_system::lookup_line(uint64_t addr, uint8_t coreID, bool is_llc){

    Cache cache; 
    if (is_llc) {
        cache = llc; 
    }
    else {
        cache = caches[coreID]; 
    }

    std::vector<uint64_t> addr_info = cache.address_convert(addr); 
    uint64_t tag = addr_info[0]; 
    uint64_t set_index = addr_info[1]; 

    Line_result result = Line_result(); 
    
    Set set = cache.sets[set_index]; 
    for (uint64_t i = 0; i < set.lines.size(); i++) {
        Line line = set.lines[i]; 
        if (line.tag == tag) {
            // If there is a match then update the access time. 
            // caches[coreID].sets[set_index].lines[i].time_accessed = global_time; 
            if(is_llc) {
                result.found = true; 
                result.line_ptr = &(llc.sets[set_index].lines[i]); 
                return result; 
            }
            else {
                result.found = true; 
                result.line_ptr = &(caches[coreID].sets[set_index].lines[i]); 
                return result; 
            }
        }
    }

    Line *empty = NULL; 
    result.found = false; 
    result.line_ptr = empty; 
    return result; 
}

// Create copy in LLC because it is a write through cache. 
void Cache_system::update_llc(uint64_t addr, uint32_t data) {
    std::vector<uint64_t> addr_info = llc.address_convert(addr); 
    uint64_t tag = addr_info[0]; 
    uint64_t set_index = addr_info[1]; 
    uint64_t block_index = addr_info[2]; 
    Set set = llc.sets[set_index]; 

    // If there is a match in the LLC then update it. 
    for (int i = 0; i < set.lines.size(); i++) {
        Line line = set.lines[i]; 
        if (line.tag == tag) {
            llc.sets[set_index].lines[i].data[block_index] = data; 
            return; 
        }
    }

    // If no match found in the LLC then create a new line and insert
    Line LLC_line(MODIFIED, tag, data, global_time, block_index); 
    llc.sets[set_index].lines.push_back(LLC_line); 
}


// Cache write 
void Cache_system::cache_write(uint8_t coreID, uint64_t addr, uint32_t data){
    global_time += 1; 

    caches[coreID].cache_stats.num_access += 1; 

    std::vector<uint64_t> addr_info = caches[coreID].address_convert(addr); 
    uint64_t tag = addr_info[0]; 
    uint64_t set_index = addr_info[1]; 
    uint64_t block_index = addr_info[2]; 

    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...>
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
    Line_result line_info = lookup_line(addr, coreID, false); 
    cout << "In Cache Write: " << boolalpha << line_info.found << endl; 
    cout << "coreid: " << unsigned(coreID) << endl; 
    if (line_info.found) {
        caches[coreID].cache_stats.num_writes_hits += 1;
        Line *line = line_info.line_ptr; 
        line->time_accessed = global_time; 
        // If address is in the cache (M), write normally
            // Remain in the same state 
            // Update in the LLC 
        if (line->state == MODIFIED) {
            line->data[block_index] = data; 
            caches[coreID].cache_stats.num_write_to_llc += 1; 
            update_llc(addr, data); 
        }

        // If address is in the cache (S, I, V)
            // Send BusRdX. and change the state of other cores 
            // Upgrade to M state
            // write
            // Change the value in the LLC 
        else if (line->state == SHARED || line->state == INVALID || line->state == VICTIMIZED) {
            stats.bus_transactions += 1; 
            line->state = MODIFIED; 
            for (uint8_t core_index = 0; core_index < num_cores; core_index++){
                if (core_index != coreID){
                    // look up helper function to find the line that tag matches
                    Line_result other_line_info = lookup_line(addr, core_index, false); 
                    if (other_line_info.found) {
                        caches[core_index].cache_stats.num_access += 1; 
                        Line *other_line = other_line_info.line_ptr; 
                        other_line->time_accessed = global_time; 
                        // if in M or S state ... don't need to flush because of write through cache
                        if (other_line->state == MODIFIED || other_line->state == SHARED) {
                            // Check whether the addr is in approximatable memory 
                            stats.total_cases += 1; 
                            // Tune how much of the spculative data 
                            double result = (double)((double)(rand() % 100) / (double)100);
                            bool speculate = result < speculation_percent;

                            if (speculate) {
                                stats.speculate_cases += 1; 
                                if(magic_memory.check_address(addr)) {
                                    stats.success_addr_bound += 1; 
                                    other_line->state = VICTIMIZED;
                                }
                                else {
                                    stats.failed_addr_bound += 1; 
                                    other_line->state = INVALID; 
                                } 
                            }
                            else {
                                other_line->state = INVALID; 
                            }
                        }
                    }
                }
            }
            line->data[block_index] = data; 
            caches[coreID].cache_stats.num_write_to_llc += 1; 
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
        caches[coreID].cache_stats.num_write_misses += 1;

        // Find the Value in the LLC 
        // std::vector<uint64_t> addr_info = caches[coreID].address_convert(addr); 
        // uint64_t tag = addr_info[0]; 
        // uint64_t set_index = addr_info[1]; 
        Set set = caches[coreID].sets[set_index]; 

        Line new_line(MODIFIED, tag, data, global_time, block_index); 
        
        caches[coreID].cache_stats.num_write_to_llc += 1; 
        update_llc(addr, data); 
        
        // If the cache isn't full. 
        if(set.lines.size() < L1_SET_ASSOCIATIVITY) {
            cout << "False, cache is not full: " << endl; 
            caches[coreID].sets[set_index].lines.push_back(new_line); 
        }
        // If the cache is full... Evict a cache line and replace it with LLC Line. 
        else {
            // cout << "False, cache is full: " << endl; 
            int LRU_index = 0; 
            uint64_t LRU_time = set.lines[0].time_accessed;
            for (uint8_t i = 1; i < set.lines.size(); i++) {
                Line line = set.lines[i]; 
                if(line.time_accessed < LRU_time) {
                    LRU_time = line.time_accessed; 
                    LRU_index = i; 
                }
            }
            // cout << "LRU Index: " << LRU_index << " time: " << LRU_time << endl; 
            caches[coreID].sets[set_index].lines[LRU_index] = new_line; 
        }

        // Check if it's in other cache and change state 
        for (uint8_t core_index = 0; core_index < num_cores; core_index++){
            stats.bus_transactions += 1; 
            if (core_index != coreID){
                // look up helper function to find the line that tag matches
                Line_result other_line_info = lookup_line(addr, core_index, false); 
                if (other_line_info.found) {
                    caches[core_index].cache_stats.num_access += 1; 
                    Line *other_line = other_line_info.line_ptr; 
                    other_line->time_accessed = global_time; 
                    // if in M or S state ... don't need to flush because of write through cache
                    if (other_line->state == MODIFIED || other_line->state == SHARED) {
                        // Check whether the addr is in approximatable memory 
                        stats.total_cases += 1; 
                        // Tune how much of the spculative data 
                        double result = (double)((double)(rand() % 100) / (double)100);
                        bool speculate = result < speculation_percent;

                        if (speculate) {
                            stats.speculate_cases += 1; 
                            if(magic_memory.check_address(addr)) {
                                stats.success_addr_bound += 1; 
                                other_line->state = VICTIMIZED;
                            }
                            else {
                                stats.failed_addr_bound += 1; 
                                other_line->state = INVALID; 
                            } 
                        }
                        else {
                            other_line->state = INVALID; 
                        }
                    }
                }
            }
        }
    }
    return; 
}

// Returns (Flag whether we speculated or not, Real Data, Invalid Data) 
Read_tuple Cache_system::cache_read(uint8_t coreID, uint64_t addr){
    global_time += 1; 
    caches[coreID].cache_stats.num_access += 1; 

    std::vector<uint64_t> addr_info = caches[coreID].address_convert(addr); 
    uint64_t tag = addr_info[0]; 
    uint64_t set_index = addr_info[1]; 
    uint64_t block_index = addr_info[2]; 

    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
    Line_result line_info = lookup_line(addr, coreID, false); 
    cout << "In Cache Read: " << boolalpha << line_info.found << "\n" << endl; 
    // If the address matches the L1 Cache 
    bool access_llc_flag = true; 

    Read_tuple result = Read_tuple(); 

    if (line_info.found) {
        caches[coreID].cache_stats.num_reads_hits += 1; 
        Line *line = line_info.line_ptr; 
        line->time_accessed = global_time; 
        // If address is in the cache (M,S), Use valid data
        // Return the data 
        // Remain in the same state 
        if (line->state == MODIFIED || line->state == SHARED) {
            result.speculated = false; 
            result.valid_data = line->data[block_index]; 
            result.invalid_data = 0; 
            return result; 
        }
        // If address is in the cache (V)
            // Go to Shared state 
            // Set Flag 
            // Find valid data in other caches 
        if (line->state == VICTIMIZED) {
            stats.bus_transactions += 1; 

            uint32_t valid_data = 0; 
            uint32_t invalid_data = line->data[block_index];

            for (uint8_t core_index = 0; core_index < num_cores; core_index++){
                if (core_index != coreID){
                    // look up helper function to find the line that tag matches
                    Line_result other_line_info = lookup_line(addr, core_index, false); 
                    if (other_line_info.found) {
                        caches[core_index].cache_stats.num_access += 1; 
                        Line *other_line = other_line_info.line_ptr; 
                        other_line->time_accessed = global_time; 
                        // if in M or S state
                        if (other_line->state == MODIFIED || other_line->state == SHARED) {
                            valid_data = other_line->data[block_index];
                            other_line->state = SHARED; 
                        }
                    }
                }
            }
            line->state = SHARED;
            line->data[block_index] = valid_data;

            // Checks whether the data is close enough 
            if(within_threshold(valid_data, invalid_data)) {
                stats.success += 1; 
                result.speculated = true; 
                result.valid_data = valid_data; 
                result.invalid_data = invalid_data; 
                return result;
            } 
            else {
                stats.rollback += 1; 
                result.speculated = false; 
                result.valid_data = valid_data; 
                result.invalid_data = invalid_data; 
                return result;
            }
            
        }

        // If address is in the cache (I)
            // (will always be allowed, only in this state if approximateble)
            // Remain in the same state 
            // Set flag
            // check for the valid data in other caches 
            // return the flag valid data, invalid data
        if (line->state == INVALID) {
            stats.bus_transactions += 1; 
            uint32_t valid_data = 0; 
            uint32_t invalid_data = line->data[block_index];
            for (uint8_t core_index = 0; core_index < num_cores; core_index++){
                if (core_index != coreID){
                    // look up helper function to find the line that tag matches
                    Line_result other_line_info = lookup_line(addr, core_index, false); 
                    if (other_line_info.found) {
                        caches[core_index].cache_stats.num_access += 1; 
                        Line *other_line = other_line_info.line_ptr; 
                        other_line->time_accessed = global_time; 
                        // if in M or S state
                        if (other_line->state == MODIFIED || other_line->state == SHARED) {
                            valid_data = other_line->data[block_index];
                            other_line->state = SHARED; 
                            access_llc_flag = false; 
                        }
                    }
                }
            }
            // If not found in the other cores then fetch from the LLC 
            if (access_llc_flag) {
                caches[coreID].cache_stats.num_read_from_llc += 1; 
                Line_result llc_line_info = lookup_line(addr, 0, true); 
                if (llc_line_info.found) {
                    valid_data = (llc_line_info.line_ptr)->data[block_index]; 
                }
                else {
                    cout << "In Read: Can't find data in cache or LLC" << endl;
                    exit(-1);
                }
            }
            line->state = SHARED; 
            line->data[block_index] = valid_data; 

            result.speculated = false; 
            result.valid_data = valid_data; 
            result.invalid_data = invalid_data; 
            return result;
        }  
    }
    // If address is not in the cache, fetch data from llc, assume a write-through cache
        // return the valid data
        // insert fake latency -- entry into update buffer to update to S state after n cycles
    else {
        caches[coreID].cache_stats.num_read_misses += 1; 
        caches[coreID].cache_stats.num_read_from_llc += 1; 
        stats.bus_transactions += 1; 
        // Find the Value in the LLC 
        Line LLC_line; 
        std::vector<uint64_t> addr_info = llc.address_convert(addr); 
        uint64_t tag = addr_info[0]; 
        uint64_t set_index = addr_info[1]; 
        Set set = llc.sets[set_index]; 

        for (int i = 0; i < set.lines.size(); i++) {
            Line line = set.lines[i]; 
            if (line.tag == tag) {
                LLC_line = line; 
                break; 
            }
        }

        // Put line into the L1 Cache 
        addr_info = caches[coreID].address_convert(addr); 
        tag = addr_info[0]; 
        set_index = addr_info[1]; 
        set = caches[coreID].sets[set_index]; 

        LLC_line.state = SHARED; 
        LLC_line.tag = tag; 
        LLC_line.time_accessed = global_time; 
        // If there isn't a capacity miss 
        if(set.lines.size() < L1_SET_ASSOCIATIVITY) {
            caches[coreID].sets[set_index].lines.push_back(LLC_line); 
        }
        // Evict a cache line and replace it with LLC Line. 
        else {
            int LRU_index = 0; 
            uint64_t LRU_time = set.lines[0].time_accessed;
            for (int i = 1; i < set.lines.size(); i++) {
                Line line = set.lines[i]; 
                if(line.time_accessed < LRU_time) {
                    LRU_time = line.time_accessed; 
                    LRU_index = i; 
                }
            }
            caches[coreID].sets[set_index].lines[LRU_index] = LLC_line; 
        }

        
//         // Update the other cache states 
//         for (uint8_t core_index = 0; core_index < num_cores; core_index++){
//             if (core_index != coreID){
//                 // look up helper function to find the line that tag matches
//                 Line_result other_line_info = lookup_line(addr, core_index, false); 
//                 if (other_line_info.found) {
//                     caches[core_index].cache_stats.num_access += 1; 
//                     Line *other_line = other_line_info.line_ptr; 
//                     other_line->time_accessed = global_time; 
//                     // if in M state
//                     if (other_line->state == MODIFIED) {
//                         other_line->state = SHARED; 
//                     }
//                 }
//             }
//         }
        
<<<<<<< HEAD
//         result.speculated = false; 
//         result.valid_data = LLC_line.data; 
//         result.invalid_data = 0; 
//         return result;
//     }
// }




// int main(){
//     uint8_t num_cores = 2; // Temporary 
//     double speculation_percent = 0.5; 
//     double margin_of_error = 0.1; 
=======
        result.speculated = false; 
        result.valid_data = LLC_line.data[block_index]; 
        result.invalid_data = 0; 
        return result;
    }
}



int main(){
    uint8_t num_cores = 2; // Temporary 
    double speculation_percent = 0.5; 
    double margin_of_error = 0.1; 
>>>>>>> 4da80df930ce627b16694353b8d265eec70d045a

//     std::vector<uint64_t> addresses; 
//     // We have to manually set the range of addresses  

//     // Create Address Range 
//     // pair<uint64_t, uint64_t> address_range0 = make_pair(0, 1000); 
//     // pair<uint64_t, uint64_t> address_range1 = make_pair(100000, 500000); 
//     addresses.push_back(0); 
//     addresses.push_back(1000); 
//     addresses.push_back(100000); 
//     addresses.push_back(500000); 

 
//     Cache_system cache_system(addresses, num_cores, speculation_percent, margin_of_error);

//     return 0; 
// }
