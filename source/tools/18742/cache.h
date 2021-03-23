
// Non-Memory Operations
#define ADD_CYCLES 1
#define MULTIPLY_CYCLES 2
#define DIVIDE_CYCLES 3


// Memory Operations
#define HIT_CYCLES 1
#define MISS_CYCLES 2
#define SPEC_PENALTY 3 


// Different Cache States for the cache coherence protocol 
enum cache_state {INVALID, SHARED, EXCLUSIVE, MODIFIED, APPROXIMATE}; 

// TODO: Do we want to name this entry or line? 
typedef struct line {

    cache_state state; //APPROXIMATE, MODIFIED, EXCLUSIVE, SHARED or INVALID
	uint8_t tag;
	uint8_t valid;
	uint8_t dirty;
    uint32_t data; 
    
    uint8_t lineID;
    // TODO: Depends on how we want to break up the data
    
    // Extra Stuff that might be useful
    
} line;

typedef struct set {

	line *lines;	
	uint8_t num_lines;	
    uint8_t num_lines_in_use; 
	
    uint8_t setID;

} set;


// Statistics for us to measure and store
typedef struct cache_stat {

    uint64_t num_access;
	uint64_t num_reads;
	uint64_t num_writes;
	uint64_t num_read_misses;
	uint64_t num_write_misses;
	uint64_t num_write_backs;
	uint64_t num_blocks_transferred;

} cache_stat; 

// Cache is essentially a core 
typedef struct cache {
    uint64_t cycles; // Clock for this cache/core 
    uint32_t  level; 

    cache_stat *cache_stats; 
	set        *sets;	

    uint64_t set_associativity;
	uint64_t num_sets;
	uint64_t num_lines_per_set;

	uint64_t num_bytes; 
	uint64_t index_length, offset_length, tag_length; // to index into the address 
	
	
	uint8_t cacheID;

} cache;


typedef struct multicore_cache {

    uint8_t  numCores; 
	cache    *caches;
    uint64_t *cache_cycles; // To store the local clock of each core

} multicore_cache;


// An Entry into the speculative buffer 
typedef struct spec_cache_line {

    uint8_t cache_src;  // Which core this cache line belongs to
    uint8_t cache_dest; // Which core this cache line is going to 
    line *cache_line; 

    uint8_t penalty_time;
    uint8_t time_remaining;  

} spec_cache_line; 

// Speculative Buffer to keep track of all the Bus transactions
typedef struct spec_buffer {

    uint64_t max_buffer_size; 
    spec_cache_line *spec_cache_lines; 

    uint8_t = full; 

} spec_buffer; 




void cache_create(uint32_t size, uint32_t assoc);

void address_convert(uint64_t ADDR, uint64_t *tag, uint64_t *index);

uint64_t address_rebuild(uint64_t tag, uint64_t index);

void cache_increment_cycle(uint64_t cycle); 

void cache_change_state(uint64_t ADDR, cache_state state);

uint32_t cache_read(int64_t ADDR, block *blk, uint64_t rank_value);

void cache_write(uint64_t ADDR, uint8_t dirty_bit, uint64_t rank_value);

uint8_t cache_search(uint64_t tag, uint64_t index, uint32_t *way_num);

uint32_t cache_evict(uint64_t index);

void cache_replace(uint64_t index, uint32_t way_num, block blk);

