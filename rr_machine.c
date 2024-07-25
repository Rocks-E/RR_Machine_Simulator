#include "rr_machine.h"

const char *instruction_mnemonics[16] = {
	"HLT",
	"ADC",
	"AND",
	"ORR",
	"XOR",
	"LDI",
	"LDM",
	"STO",
	"PSH",
	"POP",
	"JMP",
	"JSR",
	"RET",
	"BRZ",
	"BRC",
	"MDF"
};

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
	memcpy(machine->instruction_name, instruction_mnemonics[instr], 3);
	
	// Set operands[0] to the instruction number so we don't need to shift again during execute
	machine->operands[0] = instr;
	
	switch(instr) {
		
		// HLT - 0___
		// HaLT machine execution
		case 0x0:
		// RET - C___
		// RETurn from subroutine, Mem[++SP] -> PC
		case 0xC:
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
		// ORR - 3RST
		// OR Registers, S | T -> R
		// Sets zero if R = 0, clears otherwise
		case 0x3:
		// XOR - 4RST
		// XOR registers, S ^ T -> R
		// Sets zero if R = 0, clears otherwise
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
		// STO - 7RMM
		// STOre register, R -> Mem[MM]
		// Sets zero if R = 0, clears otherwise
		case 0x7:
			machine->operands[1] = (machine->instruction_register >> 8) & 0xF;
			machine->operands[2] = machine->instruction_register & 0xFF;
			break;
		// PSH - 8R__
		// PuSH register to stack, R -> [SP--]
		// Sets zero if R = 0, clears otherwise
		case 0x8:
		// POP - 9R__
		// POP stack to register, [++SP] -> R
		case 0x9:
			machine->operands[1] = (machine->instruction_register >> 8) & 0xF;
			break;
		// JMP - A_XX
		// unconditional JuMP, XX -> PC
		case 0xA:
		// JSR - B_XX
		// Jump to SubRoutine, PC + 2 -> Mem[SP--], XX -> PC
		case 0xB:
		// BRZ - D_XX
		// BRanch on Zero set, Z ? (XX : PC + 2) -> PC
		case 0xD:
		// BRC - E_XX
		// BRanch on Carry set, C ? (XX : PC + 2) -> PC
		case 0xE:
			machine->operands[1] = (machine->instruction_register) & 0xFF;
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
				
				u16 temp = REG(machine, machine->operands[2]) + REG(machine, machine->operands[3]) + (machine->status_register & 1);
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
			
		// OR
		case 0x3:
		
			REG(machine, machine->operands[1]) = REG(machine, machine->operands[2]) | REG(machine, machine->operands[3]);
		
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
			
		// XOR
		case 0x4:
		
			REG(machine, machine->operands[1]) = REG(machine, machine->operands[2]) ^ REG(machine, machine->operands[3]);
		
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
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
			
		// Store
		case 0x7:
		
			MEM(machine, machine->operands[2]) = REG(machine, machine->operands[1]);
			
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
			
		// Push register
		case 0x8:
		
			MACHINE_PUSH(machine, REG(machine, machine->operands[1]));
			
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
			
			break;
			
		// Pop to register
		case 0x9:
		
			REG(machine, machine->operands[1]) = MACHINE_POP(machine);
			
			// Update status register - [SS_C] -> maintain, [Z] -> set when the result is 0
			machine->status_register = (machine->status_register & 0b1101) | ((REG(machine, machine->operands[1]) == 0) << 1);
		
			break;
			
		// Jump unconditional
		case 0xA:
		
			machine->program_counter = machine->operands[1] - 2;
		
			break;
		
		// Jump subroutine
		case 0xB:
		
			MACHINE_PUSH(machine, machine->program_counter + 2);
			
			machine->program_counter = machine->operands[1] - 2;
			
			break;
		
		// Return from subroutine
		case 0xC:
		
			machine->program_counter = MACHINE_POP(machine);
		
			// Return instead of break so we skip adding to the program counter - this would make it problematic if the user wanted to change or read the value in the stack region
			return;
		
		// Branch on zero set
		case 0xD:
		
			if(machine->status_register & 0b0010)
				machine->program_counter = machine->operands[1] - 2;
		
			break;
			
		// Branch on carry set
		case 0xE:
		
			if(machine->status_register & 0b0001)
				machine->program_counter = machine->operands[1] - 2;
		
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