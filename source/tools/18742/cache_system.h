#include "cache.h"
#include <vector>
#include <cstdint>
#include <map>


#define ROLLBACK_CYCLES 100 
#define SUCCESS_CYCLES 30 

// Magic Memory Address Range index by 2
// Allowed Addresses to perform Victimized Protocol
// Address booking: keeps track of the address ranges that can tolerate approximation
class Magic_memory {
    public: 
        // Pair of start-inclusive, end-exclusive addresses which are allowed to perform protocol
        // std::vector<std::vector<uint64_t>> addresses;
        std::vector<uint64_t> addresses; // 2n vector of pairs
        
        Magic_memory() {
            // this->addresses = std::vector<std::vector<uint64_t>>(); 
            this->addresses = std::vector<uint64_t>();
        };
        Magic_memory(std::vector<uint64_t> addresses_param) {addresses = addresses_param;}  
        // Magic_memory(std::vector<std::vector<uint64_t>> addresses_param) {addresses = addresses_param;}  

        bool check_address(uint64_t address) {
            printf("Check address: %lx\n", address);
            if (this->addresses.size() % 2 != 0) {
                printf("Magic Memory not given 2n values (paired ranges)\n");
            }

            for(long unsigned int i = 0; i < addresses.size(); i+=2) {
                if (this->addresses[i] > this->addresses[i+1]) {
                    printf("Magic memory pair a is not less than or equal to b\n");
                }

                if(address >= this->addresses[i] && address < this->addresses[i]) {
                    return true; 
                }
            }
            return false; // Address not allowed
        }; 
}; 

class System_stats {
    public: 
        uint64_t rollback; 
        uint64_t success; 
        uint64_t success_addr_bound; // success_addr_bound ~= success + rollback
        uint64_t failed_addr_bound;

        uint64_t speculate_cases; // speculate_case = success_addr_bound + fail_address_bound
        uint64_t total_cases; 

        uint64_t bus_transactions; 
        System_stats() {
            this->rollback = 0; 
            this->success = 0; 
            this->success_addr_bound = 0; 
            this->failed_addr_bound = 0;
            this->speculate_cases = 0; 
            this->total_cases = 0; 
            this->bus_transactions = 0; 
        };
};


class Line_result {
    public: 
        bool found; 
        Line *line_ptr; 
        Line_result() {
            found = false; 
            Line l = Line(); 
            line_ptr = &(l); 
        }; 
};


class Read_tuple {
    public: 
        bool speculated; 
        uint32_t valid_data; 
        uint32_t invalid_data; 
        Read_tuple() {
            speculated = false; 
            valid_data = 0; 
            invalid_data = 0; 
        }; 
};

// The cache system acts as the controller 
class Cache_system {
    public: 
        uint64_t global_time; // For the LRU policy
        uint8_t  num_cores; 
        float speculation_percent; 
        float margin_of_error; 
        Magic_memory magic_memory; 
        std::map<uint8_t, Cache> caches;
        Cache llc; 
        System_stats stats; 

        Cache_system(std::vector<uint64_t> addresses, uint8_t num_cores, 
                            double speculation_percent, double margin_of_error) {
            this->global_time = 0; 
            this->num_cores = num_cores;
            this->speculation_percent = speculation_percent; 
            this->margin_of_error = margin_of_error; 
            
            for (uint8_t i = 0; i < num_cores; i++) {
                this->caches[i] = Cache(L1_SET_ASSOCIATIVITY, L1_NUM_SETS);
            }

            this->llc = Cache(LLC_SET_ASSOCIATIVITY, LLC_NUM_SETS);

            this->magic_memory = Magic_memory(addresses); 
            this->stats = System_stats(); 
        };

        // std::pair<bool, Line*> lookup_line(uint64_t addr, uint8_t coreID, bool is_llc); 
        Line_result lookup_line(uint64_t addr, uint8_t coreID, bool is_llc) {
            
            // printf("Called lookup\n");
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
        }; 
        void update_llc(uint64_t addr, uint32_t data) {
            std::vector<uint64_t> addr_info = llc.address_convert(addr); 
            uint64_t tag = addr_info[0]; 
            uint64_t set_index = addr_info[1]; 
            Set set = llc.sets[set_index]; 

            // If there is a match in the LLC then update it. 
            for (long unsigned int i = 0; i < set.lines.size(); i++) {
                Line line = set.lines[i]; 
                if (line.tag == tag) {
                    llc.sets[set_index].lines[i].data = data; 
                    return; 
                }
            }

            // If no match found in the LLC then create a new line and insert
            Line LLC_line(MODIFIED, tag, data, global_time); 
            llc.sets[set_index].lines.push_back(LLC_line); 
        }; 
        bool within_threshold(uint32_t valid, uint32_t invalid); 
        
        // Returns < Whether it is speculated, valid data, invalid/speculative data > 
        // std::tuple<bool, uint32_t, uint32_t> cache_read(uint8_t coreID, uint64_t addr);
        Read_tuple cache_read(uint8_t coreID, uint64_t addr);
        
        void cache_write(uint8_t coreID, uint64_t addr, uint32_t data) {

            global_time += 1; 

            // printf("HERE\n");
            caches[coreID].cache_stats.num_access += 1; 
            // printf("HERE\n");

            // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...>
            // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
            Line_result line_info = lookup_line(addr, coreID, false); 
            // std::cout << "In Cache Write: " << boolalpha << line_info.found << std::endl; 
            // printf("HERE\n");

            if (line_info.found) {
                // printf("LINE FOUND\n");

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
                // printf("LINE NOT FOUND\n");
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
        };
};


