// cmd/cmd_mycommand.c
#include <common.h>
#include <command.h>
#include <linux/delay.h>

static int do_mycommand(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
    int i;

    printf("=== My Custom U-Boot Command ===\n");

    // Parse arguments
    if (argc > 1) {
        printf("Arguments received:\n");
        for (i = 1; i < argc; i++) {
            printf("  argv[%d] = %s\n", i, argv[i]);
        }
    }

    // Example: Blink LED or simple delay
    printf("Working...\n");
    mdelay(1000);
    printf("Done!\n");

    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
    mycommand,          // Command name (what you type)
    CONFIG_SYS_MAXARGS, // Maximum number of arguments
    1,                  // Repeatable (1=yes, 0=no)
    do_mycommand,       // Function to call
    "My custom command for testing", // Short description
    "[args...]\n"       // Detailed help
    "    - Execute my custom functionality\n"
    "    - Optional arguments can be provided"
);