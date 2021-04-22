#include "cache_system.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <map>

using namespace std; 

void printLine(Line l) {
    cout << "Printing Line: " << endl; 
    cout << "State: " << l.state << endl; 
    cout << "Tag: " << unsigned(l.tag) << endl;
    cout << "Data: " << l.data << endl; 
    cout << "Time Accessed: " << l.time_accessed << "\n" << endl; 
}

void printAddressConvert(Cache c, uint64_t addr) {
    cout << "Address Convert: " << endl; 
    pair<uint64_t, uint64_t> info = c.address_convert(addr); 
    cout << "Tag: " << unsigned(info.first) << endl; 
    cout << "Index: " << info.second << "\n" << endl; 
    
}

void printLookupLine(Cache_system cs, uint64_t addr, uint8_t coreID, bool is_llc) {
    pair<bool, Line*> info_cache = cs.lookup_line(addr, coreID, is_llc); 
    cout << "\nFound: " << std::boolalpha << info_cache.first << endl; 
    if (info_cache.first) {
        printLine(*info_cache.second); 
    }
    else {
        cout << "This is not Found!" << endl; 
    }
}

void printReadAddress(tuple<bool, uint32_t, uint32_t> t) {
    cout << "Reading Address: " << endl; 
    cout << "Speculated: " << boolalpha << get<0>(t) << endl; 
    cout << "Valid Data: " << get<1>(t) << endl; 
    cout << "Invalid Data: " << get<2>(t) << "\n" <<  endl; 
}

Magic_memory::Magic_memory() {
    addresses = vector<pair<uint64_t, uint64_t>>(); 
}

bool Magic_memory::check_address(uint64_t address) {
    for (auto addr_pair = addresses.begin(); addr_pair != addresses.end(); ++addr_pair) {
        if(address >= (*addr_pair).first && address < (*addr_pair).second) {
            return true;
        }
    }
    return false; // Address not allowed
}

System_stats::System_stats() {
    rollback = 0; 
    success = 0; 
    success_addr_bound = 0; 
    failed_addr_bound = 0;
    speculate_cases = 0; 
    total_cases = 0; 
    bus_transactions = 0; 
}


Cache_system::Cache_system(std::vector<std::pair<uint64_t, uint64_t>> addresses, uint8_t num_cores_param, 
                            double speculation_percent_param, double margin_of_error_param) {
    global_time = 0; 
    num_cores = num_cores_param;
    speculation_percent = speculation_percent_param; 
    margin_of_error = margin_of_error_param; 

    for (uint8_t i = 0; i < num_cores; i++) {
        caches[i] = Cache(L1_SET_ASSOCIATIVITY, L1_NUM_SETS);
    }

    llc = Cache(LLC_SET_ASSOCIATIVITY, LLC_NUM_SETS);

    magic_memory = Magic_memory(addresses); 
    stats = System_stats(); 
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
pair<bool, Line*> Cache_system::lookup_line(uint64_t addr, uint8_t coreID, bool is_llc){

    Cache cache; 
    if (is_llc) {
        cache = llc; 
    }
    else {
        cache = caches[coreID]; 
    }

    std::pair<uint64_t, uint64_t> addr_info = cache.address_convert(addr); 
    uint64_t tag = addr_info.first; 
    uint64_t set_index = addr_info.second; 


    Set set = cache.sets[set_index]; 
    for (uint64_t i = 0; i < set.lines.size(); i++) {
        Line line = set.lines[i]; 
        if (line.tag == tag) {
            // If there is a match then update the access time. 
            // caches[coreID].sets[set_index].lines[i].time_accessed = global_time; 
            if(is_llc) {
                return std::make_pair(true, &(llc.sets[set_index].lines[i])); 
            }
            else {
                return std::make_pair(true, &(caches[coreID].sets[set_index].lines[i])); 
            }
        }
    }

    Line *empty = NULL; 
    return std::make_pair(false, empty); 
}

// Create copy in LLC because it is a write through cache. 
void Cache_system::update_llc(uint64_t addr, uint32_t data) {
    std::pair<uint64_t, uint64_t> addr_info = llc.address_convert(addr); 
    uint64_t tag = addr_info.first; 
    uint64_t set_index = addr_info.second; 
    Set set = llc.sets[set_index]; 

    // If there is a match in the LLC then update it. 
    for (int i = 0; i < set.lines.size(); i++) {
        Line line = set.lines[i]; 
        if (line.tag == tag) {
            llc.sets[set_index].lines[i].data = data; 
            return; 
        }
    }

    // If no match found in the LLC then create a new line and insert
    Line LLC_line(MODIFIED, tag, data, global_time); 
    llc.sets[set_index].lines.push_back(LLC_line); 
}


// Cache write 
void Cache_system::cache_write(uint8_t coreID, uint64_t addr, uint32_t data){
    global_time += 1; 

    caches[coreID].cache_stats.num_access += 1; 

    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...>
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
    std::pair<bool, Line*> line_info = lookup_line(addr, coreID, false); 
    cout << "In Cache Write: " << boolalpha << line_info.first << endl; 

    if (line_info.first) {
        caches[coreID].cache_stats.num_writes_hits += 1;
        Line *line = line_info.second; 
        line->time_accessed = global_time; 
        // If address is in the cache (M), write normally
            // Remain in the same state 
            // Update in the LLC 
        if (line->state == MODIFIED) {
            line->data = data; 
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
                    std::pair<bool, Line*> other_line_info = lookup_line(addr, core_index, false); 
                    if (other_line_info.first) {
                        caches[core_index].cache_stats.num_access += 1; 
                        Line *other_line = other_line_info.second; 
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
            line->data = data; 
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
        std::pair<uint64_t, uint64_t> addr_info = caches[coreID].address_convert(addr); 
        uint64_t tag = addr_info.first; 
        uint64_t set_index = addr_info.second; 
        Set set = caches[coreID].sets[set_index]; 

        Line new_line(MODIFIED, tag, data, global_time); 

        caches[coreID].cache_stats.num_write_to_llc += 1; 
        update_llc(addr, data); 

        // If the cache isn't full. 
        if(set.lines.size() < L1_SET_ASSOCIATIVITY) {
            // cout << "False, cache is not full: " << endl; 
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
                std::pair<bool, Line*> other_line_info = lookup_line(addr, core_index, false); 
                if (other_line_info.first) {
                    caches[core_index].cache_stats.num_access += 1; 
                    Line *other_line = other_line_info.second; 
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
tuple<bool, uint32_t, uint32_t> Cache_system::cache_read(uint8_t coreID, uint64_t addr){
    global_time += 1; 
    caches[coreID].cache_stats.num_access += 1; 

    // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...
    // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
    std::pair<bool, Line*> line_info = lookup_line(addr, coreID, false); 
    cout << "In Cache Read: " << boolalpha << line_info.first << "\n" << endl; 
    // If the address matches the L1 Cache 
    bool access_llc_flag = true; 
    if (line_info.first) {
        caches[coreID].cache_stats.num_reads_hits += 1; 
        Line *line = line_info.second; 
        line->time_accessed = global_time; 
        // If address is in the cache (M,S), Use valid data
        // Return the data 
        // Remain in the same state 
        if (line->state == MODIFIED || line->state == SHARED) {
            return std::make_tuple(false, line->data, 0);
        }
        // If address is in the cache (V)
            // Go to Shared state 
            // Set Flag 
            // Find valid data in other caches 
        if (line->state == VICTIMIZED) {
            stats.bus_transactions += 1; 

            uint32_t valid_data = 0; 
            uint32_t invalid_data = line->data;

            for (uint8_t core_index = 0; core_index < num_cores; core_index++){
                if (core_index != coreID){
                    // look up helper function to find the line that tag matches
                    std::pair<bool, Line*> other_line_info = lookup_line(addr, core_index, false); 
                    if (other_line_info.first) {
                        caches[core_index].cache_stats.num_access += 1; 
                        Line *other_line = other_line_info.second; 
                        other_line->time_accessed = global_time; 
                        // if in M or S state
                        if (other_line->state == MODIFIED || other_line->state == SHARED) {
                            valid_data = other_line->data;
                            other_line->state = SHARED; 
                        }
                    }
                }
            }
            line->state = SHARED;
            line->data = valid_data;

            // Checks whether the data is close enough 
            if(within_threshold(valid_data, invalid_data)) {
                stats.success += 1; 
                return std::make_tuple(true, valid_data, invalid_data); 
            } 
            else {
                stats.rollback += 1; 
                return std::make_tuple(false, valid_data, invalid_data); 
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
            uint32_t invalid_data = line->data;
            for (uint8_t core_index = 0; core_index < num_cores; core_index++){
                if (core_index != coreID){
                    // look up helper function to find the line that tag matches
                    std::pair<bool, Line*> other_line_info = lookup_line(addr, core_index, false); 
                    if (other_line_info.first) {
                        caches[core_index].cache_stats.num_access += 1; 
                        Line *other_line = other_line_info.second; 
                        other_line->time_accessed = global_time; 
                        // if in M or S state
                        if (other_line->state == MODIFIED || other_line->state == SHARED) {
                            valid_data = other_line->data;
                            other_line->state = SHARED; 
                            access_llc_flag = false; 
                        }
                    }
                }
            }
            // If not found in the other cores then fetch from the LLC 
            if (access_llc_flag) {
                caches[coreID].cache_stats.num_read_from_llc += 1; 
                std::pair<bool, Line*> llc_line_info = lookup_line(addr, 0, true); 
                valid_data = (llc_line_info.second)->data; 
            }
            line->state = SHARED; 
            line->data = valid_data; 
            return std::make_tuple(false, valid_data, invalid_data);
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
        std::pair<uint64_t, uint64_t> addr_info = llc.address_convert(addr); 
        uint64_t tag = addr_info.first; 
        uint64_t set_index = addr_info.second; 
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
        tag = addr_info.first; 
        set_index = addr_info.second; 
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

        
        // Update the other cache states 
        for (uint8_t core_index = 0; core_index < num_cores; core_index++){
            if (core_index != coreID){
                // look up helper function to find the line that tag matches
                std::pair<bool, Line*> other_line_info = lookup_line(addr, core_index, false); 
                if (other_line_info.first) {
                    caches[core_index].cache_stats.num_access += 1; 
                    Line *other_line = other_line_info.second; 
                    other_line->time_accessed = global_time; 
                    // if in M state
                    if (other_line->state == MODIFIED) {
                        other_line->state = SHARED; 
                    }
                }
            }
        }
        
        return std::make_tuple(false, LLC_line.data , 0); 
    }
}




int main(){
    uint8_t num_cores = 2; // Temporary 
    double speculation_percent = 0.5; 
    double margin_of_error = 0.1; 

    std::vector<std::pair<uint64_t, uint64_t>> addresses; 
    // We have to manually set the range of addresses  
    addresses = vector<pair<uint64_t, uint64_t>>(); 

    // Create Address Range 
    pair<uint64_t, uint64_t> address_range0 = make_pair(0, 1000); 
    pair<uint64_t, uint64_t> address_range1 = make_pair(100000, 500000); 
    addresses.push_back(address_range0); 
    addresses.push_back(address_range1);
 

    Cache_system cache_system(addresses, num_cores, speculation_percent, margin_of_error);

    Cache llc = cache_system.llc; 
    
    // for(int i = 0; i < 10; i++) {
    //     double result = (double)((double)(rand() % 100) / (double)100);
    //     bool speculate = result < speculation_percent; 
    //     cout << "Result: " << result << ", " << boolalpha << speculate << endl; 
    // }
    
    
    // Initialize L1 Caches 
    Cache c0 = cache_system.caches[0]; 
    Cache c1 = cache_system.caches[1]; 

    Set s00 = c0.sets[0]; 
    Set s01 = c0.sets[1]; 
    Set s10 = c1.sets[0]; 
    Set s11 = c1.sets[1];  

    // cout << "Start" << endl; 
    // cout << cache_system.global_time << endl; 
    // cout << unsigned(cache_system.num_cores) << endl; 
    // cout << c0.set_associativity << endl; 
    // cout << c0.num_sets << endl; 

    // cout << s00.num_lines << endl;
    // cout << s01.num_lines << endl;
    // cout << s10.num_lines << endl;
    // cout << s11.num_lines << endl;
    
    
    // Test address convert 
    uint64_t addr = 4080; // Tag = 63 Index = 1
    uint64_t addr2 = 224; // Tag = 3 Index = 1
    uint64_t addr3 = 480; // Tag = 7 Index = 1
    // printAddressConvert(llc, addr); 

    
    // Cache has addr 4080... tag = 63 and index = 1; 
    // cache_system.cache_write(0, addr, 10000); 
    // cache_system.cache_write(0, addr, 300); 
    // cache_system.cache_write(1, addr, 20000); 

    // cache_system.caches[0].sets[1].lines.push_back(Line()); 
    // cache_system.caches[0].sets[1].lines[0].state = SHARED; 
    // cache_system.caches[0].sets[1].lines[0].tag = 63; 
    // cache_system.caches[0].sets[1].lines[0].data = 10000; 
    // cache_system.caches[0].sets[1].lines[0].time_accessed = 1; 

    
    cache_system.cache_write(0, addr, 111);
    cache_system.cache_write(1, addr, 222);
    printLine(cache_system.caches[0].sets[1].lines[0]);
    printLine(cache_system.caches[1].sets[1].lines[0]);

    // cache_system.cache_write(0, addr2, 222); 
    // printLine(cache_system.caches[0].sets[1].lines[0]);
    // printLine(cache_system.caches[0].sets[1].lines[1]);

    // cache_system.cache_write(0, addr3, 333);
    // printLine(cache_system.caches[0].sets[1].lines[0]);
    // printLine(cache_system.caches[0].sets[1].lines[1]);

    cout << "\nRead" << endl; 
    tuple<bool, uint32_t, uint32_t> tuple1 = cache_system.cache_read(0, addr); 
    printLine(cache_system.caches[0].sets[1].lines[0]);
    printLine(cache_system.caches[1].sets[1].lines[0]);
    // printLine(cache_system.caches[0].sets[1].lines[1]);
    printReadAddress(tuple1); 

    // tuple<bool, uint32_t, uint32_t> tuple11 = cache_system.cache_read(0, addr); 
    // printLine(cache_system.caches[0].sets[1].lines[0]);
    // printLine(cache_system.caches[1].sets[1].lines[0]);
    // // printLine(cache_system.caches[0].sets[1].lines[1]);
    // printReadAddress(tuple11);
    // tuple<bool, uint32_t, uint32_t> tuple2 = cache_system.cache_read(0, addr2); 
    // printLine(cache_system.caches[0].sets[1].lines[0]);
    // printLine(cache_system.caches[0].sets[1].lines[1]);
    // printReadAddress(tuple2); 
    // tuple<bool, uint32_t, uint32_t> tuple3 = cache_system.cache_read(0, addr3); 
    // printLine(cache_system.caches[0].sets[1].lines[0]);
    // printLine(cache_system.caches[0].sets[1].lines[1]);
    // printReadAddress(tuple3); 

    // printLine(cache_system.caches[0].sets[1].lines[0]);
    
    // cout << "First Cache: " << endl; 
    // printLookupLine(cache_system, addr, 0, false);
    // cout << "Second Cache: " << endl; 
    // printLookupLine(cache_system, addr2, 0, false);  
    // cout << "Third Cache: " << endl; 
    // printLookupLine(cache_system, addr3, 0, false);  

    // LLC has addr 4000... tag = 7 and index = 15; 
    // cache_system.update_llc(addr, 10000); 

    // printLine(cache_system.llc.sets[15].lines[0]); 

    // cout << "LLC: " << endl; 
    // printLookupLine(cache_system, addr, 0, true); 
    // printLookupLine(cache_system, addr2, 0, true); 
    // printLookupLine(cache_system, addr3, 0, true); 

     
    

    return 0; 
}
// Line test(INVALID, 1, 1, 1); 
// Line *l = &test; 
// l->tag = 2; 
// l->time_accessed = 2; 

// Line temp = *l; 

// printLine(temp); 

