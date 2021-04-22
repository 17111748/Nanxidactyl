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

using std::cerr;
using std::ofstream;
using std::ios;
using std::string;
using std::endl;

ofstream OutFile;

// Address of memory locations that we can speculate on
std::map<std::string, unsigned long> images; 

// The running count of instructions is kept here
// make it static to help the compiler optimize docount
static UINT64 icount = 0;

// This function is called before every instruction is executed    
FILE * trace;
 
// Print a memory read record
VOID RecordMemRead(VOID * ip, VOID * addr)
{
    // RTN addressRtn = RTN_FindByName(img)
    
    
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

void fillImage(IMG img, std::string s) {
    // RTN addressRtn = RTN_FindByName(img, s.c_str());
    RTN addressRtn = RTN_FindByName(img, "matrix_addr_A");

    if (addressRtn.is_valid()) {
        
        std::cout << "Address RTN found" << std::endl;
        RTN_Open(addressRtn);
        AFUNPTR app = RTN_Funptr(addressRtn);

        unsigned long a = ((unsigned long (*) (void)) app)();
        // std::cout << a << std::endl;
        images[s] = a;
        printf("%lx\n", a);
        RTN_Close(addressRtn);

    } else {
        std::cout << "Address RTN not found" << std::endl;
    }
}

void Image(IMG img, VOID *v) {

    for( SYM sym= IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym) ) {
        // printf("SYMBOL %s\n", PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY).c_str());
        if (PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY) == "matrix_addr_A") {
            printf("FOUND!\n");
            unsigned long addr = SYM_Address(sym);
            printf("%lx\n", addr);
            unsigned long a = ((unsigned long (*) (void)) addr)();
            printf("%lx\n", a);

            RTN addressRtn = RTN_FindByName(img, SYM_Name(sym).c_str());

            if (addressRtn.is_valid()) {
                
                std::cout << "Address RTN found" << std::endl;
                RTN_Open(addressRtn);
                AFUNPTR app = RTN_Funptr(addressRtn);

                unsigned long a = ((unsigned long (*) (void)) app)();
                // std::cout << a << std::endl;
                // images[s] = a;
                printf("%lx\n", a);
                RTN_Close(addressRtn);

            } else {
                std::cout << "Address RTN not found" << std::endl;
            }

        }
    }

    // unsigned long buf_addr;
    // FILE *f;
    // f = fopen("testing.address", "r");
    // // buf_addr = 
    // fscanf(f, "%lx\n", &buf_addr);
    // fclose(f);
    // std::string name = RTN_FindNameByAddress(buf_addr);
    // printf("Name %s\n", name.c_str());
    
    // RTN addressRtn = RTN_FindByName(img, "Address");

    // if (addressRtn.is_valid()) {
        
    //     std::cout << "Address RTN found" << std::endl;
    //     RTN_Open(addressRtn);
    //     AFUNPTR app = RTN_Funptr(addressRtn);

    //     unsigned long a = ((unsigned long (*) (void)) app)();
    //     // std::cout << a << std::endl;
    //     printf("%lx\n", a);
    //     RTN_Close(addressRtn);

    // } else {
    //     std::cout << "Address RTN not found" << std::endl;
    // }
    // fillImage(img, "matrix_addr_A");
    // fillImage(img, "matrix_addr_B");
    // fillImage(img, "matrix_addr_CWT");
    // fillImage(img, "matrix_addr_CWOT");
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    
    // This doesn't work
    // FILE *f;
    // f = fopen("testing.address", "r");
    // // buf_addr = 
    // fscanf(f, "%lx\n", &buf_addr_global);
    // fclose(f);
    // printf("buf address %lx\n", buf_addr_global);


    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    trace = fopen("pinatrace.out", "w");
    
    PIN_InitSymbols();
    // for( IMG img= APP_ImgHead(); IMG_Valid(img); img = IMG_Next(img) ) {
    //     printf("HELLO");
    //     Image(img, NULL);
    // } 

    // Catches the loading of an IMAGE
    IMG_AddInstrumentFunction(Image, NULL);

    OutFile.open(KnobOutputFile.Value().c_str());

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
