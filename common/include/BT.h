// Nintendont: Common Bluetooth structs and definitions.
#ifndef __COMMON_BT_H__
#define __COMMON_BT_H__

#include "types.h"

#ifdef __cplusplus
extern "C"
#endif

struct BTPadCont {
	u32 used;
	s16 xAxisL;
	s16 xAxisR;
	s16 yAxisL;
	s16 yAxisR;
	u32 button;
	u8 triggerL;
	u8 triggerR;
	s16 xAccel;
	s16 yAccel;
	s16 zAccel;
} ALIGNED(32);

// Bluetooth button definitions.
#define BT_DPAD_UP              0x0001
#define BT_DPAD_LEFT            0x0002
#define BT_TRIGGER_ZR           0x0004
#define BT_BUTTON_X             0x0008
#define BT_BUTTON_A             0x0010
#define BT_BUTTON_Y             0x0020
#define BT_BUTTON_B             0x0040
#define BT_TRIGGER_ZL           0x0080
#define BT_TRIGGER_R            0x0200
#define BT_BUTTON_START         0x0400
#define BT_BUTTON_HOME          0x0800
#define BT_BUTTON_SELECT        0x1000
#define BT_TRIGGER_L            0x2000
#define BT_DPAD_DOWN            0x4000
#define BT_DPAD_RIGHT           0x8000

// Wii Remote + Nunchuk button definitions.
#define WM_BUTTON_TWO		0x0001
#define WM_BUTTON_ONE		0x0002
#define WM_BUTTON_B		0x0004
#define WM_BUTTON_A		0x0008
#define WM_BUTTON_MINUS		0x0010
#define NUN_BUTTON_Z		0x0020 
#define NUN_BUTTON_C		0x0040
#define WM_BUTTON_HOME		0x0080
#define WM_BUTTON_LEFT		0x0100
#define WM_BUTTON_RIGHT		0x0200
#define WM_BUTTON_DOWN		0x0400
#define WM_BUTTON_UP		0x0800
#define WM_BUTTON_PLUS		0x1000

#ifdef __cplusplus
}
#endif

#endif /* __COMMON_BT_H__ */
