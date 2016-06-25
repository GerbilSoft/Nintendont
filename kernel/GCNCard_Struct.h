// Nintendont: GameCube Memory Card structures.
#ifndef __GCNCARD_STRUCT_H__
#define __GCNCARD_STRUCT_H__

#include "global.h"

// Card header.
typedef struct PACKED _card_header
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

// Memory card directory entry struct.
// Used by GCI file headers.
#define CARD_FILENAMELEN 32
#define CARD_MAXFILES 127

/**
 * Directory control block.
 */
typedef struct PACKED _card_dircntrl
{
	u8 pad[58];
	u16 updated;	// Update counter.
	u16 chksum1;	// Checksum 1.
	u16 chksum2;	// Checksum 2.
} card_dircntrl;

/**
 * Directory entry.
 * Addresses are relative to the start of the card image.
 */
typedef struct PACKED _card_direntry
{
	char gamecode[6];       // Game code. (ID6)
	u8 pad_00;         // Padding. (0xFF)
	u8 bannerfmt;      // Banner format.
	char filename[CARD_FILENAMELEN];        // Filename.
	u32 lastmodified;  // Last modified time. (seconds since 2000/01/01)
	u32 iconaddr;      // Icon address.
	u16 iconfmt;       // Icon format.
	u16 iconspeed;     // Icon speed.
	u8 permission;     // File permissions.
	u8 copytimes;      // Copy counter.
	u16 block;         // Starting block address.
	u16 length;        // File length, in blocks.
	u16 pad_01;        // Padding. (0xFFFF)
	u32 commentaddr;   // Comment address.
} card_direntry;

/**
 * Block allocation table.
 */
typedef struct PACKED _card_bat
{
	u16 chksum1;	// Checksum 1.
	u16 chksum2;	// Checksum 2.
	u16 updated;	// Update counter.
	u16 freeblocks;	// Number of free blocks.
	u16 lastalloc;	// Last block allocated.
	
	// NOTE: Subtract 5 from the block address
	// before looking it up in the FAT!
	u16 fat[0xFFB];	// File allocation table.
} card_bat;

#endif /* __GCNCARD_STRUCT_H__ */
