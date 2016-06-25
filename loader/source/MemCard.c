/*
MemCard.c for Nintendont (Kernel)

Copyright (C) 2014 FIX94

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include "global.h"
#include "exi.h"
#include "ff_utf8.h"

// Memory card header.
typedef struct __attribute__ ((packed)) _card_header
{
	u8 reserved1[12];
	u64 formatTime;		// Format time. (OSTime value; 1 tick == 1/40,500,000 sec)
        u32 sramBias;		// SRAM bias at time of format.
        u32 sramLang;		// SRAM language.
        u32 reserved2;

        u16 device_id;		// 0 if formatted in slot A; 1 if formatted in slot B
        u16 size;		// size of card, in Mbits
        u16 encoding;		// 0 == cp1252; 1 == Shift-JIS

        u8 reserved3[0x1D6];	// Should be 0xFF.
        u16 chksum1;		// Checksum.
        u16 chksum2;		// Inverted checksum.
} card_header;


/* Same Method as SRAM Checksum in Kernel */
static void doChecksum(const u16 *buffer, u32 size, u16 *c1, u16 *c2)
{
	*c1 = 0; *c2 = 0;
	for (u32 i = size/sizeof(u16); i > 0; i--, buffer++)
	{
		*c1 += *buffer;
		*c2 += *buffer ^ 0xFFFF;
	}
	if (*c1 == 0xFFFF) *c1 = 0;
	if (*c2 == 0xFFFF) *c2 = 0;
}

/**
 * Create a blank memory card image.
 * @param MemCard Memory card filename.
 * @return True on success; false on error.
 */
bool GenerateMemCard(const char *MemCard)
{
	if (!MemCard || MemCard[0] == 0)
		return false;

	FIL f;
	if (f_open_char(&f, MemCard, FA_WRITE|FA_CREATE_NEW) != FR_OK)
		return false;

	//Get memory to format
	u8 *MemcardBase = memalign(32, MEM_CARD_SIZE(ncfg->MemCardBlocks));
	memset(MemcardBase, 0, MEM_CARD_SIZE(ncfg->MemCardBlocks));
	//Fill Header and Dir Memory with 0xFF
	memset(MemcardBase, 0xFF, 0x6000);

	// Header block.
	card_header *header = (card_header*)MemcardBase;
	//Normally this area contains a checksum which is built via SRAM and formatting time, you can
	//skip that though because if the formatting time is 0 the resulting checksum is also 0 ;)
	memset(header->reserved1, 0, sizeof(header->reserved1));
	header->formatTime = 0;		// TODO: Actually set this?
	//From SRAM as defined in PatchCodes.h
	header->sramBias = 0x17CA2A85;
	//Use current language for SRAM language
	header->sramLang = ncfg->Language;
	//Memory Card File Mode
	header->reserved2 = ((ncfg->GameID & 0xFF) == 'J' ? 2 : 0);
	//Assuming slot A.
	header->device_id = 0;
	//Memory Card size in MBits total
	header->size = MEM_CARD_SIZE(ncfg->MemCardBlocks) >> 17;
	//Memory Card filename encoding
	header->encoding = ((ncfg->GameID & 0xFF) == 'J');
	//Generate Header Checksum
	doChecksum((u16*)header, 0x1FC, &header->chksum1, &header->chksum2);

	//Set Dir Update Counters
	*((u16*)&MemcardBase[0x3FFA]) = 0;
	*((u16*)&MemcardBase[0x5FFA]) = 1;
	//Generate Dir Checksums
	doChecksum((u16*)&MemcardBase[0x2000], 0x1FFC, (u16*)&MemcardBase[0x3FFC], (u16*)&MemcardBase[0x3FFE]);
	doChecksum((u16*)&MemcardBase[0x4000], 0x1FFC, (u16*)&MemcardBase[0x5FFC], (u16*)&MemcardBase[0x5FFE]);

	//Set Block Update Counters
	*((u16*)&MemcardBase[0x6004]) = 0;
	*((u16*)&MemcardBase[0x8004]) = 1;
	//Set Free Blocks
	*((u16*)&MemcardBase[0x6006]) = MEM_CARD_BLOCKS(ncfg->MemCardBlocks);
	*((u16*)&MemcardBase[0x8006]) = MEM_CARD_BLOCKS(ncfg->MemCardBlocks);
	//Set last allocated Block
	*((u16*)&MemcardBase[0x6008]) = 4;
	*((u16*)&MemcardBase[0x8008]) = 4;
	//Generate Block Checksums
	doChecksum((u16*)&MemcardBase[0x6004], 0x1FFC, (u16*)&MemcardBase[0x6000], (u16*)&MemcardBase[0x6002]);
	doChecksum((u16*)&MemcardBase[0x8004], 0x1FFC, (u16*)&MemcardBase[0x8000], (u16*)&MemcardBase[0x8002]);

	//Write it into a file
	UINT wrote;
	f_write(&f, MemcardBase, MEM_CARD_SIZE(ncfg->MemCardBlocks), &wrote);
	f_close(&f);
	free(MemcardBase);
	gprintf("Memory Card File created!\r\n");
	return true;
}
