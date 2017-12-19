//------------------------------------------------------------------------------
// @file hal/micro/cortexm3/faults-v7m.s79
// @brief PRIV
//
//
// Copyright 2016 Silicon Laboratories, Inc. All rights reserved.           *80*
//------------------------------------------------------------------------------

#include "../compiler/asm.h"

        __IMPORT__ system_crash_handler
        __IMPORT__ system_reset
        __IMPORT__ stack_reg_dump
        __IMPORT__ debug_breakpoint

// define the values the link register can have on entering an exception
__EQU__(EXC_RETURN_HANDLER_MSP, 0xFFFFFFF1)
__EQU__(EXC_RETURN_THREAD_MSP,  0xFFFFFFF9)
__EQU__(EXC_RETURN_THREAD_PSP,  0xFFFFFFFD)

// define stack bytes needed by halCrashHandler()
__EQU__(CRASH_STACK_SIZE, 64)

//------------------------------------------------------------------------------
//
// Various types of crashes generate NMIs, hard, bus, memory, usage and debug
// monitor faults that vector to the routines defined here.
//
// Although all of these could vector directly to the common fault handler,
// each is given its own entry point to allow setting a breakpoint for a
// particular type of crash.
//
//------------------------------------------------------------------------------




//------------------------------------------------------------------------------
// Common fault handler prolog processing
//
//   determines which of the three possible EXC_RETURN values is in lr,
//   then uses lr to save processor registers r0-r12 to the crash info area
//
//   restores lr's value, and then saves lr, msp and psp to the crash info area
//
//   checks the stack pointer to see if it is valid and won't overwrite the crash
//   info area - if needed, resets the stack pointer to the top of the stack
//
//   calls halInternalCrashHandler() to continue saving crash data. This
//   C function can finish crash processing without risking further faults.
//   It returns the fault reason, and this is passed to halInternalSysReset()
//   that forces a processor reset.
//
//------------------------------------------------------------------------------
        __CODE__
        __THUMB__
        __EXPORT__ fault
fault:
        // disable interrupts
        cpsid   i

        // test thread or handler mode
        tst lr, #4
        ite eq
        mrseq r0, msp
        mrsne r0, psp

        // load auto stacked value
        bl.w stack_reg_dump

        // do initial fault diagnose
        bl.w system_crash_handler

        // reset the device with corresponding reset reason (stored in R0)
        bl.w system_reset

        __END__
