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
#include "micro/micro.h"

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
    reset_info_t * reset_info_ptr = (reset_info_t *) &__RESETINFO__begin;
    reset_info_ptr->reset_reason = reset_reason;
    reset_info_ptr->reset_signature = RESET_INFO_SIGNATURE_VALID;

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
    reset_info_t * reset_info_ptr = (reset_info_t *) &__RESETINFO__begin;

    memset(reset_info_ptr->ip_stack, 0x0, BACKTRACE_MAX_DEPTH * sizeof(uint32_t));
    unwind_ctrl_t ctrl = {
            .current_depth = 0,
            .max_depth = BACKTRACE_MAX_DEPTH,
            .ip_stack = reset_info_ptr->ip_stack,
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
    backtrace();

    // break if debugger is connected
    debug_breakpoint();

    // reset the device
    system_reset(RESET_CRASH_ASSERT);
}

uint16_t system_crash_handler(void)
{
    uint16_t reset_reason = RESET_FAULT_UNKNOWN;
    reset_info_t * reset_info_ptr = (reset_info_t *) &__RESETINFO__begin;

    // store scb contents
    reset_info_ptr->scb_info.icsr.ICSR = SCB->ICSR;
    reset_info_ptr->scb_info.shcsr.SHCSR = SCB->SHCSR;
    reset_info_ptr->scb_info.cfsr.CFSR = SCB->CFSR;
    reset_info_ptr->scb_info.hfsr.HFSR = SCB->HFSR;
    reset_info_ptr->scb_info.mmar.MMAR = SCB->MMFAR;
    reset_info_ptr->scb_info.bfar.BFAR = SCB->BFAR;
    reset_info_ptr->scb_info.afsr.AFSR = SCB->AFSR;

    // tell the reset reason
    switch (reset_info_ptr->scb_info.icsr.VECTACTIVE)
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
            // Usage can be assertion error
            if (reset_reason == RESET_FAULT_UNKNOWN)
                reset_reason = RESET_FAULT_USAGE;

            break;
        }
        case IRQ_TO_VECT_NUM(DebugMonitor_IRQn):
        {
            reset_reason = RESET_FAULT_DBGMON;
            break;
        }
        default:
        {
            if (reset_info_ptr->scb_info.icsr.VECTACTIVE && reset_info_ptr->scb_info.icsr.VECTACTIVE < VECTOR_TABLE_LENGTH)
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

void reset_info_clear(void)
{
    memset(&__RESETINFO__begin, 0x0, sizeof(reset_info_t));
}

void stack_reg_dump(uint32_t * stack_addr)
{
    reset_info_t * reset_info_ptr = (reset_info_t *) &__RESETINFO__begin;

    // fetch auto stacked register value before interrupt
    reset_info_ptr->core_registers.R0 = *(stack_addr + REG_R0);
    reset_info_ptr->core_registers.R1 = *(stack_addr + REG_R1);
    reset_info_ptr->core_registers.R2 = *(stack_addr + REG_R2);
    reset_info_ptr->core_registers.R3 = *(stack_addr + REG_R3);
    reset_info_ptr->core_registers.R12 = *(stack_addr + REG_R12);
    reset_info_ptr->core_registers.LR = *(stack_addr + REG_LR);
    reset_info_ptr->core_registers.PC = *(stack_addr + REG_PC);
    reset_info_ptr->core_registers.XPSR.XPSR = *(stack_addr + REG_PSR);
    reset_info_ptr->core_registers.SP = (uint32_t) (stack_addr + REG_PREV_SP);
}

void debug_breakpoint(void)
{
    // give user a chance to break here if debugger is attached
    if (DBG_Connected())
    {
        asm volatile ("bkpt #0");
    }

}
