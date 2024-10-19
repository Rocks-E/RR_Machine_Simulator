#ifndef RR_MACHINE_H
#define RR_MACHINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Redefines datatypes for simplicity, includes inttypes.h
#include "../../shared/shared_datatypes.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__unix__) 
#include <time.h>
#include <errno.h>
#else
#error "PLATFORM NOT SUPPORTED"
#endif

#define REG(m, x) (m->registers[x])
#define MEM(m, x) (m->memory[x])
#define CURRENT_STATE(m) (m->status_register >> 2)
#define ZERO_SET(m) ((m->status_register >> 1) & 1)
#define CARRY_SET(m) (m->status_register & 1)
#define STACK_POINTER(m) (REG(m, 15))
// m->memory[m->registers[15]++] = x
#define MACHINE_PUSH(m, x) (MEM(m, STACK_POINTER(m)--) = x)
// m->memory[++(m->registers[15])]
#define MACHINE_POP(m) (MEM(m, ++STACK_POINTER(m)))

typedef struct rr_machine_d {
	// Decode variables, holds operands
	u8 operands[4];
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
} rr_machine_t;

// Create a base machine
rr_machine_t *machine_new();

u8 machine_reset(rr_machine_t *machine);
u8 machine_clear_memory(rr_machine_t *machine);

// Load/save machine memory from file
u8 machine_load(rr_machine_t *machine, const char *memory_filename);
u8 machine_save(rr_machine_t *machine, const char *memory_filename);

// Run whichever part of the machine cycle the machine is on
u8 machine_step_part(rr_machine_t *machine);
// Run a full machine cycle (up to the next one, will only run the current cycle to the end)
u8 machine_step_full(rr_machine_t *machine);

// Run the entire program (up to a HALT) in parts or full cycles with an optional delay between each part/cycle
u8 machine_run_part(rr_machine_t *machine, u64 delay);
u8 machine_run_full(rr_machine_t *machine, u64 delay);

#endif