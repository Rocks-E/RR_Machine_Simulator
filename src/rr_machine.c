#include "rr_machine.h"

/*
const char instruction_mnemonics[16][3] = {
	"HLT",
	"ADC",
	"AND",
	"XOR",
	"ROT",
	"LDI",
	"LDM",
	"LDR",
	"STO",
	"STR",
	"PSH",
	"POP",
	"JSR",
	"RET",
	"BRA",
	"MDF"
};
*/

// Create a base machine
rr_machine_t *machine_new() {
	
	rr_machine_t *machine = (rr_machine_t *)calloc(1, sizeof(rr_machine_t));
	
	// Set the stack pointer to 0xFF, since we will build down from the end of memory
	STACK_POINTER(machine) = 0xFF;
	
	return machine;
	
}

// Load/save machine memory from file
void machine_load(rr_machine_t *machine, const char *memory_filename) {
	
	FILE *mem_file = fopen(memory_filename, "r");
	
	fread((char *)machine->memory, sizeof(u8), 256, mem_file);
	
	fclose(mem_file);
	
}
void machine_save(rr_machine_t *machine, const char *memory_filename) {
	
	FILE *mem_file = fopen(memory_filename, "w");
	
	fwrite((char *)machine->memory, sizeof(u8), 256, mem_file);
	
	fclose(mem_file);
	
}

// Run the three parts of the machine cycle (internal)
void machine_fetch(rr_machine_t *machine) {
	
	machine->instruction_register = MEM(machine, machine->program_counter) << 8;
	machine->instruction_register |= MEM(machine, machine->program_counter + 1);
	
}
void machine_decode(rr_machine_t *machine) {
	
	// Clear current operands
	memset(machine->operands, 0, 4);
	
	// Get this instruction number and copy the instruction mnemonic to the machine
	u8 instr = machine->instruction_register >> 12;
	//memcpy(machine->instruction_name, instruction_mnemonics[instr], 3);
	
	// Set operands[0] to the instruction number so we don't need to shift again during execute
	machine->operands[0] = instr;
	
	switch(instr) {
		
		// HLT - 0___
		// HaLT machine execution
		case 0x0:
		// RET - D___
		// RETurn from subroutine, Mem[++SP] -> PC
		case 0xD:
			break;
		// ADC - 1RST
		// ADd with Carry, adds S + T + C -> R
		// Sets carry if the addition exceeds 0xFF, clears otherwise
		// Sets zero if R = 0, clears otherwise
		case 0x1:
		// AND - 2RST
		// AND registers, S & T -> R
		// Sets zero if R = 0, clears otherwise
		case 0x2:
		// XOR - 3RST
		// XOR registers, S ^ T -> R
		// Sets zero if R = 0, clears otherwise
		case 0x3:
		// ROT - 4RST
		// ROTate register, S (<< | >>) T -> R
		// T[4] controls direction, 0 for left, 1 for right
		// Sets zero if R = 0, clears otherwise
		// Carry will be set according to the last bit from S to be rotated out
		case 0x4:
			machine->operands[1] = (machine->instruction_register >> 8) & 0xF;
			machine->operands[2] = (machine->instruction_register >> 4) & 0xF;
			machine->operands[3] = machine->instruction_register & 0xF;
			break;
		// LDI - 5RXX
		// LoaD Immediate, XX -> R
		// Sets zero if R = 0, clears otherwise
		case 0x5:
		// LDM - 6RMM
		// LoaD from Memory, Mem[MM] -> R
		// Sets zero if R = 0, clears otherwise
		case 0x6:
		// STO - 8RMM
		// STOre register, R -> Mem[MM]
		// Sets zero if R = 0, clears otherwise
		case 0x8:
			machine->operands[1] = (machine->instruction_register >> 8) & 0xF;
			machine->operands[2] = machine->instruction_register & 0xFF;
			break;
		// LDR - 7_RS
		// LoaD with Register offset, Mem[S] -> R
		// Sets zero if R = 0, clears otherwise
		case 0x7:
		// STR - 9_RS
		// STore with Register offset, R -> Mem[S]
		// Sets zero if R = 0, clears otherwise
		case 0x9:
			machine->operands[1] = (machine->instruction_register >> 4) & 0xF;
			machine->operands[2] = machine->instruction_register & 0xF;
			break;
		// PSH - AR__
		// PuSH register to stack, R -> [SP--]
		case 0xA:
		// POP - BR__
		// POP stack to register, [++SP] -> R
		// Sets zero if R = 0
		case 0xB:
			machine->operands[1] = (machine->instruction_register >> 8) & 0xF;
			break;
		// JSR - C_XX
		// Jump to SubRoutine, PC + 2 -> Mem[SP--], XX -> PC
		case 0xC:
			machine->operands[1] = machine->instruction_register & 0xFF;
			break;
		// BRA - EIXX
		// BRAnch on flag conditions, I ? (XX : PC + 2) -> PC
		// I consists of two 2-bit fields
		// 01 -> which flags should be considered
		// 23 -> the state of considered flags to branch on
		// ex 1, 1100 -> branch only if both zero and carry are cleared
		// ex 2, 0111 -> branch only if carry is set, ignore the zero flag (interchangeable with 0101)
		// If I <= 3, this will be an unconditional jump
		case 0xE:
			machine->operands[1] = (machine->instruction_register >> 10) & 0x3;
			machine->operands[2] = (machine->instruction_register >> 8) & 0x3;
			machine->operands[3] = machine->instruction_register & 0xFF;
			break;
		// MDF - FI__
		// MoDify Flag, sets the given status flag to the specified value
		// I consists of two 2-bit fields
		// 01 -> which flags (Z, C) should be modified
		// 23 -> state of the flag to update
		// e.g., 0111 -> set C to 1, leave Z however it is (interchangeable with 0101)
		case 0xF:
			machine->operands[1] = (machine->instruction_register >> 10) & 0x3;
			machine->operands[2] = (machine->instruction_register >> 8) & 0x3;
			break;
		
	}
	
}
void machine_execute(rr_machine_t *machine) {
	
	switch(machine->operands[0]) {
		
		// Halt
		case 0x0:
			machine->status_register |= 0b1100;
			break;
		
		// Add with carry
		case 0x1:
			
			{
				
				u16 temp = REG(machine, machine->operands[2]) + REG(machine, machine->operands[3]) + CARRY_SET(machine);
				REG(machine, machine->operands[1]) = temp & 0xFF;
				
				// Update status register - [SS] -> maintain, [Z] -> set when the result is 0, [C] -> set when the result exceeds 255
				machine->status_register = (machine->status_register & 0b1100) | (((temp & 0xFF) == 0) << 1) | (temp > 0xFF);
				
			}
	
			break;
			
		// AND
		case 0x2:
			
			REG(machine, machine->operands[1]) = REG(machine, machine->operands[2]) & REG(machine, machine->operands[3]);
		
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
			
		// XOR
		case 0x3:
		
			REG(machine, machine->operands[1]) = REG(machine, machine->operands[2]) ^ REG(machine, machine->operands[3]);
		
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
			
		// Rotate register
		case 0x4:
		
			// Early break if this should not rotate at all
			if((REG(machine, machine->operands[3]) & 0b0111) == 0)
				break;
		
			{
		
				u16 temp;
				u8 shift_count = REG(machine, machine->operands[3]) & 0b0111;
		
				// Rotate right
				if(REG(machine, machine->operands[3]) & 0b1000) {
					
					temp = CARRY_SET(machine) << (8 - shift_count);
					temp |= REG(machine, machine->operands[2]) >> shift_count;
					temp |= REG(machine, machine->operands[2]) << (9 - shift_count);
					
					
					machine->status_register = (machine->status_register & 0b1100) | (!REG(machine, machine->operands[1]) << 1) | ((REG(machine, machine->operands[2]) >> (shift_count - 1)) & 1);
					
					REG(machine, machine->operands[1]) = temp & 0xFF;
					
				}
				// Rotate left
				else {
					
					temp = CARRY_SET(machine) << (shift_count - 1);
					temp |= REG(machine, machine->operands[2]) >> (9 - shift_count);
					temp |= REG(machine, machine->operands[2]) << shift_count;
					
					machine->status_register = (machine->status_register & 0b1100) | (!REG(machine, machine->operands[1]) << 1) | ((REG(machine, machine->operands[2]) >> (8 - shift_count)) & 1);
					
					REG(machine, machine->operands[1]) = temp & 0xFF;

				}
				
			}
	
			break;
			
		// Load immediate
		case 0x5:
		
			REG(machine, machine->operands[1]) = machine->operands[2];
			
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
			
		// Load from memory
		case 0x6:
		
			REG(machine, machine->operands[1]) = MEM(machine, machine->operands[2]);
			
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
		
		// Load from memory with register offset
		case 0x7:
		
			REG(machine, machine->operands[1]) = MEM(machine, REG(machine, machine->operands[2]));
		
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
			
		// Store
		case 0x8:
		
			MEM(machine, machine->operands[2]) = REG(machine, machine->operands[1]);
			
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
			
		// Store at register offset
		case 0x9:
		
			MEM(machine, REG(machine, machine->operands[2])) = REG(machine, machine->operands[1]);
			
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
			
			break;
			
		// Push register
		case 0xA:
		
			MACHINE_PUSH(machine, REG(machine, machine->operands[1]));
			
			break;
			
		// Pop to register
		case 0xB:
		
			REG(machine, machine->operands[1]) = MACHINE_POP(machine);
			
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
		
		// Jump subroutine
		case 0xC:
		
			MACHINE_PUSH(machine, machine->program_counter + 2);
			
			machine->program_counter = machine->operands[1] - 2;
			
			break;
		
		// Return from subroutine
		case 0xD:
		
			machine->program_counter = MACHINE_POP(machine);
		
			// Return instead of break so we skip adding to the program counter - this would make it problematic if the user wanted to change or read the value in the stack region
			return;
		
		// Branch on flag conditions
		case 0xE:
		
			// Unconditional jump if I <= 3, since operands[1] will be 0 as 0 & X == 0 always
			if((machine->operands[1] & ~(machine->status_register ^ machine->operands[2])) == machine->operands[1])
				machine->program_counter = machine->operands[3] - 2;
		
			break;
			
		// Modify flags
		case 0xF:
		
			if(machine->operands[1] & 0b0010)
				machine->status_register = (machine->status_register & 0b1101) | (machine->operands[2] & 0b0010);
			if(machine->operands[1] & 0b0001)
				machine->status_register = (machine->status_register & 0b1110) | (machine->operands[2] & 0b0001);

			break;
		
	}
	
	machine->program_counter += 2;
	
}

// Run whichever part of the machine cycle the machine is on
void machine_step_part(rr_machine_t *machine) {
	
	switch(machine->status_register >> 2) {
		
		case 0b00:
			machine_fetch(machine);
			machine->status_register |= 0b0100;
			break;
			
		case 0b01:
			machine_decode(machine);
			machine->status_register += 0b0100;
			break;
		
		case 0b10:
			machine_execute(machine);
			if(((machine->status_register) >> 2) != 3)
			machine->status_register &= 0b0011;
			break;
		
		case 0b11:
			// Halt
		
	}
	
}
// Run a full machine cycle (up to the next one, will only run the current cycle to the end)
void machine_step_full(rr_machine_t *machine) {
	
	u8 c = 3 - (machine->status_register >> 2);
	
	for(; c > 0; c--)
		machine_step_part(machine);
	
}

// Run the entire program (up to a HALT) in parts or full cycles with an optional delay between each part/cycle
void machine_run_part(rr_machine_t *machine, u64 delay) {
	
	while((machine->status_register >> 2) < 0b11) {
		
		machine_step_part(machine);
		
		// Delay?
		
	}
	
}
void machine_run_full(rr_machine_t *machine, u64 delay) {
	
	while((machine->status_register >> 2) < 0b11) {
		
		machine_step_full(machine);
		
		// Delay?
		
	}
	
}