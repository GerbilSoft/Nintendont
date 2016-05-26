/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"
#include "string.h"
#include "debug.h"
#include "Config.h"

#include "SDI.h"
#include "usbstorage.h"
#include "alloc.h"
#include "common.h"

extern bool access_led;

// Device information.
FF_dev_info_t FF_dev_info[_VOLUMES] = {{0}};


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv < FF_DEV_SD || pdrv > FF_DEV_USB)
		return RES_PARERR;

	return (FF_dev_info[pdrv].s_size != 0 ? RES_OK : RES_NOTRDY);
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	if (pdrv < FF_DEV_SD || pdrv > FF_DEV_USB)
		return RES_PARERR;

	// If the drive is already initialized, don't attempt
	// to initialize it again. USBStorage ignores a
	// double init, but SDI does not.
	if (FF_dev_info[pdrv].s_size != 0)
		return RES_OK;

	int ret = RES_ERROR;
	switch (pdrv) {
		case FF_DEV_SD:
			if (SDHCInit()) {
				// SD card always uses 512-byte sectors.
				// TODO: Get s_cnt.
				FF_dev_info[pdrv].s_size = PAGE_SIZE512;
				ret = RES_OK;
			} else {
				// Error initializing SD.
				FF_dev_info[pdrv].s_size = 0;
				FF_dev_info[pdrv].s_cnt = 0;
			}
			break;

		case FF_DEV_USB:
			if (USBStorage_Startup()) {
				// USBStorage initializes FF_dev_info[FF_DEV_USB].
				ret = RES_OK;
			} else {
				// Error initializing USB.
				FF_dev_info[pdrv].s_size = 0;
				FF_dev_info[pdrv].s_cnt = 0;
			}
			dbgprintf("USB:Drive size: %dMB SectorSize:%d\r\n",
				  FF_dev_info[pdrv].s_cnt / 1024 *
				  FF_dev_info[pdrv].s_size / 1024,
				  FF_dev_info[pdrv].s_size);
			break;

		default:
			ret = RES_PARERR;
			break;
	}

	return ret;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	int ret = RES_OK;

	// Turn on the drive LED.
	if (access_led) set32(HW_GPIO_OUT, GPIO_SLOT_LED);

	switch (pdrv) {
		case FF_DEV_SD: {
			int retry;
			for (retry = 10; retry >= 0; retry--) {
				if (sdio_ReadSectors(sector, count, buff))
					break;
			}
			if (retry < 0) {
				// Too many attempts.
				ret = RES_ERROR;
			}
			break;
		}

		case FF_DEV_USB:
			if (!USBStorage_ReadSectors(sector, count, buff)) {
				dbgprintf("USB:Failed to read from USB device... Sector: %d Count: %d dst: %p\r\n", sector, count, buff);
				ret = RES_ERROR;
			}
			break;

		default:
			// Invalid device number.
			ret = RES_PARERR;
			break;
	}

	// Turn off the drive LED.
	if (access_led) clear32(HW_GPIO_OUT, GPIO_SLOT_LED);
	return ret;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
	// TODO: Default to RES_ERROR?
	int ret = RES_OK;

	// Turn on the drive LED.
	if (access_led) set32(HW_GPIO_OUT, GPIO_SLOT_LED);

	// TODO: Check for write protection and return RES_WRPRT?
	switch (pdrv) {
		case FF_DEV_SD:
			if (sdio_WriteSectors(sector, count, buff) < 0) {
				ret = RES_ERROR;
			}
			break;

		case FF_DEV_USB:
			if (USBStorage_WriteSectors(sector, count, buff) != 1) {
				dbgprintf("USB: Failed to write to USB device... Sector: %d Count: %d dst: %p\r\n", sector, count, buff);
				ret = RES_ERROR;
			}
			break;

		default:
			// Invalid device number.
			ret = RES_PARERR;
			break;
	}

	// Turn off the drive LED.
	if (access_led) clear32(HW_GPIO_OUT, GPIO_SLOT_LED);
	return ret;
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	// TODO: Implement more commands here, and use those commands
	// in kernel and/or loader via FatFS?

	int ret = RES_PARERR;
	if (pdrv < FF_DEV_SD || pdrv > FF_DEV_USB)
		return ret;

	switch (cmd) {
		case GET_SECTOR_SIZE:
			*(WORD*)buff = FF_dev_info[pdrv].s_size;
			ret = RES_OK;
			break;

		/* FIXME: Not implemented; need to get SD card CSD. */
#if 0
		case GET_SECTOR_COUNT:
			*(DWORD*)buff = FF_dev_info[pdrv].s_cnt;
			ret = RES_OK;
			break;
#endif

		default:
			break;
	}

	return ret;
}
