// cmd/cmd_secure_wipe.c
#include <common.h>
#include <command.h>
#include <linux/delay.h>
#include <blk.h>
#include <mmc.h>
#include <part.h>
#include <stdlib.h>

bool debug_mode = true;

// Macro that checks debug_mode at runtime
#define DBG(fmt, ...) \
    do { \
        if (debug_mode) { \
            fprintf(stderr, fmt, ##__VA_ARGS__); \
        } \
    } while (0)

static int wipe_partition_table(struct blk_desc *dev_desc){
	unsigned long blocks = 34;
	void *zero_buf;
	int ret = 0;

	DBG("%s", "Wiping partition table...\n");

	zero_buf = (void *)malloc(blocks * dev_desc->blksz);
	if (!zero_buf)
		return -ENOMEM;

	// Zero it out (since calloc doesn't work properly)
	memset(zero_buf, 0, blocks * dev_desc->blksz);

	ret = blk_dwrite(dev_desc, 0, blocks, zero_buf);
	free(zero_buf);

	if (ret != blocks) {
		DBG("%s", "Error: Failed to wipe partition table\n");
		return -EIO;
	}

	DBG("%s", "Partition table wiped\n");
	return 0;
}

static int wipe_bootloader(struct blk_desc *dev_desc, unsigned long size_mb){
	unsigned long blocks = 1024 * 1024 * size_mb / dev_desc->blksz;
	unsigned long chunck = 2048/*blocks*/;
	void *zero_buf;
	int ret = 0;
	int written = 0;
	int to_write = 0;

	DBG("%s%lu%s", "Wiping bootloader area (", size_mb, " MB)...\n");

	zero_buf = (void *)malloc(blocks * dev_desc->blksz);
	if (!zero_buf)
		return -ENOMEM;

	// Zero it out (since calloc doesn't work properly)
	memset(zero_buf, 0, blocks * dev_desc->blksz);

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

	DBG("%s", "Bootloader wiped\n");
}

static int wipe_partition_headers(struct blk_desc *dev_desc){
	struct disk_partition info;
	unsigned long header_bytes = 2 * 1024 * 1024;
	unsigned long header_blocks = header_bytes / dev_desc->blksz;
	void * zero_buf;
	int ret = 0;

	DBG("%s", "Wiping partition headers ...\n");

	zero_buf = (void *)malloc(header_bytes);
	if(!zero_buf){
		return -ENOMEM;
	}

	for (int part_num = 1; part_num <= 16; part_num++)
	{
		ret = part_get_info(dev_desc, part_num, &info);
		if (ret < 0){
			break;
		}

		DBG("  Part %d (%s) at LBA %lu\n", part_num, info.name, (unsigned long)info.start);

		unsigned long to_write = 0;
		if(header_blocks < info.size){
			to_write = header_blocks;
		}
		else{
			to_write = info.size;
		}

		ret = blk_dwrite(dev_desc, info.start, to_write, zero_buf);
		if (ret != to_write) {
			free(zero_buf);
			return -EIO;
		}

	}
	free(zero_buf);

	DBG("%s", "partition headers wiped.\n");
}

int secure_wipe_disk(int device, int level){
	struct blk_desc *dev_desc;
	int ret = -1;

	DBG("%s", "Program started\n");
	DBG("%s", "\n=================\n");
	DBG("%s", "WIPING DISK!\n");
	DBG("%s", "=================\n");

	dev_desc = blk_get_devnum_by_type(IF_TYPE_MMC, device);
	if (!dev_desc) {
		DBG("%s%d%s", "Error: Cannot get MMC device ", device, "\n");
		return -ENODEV;

	}
	DBG("%s%d\n", "Device: MMC ", device);
	DBG("%s%d\n\n", "Wipe level: ", level);

	switch(level){
		case(1):
			ret = wipe_partition_table(dev_desc);
			break;
		case(2):
			// ret = wipe_partition_table(dev_desc);
			// ret = wipe_bootloader(dev_desc, 8);
			ret = wipe_partition_headers(dev_desc);
			break;
		case(3):
			ret = wipe_partition_table(dev_desc);
			// ret = wipe_bootloader(dev_desc, 100);
			ret = wipe_partition_headers(dev_desc);
			break;
		default:
			DBG("%s", "Invalid Wipe level!\n");
			break;
	}

	if (ret == 0)
		DBG("%s", "\n=== WIPE COMPLETED ===\n\n");
	else
		DBG("%s", "\n=== WIPE FAILED ===\n\n");

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