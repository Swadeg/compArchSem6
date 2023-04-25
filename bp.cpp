/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <iostream>



class branchPredictor {
	
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int isShare; // 0-not_using_share, 1- using_share_lsb, 2- using_share_mid
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
	getGlobalHist();
	getGlobalTable();
	setGlobalHist(bool taken);	
};



branchPredictor bp();

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	
	bp = branchPredictor(btbSize, historySize, tagSize, fsmState, isGlobalHist, isGlobalTable, Shared);
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	
	bool taken = false;
	int idx = 0;
	uint32_t it = find(bp.tag_vector.begin(),bp.tag_vector.end(), pc);//update pc to tag
	
	//local table
	if (!bp.getGlobalTable())
	{
		if ( it != bp.tag_vector.end())
		{
			idx = it-bp.tag_vector.begin();
			//local hist
			if (!bp.getGlobalHist())
				taken =  bp.fsm_table[bp.history_vector[idx]] > 1 ;
			else
				taken =  bp.fsm_table[bp.getGlobalHist()] > 1 ;
		}
	}
	//global table
	else
	{
		if ( it != bp.tag_vector.end())
		{
			idx = it-bp.tag_vector.begin();
			//local hist
			if (!bp.getGlobalHist())
				taken =  bp.getGlobalTable()[bp.history_vector[idx]] > 1 ;
			else
				taken =  bp.getGlobalTable()[bp.getGlobalHist()] > 1 ;
		}
		
	}
	
	if (taken)
	{
		*dst =  bp.target_vector[idx];
		return true;
	}
	
	*dst = pc+4;
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	
	int idx = 0;
	int l;
	uint32_t it = find(bp.tag_vector.begin(),bp.tag_vector.end(), pc);//update pc to tag
	
	//local table
	if (!bp.getGlobalTable())
	{
		if ( it != bp.tag_vector.end())
		{
			idx = it-bp.tag_vector.begin();

			//local hist
			if (!bp.getGlobalHist())
			{
				l = bp.fsm_table[bp.history_vector[idx]];
				bp.fsm_table[bp.history_vector[idx]] += (taken?min(1,l<3):max(-1,-2*(l>0)));// make more elegant
			}
				
			//global hist
			else
			{
				l = bp.fsm_table[bp.history_vector[idx]];
				bp.fsm_table[bp.getGlobalHist()] += (taken?min(1,l<3):max(-1,-2*(l>0)));// make more elegant
				
			}
				
		}
		//pc does not exist
		else
		{
			bp.tag_vector.push_back(pc); //update pc to tag
			//local hist
			if (!bp.getGlobalHist())
			{
				bp.history_vector.push_back((uint32_t)taken);
			}
			//global hist
			else
				bp.setGlobalHist(taken);
		}
		
	}
	
	//global table
	else
	{
		if ( it != bp.tag_vector.end())
		{
			idx = it-bp.tag_vector.begin();
			//local hist
			if (!bp.getGlobalHist())
			{
				l = bp.getGlobalTable()[bp.history_vector[idx]];
				bp.getGlobalTable[bp.history_vector[idx]] += (taken?min(1,l<3):max(-1,-2*(l>0)));// make more elegant
			}
			else
			{
				l = bp.getGlobalTable()[bp.getGlobalHist()];
				bp.getGlobalTable()[bp.getGlobalHist()] += (taken?min(1,l<3):max(-1,-2*(l>0)));// make more elegant
				
			}
		}
		
		//pc does not exist
		else
		{
			bp.tag_vector.push_back(pc); //update pc to tag
			//local hist
			if (!bp.getGlobalHist())
			{
				bp.history_vector.push_back((uint32_t)taken);
			}
			//global hist
			else
				bp.setGlobalHist(taken);
		}
		
	}
	
	bp.target_vector[idx] = targetPc;
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

