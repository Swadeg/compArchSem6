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
    dependencies_vec = vector<vector<int>>(numOfInsts_, vector<int>(MAX_DEP_NUM, ENTRY)); // each inst depend on 2 inst max
    entry_idx = 0; // first inst in the trace is the entry 
    exit_idx = -1;
    // each inst at first points to entry
    for(unsigned int i=0; i<numOfInsts_; i++)
    {
        dependencies_vec[i] = vector<int>(MAX_DEP_NUM, ENTRY);
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
        if(LIidx1 != -1)
        {
            //means inst number i depends on inst number LIidx1;
            dependencies_vec[i][0] = LIidx1;
            not_exit_vec[LIidx1] = 1; // inst LIidx1 cant be exit
        }
        if(LIidx2 != -1)
        {
            //means inst number i depends on inst number LIidx2;
            dependencies_vec[i][1] = LIidx2;
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
    int dependency_idx1;
    int dependency_idx2;
    int dep_latency1;
    int dep_latency2;
    if(instIdx<0 || instIdx>numOfInsts_) return -1; // invalid inst index
    dependency_idx1 = dependencies_vec[instIdx][0];
    dependency_idx2 = dependencies_vec[instIdx][1];
    while(dependency_idx1 != ENTRY || dependency_idx2 != ENTRY)//till not arrive to entry
    {
        if(dependency_idx1 == ENTRY)
        {
            // dependency_idx2 != ENTRY
            dep_latency2 = opsLatency_[progTrace_[dependency_idx2].opcode];
            depthToReturn += dep_latency2;
            instIdx = dependency_idx2;
            dependency_idx1 = dependencies_vec[instIdx][0];
            dependency_idx2 = dependencies_vec[instIdx][1];
            continue;
        }
        else if(dependency_idx2 == ENTRY)
        {
            //dependency_idx1 != ENTRY and dependency_idx2 == ENTRY
            dep_latency1 = opsLatency_[progTrace_[dependency_idx1].opcode];
            depthToReturn += dep_latency1;
            instIdx = dependency_idx1;
            dependency_idx1 = dependencies_vec[instIdx][0];
            dependency_idx2 = dependencies_vec[instIdx][1];
            continue;
        }
        else
        {
            //dependency_idx1 != ENTRY and dependency_idx2 != ENTRY
            dep_latency1 = opsLatency_[progTrace_[dependency_idx1].opcode];
            dep_latency2 = opsLatency_[progTrace_[dependency_idx2].opcode];
            depthToReturn += ((dep_latency1 > dep_latency2) ? dep_latency1 : dep_latency2);
            instIdx = ((dep_latency1 > dep_latency2) ? dependency_idx1 : dependency_idx2);
            dependency_idx1 = dependencies_vec[instIdx][0];
            dependency_idx2 = dependencies_vec[instIdx][1];
        }
    }
    return depthToReturn;
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
    int max_prog_depth = 0;
    int prog_depth = 0;
    for(unsigned int i=0; i<not_exit_vec.size(); i++)
    {
        if(not_exit_vec[i] == 0)
        {
            prog_depth = myGetInstDepth(i);
            exit_idx = i;
        }
            
        if (prog_depth > max_prog_depth)
            max_prog_depth = prog_depth;
    } 
    return max_prog_depth + opsLatency_[progTrace_[exit_idx].opcode];
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


