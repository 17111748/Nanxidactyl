
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



// TODO: Do we want to name this entry or line? 
class Line {

    public: 

        cache_states state; //APPROXIMATE, MODIFIED, EXCLUSIVE, SHARED or INVALID
        uint8_t tag;
        uint8_t valid;
        uint8_t dirty;
        uint32_t data; 
        
        uint8_t lineID;

        Line(cache_states state, uint8_t tag, uint8_t valid, uint8_t dirty, uint32_t data, uint8_t lineID);
        // TODO: Depends on how we want to break up the data
        
        // Extra Stuff that might be useful
};

class Set {

    public: 
        Line *lines;	
        uint8_t num_lines;	
        uint8_t num_lines_in_use; 
        
        uint8_t setID;

        Set(Line *lines, uint8_t num_lines, uint8_t num_lines_in_use, uint8_t setID); 

}; 


// Statistics for us to measure and store
class Cache_stat {

    public: 
        uint64_t num_access;
        uint64_t num_reads;
        uint64_t num_writes;
        uint64_t num_read_misses;
        uint64_t num_write_misses;
        uint64_t num_write_backs;
        uint64_t num_blocks_transferred;

        // Some methods to initialize and edit these 

}; 

// An Entry into the speculative buffer 
class Spec_buffer_entry {

    public: 
        uint64_t addr; 
        Line cache_line; 
        uint64_t time_of_speculation;  
        Spec_buffer_entry(uint64_t addr, Line cache_line, uint64_t time_of_speculation); 

}; 

// Speculative Buffer to keep track of all the Bus transactions
class Spec_buffer {

    public: 
        uint64_t size; 
        bool = full; 
        Spec_buffer_entry spec_buffer_entries; 
        Spec_buffer(uint64_t size); 

        // Methods
        void insert_entry(Spec_buffer_entry entry); 
        Spec_buffer_entry buffer_search(uint64_t addr); 

} ; 

// An Entry to the LLC_Buffer 
class Buffer_entry {

    uint8_t cache_dest; 
    uint64_t addr; 
    Line cache_line; 
    bus_actions bus_action; 
    uint64_t time_of_action; 

    Buffer_entry(uint8_t cache_dest, uint64_t addr, Line cache_line, bus_actions bus_action, uint64_t time_of_action); 

};

// Handles the LLC Bus (Normal MSI Protocol)
class MSI_buffer {
    
    public: 
        uint64_t size; 
        MSI_buffer_entry msi_buffer_entries; 
        bool = full; 
        MSI_buffer(uint64_t size); 

        // Methods
        void insert_entry(MSI_buffer_entry entry); 
        MSI_buffer_entry buffer_search(uint64_t addr); 
}; 



// Cache is essentially a core 
class Cache {

    public: 
        uint64_t  cycles; // Clock for this cache/core 

        Cache_stat cache_stats; 
        Set        *sets;
        Spec_buffer spec_buffer; 
        MSI_buffer msi_buffer; 

        uint64_t set_associativity;
        uint64_t num_sets;
        uint64_t num_bytes; 
        uint64_t index_length, offset_length, tag_length; // to index into the address 

        uint8_t cacheID;

        Cache(uint64_t set_associativity, uint64_t num_sets, uint64_t num_bytes, uint8_t cacheID); 

        // TODO: FIGURE OUT WHICH FUNCTIONS BELONG WHERE ... COMMUNICATION BTWN SYSTEM
        // Actions 
        uint32_t cache_read(uint64_t ADDR);
        void cache_write(uint64_t ADDR, uint32_t data);
        uint32_t cache_evict(uint64_t ADDR);

        // Temporary Extra 
        uint8_t cache_search(uint64_t tag, uint64_t index, uint32_t *way_num);
        void cache_replace(uint64_t index, uint32_t way_num, block blk);

        // Speculative Buffer

        // MSI Protocol Buffer

        // Speculative Execution Functions (NEED EDIT)
        //bool speculative_compare(Cache c_original, Cache c_new, uint64_t threshold); // Compare the data and determine whether it is approximately close 
        //void speculative_rollback(Line cache_line, uint64_t addr, uint32_t data, uint64_t cur_cycles); // Just add a certain number of cycles and replace the cache line 

    private: 
        // Methods 
        // Basic Operations
        void address_convert(uint64_t ADDR, uint64_t *tag, uint64_t *index);
        uint64_t address_rebuild(uint64_t tag, uint64_t index);
        void cache_increment_cycle(uint64_t cycle); 

    

};















