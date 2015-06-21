#include "types.h"
#include "x86.h"
#include "file.h"
#include "stdlock.h"
#include "devman.h"
#include "fsman.h"
#include "tty.h"
#include "stdarg.h"
#include "stdlib.h"
#include "ata.h"
#include "serial.h"
#include "panic.h"
#include "pipe.h"
#include "proc.h"
#include "vm.h"
#include "console.h"
#include "keyboard.h"
#include "pic.h"
#include "ramfs.h"

extern pgdir* k_pgdir;
extern uint video_mode;
slock_t driver_table_lock;
static struct DeviceDriver drivers[MAX_DEVICES];
extern uchar serial_connected;
uint curr_mapping;

static struct DeviceDriver* dev_alloc(void)
{
	slock_acquire(&driver_table_lock);
	struct DeviceDriver* driver = NULL;
	int x;
	for(x = 1;x < MAX_DEVICES;x++)
		if(drivers[x].valid == 0)
		{
			driver = drivers + x;
			driver->valid = 1;
			break;
		}
	slock_release(&driver_table_lock);
	return driver;
}

void* dev_new_mapping(uint phy, uint sz)
{
	uint v_start = curr_mapping;
	uint v_end = PGROUNDUP(v_start + sz);
	if(v_end > KVM_HARDWARE_E)
		panic("devman: Out of hardware memory.\n");
	uint x;
	for(x = 0;x < sz;x += PGSIZE)
		mappage(phy + x, v_start + x, k_pgdir, 0, PGTBL_WTWB);

	curr_mapping = v_end;
	return (void*)v_start;
}

int dev_init()
{
	curr_mapping = KVM_HARDWARE_S;
	slock_init(&driver_table_lock);
	struct DeviceDriver* driver = NULL;

	memset(&drivers, 0, sizeof(struct DeviceDriver) * MAX_DEVICES);

	/* Setup video memory */
	uint video_mem = 0x0;
	uchar video_type = 0;
	if(video_mode == VIDEO_MODE_COLOR)
	{
		video_type = TTY_TYPE_COLOR;
		video_mem = CONSOLE_COLOR_BASE_ORIG;
	}else if(video_mode == VIDEO_MODE_MONO)
	{
		video_type = TTY_TYPE_MONO;
		video_mem = CONSOLE_MONO_BASE_ORIG;
	} else {
		video_type = 0x0;
		video_mem = 0x0;
	}

	if(video_mem)
		video_mem = (uint)dev_new_mapping(video_mem, 
				CONSOLE_MEM_SZ);

	/* Find ttys */
	uint x;
	for(x = 0;;x++)
	{
		if(x > 0 && !video_type) break;
		tty_t t = tty_find(x);
		if(t == NULL) break;
		driver = dev_alloc();
		if(driver == NULL)
			panic("Out of free device drivers.\n");
		/* Valid tty */
		if(x != 0)
			tty_init(t, x, video_type, 1, video_mem);
		tty_io_setup(&driver->io_driver, x);
		/* Set mount point */
		snprintf(driver->node, 
			FILE_MAX_PATH, "/dev/tty%d", x);
	}

	/* Fix tty0 */
	tty_t t0 = tty_find(0);
	if(t0->type == TTY_TYPE_COLOR || t0->type == TTY_TYPE_MONO)
		t0->mem_start = video_mem;
	/* Boot strap has directly mapped the video memory */
	unmappage(CONSOLE_MONO_BASE_ORIG, k_pgdir);
	unmappage(CONSOLE_COLOR_BASE_ORIG, k_pgdir);

	/* Find ata devices */
	ata_init();

	for(x = 0;x < ATA_DRIVER_COUNT;x++)
	{
		if(ata_drivers[x]->valid != 1) continue;
		/* Valid ata driver */
		driver = dev_alloc();
		ata_io_setup(&driver->io_driver, ata_drivers[x]);
		snprintf(driver->node,
			FILE_MAX_PATH, "/dev/hd%c", 'a' + x);
	}

	/* Bring up ramfs */
	ramfs_init();

	serial_init(0);
	/* Find serial port */
	if(serial_connected)
	{
		driver = dev_alloc();
		serial_io_setup(&driver->io_driver);
		/* Add serial to mask */
		pic_enable(INT_PIC_COM1_CODE);
		/* Set mount point */
                snprintf(driver->node,
                        FILE_MAX_PATH, "/dev/sl0", x);
	}

	/* Keyboard */
	kbd_init();

	/* Do final init on all io devices */
	for(x = 0;x < MAX_DEVICES;x++)
		if(drivers[x].valid)
			drivers[x].io_driver.init(&drivers[x].io_driver);
	return 0;
}

void dev_populate(void)
{
	int dev_perm = PERM_UAP | PERM_GAP;
	int x;
	for(x = 0;x < MAX_DEVICES;x++)
	{
		if(drivers[x].valid)
		{
			fs_mknod(drivers[x].node, x, drivers[x].type, dev_perm);
		}
	}
}

struct DeviceDriver* dev_lookup(int dev)
{
	if(dev > 0 && dev < MAX_DEVICES && drivers[dev].valid)
		return drivers + dev;
	else return NULL;
}
