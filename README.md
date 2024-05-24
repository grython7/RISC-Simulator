# RISC-Simulator

This project is a simple  RISC processor simulator implemented in C. The emulator can execute a subset of assembly instructions and provides basic functionalities for fetching, decoding, and executing instructions. It includes a simple memory model and register file.

## Architecture

- Harvard Architecture is used. Hence, two distinct memories, instruction memory and data memory are present.

## Features

- Supports basic arithmetic and logic instructions (ADD, SUB, MUL, AND, OR).
- Supports load and store instructions (LDI, LB, SB).
- Supports branch and jump instructions (BEQZ, JR).
- Supports shift and rotate instructions (SLC, SRC).
- Emulates a register file with 64 registers and a simple status register (SREG) for condition flags.

## Files

- `simulate.c`: The main source file containing the implementation of the RISC processor emulator.
- `program.txt`: A text file containing the assembly program to be loaded into the instruction memory.

## Usage

1. Edit the `program.txt` file to include the assembly instructions you want to simulate.
2. Run the compiled simulator as shown below.
3. The simulator will read the instructions from `program.txt`, execute them, and output the results to the console.

## Example

- An example program has been provided in [program.txt](https://github.com/theconjuring/RISC-simulator/program.txt)

## Compilation and Execution

To compile and run the simulator, follow these steps:

1. **Compile the code:**
   ```bash
   gcc -o simulate simulate.c

2. **Run the code:**
   ```bash
   ./simulate

## Output

The output of the simulator will display the registers that changed their values after each clock cycle, any relevant flags in the status register (SREG) and the final values of the registers.

## Status Register

- The 3 msb in the SREG are not used in this simulator and are set to 0.
1. C (Carry flag): Indicates that an arithmetic carry has been generated out of the msb
2. V (Overflow flag): Indicates whether there was an overflow
3. N (Negative flag): Indicates a negative result in an arithmetic or logical operation
4. S (Sign flag): Indicates the expected sign of result (not the actual sign)
5. Z (Zero flag): Indicates that the result is 0
