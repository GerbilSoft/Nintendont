// Nintendont: FatFS devoptab for libogc.
// Based on libcustomfat's devoptab.
#include <sys/iosupport.h>
#include "ff_utf8.h"

// C includes.
#define __LINUX_ERRNO_EXTENSIONS__
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Devices.
#include <sdcard/wiisd_io.h>

static const DISC_INTERFACE* get_io_wiisd (void) {
	return &__io_wiisd;
}

extern DISC_INTERFACE __io_custom_usbstorage;
static const DISC_INTERFACE* get_io_usbstorage (void) {
	return &__io_custom_usbstorage;
}

// Disc interfaces.
typedef struct {
        const char* name; 
        const DISC_INTERFACE* (*getInterface)(void);
} INTERFACE_ID;
const INTERFACE_ID _FF_disc_interfaces[] = {
	{"sd", get_io_wiisd},
	{"usb", get_io_usbstorage},
	{NULL, NULL}
};	

static int FRESULT_to_errno(FRESULT res)
{
	switch (res) {
		case FR_OK:
			return 0;
		case FR_DISK_ERR:
		case FR_INT_ERR:
			return EIO;
		case FR_NOT_READY:
			return ENOMEDIUM;	// TODO
		case FR_NO_FILE:
		case FR_NO_PATH:
			return ENOENT;
		case FR_INVALID_NAME:
			return EINVAL;
		case FR_DENIED:
			return EACCES;
		case FR_EXIST:
			return EEXIST;
		case FR_INVALID_OBJECT:
			return EINVAL;	// TODO
		case FR_WRITE_PROTECTED:
			return EROFS;
		case FR_INVALID_DRIVE:
			return EINVAL;
		case FR_NOT_ENABLED:
			return ENOMEM;	// TODO
		case FR_MKFS_ABORTED:
			return EIO;
		case FR_TIMEOUT:
			return EAGAIN;
		case FR_LOCKED:
			return ENOLCK;
		case FR_NOT_ENOUGH_CORE:
			return ENOMEM;
		case FR_TOO_MANY_OPEN_FILES:
			return EMFILE;
		case FR_INVALID_PARAMETER:
			return EINVAL;
		default:
			return EIO;
	}
}

static int _FF_open_r(struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
	FIL *fp = (FIL*)fileStruct;

	// Convert the mode to FATFS.
	BYTE ff_mode = 0;
	switch (mode & 3) {
		case O_RDONLY:
			ff_mode = FA_READ;
			break;
		case O_WRONLY:
			ff_mode = FA_WRITE;
			break;
		case O_RDWR:
			ff_mode = FA_READ | FA_WRITE;
			break;
		default:
			// Invalid mode.
			r->_errno = EACCES;
			return -1;
	}

	// Check for create/truncate.
	if (mode & (O_CREAT|O_TRUNC)) {
		// Create/truncate file.
		if (mode & O_EXCL) {
			// Only create if it doesn't exist.
			ff_mode |= FA_CREATE_NEW;
		} else {
			// Create regardless.
			ff_mode |= FA_CREATE_ALWAYS;
		}
	} else {
		// Open file.
		if (!(mode & O_EXCL)) {
			// Open even if it doesn't exist.
			ff_mode |= FA_OPEN_ALWAYS;
		}
		if (mode & O_APPEND) {
			// Append to an existing file.
			ff_mode |= FA_OPEN_APPEND;
		} else {
			// Open an existing file.
			ff_mode |= FA_OPEN_EXISTING;
		}
	}

	FRESULT res = f_open_char(fp, path, ff_mode);
	if (res != FR_OK) {
		// FIXME: If file exists but it's a directory,
		// return EISDIR.
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}

	// File opened successfully.
	return (int)fp;
}

static int _FF_close_r(struct _reent *r, int fd)
{
	FIL *fp = (FIL*)fd;
	FRESULT res = f_close(fp);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}
	return 0;
}

static ssize_t _FF_write_r(struct _reent *r, int fd, const char *ptr, size_t len)
{
#if _FS_READONLY == 0
	FIL *fp = (FIL*)fd;
	UINT written;
	FRESULT res = f_write(fp, ptr, len, &written);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
	}
	return written;
#else
	((void)fd);
	((void)ptr);
	((void)len);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static ssize_t _FF_read_r(struct _reent *r, int fd, char *ptr, size_t len)
{
	FIL *fp = (FIL*)fd;
	UINT read;
	FRESULT res = f_write(fp, ptr, len, &read);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
	}
	return read;
}

static off_t _FF_seek_r(struct _reent *r, int fd, off_t pos, int dir)
{
#if _FS_MINIMIZE < 3
	FIL *fp = (FIL*)fd;
	FSIZE_t fp_pos;
	switch (dir) {
		case SEEK_SET:
			// FIXME: Support for >4GB?
			fp_pos = pos;
			break;
		case SEEK_CUR:
			fp_pos = f_tell(fp) + pos;
			break;
		case SEEK_END:
			fp_pos = f_size(fp) + pos;
			break;
		default:
			r->_errno = EINVAL;
			return -1;
	}

	FRESULT res = f_lseek(fp, fp_pos);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
	}
	return (off_t)fp_pos;
#else
	((void)fd);
	((void)pos);
	((void)dir);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static int _FF_fstat_r(struct _reent *r, int fd, struct stat *st)
{
	// TODO
	r->_errno = ENOTSUP;
	return -1;
}

static int _FF_stat_r(struct _reent *r, const char *path, struct stat *st)
{
	// TODO
	r->_errno = ENOTSUP;
	return -1;
}

static int _FF_link_r(struct _reent *r, const char *existing, const char *newLink)
{
	// FAT does not support hardlinks.
	((void)existing);
	((void)newLink);
	r->_errno = ENOTSUP;
	return -1;
}

static int _FF_unlink_r(struct _reent *r, const char *path)
{
#if _FS_READONLY == 0 && FS_MINIMIZE < 1
	FRESULT res = f_unlink_char(path);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}
	return 0;
#else
	((void)path)
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static int _FF_chdir_r(struct _reent *r, const char *path)
{
#if _FS_RPATH >= 1
	// TODO: Do we need to f_chdrive()?
	FRESULT res = f_chdir_char(path);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}
	return 0;
#else
	((void)path);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static int _FF_rename_r(struct _reent *r, const char *oldName, const char *newName)
{
#if _FS_READONLY == 0 && FS_MINIMIZE < 1
	FRESULT res = f_rename_char(oldName, newName);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}
	return 0;
#else
	((void)oldName);
	((void)newName);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static int _FF_mkdir_r(struct _reent *r, const char *path, int mode)
{
	((void)mode);	// No file permissions on FAT.
#if _FS_READONLY == 0 && FS_MINIMIZE < 1
	FRESULT res = f_mkdir_char(path);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}
	return 0;
#else
	((void)path);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static DIR_ITER* _FF_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path)
{
#if _FS_MINIMIZE < 2
	DIR *dp = (DIR*)dirState;
	FRESULT res = f_opendir_char(dp, path);
	if (res != FR_OK) {
		// FIXME: If file exists but isn't a directory,
		// return ENOTDIR.
		r->_errno = FRESULT_to_errno(res);
		return NULL;
	}
	return (DIR_ITER*)dp;
#else
	((void)dirState);
	((void)path);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static int _FF_dirreset_r(struct _reent *r, DIR_ITER *dirState)
{
#if _FS_MINIMIZE < 2
	// FIXME: FatFS doesn't have an f_rewinddir() function.
	((void)dirState);
	r->_errno = ENOTSUP;
	return -1;
#else
	((void)dirState);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static int _FF_dirnext_r(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
#if _FS_MINIMIZE < 2
	DIR *dp = (DIR*)dirState;
	FILINFO fno;
	FRESULT res = f_readdir(dp, &fno);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}

	// Convert the filename to UTF-8.
	const char *filename_utf8 = wchar_to_char(fno.fname);
	strncpy(filename, filename_utf8, MAXPATHLEN);

	// TODO: Convert FILINFO to struct stat.
	memset(filestat, 0, sizeof(*filestat));

	return 0;
#else
	((void)dirState);
	((void)filename);
	((void)filestat);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static int _FF_dirclose_r(struct _reent *r, DIR_ITER *dirState)
{
#if _FS_MINIMIZE < 2
	DIR *dp = (DIR*)dirState;
	FRESULT res = f_closedir(dp);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}
	return 0;
#else
	((void)dirState);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static int _FF_statvfs_r(struct _reent *r, const char *path, struct statvfs *buf)
{
	// TODO
	((void)path);
	((void)buf);
	r->_errno = ENOTSUP;
	return -1;
}

static int _FF_ftruncate_r(struct _reent *r, int fd, off_t len)
{
#if _FS_READONLY == 0 && FS_MINIMIZE < 1
	// FIXME: FatFS's f_truncate() only supports truncating to 0.
	if (len != 0) {
		r->_errno = EINVAL;
		return -1;
	}
	FIL *fp = (FIL*)fd;
	FRESULT res = f_truncate(fp);
	if (res != 0) {
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}
	return 0;
#else
	((void)fd);
	((void)len);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static int _FF_fsync_r(struct _reent *r, int fd)
{
#if _FS_READONLY == 0
	FIL *fp = (FIL*)fd;
	FRESULT res = f_sync(fp);
	if (res != FR_OK) {
		r->_errno = FRESULT_to_errno(res);
		return -1;
	}
	return 0;
#else
	((void)fd);
	r->_errno = ENOTSUP;
	return -1;
#endif
}

static const devoptab_t dotab_FF = {
	"fat",
	sizeof(FIL),
	_FF_open_r,
	_FF_close_r,
	_FF_write_r,
	_FF_read_r,
	_FF_seek_r,
	_FF_fstat_r,
	_FF_stat_r,
	_FF_link_r,
	_FF_unlink_r,
	_FF_chdir_r,
	_FF_rename_r,
	_FF_mkdir_r,
	sizeof(DIR),
	_FF_diropen_r,
	_FF_dirreset_r,
	_FF_dirnext_r,
	_FF_dirclose_r,
	_FF_statvfs_r,
	_FF_ftruncate_r,
	_FF_fsync_r,
	NULL,	/* Device data */
	NULL,	// chmod_r
	NULL,	// fchmod_r
	NULL	// rmdir_r
};
