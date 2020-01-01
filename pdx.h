/*
 * This file contains types and definitions for Portland State University.
 * The contents are intended to be visible in both user and kernel space.
 */

#ifndef PDX_INCLUDE
#define PDX_INCLUDE

#define TRUE 1
#define FALSE 0
#define RETURN_SUCCESS 0
#define RETURN_FAILURE -1

#define NUL 0
#ifndef NULL
#define NULL NUL
#endif  // NULL

#define TPS 1000   // ticks-per-second
#define SCHED_INTERVAL (TPS/100)  // see trap.c

#ifdef CS333_P2
#define GID 0 // Default GID for first process
#define UID 0 // Default UID for first process
#endif // CS333_P2
#define NPROC  64  // maximum number of processes -- normally in param.h

#ifdef CS333_P4
#define TICKS_TO_PROMOTE (1*TPS)
#define PRIORITY MAXPRIO
//#define BUDGET 1000
#define MAXPRIO 6
#define BUDGET 300
#endif // CS333_P4

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#endif  // PDX_INCLUDE
