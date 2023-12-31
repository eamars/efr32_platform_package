/*
*********************************************************************************************************
*                                             EXAMPLE CODE
*********************************************************************************************************
* Licensing:
*   The licensor of this EXAMPLE CODE is Silicon Laboratories Inc.
*
*   Silicon Laboratories Inc. grants you a personal, worldwide, royalty-free, fully paid-up license to
*   use, copy, modify and distribute the EXAMPLE CODE software, or portions thereof, in any of your
*   products.
*
*   Your use of this EXAMPLE CODE is at your own risk. This EXAMPLE CODE does not come with any
*   warranties, and the licensor disclaims all implied warranties concerning performance, accuracy,
*   non-infringement, merchantability and fitness for your application.
*
*   The EXAMPLE CODE is provided "AS IS" and does not come with any support.
*
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*
*   You can contact us at: https://www.micrium.com
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            SPI MODULE BSP
*
*                                             Template BSP
*
* File : bsp_spi.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/cpu/include/cpu.h>

#include  <rtos/kernel/include/os.h>
#include  <rtos/io/include/serial.h>
#include  <rtos/drivers/io/include/serial_drv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  CPU_BOOLEAN  BSP_Serial_SPIx_BusInit       (       SERIAL_CTRLR_ISR_HANDLE_FNCT   isr_fnct,
                                                           SERIAL_DRV                    *p_ser_drv);

static  CPU_BOOLEAN  BSP_Serial_SPIx_BusClkEn      (void);

static  CPU_BOOLEAN  BSP_Serial_SPIx_BusIO_Cfg     (void);

static  CPU_BOOLEAN  BSP_Serial_SPIx_BusIntCfg     (void);

static  CPU_BOOLEAN  BSP_Serial_SPIx_BusPwrCfg     (void);

static  CPU_BOOLEAN  BSP_Serial_SPIx_BusStart      (void);

static  CPU_BOOLEAN  BSP_Serial_SPIx_BusStop       (void);

static  CPU_BOOLEAN  BSP_Serial_SPIx_BusSlaveSel   (const  SERIAL_SLAVE_INFO             *p_slave_info);

static  CPU_BOOLEAN  BSP_Serial_SPIx_BusSlaveDesel (const  SERIAL_SLAVE_INFO             *p_slave_info);

        void         BSP_Serial_SPIx_BusIntHandler (void                                 *p_arg);

static  CPU_INT32U   BSP_Serial_SPIx_ClkFreqGet    (void);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static  SERIAL_CTRLR_ISR_HANDLE_FNCT   BSP_Serial_SPIx_Bus_DrvISR_Handler;
static  SERIAL_DRV                    *BSP_Serial_SPIx_Bus_DrvPtr;


/*
*********************************************************************************************************
*                                               BUS 0
*********************************************************************************************************
*/

const  SERIAL_CTRLR_BSP_API  BSP_Serial_SPIx_Bus_API = {
    .Init           = BSP_Serial_SPIx_BusInit,
    .ClkEn          = BSP_Serial_SPIx_BusClkEn,
    .IO_Cfg         = BSP_Serial_SPIx_BusIO_Cfg,
    .IntCfg         = BSP_Serial_SPIx_BusIntCfg,
    .PwrCfg         = BSP_Serial_SPIx_BusPwrCfg,
    .Start          = BSP_Serial_SPIx_BusStart,
    .Stop           = BSP_Serial_SPIx_BusStop,
    .SlaveSel       = BSP_Serial_SPIx_BusSlaveSel,
    .SlaveDesel     = BSP_Serial_SPIx_BusSlaveDesel,
    .ClkFreqGet     = BSP_Serial_SPIx_ClkFreqGet,
    .DMA_API_Ptr    = DEF_NULL,
    .BSP_API_ExtPtr = DEF_NULL
};

SERIAL_CTRLR_DRV_INFO  BSP_Serial_SPIx_Bus_DrvInfo = {
    .HW_Info.SupportedMode =  SERIAL_CTRLR_MODE_SPI,
    .HW_Info.BaseAddr      =  0x00000000u,
    .HW_Info.InfoExtPtr    =  DEF_NULL,
    .BSP_API_Ptr           = &BSP_Serial_SPIx_Bus_API,
    .DrvAPI_Ptr            =  DEF_NULL                          /* Select the desired Serial Driver from 'serial_drv.h' */
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      BSP_Serial_SPIx_BusInit()
*
* Description : Initialize BSP.
*
* Argument(s) : isr_fnct        Driver ISR function to call.
*
*               p_ser_drv       Argument to pass to driver ISR handler.
*
* Return(s)   : DEF_OK,   If call successful.
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_Serial_SPIx_BusInit (SERIAL_CTRLR_ISR_HANDLE_FNCT   isr_fnct,
                                      SERIAL_DRV                    *p_ser_drv)
{
    BSP_Serial_SPIx_Bus_DrvISR_Handler = isr_fnct;
    BSP_Serial_SPIx_Bus_DrvPtr         = p_ser_drv;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      BSP_Serial_SPIx_BusClkEn()
*
* Description : Configures controller clock.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   If call successful.
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_Serial_SPIx_BusClkEn (void)
{
    
    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     BSP_Serial_SPIx_BusIO_Cfg()
*
* Description : Configures controller IO.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   If call successful.
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_Serial_SPIx_BusIO_Cfg (void)
{
                                                                /* ----------------- CONFIGURE IO PINS ---------------- */
    
    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     BSP_Serial_SPIx_BusIntCfg()
*
* Description : Configures controller interrupts.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   If call successful.
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_Serial_SPIx_BusIntCfg (void)
{
#if 0
    CPU_INT_SRC_HANDLER_SET_KA(/* <Int Vect> */, (CPU_FNCT_VOID)BSP_Serial_SPIx_BusIntHandler);

    CPU_IntSrcEn(/* <Int Vect> */);
#endif

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                     BSP_Serial_SPIx_BusPwrCfg()
*
* Description : Configures controller power.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   If call successful.
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_Serial_SPIx_BusPwrCfg (void)
{

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      BSP_Serial_SPIx_BusStart()
*
* Description : Starts serial bus.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   If call successful.
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_Serial_SPIx_BusStart (void)
{

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                      BSP_Serial_SPIx_BusStop()
*
* Description : Stops serial bus.
*
* Argument(s) : None.
*
* Return(s)   : DEF_OK,   If call successful.
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_Serial_SPIx_BusStop (void)
{

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                    BSP_Serial_SPIx_BusSlaveSel()
*
* Description : Selects a slave.
*
* Argument(s) : p_slave_info    Pointer to slave info, such as slave addr & active low/high flag.
*
* Return(s)   : DEF_OK,   If call successful.
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_Serial_SPIx_BusSlaveSel (const  SERIAL_SLAVE_INFO  *p_slave_info)
{

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                   BSP_Serial_SPIx_BusSlaveDesel()
*
* Description : Deselects a slave.
*
* Argument(s) : p_slave_info    Pointer to slave info, such as slave addr & active low/high flag.
*
* Return(s)   : DEF_OK,   If call successful.
*               DEF_FAIL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_BOOLEAN  BSP_Serial_SPIx_BusSlaveDesel (const  SERIAL_SLAVE_INFO  *p_slave_info)
{

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                  BSP_Serial_SPIx_BusIntHandler()
*
* Description : Receive buffer full interrupt handler.
*
* Argument(s) : p_arg           Pointer to the Argument passing to the driver's ISR Handler.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  BSP_Serial_SPIx_BusIntHandler (void  *p_arg)
{
    BSP_Serial_SPIx_Bus_DrvISR_Handler(BSP_Serial_SPIx_Bus_DrvPtr,
                                       SERIAL_CTRLR_ISR_SRC_NONE);
}


/*
*********************************************************************************************************
*                                         BSP_Serial_SPIx_ClkFreqGet()
*
* Description : Gets the Clock frequency.
*
* Argument(s) : None.
*
* Return(s)   : Clock frequency.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_INT32U  BSP_Serial_SPIx_ClkFreqGet (void)
{

    return (0u);
}
