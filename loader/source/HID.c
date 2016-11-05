// HID controller configuration loader.
#include <gccore.h>
#include "exi.h"
#include "HID.h"

#define HID_STATUS 0xD3003440
#define HID_CHANGE (HID_STATUS+4)
#define HID_CFG_SIZE (HID_STATUS+8)

#define HID_CFG_FILE 0x93003460

void HIDUpdateRegisters()
{
	if(*(vu32*)HID_CHANGE == 0)
		return;
	u32 DeviceVID = *(vu32*)HID_CHANGE;
	u32 DevicePID = *(vu32*)HID_CFG_SIZE;
	gprintf("Trying to get VID%04x PID%04x\n", DeviceVID, DevicePID);

	/* I hope this covers all possible ini files */
	char file_sd[40];
	char file_usb[40];
	snprintf(file_sd, sizeof(file_sd), "sd:/controllers/%04X_%04X.ini", DeviceVID, DevicePID);
	snprintf(file_usb, sizeof(file_usb), "usb:/controllers/%04X_%04X.ini", DeviceVID, DevicePID);

	const char *const filenames[6] =
	{
		file_sd, file_usb,
		"sd:/controller.ini",
		"sd:/controller.ini.ini",
		"usb:/controller.ini",
		"usb:/controller.ini.ini"
	};

	int i;
	FILE *f = NULL;
	for (i = 0; i < 6; i++)
	{
		f = fopen(filenames[i], "rb");
		if (f != NULL)
			break;
	}

	if (f != NULL)
	{
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);
		fread((void*)HID_CFG_FILE, 1, fsize, f);
		DCFlushRange((void*)HID_CFG_FILE, fsize);
		fclose(f);
		*(vu32*)HID_CFG_SIZE = fsize;
	}
	else
	{
		// No controller configuration file.
		*(vu32*)HID_CFG_SIZE = 0;
	}

	*(vu32*)HID_CHANGE = 0;
}
