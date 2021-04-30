/*
 * Copyright 2002-2020 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include <iostream>
#include <fstream>
#include "pin.H"

#include <dlfcn.h>
#include <vector>
#include <map>
// #include <tuple>

#include "cache_system.h"
// #include "cache.h"
// #include "sample.cpp"

using std::cerr;
using std::ofstream;
using std::ios;
using std::string;
using std::endl;

ofstream OutFile;

// Global PIN lock
PIN_LOCK pinLock;

// TOCHANGE: Copy from declaration in executable code
typedef struct pin_data {
    // unsigned long addr_sum;
    unsigned long addr_psum; 
    unsigned long addr_A; 
    // int sum_length; 
    int psum_length; 
    int A_length; 
    int num_cores;  
} pin_data_t;

// Address of memory locations that we can speculate on
std::map<std::string, unsigned long> funcMap; 
pin_data_t pin_data;

// Cache system to track reads
Cache_system* cache = NULL;

// Speculated to revert back to original 
bool speculated_revert = false;
std::vector<unsigned long> revert_coreID; 
std::vector<std::pair<unsigned long, uint64_t> > revert_vector; 

bool speculated[2] = {false}; 

// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 icount = 0;

// This function is called before every instruction is executed    
FILE * trace;


bool memory_bound(uint64_t address) {
    bool A_true = (pin_data.addr_A <= address && address < (pin_data.addr_A + pin_data.A_length*sizeof(unsigned long))); 
    bool psum_true = (pin_data.addr_psum <= address && address < (pin_data.addr_psum + pin_data.psum_length*sizeof(unsigned long))); 
    
    if(psum_true || A_true) {
        return true; 
    }
    else {
        return false; 
    }
}

bool memory_bound_A(uint64_t address) {
    bool A_true = (pin_data.addr_A <= address && address < (pin_data.addr_A + pin_data.A_length*sizeof(unsigned long))); 
    
    if(A_true) {
        return true; 
    }
    else {
        return false; 
    }
}

bool memory_bound_psum(uint64_t address) {
    bool psum_true = (pin_data.addr_psum <= address && address < (pin_data.addr_psum + pin_data.psum_length*sizeof(unsigned long))); 
    
    if(psum_true) {
        return true; 
    }
    else {
        return false; 
    }
}

// Occurs before memory access
VOID RecordMemRead(VOID * ip, VOID * addr, VOID *threadId)
{
    
    unsigned long tid = (unsigned long) threadId;
    PIN_GetLock(&pinLock, tid);
    // Only threadIds greater than 2 represent the execution we care about
    uint64_t coreID = ((uint64_t) threadId) - 2;

    if (tid >= 2) {
        fprintf(trace,"%p: R %p id %p\n", ip, addr, threadId);
        unsigned long addr_converted = (unsigned long)addr; 
         
        // printf("Addr_A: %lx, A_length %d, Addr_B: %lx, B_length: %d\n\n", pin_data.addr_A, pin_data.A_length, pin_data.addr_B, pin_data.B_length); 
        // printf("Address: %li\n", (uint64_t) addr); 
        // if(memory_bound(addr_converted)) {
        //     printf("Address Converted: %lx\n", addr_converted); 
        //     ((float *) (addr))[0] = 2; 
        //     printf("Editted 2 ... tid: %li\n", tid); 
        //     speculated = true; 
        // }


        if(memory_bound_psum(addr_converted)) {

            Read_tuple result = cache->cache_read(coreID, (uint64_t)addr); 
           
            // printf("Read %li: Address: %lx\n", coreID, addr_converted);
            // printf("Specualted: %d, Valid_data: %li\n\n", (result.speculated) ? 1:0, (unsigned long)result.valid_data);

            if(result.speculated && !result.not_in_system) {
                uint64_t valid_data = result.valid_data; 
                uint64_t invalid_data = result.invalid_data; 
                ((unsigned long *) (addr_converted))[0] = invalid_data; 

                // Store it in the global Revert Map
                printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~SPECULATED~~~~~~~~~~~~~~~~~~~~~~~~~\n"); 
                std::pair<unsigned long, uint64_t> revert_pair = std::make_pair(addr_converted, valid_data); 
                revert_coreID.push_back(coreID); 
                revert_vector.push_back(revert_pair); 
                speculated[coreID] = true; 
                printf("Read Before: coreID: %li\n", coreID); 
                
                
            }
            
        }

    }
    
    // TODO: Do speculative memory modification, if necessary
    // Read_tuple cache_read();
    // data a, data b;
    // Write into memory @addr, data a
    // PIN runs in Pin mem, Code runs in exec mem
    // addr pointer: points to exec mem
    // Cache simulates execution memory
    // Read from Cache. If speculative, *addr = spec value. Exec mem read, reads the spec value
    // [1, 2, 3] => [2, 2, 3]

    PIN_ReleaseLock(&pinLock);
}

// TODO: After memory read occurs, do the revert
// *addr = old memory value
// FIX at addr, data b
VOID RecordMemReadAfter(VOID * ip, VOID * addr, VOID *threadId)
{
    // printf("Read: Start Before Lock\n"); 
    unsigned long tid = (unsigned long) threadId;
    PIN_GetLock(&pinLock, tid);
    // if(tid >= 2) {
    //     printf("ReadAfter: End Before Lock\n"); 
    // }

    uint64_t coreID = ((uint64_t) threadId) - 2;
    
    // Only threadIds greater than 2 represent the execution we care about
    if (tid >= 2) {
        fprintf(trace,"%p: R %p id %p\n", ip, addr, threadId);

        unsigned long addr_converted = (unsigned long)addr; 

        if(speculated[coreID] && memory_bound_psum(addr_converted)) {

            for(int i = revert_coreID.size() - 1; i >= 0; i--) {
                if(revert_coreID[i] == coreID) {
                    std::pair<unsigned long, uint64_t> pair = revert_vector[i]; 
                    unsigned long valid_address_revert = pair.first; 
                    uint64_t valid_data_revert = pair.second; 
                    ((unsigned long *) (valid_address_revert))[0] = valid_data_revert; 
                    revert_coreID.erase(revert_coreID.begin() + i); 
                    revert_vector.erase(revert_vector.begin() + i); 
                    speculated[coreID] = false; 
                    printf("Read After: coreID: %li\n", coreID);
                    printf("Address Converted: %lx, Data: %li\n", valid_address_revert, (unsigned long)valid_data_revert);
                }
            }
        }
    }

    // if(tid >= 2) {
    //     printf("ReadAfter: End Before Lock\n"); 
    // }
    PIN_ReleaseLock(&pinLock);
    // printf("Read: End After Lock\n"); 
}

// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, VOID *data_size, VOID *threadId)
{    
    uint64_t tid = (uint64_t) threadId;
    PIN_GetLock(&pinLock, tid);

    uint64_t coreID = ((uint64_t) threadId) - 2;
    ADDRINT *addr_ptr = (ADDRINT *)addr;
    ADDRINT value;
    PIN_SafeCopy(&value, addr_ptr, sizeof(ADDRINT));
    uint64_t data = (uint64_t)value;

    // printf("Record write %p %li %li\n", addr, data, coreID);

    if (tid >= 2) {
        if (cache == NULL) {
            printf("CACHE WAS NULL");
        } else {
            // printf("WRITE %p\n", addr);
            fprintf(trace,"%p: W %p\n", ip, addr); // , threadId);
            // printf("Spec percent: %f", cache->speculation_percent);
            if(memory_bound_psum((unsigned long)addr)) {
                // printf("Write %li: Address: %lx, Data: %li\n", coreID, (unsigned long)addr, (unsigned long)data); 
                cache->cache_write(coreID, (uint64_t) addr, data);
            }
            // cache->cache_write(coreID, (uint64_t) addr, data);
        }
    }

    PIN_ReleaseLock(&pinLock);
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v) // Instrumentation
{
    // Instruments memory accesses using a predicated call, i.e.
    // the instrumentation is called iff the instruction will actually be executed.
    //
    // On the IA-32 and Intel(R) 64 architectures conditional moves and REP 
    // prefixed instructions appear as predicated instructions in Pin.
    UINT32 memOperands = INS_MemoryOperandCount(ins);
 
    // Iterate over each memory operand of the instruction.
    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
    {
        if (INS_MemoryOperandIsRead(ins, memOp))
        {
            if (INS_IsValidForIpointAfter(ins)) {
                INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, 
                memOp,
                IARG_THREAD_ID, 
                IARG_END);

            
                INS_InsertPredicatedCall(
                ins, IPOINT_AFTER, (AFUNPTR)RecordMemReadAfter,
                IARG_INST_PTR,

                IARG_MEMORYOP_EA, 
                memOp,
                
                IARG_THREAD_ID, 
                IARG_END);
            }
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {

            if (INS_IsValidForIpointAfter(ins)) {
                // Write into cache model after write occurs
                // Need to do this to pull the real written value to memory
                INS_InsertPredicatedCall(
                    ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                    IARG_INST_PTR,

                    IARG_MEMORYOP_EA, 
                    memOp,
                    IARG_MEMORYWRITE_SIZE,

                    IARG_THREAD_ID, 
                    IARG_END);
                // INS_InsertPredicatedCall(
                //     ins, IPOINT_AFTER, (AFUNPTR)RecordMemWriteAfter,
                //     IARG_INST_PTR,

                //     IARG_MEMORYOP_EA, 
                //     memOp,
                //     IARG_MEMORYWRITE_SIZE,

                //     IARG_THREAD_ID, 
                //     IARG_END);
            }
        }
    }
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "inscount.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application
    OutFile.setf(ios::showbase);
    OutFile << "Count " << icount << endl;
    OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

void fillFuncMap(IMG img, std::string s) {
    // Iterates through all of the symbols in the images
    for( SYM sym= IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym) ) {
        if (PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY) == s.c_str()) { // Unannotated C++ 
            RTN addressRtn = RTN_FindByName(img, SYM_Name(sym).c_str()); // Physically find the Pin routine that corresponds to func

            if (addressRtn.is_valid()) {
                std::cout << "Address RTN found" << std::endl;
                RTN_Open(addressRtn);
                AFUNPTR app = RTN_Funptr(addressRtn); // function pointer

                // Insert the function address a dictionary
                funcMap[PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY).c_str()] = (unsigned long) app; // Put the function pointer into the images

                RTN_Close(addressRtn);
            }
        }
    }
}

// TOCHANGE: Fill in this will the function names from the executable
void Image(IMG img, VOID *v) {
    fillFuncMap(img, "ret_pin_data");
}

void ThreadStart(THREADID threadId, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    printf("Thread Start %i\n", threadId);
    printf("~~~~~~~~~~~~~~~~~~~~~~\n"); 
    // Thread 0 is the main thread
    PIN_GetLock(&pinLock, threadId);
    
    // Thread 1 is the thread to execute code after init
    if (threadId == 1) {
        pin_data = (pin_data_t) ((pin_data_t (*) (void)) funcMap["ret_pin_data"])();
    
        // Initialize cache
        std::vector<uint64_t>  addresses;
        addresses.push_back((uint64_t) pin_data.addr_psum);
        addresses.push_back((uint64_t) pin_data.addr_psum + pin_data.psum_length * sizeof(unsigned long));
        addresses.push_back((uint64_t) pin_data.addr_A);
        addresses.push_back((uint64_t) pin_data.addr_A + pin_data.A_length * sizeof(unsigned long));

        cache = new Cache_system(addresses, pin_data.num_cores, 1, 1);
        

    }
    PIN_ReleaseLock(&pinLock);

    printf("Thread End %i\n", threadId);
    printf("~~~~~~~~~~~~~~~~~~~~~~\n"); 

}
/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    trace = fopen("pinatrace.out", "w");

    // Initialize the pin lock
    PIN_InitLock(&pinLock);
    
    PIN_InitSymbols();

    // Catches the loading of an IMAGE
    IMG_AddInstrumentFunction(Image, NULL);

    // Catches the start of execution, fills in `pin_data`
    PIN_AddThreadStartFunction(ThreadStart, 0);

    OutFile.open(KnobOutputFile.Value().c_str());

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}