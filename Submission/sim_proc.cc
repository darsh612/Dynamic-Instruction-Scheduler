#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_proc.h"

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim 256 32 4 gcc_trace.txt
    argc = 5
    argv[0] = "sim"
    argv[1] = "256"
    argv[2] = "32"
    ... and so on
*/

bool Advance_Cycle(FILE *fp)
{
	if (pipeLine_khaali == true && feof(fp))
		return false;
	else
		return true;
}

void alloc_src(stats * count, int destination, int source1, int source2)					//Allocate dest and srcs
{
    count->dst = destination;
	count->src1 = count->src1_original  = source1;
	count->src2 = count->src2_original = source2;
}

// Do nothing if 
// (1) there are no more instructions in the trace file or
// (2) DE is not empty (cannot accept a new decode bundle).
//
// If there are more instructions in the trace file and if DE is empty
// (can accept a new decode bundle), then fetch up to WIDTH
// instructions from the trace file into DE. Fewer than WIDTH
// instructions will be fetched only if the trace file has fewer than
// WIDTH instructions left.

void Fetch(FILE *fp)
{
	stats count;
	int uch = 0;

	if (feof(fp) || DE.khaali == false)
		return;
	
	if (!feof(fp) && DE.khaali)
	{
        int i=0;
		while(i<Width)
		{
			fscanf(fp,"%llx %d %d %d %d\n",&op,&type,&destination,&source1,&source2);
			count.pc = instr_count;
			count.type = type;
			alloc_src(&count, destination,source1,source2);
			count.src1_rdy = count.src2_rdy = count.src1_rob = count.src2_rob = false;
			count.FE_shur = cycle_count;
			count. FE_age = 1;
			count.DE_shur = cycle_count + 1;
			
			if (count.type == 0)
				count.counter = 1;
			else if (count.type == 1)
				count.counter = 2;
			else if (count.type == 2)
				count.counter = 5;

			DE.stg.push_back(count);
			instr_count++;
			uch++;
			
			if (feof(fp))
				break;
			i++;
		}
		if (uch != 0)
			DE.khaali = false;
	}
}

// If DE contains a decode bundle:
// If RN is not empty (cannot accept a new rename bundle), then do
// nothing. If RN is empty (can accept a new rename bundle), then
// advance the decode bundle from DE to RN.

void Decode()
{
	if (!DE.khaali)
	{
		if (RN.khaali)
		{
			unsigned int i = 0;
			while( i < DE.stg.size())
			{
				DE.stg[i].RN_shur = cycle_count + 1;
				DE.stg[i]. DE_age = DE.stg[i].RN_shur - DE.stg[i].DE_shur;
				i++;
			}
			DE.stg.swap(RN.stg);
			DE.stg.clear();
			DE.khaali = true;
			RN.khaali = false;
		}
			
	}
}

 // If RN contains a rename bundle:
 // If either RR is not empty (cannot accept a new register-read bundle)
 // or the ROB does not have enough free entries to accept the entire
 // rename bundle, then do nothing.
 // If RR is empty (can accept a new register-read bundle) and the ROB
 // has enough free entries to accept the entire rename bundle, then
 // process (see below) the rename bundle and advance it from RN to RR. //
 // How to process the rename bundle:
 // Apply your learning from the class lectures/notes on the steps for // renaming:
 // (1) Allocate an entry in the ROB for the instruction,
 // (2) Rename its source registers, and
 // (3) Rename its destination register (if it has one).
 // Note that the rename bundle must be renamed in program order.
 // Fortunately, the instructions in the rename bundle are in program order).

void Rename()
{
	if (!RN.khaali)
	{
		if (RR.khaali)
		{
			int robjaaga;

			if (rob.head < rob.tail)
				robjaaga = rob.rob_Size - (rob.tail - rob.head);
			else if (rob.tail < rob.head)
				robjaaga = rob.head - rob.tail;
			else
			{
				if (rob.tail >= (rob.rob_Size - 1))
				{

					if (rob.rob[rob.tail - 1].dest == 0 && rob.rob[rob.tail - 1].pc == 0 && rob.rob[rob.tail - 1].rdy == 0)
						robjaaga = rob.rob_Size;
					else
						robjaaga = 0;
				}
				else
				{
					if (rob.rob[rob.tail + 1].dest == 0 && rob.rob[rob.tail + 1].pc == 0 && rob.rob[rob.tail + 1].rdy == 0)
						robjaaga = rob.rob_Size;
					else
						robjaaga = 0;
				}
			}

			if ((unsigned)robjaaga < RN.stg.size())
				return;
			else
			{
                unsigned int i = 0;
				while( i < RN.stg.size())
				{
					if (RN.stg[i].src1 != -1)
						if (RMT.rmt[RN.stg[i].src1].valid == true)							
						{
							RN.stg[i].src1 = RMT.rmt[RN.stg[i].src1].tag;						
							RN.stg[i].src1_rob = true;										
						}


					if (RN.stg[i].src2 != -1)
						if (RMT.rmt[RN.stg[i].src2].valid == true)							
						{
							RN.stg[i].src2 = RMT.rmt[RN.stg[i].src2].tag;						
							RN.stg[i].src2_rob = true;										
						}

					

					rob.rob[rob.tail].dest = RN.stg[i].dst;									
					rob.rob[rob.tail].pc = RN.stg[i].pc;									
					rob.rob[rob.tail].rdy = false;											

					if (RN.stg[i].dst != -1)												
					{
						RMT.rmt[RN.stg[i].dst].tag = rob.tail;								
						RMT.rmt[RN.stg[i].dst].valid = true;								
					}

					RN.stg[i].dst = rob.tail;												

					if (rob.tail != (rob.rob_Size - 1))
						rob.tail++;
					else
						rob.tail = 0;

					RN.stg[i].RR_shur = cycle_count + 1;
					RN.stg[i].RN_age = RN.stg[i].RR_shur - RN.stg[i].RN_shur;
					i++;
				}
				RN.stg.swap(RR.stg);
				RN.stg.clear();
				RN.khaali = true;
				RR.khaali = false;
			}
		}

	}
}

// If RR contains a register-read bundle:
// If DI is not empty (cannot accept a new dispatch bundle), then do
// nothing. If DI is empty (can accept a new dispatch bundle), then
// process (see below) the register-read bundle and advance it from RR
// to DI.
// How to process the register-read bundle:
// Since values are not explicitly modeled, the sole purpose of the
// Register Read stage is to ascertain the readiness of the renamed
// source operands. Apply your learning from the class lectures/notes
// on this topic.
// Also take care that producers in their last cycle of execution
// wakeup dependent operands not just in the IQ, but also in two other
// stages including RegRead()(this is required to avoid deadlock). See
// Execute() description above.

void RegRead()
{
	if (!RR.khaali)
	{
		if (DI.khaali)
		{
			unsigned int i = 0;
			while( i < RR.stg.size())
			{
				if (RR.stg[i].src1_rob)
				{
					if (rob.rob[RR.stg[i].src1].rdy == 1)
						RR.stg[i].src1_rdy = true;
				}
				else
					RR.stg[i].src1_rdy = true;

				if (RR.stg[i].src2_rob)
				{
					if (rob.rob[RR.stg[i].src2].rdy == 1)
						RR.stg[i].src2_rdy = true;
				}
				else
					RR.stg[i].src2_rdy = true;

				RR.stg[i].DI_shur = cycle_count + 1;
				RR.stg[i].RR_age = RR.stg[i].DI_shur - RR.stg[i].RR_shur;
				i++;
			}
			RR.stg.swap(DI.stg);
			RR.stg.clear();
			RR.khaali = true;
			DI.khaali = false;
		}
	}
}

// If DI contains a dispatch bundle:
// If the number of free IQ entries is less than the size of the
// dispatch bundle in DI, then do nothing. If the number of free IQ
// entries is greater than or equal to the size of the dispatch bundle
// in DI, then dispatch all instructions from DI to the IQ

void Dispatch()
{
	if (!DI.khaali)
	{
		if ((IQ.iq_size - IQ.in.size()) >= DI.stg.size())
		{
			unsigned int i = 0;
			while( i < DI.stg.size())
			{
				DI.stg[i].IS_shur = cycle_count + 1;
				DI.stg[i].DI_age = DI.stg[i].IS_shur - DI.stg[i].DI_shur;
				IQ.in.push_back(DI.stg[i]);
				i++;
			}
			DI.stg.clear();
			DI.khaali = true;
		}
	}
}

// Issue up to WIDTH oldest instructions from the IQ. (One approach to
// implement oldest-first issuing is to make multiple passes through
// the IQ, each time finding the next oldest ready instruction and then
// issuing it. One way to annotate the age of an instruction is to
// assign an incrementing sequence number to each instruction as it is
// fetched from the trace file.) // To issue an instruction:
// 1)  Remove the instruction from the IQ.
// 2)  Add the instruction to the execute_list. Set a timer for
//     the instruction in the execute_list that will allow you to
//     model its execution latency.

void Issue()
{
	if (IQ.in.size() != 0)
	{
		sort(IQ.in.begin(), IQ.in.end());

		int i = 0;
		int uch = 1;
		while (uch != 0)
		{
			uch = 0;
			unsigned int j = 0; 
			while(j < IQ.in.size())
			{
				if (IQ.in[j].src1_rdy && IQ.in[j].src2_rdy)
				{
					IQ.in[j].EX_shur = cycle_count + 1;
					IQ.in[j].IS_age = IQ.in[j].EX_shur - IQ.in[j].IS_shur;
					EX.exe_vec.push_back(IQ.in[j]);
					IQ.in.erase(IQ.in.begin() + j);
					i++;
					uch++;
					break;
				}
				j++;
			}
			if (i == Width)
				break;
		}
		
	}
	
}

// From the execute_list, check for instructions that are finishing
// execution this cycle, and:
// 1) Remove the instruction from the execute_list.
// 2) Add the instruction to WB.
// 3) Wakeup dependent instructions (set their source operand ready
// flags) in the IQ, DI (dispatch bundle), and RR (the register-read
// bundle).

void Execute()
{
	if (EX.exe_vec.size() != 0)
	{
		int uch = 1;
		EX.dec_count();

		
		
		while (uch != 0)
		{
			uch = 0;
            unsigned int i = 0;
			while( i < EX.exe_vec.size())
			{
				if (EX.exe_vec[i].counter == 0)
				{
					EX.exe_vec[i].WB_shur = cycle_count + 1;
					EX.exe_vec[i].EX_age = EX.exe_vec[i].WB_shur - EX.exe_vec[i].EX_shur;

					WB.stg.push_back(EX.exe_vec[i]);
                    unsigned int x = 0;
					while(x < IQ.in.size())									
					{
						if (IQ.in[x].src1 == EX.exe_vec[i].dst)
							IQ.in[x].src1_rdy = true;

						if (IQ.in[x].src2 == EX.exe_vec[i].dst)
							IQ.in[x].src2_rdy = true;
						x++;
					}
                    unsigned int y = 0;
					while(y < DI.stg.size())									
					{
						if (DI.stg[y].src1 == EX.exe_vec[i].dst)
							DI.stg[y].src1_rdy = true;

						if (DI.stg[y].src2 == EX.exe_vec[i].dst)
							DI.stg[y].src2_rdy = true;
					    y++;
					}
                    unsigned int z = 0;
					while(z < RR.stg.size())									
					{
						if (RR.stg[z].src1 == EX.exe_vec[i].dst)
							RR.stg[z].src1_rdy = true;

						if (RR.stg[z].src2 == EX.exe_vec[i].dst)
							RR.stg[z].src2_rdy = true;
						z++;
					}

					EX.exe_vec.erase(EX.exe_vec.begin() + i);
					uch++;
					break;
				}
				i++;
			}
		}
	}
}

//writeback stage
//Process the writeback bundle in WB: For each instruction in WB, mark
// the instruction as “ready” in its entry in the ROB.

void Writeback()
{
	if (WB.stg.size() != 0)
	{
		unsigned int i = 0;
		while(i < WB.stg.size())
		{
			rob.rob[WB.stg[i].dst].rdy = true;
			WB.stg[i].RT_shur = cycle_count + 1;
			WB.stg[i].WB_age = WB.stg[i].RT_shur - WB.stg[i].WB_shur; 
			RT.stg.push_back(WB.stg[i]);
			i++;
		}
		WB.stg.clear();
	}
}

//retire stage
// Retire up to WIDTH consecutive “ready” instructions from the head of
// the ROB.

void Retire()
{
	int i = 0;

	while(i< Width)
	{
		if (rob.head == rob.tail && rob.head != rob.rob_Size - 1)
		{
			if (rob.rob[rob.head + 1].pc == 0)
				return;
		}

		if (rob.rob[rob.head].rdy)
		{
			unsigned int j = 0;
			while( j < RR.stg.size())
			{
				if (RR.stg[j].src1 == rob.head)
					RR.stg[j].src1_rdy = true;

				if (RR.stg[j].src2 == rob.head)
					RR.stg[j].src2_rdy = true;
				j++;
			}
            unsigned int x = 0;
			while( x < RT.stg.size())
			{
				if (RT.stg[x].pc == rob.rob[rob.head].pc)
				{
					RT.stg[x].RT_age = (cycle_count + 1) - RT.stg[x].RT_shur;
                    printf("%d fu{%d} src{%d,%d} dst{%d} FE{%d,%d} DE{%d,%d} RN{%d,%d} RR{%d,%d} DI{%d,%d} IS{%d,%d} EX{%d,%d} WB{%d,%d} RT{%d,%d}\n",RT.stg[x].pc,RT.stg[x].type,RT.stg[x].src1_original,RT.stg[x].src2_original,rob.rob[rob.head].dest, RT.stg[x].FE_shur,RT.stg[x].FE_age,RT.stg[x].DE_shur,RT.stg[x].DE_age,RT.stg[x].RN_shur,RT.stg[x].RN_age,RT.stg[x].RR_shur,RT.stg[x].RR_age,RT.stg[x].DI_shur,RT.stg[x].DI_age,RT.stg[x].IS_shur,RT.stg[x].IS_age,RT.stg[x].EX_shur,RT.stg[x].EX_age,RT.stg[x].WB_shur,RT.stg[x].WB_age,RT.stg[x].RT_shur,RT.stg[x].RT_age);
					RT.stg.erase(RT.stg.begin() + x);
					break;
				}
                x++;
			}

            int z = 0;
			while( z < RMT.RMT_Size)
			{
				if (RMT.rmt[z].tag == rob.head)
				{
					RMT.rmt[z].tag = 0;
					RMT.rmt[z].valid = false;
				}
				z++;
			}
			rob.rob[rob.head].clear();

			if (rob.head != (rob.rob_Size - 1))
				rob.head++;
			else
				rob.head = 0;
		}
		else
			break;
		i++;
	}
}


int main(int argc, char* argv[])
{

    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    proc_params params;       // look at sim_bp.h header file for the the definition of struct proc_params
    int op_type, dest, src1, src2;  // Variables are read from trace file
    unsigned long int pc; // Variable holds the pc read from input file


    params.rob_size     = strtoul(argv[1], NULL, 10);
    params.iq_size      = strtoul(argv[2], NULL, 10);
    params.width        = strtoul(argv[3], NULL, 10);
    trace_file          = argv[4];


	int rob_Size = params.rob_size;
	int iq_size = params.iq_size;
	Width = params.width;
	trace_file = argv[4];

	cycle_count = 0;
	instr_count = 0;
	pipeLine_khaali = false;

	rob.rob_Size = rob_Size;
	IQ.iq_size = iq_size;

	DE.khaali = RN.khaali = RR.khaali = DI.khaali = WB.khaali = RT.khaali = true;
	RMT.RMT_Size = 67;

	rob.head = rob.tail = 0;

	for (int i = 0; i < 67; i++)
	{
		RMT.rmt[i].valid = false;
		RMT.rmt[i].tag = -1;
	}

	rob_val rb;												
	rb.clear();
	for (int i = 0; i < rob_Size; i++)
		rob.rob.push_back(rb);





	FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }	

    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // The following loop just tests reading the trace and echoing it back to the screen.
    //
    // Replace this loop with the "do { } while (Advance_Cycle());" loop indicated in the Project 3 spec.
    // Note: fscanf() calls -- to obtain a fetch bundle worth of instructions from the trace -- should be
    // inside the Fetch() function.
    //
    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    /*while(fscanf(FP, "%lx %d %d %d %d", &pc, &op_type, &dest, &src1, &src2) != EOF)
        printf("%lx %d %d %d %d\n", pc, op_type, dest, src1, src2); //Print to check if inputs have been read correctly*/
    


	do
	{
		Retire();
		Writeback();
		Execute();
		Issue();
		Dispatch();
		RegRead();
		Rename();
		Decode();
		Fetch(FP);
		
		if (DE.khaali && RN.khaali && RR.khaali && DI.khaali && IQ.in.size() == 0 && EX.exe_vec.size() == 0 && WB.stg.size() == 0)
			if (rob.head == rob.tail)
				if (rob.rob[rob.tail].pc == 0)
					pipeLine_khaali = true;

		cycle_count++;
		
	}while (Advance_Cycle(FP));

	fclose(FP);

	printf( "# === Simulator Command =========\n");
	printf ("#");
	for (int i = 0; i < argc; i++)
		cout << argv[i] << " ";
	cout << endl << "# === Processor Configuration ===" << endl;
	printf("# ROB_SIZE = %d\n",rob_Size);
	printf("# IQ_SIZE  = %d\n",iq_size);
	printf("# WIDTH    = %d\n",Width);
	printf("# === Simulation Results ========\n");
	printf("# Dynamic Instruction Count    = %d\n",instr_count);
	printf("# Cycles                       = %d\n",cycle_count);
	printf("# Instructions Per Cycle (IPC)  = %0.2f\n",((float)instr_count / (float)cycle_count));
}
