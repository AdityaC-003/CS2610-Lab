#include <bits/stdc++.h>

using namespace std;

#define stall continue
#define FREE true
#define BUSY false

int PC = 0, no_of_cycles = 0, no_of_stalls = 0, no_of_instructions = 0;
string inst_reg;
char inst[4];
bool reg_free[16];
int reg_file[16];
int decode_a, decode_b, decode_c, decode_opcode;        //Buffer of decoded instruction
int mem_val, mem_add;                                   //Buffer after ALU operation
int dest_reg, dest_val;                                 //Buffer after memory operation
bool Free[5];
int LMD;
ifstream i_rd, r_rd;
ofstream r_wr;
fstream dp;
bool dependency, jump;
int running = 0, execute_opcode, memory_opcode;
bool Skip[5];

void instruction_fetch()
{
    if(i_rd.eof() || Skip[0]) 
    {
        Skip[1] = true;
        return;
    }
    running+=1;
    i_rd.seekg(PC*3);
    getline(i_rd, inst_reg, '\n');
    inst[0] = inst_reg[0], inst[1] = inst_reg[1];
    getline(i_rd, inst_reg, '\n');
    inst[2] = inst_reg[0], inst[3] = inst_reg[1];
    PC = PC + 2;
    Skip[1] = false;
}

void instruction_decode()
{
    if(Skip[1])
    {
        Skip[2] = true;
        return;
    }
    running+=10;
    int OPCode=0, dest=0, source1=0, source2=0;
    dependency = false, jump = false;
    OPCode  = (inst[0]<='9')?(inst[0]-'0'):(inst[0]-'a'+10);
    dest    = (inst[1]<='9')?(inst[1]-'0'):(inst[1]-'a'+10);
    source1 = (inst[2]<='9')?(inst[2]-'0'):(inst[2]-'a'+10);
    source2 = (inst[3]<='9')?(inst[3]-'0'):(inst[3]-'a'+10);

    cout << "Instruction Decoded: " << OPCode << " " << dest << " " << source1 << " " << source2 << endl;
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
            {
                cout << "Register "<<source1 << " " << reg_free[source1] <<"not free" << endl;
                dependency = true;
            }
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
            break;
        }

        case 8:             //LOAD instruction
        {
            if(!reg_free[source1])
                dependency = true;
            else
            {
                decode_c = dest;
                reg_free[dest] = false;
                decode_a = reg_file[source1];
                decode_b = source2;
            }
            break;
        }

        case 9:         //STORE instruction
        {
            if(!reg_free[dest] || !reg_free[source1])
                dependency = true;
            else
            {
                decode_c = reg_file[dest];
                decode_a = reg_file[source1];
                decode_b = source2;
            }
            break;
        }

        case 10:        //JMP instruction
        {
            jump = true;
            PC = PC + (dest<<4) + source1 - ((dest&8)?(1<<8):0);
            break;
        }
        case 11:        //BEQZ instruction
        {
            if(!reg_free[dest])
                dependency = true;
            else
            {
                jump = true;
                decode_a = reg_file[dest];
                decode_c = (source1<<4) + source2 - ((source1&8)?(1<<8):0);
            }
            break;
        }
        case 15:
        {
            Skip[0] = true;
        }
    }
    if(dependency)
    {
        Free[0] = false;
        Skip[2] = true;
    }
    else if(jump)
    {
        Free[0] = false;
        Skip[2] = true;
    }
    else Skip[2] = false;
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
            return PC + 2*decode_c;
    }
}

void instruction_execute()
{
    if(Skip[2])
    {
        Skip[3] = true;
        return;
    }
    Skip[3] = false;
    running++;
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
        if(decode_a == 0)
        {
            PC = PC + ALUOutput;
            Free[0] = Free[1] = false;
        }
    }
    execute_opcode = decode_opcode;
}

void instruction_memory()
{
    if(Skip[3])
    {
        Skip[4] = true;
        return;
    }
    memory_opcode = execute_opcode;
    Skip[4] = false;
    running++;
    dest_val = 0;
    if(execute_opcode == 8)
    {
        //LMD <--- D$[ALUOutput]
        // use dp
        dp.seekg(mem_add*3);
        string in;
        getline(dp, in, '\n');
        if(in[0] <= '9') dest_val += (in[0]-'0')<<4;
        else dest_val += (in[0]-'a'+10)<<4;
        if(in[1] <= '9') dest_val += in[1]-'0';
        else dest_val += in[1]-'a'+10;
        if(dest_val & 128) dest_val -= 256;
        dest_reg = mem_val;
        cout << dest_reg << " <- " << dest_val << endl;
    }
    else if(execute_opcode == 9)
    {
        //D$[ALUOutput] <--- B
        dp.seekg(mem_add*3);
        char low, high;
        if(mem_val < 0) mem_val += (1<<8);
        cout << "Storing " << mem_val << endl;
        if((mem_val & 15) <= 9) low = '0' + (mem_val&15);
        else low = 'a' + (mem_val&15)-10;
        //cout << int(low) << endl;
        mem_val = mem_val>>4;
        if((mem_val&15) <= 9) high = '0' + (mem_val&15);
        else high = 'a' + (mem_val&15)-10;
        dp.put(high);
        dp.put(low);
    }
    else
    {
        dest_reg = mem_add;
        dest_val = mem_val;
    }
}

void instruction_writeback()
{
    if(Skip[4]) return;

    running++;
    if(memory_opcode <= 8)
    {
        reg_file[dest_reg] = dest_val;
        reg_free[dest_reg] = true; 
    }
}

int main()
{
    
    i_rd.open("ICache.txt");
    dp.open("DCache.txt");
    r_rd.open("RF.txt");


    int busy_sum = 0;
    
    for(int i=0; i<16; i++)
    {
        reg_free[i] = true;
    }

    Free[0] = true;
    Skip[1] = false;
    for(int i=1; i<5; i++)
    {
        Free[i] = false;
        Skip[i] = true;
    }
    int val;

    for(int i = 0; i < 16; i++)
    {
        string in;
        r_rd.seekg(3*i);
        getline(r_rd, in, '\n');
        reg_file[i] = 0;
        ///cout<<"in "<<(int)in[0]<<" "<<in[1]<<endl;
        if(in[0] <= '9') reg_file[i] += (in[0]-'0')<<4;
        else reg_file[i] += (in[0]-'a'+10)<<4;
        if(in[1] <= '9') reg_file[i] += in[1]-'0';
        else reg_file[i] += in[1]-'a'+10;
        // reg_file[i] = 10*(in[0]<='9')?(in[0]-'0'):(in[0]-'a'+10) + (in[1]<='9')?(in[1]-'0'):(in[1]-'a'+10);
        if(reg_file[i] & 128) reg_file[i] -= 256;
        //cout<<reg_file[i]<<endl;
    }

    int no_of_cycles = 0;
    do
    {
        running = 0;
        if(Free[4])     instruction_writeback();
        if(Free[3])     instruction_memory();
        if(Free[2])     instruction_execute();
        if(Free[1])     instruction_decode();
        if(Free[0])     instruction_fetch();
        
        for(int i = 0; i < 5; i++)
        {
            Free[i] = true;
        }
        if(running)
            no_of_cycles++;
        cout << "Cycle number" << no_of_cycles << endl;
        cout << "Running = " << running << endl;
    }while(running);

    cout << "Number of cycles = "<< no_of_cycles << endl;
    for(int i = 0; i < 16; i++)
    {
        cout<<reg_file[i]<<endl;
        /*r_wr.seekp(i*4);
        char low, high;
        if(reg_file[i] < 0) reg_file[i] += 16;
        if(reg_file[i] & 15 <= 9) low = '0' + (reg_file[i]&15);
        else low = 'a' + (reg_file[i]&15)-10;
        reg_file[i] = (reg_file[i]^15)/16;
        if(reg_file[i] & 15 <= 9) high = '0' + (reg_file[i]&15);
        else high = 'a' + (reg_file[i]&15)-10;
        r_wr.put(high);
        r_wr.put(low);*/
    }
}
