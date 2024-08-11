#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/rr_machine.h"

#define INPUT_BUFFER_SIZE 4096
#define OP_1_BUFFER_SIZE (INPUT_BUFFER_SIZE - 5)
#define OP_2_BUFFER_SIZE 7

const char *state_names[4] = {
	"Fetch",
	"Decode",
	"Execute",
	"Halt"
};

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

/*/
 * Valid commands for interacting with the machine:
 * 	save <file path> 						- saves the current main memory contents to a binary file
 *	load <file path> 						- loads main memory with the contents of a 256 byte binary file
 *	step [<part|full>,<number of steps>] 	- steps the machine in parts or full steps (full if not specified), number of steps defaults to 1 if not specified
 *	run [<part|full>,<delay>] 				- runs the machine in partial or full steps (full if not specified) with an optional delay in milliseconds
 *	poke <location^>,<value> 				- sets a given memory location to the specified value
 *	peek <location^>						- view a given memory location's value
 *	dump									- print all machine contents (main memory, general purpose registers, status register, instruction register, program counter
 * 
 * ^Special locations include the following:
 *	r[0-15] 	(registers)
 *	sp OR r15 	(stack pointer)
 *	pc 			(program_counter)
 *	ir 			(instruction register) - poking may lead to undefined behavior
 *	sr 			(status register)
/*/
u8 run_command(rr_machine_t *machine, char *cmd, char **operands);

s32 main(s32 argc, const char **argv) {
	
	// User input buffer
	char input_buffer[INPUT_BUFFER_SIZE];
	char *token_str;
	char command_buffer[5];
	char *operand_buffers[2];
	
	// Allocate space for 4090 character file path
	operand_buffers[0] = (char *)malloc(OP_1_BUFFER_SIZE);
	// Allocate space for delay/step count/value - works with just hexadecimal
	operand_buffers[1] = (char *)malloc(OP_2_BUFFER_SIZE);
	
	rr_machine_t *user_machine = machine_new();
	
	fprintf(stdout, "Issue commands to the machine (leave blank to exit):\n");
	
	while(1) {
		
		u8 op_counter = 0;
		
		// Get a line of input and remove the newline
		if(!fgets(input_buffer, INPUT_BUFFER_SIZE, stdin))
			exit(1);
		
		input_buffer[strcspn(input_buffer, "\n")] = 0;
		
		// Exit if nothing was input
		if(!input_buffer[0])
			break;

		token_str = strtok(input_buffer, " ");
		strcpy(command_buffer, token_str);
		
		while((token_str = strtok(NULL, ", ")))
			strcpy(operand_buffers[op_counter++], token_str); 
		
		run_command(user_machine, command_buffer, operand_buffers);
		
		memset(operand_buffers[0], 0, OP_1_BUFFER_SIZE);
		memset(operand_buffers[1], 0, OP_2_BUFFER_SIZE);
		
	}
	
	free(operand_buffers[0]);
	free(operand_buffers[1]);
	
	free(user_machine);
	
	return 0;
	
}

u8 run_command(rr_machine_t *machine, char *cmd, char **operands) {
	
	if(!strcmp(cmd, "save")) {
		
		machine_save(machine, operands[0]);
		
	}
	else if(!strcmp(cmd, "load")) {
		
		machine_load(machine, (const char *)operands[0]);
		
	}
	else if(!strcmp(cmd, "step")) {
		
		u8 run_type = strcmp(operands[0], "part");
		u16 step_count = operands[1] ? strtoul(operands[1], NULL, 16) : 1;
		
		for(; step_count > 0; step_count--)
			if(run_type)
				machine_step_part(machine);
			else
				machine_step_full(machine);
		
	}
	else if(!strcmp(cmd, "run")) {
		
		u8 run_type = strcmp(operands[0], "part");
		u16 delay_ms = operands[1] ? strtoul(operands[1], NULL, 16) : 0;
		
		if(run_type)
			machine_run_part(machine, delay_ms);
		else
			machine_run_full(machine, delay_ms);
		
	}
	else if(!strcmp(cmd, "poke")) {
		
		u16 new_value = strtoul(operands[1], NULL, 16);
		
		switch(operands[0][0]) {
			
			// Main memory
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case 'A':
			case 'a':
			case 'B':
			case 'b':
			case 'C':
			case 'c':
			case 'D':
			case 'd':
			case 'E':
			case 'e':
			case 'F':
			case 'f':
			
			{
				
				u8 mem_location = strtoul(operands[0], NULL, 16);
				
				MEM(machine, mem_location) = new_value;
				
			}
			
				break;
			
			// Registers
			case 'r':
			
			{
				
				u8 reg_location = strtoul(operands[0] + 1, NULL, 16);
				
				REG(machine, reg_location) = new_value;
				
			}
			
				break;
			
			// Special - these really *shouldn't* be poked, but could be educational to mess with
			case 's':
			case 'i':
			case 'p':
			
				if(!strcmp(operands[0], "sr"))
					machine->status_register = new_value & 0x0F;
				else if(!strcmp(operands[0], "sp"))
					STACK_POINTER(machine) = new_value & 0xFF;
				else if(!strcmp(operands[0], "ir"))
					machine->instruction_register = new_value;
				else if(!strcmp(operands[0], "pc"))
					machine->program_counter = new_value & 0xFF;
			
				break;
			
		}
		
		
	}
	else if(!strcmp(cmd, "peek")) {
	
		switch(operands[0][0]) {
			
			// Main memory
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case 'A':
			case 'a':
			case 'B':
			case 'b':
			case 'C':
			case 'c':
			case 'D':
			case 'd':
			case 'E':
			case 'e':
			case 'F':
			case 'f':
			
			{
				
				u8 mem_location = strtoul(operands[0], NULL, 16);
				
				fprintf(stdout, "[%02X]: $%02X\n", mem_location, MEM(machine, mem_location));
				
			}
			
				break;
			
			// Registers
			case 'r':
			
			{
				
				u8 reg_location = strtoul(operands[0] + 1, NULL, 16);
				
				fprintf(stdout, "Register %X: $%02X\n", reg_location, REG(machine, reg_location));
				
			}
			
				break;
			
			// Special
			case 's':
			case 'i':
			case 'p':
			
				if(!strcmp(operands[0], "sr"))
					fprintf(stdout, "Status register: %X (State: %s, Zero: %c, Carry: %c)\n", machine->status_register, state_names[CURRENT_STATE(machine)], (ZERO_SET(machine) ? 'Y' : 'N'), (CARRY_SET(machine) ? 'Y' : 'N'));
				else if(!strcmp(operands[0], "sp"))
					fprintf(stdout, "Stack pointer: $%02X (%u elements)\n", STACK_POINTER(machine), 0xFF - STACK_POINTER(machine));
				else if(!strcmp(operands[0], "ir")) {
					
					if(machine->status_register & 0b1000)
						fprintf(stdout, "Instruction register: $%04X (%s, %02X, %02X, %02X)\n", machine->instruction_register, instruction_mnemonics[machine->operands[0]], machine->operands[1], machine->operands[2], machine->operands[3]);
					else
						fprintf(stdout, "Instruction register: $%04X\n", machine->instruction_register);
					
				}
				else if(!strcmp(operands[0], "pc"))
					fprintf(stdout, "Program counter: %02X\n", machine->program_counter);
			
				break;
			
		}
		
	}
	else if(!strcmp(cmd, "dump")) {
		
		u8 c = 0;
		
		fprintf(stdout, "\nPC: [%02X] IR: [%04X] SR: [%02X]\n", machine->program_counter, machine->instruction_register, machine->status_register);
		
		fprintf(stdout, "    0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F       R\n");
		
		for(; c < 0xF; c++) {
			fprintf(stdout, "%X [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X]    [%02X]\n", c, MEM(machine, c * 0x10), MEM(machine, c * 0x10 + 1), MEM(machine, c * 0x10 + 2), MEM(machine, c * 0x10 + 3), MEM(machine, c * 0x10 + 4), MEM(machine, c * 0x10 + 5), MEM(machine, c * 0x10 + 6), MEM(machine, c * 0x10 + 7), MEM(machine, c * 0x10 + 8), MEM(machine, c * 0x10 + 9), MEM(machine, c * 0x10 + 10), MEM(machine, c * 0x10 + 11), MEM(machine, c * 0x10 + 12), MEM(machine, c * 0x10 + 13), MEM(machine, c * 0x10 + 14), MEM(machine, c * 0x10 + 15), REG(machine, c));
			
		}
		
		fprintf(stdout, "%X [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X]  SP[%02X]\n", 0xF, MEM(machine, 0xF0), MEM(machine, 0xF1), MEM(machine, 0xF2), MEM(machine, 0xF3), MEM(machine, 0xF4), MEM(machine, 0xF5), MEM(machine, 0xF6), MEM(machine, 0xF7), MEM(machine, 0xF8), MEM(machine, 0xF9), MEM(machine, 0xFA), MEM(machine, 0xFB), MEM(machine, 0xFC), MEM(machine, 0xFD), MEM(machine, 0xFE), MEM(machine, 0xFF), REG(machine, 0xF));
		
	}
	else 
		return -1;
	
	return 0;
	
}