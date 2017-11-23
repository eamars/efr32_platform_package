/**
 * @file application/crt0.c
 * @brief C runtime initilization for Silicon Labs EFM32 (based on Cortex-M4 with FPU) series
 * @author Ran Bao (ran.bao@wirelessguard.co.nz)
 * @date May, 2017
 */

#include PLATFORM_HEADER
#include <stdlib.h>
#include <stdint.h>

#include "drivers/irq.h"
#include "drivers/bootloader_api/application_header.h"

#if USE_FREERTOS == 1
#include "FreeRTOSConfig.h"
#endif

/** Symbols defined by linker script.  These are all VMAs except those
    with a _load__ suffix which are LMAs.  */
extern uint32_t __load_start_data;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __copy_table_start__;
extern uint32_t __copy_table_end__;
extern uint32_t __zero_table_start__;
extern uint32_t __zero_table_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __StackTop;
extern uint32_t __CSTACK__begin;
extern uint32_t __CSTACK__end;
extern uint32_t __AAT__begin;

/*----------------------------------------------------------------------------
  Internal References
 *----------------------------------------------------------------------------*/
void Default_Handler(void);                          /* Default empty handler */
void Reset_Handler(void);                            /* Reset Handler */
extern void fault(void);
extern int main (void);
extern void _exit (int status);
extern void __libc_init_array (void);


// function alias (function symbols are required by other modules)
void halEntryPoint(void) __attribute__ ((weak, alias ("Reset_Handler")));

// Missing IRQn
volatile const uint32_t FRC_PRI_IRQn = 1;
volatile const uint32_t FRC_IRQn = 4;
volatile const uint32_t MODEM_IRQn = 5;
volatile const uint32_t RAC_SEQ_IRQn = 6;
volatile const uint32_t RAC_RSM_IRQn = 7;
volatile const uint32_t BUFC_IRQn = 8;
volatile const uint32_t AGC_IRQn = 28;
volatile const uint32_t PROTIMER_IRQn = 29;
volatile const uint32_t SYNTH_IRQn = 31;
volatile const uint32_t RFSENSE_IRQn = 33;

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Cortex-M Processor Exceptions */
void NMI_Handler         (void) __attribute__ ((weak, alias("Default_Handler")));
void HardFault_Handler   (void) __attribute__ ((weak, alias("Default_Handler")));
void MemManage_Handler   (void) __attribute__ ((weak, alias("Default_Handler")));
void BusFault_Handler    (void) __attribute__ ((weak, alias("Default_Handler")));
void UsageFault_Handler  (void) __attribute__ ((weak, alias("Default_Handler")));
void DebugMon_Handler    (void) __attribute__ ((weak, alias("Default_Handler")));
void SVC_Handler         (void) __attribute__ ((weak, alias("Default_Handler")));
void PendSV_Handler      (void) __attribute__ ((weak, alias("Default_Handler")));
void SysTick_Handler     (void) __attribute__ ((weak, alias("Default_Handler")));

/* Part Specific Interrupts */
void EMU_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void FRC_PRI_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void WDOG0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void WDOG1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void FRC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void MODEM_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void RAC_SEQ_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void RAC_RSM_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void BUFC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void LDMA_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void GPIO_EVEN_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void TIMER0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART0_RX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART0_TX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void ACMP0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void ADC0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void IDAC0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void I2C0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void GPIO_ODD_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void TIMER1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART1_RX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART1_TX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void LEUART0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void PCNT0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CMU_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void MSC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CRYPTO0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void LETIMER0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void AGC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void PROTIMER_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void RTCC_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void SYNTH_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CRYOTIMER_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void RFSENSE_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void FPUEH_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void SMU_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void WTIMER0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void WTIMER1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void PCNT1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void PCNT2_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART2_RX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART2_TX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void I2C1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART3_RX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void USART3_TX_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void VDAC0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CSEN_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void LESENSE_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void CRYPTO1_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));
void TRNG0_IRQHandler(void) __attribute__ ((weak, alias("Default_Handler")));


/* This needs to be carefully aligned.   */
__attribute__ ((section(".dynamic_vectors")))
irq_handler_t dynamic_vector_table[] =
{
        // cortex m4 interrupt vectors
        (irq_handler_t)&__StackTop,               /*      Initial Stack Pointer     */
        Reset_Handler,                            /*      Reset Handler             */
        NMI_Handler,                              /*      NMI Handler               */
        HardFault_Handler,                        /*      Hard Fault Handler        */
        MemManage_Handler,                        /*      MPU Fault Handler         */
        BusFault_Handler,                         /*      Bus Fault Handler         */
        UsageFault_Handler,                       /*      Usage Fault Handler       */
        Default_Handler,                          /*      Reserved                  */
        Default_Handler,                          /*      Reserved                  */
        Default_Handler,                          /*      Reserved                  */
        Default_Handler,                          /*      Reserved                  */
        SVC_Handler,                              /*      SVCall Handler            */
        DebugMon_Handler,                         /*      Debug Monitor Handler     */
        Default_Handler,                          /*      Reserved                  */
        PendSV_Handler,                           /*      PendSV Handler            */
        SysTick_Handler,                          /*      SysTick Handler           */

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
        Default_Handler,                          /*  50 - Reserved      */
};

/**
 * @brief Tiny loader that is aligned at 0x0, where MCU starts fetching instruction from FLASH
 *
 * The FLASH is not stable (code aligned at 0x0 has higher chance to be corrupted). Hence, a tiny loader
 * that jump to normal loader at latter location can be considered as an better choice.
 */
void loader(void);
__attribute__ ((section(".tiny_loader"))) __attribute__ ((naked))
void tiny_loader(void)
{
    __ASM volatile (
        "bl loader\n"
    );
}

/**
 * @brief Simple loader that set stack pointer and program counter prior to executing any code
 *
 * Since EFR32 will not automatically load static interrupt vector table, this function will load
 * first word from static interrupt vector table as stack pointer and branch to second word with
 * no condition.
 *
 * The function is expected to be executed as soon as the core starts fetching instructions
 * from FLASH memory.
 *
 * Note: no stack is allocated for this function
 */
__attribute__ ((section(".loader"))) __attribute__ ((naked)) __attribute__ ((optimize("-O0")))
void loader(void)
{
    /**
     * @brief Note: Both assembly and C code do the same job: load sp from application header and execute Reset_Handler
     */
#if 1
    __ASM volatile (
        "ldr r0, =%0\n"             // load absolute address from static interrupt vector table
        "ldr sp, [r0]\n"            // set stack pointer from static interrupt vector table
        "ldr r0, [r0, #4]\n"        // load address of Reset_Handler (1 word offset from SP) from static interrupt vector table
        "blx r0\n"                  // branch to Reset_Handler
        :: "i" ((uint32_t) &__AAT__begin) : "r0", "r1"
    );
#else
    register volatile uint32_t * sp __asm ("sp");

    // set stack pointer from static interrupt vector table
    sp = ((ExtendedApplicationHeaderTable_t *) &__AAT__begin)->basic_application_header_table.stack_top;

    // branch to reset handler (compiler will generate corresponding branching instruction)
    ((irq_handler_t) ((ExtendedApplicationHeaderTable_t *) &__AAT__begin)->basic_application_header_table.reset_handler)();
#endif

    // should never execute beyond this point
    while (1)
    {
        continue;
    }
}

volatile uint32_t priority;

/**
 * @brief Reset the microcontroller and copy initialized data from FLASH to SRAM
 *
 * Note: no stack is allocated for this function
 */
__attribute__ ((naked)) __attribute__ ((optimize("-O0")))
void Reset_Handler (void)
{
    // Use static vector table for handling core fault event when coping variables
    SCB->VTOR = ((uint32_t) &__AAT__begin) & SCB_VTOR_TBLOFF_Msk;

    // initialize floating point co-processor
    SystemInit();

    // disable system irq
    __disable_irq();

    // copy initialized data
    for (register uint32_t *pSrc = &__load_start_data, *pDest = &__data_start__; pDest < &__data_end__; )
    {
        *pDest++ = *pSrc++;
    }

    // copy uninitialized data
    for (register uint32_t *pDest = &__bss_start__; pDest < &__bss_end__; )
    {
        *pDest++ = 0x0UL;
    }

    // fill CSTACK with magic numbers
    for (register uint32_t *cstack = &__CSTACK__begin; cstack < &__CSTACK__end; )
    {
        *cstack++ = STACK_FILL_VALUE;
    }

    // remap the exception table into SRAM to allow dynamic vector relocation.
    SCB->VTOR = ((uint32_t) ((ExtendedApplicationHeaderTable_t *) &__AAT__begin)->basic_application_header_table.vector_table)
                & SCB_VTOR_TBLOFF_Msk;

    // initialize stack pointer
    {
        register volatile uint32_t * sp __asm ("sp");
        sp = (uint32_t *) *((ExtendedApplicationHeaderTable_t *) &__AAT__begin)->basic_application_header_table.vector_table ;
    }

    /**
     * @brief Upon this point, the application can safely use stack
     */

    // Initialise C library.
    __libc_init_array ();

    // configure priority for CORE interrupts
    NVIC_SetPriority(NonMaskableInt_IRQn, 0);
    NVIC_SetPriority(HardFault_IRQn, 0);
    NVIC_SetPriority(MemoryManagement_IRQn, 0);
    NVIC_SetPriority(BusFault_IRQn, 0);
    NVIC_SetPriority(UsageFault_IRQn, 0);
#if USE_FREERTOS == 1
    NVIC_SetPriority(SVCall_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY - 1);
#else
    NVIC_SetPriority(SVCall_IRQn, 3);
#endif
    NVIC_SetPriority(DebugMonitor_IRQn, 4);
    NVIC_SetPriority(PendSV_IRQn, 7);
    NVIC_SetPriority(SysTick_IRQn, 7);

    // configure priority for external interrupts
    NVIC_SetPriority(EMU_IRQn, 4);
    NVIC_SetPriority(FRC_PRI_IRQn, 3);
    NVIC_SetPriority(WDOG0_IRQn, 1);
    NVIC_SetPriority(WDOG1_IRQn, 1);
    NVIC_SetPriority(FRC_IRQn, 4);
    NVIC_SetPriority(MODEM_IRQn, 4);
    NVIC_SetPriority(RAC_SEQ_IRQn, 4);
    NVIC_SetPriority(RAC_RSM_IRQn, 4);
    NVIC_SetPriority(BUFC_IRQn, 4);
    NVIC_SetPriority(LDMA_IRQn, 4);
    NVIC_SetPriority(GPIO_EVEN_IRQn, 4);
    NVIC_SetPriority(TIMER0_IRQn, 4);
    NVIC_SetPriority(USART0_RX_IRQn, 4);
    NVIC_SetPriority(ACMP0_IRQn, 4);
    NVIC_SetPriority(ADC0_IRQn, 4);
    NVIC_SetPriority(IDAC0_IRQn, 4);
    NVIC_SetPriority(I2C0_IRQn, 4);
    NVIC_SetPriority(GPIO_ODD_IRQn, 4);
    NVIC_SetPriority(TIMER1_IRQn, 4);
    NVIC_SetPriority(USART1_RX_IRQn, 4);
    NVIC_SetPriority(USART1_TX_IRQn, 4);
    NVIC_SetPriority(LEUART0_IRQn, 4);
    NVIC_SetPriority(PCNT0_IRQn, 4);
    NVIC_SetPriority(CMU_IRQn, 4);
    NVIC_SetPriority(MSC_IRQn, 4);
    NVIC_SetPriority(CRYPTO0_IRQn, 4);
    NVIC_SetPriority(LETIMER0_IRQn, 4);
    NVIC_SetPriority(AGC_IRQn, 4);
    NVIC_SetPriority(PROTIMER_IRQn, 4);
    NVIC_SetPriority(RTCC_IRQn, 4);
    NVIC_SetPriority(SYNTH_IRQn, 4);
    NVIC_SetPriority(CRYOTIMER_IRQn, 4);
    NVIC_SetPriority(RFSENSE_IRQn, 4);
    NVIC_SetPriority(FPUEH_IRQn, 1);
    NVIC_SetPriority(SMU_IRQn, 4);
    NVIC_SetPriority(WTIMER0_IRQn, 4);
    NVIC_SetPriority(WTIMER1_IRQn, 4);
    NVIC_SetPriority(PCNT1_IRQn, 4);
    NVIC_SetPriority(PCNT2_IRQn, 4);
    NVIC_SetPriority(USART2_RX_IRQn, 4);
    NVIC_SetPriority(USART2_TX_IRQn, 4);
    NVIC_SetPriority(I2C1_IRQn, 4);
    NVIC_SetPriority(USART3_RX_IRQn, 4);
    NVIC_SetPriority(USART3_TX_IRQn, 4);
    NVIC_SetPriority(VDAC0_IRQn, 4);
    NVIC_SetPriority(CSEN_IRQn, 4);
    NVIC_SetPriority(LESENSE_IRQn, 4);
    NVIC_SetPriority(CRYPTO1_IRQn, 4);
    NVIC_SetPriority(TRNG0_IRQn, 4);

    // Set all bits to preempt priority group, leave 0 bit for subpriority as suggested by FreeRTOS
    NVIC_SetPriorityGrouping(0UL);

    // Enable Stack alignment and divide by zero trap
    SCB->CCR |= (SCB_CCR_STKALIGN_Msk | SCB_CCR_DIV_0_TRP_Msk);

    // Enable UsageFault, BusFault and MemoryFault handler
    SCB->SHCSR |= (SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk);

    // enable irq
    __enable_irq();

    // call main routine
    // main is not expect to return anything
    main();

    /* Hang.  */
    while (1)
    {
        continue;
    }
}


void Default_Handler(void)
{
    // TODO: error message should be written to a protected region in the SRAM to avoid any stack corruption
    ICSR_t icsr;
    int32_t active_irq = 0;

    icsr.ICSR = SCB->ICSR;
    active_irq = icsr.VECTACTIVE - 16;

    _exit(-1);
}
