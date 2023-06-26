/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <vector>
#include <stdio.h>
#include <iostream>

using namespace std;

typedef enum {
	IDLE=0,BUSY=1,HALT=2
}thread_state;

class thread
{
	thread_state state;
	Instruction inst;
	int finish_cycle;
	tcontext thread_reg;
	uint32_t line;
	int thread_id;

	public:
		thread() {};
		thread(int _thread_id, int cycle);
		~thread() = default;
		thread_state get_state() {return state;};
		int get_reg_value(int i) {return thread_reg.reg[i];};
		void update_state_regard_cyc(int cycle);
		void execute_inst(int *sw_flag, int cycle);
		cmd_opcode get_opcode() {return inst.opcode;}; 
};

double blocked_CPI;
double fine_grained_CPI;
vector<thread*> blocked_threads_vec;
vector<thread*> fine_grained_threads_vec;

thread::thread(int _thread_id, int cycle)
{
	state = IDLE;
	thread_id = _thread_id;
	line = 0;
	SIM_MemInstRead(line, &inst, thread_id);
	for (int i=0; i<REGS_COUNT; i++)
	{
		thread_reg.reg[i] = 0;
	}
	if (inst.opcode == CMD_LOAD) finish_cycle = cycle + SIM_GetLoadLat();
	else if (inst.opcode == CMD_STORE) finish_cycle = cycle + SIM_GetStoreLat();
	else finish_cycle = cycle + 1;
	
}

void thread::execute_inst(int *sw_flag, int cycle)
{
	//updates thread reg file
	//updates thread state 
	//updates switch flag if needed
	switch (inst.opcode)
	{
	case CMD_NOP:{
			break;
		}
	case CMD_ADD:{
			thread_reg.reg[inst.dst_index] = thread_reg.reg[inst.src1_index] +
											 thread_reg.reg[inst.src2_index_imm];
			//the state still idle
			break;
		}
	case CMD_ADDI:{
			thread_reg.reg[inst.dst_index] = thread_reg.reg[inst.src1_index] +
											 inst.src2_index_imm;
			//the state still idle
			break;
		}	
	case CMD_SUB:{
			thread_reg.reg[inst.dst_index] = thread_reg.reg[inst.src1_index] -
											 thread_reg.reg[inst.src2_index_imm];
			//the state still idle
			break;
		}	
	case CMD_SUBI:{
			thread_reg.reg[inst.dst_index] = thread_reg.reg[inst.src1_index] -
											 inst.src2_index_imm;
			//the state still idle
			break;
		}
	case CMD_LOAD:{
			int src2 = inst.isSrc2Imm ? inst.src2_index_imm : thread_reg.reg[inst.src2_index_imm];
			int32_t* dst = &(thread_reg.reg[inst.dst_index]);
			uint32_t address = thread_reg.reg[inst.dst_index] + src2;
			SIM_MemDataRead(address, dst);
			state = BUSY;
			*sw_flag = 1;
			finish_cycle = cycle + SIM_GetLoadLat();
			break;
		}
	case CMD_STORE:{
			int src2 = inst.isSrc2Imm ? inst.src2_index_imm : thread_reg.reg[inst.src2_index_imm];
			int32_t val = thread_reg.reg[inst.src1_index];
			uint32_t address = thread_reg.reg[inst.src1_index] + src2;
			SIM_MemDataWrite(address, val);	
			state = BUSY;
			*sw_flag = 1;	
			finish_cycle = cycle + SIM_GetStoreLat();	
			break;
		}
	case CMD_HALT:{
			state = HALT;
			*sw_flag = 1;
			break;
		}
	default:
		break;
	}
	line++;
	SIM_MemInstRead(line, &inst, thread_id);

}

int get_thread_id_by_RR(int num_threads, int thread_id_, bool isBlockedMT)
{
	//returns next idle thread id, -1 if all the threads are halted , otherwise -2
	int i = thread_id_;
	int res = -2;
	int not_all_halted_and_no_idle = 0;
	if(isBlockedMT && blocked_threads_vec[i%num_threads]->get_state() == IDLE) return thread_id_;
	if(!isBlockedMT && fine_grained_threads_vec[i%num_threads]->get_state() == IDLE) return thread_id_;
	i++;
	while ( i%num_threads != thread_id_)
	{
		if((isBlockedMT && blocked_threads_vec[i%num_threads]->get_state() == IDLE)
		   || (!isBlockedMT && fine_grained_threads_vec[i%num_threads]->get_state() == IDLE))
		{
			res = i%num_threads;	
			break;
		}
			
		i++;	
	}
	int num_of_halted_threads;
	if(isBlockedMT)
	{
		num_of_halted_threads = 0;
		for(int i =0; i<num_threads; i++)
		{
			if(blocked_threads_vec[i]->get_state() == HALT) num_of_halted_threads++;
		}
	}
	else
	{
		num_of_halted_threads = 0;
		for(int i =0; i<num_threads; i++)
		{
			if(fine_grained_threads_vec[i]->get_state() == HALT) num_of_halted_threads++;
		}
	}
	if(num_of_halted_threads == num_threads) res = -1;
	if(res == -2) res = thread_id_;
	return res;
}

void thread::update_state_regard_cyc(int cycle){
	if (state==BUSY){
		if (cycle >= finish_cycle){
			state = IDLE;
		}
	}	
}

//----------------------------------BLOCKED & FINE GRAINED MT---------------------//
void CORE_BlockedMT() {
	int num_threads = SIM_GetThreadsNum();
	thread *init_thread = new thread();
	blocked_threads_vec = vector<thread*>(num_threads,init_thread);
	int cycles_num = 0;
	for(int i=0; i<num_threads; i++)
	{
		blocked_threads_vec[i] = new thread(i, cycles_num);
	}

	int instructions_num=0;
	int thread_id = 0;
	int switch_flag = 0;
	int switch_penality = SIM_GetSwitchCycles();

	thread_id = get_thread_id_by_RR(num_threads, thread_id, true);
	while(thread_id != -1)
	{	
		for(int i=0; i<num_threads; i++) blocked_threads_vec[i]->update_state_regard_cyc(cycles_num);
		
		while(blocked_threads_vec[thread_id]->get_state() == IDLE && switch_flag == 0)
		{
			blocked_threads_vec[thread_id]->execute_inst(&switch_flag, cycles_num);
			instructions_num++;
			cycles_num++;
		}
		switch_flag == 1 ? cycles_num += switch_penality : cycles_num++;
		switch_flag = 0;
		thread_id = get_thread_id_by_RR(num_threads, thread_id, true);
	}
	blocked_CPI = (double)cycles_num / instructions_num;
}
//////////////////////////////////////////////////////// ///////////////
void CORE_FinegrainedMT() {
	int num_threads = SIM_GetThreadsNum();
	int cycles_num = 0;
	thread *init_thread = new thread();
	fine_grained_threads_vec = vector<thread*>(num_threads,init_thread);
	for(int i=0; i<num_threads; i++)
	{
		fine_grained_threads_vec[i] = new thread(i, cycles_num);
	}

	int instructions_num=0;
	int thread_id = 0;
	int null_switch_flag = 0;

	thread_id = get_thread_id_by_RR(num_threads, thread_id, false);
	while(thread_id != -1)
	{
		for(int i=0; i<num_threads; i++) fine_grained_threads_vec[i]->update_state_regard_cyc(cycles_num);
		while(fine_grained_threads_vec[thread_id]->get_state() == IDLE)
		{
			fine_grained_threads_vec[thread_id]->execute_inst(&null_switch_flag, cycles_num);
			instructions_num++;
			cycles_num++;
		}
		cycles_num++;
		if(fine_grained_threads_vec[thread_id]->get_state() == HALT) cycles_num--;
		thread_id = get_thread_id_by_RR(num_threads, thread_id, false);
	}
	fine_grained_CPI = (double)cycles_num / instructions_num;
}
//////////////////////////////////////////////////////// ///////////////

double CORE_BlockedMT_CPI(){
	return blocked_CPI;
}

double CORE_FinegrainedMT_CPI(){
	return fine_grained_CPI;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for(unsigned i=0; i<REGS_COUNT; i++)
	{
		context->reg[threadid * REGS_COUNT + i] = blocked_threads_vec[threadid]->get_reg_value(i);
	}
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for(unsigned i=0; i<REGS_COUNT; i++)
	{
		context->reg[threadid * REGS_COUNT + i] = fine_grained_threads_vec[threadid]->get_reg_value(i);
	}
}
