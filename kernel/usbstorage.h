#ifndef __USBSTORAGE_H__
#define __USBSTORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define	USBSTORAGE_OK			0
#define	USBSTORAGE_ENOINTERFACE		-10000
#define	USBSTORAGE_ESENSE		-10001
#define	USBSTORAGE_ESHORTWRITE		-10002
#define	USBSTORAGE_ESHORTREAD		-10003
#define	USBSTORAGE_ESIGNATURE		-10004
#define	USBSTORAGE_ETAG			-10005
#define	USBSTORAGE_ESTATUS		-10006
#define	USBSTORAGE_EDATARESIDUE		-10007
#define	USBSTORAGE_ETIMEDOUT		-10008
#define	USBSTORAGE_EINIT		-10009
#define USBSTORAGE_PROCESSING	-10010

#define B_RAW_DEVICE_DATA_IN 0x01
#define B_RAW_DEVICE_COMMAND 0

typedef struct {
   uint8_t         command[16];
   uint8_t         command_length;
   uint8_t         flags;
   uint8_t         scsi_status;
   void*           data;
   size_t          data_length;
} raw_device_command;

bool USBStorage_Startup(void);
bool USBStorage_ReadSectors(u32 sector, u32 numSectors, void *buffer);
bool USBStorage_WriteSectors(u32 sector, u32 numSectors, const void *buffer);
void USBStorage_Shutdown(void);

/**
 * Get USB mass storage sector information.
 * @param s_size [out] Sector size.
 * @param s_cnt  [out] Sector count.
 * @return 0 on success; non-zero on error.
 */
int USBStorage_GetSectorInfo(u32 *s_size, u32 *s_cnt);

#ifdef __cplusplus
}
#endif

#endif /* __USBSTORAGE_H__ */
