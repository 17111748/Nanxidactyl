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

        bool check_address(uint64_t address); 
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
        Line_result lookup_line(uint64_t addr, uint8_t coreID, bool is_llc); 
        void update_llc(uint64_t addr, uint32_t data); 
        bool within_threshold(uint32_t valid, uint32_t invalid); 
        
        // Returns < Whether it is speculated, valid data, invalid/speculative data > 
        // std::tuple<bool, uint32_t, uint32_t> cache_read(uint8_t coreID, uint64_t addr);
        Read_tuple cache_read(uint8_t coreID, uint64_t addr);
        void cache_write(uint8_t coreID, uint64_t addr, uint32_t data);
};


