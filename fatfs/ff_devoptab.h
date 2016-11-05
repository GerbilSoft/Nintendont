// Nintendont: FatFS devoptab for libogc.
// Based on libcustomfat's devoptab.
#ifndef _FATFS_DEVOPTAB
#define _FATFS_DEVOPTAB

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize and mount all devices
 */
void ffInit(void);

/**
 * Unmount and shut down a device.
 * @param name Device name.
 */
void ffUnmount(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* _FATFS_DEVOPTAB */
