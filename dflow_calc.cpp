/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <vector>
#include <iostream>

#define REGS_NUM 32
#define MAX_DEP_NUM 2
#define ENTRY -2

using namespace std;

class Prog{
    vector<vector<int>> dependencies_vec; // each inst dependecies
    vector<int> not_exit_vec; // 0-means no inst that depends on inst i
    int reg_arr[REGS_NUM]; // latest inst that modefied reg i 
    unsigned int *opsLatency_;
    unsigned int numOfInsts_;
    InstInfo *progTrace_;
    int entry_idx;
    int exit_idx;

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
    not_exit_vec = vector<int>(numOfInsts_, 0); 
    dependencies_vec = vector<vector<int>>(numOfInsts_, vector<int>(MAX_DEP_NUM, -1)); // each inst depend on 2 inst max
    entry_idx = ENTRY; // first inst in the trace is the entry 
    exit_idx = -1;
    for(unsigned int i=0; i<numOfInsts_; i++)
    {
        dependencies_vec[i] = vector<int>(MAX_DEP_NUM, -1);
    }
    for(unsigned int i=0; i<REGS_NUM; i++)
    {
        reg_arr[i] = -1; // means at begin, no insts that changed any register
    }
}

void Prog::myAnalyzeProg()
{
    int dst;
    unsigned int src1;
    unsigned int src2;
    int LIidx1; // latest inst idx that changed src1
    int LIidx2; // latest inst idx that changed src2
    for(unsigned int i=0; i<numOfInsts_; i++)
    {
        dst = progTrace_[i].dstIdx;
        src1 = progTrace_[i].src1Idx;
        src2 = progTrace_[i].src2Idx;
        LIidx1 = reg_arr[src1];
        LIidx2 = reg_arr[src2];
        //check "distance"
        if((LIidx1 != -1) && (i-LIidx1 <= opsLatency_[progTrace_[LIidx1].opcode]))
        {
            //means inst number i depends on inst number LIidx1;
            dependencies_vec[i].push_back(LIidx1);
            not_exit_vec[LIidx1] = 1; // inst LIidx1 cant be exit
        }
        if((LIidx2 != -1) && (i-LIidx2 <= opsLatency_[progTrace_[LIidx2].opcode]))
        {
            //means inst number i depends on inst number LIidx2;
            dependencies_vec[i].push_back(LIidx2);
            not_exit_vec[LIidx2] = 1; // inst LIidx2 cant be exit
        }
        reg_arr[dst] = i; // mark that inst number i changed reg dst
    }
    //get exit_idx;
    for(unsigned int i=0; i<numOfInsts_; i++)
    {
        if(not_exit_vec[i] == 0)
        {
            exit_idx = i;
            break;
        }
    }    
}

int Prog::myGetInstDepth(unsigned int instIdx)
{
    int depthToReturn = 0;
    int dependency_idx;
    if(instIdx<0 || instIdx>numOfInsts_) return -1; // invalid inst index
    for(unsigned int i=0; i<dependencies_vec[instIdx].size(); i++)
    {
        dependency_idx = dependencies_vec[instIdx][i];
        if(dependency_idx != -1)
        {
            //get the latency of the opcode of the dependency
            depthToReturn += opsLatency_[progTrace_[dependency_idx].opcode];
        }
    }
    return depthToReturn;
}

int Prog::myGetInstDeps(unsigned int instIdx, int *src1DepInst, int *src2DepInst)
{
    if(instIdx<0 || instIdx>numOfInsts_) return -1; // invalid inst index
    src1DepInst = &dependencies_vec[instIdx][0];
    src2DepInst = &dependencies_vec[instIdx][1];
    return 0; //success
}

int Prog::myGetProgDepth()
{
    return myGetInstDepth(exit_idx);
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
    //return -1;
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    return ((Prog*)ctx) -> myGetInstDeps(theInst, src1DepInst, src2DepInst);
    //return -1;
}

int getProgDepth(ProgCtx ctx) {
    return ((Prog*)ctx) -> myGetProgDepth();
    //return 0;
}


