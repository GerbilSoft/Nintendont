// Nintendont (kernel): GameCube Memory Card functions.
// Used by EXI.c.

#include "GCNCard.h"
#include "Config.h"
#include "debug.h"
#include "ff_utf8.h"

// GCN card structures.
#include "GCNCard_Struct.h"

// Triforce variables.
extern vu32 TRIGame;

// Memory Card context.
static u8 *const GCNCard_base = (u8*)(0x11000000);

// Use GCI Folders instead of card images?
// TODO: Make this a configurable option.
static const bool UseGCIFolders = true;

typedef struct _GCNCard_ctx {
	char filename[0x20];    // Memory Card filename, or directory for GCI folders.
	u8 *base;               // Base address.
	u32 size;               // Size, in bytes.
	u32 code;               // Memory card "code".

	// BlockOffLow starts from 0xA000; does not include "system" blocks.
	// For system blocks, check 'changed_system'.
	bool changed;		// True if the card has been modified at all.
				// (NOTE: Reset after calling GCNCard_CheckChanges().)
	bool changed_system;	// True if the system area (first 5 blocks)
				// has been modified. These blocks are NOT
				// included in BlockOffLow / BlockOffHigh.

	// NOTE: BlockOff is in bytes, not blocks.
	u32 BlockOff;           // Current offset.
	u32 BlockOffLow;        // Low address of last modification.
	u32 BlockOffHigh;       // High address of last modification.
	u32 CARDWriteCount;     // Write count. (TODO: Is this used anywhere?)

	// Filenames for .gci files when using GCI Folders.
	// Array index corresponds to the index in the directory block.
	// Filename length is 32 (maximum length on GameCube) plus 7 for
	// ID6 and hyphen, and 1 extra.
	// FIXME: Only 4 files for now due to memory limitations.
	// TODO: Dynamically allocate?
	char gci_filenames[4][40];
} GCNCard_ctx;
#ifdef GCNCARD_ENABLE_SLOT_B
static GCNCard_ctx memCard[2] __attribute__((aligned(32)));
#else /* !GCNCARD_ENABLE_SLOT_B */
static GCNCard_ctx memCard[1] __attribute__((aligned(32)));
#endif /* GCNCARD_ENABLE_SLOT_B */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

static void GCNCard_InitCtx(GCNCard_ctx *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->BlockOffLow = 0xFFFFFFFF;
}

/**
 * Is a memory card enabled?
 * @param slot Slot number. (0 == Slot A, 1 == Slot B)
 * @return 0 if disabled; 1 if enabled.
 */
inline u32 GCNCard_IsEnabled(int slot)
{
	if (slot < 0 || slot > ARRAY_SIZE(memCard))
		return 0;

	// Card is enabled if it's larger than 0 bytes.
	return (memCard[slot].size > 0);
}

// TODO: Combine with SRAM_UpdateChecksum().
static void doChecksum(const u16 *buffer, u32 size, u16 *c1, u16 *c2)
{
	u32 i;
	*c1 = 0; *c2 = 0;
	for (i = size/sizeof(u16); i > 0; i--, buffer++)
	{
		*c1 += *buffer;
		*c2 += *buffer ^ 0xFFFF;
	}
	if (*c1 == 0xFFFF) *c1 = 0;
	if (*c2 == 0xFFFF) *c2 = 0;
}

/**
 * Format a GCN card image in memory.
 * @param ctx GCNCard_ctx to format.
 * @return 0 on success; non-zero on reror.
 */
static int GCNCard_Format(GCNCard_ctx *ctx)
{
	if (!ctx->base || ctx->size < 524288)
	{
		// Invalid base address and/or size.
		return -1;
	}

	// Get the Game ID.
	const u32 GameID = ConfigGetGameID();

	// Fill the header and directory blocks with 0xFF.
	memset32(ctx->base, 0xFFFFFFFF, 0x6000);
	// Clear the rest of the memory card area.
	memset32(ctx->base+0x6000, 0, ctx->size-0x6000);

	// Header block.
	card_header *header = (card_header*)ctx->base;
	// Normally this area contains a checksum which is built via SRAM and formatting time, you can
	// skip that though because if the formatting time is 0 the resulting checksum is also 0 ;)
	memset(header->reserved1, 0, sizeof(header->reserved1));
	header->formatTime = 0;		// TODO: Actually set this?
	// From SRAM as defined in PatchCodes.h
	header->sramBias = 0x17CA2A85;
	// Use current language for SRAM language
	header->sramLang = ncfg->Language;
	// Memory Card File Mode
	header->reserved2 = ((GameID & 0xFF) == 'J' ? 2 : 0);
	// Assuming slot A.
	header->device_id = 0;
	// Memory Card size in MBits total
	header->size = (ctx->size >> 17);
	// Memory Card filename encoding
	header->encoding = ((GameID & 0xFF) == 'J');
	// Generate Header Checksum
	doChecksum((u16*)header, 0x1FC, &header->chksum1, &header->chksum2);

	// Set Dir Update Counters
	card_dircntrl *dircntrl1 = (card_dircntrl*)&ctx->base[0x3FC0];
	card_dircntrl *dircntrl2 = (card_dircntrl*)&ctx->base[0x5FC0];
	dircntrl1->updated = 0;
	dircntrl2->updated = 1;
	// Generate Dir Checksums
	doChecksum((u16*)&ctx->base[0x2000], 0x1FFC, &dircntrl1->chksum1, &dircntrl1->chksum2);
	doChecksum((u16*)&ctx->base[0x4000], 0x1FFC, &dircntrl2->chksum1, &dircntrl2->chksum2);

	// Set Block Update Counters
	card_bat *bat = (card_bat*)&ctx->base[0x6000];
	bat[0].updated = 0;
	bat[1].updated = 1;
	// Set Free Blocks
	bat[0].freeblocks = (ctx->size / 8192) - 5;
	bat[1].freeblocks = (ctx->size / 8192) - 5;
	// Set last allocated Block
	bat[0].lastalloc = 4;
	bat[1].lastalloc = 4;
	// Generate Block Checksums
	doChecksum((u16*)&ctx->base[0x6004], 0x1FFC, &bat[0].chksum1, &bat[0].chksum2);
	doChecksum((u16*)&ctx->base[0x8004], 0x1FFC, &bat[1].chksum1, &bat[1].chksum2);

	// Memory card image formatted successfully.
	// NOTE: Caller must call sync_after_write().
	return 0;
}

/**
 * Get the current block table.
 * @param ctx Memory Card context.
 * @return Current block table.
 */
static inline card_bat *GCNCard_GetCurrentBAT(GCNCard_ctx *const ctx)
{
	card_bat *bat = (card_bat*)&ctx->base[CARD_SYSBAT];
	return (bat[1].updated > bat[0].updated ? &bat[1] : &bat[0]);
}

/**
 * Get the current directory table.
 * @param ctx Memory Card context.
 * @return Current directory table.
 */
static inline card_dat *GCNCard_GetCurrentDAT(GCNCard_ctx *const ctx)
{
	card_dat *dat = (card_dat*)&ctx->base[CARD_SYSDIR];
	return (dat[1].dircntrl.updated > dat[0].dircntrl.updated ? &dat[1] : &dat[0]);
}

/**
 * Get the "old" block table.
 * @param ctx Memory Card context.
 * @return Old block table.
 */
static inline card_bat *GCNCard_GetOldBAT(GCNCard_ctx *const ctx)
{
	card_bat *bat = (card_bat*)&ctx->base[CARD_SYSBAT];
	return (bat[1].updated > bat[0].updated ? &bat[0] : &bat[1]);
}

/**
 * Get the "old" directory table.
 * @param ctx Memory Card context.
 * @return Old directory table.
 */
static inline card_dat *GCNCard_GetOldDAT(GCNCard_ctx *const ctx)
{
	card_dat *dat = (card_dat*)&ctx->base[CARD_SYSDIR];
	return (dat[1].dircntrl.updated > dat[0].dircntrl.updated ? &dat[0] : &dat[1]);
}

/**
 * Allocate a block for a GCI file.
 * @param ctx Memory Card context.
 * @return Block number, or 0 if a block cannot be allocated.
 */
static u16 GCNCard_GCI_AllocBlock(GCNCard_ctx *const ctx)
{
	// Get the current block table.
	card_bat *cur_bat = GCNCard_GetCurrentBAT(ctx);

	// Find an available block.
	const u16 totalBlocks = (ctx->size / 8192);
	u16 block;
	for (block = cur_bat->lastalloc+1; block < totalBlocks; block++)
	{
		if (cur_bat->fat[block-5] == 0)
		{
			// Found a block.
			cur_bat->fat[block-5] = 0xFFFF;
			cur_bat->freeblocks--;
			cur_bat->lastalloc = block;
			return (u16)block;
		}
	}

	// Couldn't find an available block.
	// Start again from the beginning of the card.
	for (block = 5; block <= cur_bat->lastalloc; block++)
	{
		if (cur_bat->fat[block-5] == 0)
		{
			// Found a block.
			cur_bat->fat[block-5] = 0xFFFF;
			cur_bat->freeblocks--;
			cur_bat->lastalloc = block;
			return (u16)block;
		}
	}

	// No block available.
	return 0;
}

/**
 * Load a GCI file.
 * NOTE: Must be chdir'd to the game-specific save directory.
 * @param ctx Memory Card context.
 * @param filename GCI filename from the game-specific save directory.
 * @param idx File index. (must be 0 to 127)
 * @return 0 on success; non-zero on error.
 */
static int GCNCard_LoadGCIFile(GCNCard_ctx *const ctx, const char *filename, int idx)
{
	// Open the GCI file.
	FIL fd;
	int ret = f_open_char(&fd, filename, FA_READ|FA_OPEN_EXISTING);
	if (ret != FR_OK)
		return -1;

	// Check the filesize.
	// Must be a multiple of 8192, plus 64.
	if (fd.obj.objsize <= 64 || ((fd.obj.objsize - 64) & (8192-1)) != 0)
		return -2;

	// Get the current block table.
	card_bat *cur_bat = GCNCard_GetCurrentBAT(ctx);

	// Check if we have enough free blocks.
	u16 blocks = (fd.obj.objsize - 64) / 8192;
	if (cur_bat->freeblocks < blocks)
	{
		// Not enough free blocks.
		return -3;
	}

	// Make sure the specified directory entry is empty.
	card_dat *cur_dat = GCNCard_GetCurrentDAT(ctx);
	card_direntry *entry = &cur_dat->entries[idx];
	static const u8 gamecode_empty[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	if (memcmp(entry->gamecode, gamecode_empty, sizeof(gamecode_empty)) != 0)
	{
		// Directory entry isn't empty.
		return -4;
	}

	// Read the GCI header into the directory entry.
	UINT read;
	f_lseek(&fd, 0);
	f_read(&fd, entry, sizeof(*entry), &read);
	if (read != 64)
		Shutdown();

	// Make sure the file size is correct
	entry->length = blocks;

	// Read in the rest of the file, one block at a time.
	int i;
	u16 cur_block = 0, new_block;
	for (i = 0; i < blocks; i++)
	{
		// Allocate a block.
		new_block = GCNCard_GCI_AllocBlock(ctx);
		if (new_block == 0)
			Shutdown();

		if (cur_block == 0)
		{
			// First block. Update the directory entry.
			entry->block = new_block;
		}
		else
		{
			// Not the first block. Update the chain.
			cur_bat->fat[cur_block-5] = new_block;
		}

		cur_block = new_block;

		// Load this block from the file.
		f_read(&fd, &ctx->base[8192 * cur_block], 8192, &read);
		if (read != 8192)
			Shutdown();
	}

	// Save the filename for later.
	// (Remove the .gci extension.)
	int len = strlen(filename) - 4;
	if (len <= 0 || len > 39)
		Shutdown();
	strncpy(ctx->gci_filenames[idx], filename, len+1);

	// GCI file loaded.
	return 0;
}

/**
 * Load a GCI folder.
 * @param slot Slot number. (0 == Slot A, 1 == Slot B)
 * @return 0 on success; non-zero on error.
 */
static int GCNCard_LoadGCIFolder(int slot)
{
	// Get the Game ID.
	const u32 GameID = ConfigGetGameID();

	// Set up the Memory Card context.
	// NOTE: Do NOT include a trailing slash.
	// f_mkdir() doesn't like that.
	GCNCard_ctx *const ctx = &memCard[slot];
	GCNCard_InitCtx(ctx);
	memcpy(ctx->filename, "/saves/", 7);
	memcpy(&ctx->filename[7], &GameID, 4);
	ctx->filename[11] = 0;

	if (slot != 0)
	{
		// Only Slot A is supported for GCI folders right now.
		Shutdown();
	}

	// Format the virtual image.
	ctx->base = GCNCard_base;
	ctx->size = ConfigGetMemcardSize();

	// Set the code correctly.
	// 0 = 59-block
	// 1 = 123-block
	// 2 = 251-block, etc.
	ctx->code = MEM_CARD_CODE(ctx->size >> 20);

	int ret = GCNCard_Format(ctx);
	if (ret != 0)
	{
		// Error formatting the memory card.
		Shutdown();
	}

	// Make sure the card directory exists.
	ret = f_mkdir_char("/saves");
	if (ret != FR_OK && ret != FR_EXIST)
		Shutdown();
	ret = f_mkdir_char(ctx->filename);
	if (ret != FR_OK && ret != FR_EXIST)
		Shutdown();

	// chdir to the game-specific save directory.
	ret = f_chdir_char(ctx->filename);
	if (ret != FR_OK)
		Shutdown();

	// Read the directory.
	DIR dp;
	ret = f_opendir_char(&dp, ctx->filename);
	if (ret != FR_OK)
		Shutdown();

	// Find files that end with ".gci".
	FILINFO fInfo;
	int idx = 0;	// 0 to 127; GCN filename index.
	while (idx < 4/*FIXME*/ && f_readdir(&dp, &fInfo) == FR_OK && fInfo.fname[0] != '\0')
	{
		// Skip subdirectories.
		if (fInfo.fattrib & AM_DIR)
			continue;

		// Skip "." and "..".
		// This will also skip "hidden" files.
		if (fInfo.fname[0] == '.')
			continue;

		// Convert the filename to UTF-8.
		const char *filename_utf8 = wchar_to_char(fInfo.fname);

		// Check if this filename is valid:
		// Must be at least 5 characters (".gci" plus name)
		// Must not be more than 39 characters + ".gci" extension.
		size_t len = strlen(filename_utf8);
		if (len > 5 && len < 39+4 &&
		    !strcmp(&filename_utf8[len-4], ".gci"))
		{
			// This is a .gci file.
			ret = GCNCard_LoadGCIFile(ctx, filename_utf8, idx);
			// TODO: Error messages?
			if (ret == 0)
				idx++;
		}
	}

	f_closedir(&dp);

	// Copy the current DAT to the old DAT,
	// and update checksums.
	card_dat *cur_dat = GCNCard_GetCurrentDAT(ctx);
	card_dat *old_dat = GCNCard_GetOldDAT(ctx);
	memcpy(old_dat, cur_dat, sizeof(*old_dat));
	cur_dat->dircntrl.updated++;
	doChecksum((u16*)cur_dat, 0x1FFC, &cur_dat->dircntrl.chksum1, &cur_dat->dircntrl.chksum2);
	doChecksum((u16*)old_dat, 0x1FFC, &old_dat->dircntrl.chksum1, &old_dat->dircntrl.chksum2);

	// Copy the current BAT to the old BAT,
	// and update checksums.
	card_bat *cur_bat = GCNCard_GetCurrentBAT(ctx);
	card_bat *old_bat = GCNCard_GetOldBAT(ctx);
	memcpy(old_bat, cur_bat, sizeof(*old_bat));
	cur_bat->updated++;
	doChecksum((u16*)cur_bat + 2, 0x1FFC, &cur_bat->chksum1, &cur_bat->chksum2);
	doChecksum((u16*)old_bat + 2, 0x1FFC, &old_bat->chksum1, &old_bat->chksum2);

	// chdir back to "/".
	static const WCHAR root_dir[2] = {'/', 0};
	ret = f_chdir(root_dir);
	if (ret != FR_OK)
		Shutdown();

	// Reset the low/high offsets to indicate that everything was just loaded.
	ctx->BlockOffLow = 0xFFFFFFFF;
	ctx->BlockOffHigh = 0x00000000;

	// Synchronize the memory card data.
	sync_after_write(ctx->base, ctx->size);

	// Dump to a test file.
	UINT wrote;
	FIL x;
	f_open_char(&x, "/TEST.raw", FA_WRITE|FA_CREATE_ALWAYS);
	f_write(&x, ctx->base, ctx->size, &wrote);
	f_close(&x);

	// GCI folder loaded.
	return 0;
}

/**
 * Load a RAW memory card image.
 * @param slot Slot number. (0 == Slot A, 1 == Slot B)
 * @return 0 on success; non-zero on error.
 */
static int GCNCard_LoadRAWImage(int slot)
{
	// Get the Game ID.
	const u32 GameID = ConfigGetGameID();

	// Make sure the "/saves/" directory exists.
	int ret = f_mkdir_char("/saves/");
	if (ret != FR_OK && ret != FR_EXIST)
		Shutdown();

	// Set up the Memory Card context.
	GCNCard_ctx *const ctx = &memCard[slot];
	GCNCard_InitCtx(ctx);
	memcpy(ctx->filename, "/saves/", 7);
	char *fname_ptr = &ctx->filename[7];
	if (ConfigGetConfig(NIN_CFG_MC_MULTI))
	{
		// "Multi" mode enabled. (one card for all saves, per region)
		memcpy(fname_ptr, "ninmem", 6);
		fname_ptr += 6;

		if ((GameID & 0xFF) == 'J')
		{
			// JPN game. Append a 'j'.
			*fname_ptr++ = 'j';
		}
#ifdef GCNCARD_ENABLE_SLOT_B
		if (slot)
		{
			// Slot B. Append a 'b'.
			*fname_ptr++ = 'b';
		}
#endif /* GCNCARD_ENABLE_SLOT_B */

		// Append the file extension.
		memcpy(fname_ptr, ".raw", 4);
	}
	else
	{
		// Single mode. One card per game.
		memcpy(fname_ptr, &GameID, 4);
		fname_ptr += 4;

#ifdef GCNCARD_ENABLE_SLOT_B
		if (slot)
		{
			// Slot B. Append "_B".
			*(fname_ptr+0) = '_';
			*(fname_ptr+1) = 'B';
			fname_ptr += 2;
		}
#endif /* GCNCARD_ENABLE_SLOT_B */

		// Append the file extension.
		memcpy(fname_ptr, ".raw", 4);
	}

	sync_after_write(ctx->filename, sizeof(ctx->filename));

	dbgprintf("EXI: Trying to open %s\r\n", ctx->filename);
	FIL fd;
	ret = f_open_char(&fd, ctx->filename, FA_READ|FA_OPEN_EXISTING);
	if (ret != FR_OK || fd.obj.objsize == 0)
	{
#ifdef DEBUG_EXI
		dbgprintf("EXI: Slot %c: Failed to open %s: %u\r\n", (slot+'A'), ctx->filename, ret );
#endif
		if (ret == FR_OK)
			f_close(&fd);

		if (slot == 0)
		{
			// Slot A failure is usually fatal.
			// Let's try creating the card image manually.
			ctx->base = GCNCard_base;
			ctx->size = ConfigGetMemcardSize();
			ctx->code = MEM_CARD_CODE(ctx->size >> 20);
			ret = GCNCard_Format(ctx);
			if (ret != 0)
			{
				// Error formatting the memory card.
				Shutdown();
			}

			// Attempt to write to the file.
			ret = f_open_char(&fd, ctx->filename, FA_WRITE|FA_CREATE_ALWAYS);
			if (ret != FR_OK)
			{
				// Cannot open file for writing.
				// NOW we'll abort.
				Shutdown();
			}

			// File opened. Let's write the virtual memory card image.
			UINT wrote;
			f_write(&fd, ctx->base, ctx->size, &wrote);
			f_close(&fd);

			// Make sure we wrote the entire image.
			if (wrote != ctx->size)
			{
				// Wrong size.
				// Delete the file, then abort.
				f_unlink_char(ctx->filename);
				Shutdown();
			}

			// Memory card image loaded successfully.
			ctx->BlockOffLow = 0xFFFFFFFF;
			ctx->BlockOffHigh = 0x00000000;
			sync_after_write(ctx->base, ctx->size);

		#ifdef DEBUG_EXI
			dbgprintf("EXI: Formatted and Saved Slot %c memory card size %u\r\n", (slot+'A'), ctx->size);
		#endif
			return 0;
		}

#ifdef GCNCARD_ENABLE_SLOT_B
		// Slot B failure will simply disable Slot B.
		dbgprintf("EXI: Slot %c has been disabled.\r\n", (slot+'A'));
		return -3;
#endif /* GCNCARD_ENABLE_SLOT_B */
	}

#ifdef DEBUG_EXI
	dbgprintf("EXI: Loading memory card for Slot %c...", (slot+'A'));
#endif

	// Check if the card filesize is valid.
	u32 FindBlocks = 0;
	for (FindBlocks = 0; FindBlocks <= MEM_CARD_MAX; FindBlocks++)
	{
		if (MEM_CARD_SIZE(FindBlocks) == fd.obj.objsize)
			break;
	}
	if (FindBlocks > MEM_CARD_MAX)
	{
		dbgprintf("EXI: Slot %c unexpected size %s: %u\r\n",
				(slot+'A'), ctx->filename, fd.obj.objsize);
#ifdef GCNCARD_ENABLE_SLOT_B
		if (slot == 0)
		{
			// Slot A failure is fatal.
			Shutdown();
		}

		// Slot B failure will simply disable Slot B.
		dbgprintf("EXI: Slot %c has been disabled.\r\n", (slot+'A'));
		f_close(&fd);
		return -4;
#else /* !GCNCARD_ENABLE_SLOT_B */
		// Slot A failure is fatal.
		Shutdown();
#endif /* GCNCARD_ENABLE_SLOT_B */
	}

#if GCNCARD_ENABLE_SLOT_B
	if (slot == 0)
	{
		// Slot A starts at GCNCard_base.
		ctx->base = GCNCard_base;
		// Set the memory card size for Slot A only.
		ConfigSetMemcardBlocks(FindBlocks);
	}
	else
	{
		// Slot B starts immediately after Slot A.
		// Make sure both cards fit within 16 MB.
		if (memCard[0].size + fd.obj.objsize > (16*1024*1024))
		{
			// Not enough memory for both cards.
			// Disable Slot B.
			dbgprintf("EXI: Slot A is %u MB; not enough space for Slot %c, which is %u MB.\r\n",
					"EXI: Slot %c has been disabled.\r\n",
					memCard[0].size / 1024 / 1024, (slot+'A'),
					fd.obj.objsize / 1024 / 1024, (slot+'A'));
			f_close(&fd);
			return -4;
		}
		ctx->base = memCard[0].base + memCard[0].size;
	}
#else /* !GCNCARD_ENABLE_SLOT_B */
	// Slot A starts at GCNCard_base.
	ctx->base = GCNCard_base;
	// Set the memory card size for Slot A only.
	ConfigSetMemcardBlocks(FindBlocks);
#endif /* GCNCARD_ENABLE_SLOT_B */

	// Size and "code".
	ctx->size = fd.obj.objsize;
	ctx->code = MEM_CARD_CODE(FindBlocks);

	// Read the memory card contents into RAM.
	UINT read;
	f_lseek(&fd, 0);
	f_read(&fd, ctx->base, ctx->size, &read);
	f_close(&fd);

	// Reset the low/high offsets to indicate that everything was just loaded.
	ctx->BlockOffLow = 0xFFFFFFFF;
	ctx->BlockOffHigh = 0x00000000;

	// Synchronize the memory card data.
	sync_after_write(ctx->base, ctx->size);

#ifdef GCNCARD_ENABLE_SLOT_B
	if (slot == 1)
	{
		// Slot B card image loaded successfully.
		ncfg->Config |= NIN_CFG_MC_SLOTB;
	}
#endif /* GCNCARD_ENABLE_SLOT_B */
}

/**
 * Load a memory card from disk.
 * @param slot Slot number. (0 == Slot A, 1 == Slot B)
 * NOTE: Slot B is not valid on Triforce.
 * @return 0 on success; non-zero on error.
 */
int GCNCard_Load(int slot)
{
	switch (slot)
	{
		case 0:
			// Slot A
			break;
#ifdef GCNCARD_ENABLE_SLOT_B
		case 1:
			// Slot B (not valid on Triforce)
			if (TRIGame != 0)
				return -1;
			break;
#endif /* GCNCARD_ENABLE_SLOT_B */
		default:
			// Invalid slot.
			return -2;
	}

	int ret;
	if (UseGCIFolders)
		ret = GCNCard_LoadGCIFolder(slot);
	else
		ret = GCNCard_LoadRAWImage(slot);

	if (ret != 0)
	{
		// Error loading the memory card.
		return -3;
	}

#ifdef DEBUG_EXI
	dbgprintf("EXI: Loaded Slot %c memory card size %u\r\n", (slot+'A'), ctx->size);
#endif
	return 0;
}

/**
* Get the total size of the loaded memory cards.
* @return Total size, in bytes.
*/
u32 GCNCard_GetTotalSize(void)
{
#ifdef GCNCARD_ENABLE_SLOT_B
	return (memCard[0].size + memCard[1].size);
#else /* !GCNCARD_ENABLE_SLOT_B */
	return memCard[0].size;
#endif /* GCNCARD_ENABLE_SLOT_B */
}

/**
* Check if the memory cards have changed.
* @return True if either memory card has changed; false if not.
*/
bool GCNCard_CheckChanges(void)
{
	int slot;
	bool ret = false;
	for (slot = 0; slot < ARRAY_SIZE(memCard); slot++)
	{
		// CHeck if the memory card is dirty in general.
		if (memCard[slot].changed)
		{
			memCard[slot].changed = false;
			ret = true;
		}
	}
	return ret;
}

/**
* Save the memory card(s).
*/
void GCNCard_Save(void)
{
	if (UseGCIFolders)
	{
		// FIXME: Not supported yet.
		return;
	}

	if (TRIGame)
	{
		// Triforce doesn't use the standard EXI CARD interface.
		return;
	}

	int slot;
	for (slot = 0; slot < ARRAY_SIZE(memCard); slot++)
	{
		if (!GCNCard_IsEnabled(slot))
		{
			// Card isn't initialized.
			continue;
		}

		// Does this card have any unsaved changes?
		GCNCard_ctx *const ctx = &memCard[slot];
		if (ctx->changed_system ||
		    ctx->BlockOffLow < ctx->BlockOffHigh)
		{
//#ifdef DEBUG_EXI
			//dbgprintf("EXI: Saving memory card in Slot %c...", (slot+'A'));
//#endif
			FIL fd;
			int ret = f_open_char(&fd, ctx->filename, FA_WRITE|FA_OPEN_EXISTING);
			if (ret == FR_OK)
			{
				UINT wrote;
				sync_before_read(ctx->base, ctx->size);

				// Save the system area, if necessary.
				if (ctx->changed_system)
				{
					f_lseek(&fd, 0);
					f_write(&fd, ctx->base, 0xA000, &wrote);
				}

				// Save the general area, if necessary.
				if (ctx->BlockOffLow < ctx->BlockOffHigh)
				{
					f_lseek(&fd, ctx->BlockOffLow);
					f_write(&fd, &ctx->base[ctx->BlockOffLow],
						(ctx->BlockOffHigh - ctx->BlockOffLow), &wrote);
				}

				f_close(&fd);
//#ifdef DEBUG_EXI
				//dbgprintf("Done!\r\n");
//#endif
			}
			else
			{
				dbgprintf("\r\nEXI: Unable to open Slot %c memory card file: %u\r\n", (slot+'A'), ret);
			}

			// Reset the low/high offsets to indicate that everything has been saved.
			ctx->BlockOffLow = 0xFFFFFFFF;
			ctx->BlockOffHigh = 0x00000000;
			ctx->changed_system = false;
		}
	}
}

/** Functions used by EXIDeviceMemoryCard(). **/

void GCNCard_ClearWriteCount(int slot)
{
	if (!GCNCard_IsEnabled(slot))
		return;

	memCard[slot].CARDWriteCount = 0;
}

/**
 * Set the current block offset.
 * @param slot Slot number.
 * @param data Block offset from the EXI command.
 */
void GCNCard_SetBlockOffset(int slot, u32 data)
{
	if (!GCNCard_IsEnabled(slot))
		return;

	u32 BlockOff = ((data>>16)&0xFF)  << 17;
	BlockOff    |= ((data>> 8)&0xFF)  << 9;
	BlockOff    |= ((data&0xFF)  &3)  << 7;
	memCard[slot].BlockOff = BlockOff;
}

/**
 * Write data to the card using the current block offset.
 * @param slot Slot number.
 * @param data Data to write.
 * @param length Length of data to write, in bytes.
 */
void GCNCard_Write(int slot, const void *data, u32 length)
{
	if (!GCNCard_IsEnabled(slot))
		return;
	GCNCard_ctx *const ctx = &memCard[slot];
	ctx->changed = true;

	// Is this update entirely within the "system area"?
	if (ctx->BlockOff < 0xA000 && ctx->BlockOff + length < 0xA000)
	{
		// This update is entirely within the "system area".
		// Only set the flag; don't set block offsets.
		ctx->changed_system = true;
	}
	else
	{
		// Update the block offsets for saving.
		if (ctx->BlockOff < ctx->BlockOffLow)
			ctx->BlockOffLow = ctx->BlockOff;
		if (ctx->BlockOff + length > ctx->BlockOffHigh)
			ctx->BlockOffHigh = ctx->BlockOff + length;
		ctx->changed = true;

		if (ctx->BlockOff < 0xA000)
		{
			// System area as well as general area.
			ctx->changed_system = true;
		}
		if (ctx->BlockOffLow < 0xA000)
		{
			// BlockOffLow shouldn't be less than 0xA000.
			// Otherwise, we end up with double writing.
			// (Not a problem; just wastes time.)
			ctx->BlockOffLow = 0xA000;
			ctx->changed_system = true;
		}
	}

	// FIXME: Verify that this doesn't go out of bounds.
	sync_before_read((void*)data, length);
	memcpy(&ctx->base[ctx->BlockOff], data, length);
	sync_after_write(&ctx->base[ctx->BlockOff], length);
}

/**
 * Read data from the card using the current block offset.
 * @param slot Slot number.
 * @param data Buffer for the read data.
 * @param length Length of data to read, in bytes.
 */
void GCNCard_Read(int slot, void *data, u32 length)
{
	if (!GCNCard_IsEnabled(slot))
		return;
	GCNCard_ctx *const ctx = &memCard[slot];

	// FIXME: Verify that this doesn't go out of bounds.
	sync_before_read(&ctx->base[ctx->BlockOff], length);
	memcpy(data, &ctx->base[ctx->BlockOff], length);
	sync_after_write(data, length);
}

/**
 * Get the card's "code" value.
 * @param slot Slot number.
 * @param Card's "code" value.
 */
u32 GCNCard_GetCode(int slot)
{
	if (!GCNCard_IsEnabled(slot))
		return 0;
	return memCard[slot].code;
}

/**
 * Set the current block offset. (ERASE mode; uses sector values only.)
 * @param slot Slot number.
 * @param Data Block offset (sector values) from the EXI command.
 */
void GCNCard_SetBlockOffset_Erase(int slot, u32 data)
{
	if (!GCNCard_IsEnabled(slot))
		return;

	u32 BlockOff = (((u32)data>>16)&0xFF)  << 17;
	BlockOff    |= (((u32)data>> 8)&0xFF)  << 9;
	memCard[slot].BlockOff = BlockOff;
}
