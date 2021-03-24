
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


// Different Cache States for the cache coherence protocol 
enum cache_states {INVALID, SHARED, VICTIMIZED, MODIFIED}; 

enum bus_actions {INVALIDATE, BUSRD, BUSRDOWN, SPECULATE}; 

enum cache_actions {READ, WRITE}; 

// Magic Memory Address Range index by 2
typedef struct magic_memory {

    uint64_t size; 
    uint64_t *addresses; 

} magic_memory; 

// TODO: Do we want to name this entry or line? 
typedef struct line {

    cache_states state; //APPROXIMATE, MODIFIED, EXCLUSIVE, SHARED or INVALID
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
    uint64_t  cycles; // Clock for this cache/core 
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


typedef struct cache_system {

    uint8_t  numCores; 
	cache    *caches;
    uint64_t *cache_cycles; // To store the local clock of each core

} cache_system;


// An Entry into the speculative buffer 
typedef struct spec_buffer_entry {

    uint64_t address; 
    line *cache_line; 
    uint64_t time_of_speculation;  

} spec_buffer_entry; 

// Speculative Buffer to keep track of all the Bus transactions
typedef struct spec_buffer {

    uint64_t size; 
    spec_buffer_entry *spec_buffer_entries; 
    uint8_t = full; 

} spec_buffer; 

// An Entry to the LLC_Buffer 
typedef struct buffer_entry {

    uint8_t cache_dest; 
    uint64_t address; 
    line *cache_line; 
    bus_actions bus_action; 
    uint8_t time_of_action; 

} buffer_entry;

// Handles the LLC Bus (Normal MSI Protocol)
typedef struct llc_buffer {
    
    uint64_t size; 
    buffer_entry *buffer_entries; 
    uint8_t = full; 

}; 


// General Cache Operations 

void cache_create(uint32_t size, uint32_t assoc);

void address_convert(uint64_t ADDR, uint64_t *tag, uint64_t *index);

uint64_t address_rebuild(uint64_t tag, uint64_t index);

void cache_increment_cycle(uint64_t cycle); 

// Cache Actions

uint32_t cache_read(uint64_t ADDR);

void cache_write(uint64_t ADDR, uint8_t dirty_bit);

uint32_t cache_evict(uint64_t index);

// Bus Actions

void cache_bus_actions(cache *cache, cache_states new_cache_state, bus_actions bus_action);


// Temporary Extra 
uint8_t cache_search(uint64_t tag, uint64_t index, uint32_t *way_num);

void cache_replace(uint64_t index, uint32_t way_num, block blk);


// Speculative Execution Functions 
bool speculative_compare(cache *c_original, cache *c_new, uint64_t threshold); // Compare the data and determine whether it is approximately close 

void speculative_rollback(cache *c_original, cache *c_new, uint64_t cur_cycles); // Just add a certain number of cycles and replace the cache line 



