// Nintendont: Common HID struct definitions.
#ifndef __COMMON_HID_H__
#define __COMMON_HID_H__

#include "types.h"

typedef struct Layout
{
	u32 Offset;
	u32 Mask;
} layout;

typedef struct StickLayout
{
	u32 	Offset;
	s8		DeadZone;
	u32		Radius;
} stickLayout;

// HID controller quirks.
typedef enum {
	HID_QUIRK_NONE = 0,						// No quirks
	HID_QUIRK_WIIU_GCN_ADAPTER,					// Nintendo WiiU GameCube Adapter (057E:0337)
	HID_QUIRK_MAYFLASH_CCPRO_ADAPTER,				// Mayflash Classic Controller Pro Adapter (0925:03E8)
	HID_QUIRK_LOGITECH_THRUSTMASTER_FIRESTORM_DUAL_ANALOG_2,	// Logitech Thrustmaster Firestorm Dual Analog 2 (044F:B303)
	HID_QUIRK_MAYFLASH_3_IN_1_MAGIC_JOY_BOX,			// Mayflash 3 in 1 Magic Joy Box (0926:2526)
	HID_QUIRK_MS_SIDEWINDER_FF_2_JOYSTICK,				// Microsoft Sidewinder Force Feedback 2 Joystick (045E:001B)
	HID_QUIRK_THRUSTMASTER_DUAL_ANALOG_4,				// Thrustmaster Dual Analog 4 (044F:B315)
	HID_QUIRK_TRIO_LINKER_PLUS,					// Trio Linker Plus (2006:0118)
	HID_QUIRK_PS3_DUALSHOCK,					// PS3 DualShock Controller (054C:0268)
} HIDQuirkType;

typedef struct Controller
{
	u32 VID;	// actually u16
	u32 PID;	// actually u16

	u32 Polltype;
	u32 DPAD;
	u32 DPADMask;
	u32 DigitalLR;
	u32 MultiIn;
	u32 MultiInValue;

	layout Power;

	layout A;
	layout B;
	layout X;
	layout Y;
	layout ZL;
	layout Z;
	
	layout L;
	layout R;
	layout S;
	
	layout Left;
	layout Down;
	layout Right;
	layout Up;

	layout RightUp;
	layout DownRight;
	layout DownLeft;
	layout UpLeft;

	stickLayout StickX;
	stickLayout StickY;
	stickLayout CStickX;
	stickLayout CStickY;
	u32 LAnalog;
	u32 RAnalog;

	// Not part of controllers.ini
	u32 QuirkType;	// HIDQuirkType
} controller;

#endif /* __COMMON_HID_H__ */
