// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <bootstage.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <env.h>
#include <init.h>
#include <net.h>
#include <version.h>

static void run_preboot_environment_command(void)
{
	char *p;

	p = env_get("preboot");
	if (p != NULL) {
		int prev = 0;

		if (IS_ENABLED(CONFIG_AUTOBOOT_KEYED))
			prev = disable_ctrlc(1); /* disable Ctrl-C checking */

		run_command_list(p, -1, 0);

		if (IS_ENABLED(CONFIG_AUTOBOOT_KEYED))
			disable_ctrlc(prev);	/* restore Ctrl-C checking */
	}
}

/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;
	const unsigned long expected_size = 3654; // boot.scr known good size in bytes
	char *filesize_env;
	unsigned long actual_size;

	/* Develop mode flag - set to 1 to enable debug logs, 0 for production */
	const int develop_mode = 0;

	/* Debug macro */
	#define DBG(fmt, ...) \
		do { \
			if (develop_mode) { \
				printf(fmt, ##__VA_ARGS__); \
			} \
		} while (0)

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");

	if (IS_ENABLED(CONFIG_VERSION_VARIABLE))
		env_set("ver", version_string);  /* set version variable */

	cli_init();

	if (IS_ENABLED(CONFIG_USE_PREBOOT))
		run_preboot_environment_command();

	if (IS_ENABLED(CONFIG_UPDATE_TFTP))
		update_tftp(0UL, NULL, NULL);

	DBG("=== SECURE BOOT INTEGRITY CHECK ===\n");

	/* Load boot.scr and get actual size */
	if (run_command("load mmc 1:1 0x88000000 boot.scr", 0) != 0) {
		DBG("SECURITY: Cannot load boot.scr!\n");
		goto security_failure;
	}

	/* Get file size from environment */
	filesize_env = env_get("filesize");
	if (!filesize_env) {
		DBG("SECURITY: Cannot get boot.scr size!\n");
		goto security_failure;
	}

	/* Convert hex string to number */
	actual_size = simple_strtoul(filesize_env, NULL, 16);

	DBG("DEBUG: Expected size: %lu bytes (0x%lX)\n", expected_size, expected_size);
	DBG("DEBUG: Actual size: %lu bytes (0x%lX)\n", actual_size, actual_size);

	/* Verify file size matches expected */
	if (actual_size != expected_size) {
		DBG("SECURITY: boot.scr size mismatch!\n");
		DBG("Expected: %lu bytes (0x%lX)\n", expected_size, expected_size);
		DBG("Actual: %lu bytes (0x%lX)\n", actual_size, actual_size);
		goto security_failure;
	}

	DBG("SUCCESS: boot.scr size verified (%lu bytes)\n", actual_size);

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

	autoboot_command(s);

	cli_loop();
	panic("No CLI available");

security_failure:
	DBG("SECURITY: Triggering secure wipe...\n");
	// run_command("secure_wipe 1 2", 0);
	// run_command("secure_wipe 1 1", 0);
	printf("System halted. Power cycle required.\n");
	hang();

	#undef DBG
}