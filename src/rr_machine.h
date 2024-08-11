#ifndef RR_MACHINE_H
#define RR_MACHINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define REG(m, x) (m->registers[x])
#define MEM(m, x) (m->memory[x])
#define STACK_POINTER(m) (REG(m, 15))
// m->memory[m->registers[15]++] = x
#define MACHINE_PUSH(m, x) (MEM(m, STACK_POINTER(m)--) = x)
// m->memory[++(m->registers[15])]
#define MACHINE_POP(m) (MEM(m, ++STACK_POINTER(m)))

typedef uint8_t 	u8;
typedef uint16_t 	u16;
typedef uint32_t 	u32;
typedef uint64_t 	u64;
typedef int8_t 		s8;
typedef int16_t 	s16;
typedef int32_t 	s32;
typedef int64_t 	s64;
typedef float 		f32;
typedef double 		f64;

typedef struct rr_machine_d {
	// Controls where the program is in memory
	u8 program_counter;
	// Status register, SSZC - State (00 -> fetch, 01 -> decode, 10 -> execute, 11 -> halt), Zero, Carry
	u8 status_register;
	// Holds the current instruction and operands
	u16 instruction_register;
	// 0-14 -> general purpose registers
	// 15 -> stack pointer
	u8 registers[16];
	// 256 bytes for RAM
	u8 memory[256];
	// Decode variables, holds instruction mnemonics and operands
	char instruction_name[3];
	u8 operands[4];
} rr_machine_t;

// Create a base machine
rr_machine_t *machine_new();

// Load/save machine memory from file
void machine_load(rr_machine_t *machine, const char *memory_filename);
void machine_save(rr_machine_t *machine, const char *memory_filename);

// Run whichever part of the machine cycle the machine is on
void machine_step_part(rr_machine_t *machine);
// Run a full machine cycle (up to the next one, will only run the current cycle to the end)
void machine_step_full(rr_machine_t *machine);

// Run the entire program (up to a HALT) in parts or full cycles with an optional delay between each part/cycle
void machine_run_part(rr_machine_t *machine, u64 delay);
void machine_run_full(rr_machine_t *machine, u64 delay);

#endif