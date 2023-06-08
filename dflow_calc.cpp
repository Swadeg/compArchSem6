/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"

using namespace std;

#define MAX_REGISTERS

class Prog{
    Prog** dependencies;
    unsigned int num_dependencies;
    int changedIdx;
    unsigned int run_time;
    public:
        Prog() {}
        Prog(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts)
        ~Prog() {}
};

//define entry - Prog with no dependencies

// define global array with Register depenencies. It is updated
// whenever we perform a new command

Prog::Prog(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts){
    // Look in register dependency array and connect the Prog
    // to previous progs that are located in the array
    // save runtime


    //check for all programs that dont have multiple dependencies and connect to exit
    //define exit which has no program but connects to all dependencies in array
    // check for multiple registers that point to the same Prog
}

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    Prog *p = new Prog(opsLatency[],progTrace,);


    return prog;
    return PROG_CTX_NULL;
}

void freeProgCtx(ProgCtx ctx) {
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    return -1;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    return -1;
}

int getProgDepth(ProgCtx ctx) {
    return 0;
}


