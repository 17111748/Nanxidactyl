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

// #include "cache_system.h"
#include "sample.cpp"

using std::cerr;
using std::ofstream;
using std::ios;
using std::string;
using std::endl;

ofstream OutFile;

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
// Cache_system cache = NULL;

// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 icount = 0;

// This function is called before every instruction is executed    
FILE * trace;
 
// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr)
{
    fprintf(trace,"%p: R %p\n", ip, addr);

    unsigned long buf_addr = 0;

    FILE *f;
    f = fopen("testing.address", "r");
    // buf_addr = 
    fscanf(f, "%lx\n", &buf_addr);
    fclose(f);
    
    if (((unsigned long) addr) == buf_addr) {
        printf("address %lx\n", buf_addr);
    }
    if (((unsigned long) addr) == buf_addr) {
        ((int *) (addr))[0] = 69;
    }
}
 
// Print a memory write record
VOID RecordMemWrite(VOID * ip, VOID * addr)
{
    fprintf(trace,"%p: W %p\n", ip, addr);
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
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
        }
        // Note that in some architectures a single memory operand can be 
        // both read and written (for instance incl (%eax) on IA-32)
        // In that case we instrument it once for read and once for write.
        if (INS_MemoryOperandIsWritten(ins, memOp))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
                IARG_INST_PTR,
                IARG_MEMORYOP_EA, memOp,
                IARG_END);
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
    // Thread 0 is the main thread

    // Thread 1 is the thread to execute code after init
    if (threadId == 1) {
        pin_data = (pin_data_t) ((pin_data_t (*) (void)) funcMap["ret_pin_data"])();
    
        // Create cache system
        std::vector<std::pair<uint64_t, uint64_t>> addresses;
        // addresses.push_back(std::make_pair((uint64_t) pin_data.addr_A, (uint64_t) pin_data.addr_A + pin_data.A_length * sizeof(float)));
        // addresses.push_back(std::make_pair((uint64_t) pin_data.addr_B, (uint64_t) pin_data.addr_B + pin_data.B_length * sizeof(float)));
        // addresses.push_back(std::make_pair((uint64_t) pin_data.addr_CWT, (uint64_t) pin_data.addr_CWT + pin_data.CWT_length * sizeof(float)));
        // addresses.push_back(std::make_pair((uint64_t) pin_data.addr_CWOT, (uint64_t) pin_data.addr_CWOT + pin_data.CWOT_length * sizeof(float)));
        // cache = Cache_system(addresses, pin_data.num_cores, 0.1, 5);
    }
}
/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    std::vector<int> s;
    s.push_back(1);

    std::map<int, int> m;
    Sample k = Sample();
    printf("%i\n", k.ret(s));
    
    // std::tuple<int, int> k;

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    trace = fopen("pinatrace.out", "w");
    
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
