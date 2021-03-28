#include "cache.h"

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


class LLC [
    public: 
        uint8_t num_cores; 
        Cache llc;
        LLC(uint64_t set_associativity, uint64_t num_sets, uint64_t num_bytes); 
]; 