#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "src/rr_machine.h"

// Just for use inside the run command function
// Kind of ugly, but I got tired of copying/typing this stuff
#define STR_TO_UINT(str, var) {											\
	switch(string_to_unsigned((str), &(var), sizeof(var))) { 			\
																		\
		/* No error */													\
		case 0: 														\
		/* Invalid value size (should not ever occur) */				\
		case 1: 														\
			break; 														\
																		\
		/* Bad prefix */												\
		case 2:															\
			fprintf(stderr, "Unrecognized base prefix\n");				\
			return 1;													\
																		\
		/* Unable to convert */											\
		case 3:															\
			fprintf(stderr, "Unable to convert value specified\n");		\
			return 1;													\
																		\
		/* Should not occur */											\
		default:														\
			exit(1);													\
	}																	\
}

#define INPUT_BUFFER_SIZE 4096
#define OP_1_BUFFER_SIZE (INPUT_BUFFER_SIZE - 5)
#define OP_2_BUFFER_SIZE 7

#define COMMAND_SIZE 5
#define COMMAND_COUNT 10
#define SPECIAL_LOC_COUNT 5

const char *state_names[4] = {
	"Fetch\0",
	"Decode\0",
	"Execute\0",
	"Halt\0"
};

const char instruction_mnemonics[16][4] = {
	"HLT\0",
	"ADC\0",
	"AND\0",
	"XOR\0",
	"ROT\0",
	"LDI\0",
	"LDM\0",
	"LDR\0",
	"STO\0",
	"STR\0",
	"PSH\0",
	"POP\0",
	"JSR\0",
	"RET\0",
	"BRA\0",
	"MDF\0"
};

const char *command_helptext[COMMAND_COUNT << 1] = {
	"save <file path>\0",
	"saves the current main memory contents to a binary file\0",
	"load <file path>\0",
	"loads main memory with the contents of a 256 byte binary file\0",
	"step [<part|full>,<number of steps>]\0",
	"steps the machine in parts or full steps (full if not specified), number of steps defaults to 1 if not specified\0",
	"run [<part|full>,<delay>]\0",
	"runs the machine in partial or full steps (full if not specified) with an optional delay in milliseconds\0",
	"poke <location^>,<value>\0",
	"sets a given memory location to the specified value\0",
	"peek <location^>\0",
	"view a given memory location's value\0",
	"dump\0",
	"print all machine contents (main memory, general purpose registers, status register, instruction register, program counter\0",
	"reset\0",
	"resets machine registers\0",
	"clear\0",
	"clears machine memory\0"
	"help\0",
	"display all valid commands\0"
};

const char *location_helptext[SPECIAL_LOC_COUNT << 1] = {
	"r[0-15]\0",
	"(registers)\0",
	"sp\0",
	"(stack pointer)\0",
	"pc\0",
	"(program_counter)\0",
	"ir\0",
	"(instruction register) - poking may lead to undefined behavior\0",
	"sr\0",
	"(status register) - poking may lead to undefined behavior\0"
};

u8 string_to_unsigned(char *str, void *value_ptr, u8 value_bytes);
void string_to_lower(char *dest, char *src);
void help_command(char *cmd);
void display_helptext();
u8 run_command(rr_machine_t *machine, char *cmd, char **operands);

s32 main(s32 argc, const char **argv) {
	
	// User input buffer
	char input_buffer[INPUT_BUFFER_SIZE];
	char *token_str;
	char command_buffer[COMMAND_SIZE + 1];
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

		string_to_lower(input_buffer, input_buffer);

		token_str = strtok(input_buffer, " ");
		strncpy(command_buffer, token_str, COMMAND_SIZE);
		
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

u8 string_to_unsigned(char *str, void *value_ptr, u8 value_bytes) {
	
	u8 base;
	u64 value;
	char *end_ptr;
	char prefix;
	
	// Quick validation to ensure this is a standard primitive-sized integer
	switch(value_bytes) {
		
		case 1:
		case 2:
		case 4:
		case 8:
			break;
		
		// Bad value size
		default:
			return 1;
		
	}
	
	switch((prefix = *(str++))) {
		
		case '%':
			base = 2;
			break;
			
		case '0':
			base = 8;
			break;
			
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			base = 10;
			str--;
			break;
			
		case '$':
			base = 16;
			break;
			
		// Base error
		default:
			return 2;
		
	}
	
	value = strtoul(str, &end_ptr, base);
	
	// Unable to convert
	if(str == end_ptr)
		return 3;
	
	switch(value_bytes) {
		
		case 1:
			*(u8 *)value_ptr = (u8)value;
			break;
		
		case 2:
			*(u16 *)value_ptr = (u16)value;
			break;
		
		case 4:
			*(u32 *)value_ptr = (u32)value;
			break;
		
		case 8:
			*(u64 *)value_ptr = value;
			break;
		
	}
	
	return 0;
	
}

// dest = src.to_lower()
void string_to_lower(char *dest, char *src) {
	
	size_t c = strlen(src);
	
	do
		dest[c] = tolower(src[c]);
	while(c--);
	
}

void help_command(char *cmd) {
	
	u8 cmd_num = 0;
	
	char *temp;
	char buffer[1000];
	
	for(; cmd_num < (COMMAND_COUNT << 1); cmd_num += 2) {
		
		strcpy(buffer, command_helptext[cmd_num]);
		temp = strtok(buffer, " ");
		
		if(!strcmp(temp, cmd)) {
			fprintf(stdout, "%s\n%s\n", command_helptext[cmd_num], command_helptext[cmd_num + 1]);
			return;
		}
		
	}
	
	fprintf(stderr, "Unknown command\n");
	display_helptext();
	
}

void display_helptext() {
	
	u8 c = 0;
		
	for(; c < COMMAND_COUNT; c++)
		fprintf(stdout, "%-36s - %s\n", command_helptext[c << 1], command_helptext[(c << 1) + 1]);
	
	fprintf(stdout, "\n^Special locations include the following:\n");
	
	for(c = 0; c < SPECIAL_LOC_COUNT; c++)
		fprintf(stdout, "%-7s - %s\n", location_helptext[c << 1], location_helptext[(c << 1) + 1]);
	
}

u8 run_command(rr_machine_t *machine, char *cmd, char **operands) {
	
	fprintf(stdout, "\n");
	
	if(!strcmp(cmd, "save")) {
		
		if(!operands[0][0]) {
			fprintf(stderr, "Missing parameter for save\n");
			return 1;
		}
		
		switch(machine_save(machine, operands[0])) {
			
			case 0:
				fprintf(stdout, "Memory contents saved to file\n");
				break;
				
			case 1:
				fprintf(stderr, "Could not open file for saving\n");
				break;
			
			case 2:
				fprintf(stderr, "Could not write to file\n");
				break;
			
		}
		
	}
	else if(!strcmp(cmd, "load")) {
		
		if(!operands[0][0]) {
			fprintf(stderr, "Missing parameter for load\n");
			return 1;
		}
		
		switch(machine_load(machine, (const char *)operands[0])) {
			
			case 0:
				fprintf(stdout, "Loaded memory from file\n");
				break;
			
			case 1:
				fprintf(stderr, "Could not open file for loading\n");
				break;
			
			case 2:
				fprintf(stderr, "Could not read from file\n");
				break;
			
		}
		
	}
	else if(!strcmp(cmd, "step")) {
		
		u8 op0_specified = 0;
		u8 part_run = 0;
		u16 step_count = 1;
		
		if(!strcmp(operands[0], "part")) {
			
			part_run = 1;
			op0_specified = 1;
			
		}
		else if(!strcmp(operands[0], "full"))
			op0_specified = 1;
		
		if(operands[op0_specified][0])
			STR_TO_UINT(operands[op0_specified], step_count);
		
		while(step_count--)
			if(part_run)
				machine_step_part(machine);
			else
				machine_step_full(machine);
		
	}
	else if(!strcmp(cmd, "run")) {

		u8 op0_specified = 0;
		u8 part_run = 0;
		u16 delay_ms = 0;
		
		if(!strcmp(operands[0], "part")) {
			
			part_run = 1;
			op0_specified = 1;
			
		}
		else if(!strcmp(operands[0], "full"))
			op0_specified = 1;
		
		if(operands[op0_specified][0])
			STR_TO_UINT(operands[op0_specified], delay_ms);
		
		if(part_run)
			machine_run_part(machine, delay_ms);
		else
			machine_run_full(machine, delay_ms);
		
	}
	else if(!strcmp(cmd, "poke")) {
		
		u16 new_value; 
		
		if(!operands[0][0] || !operands[1][0]) {
			fprintf(stderr, "Missing parameter for poke\n");
			return 1;
		}
		
		STR_TO_UINT(operands[1], new_value);
		
		switch(operands[0][0]) {
			
			// Main memory
			case '%':
			case '$':
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
			
			{
				
				u8 mem_location;
				STR_TO_UINT(operands[1], mem_location);
				
				MEM(machine, mem_location) = new_value;
				
			}
			
				break;
			
			// Registers
			case 'r':
			
			{
				
				u8 reg_location;
				STR_TO_UINT(operands[0] + 1, reg_location);
				
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
		
		if(!operands[0][0]) {
			fprintf(stderr, "Missing parameter for peek\n");
			return 1;
		}
	
		switch(operands[0][0]) {
			
			// Main memory
			case '%':
			case '$':
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
			
			{
				
				u8 mem_location;
				STR_TO_UINT(operands[1], mem_location);
				
				fprintf(stdout, "[%02X]: $%02X\n", mem_location, MEM(machine, mem_location));
				
			}
			
				break;
			
			// Registers
			case 'r':
			
			{
				
				u8 reg_location;
				STR_TO_UINT(operands[0] + 1, reg_location);
				
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
		
		fprintf(stdout, "PC: [%02X] IR: [%04X] SR: [%02X]\n", machine->program_counter, machine->instruction_register, machine->status_register);
		
		fprintf(stdout, "    0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F       R\n");
		
		for(; c < 0xF; c++) {
			fprintf(stdout, "%X [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X]    [%02X]\n", c, MEM(machine, c * 0x10), MEM(machine, c * 0x10 + 1), MEM(machine, c * 0x10 + 2), MEM(machine, c * 0x10 + 3), MEM(machine, c * 0x10 + 4), MEM(machine, c * 0x10 + 5), MEM(machine, c * 0x10 + 6), MEM(machine, c * 0x10 + 7), MEM(machine, c * 0x10 + 8), MEM(machine, c * 0x10 + 9), MEM(machine, c * 0x10 + 10), MEM(machine, c * 0x10 + 11), MEM(machine, c * 0x10 + 12), MEM(machine, c * 0x10 + 13), MEM(machine, c * 0x10 + 14), MEM(machine, c * 0x10 + 15), REG(machine, c));
			
		}
		
		fprintf(stdout, "%X [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X] [%02X]  SP[%02X]\n", 0xF, MEM(machine, 0xF0), MEM(machine, 0xF1), MEM(machine, 0xF2), MEM(machine, 0xF3), MEM(machine, 0xF4), MEM(machine, 0xF5), MEM(machine, 0xF6), MEM(machine, 0xF7), MEM(machine, 0xF8), MEM(machine, 0xF9), MEM(machine, 0xFA), MEM(machine, 0xFB), MEM(machine, 0xFC), MEM(machine, 0xFD), MEM(machine, 0xFE), MEM(machine, 0xFF), REG(machine, 0xF));
		
	}
	else if(!strcmp(cmd, "reset"))
		machine_reset(machine);
	else if(!strcmp(cmd, "clear"))
		machine_clear_memory(machine);
	else if(!strcmp(cmd, "help")) {
	
		if(operands[0][0])
			help_command(operands[0]);
		else
			display_helptext();
		
		fprintf(stdout, "\n");
	
	}
	else {
		
		fprintf(stderr, "Unrecognized command entered. List of commands:\n");
		
		display_helptext();
		
		fprintf(stdout, "\n");
		
	}
	
	return 0;
	
}