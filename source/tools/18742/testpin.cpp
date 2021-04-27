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
    unsigned long addr_A;
    unsigned long addr_B;
    unsigned long addr_CWT;
    unsigned long addr_CWOT;
    int A_length;
    int B_length;
    int CWT_length;
    int CWOT_length;
    int num_cores;
} pin_data_t;

// Address of memory locations that we can speculate on
std::map<std::string, unsigned long> funcMap; 
pin_data_t pin_data;

// Cache system to track reads
Cache_system* cache = NULL;

// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 icount = 0;

// This function is called before every instruction is executed    
FILE * trace;

// Occurs before memory access
VOID RecordMemRead(VOID * ip, VOID * addr, VOID *threadId)
{
    
    unsigned long tid = (unsigned long) threadId;
    PIN_GetLock(&pinLock, tid);
    // Only threadIds greater than 2 represent the execution we care about
    
    if (tid >= 2) {
        fprintf(trace,"%p: R %p id %p\n", ip, addr, threadId);
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
 
// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr, VOID *data_size, VOID *threadId)
{    
    uint64_t tid = (uint64_t) threadId;
    PIN_GetLock(&pinLock, tid);

    uint64_t coreId = ((uint64_t) threadId) - 2;
    ADDRINT *addr_ptr = (ADDRINT *)addr;
    ADDRINT value;
    PIN_SafeCopy(&value, addr_ptr, sizeof(ADDRINT));
    uint64_t data = value;

    // printf("Record write %p %li %li\n", addr, data, coreId);

    if (tid >= 2) {
        if (cache == NULL) {
            // printf("CACHE WAS NULL");
        } else {
            // printf("WRITE %p\n", addr);
            fprintf(trace,"%p: W %p\n", ip, addr); // , threadId);
            // printf("Spec percent: %f", cache->speculation_percent);
            cache->cache_write(coreId, (uint64_t) addr, data);
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
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, 
                memOp,
                IARG_THREAD_ID, 
                IARG_END);
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
        if (PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY) == s.c_str()) {
            RTN addressRtn = RTN_FindByName(img, SYM_Name(sym).c_str());

            if (addressRtn.is_valid()) {
                std::cout << "Address RTN found" << std::endl;
                RTN_Open(addressRtn);
                AFUNPTR app = RTN_Funptr(addressRtn);

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
    // Thread 0 is the main thread
    PIN_GetLock(&pinLock, threadId);
    
    // Thread 1 is the thread to execute code after init
    if (threadId == 1) {
        pin_data = (pin_data_t) ((pin_data_t (*) (void)) funcMap["ret_pin_data"])();
    
        // Initialize cache
        std::vector<uint64_t>  addresses;
        addresses.push_back((uint64_t) pin_data.addr_A);
        addresses.push_back((uint64_t) pin_data.addr_A + pin_data.A_length * sizeof(float));
        addresses.push_back((uint64_t) pin_data.addr_B);
        addresses.push_back((uint64_t) pin_data.addr_B + pin_data.B_length * sizeof(float));
        addresses.push_back((uint64_t) pin_data.addr_CWT);
        addresses.push_back((uint64_t) pin_data.addr_CWT + pin_data.CWT_length * sizeof(float));
        addresses.push_back((uint64_t) pin_data.addr_CWOT);
        addresses.push_back((uint64_t) pin_data.addr_CWOT + pin_data.CWOT_length * sizeof(float));

        cache = new Cache_system(addresses, pin_data.num_cores, 0.1, 5);
        

    }
    PIN_ReleaseLock(&pinLock);

    printf("Thread End %i\n", threadId);

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
