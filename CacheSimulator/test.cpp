#include "cache_system.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <map>

using namespace std; 

// void printLine(Line l) {
//     cout << "Printing Line: " << endl; 
//     cout << "State: " << l.state << endl; 
//     cout << "Tag: " << unsigned(l.tag) << endl;
//     for(int i = 0; i < l.data.size(); i++) {
//         cout << "Data " << i << ": " << l.data[i] << endl; 
//     }
//     cout << "Time Accessed: " << l.time_accessed << "\n" << endl; 
// }
void printLine(Line l) {
    cout << "Printing Line: " << endl; 
    cout << "State: " << l.state << endl; 
    cout << "Tag: " << unsigned(l.tag) << endl;
    cout << "Data: " << l.data << endl;  
    cout << "Time Accessed: " << l.time_accessed << "\n" << endl; 
}

void printAddressConvert(Cache c, uint64_t addr) {
    cout << "Address Convert: " << endl; 
    vector<uint64_t> info = c.address_convert(addr); 
    cout << "Tag: " << unsigned(info[0]) << endl; 
    cout << "Index: " << unsigned(info[1]) << "\n" << endl; 
    
}

void printLookupLine(Cache_system cs, uint64_t addr, uint8_t coreID, bool is_llc) {
    Line_result info_cache = cs.lookup_line(addr, coreID, is_llc); 
    cout << "\nFound: " << std::boolalpha << info_cache.found << endl; 
    if (info_cache.found) {
        printLine(*info_cache.line_ptr); 
    }
    else {
        cout << "This is not Found!" << endl; 
    }
}

void printReadAddress(Read_tuple t) {
    cout << "Reading Address: " << endl; 
    cout << "Speculated: " << boolalpha << t.speculated << endl; 
    cout << "Valid Data: " << t.valid_data << endl; 
    cout << "Invalid Data: " << t.invalid_data << "\n" <<  endl; 
}


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
        std::vector<uint64_t> addr_info = caches[coreID].address_convert(addr); 
        uint64_t tag = addr_info[0]; 
        uint64_t set_index = addr_info[1]; 
        Set set = caches[coreID].sets[set_index]; 

        Line new_line(MODIFIED, tag, data, global_time); 
        
        caches[coreID].cache_stats.num_write_to_llc += 1; 
        update_llc(addr, data); 
        
        // If the cache isn't full. 
        if(set.lines.size() < L1_SET_ASSOCIATIVITY) {
            cout << "False, cache is not full: " << endl; 
            caches[coreID].sets[set_index].lines.push_back(new_line); 
            printLine(caches[0].sets[1].lines[0]);
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
            result.valid_data = line->data; 
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
            uint32_t invalid_data = line->data;

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
            uint32_t invalid_data = line->data;
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
                Line_result llc_line_info = lookup_line(addr, 0, true); 
                valid_data = (llc_line_info.line_ptr)->data; 
            }
            line->state = SHARED; 
            line->data = valid_data; 

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

        
        // Update the other cache states 
        for (uint8_t core_index = 0; core_index < num_cores; core_index++){
            if (core_index != coreID){
                // look up helper function to find the line that tag matches
                Line_result other_line_info = lookup_line(addr, core_index, false); 
                if (other_line_info.found) {
                    caches[core_index].cache_stats.num_access += 1; 
                    Line *other_line = other_line_info.line_ptr; 
                    other_line->time_accessed = global_time; 
                    // if in M state
                    if (other_line->state == MODIFIED) {
                        other_line->state = SHARED; 
                    }
                }
            }
        }
        
        result.speculated = false; 
        result.valid_data = LLC_line.data; 
        result.invalid_data = 0; 
        return result;
    }
}



int main(){
    uint8_t num_cores = 2; // Temporary 
    double speculation_percent = 0.5; 
    double margin_of_error = 0.1; 

    std::vector<uint64_t> addresses; 
    // We have to manually set the range of addresses  

    // Create Address Range 
    // pair<uint64_t, uint64_t> address_range0 = make_pair(0, 1000); 
    // pair<uint64_t, uint64_t> address_range1 = make_pair(100000, 500000); 

    addresses.push_back(0); 
    addresses.push_back(1000); 
    addresses.push_back(100000); 
    addresses.push_back(500000); 

 
    Cache_system cache_system(addresses, 2, speculation_percent, margin_of_error);

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
    printAddressConvert(c0, addr); 

    
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
    cout << "Hello" << endl; 
    printLine(cache_system.caches[0].sets[1].lines[0]);
    printLine(cache_system.caches[1].sets[1].lines[0]);

    // cache_system.cache_write(0, addr2, 222); 
    // printLine(cache_system.caches[0].sets[1].lines[0]);
    // printLine(cache_system.caches[0].sets[1].lines[1]);

    // cache_system.cache_write(0, addr3, 333);
    // printLine(cache_system.caches[0].sets[1].lines[0]);
    // printLine(cache_system.caches[0].sets[1].lines[1]);

    cout << "\nRead" << endl; 
    Read_tuple tuple1 = cache_system.cache_read(0, addr); 
    printLine(cache_system.caches[0].sets[1].lines[0]);
    printLine(cache_system.caches[1].sets[1].lines[0]);
    // printLine(cache_system.caches[0].sets[1].lines[1]);
    printReadAddress(tuple1); 

    // Read_tuple tuple11 = cache_system.cache_read(0, addr); 
    // printLine(cache_system.caches[0].sets[1].lines[0]);
    // printLine(cache_system.caches[1].sets[1].lines[0]);
    // // printLine(cache_system.caches[0].sets[1].lines[1]);
    // printReadAddress(tuple11);
    // Read_tuple tuple2 = cache_system.cache_read(0, addr2); 
    // printLine(cache_system.caches[0].sets[1].lines[0]);
    // printLine(cache_system.caches[0].sets[1].lines[1]);
    // printReadAddress(tuple2); 
    // Read_tuple tuple3 = cache_system.cache_read(0, addr3); 
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

