#include <string.h>
#include "src/rr_machine.h"

#define INPUT_BUFFER_SIZE 4096
#define OP_1_BUFFER_SIZE (INPUT_BUFFER_SIZE - 6)
#define OP_2_BUFFER_SIZE 5

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
	// Allocate space for up to 4 digit delay/step count/value (only 2 digits are used for values)
	operand_buffers[1] = (char *)malloc(OP_2_BUFFER_SIZE);
	
	// Parameters for save/load commands and full run commands respectively
	char *filename;
	u64 delay;
	
	rr_machine_t *user_machine = machine_new();
	
	while(1) {
		
		u8 op_counter = 0;
		
		fgets(input_buffer, INPUT_BUFFER_SIZE, stdin);
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
	
	fprintf(stdout, "%s: ", cmd);
	
	if(!strcmp(cmd, "save")) {
		
		fprintf(stdout, "%s\n", operands[0]);
		
		machine_save(machine, operands[0]);
		
	}
	else if(!strcmp(cmd, "load")) {
		
		fprintf(stdout, "%s\n", operands[0]);
		
		machine_load(machine, (const char *)operands[0]);
		
	}
	else if(!strcmp(cmd, "step")) {
		
		fprintf(stdout, "%s, %s\n", operands[0], operands[1]);
		
//		if(operands[1]
		
//		if(strcmp(operands[0], "part"))
			
		
	}
	else if(!strcmp(cmd, "run")) {
		
		fprintf(stdout, "%s, %s\n", operands[0], operands[1]);
		
	}
	else if(!strcmp(cmd, "poke")) {
		
		fprintf(stdout, "%s, %s\n", operands[0], operands[1]);
		
	}
	else if(!strcmp(cmd, "peek")) {
		
		fprintf(stdout, "%s\n", operands[0]);
		
	}
	else if(!strcmp(cmd, "dump")) {}
	else 
		return -1;
	
	return 0;
	
}