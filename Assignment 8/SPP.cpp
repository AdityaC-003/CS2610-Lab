#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <emmintrin.h>

#define stall continue
#define FREE true
#define BUSY false

int PC = 0;
char inst_reg[16];
char inst[5][16];
bool reg_free[16];
int decode_a, decode_b, decode_c, decode_opcode;        //Buffer of decoded instruction
int mem_val, mem_add;                                   //Buffer after ALU operation
int dest_reg, dest_val;                                 //Buffer after memory operation
bool free[5];

void execute(int pipeline_stage, char instruction[])
{
    switch(pipeline_stage)
    {
        case 0:
            instruction_fetch(instruction);
            break;
        case 1:
            instruction_decode(instruction);
            break;
        case 2:
            instruction_execute(instruction);
            break;
        case 3:
            instruction_memory(instruction);
            break;
        case 4:
            instruction_writeback(instruction);
            break;
    }
}

void instruction_fetch(char instruction[])
{
    //read from file
    PC = PC + 2;
}

pair<bool,bool> instruction_decode(char instruction[])
{
    int OPCode=0, dest=0, source1=0, source2=0, i=15;
    bool dependency = false, jump = false;
    while(i>11)             //Decoding instruction                 
    {
        OPCode = (OPCode<<1) + instruction[i];
        dest = (dest<<1) + instruction[i-4];
        source1 = (source1<<1) + instruction[i-8];
        source2 = (source2<<1) + instruction[i-12];
        i--;
    }
    decode_opcode = OPCode;

    switch(OPCode)
    {
        case 0:             //ADD instruction
        case 1:             //SUB instruction
        case 2:             //MUL instruction
        case 4:             //AND instruction
        case 5:             //OR instruction
        case 7:             //XOR instruction
        {
            if(!reg_free[source1] || !reg_free[source2])
                dependency = true;
            else
            {
                reg_free[dest] = false;
                decode_c = dest;
                decode_a = reg_file[source1];
                decode_b = reg_file[source2];
            }
            break;
        }
        
        case 3:             //INC instruction
        {
            if(!reg_free[dest])
                dependency = true;
            else
            {
                decode_c = dest;
                reg_free[dest] = false;
                decode_a = reg_file[dest];
            }
            break;
        }

        case 6:             //NOT instruction
        {
            if(!reg_free[source1])
                dependency = true;
            else
            {
                decode_c = dest;
                reg_free[dest] = false;
                decode_a = reg_file[source1];
            }
        }

        case 8:             //LOAD instruction
        {
            if(!reg_free[source1])
                dependency = true;
            else
            {
                c = dest;
                reg_free[dest] = false;
                a = reg_file[source1];
                b = source2;
            }
        }

        case 9:         //STORE instruction
        {
            if(!reg_free[dest] || !reg_free[source1])
                dependency = true;
            else
            {
                c = reg_file[dest];
                a = reg_file[source1];
                b = source2;
            }
        }

        case 10:        //JMP instruction
        {
            jump = true;
            PC = PC + (dest<<4) + source1 - ((dest&8)?(1<<8):0);
        }
        case 11:        //BEQZ instruction
        {
            if(!reg_free[dest])
                dependency = true;
            else
            {
                jump = true;
                PC = PC + (source1<<4) + source2 - ((source1&8)?(1<<8):0);
            }
        }
        case 15:        //HALT instruction
        {
            dependency = true;
        }
    }
    return {dependency,jump};
}

void instruction_execute()
{
    if(decode_opcode==10 || decode_opcode==15)
        return;
    
    int ALUOutput = ALU(decode_opcode, decode_a, decode_b);

    if(decode_opcode < 8)
    {
        mem_val = ALUOutput;
        mem_add = decode_c;
    }
    else if(decode_opcode < 10)
    {
        mem_add = ALUOutput;
        mem_val = decode_c;
    }
    else if(decode_opcode == 11)
    {
        if(a[1]==0)
            PC = PC + ALUOutput;
        //Not complete!
    }
}

int ALU(int opcode, int a, int b=1)
{
    switch(opcode)
    {
        case 0:         //ADD
            return a+b;
        case 1:         //SUB
            return a-b;
        case 2:         //MUL
            return a*b;
        case 3:         //INC
            return a+1;
        case 4:         //AND   
            return a&b;
        case 5:         //OR
            return a|b;
        case 6:         //NOT
            return ~a;
        case 7:         //XOR
            return a^b;
        case 8:         //LOAD
            return a+b;
        case 9:         //STORE
            return a+b;
        case 11:        //JMP
            return PC + (L2<<1);
    }
}

void instruction_memory(char instruction[])
{
    if(execute_opcode == 8)
    {
        //LMD <--- D$[ALUOutput]
    }
    else if(execute_opcode == 9)
    {
        //D$[ALUOutput] <--- B
    }
    else
    {
        dest_reg = mem_add;
        dest_val = mem_val;
    }
}

void instruction_writeback()
{
    if(memory_opcode<=8)
    {
        reg_file[dest_reg] = dest_val;
        break;
    }
}

int main()
{
    FILE* i_rd = fopen("ICache.txt", "r");
    FILE* d_rd = fopen("DCache.txt", "r");
    FILE* r_rd1 = fopen("RF.txt", "r");
    FILE* r_rd2 = fopen("RF.txt", "r");

    FILE* i_wr = fopen("ICache.txt", "w");
    FILE* d_wr = fopen("DCache.txt", "w");
    FILE* r_wr = fopen("RF.txt", "w");

    int busy_sum = 0;
    do
    {
        for (int i = 4; i >= 1; i--)
        {
            if(free[i] && free[i-1])
            {
                copy(inst[i-1], inst[i]);
                free[i] = false;
                busy_sum++;
            }

            if(!free[i])
            {
                if(check_dependency(inst[i], reg_free))
                {
                    stall;
                }
                else
                    execute(i,inst[i]);
            }
        }
        
        if(!i_rd.eof() && free[0])
        {
            fscanf(i_rd, "%s", &inst_reg);
            copy(inst_reg, inst[0]);
            free[0] = false;
        }

    }while(/* Don't forget to add the condition */);

    fprintf(fout, "\nNumber of cache misses = %d\n", m);     		// m gives the number of cache misses
    fprintf(fout, "\nAverage hits per miss = %f\n", sum / m);  		// sum stores the sum of values in miss array, hence average = sum/m
    fprintf(fout, "\nMode is %d, with frequency %d", mode, freq);	// prints the mode of the data with its frequency
}
