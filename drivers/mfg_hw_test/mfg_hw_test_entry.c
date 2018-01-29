/**
 * @brief Manufacture Hardware Test implementation
 * @file mfg_hw_test_entry.c
 * @date 29/Jan/2018
 * @author Ran Bao
 */
#include PLATFORM_HEADER
#include BOARD_HEADER
#include "application_header.h"
#include "bootloader_api.h"

extern uint32_t __load_start_data;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __DYNAMIC_VTOR__begin;
extern uint32_t __DYNAMIC_VTOR__end;

void __libc_init_array (void);

extern uint32_t __StackTop;
void Default_Handler(void);

void mfg_main(void);
void mfg_generic_irq_handler(uint32_t);

void mfg_default_handler(void);
void mfg_reset_handler(void);

void mfg_nmi_handler(void) __attribute__((weak, alias("mfg_default_handler")));
void mfg_hardfault_handler(void) __attribute__((weak, alias("mfg_default_handler")));
void mfg_memmanage_handler(void) __attribute__((weak, alias("mfg_default_handler")));
void mfg_busfault_handler(void) __attribute__((weak, alias("mfg_default_handler")));
void mfg_usagefault_handler(void) __attribute__((weak, alias("mfg_default_handler")));
void mfg_svc_handler(void) __attribute__((weak, alias("mfg_default_handler")));
void mfg_debugmon_handler(void) __attribute__((weak, alias("mfg_default_handler")));
void mfg_pendsv_handler(void) __attribute__((weak, alias("mfg_default_handler")));
void mfg_systick_handler(void) __attribute__((weak, alias("mfg_default_handler")));

__attribute__((aligned(0x40)))
const irq_handler_t mfg_test_vector_table[] =
{
        // cortex m4 interrupt vectors
        (irq_handler_t)&__StackTop,                   /*      Initial Stack Pointer     */
        mfg_reset_handler,                            /*      Reset Handler             */
        mfg_nmi_handler,                              /*      NMI Handler               */
        mfg_hardfault_handler,                        /*      Hard Fault Handler        */
        mfg_memmanage_handler,                        /*      MPU Fault Handler         */
        mfg_busfault_handler,                         /*      Bus Fault Handler         */
        mfg_usagefault_handler,                       /*      Usage Fault Handler       */
        mfg_default_handler,                          /*      Reserved                  */
        mfg_default_handler,                          /*      Reserved                  */
        mfg_default_handler,                          /*      Reserved                  */
        mfg_default_handler,                          /*      Reserved                  */
        mfg_svc_handler,                              /*      SVCall Handler            */
        mfg_debugmon_handler,                         /*      Debug Monitor Handler     */
        mfg_default_handler,                          /*      Reserved                  */
        mfg_pendsv_handler,                           /*      PendSV Handler            */
        mfg_systick_handler,                          /*      SysTick Handler           */

        // external interrupts
        EMU_IRQHandler,                       /*  0 - EMU       */
        FRC_PRI_IRQHandler,                       /*  1 - FRC_PRI       */
        WDOG0_IRQHandler,                       /*  2 - WDOG0       */
        WDOG1_IRQHandler,                       /*  3 - WDOG1       */
        FRC_IRQHandler,                       /*  4 - FRC       */
        MODEM_IRQHandler,                       /*  5 - MODEM       */
        RAC_SEQ_IRQHandler,                       /*  6 - RAC_SEQ       */
        RAC_RSM_IRQHandler,                       /*  7 - RAC_RSM       */
        BUFC_IRQHandler,                       /*  8 - BUFC       */
        LDMA_IRQHandler,                       /*  9 - LDMA       */
        GPIO_EVEN_IRQHandler,                       /*  10 - GPIO_EVEN       */
        TIMER0_IRQHandler,                       /*  11 - TIMER0       */
        USART0_RX_IRQHandler,                       /*  12 - USART0_RX       */
        USART0_TX_IRQHandler,                       /*  13 - USART0_TX       */
        ACMP0_IRQHandler,                       /*  14 - ACMP0       */
        ADC0_IRQHandler,                       /*  15 - ADC0       */
        IDAC0_IRQHandler,                       /*  16 - IDAC0       */
        I2C0_IRQHandler,                       /*  17 - I2C0       */
        GPIO_ODD_IRQHandler,                       /*  18 - GPIO_ODD       */
        TIMER1_IRQHandler,                       /*  19 - TIMER1       */
        USART1_RX_IRQHandler,                       /*  20 - USART1_RX       */
        USART1_TX_IRQHandler,                       /*  21 - USART1_TX       */
        LEUART0_IRQHandler,                       /*  22 - LEUART0       */
        PCNT0_IRQHandler,                       /*  23 - PCNT0       */
        CMU_IRQHandler,                       /*  24 - CMU       */
        MSC_IRQHandler,                       /*  25 - MSC       */
        CRYPTO0_IRQHandler,                       /*  26 - CRYPTO0       */
        LETIMER0_IRQHandler,                       /*  27 - LETIMER0       */
        AGC_IRQHandler,                       /*  28 - AGC       */
        PROTIMER_IRQHandler,                       /*  29 - PROTIMER       */
        RTCC_IRQHandler,                       /*  30 - RTCC       */
        SYNTH_IRQHandler,                       /*  31 - SYNTH       */
        CRYOTIMER_IRQHandler,                       /*  32 - CRYOTIMER       */
        RFSENSE_IRQHandler,                       /*  33 - RFSENSE       */
        FPUEH_IRQHandler,                       /*  34 - FPUEH       */
        SMU_IRQHandler,                       /*  35 - SMU       */
        WTIMER0_IRQHandler,                       /*  36 - WTIMER0       */
        WTIMER1_IRQHandler,                       /*  37 - WTIMER1       */
        PCNT1_IRQHandler,                       /*  38 - PCNT1       */
        PCNT2_IRQHandler,                       /*  39 - PCNT2       */
        USART2_RX_IRQHandler,                       /*  40 - USART2_RX       */
        USART2_TX_IRQHandler,                       /*  41 - USART2_TX       */
        I2C1_IRQHandler,                       /*  42 - I2C1       */
        USART3_RX_IRQHandler,                       /*  43 - USART3_RX       */
        USART3_TX_IRQHandler,                       /*  44 - USART3_TX       */
        VDAC0_IRQHandler,                       /*  45 - VDAC0       */
        CSEN_IRQHandler,                       /*  46 - CSEN       */
        LESENSE_IRQHandler,                       /*  47 - LESENSE       */
        CRYPTO1_IRQHandler,                       /*  48 - CRYPTO1       */
        TRNG0_IRQHandler,                       /*  49 - TRNG0       */
        mfg_default_handler,                          /*  50 - Reserved      */
};

__attribute__((naked))
void mfg_default_handler(void)
{
    asm(
        // dump auto stacked value
        "cpsid i\n" // prevent nested interrupt
        "tst lr, #4\n"
        "ite eq\n"
        "mrseq r0, msp\n"
        "mrsne r0, psp\n"
        "bl.w stack_reg_dump\n"

        // call generic irq handler
        "mrs r0, ipsr\n"
        "bl.w mfg_generic_irq_handler\n"
    );

    while (1)
    {

    }
}

__attribute__ ((naked)) __attribute__ ((optimize("-O0")))
void mfg_reset_handler(void)
{
    // Use static vector table for handling core fault event when coping variables
    SCB->VTOR = ((uint32_t) &mfg_test_vector_table) & SCB_VTOR_TBLOFF_Msk;

    // initialize floating point co-processor
    SystemInit();

    // disable system irq
    __disable_irq();

    // copy initialized data (.data)
    for (register uint32_t *pSrc = &__load_start_data, *pDest = &__data_start__; pDest < &__data_end__;)
    {
        *pDest++ = *pSrc++;
    }

    // copy uninitialized data (.bss)
    for (register uint32_t *pDest = &__bss_start__; pDest < &__bss_end__;)
    {
        *pDest++ = 0x0UL;
    }

    // copy the mfg specific vector table to SRAM to replace application VTOR
    for (register uint32_t *dest = &__DYNAMIC_VTOR__begin, *src = (uint32_t *) mfg_test_vector_table; dest < &__DYNAMIC_VTOR__end;)
    {
        *dest++ = *src++;
    }

    // runtime patch that replace Default_Handler with mfg_default_handler
    for (register irq_handler_t * handler = (irq_handler_t *) &__DYNAMIC_VTOR__begin; handler < (irq_handler_t *) &__DYNAMIC_VTOR__end; handler++)
    {
        if (*handler == Default_Handler)
            *handler = mfg_default_handler;
    }

    // remap the exception table into SRAM to allow dynamic vector relocation.
    SCB->VTOR = (uint32_t) &__DYNAMIC_VTOR__begin & SCB_VTOR_TBLOFF_Msk;

    // initialize stack pointer
    {
        register volatile uint32_t *sp __asm ("sp");
        sp = (uint32_t *) (&__DYNAMIC_VTOR__begin)[0];
    }

    /**
     * @brief Upon this point, the application can safely use stack
     */

    // Initialise C library.
    __libc_init_array();

    // Enable Stack alignment and divide by zero trap
    SCB->CCR |= (SCB_CCR_STKALIGN_Msk | SCB_CCR_DIV_0_TRP_Msk);

    // Enable UsageFault, BusFault and MemoryFault handler
    SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk);

    // enable irq
    __enable_irq();

    // call main
    mfg_main();

    // trap here
    while (1)
    {

    }
}
