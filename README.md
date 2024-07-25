# RR_Machine_Simulator
Simple machine language simulator written in C
Inspired/made in spite of the Brookshear machine. Intent is to eventually finish the delay portion of the run functions and add a graphical front end so that this can be used as an open source educational resource for introductory CS classes.
This machine is not meant to replicate existing modern CPU instruction sets or architecture, nor is it meant to necessarily emulate "good" CPU design. Instead, it is intended to be a simplified view of machine language and processor cycles for early students - similar to the Brookshear machine. 

Currently no command-line or graphical interface exists. The machine is a struct (rr_machine_t) consisting of the following fields:
  1-byte program counter
  1-byte status register - ____SSZC
    The top four bits are unused
    The next two bits (SS) are internal to the machine and hold its current state in the fetch-decode-execute cycle, as well as a fourth state for halt
    The last two bits are the zero and carry flags respectively, holding information on the results of previous instructions
  2-byte instruction register
  15 1-byte general-purpose registers
  1 1-byte special-purpose register, the stack pointer
    Empty stack convention, grows down from 0xFF
  256 bytes of main memory
  3-char string holding the current instruction name
    To be used in the future for the front end, populated during each decode step
  4 1-byte operand variables
    Also populated in the decode step, used in the execute step as the work is already done anyway

The following functions are provided to use the machine:
  rr_machine_t *macine_new() -> Allocates memory for a machine struct, initializes all values to 0 except the stack pointer (register 15) which is initialized to 255
  
  void machine_load(rr_machine_t *, const char *) -> Loads a 256 byte binary file to populate main memory
  void machine_save(rr_machine_t *, const char *) -> Saves the machine's current memory to a binary file
  
  void machine_step_part(rr_machine_t *) -> Runs the current part of the machine cycle (fetch, decode, or execute)
  void machine_step_full(rr_machine_t *) -> Runs all remaining parts of the current machine cycle

  void machine_run_part(rr_machine_t *, uint64_t) -> Runs the contents of memory by each cycle part until a halt instruction is executed - will eventually incorporate a delay
  void machine_run_part(rr_machine_t *, uint64_t) -> Runs the contents of memory by full cycles at a time until a halt instruction is executed - will eventually incorporate a delay

The following instructions can be used in main memory:
  HLT		0___	HaLT, suspends execution
  ADC		1RST	ADd w/ Carry, S + T + C -> R
  AND		2RST	AND registers, S & T -> R
  ORR		3RST	OR Registers, S | T -> R
  XOR		4RST	XOR registers, S ^ T -> R
  LDI		5RXX	LoaD Immediate, XX -> R
  LDM		6RMM	LoaD from Memory, Mem[MM] -> R
  STO		7RMM	STOre in memory, R -> Mem[MM]
  PSH		8R__	PuSH register to stack, R -> [SP--]
  POP		9R__	POP stack to register, [++SP] -> R
  JMP		A_XX	JuMP, XX -> PC
  JSR		B_XX	Jump to SubRoutine, PC + 2 -> [SP--]; XX -> PC
  RET		C___	RETurn from subroutine, [++SP] -> PC
  BRZ		D_XX	BRanch on Zero, Z ? (XX : PC + 2) -> PC
  BRC		E_XX	BRanch on Carry, C ? (XX : PC + 2) -> PC
  MDF		FI__	MoDify Flags*
*[I] can be broken down into two 2-bit fields as follows:
-01 -> Which flags (0 - Z, 1 - C) should be modified. If the flag's bit is 1, it will be changed to the corresponding bit in the second field. If the flag's bit is 0, it will remain as it was before the instruction
-23 -> Determines the new state of its respective flag
-e.g., 0111 would set the carry flag to 1 and leave Z as it was before. 0101 would do the same, as bit 0 being low tells the machine to ignore bit 2
