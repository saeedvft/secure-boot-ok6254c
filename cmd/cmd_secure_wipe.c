// cmd/cmd_mycommand.c
#include <common.h>
#include <command.h>
#include <linux/delay.h>
#include <blk.h>
#include <mmc.h>
#include <part.h>

static int wipe_partition_table(struct blk_desc *dev_desc){
	unsigned long blocks = 34;
	void *zero_buf;
	int ret = 0;

	printf("Wiping partition table...\n");

	zero_buf = calloc(blocks, dev_desc->blksz);
	if (!zero_buf)
    		return -ENOMEM;

	ret = blk_dwrite(dev_desc, 0, blocks, zero_buf);
	free(zero_buf);

	if (ret != blocks) {
		printf("Error: Failed to wipe partition table\n");
		return -EIO;
	}

	printf("Partition table wiped\n");
	return 0;
}

static int wipe_bootloader(struct blk_desc *dev_desc, unsigned long size_mb){
	unsigned long blocks = 1024 * 1024 * size_mb / dev_desc->blksz;
	unsigned long chunck = 2048/*blocks*/;
	void *zero_buf;
	int ret = 0;
	int written = 0;
	int to_write = 0;

	printf("Wiping bootloader area (%lu MB)...\n", size_mb);

	zero_buf = calloc(blocks, dev_desc->blksz);
	if(!zero_buf){
		return -ENOMEM;
	}

	while(written < blocks){
		if (blocks -  written < chunck){
			to_write = blocks -  written;
		}
		else{
			to_write = chunck;
		}
		ret = blk_dwrite(dev_desc, written, chunck, zero_buf);
		if (ret != to_write) {
			free(zero_buf);
			return -EIO;
		}
		written += to_write;
	}

	free(zero_buf);

	printf("Bootloader wiped\n");
}

static int wipe_partition_headers(struct blk_desc *dev_desc){

}

int secure_wipe_disk(int device, int level){
	struct blk_desc *dev_desc;
	int ret = 0;

	printf("\n=================\n");
	printf("WIPING DISK!\n");
	printf("=================\n");

	dev_desc = blk_get_devnum_by_type(IF_TYPE_MMC, device);
	if (!dev_desc) {
		printf("Error: Cannot get MMC device %d\n", device);
		return -ENODEV;

	}
	printf("Device: MMC %d\n", device);
	printf("Wipe level: %d\n\n", level);

	switch(level){
		case(1):
			ret = wipe_partition_table(dev_desc);
		case(2):
			// wipe_partition_table;
			// wipe_bootloader - 8;
			// wipe_partition_headers'
		case(3):
			// wipe_bootloader - 100;
			// wipe_partition_headers;
		default:
			printf("Invalid Wipe level!\n");
	}

	if (ret == 0)
		printf("\n=== WIPE COMPLETED ===\n\n");
	else
		printf("\n=== WIPE FAILED ===\n\n");

	return ret;
}

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

	secure_wipe_disk(device, level);

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