#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <emmintrin.h>

#define stall continue
#define FREE true
#define BUSY false

char inst_reg[16];
char inst[5][16];
bool reg_free[16];
int buffer[5];
bool free[5];
int a[5],b[5],c[5];

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
    
}

bool instruction_decode(char instruction[])
{
    int opcode=0, dest=0, source1=0, source2=0, i=15;
    while(i>11)             //Decoding instruction                 
    {
        opcode = (opcode<<1) + instruction[i];
        dest = (dest<<1) + instruction[i-4];
        source1 = (source1<<1) + instruction[i-8];
        source2 = (source2<<1) + instruction[i-12];
        i--;
    }
    
    switch(opcode)
    {
        case 0:             //ADD instruction
        case 1:             //SUB instruction
        case 2:             //MUL instruction
        case 4:             //AND instruction
        case 5:             //OR instruction
        case 7:             //XOR instruction
        {
            if(!reg_free[source1] || !reg_free[source2])
                return false;
            else
            {
                reg_free[dest] = false;
                c[1] = dest;
                a[1] = reg_file[source1];
                b[1] = reg_file[source2];
                return true;
            }
        }
        
        case 3:             //INC instruction
        {
            if(!reg_free[dest])
                return false;
            else
            {
                c[1] = dest;
                reg_free[dest] = false;
                a[1] = reg_file[dest];
                return true;
            }
        }

        case 6:             //NOT instruction
        {
            if(!reg_free[source1])
                return false;
            else
            {
                c[1] = dest;
                reg_free[dest] = false;
                a[1] = reg_file[source1];
                return true;
            }
        }

        case 8:             //LOAD instruction
        {
            if(!reg_free[source1])
                return false;
            else
            {
                c = dest;
                reg_free[dest] = false;
                a = reg_file[source1];
                b = source2;
                return true;
            }
        }

        case 9:         //STORE instruction
        {
            if(!reg_free[dest] || !reg_free[source1])
                return false;
            else
            {
                c = reg_file[dest];
                a = reg_file[source1];
                b = source2;
                return true;
            }
        }

        case 10:        //JMP instruction
        {

        }
        case 11:        //BEQZ instruction
        {
            
        }
        case 15:        //HALT instruction
        {
            
        }
    }
}

void instruction_execute(char instruction[])
{
    
}

void instruction_memory(char instruction[])
{
    
}

void instruction_writeback(char instruction[])
{
    
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
