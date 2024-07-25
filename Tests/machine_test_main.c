#include <assert.h>
#include "rr_machine.h"

s32 main(s32 argc, const char **argv) {
	
	rr_machine_t *test_machine = machine_new();
	
	machine_load(test_machine, "./test_a.bin");

	machine_run_full(test_machine, 0);
	
	machine_save(test_machine, "./test_a_out.bin");
	
	return 0;
	
}