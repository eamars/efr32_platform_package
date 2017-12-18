/**
 * @brief Debug library for EFR32
 */
#include <string.h>
#include <stdint.h>

#include "drv_debug.h"
#include "em_wdog.h"
#include "bits.h"
#include "reset_info.h"
#include "unwind.h"


void system_reset(uint16_t reset_reason)
{
    while (1)
    {

    }
}

static _Unwind_Reason_Code backtrace_handler(_Unwind_Context * contex, void * args)
{
    unwind_ctrl_t * ctrl = args;

    if (ctrl->current_depth < ctrl->max_depth)
    {
        uint32_t inst = _Unwind_GetRegionStart(contex);
        uint32_t lr = _Unwind_GetIP(contex);

        // record the address of lr
        if (ctrl->ip_stack)
            ctrl->ip_stack[ctrl->current_depth] = lr;
        if (ctrl->code_stack)
            ctrl->code_stack[ctrl->current_depth] = inst;

        ctrl->current_depth += 1;

        // end of stack
        // Note: the instruction for Thumb function requires LSB to be 1 to distinguish with ARM call routine
        if ((inst | 0x01) == (uint32_t) &Reset_Handler)
            return _URC_END_OF_STACK;

        return _URC_OK;
    }

    return _URC_END_OF_STACK;
}

void backtrace(unwind_ctrl_t * ctrl)
{
    _Unwind_Backtrace(&backtrace_handler, ctrl);
}

void assert_efr32(const char * file, uint32_t line)
{
    // disable watchdog timer
    BITS_CLEAR(WDOG0->CTRL, WDOG_CTRL_EN);
    BITS_CLEAR(WDOG1->CTRL, WDOG_CTRL_EN);

    // disable interrupts
    __disable_irq();

    // store the assert information
    reset_info_t * reset_info_ptr = (reset_info_t *) &__RESETINFO__begin;
    reset_info_ptr->assert_info.assert_filename_path_ptr = file;
    reset_info_ptr->assert_info.assert_line = line;

    // copy filename to struct
    char * base = strrchr(file, '/');
    strncpy(
            reset_info_ptr->assert_info.assert_filename,
            base ? base + 1 : file,
            sizeof(reset_info_ptr->assert_info.assert_filename)
    );

    // dump call stack
    memset(reset_info_ptr->ip_stack, 0x0, BACKTRACE_MAX_DEPTH * sizeof(uint32_t));
    unwind_ctrl_t ctrl = {
            .current_depth = 0,
            .max_depth = BACKTRACE_MAX_DEPTH,
            .ip_stack = reset_info_ptr->ip_stack,
            .code_stack = NULL
    };
    backtrace(&ctrl);

    // reset the device
    system_reset(0);
}
