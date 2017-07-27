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
*                                     NOR QUAD SPI CONTROLLER BSP
*
*                                              TEMPLATE
*
* File : bsp_fs_nor_quad_spi.c
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

#if (defined(RTOS_MODULE_FS_AVAIL) && defined(RTOS_MODULE_FS_STORAGE_NOR_AVAIL))

#include  <rtos/fs/include/fs_nor_quad_spi.h>


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


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  REG_BASE_ADDR      0x00000000


/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MACROS
*********************************************************************************************************
*********************************************************************************************************
*/

#define  TEMPLATE_QUAD_SPI_CTRLR_INFO_INIT(p_ctrlr_gen_ext_hw_info, p_bsp_api, align_req) \
             { \
                 .QuadSpiCtrlrInfo.AlignReq    = align_req, \
                 .MspApiPtr                    = p_bsp_api \
             };


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/
typedef  struct  template_quad_spi_msp_api  TEMPLATE_QUAD_SPI_MSP_API;

typedef  struct  template_quad_spi_ctrlr_info {
    struct quad_spi_ctrlr_info   QuadSpiCtrlrInfo;
    const TEMPLATE_QUAD_SPI_MSP_API   *MspApiPtr;
} TEMPLATE_QUAD_SPI_CTRLR_INFO;

struct  template_quad_spi_msp_api {
    void         (*Init)        (const TEMPLATE_QUAD_SPI_CTRLR_INFO  *p_ctrlr_info,
                                       RTOS_ERR                *p_err);
    CPU_INT32U   (*ClkFreqGet)  (const TEMPLATE_QUAD_SPI_CTRLR_INFO  *p_ctrlr_info,
                                       RTOS_ERR                *p_err);
};


/*
*********************************************************************************************************
*********************************************************************************************************
*                                        LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void        BSP_FS_NOR_QuadSPI_Init      (const TEMPLATE_QUAD_SPI_CTRLR_INFO  *p_ctrlr_info,
                                                        RTOS_ERR                      *p_err);

static  CPU_INT32U  BSP_FS_NOR_QuadSPI_ClkFreqGet(const TEMPLATE_QUAD_SPI_CTRLR_INFO  *p_ctrlr_info,
                                                        RTOS_ERR                      *p_err);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

static const TEMPLATE_QUAD_SPI_MSP_API  QuadSPI_Ctrlr_MSP_API = { .Init       = BSP_FS_NOR_QuadSPI_Init,
                                                                  .ClkFreqGet = BSP_FS_NOR_QuadSPI_ClkFreqGet };


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

const TEMPLATE_QUAD_SPI_CTRLR_INFO  BSP_QuadSPI_HwInfo = TEMPLATE_QUAD_SPI_CTRLR_INFO_INIT( REG_BASE_ADDR,
                                                                                           &QuadSPI_Ctrlr_MSP_API,
                                                                                            sizeof(CPU_ALIGN));


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      BSP_FS_NOR_QuadSPI_Init()
*
* Description : MCU-specific quad SPI controller initialization (dummy).
*
* Argument(s) : p_ctrlr_info    Pointer to a quad SPI controller descriptor.
*
*               p_err           Error pointer.
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static void  BSP_FS_NOR_QuadSPI_Init (const  TEMPLATE_QUAD_SPI_CTRLR_INFO  *p_ctrlr_info,
                                             RTOS_ERR                *p_err)
{
    /* TODO: Add initialization steps related to QSPI controller (e.g. clock, IO settings, etc.) */
	
    PP_UNUSED_PARAM(p_ctrlr_info);
	RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                    BSP_FS_NOR_QuadSPI_ClkFreqGet()
*
* Description : Get input clock frequency of QSPI controller.
*
* Argument(s) : p_ctrlr_info    Pointer to a quad SPI controller descriptor.
*
*               p_err           Error pointer.
*
* Return(s)   : AHB clock.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static CPU_INT32U  BSP_FS_NOR_QuadSPI_ClkFreqGet (const  TEMPLATE_QUAD_SPI_CTRLR_INFO  *p_ctrlr_info,
                                                         RTOS_ERR                *p_err)
{
    /* TODO: Retrieve input clock frequency.*/
	
    PP_UNUSED_PARAM(p_ctrlr_info);
	RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    return (0u);
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                   DEPENDENCIES & AVAIL CHECK(S) END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* RTOS_MODULE_FS_AVAIL && RTOS_MODULE_FS_STORAGE_NOR_AVAIL*/
