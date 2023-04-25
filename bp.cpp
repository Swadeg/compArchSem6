/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <iostream>

using namespace std;

class branchPredictor {
	
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int isShare; /*0-not_using_share, 1- using_share_lsb, 2- using_share_mid*/
	SIM_stats stats; 
	vector<uint32_t> tag_vector;
	vector<uint32_t> history_vector;
	vector<uint32_t> target_vector;
	vector<vector<int>> fsm_table;  
	uint32_t global_history;
	vector<int> global_fsm_table;
	
public:
	branchPredictor();
	branchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	~branchPredictor();
	bool predict(uint32_t pc, uint32_t *dst);
	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
	/*add methods here*/
	uint32_t getSharedHistory(uint32_t pc, uint32_t history);
	uint32_t getTagFromPc(uint32_t pc);
	int getTagIdx(uint32_t tag); /*return -1 if tag doesnt exist else returns tagIdx*/
	uint32_t getTargetFromTagIdx(int tagIdx); /*assume tag is exist in btb*/
	void updateTarget(int tagIdx, uint32_t targetPc);
	void updateFsm(uint32_t pc, int tagIdx, bool taken);
	void insertNewBranch(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
	void updateHistory( int tagIdx, bool taken);
};



//class methods

uint32_t branchPredictor::getSharedHistory(uint32_t pc, uint32_t history)
{
	if ( isShare==0 ) /*not_using_share*/
	{
		return history;
	}
	if ( isShare==1 ) /*using_share_lsb*/
	{
		return ((pc>>2)^(history)) & historySize;
	}
	if ( isShare==2 ) /*using_share_mid*/
	{
		return ((pc>>16)^(history)) & historySize;
	}
}

uint32_t branchPredictor::getTagFromPc(uint32_t pc)
{
	return pc >> ( 2 + (int)log2(btbSize) ); /*2 is due to 2 zeroes in 2 lsb bits*/
}

int branchPredictor::getTagIdx(uint32_t tag) /*return -1 if tag doesnt exist else returns tagIdx*/
{
	uint32_t it = find(tag_vector.begin(),tag_vector.end(), tag);
	if (it != tag_vector.end())
	{
		return ( it-tag_vector.begin() );
	}
	return -1;
	
}

uint32_t branchPredictor::getTargetFromTagIdx(int tagIdx) /*assume tag is exist in btb*/
{
	return target_vector[tagIdx];
}

void branchPredictor::updateTarget(int tagIdx, uint32_t targetPc)
{
	target_vector[tagIdx] = targetPc;
}

void branchPredictor::updateHistory( int tagIdx, bool taken)
{
	if ( isGlobalHist )
	{
		global_history <<= 1;
		global_history += (int)taken; // may have to do & with historySize
	}
	else /*local hist*/
	{		
		history_vector[tagIdx]  <<= 1;
		history_vector[tagIdx]  += (int)taken; // may have to do & with historySize
	}
}

void branchPredictor::updateFsm(uint32_t pc, int tagIdx, bool taken)
{
	if ( isGlobalTable && isGlobalHist)  
	{
		uint32_t sharedHistory = getSharedHistory(pc, global_history); //depends on isShare
		int fsmState = global_fsm_table[ sharedHistory ];
		if ( fsmState==1 || fsmState==2 )  { global_fsm_table[ sharedHistory ] += (taken?1:-1); }
		else if ( fsmState==0 ) { global_fsm_table[ sharedHistory ] += (taken?1:0); }
		else if ( fsmState==3 ) { global_fsm_table[ sharedHistory ] += (taken?0:-1); }			
		
	}
	else if ( isGlobalTable && !isGlobalHist ) 
	{
		uint32_t sharedHistory = getSharedHistory(pc, history_vector[tagIdx]); //depends on isShare
		int fsmState = global_fsm_table[ sharedHistory ];
		if ( fsmState==1 || fsmState==2 )  { global_fsm_table[ sharedHistory ] += (taken?1:-1); }
		else if ( fsmState==0 ) { global_fsm_table[ sharedHistory ] += (taken?1:0); }
		else if ( fsmState==3 ) { global_fsm_table[ sharedHistory ] += (taken?0:-1); }		
		
	}
	else if ( !isGlobalTable && isGlobalHist )
	{
		int fsmState = fsm_table[tagIdx][ global_history ];
		if ( fsmState==1 || fsmState==2 )  { fsm_table[tagIdx][ global_history ] += (taken?1:-1); }
		else if ( fsmState==0 ) { fsm_table[tagIdx][ global_history ] += (taken?1:0); }
		else if ( fsmState==3 ) { fsm_table[tagIdx][ global_history ] += (taken?0:-1); }
	}
	else if ( !isGlobalTable && !isGlobalHist )
	{
		int fsmState = fsm_table[tagIdx][ history_vector[tagIdx] ];
		if ( fsmState==1 || fsmState==2 )  { fsm_table[tagIdx][ history_vector[tagIdx] ] += (taken?1:-1); }
		else if ( fsmState==0 ) { fsm_table[tagIdx][ history_vector[tagIdx] ] += (taken?1:0); }
		else if ( fsmState==3 ) { fsm_table[tagIdx][ history_vector[tagIdx] ] += (taken?0:-1); }
	}
}

void branchPredictor::insertNewBranch(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	
}


bool branchPredictor::predict(uint32_t pc, uint32_t *dst)
{
	uint32_t tag = getTagFromPc(pc);
	int tagIdx = getTagIdx(tag);
	
	if( (tagIdx!=-1) && isTaken(tag) ) 
	{
		*dst = getTargetFromTagIdx(tagIdx);
		return true;
	}
	else /*pc not in btb or is exist but NT*/
	{
		*dst = pc + 4;
		return false;
	}
}


void branchPredictor::update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst)
{
	predict(pc, &pred_dst);
	uint32_t tag = getTagFromPc(pc);
	int tagIdx = getTagIdx(tag);
	if( tagIdx != -1 ) /*pc exist in btb*/
	{
		updateTarget(tagIdx, targetPc);
		updateHistory(tagIdx, taken);
		updateFsm(pc, tagIdx, taken);
	}
	else /*pc does not exist in btb*/
	{
		insertNewBranch(pc, targetPc, taken, pred_dst);
	}
}






////////////////////////////////////////////////////////////////////////////////////

branchPredictor bp();


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	
	bp = branchPredictor(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	return 0;
}


bool BP_predict(uint32_t pc, uint32_t *dst){
	
	return bp.predict(pc, dst);
}


void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	
	bp.update(pc, targetPc, taken, pred_dst);
}


void BP_GetStats(SIM_stats *curStats){
	return;
}

