#include "cache.h"
#include <vector>
#include <cstdint>
#include <map>
#include <iostream>

#define ROLLBACK_CYCLES 100 
#define SUCCESS_CYCLES 30 

void printLine(Line l) {
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"); 
    printf("Printing Line: State: %d, Tag: %li\n", l.state, l.tag); 
    for(unsigned int i = 0; i < l.data.size(); i++) {
        // if(l.data[i] != 0){
        //     printf("DataOffset %d: %li\n", i, l.data[i]); 
        // }
        printf("DataOffset %d: %li\n", i, l.data[i]);
    }
    // printf("Time Accessed: %li\n", l.time_accessed); 
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"); 
}


FILE *output = fopen("output.txt", "w"); 
FILE *bus = fopen("bus.txt", "w"); 
int success_count = 0; 
int rollback_count = 0; 

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
            
            if (this->addresses.size() % 2 != 0) {
                printf("Magic Memory not given 2n values (paired ranges)\n");
            }

            for(unsigned int i = 0; i < addresses.size(); i+=2) {
                if (this->addresses[i] > this->addresses[i+1]) {
                    printf("Magic memory pair a is not less than or equal to b\n");
                }

                if(address >= this->addresses[i] && address < this->addresses[i+1]) {
                    // printf("Check address TRUE: %lx\n", address);
                    return true; 
                }
            }
            // printf("Check address FALSE: %lx\n", address);
            // printf("psum start: %lx, psum end: %lx\n", this->addresses[0], this->addresses[1]); 
            return false; // Address not allowed
        }; 
}; 

class System_stats {
    public: 
        uint64_t rollback; // in read: doesn't pass the accuracy test
        uint64_t success; // in write: passes the accuracy test 
        uint64_t success_addr_bound; // success_addr_bound: in write: goes into victimized state
        uint64_t failed_addr_bound; // failed addr_bound: in write: goes into invalid state

        uint64_t speculate_cases; // speculate_case = success_addr_bound + fail_address_bound
        uint64_t total_cases; 
        uint64_t same_line; 

        uint64_t bus_transactions; 
        System_stats() {
            this->rollback = 0; 
            this->success = 0; 
            this->success_addr_bound = 0; 
            this->failed_addr_bound = 0;
            this->speculate_cases = 0; 
            this->total_cases = 0; 
            this->bus_transactions = 0; 
            this->same_line = 0; 
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
        bool not_in_system; 
        uint64_t valid_data; 
        uint64_t invalid_data; 
        Read_tuple() {
            speculated = false; 
            not_in_system = false; 
            valid_data = 0; 
            invalid_data = 0; 
        }; 
};

class Instr {
    public: 
        bool read; 
        uint8_t coreID; 
        uint64_t addr; 
        uint64_t data; 
        Instr() {
            read = true; 
            coreID = 0; 
            addr = 0; 
            data = 0; 
        };
        Instr(bool read, uint8_t coreID, uint64_t addr, uint64_t data) {
            this->read = read; 
            this->coreID = coreID; 
            this->addr = addr; 
            this->data = data; 
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
        std::vector<double> percentage_array; 
        std::vector<int> error_array; 
        std::vector<int> failure_array; 

        uint64_t total_instr; 
        std::vector<std::vector<Instr> > instruction_map; 

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

            this->total_instr = 0; 
            for(uint8_t i = 0; i < num_cores; i++) {
                std::vector<Instr> temp_vector; 
                this->instruction_map.push_back(temp_vector); 
            }
            
            this->percentage_array.push_back(0.1); 
            this->percentage_array.push_back(1); 
            this->percentage_array.push_back(5); 
            this->percentage_array.push_back(10); 
            this->percentage_array.push_back(20); 
            this->percentage_array.push_back(50); 
            this->percentage_array.push_back(100); 

            for(int i = 0; i < 7; i++) {
                this->error_array.push_back(0); 
                this->failure_array.push_back(0);
            }
            // print_parallel_stats(); 
        };
        
        void fake_read(unsigned long temp_coreID, uint64_t addr, FILE * trace = NULL) {
            Instr instr = Instr(true, temp_coreID, addr, 0); 
            instruction_map[temp_coreID].push_back(instr); 
            total_instr += 1; 
            // fprintf(trace, "total_instr: %li\n", total_instr); 
            // if(total_instr == 304195) {
            //     print_parallel_stats(trace); 
            // }
        }

        void fake_write(unsigned long temp_coreID, uint64_t addr, uint64_t data, FILE *trace = NULL) {
            Instr instr = Instr(false, temp_coreID, addr, data); 
            instruction_map[temp_coreID].push_back(instr); 
            total_instr += 1; 
            // fprintf(trace, "total_instr: %li\n", total_instr); 
            // if(total_instr == 304195) {
            //     print_parallel_stats(trace); 
            // }
        }

        void print_parallel_stats(FILE *trace = NULL) {
            printf("Total Instr: %li\n", total_instr); 
            uint64_t finished_instr = total_instr; 
            uint64_t counter = 0; 
            std::vector<uint64_t> indexes; 
            for(unsigned long i = 0; i < num_cores; i++) {
                indexes.push_back(0); 
            }

            while(finished_instr > (uint64_t)0) {
                int core_index = counter % (uint64_t) num_cores; 

                if(instruction_map[core_index].size() <= indexes[core_index]) {
                    counter += 1; 
                }
                else {
                    uint64_t index = indexes[core_index]; 
                    Instr instr = instruction_map[core_index][index]; 

                    if(instr.read) {
                        cache_read(core_index, instr.addr); 
                    }
                    else {
                        cache_write(core_index, instr.addr, instr.data); 
                    }

                    indexes[core_index] += 1; 
                    finished_instr -= 1; 
                    counter += 1; 
                }
                
            }
            // printf("End\n"); 
            print_system_stats(trace); 
        }

        void print_system_stats(FILE * trace = NULL) {
            if(trace == NULL) {
                printf("~~~~~~~~~~~~~~~Printing System Stats~~~~~~~~~~~~~~~~~~~~~\n"); 
                printf("Rollback: %li\n", stats.rollback); 
                printf("Success: %li\n", stats.success); 
                printf("Success Address Bound: %li\n", stats.success_addr_bound); 
                printf("Failed Address Bound: %li\n", stats.failed_addr_bound); 
                printf("Speculated Cases: %li\n", stats.speculate_cases); 
                printf("Total Cases: %li\n", stats.total_cases);
                printf("Bus Transactions: %li\n", stats.bus_transactions);  
                printf("~~~~~~~~~~~~~~~~~~~~~End Print~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"); 
            }
            else {
                fprintf(trace, "~~~~~~~~~~~~~~~Printing System Stats~~~~~~~~~~~~~~~~~~~~~\n"); 
                fprintf(trace, "Rollback: %li\n", stats.rollback); 
                fprintf(trace, "Success: %li\n", stats.success); 
                fprintf(trace, "Same_line: %li\n", stats.same_line);
                fprintf(trace, "Victimized State: %li\n", stats.success_addr_bound); 
                // fprintf(trace, "Failed Address Bound: %li\n", stats.failed_addr_bound); 
                fprintf(trace, "Speculated Cases: %li\n", stats.speculate_cases); 
                // fprintf(trace, "Total Cases: %li\n", stats.total_cases);
                fprintf(trace, "Bus Transactions: %li\n", stats.bus_transactions);  
                fprintf(trace, "~~~~~~~~~~~~~~~~~~~~~End Print~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"); 
            }
        }

        bool within_threshold(uint64_t valid, uint64_t invalid) {
            // printf("Valid Data: %li, invalid data: %li\n", valid, invalid); 

            double percentDiff; 
            if(valid == 0 && invalid == 0) {
                percentDiff = 0; 
            }
            else {
                percentDiff = ((  abs((double)invalid - (double)valid) / (double)valid) * 100); 
            }
            

            if(percentDiff < 0.0) {
                percentDiff = -1 * percentDiff; 
            }
            // printf("percent Diff %f, valid_data: %li, invalid_data: %li\n", percentDiff, valid, invalid); 

            
            for(unsigned int i = 0; i < error_array.size(); i++) {
                if(percentDiff <= percentage_array[i]) {
                    error_array[i] += 1; 
                }
                else{
                    failure_array[i] += 1; 
                }
            }
            if (percentDiff <= margin_of_error) {
                success_count += 1; 
                fprintf(output, "SUCCESS! Count: %d, VALID DATA: %li, INVALID DATA: %li, diff: %f\n", success_count, valid, invalid, percentDiff);
            }
            else {
                rollback_count += 1; 
                fprintf(output, "FAILED! Count: %d, VALID DATA: %li, INVALID DATA: %li, diff: %f\n", rollback_count, valid, invalid, percentDiff);
            }
            return true; 
            // return percentDiff <= margin_of_error;  
        }; 

        Line_result lookup_line(uint64_t addr, uint8_t coreID, bool is_llc) {
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


        void update_llc(uint64_t addr, uint64_t data) {
            // printf("Update LLC: Start Update LLC\n"); 
            std::vector<uint64_t> addr_info = llc.address_convert(addr); 
            uint64_t tag = addr_info[0]; 
            uint64_t set_index = addr_info[1]; 
            uint64_t block_index = addr_info[2]; 

            
            Set set = llc.sets[set_index]; 
            // printf("Update LLC: Get the Set %li\n", set.lines.size()); 
            // If there is a match in the LLC then update it. 
            for (unsigned int i = 0; i < set.lines.size(); i++) {
                Line line = set.lines[i]; 
                if (line.tag == tag) {
                    llc.sets[set_index].lines[i].data[block_index] = data; 
                    return; 
                }
            }
            // printf("Update LLC: no match create new line\n");
            // If no match found in the LLC then create a new line and insert
            Line LLC_line(MODIFIED, tag, data, global_time, block_index); 
            // printf("Update LLC: Create LLC Line\n"); 
            llc.sets[set_index].lines.push_back(LLC_line);
            // printf("Update LLC: End: No Match Found LLC\n"); 
        }; 
        

        
        

        void cache_write(uint8_t coreID, uint64_t addr, uint64_t data) {
            total_instr += 1; 
            global_time += 1; 
            // printf("\nIn cache_system.h: In Cache Write Function \n"); 
            // printf("Cache Write: CoreID: %i, Address: %li, Data: %li\n", coreID, addr, data);
            caches[coreID].cache_stats.num_access += 1; 
            // printf("Cache Write: Address Convert\n"); 
            std::vector<uint64_t> addr_info = caches[coreID].address_convert(addr);
            uint64_t tag = addr_info[0]; 
            uint64_t set_index = addr_info[1]; 
            uint64_t block_index = addr_info[2]; 

            
            // printf("cache_write: Finish Address Convert: Tag: %li, Set: %li, Block: %li\n", tag, set_index, block_index);


            // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...>
            // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
            Line_result line_info = lookup_line(addr, coreID, false); 
            // cout << "In Cache Write: " << boolalpha << line_info.found << endl; 
            
            // cout << "coreid: " << unsigned(coreID) << endl; 
            if (line_info.found) {
                // printf("cache_write: inside the cache (Hit)\n"); 

                // printf("Core: %d\n", coreID); 
                // printLine(caches[coreID].sets[3].lines[0]); 
                // printf("\n"); 

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
                    fprintf(bus, "counter: %li\n", stats.bus_transactions); 
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

                                    // Copy the cache contents over 
                                    if(other_line->state == MODIFIED) {
                                        line->data = other_line->data; 
                                    }
                                    
                                    if (speculate) {
                                        // printf("~~~~~~~~~~~~~~~~~~~~cache_write: HIT SPECUALTED~~~~~~~~~~~~~~~~~\n"); 
                                        stats.speculate_cases += 1; 
                                        if(magic_memory.check_address(addr)) {
                                            // printf("cache_write: other_line coreID: %d, state: %d\n", core_index, other_line->state);
                                            stats.success_addr_bound += 1; 
                                            other_line->state = VICTIMIZED;
                                            // printf("cache_write: other_line coreID: %d, state: %d\n", core_index, other_line->state);
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
                // printf("Cache Write: NOT inside the cache (Miss)\n"); 
                caches[coreID].cache_stats.num_write_misses += 1;
                // printf("Cache Write: add num_write_misses\n");
                // Find the Value in the LLC 
                // std::vector<uint64_t> addr_info = caches[coreID].address_convert(addr); 
                // uint64_t tag = addr_info[0]; 
                // uint64_t set_index = addr_info[1]; 
                Set set = caches[coreID].sets[set_index]; 
                // printf("Cache Write: create new line\n");
                Line new_line(MODIFIED, tag, data, global_time, block_index); 
                
                caches[coreID].cache_stats.num_write_to_llc += 1; 
                // printf("Cache Write: Update LLC\n");
                update_llc(addr, data); 
                // printf("Cache Write: After update llc \n");
                
                // printf("Cache Write: Start checking other caches\n");
                // Check if it's in other cache and change state 
                for (uint8_t core_index = 0; core_index < num_cores; core_index++){
                    stats.bus_transactions += 1; 
                    fprintf(bus, "counter: %li\n", stats.bus_transactions); 
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

                                // Copy the cache contents over 
                                if(other_line->state == MODIFIED) {
                                    new_line.data = other_line->data; 
                                }

                                if (speculate) {
                                    // printf("~~~~~~~~~~~~~~~~~~~~cache_write: MISS SPECUALTED~~~~~~~~~~~~~~~~~\n"); 
                                    stats.speculate_cases += 1; 
                                    if(magic_memory.check_address(addr)) {
                                        // printf("cache_write: other_line coreID: %d, state: %d\n", core_index, other_line->state);
                                        stats.success_addr_bound += 1; 
                                        other_line->state = VICTIMIZED;
                                        //  printf("cache_write: other_line coreID: %d, state: %d\n", core_index, other_line->state); 
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

                // If the cache isn't full. 
                if(set.lines.size() < L1_SET_ASSOCIATIVITY) {
                    // cout << "False, cache is not full: " << endl; 
                    // printf("Cache Write: cache is not full\n");
                    caches[coreID].sets[set_index].lines.push_back(new_line); 
                }
                // If the cache is full... Evict a cache line and replace it with LLC Line. 
                else {
                    // printf("Cache Write: cache is full\n");
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
            }
            return; 
        };

        // Returns < Whether it is speculated, valid data, invalid/speculative data > 
        // std::tuple<bool, uint32_t, uint32_t> cache_read(uint8_t coreID, uint64_t addr);
        Read_tuple cache_read(uint8_t coreID, uint64_t addr) {
            total_instr +=1; 
            // printf("Cache Read: Start Cache Read \n"); 
            global_time += 1; 
            caches[coreID].cache_stats.num_access += 1; 

            std::vector<uint64_t> addr_info = caches[coreID].address_convert(addr); 
            // uint64_t tag = addr_info[0]; 
            // uint64_t set_index = addr_info[1]; 
            uint64_t block_index = addr_info[2]; 

            // Check cache[coreID] for the address -- gives set <Line1, Line2, Line3, ...
            // Compare tags -- see if address is in the set at all -- gives line <tag, valid, dirty, state> (Hit/Miss)
            Line_result line_info = lookup_line(addr, coreID, false); 

            // printf("cache_read: Finish Address Convert: Tag: %li, Set: %li, Block: %li\n", tag, set_index, block_index);

    
            // cout << "In Cache Read: " << boolalpha << line_info.found << "\n" << endl; 
            // If the address matches the L1 Cache 
            bool access_llc_flag = true; 

            Read_tuple result = Read_tuple(); 
            
            
            if (line_info.found) {
                // printf("cache_read: inside the cache (Hit)\n"); 

                // printf("Core: %d\n", coreID); 
                // printLine(caches[coreID].sets[4].lines[0]); 
                // printf("\n"); 

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
                    // printf("~~~~~~~~~~~~~~~~~~`cache_read: read in victimized~~~~~~~~~~~~~~~~~~~\n"); 
                    stats.bus_transactions += 1; 
                    fprintf(bus, "counter: %li\n", stats.bus_transactions); 
                    uint64_t valid_data = 0; 
                    uint64_t invalid_data = line->data[block_index];
                    // printf("invalid_data: %li\n", invalid_data); 

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
                                    // printf("OtherLine State: %d, Data: %li\n", other_line->state, other_line->data[block_index]);
                                    // printLine(*other_line); 

                                    // Copy the cache contents over 
                                    if(other_line->state == MODIFIED) {
                                        line->data = other_line->data; 
                                    }

                                    valid_data = other_line->data[block_index];
                                    other_line->state = SHARED; 
                                }
                            }
                        }
                    }

                    // printf("valid_data: %li\n", valid_data); 
                    line->state = SHARED;
                    line->data[block_index] = valid_data;
                    // printf("SPECULATION!!!!        Address: %lx\n", addr); 
                    // Checks whether the data is close enough 
                    if(magic_memory.check_address(addr)) {
                        if(within_threshold(valid_data, invalid_data)) {
                            // printf("SUCESSSSSSS Address: %lx\n", addr); 
                            stats.success += 1; 
                            result.speculated = true; 
                            result.valid_data = valid_data; 
                            result.invalid_data = invalid_data; 
                            return result;
                        } 
                        else {
                            // printf("ROLLLBACKKKKK\n\n"); 
                             
                            stats.rollback += 1; 
                            result.speculated = false; 
                            result.valid_data = valid_data; 
                            result.invalid_data = invalid_data; 
                            return result;
                        }
                    }
                    else {
                        stats.same_line += 1; 
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
                    fprintf(bus, "counter: %li\n", stats.bus_transactions); 
                    uint64_t valid_data = 0; 
                    uint64_t invalid_data = line->data[block_index];
                    for (uint8_t core_index = 0; core_index < num_cores; core_index++){
                        if (core_index != coreID){
                            // look up helper function to find the line that tag matches
                            Line_result other_line_info = lookup_line(addr, core_index, false); 
                            if (other_line_info.found) {
                                caches[core_index].cache_stats.num_access += 1; 
                                Line *other_line = other_line_info.line_ptr; 
                                other_line->time_accessed = global_time; 
                                
                                std::vector<uint64_t> addr_info = caches[core_index].address_convert(addr); 
                                uint64_t block_index = addr_info[2]; 

                                // if in M or S state
                                if (other_line->state == MODIFIED || other_line->state == SHARED) {

                                    // Copy the cache contents over 
                                    if(other_line->state == MODIFIED) {
                                        line->data = other_line->data; 
                                    }

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
                        else { // Not in the LLC as well 
                            result.not_in_system = true; 
                            return result;
                        }
                    }
                    line->state = SHARED; 
                    line->data[block_index] = valid_data; 

                    result.speculated = false; 
                    result.valid_data = valid_data; 
                    result.invalid_data = invalid_data; 
                    // printf("Cache Read: End Cache Read with cache (Hit)\n"); 
                    return result;
                }  
            }
            // If address is not in the cache, fetch data from llc, assume a write-through cache
                // return the valid data
                // insert fake latency -- entry into update buffer to update to S state after n cycles
            else {
                // printf("Cache Read: Cache Miss\n"); 
                caches[coreID].cache_stats.num_read_misses += 1; 
                caches[coreID].cache_stats.num_read_from_llc += 1; 
                stats.bus_transactions += 1; 
                fprintf(bus, "counter: %li\n", stats.bus_transactions); 
                // Find the Value in the LLC 
                Line LLC_line; 
                std::vector<uint64_t> addr_info = llc.address_convert(addr); 
                uint64_t tag = addr_info[0]; 
                uint64_t set_index = addr_info[1]; 
                uint64_t block_index = addr_info[2]; 

                Set set = llc.sets[set_index]; 

                bool in_LLC = false; 
                // printf("Cache Read: Attempt to Find Tag\n"); 
                for (unsigned int i = 0; i < set.lines.size(); i++) {
                    Line line = set.lines[i]; 
                    if (line.tag == tag) {
                        in_LLC = true; 
                        LLC_line = line; 
                        break; 
                    }
                }

                // If Value is not in the LLC as well 
                if(!in_LLC) {
                    result.not_in_system = true; 
                    return result; 
                }

                // Put line into the L1 Cache 
                addr_info = caches[coreID].address_convert(addr); 
                tag = addr_info[0]; 
                set_index = addr_info[1]; 
                block_index = addr_info[2]; 
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
                    for (unsigned int i = 1; i < set.lines.size(); i++) {
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

                                if(other_line->data != LLC_line.data) {
                                    // printf("ERROR!!!!..... cache_read: other_line != LLC_line\n");
                                }
                                other_line->data = LLC_line.data; 
                            }
                        }
                    }
                }
                // printf("Cache Read: Done Updating\n"); 
                result.speculated = false; 
                result.valid_data = LLC_line.data[block_index]; 
                result.invalid_data = 0; 
                // printf("Cache Read: End Cache Read with cache (Miss)\n"); 
                return result;
            }
            // printf("\nIn cache_system.h: Shouldn't get here in Cache Read\n"); 
            return result; 
        };
};


