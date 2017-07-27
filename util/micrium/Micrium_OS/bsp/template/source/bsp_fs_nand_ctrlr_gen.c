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
*                                     NAND MEMORY CONTROLLER BSP
*
*                                              TEMPLATE
*
* File : bsp_fs_nand_ctrlr_gen.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                     DEPENDENCIES & AVAIL CHECK(S)
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/rtos_path.h>
#include  <rtos_description.h>

#if  defined(RTOS_MODULE_FS_AVAIL) && defined(RTOS_MODULE_FS_STORAGE_NAND_AVAIL)


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/cpu/include/cpu.h>
#include  <rtos/common/include/lib_def.h>
#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/source/rtos/rtos_utils_priv.h>

#include  <rtos/fs/include/fs_nand.h>
#include  <rtos/fs/include/fs_nand_ctrlr_gen.h>
#include  <rtos/fs/include/fs_nand_ctrlr_gen_ext_soft_ecc.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  RTOS_MODULE_CUR            RTOS_CFG_MODULE_BSP


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void  BSP_FS_NAND_Open         (RTOS_ERR      *p_err);

static  void  BSP_FS_NAND_Close        (void);

static  void  BSP_FS_NAND_ChipSelEn    (void);

static  void  BSP_FS_NAND_ChipSelDis   (void);

static  void  BSP_FS_NAND_CmdWr        (CPU_INT08U    *p_cmd,
                                        CPU_SIZE_T     cnt,
                                        RTOS_ERR      *p_err);

static  void  BSP_FS_NAND_AddrWr       (CPU_INT08U    *p_addr,
                                        CPU_SIZE_T     cnt,
                                        RTOS_ERR      *p_err);

static  void  BSP_FS_NAND_DataWr       (void          *p_src,
                                        CPU_SIZE_T     cnt,
                                        CPU_INT08U     bus_width,
                                        RTOS_ERR      *p_err);

static  void  BSP_FS_NAND_DataRd       (void          *p_dest,
                                        CPU_SIZE_T     cnt,
                                        CPU_INT08U     bus_width,
                                        RTOS_ERR      *p_err);

static  void  BSP_FS_NAND_WaitWhileBusy(void          *poll_fnct_arg,
                                        CPU_BOOLEAN  (*poll_fnct)(void  *p_arg),
                                        CPU_INT32U     to_us,
                                        RTOS_ERR      *p_err);


/*
*********************************************************************************************************
*                                         INTERFACE STRUCTURE
*********************************************************************************************************
*/

static const  FS_NAND_CTRLR_GEN_BSP_API  BSP_FS_NAND_BSP_API = {
    .Open          = BSP_FS_NAND_Open,
    .Close         = BSP_FS_NAND_Close,
    .ChipSelEn     = BSP_FS_NAND_ChipSelEn,
    .ChipSelDis    = BSP_FS_NAND_ChipSelDis,
    .CmdWr         = BSP_FS_NAND_CmdWr,
    .AddrWr        = BSP_FS_NAND_AddrWr,
    .DataWr        = BSP_FS_NAND_DataWr,
    .DataRd        = BSP_FS_NAND_DataRd,
    .WaitWhileBusy = BSP_FS_NAND_WaitWhileBusy
};
                                                                /* TODO: define the free spare map specific to the NAND memory chip.*/
static const FS_NAND_FREE_SPARE_DATA     BSP_FS_NAND_FreeSpareMapTbl[] = {{0x04u,  4u},
                                                                          {0x14u,  4u},
                                                                          {0x24u,  4u},
                                                                          {0x34u,  4u},
                                                                          {  -1 , -1 }};
                                                                /* NAND controller hardware info.                       */
static const  FS_NAND_CTRLR_GEN_HW_INFO  BSP_FS_NAND_CtrlrGen_HwInfo    = FS_NAND_CTRLR_GEN_HW_INFO_INIT(&FS_NAND_CtrlrGen_SoftEcc_Hamming_HwInfo,
                                                                                                         &BSP_FS_NAND_BSP_API,
                                                                                                         sizeof(CPU_ALIGN));
                                                                /* NAND memory chip hardware info.                      */
static const  FS_NAND_PART_HW_INFO       BSP_FS_NAND_Part_HwInfo        = FS_NAND_PART_HW_INFO_INIT(DEF_NULL,
                                                                                                    BSP_FS_NAND_FreeSpareMapTbl);
                                                                /* NAND info composed of NAND controller and memory chip*/
       const  FS_NAND_HW_INFO            BSP_FS_NAND_HwInfo             = FS_NAND_HW_INFO_INIT(&BSP_FS_NAND_CtrlrGen_HwInfo,
                                                                                               &BSP_FS_NAND_Part_HwInfo);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                           BSP_FS_NAND_Open()
*
* Description : Open (initialize) bus for NAND.
*
* Argument(s) : p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RTOS_ERR_NONE            No error.
*
* Return(s)   : None.
*
* Note(s)     : (1) This function will be called EVERY time the device is opened.
*********************************************************************************************************
*/

static  void  BSP_FS_NAND_Open (RTOS_ERR  *p_err)
{
    /* TODO: Add open instructions related to NAND controller.*/
	
	RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                        BSP_FS_NAND_Close()
*
* Description : Close (uninitialize) bus for NAND.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : (1) This function will be called EVERY time the device is closed.
*********************************************************************************************************
*/

static  void  BSP_FS_NAND_Close (void)
{
	/* TODO: Add close instructions related to NAND controller.*/
}


/*
*********************************************************************************************************
*                                        BSP_FS_NAND_ChipSelEn()
*
* Description : Enable NAND chip select/enable.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : (1) If you are sharing the bus/hardware with other software, this function MUST get an
*                   exclusive resource lock and configure any peripheral that could have been setup
*                   differently by the other software.
*********************************************************************************************************
*/

static  void  BSP_FS_NAND_ChipSelEn (void)
{

    /* TODO: Add instructions to enable the NAND chip select.*/
}


/*
*********************************************************************************************************
*                                      BSP_FS_NAND_ChipSelDis()
*
* Description : Disable NAND chip select/enable.
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : (1) If you are sharing the bus/hardware with other software, this function MUST release
*                   the exclusive resource lock obtained in function FS_NAND_BSP_ChipSelEn().
*********************************************************************************************************
*/

static  void  BSP_FS_NAND_ChipSelDis (void)
{

    /* TODO: Add instructions to disable the NAND chip select.*/
}


/*
*********************************************************************************************************
*                                          BSP_FS_NAND_CmdWr()
*
* Description : Write command to NAND.
*
* Argument(s) : p_cmd   Pointer to buffer that holds command.
*
*               cnt     Number of octets to write.
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RTOS_ERR_NONE            No error.
*
* Return(s)   : None.
*
* Note(s)     : (1) This function must send 'cnt' octets on the data bus of the NAND. The pins of this
*                   bus are usually labeled IO0_0 - IO7_0 or DQ0_0 - DQ7_0 in the datasheet of the
*                   NAND flash, as specified by the ONFI 3.0 specification. The CLE (Command Latch Enable)
*                   pin must also be set high to indicate the data is an address.
*********************************************************************************************************
*/

static  void  BSP_FS_NAND_CmdWr (CPU_INT08U  *p_cmd,
                                 CPU_SIZE_T   cnt,
                                 RTOS_ERR    *p_err)
{
    /* TODO: Add instructions to send the command to be executed by the NAND chip.*/
    PP_UNUSED_PARAM(p_cmd);
    PP_UNUSED_PARAM(cnt);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          BSP_FS_NAND_AddrWr()
*
* Description : Write address to NAND.
*
* Argument(s) : p_addr  Pointer to buffer that holds address.
*
*               cnt     Number of octets to write (size of the address).
*
*               p_err   Pointer to variable that will receive the return error code from this function :
*
*                           RTOS_ERR_NONE            No error.
*
* Return(s)   : None.
*
* Note(s)     : (1) This function must send 'cnt' octets on the data bus of the NAND. The pins of this
*                   bus are usually labeled IO0_0 - IO7_0 or DQ0_0 - DQ7_0 in the datasheet of the
*                   NAND flash, as specified by the ONFI 3.0 specification. The ALE (Address Latch Enable)
*                   pin must also be set high to indicate the data is an address.
*********************************************************************************************************
*/

static void  BSP_FS_NAND_AddrWr (CPU_INT08U  *p_addr,
                                 CPU_SIZE_T   cnt,
                                 RTOS_ERR    *p_err)
{
    /* TODO: Add instructions to send the flash address to read/write data from/to NAND chip.*/
    PP_UNUSED_PARAM(p_addr);
    PP_UNUSED_PARAM(cnt);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          BSP_FS_NAND_DataWr()
*
* Description : Write data to NAND.
*
* Argument(s) : p_src       Pointer to source memory buffer.
*
*               cnt         Number of octets to write.
*
*               bus_width   Parallel bus width. Usually 8 or 16 bits.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
* Return(s)   : None.
*
* Note(s)     : (1) This function must send 'cnt' octets on the data bus of the NAND. The pins of this
*                   bus are usually labeled IO0_0 - IO7_0 or DQ0_0 - DQ7_0 in the datasheet of the
*                   NAND flash, as specified by the ONFI 3.0 specification. The CLE (Command Latch Enable)
*                   and ALE (Address Latch Enabled) pins must both be set low.
*
*               (2) When calling this function, the generic controller layer has already sent the proper
*                   address and command to the NAND Flash. The purpose of this function is only to write
*                   data at a specific address.
*********************************************************************************************************
*/

static  void  BSP_FS_NAND_DataWr (void        *p_src,
                                  CPU_SIZE_T   cnt,
                                  CPU_INT08U   bus_width,
                                  RTOS_ERR    *p_err)
{
    /* TODO: Add instructions send data to NAND chip.*/
    PP_UNUSED_PARAM(p_src);
    PP_UNUSED_PARAM(cnt);
    PP_UNUSED_PARAM(bus_width);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                          BSP_FS_NAND_DataRd()
*
* Description : Read data from NAND.
*
* Argument(s) : p_dest      Pointer to destination memory buffer.
*
*               cnt         Number of octets to read.
*
*               bus_width   Parallel bus width. Usually 8 or 16 bits.
*
*               p_err       Pointer to variable that will receive the return error code from this function :
*
*                           RTOS_ERR_NONE            No error.
*
* Return(s)   : None.
*
* Note(s)     : (1) This function must read 'cnt' octets on the data bus of the NAND. The pins of this
*                   bus are usually labeled IO0_0 - IO7_0 or DQ0_0 - DQ7_0 in the datasheet of the
*                   NAND flash, as specified by the ONFI 3.0 specification.
*
*               (2) When calling this function, the generic controller layer has already sent the proper
*                   address and command to the NAND Flash. The purpose of this function is only to read
*                   data at a specific address.
*********************************************************************************************************
*/

static  void  BSP_FS_NAND_DataRd (void        *p_dest,
                                  CPU_SIZE_T   cnt,
                                  CPU_INT08U   bus_width,
                                  RTOS_ERR    *p_err)
{
    /* TODO: Add instructions to read data sent by NAND chip.*/
    PP_UNUSED_PARAM(p_dest);
    PP_UNUSED_PARAM(cnt);
    PP_UNUSED_PARAM(bus_width);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                      BSP_FS_NAND_WaitWhileBusy()
*
* Description : Wait while NAND is busy.
*
* Argument(s) : poll_fnct_arg   Pointer to argument that must be passed to the poll_fnct, if needed.
*
*               poll_fnct       Pointer to function to poll, if there is no hardware ready/busy signal.
*
*               to_us           Timeout, in microseconds.
*
*               p_err           Pointer to variable that will receive the return error code from this function :
*
*                                   RTOS_ERR_NONE            No error.
*
* Return(s)   : None.
*
* Note(s)     : (1) This template show how to implement this function using the 'poll_fnct'. However,
*                   reading the appropriate register should be more efficient, if available. In this
*                   case, you should replace all the code in the function.
*********************************************************************************************************
*/

static  void  BSP_FS_NAND_WaitWhileBusy (void          *poll_fnct_arg,
                                         CPU_BOOLEAN  (*poll_fnct)(void  *p_arg),
                                         CPU_INT32U     to_us,
                                         RTOS_ERR      *p_err)
{
	/* TODO: Add wait while busy instructions related to NAND controller.*/
    PP_UNUSED_PARAM(poll_fnct_arg);
    PP_UNUSED_PARAM(poll_fnct);
	PP_UNUSED_PARAM(to_us);
	
	RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_FS_AVAIL && RTOS_MODULE_FS_STORAGE_NAND_AVAIL*/
