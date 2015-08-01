#ifndef _TYPES_H_
#define _TYPES_H_

/**
 * Because this file is always the first include in every
 * file, it doubles as the configuration file.
 */

#define ARCH_i386 /* Build target */

#ifdef ARCH_i386
/* Unsigned types */
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;

/* data type sizes for i386 */
typedef unsigned char uint_8;
typedef unsigned short uint_16;
typedef unsigned int uint_32;
typedef unsigned long long uint_64;

#ifndef __LINUX_DEFS__
/* Types that conflict with the Linux definitions*/

typedef unsigned int off_t; /* Used to represent file offsets */
typedef unsigned int size_t; /* Used to represent the size of an object. */
typedef signed short uid_t; /* User id*/
typedef signed short gid_t; /* Group id*/

/* File permissions */
typedef uint_32 mode_t;

typedef signed int pid_t; /* Process ID */
typedef signed int clock_t;

/* NULL */
#define NULL ((void*)0)

/* Some POSIX definitions: */
typedef uint_16 dev_t;
typedef uint_16 ino_t;
typedef uint_16 nlink_t;
typedef unsigned int blksize_t;
typedef unsigned int blkcnt_t;
typedef signed int time_t;
typedef signed int suseconds_t;

#endif

typedef uint pgdir;
typedef uint pgtbl;

#endif /* i386 */

/* Uncomment to enable global debug */
//#define __GLOBAL_DEBUG__

#ifdef __GLOBAL_DEBUG__
	static const uchar debug = 1;
#else
	static const uchar debug = 0;
#endif

//#define CACHE_WATCH 

/** 
 * Uncomment if you want the kernel to panic when it runs out
 * of cache space. (fatal error)
 */
#define CACHE_PANIC 

#endif