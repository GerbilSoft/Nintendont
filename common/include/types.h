// Nintendont: Common type definitions.
#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

// gcc attributes
#define ALIGNED(n)	__attribute__((aligned(n)))
#define PACKED		__attribute__((packed))

// On PowerPC, only define these types if
// libogc's gctypes.h wasn't included already.
#if !defined(__PPC__) || (defined(__PPC__) && !defined(__GCTYPES_H__))
typedef volatile unsigned char vu8;
typedef volatile unsigned short vu16;
typedef volatile unsigned int vu32;
typedef volatile unsigned long long vu64;

typedef volatile signed char vs8;
typedef volatile signed short vs16;
typedef volatile signed int vs32;
typedef volatile signed long long vs64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
#endif /* !defined(__PPC__) || (defined(__PPC__) && !defined(__GCTYPES_H__)) */

#endif /* __COMMON_TYPES_H__ */
