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
	
	unsigned btbSize_;
	unsigned historySize_;
	unsigned tagSize_;
	unsigned fsmState_;
	bool isGlobalHist_;
	bool isGlobalTable_;
	int isShare_; /*0-not_using_share, 1- using_share_lsb, 2- using_share_mid*/
	SIM_stats stats_; 
	uint32_t global_history_;
	vector<uint32_t> tag_vector_;
	vector<bool> valid_vector_;
	vector<uint32_t> history_vector_;
	vector<uint32_t> target_vector_;
	vector<int> global_fsm_table_;
	vector<vector<int>> fsm_table_;  
	
public:
	branchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared);
	branchPredictor(branchPredictor& cpy) = default;
	~branchPredictor() {};
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
	unsigned getBrNum();
	unsigned getBtbSize();
	unsigned getFlushNum();
};



//class methods

branchPredictor::branchPredictor(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
		bool isGlobalHist, bool isGlobalTable, int Shared)
{
	btbSize_ = btbSize;
	historySize_ = historySize;
	tagSize_ = tagSize;
	fsmState_ = fsmState;
	isGlobalHist_ = isGlobalHist;
	isGlobalTable_ = isGlobalTable;
	isShare_ = Shared;
	stats_.flush_num = 0;
	stats_.br_num = 0;
	stats_.size = 0;
	global_history_ = 0;
	tag_vector_ = vector<uint32_t>(btbSize,0);
	history_vector_ = vector<uint32_t>(btbSize,0);
	target_vector_ = vector<uint32_t>(btbSize,0);
	valid_vector_ = vector<bool>(btbSize,false);
	global_fsm_table_ = vector<int>(pow(2,historySize),fsmState);
	fsm_table_.resize(btbSize, vector<int>(pow(2,historySize), fsmState));

	/*for (uint32_t i = 0; i < btbSize_; i++)
	{
		valid_vector_ [i]= false;
	}*/
	
}

uint32_t branchPredictor::getSharedHistory(uint32_t pc, uint32_t history)
{
	if ( isShare_==0 ) /*not_using_share*/
	{
		return history;
	}
	if ( isShare_==1 ) /*using_share_lsb*/
	{
		return ((pc>>2)^(history)) & (uint32_t)(pow(2,historySize_)-1);
	}
	if ( isShare_==2 ) /*using_share_mid*/
	{
		return ((pc>>16)^(history)) & (uint32_t)(pow(2,historySize_)-1);
	}
	return -1;
}

uint32_t branchPredictor::getTagFromPc(uint32_t pc)
{
	return pc >> ( 2 + (int)log2(btbSize_) ); /*2 is due to 2 zeroes in 2 lsb bits*/

}

int branchPredictor::getTagIdx(uint32_t pc , uint32_t tag) /*return -1 if tag doesnt exist else returns tagIdx*/
{
	uint32_t mask = btbSize_;
	uint32_t shiftedPc = pc>>2;
	int idx = shiftedPc & (mask-1);
	return idx;	
}

uint32_t branchPredictor::getTargetFromTagIdx(int tagIdx) /*assume tag is exist in btb*/
{
	return target_vector_[tagIdx];
}

void branchPredictor::updateTarget(int tagIdx, uint32_t targetPc)
{
	target_vector_[tagIdx] = targetPc;
}

void branchPredictor::updateHistory( int tagIdx, bool taken)
{
	if ( isGlobalHist_ )
	{
		global_history_ <<= 1;
		global_history_ += (int)taken;
		global_history_ = global_history_ & (uint32_t)(pow(2,historySize_)-1);
	}
	else /*local hist*/
	{		
		history_vector_[tagIdx]  <<= 1;
		history_vector_[tagIdx]  += (int)taken;
		history_vector_[tagIdx] = history_vector_[tagIdx] & (uint32_t)(pow(2,historySize_)-1);
	}
}

void branchPredictor::updateFsm(uint32_t pc, int tagIdx, bool taken)
{
	int State,change=0;
	uint32_t sharedHistory;
	if ( isGlobalTable_ && isGlobalHist_) {
		sharedHistory = getSharedHistory(pc, global_history_); //depends on isShare
		State = global_fsm_table_[ sharedHistory ];
	}
	else if ( isGlobalTable_ && !isGlobalHist_ ) {
		sharedHistory = getSharedHistory(pc, history_vector_[tagIdx]); //depends on isShare
		State = global_fsm_table_[ sharedHistory ];
	}
		

	else if ( !isGlobalTable_ && isGlobalHist_ ){
		State = fsm_table_[tagIdx][ global_history_ ];
	} 
		

	else if ( !isGlobalTable_ && !isGlobalHist_ ){

		State = fsm_table_[tagIdx][ history_vector_[tagIdx] ];
	}
	
	if (taken){
		if (State<3) change =1;
	}
	else if(State>0) change = -1;

	if ( isGlobalTable_ && isGlobalHist_){
		global_fsm_table_[getSharedHistory(pc, global_history_)]+=change;
	}
	else if ( isGlobalTable_ && !isGlobalHist_ ) {
		global_fsm_table_[ getSharedHistory(pc, history_vector_[tagIdx])]+=change;
	}
	else if ( !isGlobalTable_ && isGlobalHist_ ){
		fsm_table_[tagIdx][ global_history_ ]+= change;
	}
	else if ( !isGlobalTable_ && !isGlobalHist_ ){
		fsm_table_[tagIdx][ history_vector_[tagIdx] ]+=change;
	}
}

void branchPredictor::insertNewBranch(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst, int tagIdx, uint32_t tag)
{
	/*replacement or inserting*/
	tag_vector_[tagIdx] = tag;
	target_vector_[tagIdx] = targetPc;
	valid_vector_[tagIdx] = true;
	if ( !isGlobalHist_ )
		history_vector_[tagIdx] = 0;

	//if ( !isGlobalTable_ && !isGlobalHist_ ){
	if ( !isGlobalTable_ ){
		fsm_table_[tagIdx] = vector<int>(pow(2,historySize_),fsmState_);
	}
		//fsm_table_[tagIdx][ history_vector_[tagIdx] ] = fsmState_;

	updateFsm(pc, tagIdx, taken);
	updateHistory(tagIdx, taken);
}

bool branchPredictor::isTaken(uint32_t pc, int tagIdx)
{
	int State;
	uint32_t tableIndex;
	if (isGlobalHist_){
		tableIndex= getSharedHistory(pc, global_history_);
	}
	else{
		tableIndex = getSharedHistory(pc,history_vector_[tagIdx]);
	}
	State = (isGlobalTable_)?global_fsm_table_[tableIndex]:fsm_table_[tagIdx][tableIndex];

	return State>=2;
	
}

bool branchPredictor::predict(uint32_t pc, uint32_t *dst)
{
	uint32_t tag = getTagFromPc(pc);
	int tagIdx = getTagIdx(pc, tag);
	
	if( (tag_vector_[tagIdx]==tag) && isTaken(pc, tagIdx) && valid_vector_[tagIdx]==true) 
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
	
	uint32_t tag = getTagFromPc(pc);
	int tagIdx = getTagIdx(pc, tag); //getting tag ID pc from btb
if( tag_vector_[tagIdx]==tag && valid_vector_[tagIdx]) /*pc exist in btb*/ 
	{
		updateTarget(tagIdx, targetPc);
		updateFsm(pc, tagIdx, taken);
		updateHistory(tagIdx, taken);
	}
	else /*pc does not exist in btb: may have to replace or insert*/
		insertNewBranch(pc, targetPc, taken, pred_dst, tagIdx, tag);

	stats_.br_num++; // New Branch, Taken or not

	if ((taken && (pred_dst!=targetPc))|| (!taken && (pred_dst != pc+4))) //Finding when we need to flush
		stats_.flush_num++;

}

unsigned branchPredictor::getFlushNum()
{
	return stats_.flush_num;
}
unsigned branchPredictor::getBrNum()
{
	return stats_.br_num;
}
unsigned branchPredictor::getBtbSize()
{
	unsigned targetSize = 30;
	unsigned historySize = historySize_;
	unsigned fsmSize = pow(2,historySize) * 2;
	unsigned validBitSize = 1;
	stats_.size = btbSize_*(validBitSize+targetSize+tagSize_);

	if(isGlobalHist_){
		stats_.size += historySize_;
	}
	else{
		stats_.size+= btbSize_*historySize_;
	}
	
	if (isGlobalTable_){
		stats_.size+= fsmSize;
	}
	else{
		stats_.size+= btbSize_*fsmSize;
	}
	return stats_.size;
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
	curStats->size = bp.getBtbSize();
	curStats->br_num = bp.getBrNum();
	curStats->flush_num = bp.getFlushNum();
	return;
}