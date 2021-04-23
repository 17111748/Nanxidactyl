#include "cache.h"
#include <vector>
#include <cstdint>
#include <map>


// Non-Memory Operations
#define ADD_CYCLES 1
#define MULTIPLY_CYCLES 2
#define DIVIDE_CYCLES 3


// Memory Operations
#define READ_HIT_CYCLES 1
#define READ_MISS_CYCLES 2
#define WRITE_HIT_CYCLES 1
#define WRITE_MISS_CYCLES 2
#define READ_TO_MEMORY_CYCLES 100
#define WRITE_TO_MEMORY_CYCLES 100 

// // L1 Cache parameters 
// #define L1_SET_ASSOCIATIVITY 8
// #define L1_NUM_SETS 32
// // LLC Cache parameters
// #define LLC_SET_ASSOCIATIVITY 8
// #define LLC_NUM_SETS 32

#define L1_SET_ASSOCIATIVITY 2
#define L1_NUM_SETS 2

#define LLC_SET_ASSOCIATIVITY 2
#define LLC_NUM_SETS 16


// Magic Memory Address Range index by 2
// Allowed Addresses to perform Victimized Protocol
// Address booking: keeps track of the address ranges that can tolerate approximation
class Magic_memory {
    public: 
        // Pair of start-inclusive, end-exclusive addresses which are allowed to perform protocol
        std::vector<std::vector<uint64_t>> addresses;

        Magic_memory();
        Magic_memory(std::vector<std::vector<uint64_t>> addresses_param) {addresses = addresses_param;}  

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
        System_stats(); 
};


class Line_result {
    public: 
        bool found; 
        Line *line_ptr; 
        Line_result(); 
};


class Read_tuple {
    public: 
        bool speculated; 
        uint32_t valid_data; 
        uint32_t invalid_data; 
        Read_tuple(); 
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

        Cache_system(std::vector<std::vector<uint64_t>> addresses, uint8_t number_cores, 
                                                    double speculation_percent, double margin_of_error);

        // std::pair<bool, Line*> lookup_line(uint64_t addr, uint8_t coreID, bool is_llc); 
        Line_result lookup_line(uint64_t addr, uint8_t coreID, bool is_llc); 
        void update_llc(uint64_t addr, uint32_t data); 
        bool within_threshold(uint32_t valid, uint32_t invalid); 
        
        // Returns < Whether it is speculated, valid data, invalid/speculative data > 
        // std::tuple<bool, uint32_t, uint32_t> cache_read(uint8_t coreID, uint64_t addr);
        Read_tuple cache_read(uint8_t coreID, uint64_t addr);
        void cache_write(uint8_t coreID, uint64_t addr, uint32_t data);
};


