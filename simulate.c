#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define ADD 0
#define SUB 1
#define MUL 2
#define LDI 3
#define BEQZ 4
#define AND 5
#define OR 6
#define JR 7
#define SLC 8
#define SRC 9
#define LB 10
#define SB 11

struct register_file
{
    char reg[64]; // 8 bits per register (0-63)
    char sreg;    // 000CVNSZ
    short pc;     // 16 bits -> 65536 addresses
};

struct decoded_instruction
{
    short instruction;       // 16 bits
    char opcode;             // 4 bits
    char r1_address;         // 6 bits
    char r2_address;         // 6 bits
    char imm;                // 6 bits
    char r1_value;           // 8 bits
    char r2_value;           // 8 bits
    short pc_val_for_branch; // 16 bits
};

struct previous_clock_cycle
{
    short instruction_fetched;
    struct decoded_instruction instruction_decoded;
};

short instruction_memory[1024]; // 16 bits per word
char data_memory[2048];         // 8 bits per word
struct register_file register_file;
int clock_cycles = 1;
struct previous_clock_cycle previous_clock_cycle;
bool fetched_last_inst = false;
bool branch = false;
int ctr_branch=0;
bool finish = false;
int instructions_cnt=0;
int cnt_to_end=0;

void print_flags(char sreg)
{
    bool C, V, N, S, Z;
    C = sreg & 0b00010000;
    V = sreg & 0b00001000;
    N = sreg & 0b00000100;
    S = sreg & 0b00000010;
    Z = sreg & 0b00000001;
    printf("C=%d, V=%d, N=%d, S=%d, Z=%d\n", C, V, N, S, Z);
}

void print_instruction(short instruction)
{
    char opcode = (instruction >> 12) & 0xF;
    char r1 = (instruction >> 6) & 0x3F;
    char r2 = (instruction) & 0x3F;
    char imm = (instruction) & 0x3F;
    if ((imm & 0b00100000) != 0)
    {
        imm = imm | 0b11000000;
    }
    switch (opcode)
    {
    case ADD:
        printf("ADD R%d R%d\n", r1, r2);
        break;
    case SUB:
        printf("SUB R%d R%d\n", r1, r2);
        break;
    case MUL:
        printf("MUL R%d R%d\n", r1, r2);
        break;
    case LDI:
        printf("LDI R%d %d\n", r1, imm);
        break;
    case BEQZ:
        printf("BEQZ R%d %d\n", r1, imm);
        break;
    case AND:
        printf("AND R%d R%d\n", r1, r2);
        break;
    case OR:
        printf("OR R%d R%d\n", r1, r2);
        break;
    case JR:
        printf("JR R%d R%d\n", r1, r2);
        break;
    case SLC:
        printf("SLC R%d %d\n", r1, imm);
        break;
    case SRC:
        printf("SRC R%d %d\n", r1, imm);
        break;
    case LB:
        printf("LB R%d %d\n", r1, imm);
        break;
    case SB:
        printf("SB R%d %d\n", r1, imm);
        break;
    }
}

char str_opcode_to_char(char *op)
{
    if (strcmp(op, "ADD") == 0)
    {
        return ADD;
    }
    else if (strcmp(op, "SUB") == 0)
    {
        return SUB;
    }
    else if (strcmp(op, "MUL") == 0)
    {
        return MUL;
    }
    else if (strcmp(op, "LDI") == 0)
    {
        return LDI;
    }
    else if (strcmp(op, "BEQZ") == 0)
    {
        return BEQZ;
    }
    else if (strcmp(op, "AND") == 0)
    {
        return AND;
    }
    else if (strcmp(op, "OR") == 0)
    {
        return OR;
    }
    else if (strcmp(op, "JR") == 0)
    {
        return JR;
    }
    else if (strcmp(op, "SLC") == 0)
    {
        return SLC;
    }
    else if (strcmp(op, "SRC") == 0)
    {
        return SRC;
    }
    else if (strcmp(op, "LB") == 0)
    {
        return LB;
    }
    else if (strcmp(op, "SB") == 0)
    {
        return SB;
    }
    else
    {
        printf("str_opcode_to_char: Invalid opcode\n");
        return -1;
    }
}

int parse_program_file_to_inst_mem()
{
    char *filename = "program.txt";
    FILE *fp = fopen(filename, "r");
    short ctr = 0;
    if (fp == NULL)
    {
        printf("Error: could not open file %s", filename);
        return 1;
    }
    // reading line by line, max 256
    int MAX_LENGTH = 256;
    char buffer[MAX_LENGTH];

    while (fgets(buffer, MAX_LENGTH, fp))
    {
        short instruction;
        char *splitString = strtok(buffer, " ");
        char opcode = str_opcode_to_char(splitString);
        instruction = opcode << 12;
        int i = 0;
        while (splitString != NULL)
        {
            splitString = strtok(NULL, " ");
            if (i == 0)
            {
                char reg = atoi(splitString);
                instruction = instruction | reg << 6;
            }
            else if (i == 1)
            {
                char x = atoi(splitString);
                instruction = instruction | x & 0b000000000000111111;
                ;
            }
            i++;
        }

        instruction_memory[ctr] = instruction;
        instructions_cnt++;
        // printf("instruction_memory[%d]: %d\n", ctr, instruction_memory[ctr]);
        ctr++;
    }

    fclose(fp);
    return 0;
}

short fetch()
{
    short instruction = instruction_memory[register_file.pc++];
    if (register_file.pc == instructions_cnt)
        fetched_last_inst = true;
    printf("Instruction fetched: ");
    print_instruction(instruction);
    return instruction;
}

struct decoded_instruction decode(short instruction)
{
    struct decoded_instruction current_instruction;
    printf("Instruction decoded: ");
    print_instruction(instruction);
    current_instruction.instruction = instruction;
    current_instruction.opcode = (instruction >> 12) & 0xF;
    current_instruction.r1_address = (instruction >> 6) & 0x3F;
    current_instruction.r1_value = register_file.reg[current_instruction.r1_address];
    current_instruction.r2_address = (instruction) & 0x3F;
    current_instruction.r2_value = register_file.reg[current_instruction.r2_address];
    current_instruction.imm = (instruction) & 0x3F;
    if ((current_instruction.imm & 0b00100000) != 0)
    {
        current_instruction.imm = current_instruction.imm | 0b11000000;
    }
    current_instruction.pc_val_for_branch = register_file.pc;
    return current_instruction;
}

void add(char r1, char r2, char rd)
{
    short tmp = r1 + r2;
    // update carry flag 000C0000
    if ((tmp & 0b100000000) != 0)
        register_file.sreg = register_file.sreg | 0b00010000;
    else
        register_file.sreg = register_file.sreg & 0b11101111;

    // update overflow flag 0000V000
    bool sign1 = r1 & 0b10000000;
    bool sign2 = r2 & 0b10000000;
    bool sign_result = tmp & 0b10000000;
    if (sign1 == sign2 && sign_result != sign1)
        register_file.sreg = register_file.sreg | 0b00001000;
    else
        register_file.sreg = register_file.sreg & 0b11110111;

    // update negative flag 00000N00
    if ((tmp & 0b10000000) != 0)
        register_file.sreg = register_file.sreg | 0b00000100;
    else
        register_file.sreg = register_file.sreg & 0b11111011;

    // update sign flag S=V XOR N 000000S0
    bool sign_bit = ((register_file.sreg & 0b00001000) >> 3) ^ ((register_file.sreg & 0b00000100) >> 2);
    if (sign_bit == 1)
        register_file.sreg = register_file.sreg | 0b00000010;
    else
        register_file.sreg = register_file.sreg & 0b11111101;

    // update zero flag 0000000Z
    if (tmp == 0)
        register_file.sreg = register_file.sreg | 0b00000001;
    else
        register_file.sreg = register_file.sreg & 0b11111110;

    // write result to register
    register_file.reg[rd] = tmp;
}

void sub(char r1, char r2, char rd)
{
    add(r1, -r2, rd);
}

void mul(char r1, char r2, char rd)
{
    char tmp = r1 * r2;

    register_file.reg[rd] = tmp;

    // update negative flag 00000N00
    if ((tmp & 0b10000000) != 0)
        register_file.sreg = register_file.sreg | 0b00000100;
    else
        register_file.sreg = register_file.sreg & 0b11111011;

    // update zero flag 0000000Z
    if (tmp == 0)
        register_file.sreg = register_file.sreg | 0b00000001;
    else
        register_file.sreg = register_file.sreg & 0b11111110;

    // write result to register
    register_file.reg[rd] = tmp;
}

void and (char r1, char r2, char rd)
{
    char tmp = r1 & r2;

    // update negative flag 00000N00
    if ((tmp & 0b10000000) != 0)
        register_file.sreg = register_file.sreg | 0b00000100;
    else
        register_file.sreg = register_file.sreg & 0b11111011;

    // update zero flag 0000000Z
    if (tmp == 0)
        register_file.sreg = register_file.sreg | 0b00000001;
    else
        register_file.sreg = register_file.sreg & 0b11111110;

    // write result to register
    register_file.reg[rd] = tmp;
}

void or (char r1, char r2, char rd)
{
    char tmp = r1 | r2;

    // update negative flag 00000N00
    if ((tmp & 0b10000000) != 0)
        register_file.sreg = register_file.sreg | 0b00000100;
    else
        register_file.sreg = register_file.sreg & 0b11111011;

    // update zero flag 0000000Z
    if (tmp == 0)
        register_file.sreg = register_file.sreg | 0b00000001;
    else
        register_file.sreg = register_file.sreg & 0b11111110;

    // write result to register
    register_file.reg[rd] = tmp;
}

void slc(char r1, char imm, char rd)
{

    char tmp = r1 << imm | r1 >> (8 - imm);

    // update negative flag 00000N00
    if ((register_file.reg[r1] & 0b10000000) != 0)
        register_file.sreg = register_file.sreg | 0b00000100;
    else
        register_file.sreg = register_file.sreg & 0b11111011;

    // update zero flag 0000000Z
    if (register_file.reg[r1] == 0)
        register_file.sreg = register_file.sreg | 0b00000001;
    else
        register_file.sreg = register_file.sreg & 0b11111110;

    // write result to register
    register_file.reg[rd] = tmp;
}

void src(char r1, char imm, char rd)
{
    char tmp = r1 >> imm | r1 << (8 - imm);

    // update negative flag 00000N00
    if ((register_file.reg[r1] & 0b10000000) != 0)
        register_file.sreg = register_file.sreg | 0b00000100;
    else
        register_file.sreg = register_file.sreg & 0b11111011;

    // update zero flag 0000000Z
    if (register_file.reg[r1] == 0)
        register_file.sreg = register_file.sreg | 0b00000001;
    else
        register_file.sreg = register_file.sreg & 0b11111110;

    // write result to register
    register_file.reg[rd] = tmp;
}

short concatenate(char r1, char r2)
{
    return (r1 << 8) | r2;
}

int execute(struct decoded_instruction current_instruction)
{
    char opcode = current_instruction.opcode;
    char r1_val = current_instruction.r1_value;
    char r2_val = current_instruction.r2_value;
    char rd = current_instruction.r1_address;
    char imm = current_instruction.imm;
    printf("Instruction executed: ");
    print_instruction(current_instruction.instruction);
    // ALU operations
    switch (opcode)
    {
    case 0:
        // R-TYPE - ADD
        add(r1_val, r2_val, rd);
        printf("R%d value changed to %d.\n", rd, register_file.reg[rd]);
        // print sreg
        printf(">>>> Flags:\n");
        print_flags(register_file.sreg);
        break;
    case 1:
        // R-TYPE - SUB
        sub(r1_val, r2_val, rd);
        printf("R%d value changed to %d.\n", rd, register_file.reg[rd]);
        // print sreg
        printf(">>>> Flags:\n");
        print_flags(register_file.sreg);
        break;
    case 2:
        // R-TYPE - MUL
        mul(r1_val, r2_val, rd);
        printf("R%d value changed to %d.\n", rd, register_file.reg[rd]);
        // print sreg
        printf(">>>> Flags:\n");
        print_flags(register_file.sreg);
        break;
    case 3:
        // I-TYPE - LDI
        register_file.reg[rd] = imm;
        printf("R%d value changed to %d.\n", rd, register_file.reg[rd]);
        break;
    case 4:
        // I-TYPE - BEQZ
        if (r1_val == 0)
        {
            register_file.pc = current_instruction.pc_val_for_branch + imm;
            branch = true;
            ctr_branch=1;
            printf(">>>> PC branched to: %d, branch_flag=%d\n", register_file.pc, branch);
        }
        break;
    case 5:
        // R-TYPE - AND
        and(r1_val, r2_val, rd);
        printf("R%d value changed to %d.\n", rd, register_file.reg[rd]);
        // print sreg
        printf(">>>> Flags:\n");
        print_flags(register_file.sreg);
        break;
    case 6:
        // R-TYPE - OR
        or (r1_val, r2_val, rd);
        printf("R%d value changed to %d.\n", rd, register_file.reg[rd]);
        // print sreg
        printf(">>>> Flags:\n");
        print_flags(register_file.sreg);
        break;
    case 7:
        // R-TYPE - JR
        register_file.pc = concatenate(r1_val, r2_val);
        // print pc
        branch = true;
        ctr_branch=1;
        printf(">>>> PC jumped to: %d\n", register_file.pc);
        break;
    case 8:
        // I-TYPE - SLC
        slc(r1_val, imm, rd);
        printf("R%d value changed to %d.\n", rd, register_file.reg[rd]);
        // print sreg
        printf(">>>> Flags:\n");
        print_flags(register_file.sreg);
        break;
    case 9:
        // I-TYPE - SRC
        src(r1_val, imm, rd);
        printf("R%d value changed to %d.\n", rd, register_file.reg[rd]);
        // print sreg
        printf(">>>> Flags:\n");
        print_flags(register_file.sreg);
        break;
    case 10:
        // I-TYPE - LB
        register_file.reg[rd] = data_memory[imm];
        printf("R%d value changed to %d.\n", rd, register_file.reg[rd]);
        break;
    case 11:
        // I-TYPE - SB
        data_memory[imm] = r1_val;
        printf("mem[%d] value changed to %d.\n", imm, r1_val);
        break;
    default:
        printf("INVALID OPCODE\n");
        break;
    }

    return 0;
}

void next_cycle()
{
    short old_pc = register_file.pc;
    short pc_after_branch;
    clock_cycles++;

    if(branch){
        ctr_branch++;
        if(ctr_branch>3)
            branch=false;
    }

    printf("\n----------clock_cycle: [%d]----------\n",clock_cycles);
    if (clock_cycles > 2)
    {
        if(!(branch && ctr_branch<=3)){
            execute(previous_clock_cycle.instruction_decoded);
            if(branch)
                pc_after_branch = register_file.pc;
        }
    }
  
    if(!(branch && ctr_branch==2) && cnt_to_end<1)
        previous_clock_cycle.instruction_decoded = decode(previous_clock_cycle.instruction_fetched);
 
    if (!fetched_last_inst)
    {
        if (branch && ctr_branch==1)
        {
            register_file.pc = old_pc;
            previous_clock_cycle.instruction_fetched = fetch();
            register_file.pc = pc_after_branch;
        }
        else
        {
            previous_clock_cycle.instruction_fetched = fetch();
        }
    }
    else{
        cnt_to_end++;
        if(cnt_to_end==2){
            finish=true;
        }
    }
}

int main()

{
    parse_program_file_to_inst_mem();
    register_file.pc = 0;
    printf("\n----------clock_cycle: [%d]----------\n", clock_cycles);
    previous_clock_cycle.instruction_fetched = fetch();
    while (!finish)
    {
        next_cycle();
    }
    printf("\n>>>> TOTAL CLOCK CYCLES TO FINISH EXECUTION: %d\n", clock_cycles);
    printf(">>>> Registers:\n");
    for (int i = 0; i < 64; i++)
    {
        printf("R%d=%d ", i, register_file.reg[i]);
        if ((i > 0 && i % 8 == 0) || i == 63)
            printf("\n");
    }
    // print sreg
    printf(">>>> Flags:\n");
    print_flags(register_file.sreg);

    // print pc
    printf(">>>> PC: %d\n", register_file.pc);

    // print data memory
    // printf(">>>> Data Memory:\n");
    // for (int i = 0; i < 2048; i++)
    // {
    //     printf("mem[%d]=%d ", i, data_memory[i]);
    //     if((i>0 && i%8==0) || i==2047)
    //         printf("\n");
    // }
    // return 0;
}
