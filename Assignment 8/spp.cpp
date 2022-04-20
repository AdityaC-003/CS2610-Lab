#include <bits/stdc++.h>

using namespace std;

#define stall continue
#define FREE true
#define BUSY false

int PC = 0;

class fetch_buffer
{
    public:
    char inst[4];
}f_bf;

class decode_buffer
{
    public:
    bool dependency, jump;
    int opcode;
    int a,b,c;
}d_bf;

class execute_buffer
{
    public:
    int opcode;
    int val, add;
}e_bf;

class memory_buffer
{
    public:
    int opcode;
    int reg, val;
}m_bf;

class Register
{
    public:
    int val;
    bool free;
    Register()
    {
        val = 0;
        free = true;
    }
}reg[16];

int arithmetic_instructions = 0, logical_instructions = 0, data_instructions = 0, control_instructions = 0, halt_instructions = 0;
int no_of_cycles = 0, no_of_stalls = 0, no_of_instructions = 0;
int data_stalls = 0, control_stalls = 0;
string inst_reg;

bool Free[5], Skip[5];

ifstream i_rd, r_rd;
ofstream r_wr;
fstream dp;

int running = 0;

void instruction_fetch()
{
    if(i_rd.eof() || Skip[0] || !Free[0]) 
    {
        if(!d_bf.dependency)
            Skip[1] = true;             //If instruction fetch is skipped & no dependency present, then skip decode stage next cycle
        return;
    }
    running += 1;
    
    i_rd.seekg(PC*3);                   //Reading from ICache
    getline(i_rd, inst_reg, '\n');
    f_bf.inst[0] = inst_reg[0], f_bf.inst[1] = inst_reg[1];
    getline(i_rd, inst_reg, '\n');
    f_bf.inst[2] = inst_reg[0], f_bf.inst[3] = inst_reg[1];
    
    PC = PC + 2;                    //Update PC
    Skip[1] = false;                //Don't skip decode stage next cycle
}

void instruction_decode()
{
    if(Skip[1] || !Free[1])
    {
        Skip[2] = true;             //If instruction decode is skipped, then skip execute stage next cycle
        return;
    }
    running += 1;
    int OPCode=0, dest=0, source1=0, source2=0;     //Split instruction into 4 parts
    OPCode  = (f_bf.inst[0]<='9')?(f_bf.inst[0]-'0'):(f_bf.inst[0]-'a'+10);
    dest    = (f_bf.inst[1]<='9')?(f_bf.inst[1]-'0'):(f_bf.inst[1]-'a'+10);
    source1 = (f_bf.inst[2]<='9')?(f_bf.inst[2]-'0'):(f_bf.inst[2]-'a'+10);
    source2 = (f_bf.inst[3]<='9')?(f_bf.inst[3]-'0'):(f_bf.inst[3]-'a'+10);

    d_bf.opcode = OPCode;
    d_bf.dependency = false, d_bf.jump = false;

    switch(OPCode)
    {
        case 0:             //ADD instruction
        case 1:             //SUB instruction
        case 2:             //MUL instruction
        case 4:             //AND instruction
        case 5:             //OR instruction
        case 7:             //XOR instruction
        {
            if(!reg[source1].free || !reg[source2].free)
            {
                d_bf.dependency = true;
            }
            else
            {
                reg[dest].free = false;
                d_bf.c = dest;
                d_bf.a = reg[source1].val;
                d_bf.b = reg[source2].val;
            }
            break;
        }
        
        case 3:             //INC instruction
        {
            if(!reg[dest].free)
                d_bf.dependency = true;
            else
            {
                d_bf.c = dest;
                reg[dest].free = false;
                d_bf.a = reg[dest].val;
            }
            break;
        }

        case 6:             //NOT instruction
        {
            if(!reg[source1].free)
                d_bf.dependency = true;
            else
            {
                d_bf.c = dest;
                reg[dest].free = false;
                d_bf.a = reg[source1].val;
            }
            break;
        }

        case 8:             //LOAD instruction
        {
            if(!reg[source1].free)
                d_bf.dependency = true;
            else
            {
                d_bf.c = dest;
                reg[dest].free = false;
                d_bf.a = reg[source1].val;
                d_bf.b = source2;
            }
            break;
        }

        case 9:         //STORE instruction
        {
            if(!reg[dest].free || !reg[source1].free)
                d_bf.dependency = true;
            else
            {
                d_bf.c = reg[dest].val;
                d_bf.a = reg[source1].val;
                d_bf.b = source2;
            }
            break;
        }

        case 10:        //JMP instruction
        {
            d_bf.jump = true;
            d_bf.c = ((dest<<4) + source1 - ((dest&8)?(1<<8):0));
            break;
        }

        case 11:        //BEQZ instruction
        {
            if(!reg[dest].free)
                d_bf.dependency = true;
            else
            {
                d_bf.jump = true;
                d_bf.a = reg[dest].val;
                d_bf.c = (source1<<4) + source2 - ((source1&8)?(1<<8):0);
            }
            break;
        }

        case 15:
        {
            Skip[0] = true;
            break;
        }
    }
    
    if(d_bf.dependency)
    {
        Free[0] = false;
        Skip[2] = true;
        data_stalls++;
    }
    else if(d_bf.jump)
    {
        Free[0] = false;
        Skip[1] = true;
        Skip[2] = false;
        control_stalls++;
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
        case 10:        //JMP
        case 11:        //BEQZ
            return PC + 2*d_bf.c;
        default: 
            return 0;
    }
}

void instruction_execute()
{
    if(Skip[2] || !Free[2])
    {
        Skip[3] = true;
        return;
    }
    
    Skip[3] = false;
    running++;
    e_bf.opcode = d_bf.opcode;

    if(d_bf.opcode==15)
        return;
    
    int ALUOutput = ALU(d_bf.opcode, d_bf.a, d_bf.b);

    if(d_bf.opcode < 8)
    {
        e_bf.val = ALUOutput;
        e_bf.add = d_bf.c;
    }
    else if(d_bf.opcode < 10)
    {
        e_bf.add = ALUOutput;
        e_bf.val = d_bf.c;
    }
    else if(d_bf.opcode == 10)
    {
        PC = ALUOutput;
        Free[0] = Free[1] = false;
    }
    else if(d_bf.opcode == 11)
    {
        if(d_bf.a == 0)
        {
            PC = ALUOutput;
        }
        Free[0] = Free[1] = false;
    }
}

void instruction_memory()
{
    if(Skip[3] || !Free[3])
    {
        Skip[4] = true;
        return;
    }

    m_bf.opcode = e_bf.opcode;
    Skip[4] = false;
    running++;
    m_bf.val = 0;
    
    if(e_bf.opcode == 8)
    {
        dp.seekg(e_bf.add*3);
        string in;
        getline(dp, in, '\n');
        
        if(in[0] <= '9')    m_bf.val += (in[0]-'0')<<4;
        else                m_bf.val += (in[0]-'a'+10)<<4;
        
        if(in[1] <= '9')    m_bf.val += in[1]-'0';
        else                m_bf.val += in[1]-'a'+10;
        
        if(m_bf.val & 128)  m_bf.val -= 256;
        
        m_bf.reg = e_bf.val;
    }
    else if(e_bf.opcode == 9)
    {
        dp.seekg(e_bf.add*3);
        char low, high;
        
        if(e_bf.val < 0)    e_bf.val += (1<<8);
        
        if((e_bf.val & 15) <= 9)    low = '0' + (e_bf.val&15);
        else                        low = 'a' + (e_bf.val&15)-10;
        
        e_bf.val = e_bf.val>>4;
        
        if((e_bf.val&15) <= 9)      high = '0' + (e_bf.val&15);
        else                        high = 'a' + (e_bf.val&15)-10;
        
        dp.put(high);
        dp.put(low);
    }
    else
    {
        m_bf.reg = e_bf.add;
        m_bf.val = e_bf.val;
    }
}

void update_status()
{
    cout << m_bf.opcode << " " << m_bf.reg << " " << m_bf.val << endl;
    no_of_instructions++;
    switch(m_bf.opcode)
    {
        case 0:         //ADD
        case 1:         //SUB
        case 2:         //MUL
        case 3:         //INC
        {
            arithmetic_instructions++;
            break;
        }
        case 4:         //AND
        case 5:         //OR
        case 6:         //NOT
        case 7:         //XOR
        {
            logical_instructions++;
            break;
        }

        case 8:         //LOAD
        case 9:         //STORE
        {
            data_instructions++;
            break;
        }

        case 10:        //JMP
        case 11:        //BEQZ
        {
            control_instructions++;
            break;
        }
        case 15:
        {
            halt_instructions++;
            break;
        }
    }
}

void instruction_writeback()
{
    if(Skip[4] || !Free[4]) return;

    running++;
    if(m_bf.opcode <= 8)
    {
        reg[m_bf.reg].val = m_bf.val;
        reg[m_bf.reg].free = true; 
    }
    update_status();
}

void open_files()
{
    i_rd.open("IC1.txt");
    dp.open("DCache.txt");
    r_rd.open("RF.txt");
}

void read_RF()
{
    for(int i = 0; i < 16; i++)
    {
        string in;
        r_rd.seekg(3*i);
        getline(r_rd, in, '\n');
        reg[i].val = 0;
        
        if(in[0] <= '9') reg[i].val += (in[0]-'0')<<4;
        else reg[i].val += (in[0]-'a'+10)<<4;
        
        if(in[1] <= '9') reg[i].val += in[1]-'0';
        else reg[i].val += in[1]-'a'+10;
        
        if(reg[i].val & 128) reg[i].val -= 256;
    }
}

void print_stats()
{
    no_of_stalls = no_of_cycles - no_of_instructions - 4;
    cout << "Total number of instructions executed: " << no_of_instructions << endl;
    cout << "Total number of cycles               : " << no_of_cycles << endl;
    cout << "Number of instructions in each class " << endl;   
    cout << "Arithmetic instructions              : " << arithmetic_instructions << endl;
    cout << "Logical instructions                 : " << logical_instructions << endl;
    cout << "Data instructions                    : " << data_instructions << endl;
    cout << "Control instructions                 : " << control_instructions << endl;
    cout << "Halt instructions                    : " << halt_instructions << endl;
    cout << "Cycles Per Instruction               : " << (double)no_of_cycles/no_of_instructions << endl;
    cout << "Total number of stalls               : " << no_of_stalls << endl;
    cout << "Data stalls (RAW)                    : " << data_stalls << endl;
    cout << "Control stalls                       : " << control_stalls << endl;
}

int main()
{
    open_files();
    read_RF();

    Free[0] = true;
    Skip[1] = false;
    for(int i=1; i<5; i++)
    {
        Free[i] = false;
        Skip[i] = true;
    }

    do
    {
        cout << "Cycle number = " << no_of_cycles+1 << endl;
        running = 0;
        instruction_writeback();
        instruction_memory();
        instruction_execute();
        instruction_decode();
        instruction_fetch();
        
        for(int i = 0; i < 5; i++)
            Free[i] = true;
    
        if(running)
            no_of_cycles++;
        
    }while(running && no_of_cycles < 100);

    print_stats();
    for(int i = 0; i < 16; i++)
    {
        cout<<reg[i].val<<endl;
    }
}