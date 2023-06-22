/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <stdio.h>

using namespace std;

double CPI;

typedef enum {
	IDLE=0,BUSY=1,HALT=2
}thread_state;

class thread
{
	thread_state state;
	Instruction inst;
	int finish_cycle;
	tcontext thread_reg;
	unsigned line;

	public:
		thread();
		unsigned get_line();
		Instruction get_inst();
		void update_state(unsigned cycle);
		void execute_inst();
};

void thread::update_state(unsigned cycle){
	if (state==BUSY){
		if (cycle > finish_cycle){
		state = IDLE;
		}
	}
	else{
		//switch busy , halt
	}
	
}

void set_idle(unsigned cycle, vector<thread> *thread_vec_)
{
	for(int i=0; i<thread_vec_->size(); i++)
	{
		(*thread_vec_)[i].update_state(cycle);
	}
}

int check_idle(unsigned int* thread_id,vector<thread> *thread_vec_, bool is_blocked, unsigned *cycle){ //return -1 if no free thread
	// if is_blocked (*cycle)++
}


void CORE_BlockedMT() {
	int num_threads = SIM_GetThreadsNum();
	thread init_thread = thread();
	vector<thread> thread_vec = vector<thread>(num_threads, init_thread);
	
	unsigned halted_threads_num = 0;
	unsigned cycle = 0, instructions=0;
	unsigned thread_id = 0;

	while(halted_threads_num != num_threads)
	{
		set_idle(cycle, &thread_vec); // free busy threads
		// Check for idle threads
		if (check_idle(&thread_id, &thread_vec, true, &cycle)!= -1){
			SIM_MemInstRead(thread_vec[thread_id].get_line(), &thread_vec[thread_id].get_inst(), thread_id);
			thread_vec[thread_id].execute_inst();
			instructions++;
		}
		
		cycle++;
	}
	CPI = (double)cycle / instructions;
}

void CORE_FinegrainedMT() {
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
