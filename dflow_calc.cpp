/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <vector>
#include <iostream>

#define REGS_NUM 32
#define MAX_DEP_NUM 2
#define ENTRY -1

using namespace std;

class Prog{
    vector<vector<int>> dependencies_vec; // each inst dependecies
    vector<unsigned int> depth;
    int reg_arr[REGS_NUM]; // latest inst that modefied reg i 
    unsigned int *opsLatency_;
    unsigned int numOfInsts_;
    InstInfo *progTrace_;

    public:
        Prog() = default;
        Prog(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts);
        ~Prog() = default;
        void myAnalyzeProg(); 
        int myGetInstDepth(unsigned int instIdx);
        int myGetInstDeps(unsigned int instIdx, int *src1DepInst, int *src2DepInst);
        int myGetProgDepth();
};

Prog::Prog(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts)
{
    opsLatency_ = (unsigned int*)opsLatency; // forbiddin to change the content of these 2 arrays
    progTrace_ = (InstInfo*)progTrace;
    numOfInsts_ = numOfInsts;
    depth = vector<unsigned int>(numOfInsts_,0);
    dependencies_vec = vector<vector<int>>(numOfInsts_, vector<int>(MAX_DEP_NUM, ENTRY)); // each inst depend on 2 inst max
    // each inst at first points to entry
    for(unsigned int i=0; i<numOfInsts_; i++){
        dependencies_vec[i] = vector<int>(MAX_DEP_NUM, ENTRY);}

    for(unsigned int i=0; i<REGS_NUM; i++){
        reg_arr[i] = -1;} // means at begin, no insts that changed any register

    
}

void Prog::myAnalyzeProg()
{
    int dst;
    unsigned int src1, src2;
    int LIidx1, LIidx2; // latest inst idx that changed srcs
    unsigned int depth1, depth2; //dependency's depths
    for(unsigned int i=0; i<numOfInsts_; i++)
    {
        dst = progTrace_[i].dstIdx;
        src1 = progTrace_[i].src1Idx;
        src2 = progTrace_[i].src2Idx;
        LIidx1 = reg_arr[src1];
        LIidx2 = reg_arr[src2];
        depth1 = 0,depth2=0;
        //check "distance"
        if(LIidx1 != -1)
        {//means inst number i depends on inst number LIidx1;
            dependencies_vec[i][0] = LIidx1;
            depth1=depth[LIidx1]+opsLatency_[progTrace_[LIidx1].opcode];
        }
        if(LIidx2 != -1)
        {//means inst number i depends on inst number LIidx2;
            dependencies_vec[i][1] = LIidx2;
            depth2 = depth[LIidx2]+opsLatency_[progTrace_[LIidx2].opcode];;
        }
        reg_arr[dst] = i; // mark that inst number i changed reg dst
        depth[i] = max(depth1,depth2);
    }
}

int Prog::myGetInstDepth(unsigned int instIdx)
{
    return depth[instIdx];
}

int Prog::myGetInstDeps(unsigned int instIdx, int *src1DepInst, int *src2DepInst)
{
    if(instIdx<0 || instIdx>numOfInsts_) return -1; // invalid inst index
    *src1DepInst = dependencies_vec[instIdx][0];
    *src2DepInst = dependencies_vec[instIdx][1];
    return 0; //success
}

int Prog::myGetProgDepth()
{
    unsigned int max_prog_depth = 0;
    for (unsigned i = 0; i < numOfInsts_; i++)
    {
        max_prog_depth = max(max_prog_depth,
        depth[i]+opsLatency_[progTrace_[i].opcode]);
    }
    return (int)max_prog_depth;
}

///////////////////////// manage the adt of the analyzer ////////////////////////////////////
ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    Prog *p = new Prog(opsLatency, progTrace, numOfInsts);
    p->myAnalyzeProg();
    return p;
    return PROG_CTX_NULL;
}

void freeProgCtx(ProgCtx ctx) {
    delete ((Prog*)ctx);
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    return ((Prog*)ctx) -> myGetInstDepth(theInst);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    return ((Prog*)ctx) -> myGetInstDeps(theInst, src1DepInst, src2DepInst);
}

int getProgDepth(ProgCtx ctx) {
    return ((Prog*)ctx) -> myGetProgDepth();
}