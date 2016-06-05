/*
BT.h for Nintendont (Kernel)

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
#ifndef _BT_H_
#define _BT_H_

#include "lwbt/bte.h"
#include "../common/include/BT.h"

void BTInit(void);
void BTUpdateRegisters(void);

struct BTPadStat {
	u32 controller;
	u32 timeout;
	u32 transfertype;
	u32 transferstate;
	u32 channel;
	u32 rumble;
	u32 rumbletime;
	s16 xAxisLmid;
	s16 xAxisRmid;
	s16 yAxisLmid;
	s16 yAxisRmid;
	struct bte_pcb *sock;
	struct bd_addr bdaddr;
} ALIGNED(32);

/* From LibOGC conf.h */

#define CONF_PAD_MAX_REGISTERED 10
#define CONF_PAD_MAX_ACTIVE 4

typedef struct PACKED _conf_pad_device {
	u8 bdaddr[6];
	char name[0x40];
} conf_pad_device;

typedef struct PACKED _conf_pads {
	u8 num_registered;
	conf_pad_device registered[CONF_PAD_MAX_REGISTERED];
	conf_pad_device active[CONF_PAD_MAX_ACTIVE];
	conf_pad_device balance_board;
	conf_pad_device unknown;
} conf_pads;

#endif
