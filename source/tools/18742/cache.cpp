// #include "cache.h"
// #include "vector"
// #include <cstdint>
// #include <map>
// #include <cmath>
// #include <iostream>


// // Buffer Sizes 
// #define BLOCK_SIZE 5
// #define ADDR_SIZE 64

// // Cache Helper Functions 
// std::vector<uint64_t> Cache::address_convert(uint64_t addr) {
//     uint8_t index_length = log2(num_sets); 
//     uint8_t tag_length = ADDR_SIZE - BLOCK_SIZE - index_length; 

//     uint64_t tag = addr >> (ADDR_SIZE - tag_length); 
//     uint64_t index = (addr << tag_length) >> (tag_length + BLOCK_SIZE); 

//     std::vector<uint64_t> result; 
//     result.push_back(tag); 
//     result.push_back(unsigned(index)); 
//     return result; 
// }








