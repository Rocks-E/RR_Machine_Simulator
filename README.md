# RR_Machine_Simulator
Simple machine language simulator written in C

Inspired/made in spite of the Brookshear machine. Intent is to eventually finish the delay portion of the run functions and add a graphical front end so that this can be used as an open source educational resource for introductory CS classes.

This machine is not meant to replicate existing modern CPU instruction sets or architecture, nor is it meant to necessarily emulate "good" CPU design. Instead, it is intended to be a simplified view of machine language and processor cycles for early students - similar to the Brookshear machine. 

Currently no command-line or graphical interface exists. The machine is a struct (rr_machine_t) consisting of the following fields:
- 1-byte program counter
- 1-byte status register - ____SSZC
  - The top four bits are unused
  - The next two bits (SS) are internal to the machine and hold its current state in the fetch-decode-execute cycle, as well as a fourth state for halt
  - The last two bits are the zero and carry flags respectively, holding information on the results of previous instructions
- 2-byte instruction register
- **15** 1-byte general-purpose registers
- **1** 1-byte special-purpose register, the stack pointer
  - Empty stack convention, grows down from 0xFF
- 256 bytes of main memory
- 3-char string holding the current instruction name
  - To be used in the future for the front end, populated during each decode step
- **4** 1-byte operand variables
  - Also populated in the decode step, used in the execute step as the work is already done anyway

The following functions are provided to use the machine:
- `rr_machine_t *machine_new()` -> Allocates memory for a machine struct, initializes all values to 0 except the stack pointer (register 15) which is initialized to 255
  
- `void machine_load(rr_machine_t *, const char *)` -> Loads a 256 byte binary file to populate main memory
- `void machine_save(rr_machine_t *, const char *)` -> Saves the machine's current memory to a binary file

- `void machine_step_part(rr_machine_t *)` -> Runs the current part of the machine cycle (fetch, decode, or execute)
- `void machine_step_full(rr_machine_t *)` -> Runs all remaining parts of the current machine cycle

- `void machine_run_part(rr_machine_t *, uint64_t)` -> Runs the contents of memory by each cycle part until a halt instruction is executed - will eventually incorporate a delay
- `void machine_run_part(rr_machine_t *, uint64_t)` -> Runs the contents of memory by full cycles at a time until a halt instruction is executed - will eventually incorporate a delay

The following instructions can be used in main memory:
- HLT
  - 0___
  - HaLT
  - Suspends execution
- ADC
  - 1RST
  - ADd w/ Carry
  - S + T + C -> R
- AND
  - 2RST
  - AND registers
  - S & T -> R
- ORR
  - 3RST
  - OR Registers
  - S | T -> R
- XOR
  - 4RST
  - XOR registers
  - S ^ T -> R
- ROT
  - 5RST
  - ROTate register
  - T[4] controls direction (0 for left, 1 for right)
  - T[567] control the count to rotate S by (0-7)
  - The carry bit is used as a "9th" bit off the left side for a left rotate or the right side for a right rotate
- LDI
  - 6RXX
  - LoaD Immediate
  - XX -> R
- LDM
  - 7RMM
  - LoaD from Memory
  - Mem[MM] -> R
- LDR
  - 8_RS
  - LoaD from memory with Register offset
  - Mem[S] -> R 
- STO
  - 9RMM
  - STOre in memory
  - R -> Mem[MM]
- PSH
  - AR__
  - PuSH register to stack
  - R -> [SP--]
- POP
  - BR__
  - POP stack to register
  - [++SP] -> R
- JSR
  - C_XX
  - Jump to SubRoutine
  - PC + 2 -> [SP--]; XX -> PC
- RET
  - D___
  - RETurn from subroutine
  - [++SP] -> PC
- BRA
  - EIXX
  - BRAnch on status conditions*
- MDF
  - FI__
  - MoDify Flags*

*[I] can be broken down into two 2-bit fields as follows:
- 01 -> Which flags (0 - Z, 1 - C) should be considered. If the flag's bit is 1, the corresponding bit in the second field is used. If the flag's bit is 0, the corresponding bit in the second field is ignored.
- 23 -> If the corresponding bit in the first field was set, determines what to do with the status flag
- BRA ex., 0111 would branch if the carry flag is 1, regardless of the state of the zero flag. 0101 would do the same, as the zero flag bit in the first field is low.
- Note: If I <= 3 it will ALWAYS be an unconditional jump as neither flag is considered
- MDF ex., 1001 would set the zero flag to 0 and leave the carry flag as it was before. 1000 would do the same, as bit 0 being low tells the machine to ignore bit 3.
