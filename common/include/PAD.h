// Nintendont: Common GameCube PAD structs and definitions.
#ifndef __COMMON_PAD_H__
#define __COMMON_PAD_H__

#include "types.h"

#ifdef __cplusplus
extern "C"
#endif

typedef struct _padstatus {
	unsigned short button;
	char stickX;
	char stickY;
	char substickX;
	char substickY;
	unsigned char triggerLeft;
	unsigned char triggerRight;
	unsigned char analogA;
	unsigned char analogB;
	char err;
} PADStatus;

// GameCube button definitions.
#define PAD_BUTTON_LEFT		0x0001
#define PAD_BUTTON_RIGHT	0x0002
#define PAD_BUTTON_DOWN		0x0004
#define PAD_BUTTON_UP		0x0008
#define PAD_TRIGGER_Z		0x0010
#define PAD_TRIGGER_R		0x0020
#define PAD_TRIGGER_L		0x0040
#define PAD_BUTTON_A		0x0100
#define PAD_BUTTON_B		0x0200
#define PAD_BUTTON_X		0x0400
#define PAD_BUTTON_Y		0x0800
#define PAD_BUTTON_MENU		0x1000
#define PAD_BUTTON_START	0x1000

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_PAD_H__ */
