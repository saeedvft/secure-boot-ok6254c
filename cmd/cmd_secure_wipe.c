// cmd/cmd_mycommand.c
#include <common.h>
#include <command.h>
#include <linux/delay.h>

static int do_secure_wipe(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{

	int device = 0; // Default device
	int level = 2;  // Default wipe level (standard)

	if (argc > 1){
		device = simple_strtoul(argv[1], NULL, 10);
	}
	if (argc > 2){
		level = simple_strtoul(argv[2], NULL, 10);
	}



	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	secure_wipe,		// Command name (what you type)
	3, 			// Maximum number of arguments
	0,			// Repeatable (1=yes, 0=no)
	do_secure_wipe,		// Function to call
	"securely wipe disk on security violation",
	"[device] [level]\n"
	"	device - MMC device number (default: 0)\n"
	"	level  - 1:minimal 2:standard(default) 3:paranoid"
);