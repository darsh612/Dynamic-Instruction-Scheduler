#ifndef SIM_PROC_H
#define SIM_PROC_H

#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <iomanip>
#include <string.h>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

typedef struct proc_params{
    unsigned long int rob_size;
    unsigned long int iq_size;
    unsigned long int width;
}proc_params;

//Adding new stages 

int cycle_count, instr_count, Width;
bool pipeLine_khaali;

 int type;
 int source1, source2, destination;
 unsigned long long op;

// Put additional data structures here as per your requirement

typedef struct rmapt_att
{
	bool valid;
	int tag;
}rmapt_val;

typedef struct rob_att
{
	int dest,pc;
	bool rdy;
	void clear()
	{
		dest = pc = 0;
		rdy = false;
	}
}rob_val;

typedef struct statistic
{
	int pc, type, dst, src1, src2, counter;
	bool src1_rdy, src2_rdy, src1_rob, src2_rob;
	int FE_shur,  FE_age, DE_shur,  DE_age, RN_shur, RN_age, RR_shur, RR_age, DI_shur, DI_age;
	int IS_shur, IS_age, EX_shur, EX_age, WB_shur, WB_age, RT_shur, RT_age;
	int src1_original, src2_original;

	bool operator < (const statistic &temp) const
	{
		return (pc < temp.pc);
	}
}stats;

class stage
{
public:
	bool khaali;
	vector <stats> stg;
};

class instr_q
{
public:
	int iq_size;
	vector <stats> in;
};

class reorder_buf
{
public:
	int head, tail;
	int rob_Size;

	vector <rob_val> rob;
};

class rename_map
{
public:
	int RMT_Size;
	rmapt_val rmt[67];
};

class exe
{
public:
	vector <stats> exe_vec;
	void dec_count()
	{
		for (unsigned int i = 0; i < exe_vec.size(); i++)
				exe_vec[i].counter--;
	}
	
};

stage DE, RN, RR, DI, WB, RT;
exe EX;
reorder_buf rob;
instr_q IQ;
rename_map RMT;

bool Advance_Cycle(FILE *fp);
void alloc_src(stats * count, int dest,int src1, int src2);
void Fetch(FILE *fp);
void Decode(void);
void Rename(void);
void RegRead(void);
void Dispatch(void);
void Issue(void);
void Execute(void);
void Writeback(void);
void Retire(void);


#endif
