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
	uint32_t global_history;
	vector<uint32_t> tag_vector;
	vector<uint32_t> history_vector;
	vector<uint32_t> target_vector;
	vector<int> global_fsm_table;
	vector<vector<int>> fsm_table;  
	
public:
	branchPredictor(){};
	branchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	~branchPredictor(){};
	bool predict(uint32_t pc, uint32_t *dst);
	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);
	/*add methods here*/
	uint32_t getSharedHistory(uint32_t pc, uint32_t history);
	uint32_t getTagFromPc(uint32_t pc);
	int getTagIdx(uint32_t pc, uint32_t tag); /*return -1 if tag doesnt exist else returns tagIdx*/
	uint32_t getTargetFromTagIdx(int tagIdx); /*assume tag is exist in btb*/
	void updateTarget(int tagIdx, uint32_t targetPc);
	void updateFsm(uint32_t pc, int tagIdx, bool taken);
	void insertNewBranch(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst, int tagIdx, uint32_t tag);
	void updateHistory( int tagIdx, bool taken);
	bool isTaken(uint32_t pc, int tagIdx);
};



//class methods

branchPredictor::branchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
		bool isGlobalHist, bool isGlobalTable, int Shared)
{
	btbSize = btbSize;
	historySize = historySize;
	tagSize = tagSize;
	fsmState = fsmState;
	isGlobalHist = isGlobalHist;
	isGlobalTable = isGlobalTable;
	Shared = Shared;
	global_history = 0;
	tag_vector = vector<uint32_t>(btbSize,0);
	history_vector = vector<uint32_t>(btbSize,0);
	target_vector = vector<uint32_t>(btbSize,0);
	global_fsm_table = vector<int>(btbSize,0);
	fsm_table = vector<vector<int>>(btbSize,vector<int>(pow(2,historySize), fsmState));
}

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
	return -1;
}

uint32_t branchPredictor::getTagFromPc(uint32_t pc)
{
	return pc >> ( 2 + (int)log2(btbSize) ); /*2 is due to 2 zeroes in 2 lsb bits*/
}

int branchPredictor::getTagIdx(uint32_t pc , uint32_t tag) /*return -1 if tag doesnt exist else returns tagIdx*/
{
	uint32_t mask = pow(2,log2(btbSize)) - 1;
	pc >>= 2;
	int idx = pc & mask;
	if ( tag_vector[idx]==tag )
	{
		return idx;
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

void branchPredictor::insertNewBranch(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst, int tagIdx, uint32_t tag)
{
	/*replacement or inserting*/
	tag_vector[tagIdx] = tag;
	target_vector[tagIdx] = targetPc;
	if ( !isGlobalHist )
	{
		history_vector[tagIdx] = 0;
	}
	if ( !isGlobalTable )
	{
		fsm_table[tagIdx][history_vector[tagIdx]] = fsmState;
	}
	updateHistory(tagIdx, taken);
	updateFsm(pc, tagIdx, taken);	
}

bool branchPredictor::isTaken(uint32_t pc, int tagIdx)
{
	int fsmState;
	if ( isGlobalTable && isGlobalHist)  
	{
		uint32_t sharedHistory = getSharedHistory(pc, global_history); //depends on isShare
		fsmState = global_fsm_table[ sharedHistory ];			
	}
	else if ( isGlobalTable && !isGlobalHist ) 
	{
		uint32_t sharedHistory = getSharedHistory(pc, history_vector[tagIdx]); //depends on isShare
		fsmState = global_fsm_table[ sharedHistory ];		
	}
	else if ( !isGlobalTable && isGlobalHist )
	{
		fsmState = fsm_table[tagIdx][ global_history ];
	}
	else if ( !isGlobalTable && !isGlobalHist )
	{
		fsmState = fsm_table[tagIdx][ history_vector[tagIdx] ];
	}
	return ( fsmState==2 || fsmState==3 );
}

bool branchPredictor::predict(uint32_t pc, uint32_t *dst)
{
	uint32_t tag = getTagFromPc(pc);
	int tagIdx = getTagIdx(pc, tag);
	
	if( (tagIdx!=-1) && isTaken(pc, tagIdx) ) 
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
	int tagIdx = getTagIdx(pc, tag);
	if( tagIdx != -1 ) /*pc exist in btb*/ 
	{
		updateTarget(tagIdx, targetPc);
		updateHistory(tagIdx, taken);
		updateFsm(pc, tagIdx, taken);
	}
	else /*pc does not exist in btb: may have to replace or insert*/
	{
		insertNewBranch(pc, targetPc, taken, pred_dst, tagIdx, tag);
	}
}






////////////////////////////////////////////////////////////////////////////////////

branchPredictor bp(0,0,0,0,0,0,0);


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

