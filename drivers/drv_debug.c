/**
 * @brief Debug library for EFR32
 */
#include <string.h>
#include <stdint.h>
#include <unwind.h>

#include "em_device.h"
#include "drv_debug.h"
#include "em_wdog.h"
#include "em_dbg.h"
#include "bits.h"
#include "reset_info.h"

#define IRQ_TO_VECT_NUM(x) ((x) + 16)

typedef enum
{
    REG_R0 = 0,
    REG_R1,
    REG_R2,
    REG_R3,
    REG_R12,
    REG_LR,
    REG_PC,
    REG_PSR,
    REG_PREV_SP,
} core_regs_auto_stack_t;

void system_reset(uint16_t reset_reason)
{
    // write to reset info with reset reason
    reset_info_set_reset_reason(reset_reason);

    // set reset region valid
    reset_info_set_valid();

    // break here if debugger is attached
    debug_breakpoint();

    // disable global interrupts
    __disable_irq();

    // disable irqs
    for (register uint32_t i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
    }
    for (register uint32_t i = 0; i < 8; i++)
    {
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // barriers
    __DSB();
    __ISB();

    // enable interrupts
    __enable_irq();

    // reset the device
    NVIC_SystemReset();

    // shouldn't run to this point
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

void backtrace(void)
{
    // get reset info
    reset_info_map_t * reset_info_map = (reset_info_map_t *) reset_info_read();

    memset(reset_info_map->ip_stack, 0x0, BACKTRACE_MAX_DEPTH * sizeof(uint32_t));
    unwind_ctrl_t ctrl = {
            .current_depth = 0,
            .max_depth = BACKTRACE_MAX_DEPTH,
            .ip_stack = reset_info_map->ip_stack,
            .code_stack = NULL
    };

    // dump backtrace info
    _Unwind_Backtrace(&backtrace_handler, &ctrl);
}

void assert_failed(const char * file, uint32_t line)
{
    // disable watchdog timer
    BITS_CLEAR(WDOG0->CTRL, WDOG_CTRL_EN);
    BITS_CLEAR(WDOG1->CTRL, WDOG_CTRL_EN);

    // store the assert information
    reset_info_map_t * reset_info_map = (reset_info_map_t *) reset_info_read();
    reset_info_map->assert_info.assert_filename_path_ptr = file;
    reset_info_map->assert_info.assert_line = line;

    // copy filename to struct
    char * base = strrchr(file, '/');
    strncpy(
            reset_info_map->assert_info.assert_filename,
            base ? base + 1 : file,
            sizeof(reset_info_map->assert_info.assert_filename)
    );

    // dump call stack
    backtrace();

    // reset the device
    system_reset(RESET_CRASH_ASSERT);
}

void assert_failed_func(const char * function_name)
{
    assert_failed(function_name, 0);
}

void assert_failed_null()
{
    assert_failed("IGNORED", 0);
}

uint16_t system_crash_handler(void)
{
    uint16_t reset_reason = RESET_FAULT_UNKNOWN;
    reset_info_map_t * reset_info_map = (reset_info_map_t *) reset_info_read();

    // store scb contents
    reset_info_map->scb_info.icsr.ICSR = SCB->ICSR;
    reset_info_map->scb_info.shcsr.SHCSR = SCB->SHCSR;
    reset_info_map->scb_info.cfsr.CFSR = SCB->CFSR;
    reset_info_map->scb_info.hfsr.HFSR = SCB->HFSR;
    reset_info_map->scb_info.mmar.MMAR = SCB->MMFAR;
    reset_info_map->scb_info.bfar.BFAR = SCB->BFAR;
    reset_info_map->scb_info.afsr.AFSR = SCB->AFSR;

    // tell the reset reason
    switch (reset_info_map->scb_info.icsr.VECTACTIVE)
    {
        case IRQ_TO_VECT_NUM(WDOG0_IRQn):
        {
            if (WDOG0->IF & WDOG_IF_WARN)
            {
                reset_reason = RESET_WATCHDOG_CAUGHT;
            }
            break;
        }
        case IRQ_TO_VECT_NUM(WDOG1_IRQn):
        {
            if (WDOG1->IF & WDOG_IF_WARN)
            {
                reset_reason = RESET_WATCHDOG_CAUGHT;
            }
            break;
        }
        case IRQ_TO_VECT_NUM(HardFault_IRQn):
        {
            reset_reason = RESET_FAULT_HARD;
            break;
        }
        case IRQ_TO_VECT_NUM(MemoryManagement_IRQn):
        {
            reset_reason = RESET_FAULT_MEM;
            break;
        }
        case IRQ_TO_VECT_NUM(BusFault_IRQn):
        {
            reset_reason = RESET_FAULT_BUS;
            break;
        }
        case IRQ_TO_VECT_NUM(UsageFault_IRQn):
        {
            // UsageFault caused by assertion error
            if (reset_info_map->scb_info.cfsr.CFSR & (1 << 16) && // TODO: not sure if this applies
                    reset_info_map->core_registers.PC == 0xDE42)
            {
                reset_reason = RESET_CRASH_ASSERT;
            }
            else
            {
                reset_reason = RESET_FAULT_USAGE;
            }

            break;
        }
        case IRQ_TO_VECT_NUM(DebugMonitor_IRQn):
        {
            reset_reason = RESET_FAULT_DBGMON;
            break;
        }
        default:
        {
            if (reset_info_map->scb_info.icsr.VECTACTIVE &&
                    reset_info_map->scb_info.icsr.VECTACTIVE < VECTOR_TABLE_LENGTH)
            {
                reset_reason = RESET_FAULT_BADVECTOR;
            }
            break;
        }
    }

    // rewind
    backtrace();

    return reset_reason;
}

void stack_reg_dump(const uint32_t * stack_addr)
{
    reset_info_map_t * reset_info_map = (reset_info_map_t *) reset_info_read();

    // fetch auto stacked register value before interrupt on stack
    reset_info_map->core_registers.R0 = *(stack_addr + REG_R0);
    reset_info_map->core_registers.R1 = *(stack_addr + REG_R1);
    reset_info_map->core_registers.R2 = *(stack_addr + REG_R2);
    reset_info_map->core_registers.R3 = *(stack_addr + REG_R3);
    reset_info_map->core_registers.R12 = *(stack_addr + REG_R12);
    reset_info_map->core_registers.LR = *(stack_addr + REG_LR);
    reset_info_map->core_registers.PC = *(stack_addr + REG_PC);
    reset_info_map->core_registers.XPSR.XPSR = *(stack_addr + REG_PSR);
    reset_info_map->core_registers.SP = (uint32_t) (stack_addr + REG_PREV_SP);
}

void debug_breakpoint(void)
{
    // give user a chance to break here if debugger is attached
    if (DBG_Connected())
    {
        asm volatile ("bkpt #0");
    }
}
