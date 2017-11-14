/*
*********************************************************************************************************
*                                              Micrium OS
*                                                Common
*
*                             (c) Copyright 2004; Silicon Laboratories Inc.
*                                        https://www.micrium.com
*
*********************************************************************************************************
* Licensing:
*           YOUR USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS OF A MICRIUM SOFTWARE LICENSE.
*   If you are not willing to accept the terms of an appropriate Micrium Software License, you must not
*   download or use this software for any reason.
*   Information about Micrium software licensing is available at https://www.micrium.com/licensing/
*   It is your obligation to select an appropriate license based on your intended use of the Micrium OS.
*   Unless you have executed a Micrium Commercial License, your use of the Micrium OS is limited to
*   evaluation, educational or personal non-commercial uses. The Micrium OS may not be redistributed or
*   disclosed to any third party without the written consent of Silicon Laboratories Inc.
*********************************************************************************************************
* Documentation:
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*********************************************************************************************************
* Technical Support:
*   Support is available for commercially licensed users of Micrium's software. For additional
*   information on support, you can contact info@micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                           MEMORY OPERATIONS
*
* File : lib_mem.c
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/cpu/include/cpu.h>

#include  <rtos/common/include/rtos_path.h>
#include  <common_cfg.h>

#include  <rtos/common/include/rtos_err.h>
#include  <rtos/common/include/toolchains.h>
#include  <rtos/common/include/lib_mem.h>
#include  <rtos/common/include/lib_math.h>
#include  <rtos/common/include/lib_str.h>
#include  <rtos/common/include/lib_utils.h>

#include  <rtos/common/source/rtos/rtos_utils_priv.h>
#include  <rtos/common/source/kal/kal_priv.h>
#include  <rtos/common/source/logging/logging_priv.h>


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

#define  LOG_DFLT_CH                       (COMMON, LIB)
#define  RTOS_MODULE_CUR                    RTOS_CFG_MODULE_COMMON


/*
*********************************************************************************************************
*                                     DYNAMIC MEMORY POOL OPTIONS
*********************************************************************************************************
*/

#define  MEM_DYN_POOL_OPT_NONE              DEF_BIT_NONE
#define  MEM_DYN_POOL_OPT_HW                DEF_BIT_00
#define  MEM_DYN_POOL_OPT_PERSISTENT        DEF_BIT_01


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

typedef  enum  mem_seg_status {
    MEM_SEG_STATUS_NONE,
    MEM_SEG_STATUS_EXISTS,
    MEM_SEG_STATUS_OVERLAPS
} MEM_SEG_STATUS;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
#ifndef  LIB_MEM_CFG_HEAP_BASE_ADDR
CPU_INT08U   Mem_Heap[LIB_MEM_CFG_HEAP_SIZE];                   /* Mem heap data.                                       */
MEM_SEG      Mem_SegHeap = MEM_SEG_INIT("Heap", (CPU_ADDR)&Mem_Heap[0u], LIB_MEM_CFG_HEAP_SIZE, LIB_MEM_CFG_HEAP_PADDING_ALIGN);
#else
MEM_SEG      Mem_SegHeap = MEM_SEG_INIT("Heap", (CPU_ADDR)LIB_MEM_CFG_HEAP_BASE_ADDR, LIB_MEM_CFG_HEAP_SIZE, LIB_MEM_CFG_HEAP_PADDING_ALIGN);
#endif
#endif

MEM_SEG     *Mem_SegHeadPtr;                                    /* Ptr to head of seg list.                             */


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

static  void      Mem_SegCreateCritical    (const  CPU_CHAR        *p_name,
                                                   MEM_SEG         *p_seg,
                                                   CPU_ADDR         seg_base_addr,
                                                   CPU_SIZE_T       padding_align,
                                                   CPU_SIZE_T       size);

#if ((LIB_MEM_CFG_HEAP_SIZE > 0u) || \
     (RTOS_ARG_CHK_EXT_EN))
static  MEM_SEG  *Mem_SegOverlapChkCritical(       CPU_ADDR         seg_base_addr,
                                                   CPU_SIZE_T       size,
                                                   MEM_SEG_STATUS  *p_err);
#endif

static  void     *Mem_SegAllocInternal     (const  CPU_CHAR        *p_name,
                                                   MEM_SEG         *p_seg,
                                                   CPU_SIZE_T       size,
                                                   CPU_SIZE_T       align,
                                                   CPU_SIZE_T       padding_align,
                                                   CPU_SIZE_T      *p_bytes_reqd,
                                                   RTOS_ERR        *p_err);

static  void     *Mem_SegAllocExtCritical  (       MEM_SEG         *p_seg,
                                                   CPU_SIZE_T       size,
                                                   CPU_SIZE_T       align,
                                                   CPU_SIZE_T       padding_align,
                                                   CPU_SIZE_T      *p_bytes_reqd,
                                                   RTOS_ERR        *p_err);

static  void      Mem_DynPoolCreateInternal(const  CPU_CHAR        *p_name,
                                                   MEM_DYN_POOL    *p_pool,
                                                   MEM_SEG         *p_seg,
                                                   CPU_SIZE_T       blk_size,
                                                   CPU_SIZE_T       blk_align,
                                                   CPU_SIZE_T       blk_qty_init,
                                                   CPU_SIZE_T       blk_qty_max,
                                                   RTOS_ERR        *p_err);

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
static  void      Mem_SegAllocTrackCritical(const  CPU_CHAR        *p_name,
                                                   MEM_SEG         *p_seg,
                                                   CPU_SIZE_T       size,
                                                   RTOS_ERR        *p_err);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                 DEPRECATED LOCAL FUNCTIONS PROTOTYPES
*********************************************************************************************************
*********************************************************************************************************
*/

#if ((RTOS_ARG_CHK_EXT_EN) && \
     (LIB_MEM_CFG_HEAP_SIZE > 0u))
static  CPU_BOOLEAN  Mem_PoolBlkIsValidAddr(MEM_POOL  *p_pool,
                                            void      *p_mem);
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               Mem_Clr()
*
* Description : Clears the data buffer (see Note #2).
*
* Argument(s) : p_mem   Pointer to the memory buffer to clear.
*
*               size    Number of data buffer octets to clear (see Note #1).
*
* Return(s)   : None.
*
* Note(s)     : (1) Null clears are allowed (i.e. zero-length clears).
*
*                   See also 'Mem_Set()  Note #1'.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_STD_C_LIB_EN == DEF_DISABLED)
void  Mem_Clr (void        *p_mem,
               CPU_SIZE_T   size)
{
    Mem_Set(p_mem,
            0u,                                                 /* Clear data by setting each data octet to 0.          */
            size);
}
#endif


/*
*********************************************************************************************************
*                                               Mem_Set()
*
* Description : Fills the data buffer with specified data octet.
*
* Argument(s) : p_mem       Pointer to the memory buffer to fill with the specified data octet.
*
*               data_val    Data filled octet value.
*
*               size        Number of data buffer octets to fill (see Note #1).
*
* Return(s)   : None.
*
* Note(s)     : (1) Null sets are allowed (i.e. zero-length sets).
*
*               (2) For best CPU performance, this function fills the data buffer using 'CPU_ALIGN'-sized
*                   data words. Since many word-aligned processors REQUIRE that multi-octet words be
*                   accessed on word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on
*                   'CPU_ALIGN' addresses.
*
*               (3) Modulo arithmetic determines if a memory buffer starts on a 'CPU_ALIGN' address
*                   boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations are performed on integer values, so
*                   address values MUST be cast to an appropriately-sized integer value before any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_STD_C_LIB_EN == DEF_DISABLED)
void  Mem_Set (void        *p_mem,
               CPU_INT08U   data_val,
               CPU_SIZE_T   size)
{
    CPU_SIZE_T   size_rem;
    CPU_ALIGN    data_align;
    CPU_ALIGN   *p_mem_align;
    CPU_INT08U  *p_mem_08;
    CPU_DATA     mem_align_mod;
    CPU_DATA     i;


    if ((size  <  1) ||                                         /* See Note #1.                                         */
        (p_mem == DEF_NULL)) {
        return;
    }

    data_align = 0u;
    for (i = 0u; i < sizeof(CPU_ALIGN); i++) {                  /* Fill each data_align octet with data val.            */
        data_align <<=  DEF_OCTET_NBR_BITS;
        data_align  |= (CPU_ALIGN)data_val;
    }

    size_rem      =  size;                                      /* See Note #3.                                         */
    mem_align_mod = (CPU_INT08U)((CPU_ADDR)p_mem % sizeof(CPU_ALIGN));

    p_mem_08 = (CPU_INT08U *)p_mem;
    if (mem_align_mod != 0u) {                                  /* If leading octets avail,                   ...       */
        i = mem_align_mod;
        while ((size_rem > 0) &&                                /* ... start mem buf fill with leading octets ...       */
               (i        < sizeof(CPU_ALIGN ))) {               /* ... until next CPU_ALIGN word boundary.              */
           *p_mem_08++ = data_val;
            size_rem  -= sizeof(CPU_INT08U);
            i++;
        }
    }

    p_mem_align = (CPU_ALIGN *)p_mem_08;                        /* See Note #2.                                         */
    while (size_rem >= sizeof(CPU_ALIGN)) {                     /* While mem buf aligned on CPU_ALIGN word boundaries,  */
       *p_mem_align++ = data_align;                             /* ... fill mem buf with    CPU_ALIGN-sized data.       */
        size_rem     -= sizeof(CPU_ALIGN);
    }

    p_mem_08 = (CPU_INT08U *)p_mem_align;
    while (size_rem > 0) {                                      /* Finish mem buf fill with trailing octets.            */
       *p_mem_08++ = data_val;
        size_rem  -= sizeof(CPU_INT08U);
    }
}
#endif


/*
*********************************************************************************************************
*                                              Mem_Copy()
*
* Description : Copies data octets from one memory buffer to another memory buffer.
*
* Argument(s) : p_dest  Pointer to the destination memory buffer.
*
*               p_src   Pointer to the source memory buffer.
*
*               size    Number of octets to copy (see Note #1).
*
* Return(s)   : None.
*
* Note(s)     : (1) Null copies are allowed (i.e. zero-length copies).
*
*               (2) Memory buffers NOT checked for overlapping.
*
*                   (a) IEEE Std 1003.1, 2004 Edition, Section 'memcpy() : DESCRIPTION' states that "if
*                       copying takes place between objects that overlap, the behavior is undefined".
*
*                   (b) Data octets from a source memory buffer at a higher address value SHOULD
*                       successfully copy to a destination memory buffer at a lower address value. This
*                       occurs even if any octets of the memory buffers overlap, as long as there are no
*                       individual, atomic CPU word copy overlaps.
*
*                       Since Mem_Copy() copies the data octet via 'CPU_ALIGN'-sized words &/or octets,
*                       and since 'CPU_ALIGN'-sized words MUST be accessed on word-aligned addresses (see
*                       Note #3b), neither 'CPU_ALIGN'-sized words nor octets at unique addresses can
*                       ever overlap.
*
*                       Therefore, Mem_Copy() SHOULD be able to successfully copy overlapping memory
*                       buffers as long as the source memory buffer is at a higher address value than the
*                       destination memory buffer.
*
*               (3) For best CPU performance, this function copies data buffers using 'CPU_ALIGN'-sized
*                   data words. Since many word-aligned processors REQUIRE that multi-octet words be
*                   accessed on word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on
*                   'CPU_ALIGN' addresses.
*
*               (4) Modulo arithmetic determines if a memory buffer starts on a 'CPU_ALIGN' address
*                   boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIRE operations are performed on integer values, so
*                   address values MUST be cast to an appropriately-sized integer value before any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_STD_C_LIB_EN == DEF_DISABLED)
void  Mem_Copy (       void        *p_dest,
                const  void        *p_src,
                       CPU_SIZE_T   size)
{
           CPU_SIZE_T    size_rem;
           CPU_SIZE_T    mem_gap_octets;
           CPU_ALIGN    *p_mem_align_dest;
    const  CPU_ALIGN    *p_mem_align_src;
           CPU_INT08U   *p_mem_08_dest;
    const  CPU_INT08U   *p_mem_08_src;
           CPU_DATA      i;
           CPU_DATA      mem_align_mod_dest;
           CPU_DATA      mem_align_mod_src;
           CPU_BOOLEAN   mem_aligned;


    if ((size   <  1)        ||                                 /* See Note #1.                                         */
        (p_dest == DEF_NULL) ||
        (p_src  == DEF_NULL)) {
        return;
    }

    size_rem       =  size;

    p_mem_08_dest  = (      CPU_INT08U *)p_dest;
    p_mem_08_src   = (const CPU_INT08U *)p_src;

    mem_gap_octets = (CPU_SIZE_T)(p_mem_08_src - p_mem_08_dest);


    if (mem_gap_octets >= sizeof(CPU_ALIGN)) {                  /* Avoid bufs overlap.                                  */
                                                                /* See Note #4.                                         */
        mem_align_mod_dest = (CPU_INT08U)((CPU_ADDR)p_mem_08_dest % sizeof(CPU_ALIGN));
        mem_align_mod_src  = (CPU_INT08U)((CPU_ADDR)p_mem_08_src  % sizeof(CPU_ALIGN));

        mem_aligned        = (mem_align_mod_dest == mem_align_mod_src) ? DEF_YES : DEF_NO;

        if (mem_aligned == DEF_YES) {                           /* If mem bufs' alignment offset equal, ...             */
                                                                /* ... optimize copy for mem buf alignment.             */
            if (mem_align_mod_dest != 0u) {                     /* If leading octets avail,                   ...       */
                i = mem_align_mod_dest;
                while ((size_rem   >  0) &&                     /* ... start mem buf copy with leading octets ...       */
                       (i          <  sizeof(CPU_ALIGN ))) {    /* ... until next CPU_ALIGN word boundary.              */
                   *p_mem_08_dest++ = *p_mem_08_src++;
                    size_rem      -=  sizeof(CPU_INT08U);
                    i++;
                }
            }

            p_mem_align_dest = (      CPU_ALIGN *)p_mem_08_dest;/* See Note #3.                                         */
            p_mem_align_src  = (const CPU_ALIGN *)p_mem_08_src;
            while (size_rem      >=  sizeof(CPU_ALIGN)) {       /* While mem bufs aligned on CPU_ALIGN word boundaries, */
               *p_mem_align_dest++  = *p_mem_align_src++;       /* ... copy psrc to pdest with CPU_ALIGN-sized words.   */
                size_rem           -=  sizeof(CPU_ALIGN);
            }

            p_mem_08_dest = (      CPU_INT08U *)p_mem_align_dest;
            p_mem_08_src  = (const CPU_INT08U *)p_mem_align_src;
        }
    }

    while (size_rem > 0) {                                      /* For unaligned mem bufs or trailing octets, ...       */
       *p_mem_08_dest++  = *p_mem_08_src++;                     /* ... copy psrc to pdest by octets.                    */
        size_rem        -=  sizeof(CPU_INT08U);
    }
}
#endif


/*
*********************************************************************************************************
*                                              Mem_Move()
*
* Description : Moves data octets from one memory buffer to another memory buffer, or within the same
*               memory buffer. Overlapping is correctly handled for all move operations.
*
* Argument(s) : p_dest  Pointer to destination memory buffer.
*
*               p_src   Pointer to source memory buffer.
*
*               size    Number of octets to move (see Note #1).
*
* Return(s)   : None.
*
* Note(s)     : (1) Null move operations are allowed (i.e. zero-length).
*
*               (2) Memory buffers checked for overlapping.
*
*               (3) For best CPU performance, this function copies data buffer using 'CPU_ALIGN'-sized
*                   data words. Since many word-aligned processors REQUIRE that multi-octet words be
*                   accessed on word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on
*                   'CPU_ALIGN'd addresses.
*
*               (4) Modulo arithmetic determines if a memory buffer starts on a 'CPU_ALIGN' address
*                   boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values, so
*                   address values MUST be cast to an appropriately-sized integer value before any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_STD_C_LIB_EN == DEF_DISABLED)
void  Mem_Move (       void        *p_dest,
                const  void        *p_src,
                       CPU_SIZE_T   size)
{
           CPU_SIZE_T    size_rem;
           CPU_SIZE_T    mem_gap_octets;
           CPU_ALIGN    *p_mem_align_dest;
    const  CPU_ALIGN    *p_mem_align_src;
           CPU_INT08U   *p_mem_08_dest;
    const  CPU_INT08U   *p_mem_08_src;
           CPU_INT08S    i;
           CPU_DATA      mem_align_mod_dest;
           CPU_DATA      mem_align_mod_src;
           CPU_BOOLEAN   mem_aligned;


    if ((size   <  1)        ||                                 /* See Note #1.                                         */
        (p_dest == DEF_NULL) ||
        (p_src  == DEF_NULL)) {
        return;
    }

    p_mem_08_src  = (const CPU_INT08U *)p_src;
    p_mem_08_dest = (      CPU_INT08U *)p_dest;
    if (p_mem_08_src > p_mem_08_dest) {
        Mem_Copy(p_dest, p_src, size);
        return;
    }

    size_rem       =  size;

    p_mem_08_dest   = (      CPU_INT08U *)p_dest + size - 1;
    p_mem_08_src    = (const CPU_INT08U *)p_src  + size - 1;

    mem_gap_octets = (CPU_SIZE_T)(p_mem_08_dest - p_mem_08_src);


    if (mem_gap_octets >= sizeof(CPU_ALIGN)) {                  /* Avoid bufs overlap.                                  */

                                                                /* See Note #4.                                         */
        mem_align_mod_dest = (CPU_INT08U)((CPU_ADDR)p_mem_08_dest % sizeof(CPU_ALIGN));
        mem_align_mod_src  = (CPU_INT08U)((CPU_ADDR)p_mem_08_src  % sizeof(CPU_ALIGN));

        mem_aligned        = (mem_align_mod_dest == mem_align_mod_src) ? DEF_YES : DEF_NO;

        if (mem_aligned == DEF_YES) {                           /* If mem bufs' alignment offset equal, ...             */
                                                                /* ... optimize copy for mem buf alignment.             */
            if (mem_align_mod_dest != (sizeof(CPU_ALIGN) - 1)) {/* If leading octets avail,                   ...       */
                i = (CPU_INT08S)mem_align_mod_dest;
                while ((size_rem   >  0) &&                     /* ... start mem buf copy with leading octets ...       */
                       (i          >= 0)) {                     /* ... until next CPU_ALIGN word boundary.              */
                   *p_mem_08_dest--  = *p_mem_08_src--;
                    size_rem        -=  sizeof(CPU_INT08U);
                    i--;
                }
            }

                                                                /* See Note #3.                                         */
            p_mem_align_dest = (      CPU_ALIGN *)(((CPU_INT08U *)p_mem_08_dest - sizeof(CPU_ALIGN)) + 1);
            p_mem_align_src  = (const CPU_ALIGN *)(((CPU_INT08U *)p_mem_08_src  - sizeof(CPU_ALIGN)) + 1);
            while (size_rem      >=  sizeof(CPU_ALIGN)) {       /* While mem bufs aligned on CPU_ALIGN word boundaries, */
               *p_mem_align_dest--  = *p_mem_align_src--;       /* ... copy psrc to pdest with CPU_ALIGN-sized words.   */
                size_rem           -=  sizeof(CPU_ALIGN);
            }

            p_mem_08_dest = (      CPU_INT08U *)p_mem_align_dest + sizeof(CPU_ALIGN) - 1;
            p_mem_08_src  = (const CPU_INT08U *)p_mem_align_src  + sizeof(CPU_ALIGN) - 1;

        }
    }

    while (size_rem > 0) {                                      /* For unaligned mem bufs or trailing octets, ...       */
       *p_mem_08_dest--  = *p_mem_08_src--;                     /* ... copy psrc to pdest by octets.                    */
        size_rem        -=  sizeof(CPU_INT08U);
    }
}
#endif


/*
*********************************************************************************************************
*                                               Mem_Cmp()
*
* Description : Verifies that ALL data octets in the two memory buffers are identical in sequence.
*
* Argument(s) : p1_mem  Pointer to first memory buffer.
*
*               p2_mem  Pointer to second memory buffer.
*
*               size    Number of data buffer octets to compare (see Note #1).
*
* Return(s)   : DEF_YES, if 'size' number of data octets are identical in both memory buffers.
*
*               DEF_NO, otherwise.
*
* Note(s)     : (1) Null compares are allowed (i.e. zero-length compares); 'DEF_YES' is returned to
*                   indicate identical null compare.
*
*               (2) Many memory buffer comparisons vary ONLY in the least significant octets (e.g.
*                   network address buffers). Consequently, memory buffer comparison is more efficient if
*                   the comparison starts from the end of the memory buffers. This aborts sooner on
*                   dissimilar memory buffers that vary only in the least significant octets.
*
*               (3) For best CPU performance, this function compares data buffers using 'CPU_ALIGN'-sized
*                   data words. Since many word-aligned processors REQUIRE that multi-octet words be
*                   accessed on word-aligned addresses, 'CPU_ALIGN'-sized words MUST be accessed on
*                   'CPU_ALIGN'd addresses.
*
*               (4) Modulo arithmetic determines if a memory buffer starts on a 'CPU_ALIGN' address
*                   boundary.
*
*                   Modulo arithmetic in ANSI-C REQUIREs operations performed on integer values, so
*                   address values MUST be cast to an appropriately-sized integer value before any
*                  'mem_align_mod' arithmetic operation.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_STD_C_LIB_EN == DEF_DISABLED)
CPU_BOOLEAN  Mem_Cmp (const  void        *p1_mem,
                      const  void        *p2_mem,
                             CPU_SIZE_T   size)
{
           CPU_SIZE_T    size_rem;
           CPU_ALIGN    *p1_mem_align;
           CPU_ALIGN    *p2_mem_align;
    const  CPU_INT08U   *p1_mem_08;
    const  CPU_INT08U   *p2_mem_08;
           CPU_DATA      i;
           CPU_DATA      mem_align_mod_1;
           CPU_DATA      mem_align_mod_2;
           CPU_BOOLEAN   mem_aligned;
           CPU_BOOLEAN   mem_cmp;


    if ((size   <  1)        ||                                 /* See Note #1.                                         */
        (p1_mem == DEF_NULL) ||
        (p2_mem == DEF_NULL)) {
        return (DEF_NO);
    }

    mem_cmp         =  DEF_YES;                                 /* Assume mem bufs are identical until cmp fails.       */
    size_rem        =  size;
                                                                /* Start @ end of mem bufs (see Note #2).               */
    p1_mem_08       = (const CPU_INT08U *)p1_mem + size;
    p2_mem_08       = (const CPU_INT08U *)p2_mem + size;
                                                                /* See Note #4.                                         */
    mem_align_mod_1 = (CPU_INT08U)((CPU_ADDR)p1_mem_08 % sizeof(CPU_ALIGN));
    mem_align_mod_2 = (CPU_INT08U)((CPU_ADDR)p2_mem_08 % sizeof(CPU_ALIGN));

    mem_aligned     = (mem_align_mod_1 == mem_align_mod_2) ? DEF_YES : DEF_NO;

    if (mem_aligned == DEF_YES) {                               /* If mem bufs' alignment offset equal, ...             */
                                                                /* ... optimize cmp for mem buf alignment.              */
        if (mem_align_mod_1 != 0u) {                            /* If trailing octets avail,                  ...       */
            i = mem_align_mod_1;
            while ((mem_cmp == DEF_YES) &&                      /* ... cmp mem bufs while identical &         ...       */
                   (size_rem > 0)       &&                      /* ... start mem buf cmp with trailing octets ...       */
                   (i        > 0)) {                            /* ... until next CPU_ALIGN word boundary.              */
                p1_mem_08--;
                p2_mem_08--;
                if (*p1_mem_08 != *p2_mem_08) {                 /* If ANY data octet(s) NOT identical, cmp fails.       */
                     mem_cmp = DEF_NO;
                }
                size_rem -= sizeof(CPU_INT08U);
                i--;
            }
        }

        if (mem_cmp == DEF_YES) {                               /* If cmp still identical, cmp aligned mem bufs.        */
            p1_mem_align = (CPU_ALIGN *)p1_mem_08;              /* See Note #3.                                         */
            p2_mem_align = (CPU_ALIGN *)p2_mem_08;

            while ((mem_cmp  == DEF_YES) &&                     /* Cmp mem bufs while identical & ...                   */
                   (size_rem >= sizeof(CPU_ALIGN))) {           /* ... mem bufs aligned on CPU_ALIGN word boundaries.   */
                p1_mem_align--;
                p2_mem_align--;
                if (*p1_mem_align != *p2_mem_align) {           /* If ANY data octet(s) NOT identical, cmp fails.       */
                     mem_cmp = DEF_NO;
                }
                size_rem -= sizeof(CPU_ALIGN);
            }

            p1_mem_08 = (CPU_INT08U *)p1_mem_align;
            p2_mem_08 = (CPU_INT08U *)p2_mem_align;
        }
    }

    while ((mem_cmp == DEF_YES) &&                              /* Cmp mem bufs while identical ...                     */
           (size_rem > 0)) {                                    /* ... for unaligned mem bufs or trailing octets.       */
        p1_mem_08--;
        p2_mem_08--;
        if (*p1_mem_08 != *p2_mem_08) {                         /* If ANY data octet(s) NOT identical, cmp fails.       */
             mem_cmp = DEF_NO;
        }
        size_rem -= sizeof(CPU_INT08U);
    }

    return (mem_cmp);
}
#endif


/*
*********************************************************************************************************
*                                            Mem_SegCreate()
*
* Description : Creates a new memory segment to be used for runtime memory allocation.
*
* Argument(s) : p_name          Pointer to segment name.
*
*               p_seg           Pointer to segment data. Must be allocated by caller.
*
*               seg_base_addr   Address of segment's first byte.
*
*               size            Total size of segment (in bytes).
*
*               padding_align   Padding alignment (in bytes) that will be added to any allocated buffer
*                               from this memory segment. MUST be a power of 2.
*                               LIB_MEM_PADDING_ALIGN_NONE means no padding.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_INVALID_ARG
*
* Return(s)   : None.
*
* Note(s)     : (1) New segments are checked for overlap with existing segments. A critical section must
*                   be maintained during the whole list search and adds a procedure to prevent a
*                   re-entrant call from creating another segment that would overlaps with the new one.
*********************************************************************************************************
*/

void  Mem_SegCreate (const  CPU_CHAR    *p_name,
                            MEM_SEG     *p_seg,
                            CPU_ADDR     seg_base_addr,
                            CPU_SIZE_T   size,
                            CPU_SIZE_T   padding_align,
                            RTOS_ERR    *p_err)
{
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for null seg ptr.                                */
    RTOS_ASSERT_DBG_ERR_SET((p_seg != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

                                                                /* Chk for invalid sized seg.                           */
    RTOS_ASSERT_DBG_ERR_SET((size >= 1u), *p_err, RTOS_ERR_INVALID_ARG, ;);
                                                                /* Chk for addr space ovf.                              */
    RTOS_ASSERT_DBG_ERR_SET((seg_base_addr + (size - 1u) >= seg_base_addr), *p_err, RTOS_ERR_INVALID_ARG, ;);

    RTOS_ASSERT_DBG_ERR_SET(((padding_align == LIB_MEM_PADDING_ALIGN_NONE) ||
                             (padding_align == LIB_MEM_BUF_ALIGN_AUTO) ||
                             (MATH_IS_PWR2(padding_align) == DEF_YES)), *p_err, RTOS_ERR_INVALID_ARG, ;);

    CPU_CRITICAL_ENTER();
#if RTOS_ARG_CHK_EXT_EN
    {
        MEM_SEG_STATUS  status;


        (void)Mem_SegOverlapChkCritical(seg_base_addr,          /* Chk for overlap.                                     */
                                        size,
                                       &status);
        if (status != MEM_SEG_STATUS_NONE) {
            CPU_CRITICAL_EXIT();
            RTOS_ERR_SET(*p_err, RTOS_ERR_INVALID_ARG);
            return;
        }
    }
#endif

    Mem_SegCreateCritical(p_name,                               /* Create seg.                                          */
                          p_seg,
                          seg_base_addr,
                          padding_align,
                          size);
    CPU_CRITICAL_EXIT();

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                             Mem_SegReg()
*
* Description : Registers a memory segment that was created at compile-time to enable both usage output
*               and overlap checks when creating a new memory segment.
*
* Argument(s) : p_seg   Pointer to segment data already created.
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_ALREADY_EXISTS
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  Mem_SegReg (MEM_SEG   *p_seg,
                  RTOS_ERR  *p_err)
{
    MEM_SEG  *p_seg_loop;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for null seg ptr.                                */
    RTOS_ASSERT_DBG_ERR_SET((p_seg != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

    CPU_CRITICAL_ENTER();
    p_seg_loop = Mem_SegHeadPtr;
    while (p_seg_loop != DEF_NULL) {

        if (p_seg_loop == p_seg) {                              /* Check if seg already in list.                        */
            CPU_CRITICAL_EXIT();
            RTOS_ERR_SET(*p_err, RTOS_ERR_ALREADY_EXISTS);
            return;
        }

        p_seg_loop = p_seg_loop->NextPtr;
    }

    p_seg->NextPtr = Mem_SegHeadPtr;                            /* Add segment at head of list.                         */
    Mem_SegHeadPtr = p_seg;
    CPU_CRITICAL_EXIT();

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                             Mem_SegClr()
*
* Description : Clears a memory segment.
*
* Argument(s) : p_seg   Pointer to segment data. Must be allocated by caller.
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                           RTOS_ERR_NONE
*
* Return(s)   : None.
*
* Note(s)     : (1) Use this function with extreme caution. It must only be called on memory segments
*                   that are no longer used.
*
*               (2) This function is disabled when debug mode is enabled to avoid heap memory leaks.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_DISABLED)
void  Mem_SegClr (MEM_SEG   *p_seg,
                  RTOS_ERR  *p_err)
{
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for null seg ptr.                                */
    RTOS_ASSERT_DBG_ERR_SET((p_seg != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

    CPU_CRITICAL_ENTER();
    p_seg->AddrNext = p_seg->AddrBase;
    CPU_CRITICAL_EXIT();

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}
#endif


/*
*********************************************************************************************************
*                                          Mem_SegRemSizeGet()
*
* Description : Calculates the remaining free space in the memory segment.
*
* Argument(s) : p_seg       Pointer to segment data.
*
*               align       Alignment in bytes to assume for calculation of free space.
*                           LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               p_seg_info  Pointer to structure that will receive further segment info data (used size,
*                           total size, base address, and next allocation address).
*
*               p_err       Pointer to the variable that will receive one of the following error code(s)
*                           from this function:
*
*                               RTOS_ERR_NONE
*
* Return(s)   : The amount of free space in the memory segment (bytes), if successful.
*               0, otherwise or if the memory segment empty.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_SIZE_T  Mem_SegRemSizeGet (MEM_SEG       *p_seg,
                               CPU_SIZE_T     align,
                               MEM_SEG_INFO  *p_seg_info,
                               RTOS_ERR      *p_err)
{
    CPU_SIZE_T   rem_size;
    CPU_SIZE_T   total_size;
    CPU_SIZE_T   used_size;
    CPU_SIZE_T   next_buf_align;
    CPU_ADDR     next_addr_align;
    MEM_SEG     *p_seg_valid     = p_seg;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, 0);

                                                                /* Chk for invalid align val.                           */
    RTOS_ASSERT_DBG_ERR_SET((MATH_IS_PWR2(align) == DEF_YES) ||
                            (align               == LIB_MEM_BUF_ALIGN_AUTO), *p_err, RTOS_ERR_INVALID_ARG, 0);

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    if (p_seg_valid == DEF_NULL) {                              /* Dflt to heap in case p_seg is null.                  */
        p_seg_valid = &Mem_SegHeap;
    }
#else
    RTOS_ASSERT_DBG_ERR_SET((p_seg_valid != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, 0);
#endif

    next_buf_align = (align == LIB_MEM_BUF_ALIGN_AUTO) ? CPU_MIN_DATA_ALIGN_BYTES() : align;

    CPU_CRITICAL_ENTER();                                       /* Calc seg stats.                                      */
    next_addr_align = MATH_ROUND_INC_UP_PWR2(p_seg_valid->AddrNext, next_buf_align);
    CPU_CRITICAL_EXIT();

    total_size = (p_seg_valid->AddrEnd  - p_seg_valid->AddrBase) + 1u;
    used_size  =  p_seg_valid->AddrNext - p_seg_valid->AddrBase;

    if (next_addr_align > p_seg_valid->AddrEnd){
        next_addr_align = 0u;
        rem_size        = 0u;
    } else {
        rem_size        = total_size - (next_addr_align - p_seg_valid->AddrBase);
    }

    if (p_seg_info != DEF_NULL) {
        p_seg_info->TotalSize     = total_size;
        p_seg_info->UsedSize      = used_size;
        p_seg_info->AddrBase      = p_seg_valid->AddrBase;
        p_seg_info->AddrNextAlloc = next_addr_align;
    }

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (rem_size);
}


/*
*********************************************************************************************************
*                                            Mem_SegAlloc()
*
* Description : Allocates memory from a specified segment. The returned memory block will be aligned on a
*               CPU word boundary.
*
* Argument(s) : p_name  Pointer to the allocated object name. Used to track allocations. May be DEF_NULL.
*
*               p_seg   Pointer to the segment from which to allocate memory. If NULL, it will allocate
*                       from the general-purpose heap.
*
*               size    Size of memory block to allocate (in bytes).
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_SEG_OVF
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Note(s)     : (1) The memory block returned by this function will be aligned on a word boundary.
*                   To specify an alignment value, use either Mem_SegAllocExt() or Mem_SegAllocHW().
*********************************************************************************************************
*/

void  *Mem_SegAlloc (const  CPU_CHAR    *p_name,
                            MEM_SEG     *p_seg,
                            CPU_SIZE_T   size,
                            RTOS_ERR    *p_err)
{
    void     *p_blk;
    MEM_SEG  *p_seg_valid = p_seg;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_NULL);

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    if (p_seg_valid == DEF_NULL) {                              /* Dflt to heap in case p_seg is null.                  */
        p_seg_valid = &Mem_SegHeap;
    }
#else
    RTOS_ASSERT_DBG_ERR_SET((p_seg_valid != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_NULL);
#endif

    p_blk = Mem_SegAllocInternal(p_name,
                                 p_seg_valid,
                                 size,
                                 sizeof(CPU_ALIGN),
                                 LIB_MEM_PADDING_ALIGN_NONE,
                                 DEF_NULL,
                                 p_err);

    return (p_blk);
}


/*
*********************************************************************************************************
*                                           Mem_SegAllocExt()
*
* Description : Allocates memory from specified memory segment.
*
* Argument(s) : p_name          Pointer to the allocated object name. Used to track allocations. May be
*                               DEF_NULL.
*
*               p_seg           Pointer to segment from which to allocate memory. If NULL, it will
*                               allocate from the general-purpose heap.
*
*               size            Size of memory block to allocate (in bytes).
*
*               align           Required alignment of memory block (in bytes). MUST be a power of 2.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               p_bytes_reqd    Pointer to a variable that will receive the number of free bytes missing
*                               for the allocation to succeed. Set to DEF_NULL to skip calculation.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  *Mem_SegAllocExt (const  CPU_CHAR    *p_name,
                               MEM_SEG     *p_seg,
                               CPU_SIZE_T   size,
                               CPU_SIZE_T   align,
                               CPU_SIZE_T  *p_bytes_reqd,
                               RTOS_ERR    *p_err)
{
    void     *p_blk;
    MEM_SEG  *p_seg_valid = p_seg;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_NULL);

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    if (p_seg_valid == DEF_NULL) {                              /* Dflt to heap in case p_seg is null.                  */
        p_seg_valid = &Mem_SegHeap;
    }
#else
    RTOS_ASSERT_DBG_ERR_SET((p_seg_valid != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_NULL);
#endif

    p_blk = Mem_SegAllocInternal(p_name,
                                 p_seg_valid,
                                 size,
                                 align,
                                 LIB_MEM_PADDING_ALIGN_NONE,
                                 p_bytes_reqd,
                                 p_err);

    return (p_blk);
}


/*
*********************************************************************************************************
*                                           Mem_SegAllocHW()
*
* Description : Allocates memory from specified segment. The returned buffer will be padded in function
*               of memory segment's properties.
*
* Argument(s) : p_name          Pointer to allocated object name. Used to track allocations. May be
*                               DEF_NULL.
*
*               p_seg           Pointer to segment from which to allocate memory. If NULL, it will
*                               allocate from the general-purpose heap.
*
*               size            Size of memory block to allocate (in bytes).
*
*               align           Required alignment of memory block (in bytes). MUST be a power of 2.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               p_bytes_reqd    Pointer to a variable that will receive the number of free bytes missing
*                               for the allocation to succeed. Set to DEF_NULL to skip calculation.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  *Mem_SegAllocHW (const  CPU_CHAR    *p_name,
                              MEM_SEG     *p_seg,
                              CPU_SIZE_T   size,
                              CPU_SIZE_T   align,
                              CPU_SIZE_T  *p_bytes_reqd,
                              RTOS_ERR    *p_err)
{
    void     *p_blk;
    MEM_SEG  *p_seg_valid = p_seg;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_NULL);

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    if (p_seg_valid == DEF_NULL) {                              /* Dflt to heap in case p_seg is null.                  */
        p_seg_valid = &Mem_SegHeap;
    }
#else
    RTOS_ASSERT_DBG_ERR_SET((p_seg_valid != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_NULL);
#endif

    p_blk = Mem_SegAllocInternal(p_name,
                                 p_seg_valid,
                                 size,
                                 align,
                                 p_seg_valid->PaddingAlign,
                                 p_bytes_reqd,
                                 p_err);

    return (p_blk);
}


/*
*********************************************************************************************************
*                                          Mem_DynPoolCreate()
*
* Description : Creates a dynamic memory pool.
*
* Argument(s) : p_name          Pointer to the pool name.
*
*               p_pool          Pointer to the pool data.
*
*               p_seg           Pointer to segment from which to allocate memory. If NULL, it will be
*                               allocated from the general-purpose heap.
*
*               blk_size        Size of memory block to allocate from pool (in bytes). See Note #1.
*
*               blk_align       Required alignment of memory block (in bytes). MUST be a power of 2.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               blk_qty_init    Initial number of elements to be allocated in pool.
*
*               blk_qty_max     Maximum number of elements that can be allocated from this pool. Set to
*                               LIB_MEM_BLK_QTY_UNLIMITED if there is no limit.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_BLK_ALLOC_CALLBACK
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : None.
*
* Note(s)     : (1) 'blk_size' must be big enough to fit a pointer since the pointer to the next free
*                   block is stored in the block itself (only when free/unused).
*********************************************************************************************************
*/

void  Mem_DynPoolCreate (const  CPU_CHAR      *p_name,
                                MEM_DYN_POOL  *p_pool,
                                MEM_SEG       *p_seg,
                                CPU_SIZE_T     blk_size,
                                CPU_SIZE_T     blk_align,
                                CPU_SIZE_T     blk_qty_init,
                                CPU_SIZE_T     blk_qty_max,
                                RTOS_ERR      *p_err)
{
    MEM_SEG  *p_seg_valid = p_seg;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    if (p_seg_valid == DEF_NULL) {                              /* Dflt to heap in case p_seg is null.                  */
        p_seg_valid = &Mem_SegHeap;
    }
#else
    RTOS_ASSERT_DBG_ERR_SET((p_seg_valid != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);
#endif

    p_pool->Opt          = MEM_DYN_POOL_OPT_NONE;
    p_pool->AllocFnct    = DEF_NULL;
    p_pool->AllocFnctArg = DEF_NULL;

    Mem_DynPoolCreateInternal(p_name,
                              p_pool,
                              p_seg_valid,
                              blk_size,
                              blk_align,
                              blk_qty_init,
                              blk_qty_max,
                              p_err);
}


/*
*********************************************************************************************************
*                                     Mem_DynPoolCreatePersistent()
*
* Description : Creates a persistent dynamic memory pool.
*
* Argument(s) : p_name          Pointer to the pool name.
*
*               p_pool          Pointer to the pool data.
*
*               p_seg           Pointer to segment from which to allocate memory.
*
*               blk_size        Size of memory block to allocate from pool (in bytes). See Note #1.
*
*               blk_align       Required alignment of memory block (in bytes). MUST be a power of 2.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               blk_qty_init    Initial number of elements to be allocated in pool.
*
*               blk_qty_max     Maximum number of elements that can be allocated from this pool. Set to
*                               LIB_MEM_BLK_QTY_UNLIMITED if there is no limit.
*
*               alloc_callback  Function that will be called the first time each block is allocated.
*
*               p_callback_arg  Pointer to argument that will be passed to callback.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_BLK_ALLOC_CALLBACK
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : None.
*
* Note(s)     : (1) 'blk_size' must be big enough to fit a pointer since the pointer to the next free
*                   block is stored in the block itself (only when free/unused).
*********************************************************************************************************
*/

void  Mem_DynPoolCreatePersistent (const  CPU_CHAR                 *p_name,
                                          MEM_DYN_POOL             *p_pool,
                                          MEM_SEG                  *p_seg,
                                          CPU_SIZE_T                blk_size,
                                          CPU_SIZE_T                blk_align,
                                          CPU_SIZE_T                blk_qty_init,
                                          CPU_SIZE_T                blk_qty_max,
                                          MEM_DYN_POOL_ALLOC_FNCT   alloc_callback,
                                          void                     *p_callback_arg,
                                          RTOS_ERR                 *p_err)
{
    MEM_SEG  *p_seg_valid = p_seg;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    if (p_seg_valid == DEF_NULL) {                              /* Dflt to heap in case p_seg is null.                  */
        p_seg_valid = &Mem_SegHeap;
    }
#else
    RTOS_ASSERT_DBG_ERR_SET((p_seg_valid != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);
#endif

    p_pool->Opt          = MEM_DYN_POOL_OPT_PERSISTENT;
    p_pool->AllocFnct    = alloc_callback;
    p_pool->AllocFnctArg = p_callback_arg;

    Mem_DynPoolCreateInternal(p_name,
                              p_pool,
                              p_seg_valid,
                              blk_size,
                              blk_align,
                              blk_qty_init,
                              blk_qty_max,
                              p_err);
}


/*
*********************************************************************************************************
*                                         Mem_DynPoolCreateHW()
*
* Description : Creates a dynamic memory pool. Memory blocks will be padded according to memory segment's
*               properties.
*
* Argument(s) : p_name          Pointer to the pool name.
*
*               p_pool          Pointer to the pool data.
*
*               p_seg           Pointer to the segment from which to allocate memory. If NULL, it will be
*                               allocated from the general-purpose heap.
*
*               blk_size        Size of memory block to allocate from pool (in bytes). See Note #1.
*
*               blk_align       Required alignment of memory block (in bytes). MUST be a power of 2.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               blk_qty_init    Initial number of elements to be allocated in pool.
*
*               blk_qty_max     Maximum number of elements that can be allocated from this pool. Set to
*                               LIB_MEM_BLK_QTY_UNLIMITED if no limit.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_BLK_ALLOC_CALLBACK
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : None.
*
* Note(s)     : (1) 'blk_size' must be big enough to fit a pointer since the pointer to the next free
*                   block is stored in the block itself (only when free/unused).
*********************************************************************************************************
*/

void  Mem_DynPoolCreateHW (const  CPU_CHAR      *p_name,
                                  MEM_DYN_POOL  *p_pool,
                                  MEM_SEG       *p_seg,
                                  CPU_SIZE_T     blk_size,
                                  CPU_SIZE_T     blk_align,
                                  CPU_SIZE_T     blk_qty_init,
                                  CPU_SIZE_T     blk_qty_max,
                                  RTOS_ERR      *p_err)
{
    MEM_SEG  *p_seg_valid = p_seg;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    if (p_seg_valid == DEF_NULL) {                              /* Dflt to heap in case p_seg is null.                  */
        p_seg_valid = &Mem_SegHeap;
    }
#else
    RTOS_ASSERT_DBG_ERR_SET((p_seg_valid != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);
#endif

    p_pool->Opt          = MEM_DYN_POOL_OPT_HW;
    p_pool->AllocFnct    = DEF_NULL;
    p_pool->AllocFnctArg = DEF_NULL;

    Mem_DynPoolCreateInternal(p_name,
                              p_pool,
                              p_seg_valid,
                              blk_size,
                              blk_align,
                              blk_qty_init,
                              blk_qty_max,
                              p_err);
}


/*
*********************************************************************************************************
*                                          Mem_DynPoolBlkGet()
*
* Description : Gets a memory block from specified pool, growing it if needed.
*
* Argument(s) : p_pool  Pointer to the pool data.
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_POOL_EMPTY
*                           RTOS_ERR_BLK_ALLOC_CALLBACK
*                           RTOS_ERR_SEG_OVF
*
* Return(s)   : Pointer to memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  *Mem_DynPoolBlkGet (MEM_DYN_POOL  *p_pool,
                          RTOS_ERR      *p_err)
{
           void      *p_blk;
    const  CPU_CHAR  *p_pool_name;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_NULL);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_NULL);

                                                                /* Ensure pool is not empty if qty is limited.          */
    CPU_CRITICAL_ENTER();
    if (p_pool->BlkQtyMax != LIB_MEM_BLK_QTY_UNLIMITED) {
        if (p_pool->BlkAllocCnt >= p_pool->BlkQtyMax) {
            CPU_CRITICAL_EXIT();

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
            RTOS_ERR_SET_AND_LOG_DBG(*p_err, RTOS_ERR_POOL_EMPTY, ("Mem_DynPoolBlkGet: pool ", (s)p_pool->NamePtr," has reached its max."));
#else
            RTOS_ERR_SET_AND_LOG_DBG(*p_err, RTOS_ERR_POOL_EMPTY, ("Mem_DynPoolBlkGet: pool has reached its max."));
#endif
            return (DEF_NULL);
        }
    }

                                                                /* --------------- ALLOC FROM FREE LIST --------------- */
    p_pool->BlkAllocCnt++;

    if (p_pool->BlkFreePtr != DEF_NULL) {
        p_blk              =  p_pool->BlkFreePtr;
        p_pool->BlkFreePtr = (DEF_BIT_IS_CLR(p_pool->Opt, MEM_DYN_POOL_OPT_PERSISTENT) == DEF_YES) ? *((void **)p_blk) :
                                                                                                     *((void **)((CPU_INT08U *)p_blk + p_pool->BlkSize - sizeof(void *)));
        CPU_CRITICAL_EXIT();

        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

        LOG_VRB(("Mem_DynPoolBlkGet: obtained already allocated block."));

        return (p_blk);
    }

    CPU_CRITICAL_EXIT();

                                                                /* ------------------ ALLOC NEW BLK ------------------- */
#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
    p_pool_name = p_pool->NamePtr;
#else
    p_pool_name = DEF_NULL;
#endif
    p_blk = Mem_SegAllocInternal(p_pool_name,
                                 p_pool->PoolSegPtr,
                                 p_pool->BlkSize,
                                 p_pool->BlkAlign,
                                (DEF_BIT_IS_CLR(p_pool->Opt, MEM_DYN_POOL_OPT_HW) == DEF_YES) ? LIB_MEM_PADDING_ALIGN_NONE :
                                                                                                p_pool->PoolSegPtr->PaddingAlign,
                                 DEF_NULL,
                                 p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        CPU_CRITICAL_ENTER();
        p_pool->BlkAllocCnt--;
        CPU_CRITICAL_EXIT();
        return (DEF_NULL);
    }

    if (p_pool->AllocFnct != DEF_NULL) {
        CPU_BOOLEAN  alloc_ok;


        alloc_ok = p_pool->AllocFnct(p_pool,
                                     p_pool->PoolSegPtr,
                                     p_blk,
                                     p_pool->AllocFnctArg);
        if (alloc_ok != DEF_OK) {
            CPU_CRITICAL_ENTER();                               /* The allocated block will be lost.                    */
            p_pool->BlkAllocCnt--;
            CPU_CRITICAL_EXIT();

            RTOS_ERR_SET_AND_LOG_ERR(*p_err, RTOS_ERR_BLK_ALLOC_CALLBACK, ("Mem_DynPoolBlkGet: failed due to callback, block lost."));
            return (DEF_NULL);
        }
    }

    LOG_VRB(("Mem_DynPoolBlkGet: obtained newly allocated block."));

    return (p_blk);
}


/*
*********************************************************************************************************
*                                         Mem_DynPoolBlkFree()
*
* Description : Frees a memory block, making it available for future use.
*
* Argument(s) : p_pool  Pointer to the pool data.
*
*               p_blk   Pointer to first byte of memory block.
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_POOL_FULL
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

void  Mem_DynPoolBlkFree (MEM_DYN_POOL  *p_pool,
                          void          *p_blk,
                          RTOS_ERR      *p_err)
{
    void  *p_blk_next_addr;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);
    RTOS_ASSERT_DBG_ERR_SET((p_blk  != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

    CPU_CRITICAL_ENTER();                                       /* Ensure pool is not full.                             */
    if (p_pool->BlkAllocCnt == 0u) {
        CPU_CRITICAL_EXIT();


#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
        RTOS_ERR_SET_AND_LOG_DBG(*p_err, RTOS_ERR_POOL_FULL, ("Mem_DynPoolBlkFree: pool ", (s)p_pool->NamePtr," is already full."));
#else
        RTOS_ERR_SET_AND_LOG_DBG(*p_err, RTOS_ERR_POOL_FULL, ("Mem_DynPoolBlkFree: pool is already full."));
#endif
        return;
    }

    p_pool->BlkAllocCnt--;
    CPU_CRITICAL_EXIT();

    p_blk_next_addr = (DEF_BIT_IS_CLR(p_pool->Opt, MEM_DYN_POOL_OPT_PERSISTENT) == DEF_YES) ? p_blk :
                                                                                             ((CPU_INT08U *)p_blk) + p_pool->BlkSize - sizeof(void *);

    CPU_CRITICAL_ENTER();
   *((void **)p_blk_next_addr) = p_pool->BlkFreePtr;
    p_pool->BlkFreePtr = p_blk;
    CPU_CRITICAL_EXIT();

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}


/*
*********************************************************************************************************
*                                      Mem_DynPoolBlkNbrAvailGet()
*
* Description : Gets a number of available blocks in dynamic memory pool. If 'p_pool_info' is DEF_NULL,
*               this call will fail with a dynamic memory pool for which no limit was set at creation.
*
* Argument(s) : p_pool          Pointer to the pool data.
*
*               p_pool_info     Pointer to MEM_DYN_POOL_INFO that will be filled by this function. If
*                               DEF_NULL and if pool has a block limit, only the number of blocks
*                               remaining is returned.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_POOL_UNLIMITED
*
* Return(s)   : Number of blocks remaining in dynamic memory pool, if successful.
*
*               0, if pool is empty or if an error occurred.
*
* Note(s)     : None.
*********************************************************************************************************
*/

CPU_SIZE_T  Mem_DynPoolBlkNbrAvailGet (MEM_DYN_POOL       *p_pool,
                                       MEM_DYN_POOL_INFO  *p_pool_info,
                                       RTOS_ERR           *p_err)
{
    CPU_SIZE_T  blk_nbr_rem;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, 0);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, 0);

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    CPU_CRITICAL_ENTER();
    if (p_pool->BlkQtyMax != LIB_MEM_BLK_QTY_UNLIMITED) {
        blk_nbr_rem = p_pool->BlkQtyMax - p_pool->BlkAllocCnt;
    } else {
        blk_nbr_rem = LIB_MEM_BLK_QTY_UNLIMITED;
        if (p_pool_info == DEF_NULL) {
            RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_UNLIMITED);
        }
    }

    if (p_pool_info != DEF_NULL) {
        CPU_SIZE_T   blk_nbr_avail = 0u;
        void        *p_blk;


        p_blk = p_pool->BlkFreePtr;
        while (p_blk != DEF_NULL) {                             /* Iterate through free list to cnt nbr of blks.        */
            blk_nbr_avail++;

            p_blk = (DEF_BIT_IS_CLR(p_pool->Opt, MEM_DYN_POOL_OPT_PERSISTENT) == DEF_YES) ? *((void **)p_blk) :
                                                                                            *((void **)((CPU_INT08U *)p_blk + p_pool->BlkSize - sizeof(void *)));
        }

        p_pool_info->BlkQtyMax      = p_pool->BlkQtyMax;
        p_pool_info->BlkNbrAllocCnt = p_pool->BlkAllocCnt;
        CPU_CRITICAL_EXIT();

        p_pool_info->BlkNbrRemCnt   = blk_nbr_rem;
        p_pool_info->BlkNbrAvailCnt = blk_nbr_avail;

        LOG_VRB(("Calculated number of max blocks (",             (u)p_pool_info->BlkQtyMax,
                     ") allocated and used blocks (",             (u)(p_pool_info->BlkNbrAllocCnt - blk_nbr_avail),
                     ") allocated and currently unused blocks (", (u)blk_nbr_avail,
                     ") and remaining blocks to allocate (",      (u)blk_nbr_rem, ")."));
    } else {
        CPU_CRITICAL_EXIT();
    }

    return (blk_nbr_rem);
}


/*
*********************************************************************************************************
*                                           Mem_OutputUsage()
*
* Description : Outputs the memory usage report through 'out_fnct'.
*
* Argument(s) : out_fnct    Pointer to output function.
*
*               p_err       Pointer to the variable that will receive one of the following error code(s)
*                           from this function:
*
*                               RTOS_ERR_NONE
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
void  Mem_OutputUsage(void      (*out_fnct) (CPU_CHAR  *p_str),
                      RTOS_ERR   *p_err)
{
    CPU_SIZE_T   rem_size;
    CPU_CHAR     str[DEF_INT_32U_NBR_DIG_MAX + 1u];
    MEM_SEG     *p_seg;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for NULL out fnct ptr.                           */
    RTOS_ASSERT_DBG_ERR_SET((out_fnct != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

    out_fnct((CPU_CHAR *)"---------------- Memory allocation info ----------------\r\n");
    out_fnct((CPU_CHAR *)"| Type    | Size       | Free size  | Name\r\n");
    out_fnct((CPU_CHAR *)"|---------|------------|------------|-------------------\r\n");

    CPU_CRITICAL_ENTER();
    p_seg = Mem_SegHeadPtr;
    while (p_seg != DEF_NULL) {
        MEM_SEG_INFO     seg_info;
        MEM_ALLOC_INFO  *p_alloc;


        rem_size = Mem_SegRemSizeGet(p_seg, 1u, &seg_info, p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            return;
        }

        out_fnct((CPU_CHAR *)"| Section | ");

        (void)Str_FmtNbr_Int32U(seg_info.TotalSize,
                                10u,
                                DEF_NBR_BASE_DEC,
                                ' ',
                                DEF_NO,
                                DEF_YES,
                               &str[0u]);

        out_fnct(str);
        out_fnct((CPU_CHAR *)" | ");

        (void)Str_FmtNbr_Int32U(rem_size,
                                10u,
                                DEF_NBR_BASE_DEC,
                                ' ',
                                DEF_NO,
                                DEF_YES,
                               &str[0u]);

        out_fnct(str);
        out_fnct((CPU_CHAR *)" | ");
        out_fnct((p_seg->NamePtr != DEF_NULL) ? (CPU_CHAR *)p_seg->NamePtr : (CPU_CHAR *)"Unknown");
        out_fnct((CPU_CHAR *)"\r\n");

        p_alloc = p_seg->AllocInfoHeadPtr;
        while (p_alloc != DEF_NULL) {
            out_fnct((CPU_CHAR *)"| -> Obj  | ");

            (void)Str_FmtNbr_Int32U(p_alloc->Size,
                                    10u,
                                    DEF_NBR_BASE_DEC,
                                    ' ',
                                    DEF_NO,
                                    DEF_YES,
                                   &str[0u]);

            out_fnct(str);
            out_fnct((CPU_CHAR *)" |            | ");

            out_fnct((p_alloc->NamePtr != DEF_NULL) ? (CPU_CHAR *)p_alloc->NamePtr : (CPU_CHAR *)"Unknown");
            out_fnct((CPU_CHAR *)"\r\n");

            p_alloc = p_alloc->NextPtr;
        }

        p_seg = p_seg->NextPtr;
    }
    CPU_CRITICAL_EXIT();

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      DEPRECATED GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                              Mem_Init()
*
* Description : Initializes the Memory Management Module as follows :
*
*                   (a) Initialize the heap memory pool.
*                   (b) Initialize the memory pool table.
*
*
* Argument(s) : None.
*
* Return(s)   : None.
*
* Note(s)     : (1) The following function is deprecated. Unless already used, it should no longer be
*                   used, since they will be removed in a future version.
*********************************************************************************************************
*/

void  Mem_Init (void)
{
#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
    {
        RTOS_ERR  err;


        Mem_SegReg(&Mem_SegHeap,
                   &err);
        RTOS_ASSERT_CRITICAL((RTOS_ERR_CODE_GET(err) == RTOS_ERR_NONE), RTOS_ERR_ASSERT_CRITICAL_FAIL, ;);
    }
#endif
}


/*
*********************************************************************************************************
*                                           Mem_HeapAlloc()
*
* Description : Allocates a memory block from the heap memory segment.
*
* Argument(s) : size            Size      of memory block to allocate (in bytes).
*
*               align           Alignment of memory block to specific word boundary (in bytes).
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               p_bytes_reqd    Optional pointer to a variable to ... :
*
*                                   (a) Return the number of bytes required to successfully
*                                           allocate the memory block, if any error(s);
*                                   (b) Return 0, otherwise.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*
*                                   ---------------- RETURNED BY Mem_SegAllocInternal() -----------------
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : Pointer to memory block, if NO error(s).
*
*               Pointer to NULL, otherwise.
*
* Note(s)     : (1) Pointers to variables that return values MUST be initialized before all other
*                   validations or function handling in case of any error(s).
*
*               (2) This function is DEPRECATED and will be removed in a future version of this product.
*                   Instead, use Mem_SegAlloc(), Mem_SegAllocExt(), or Mem_SegAllocHW().
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  *Mem_HeapAlloc (CPU_SIZE_T   size,
                      CPU_SIZE_T   align,
                      CPU_SIZE_T  *p_bytes_reqd,
                      RTOS_ERR    *p_err)
{
    void  *p_mem;


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_NULL);

    p_mem = Mem_SegAllocInternal(DEF_NULL,
                                &Mem_SegHeap,
                                 size,
                                 align,
                                 LIB_MEM_CFG_HEAP_PADDING_ALIGN,
                                 p_bytes_reqd,
                                 p_err);

    return (p_mem);
}
#endif


/*
*********************************************************************************************************
*                                        Mem_HeapGetSizeRem()
*
* Description : Gets remaining heap memory size available to allocate.
*
* Argument(s) : align       Desired word boundary alignment (in bytes) to return remaining memory size from.
*                           LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               p_err       Pointer to the variable that will receive one of the following error code(s)
*                           from this function:
*
*                               RTOS_ERR_NONE
*
* Return(s)   : Remaining heap memory size (in bytes), if NO error(s).
*
*               0, otherwise.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Instead, use Mem_SegRemSizeGet().
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
CPU_SIZE_T  Mem_HeapGetSizeRem (CPU_SIZE_T   align,
                                RTOS_ERR    *p_err)
{
    CPU_SIZE_T  rem_size;


    rem_size = Mem_SegRemSizeGet(&Mem_SegHeap,
                                  align,
                                  DEF_NULL,
                                  p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return (0u);
    }

    return (rem_size);
}
#endif


/*
*********************************************************************************************************
*                                          Mem_PoolCreate()
*
* Description : (1) Creates a memory pool as follows :
*
*                   (a) Create a memory pool from heap or dedicated memory
*                   (b) Allocate memory pool memory blocks
*                   (c) Configure a memory pool
*
*
* Argument(s) : p_pool          Pointer to a memory pool structure to create (see Note #1).
*
*               p_mem_base      Memory pool segment base address :
*
*                                       (a) Null address : Memory pool allocated from general-purpose heap.
*                                       (b) Non-null address: Memory pool allocated from dedicated memory
*                                                             specified by its base address.
*
*               mem_size        Size of memory pool segment (in bytes).
*
*               blk_nbr         Number of memory pool blocks to create.
*
*               blk_size        Size of memory pool blocks to create (in bytes).
*
*               blk_align       Alignment of memory pool blocks to specific word boundary (in bytes).
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               p_bytes_reqd    Optional pointer to a variable to :
*
*                                   (a) Return the number of bytes required to successfully
*                                       allocate the memory pool, if any error(s);
*                                   (b) Return 0, otherwise.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*
*                                   -----------------RETURNED BY Mem_SegAllocExtCritical()-----------------
*                                   ----------------RETURNED BY Mem_SegAllocTrackCritical()----------------
*                                   ------------------RETURNED BY Mem_SegAllocInternal()-------------------
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : None.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Instead, use Mem_DynPoolCreate() or Mem_DynPoolCreateHW().
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  Mem_PoolCreate (MEM_POOL          *p_pool,
                      void              *p_mem_base,
                      CPU_SIZE_T         mem_size,
                      MEM_POOL_BLK_QTY   blk_nbr,
                      CPU_SIZE_T         blk_size,
                      CPU_SIZE_T         blk_align,
                      CPU_SIZE_T        *p_bytes_reqd,
                      RTOS_ERR          *p_err)
{
    MEM_SEG           *p_seg;
    void              *p_pool_mem;
    CPU_SIZE_T         pool_size;
    CPU_SIZE_T         blk_size_align;
    CPU_SIZE_T         blk_align_real;
    CPU_ADDR           pool_addr_end;
    MEM_POOL_BLK_QTY   blk_ix;
    CPU_INT08U        *p_blk;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

    RTOS_ASSERT_DBG_ERR_SET((blk_nbr >= 1u), *p_err, RTOS_ERR_INVALID_ARG, ;);

    RTOS_ASSERT_DBG_ERR_SET(((p_mem_base == DEF_NULL) || (mem_size >= 1u)), *p_err, RTOS_ERR_INVALID_ARG, ;);

    RTOS_ASSERT_DBG_ERR_SET((blk_size >= 1u), *p_err, RTOS_ERR_INVALID_ARG, ;);

                                                                /* Chk that req alignment is a pwr of 2.                */
    RTOS_ASSERT_DBG_ERR_SET((MATH_IS_PWR2(blk_align) == DEF_YES) ||
                            (blk_align               == LIB_MEM_BUF_ALIGN_AUTO), *p_err, RTOS_ERR_INVALID_ARG, ;);


    Mem_PoolClr(p_pool, p_err);                                 /* Init mem pool.                                       */
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
         return;
    }

                                                                /* -------- DETERMINE AND/OR ALLOC SEG TO USE --------- */
    if (p_mem_base == DEF_NULL) {                               /* Use heap seg.                                        */
        p_seg = &Mem_SegHeap;
    } else {                                                    /* Use other seg.                                       */
        MEM_SEG_STATUS  status;


        CPU_CRITICAL_ENTER();
        p_seg = Mem_SegOverlapChkCritical((CPU_ADDR)p_mem_base,
                                                    mem_size,
                                                   &status);
        switch (status) {
            case MEM_SEG_STATUS_EXISTS:                         /* Seg already exists.                                  */
                 break;

            case MEM_SEG_STATUS_NONE:                           /* Seg must be created.                                 */
                 p_seg = (MEM_SEG *)Mem_SegAllocExtCritical(&Mem_SegHeap,
                                                             sizeof(MEM_SEG),
                                                             sizeof(CPU_ALIGN),
                                                             LIB_MEM_PADDING_ALIGN_NONE,
                                                             p_bytes_reqd,
                                                             p_err);
                 if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                     CPU_CRITICAL_EXIT();
                     return;
                 }

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)                    /* Track alloc if req'd.                                */
                 Mem_SegAllocTrackCritical("Unknown segment data",
                                           &Mem_SegHeap,
                                            sizeof(MEM_SEG),
                                            p_err);
                 if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
                     CPU_CRITICAL_EXIT();
                     return;
                 }
#endif

                 Mem_SegCreateCritical(          DEF_NULL,
                                                 p_seg,
                                       (CPU_ADDR)p_mem_base,
                                                 LIB_MEM_PADDING_ALIGN_NONE,
                                                 mem_size);
                 break;


            case MEM_SEG_STATUS_OVERLAPS:
            default:
                 CPU_CRITICAL_EXIT();
                 return;                                        /* Prevent 'break NOT reachable' compiler warning.      */
        }

        CPU_CRITICAL_EXIT();
    }


                                                                /* ---------------- ALLOC MEM FOR POOL ---------------- */
                                                                /* Calc blk size with align.                            */
    blk_align_real = (blk_align == LIB_MEM_BUF_ALIGN_AUTO) ? CPU_MIN_DATA_ALIGN_BYTES() : blk_align;
    blk_size_align =  MATH_ROUND_INC_UP_PWR2(blk_size, blk_align_real);
    pool_size      =  blk_size_align * blk_nbr;                 /* Calc required size for pool.                         */

                                                                /* Alloc mem for pool.                                  */
    p_pool_mem = (void *)Mem_SegAllocInternal("Unnamed static pool",
                                               p_seg,
                                               pool_size,
                                               blk_align_real,
                                               LIB_MEM_PADDING_ALIGN_NONE,
                                               p_bytes_reqd,
                                               p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

                                                                /* ------------ ALLOC MEM FOR FREE BLK TBL ------------ */
    p_pool->BlkFreeTbl = (void **)Mem_SegAllocInternal("Unnamed static pool free blk tbl",
                                                       &Mem_SegHeap,
                                                        blk_nbr * sizeof(void *),
                                                        sizeof(CPU_ALIGN),
                                                        LIB_MEM_PADDING_ALIGN_NONE,
                                                        p_bytes_reqd,
                                                        p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

                                                                /* ------------------ INIT BLK LIST ------------------- */
    p_blk = (CPU_INT08U *)p_pool_mem;
    for (blk_ix = 0; blk_ix < blk_nbr; blk_ix++) {
        p_pool->BlkFreeTbl[blk_ix]  = p_blk;
        p_blk                      += blk_size_align;
    }


                                                                /* ------------------ INIT POOL DATA ------------------ */
    pool_addr_end         = (CPU_ADDR)p_pool_mem + (pool_size - 1u);
    p_pool->PoolAddrStart =  p_pool_mem;
    p_pool->PoolAddrEnd   = (void *)pool_addr_end;
    p_pool->BlkNbr        =  blk_nbr;
    p_pool->BlkSize       =  blk_size_align;
    p_pool->BlkFreeTblIx  =  blk_nbr;
}
#endif


/*
*********************************************************************************************************
*                                            Mem_PoolClr()
*
* Description : Clears a memory pool (see Note #1).
*
* Argument(s) : p_pool  Pointer to a memory pool structure to clear (see Note #2).
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                           RTOS_ERR_NONE
*
* Return(s)   : None.
*
* Note(s)     : (1) (a) Mem_PoolClr() ONLY clears a memory pool structure's variables and should ONLY be
*                       called to initialize a memory pool structure prior to calling Mem_PoolCreate().
*
*                   (b) Mem_PoolClr() does NOT deallocate memory from the memory pool or deallocate the
*                       memory pool itself and MUST NOT be called after calling Mem_PoolCreate() since
*                       this will likely corrupt the memory pool management.
*
*               (2) Assumes 'p_pool' points to a valid memory pool (if non-NULL).
*
*               (3) This function is DEPRECATED and will be removed in a future version of this product.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  Mem_PoolClr (MEM_POOL  *p_pool,
                   RTOS_ERR  *p_err)
{
    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

    p_pool->PoolAddrStart = DEF_NULL;
    p_pool->PoolAddrEnd   = DEF_NULL;
    p_pool->BlkSize       = 0u;
    p_pool->BlkNbr        = 0u;
    p_pool->BlkFreeTbl    = DEF_NULL;
    p_pool->BlkFreeTblIx  = 0u;

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}
#endif


/*
*********************************************************************************************************
*                                          Mem_PoolBlkGet()
*
* Description : Gets a memory block from memory pool.
*
* Argument(s) : p_pool  Pointer to a memory pool to get memory block from.
*
*               size    Size of requested memory (in bytes).
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_POOL_EMPTY
*
* Return(s)   : Pointer to memory block, if NO error(s).
*
*               Pointer to NULL, otherwise.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Instead, use Mem_DynPoolBlkGet().
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  *Mem_PoolBlkGet (MEM_POOL    *p_pool,
                       CPU_SIZE_T   size,
                       RTOS_ERR    *p_err)
{
    CPU_INT08U  *p_blk;
    void        *p_ret_blk;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, DEF_NULL);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, DEF_NULL);

                                                                /* Validate req'd size as non-NULL.                     */
    RTOS_ASSERT_DBG_ERR_SET((size >= 1u), *p_err, RTOS_ERR_INVALID_ARG, DEF_NULL);

                                                                /* Validate req'd size <= mem pool blk size.            */
    RTOS_ASSERT_DBG_ERR_SET((size <= p_pool->BlkSize), *p_err, RTOS_ERR_INVALID_ARG, DEF_NULL);


                                                                /* -------------- GET MEM BLK FROM POOL --------------- */
    p_blk = DEF_NULL;
    CPU_CRITICAL_ENTER();
    if (p_pool->BlkFreeTblIx > 0u) {
        p_pool->BlkFreeTblIx                     -=  1u;
        p_blk                                     = (CPU_INT08U *)p_pool->BlkFreeTbl[p_pool->BlkFreeTblIx];
        p_pool->BlkFreeTbl[p_pool->BlkFreeTblIx]  =  DEF_NULL;
    }
    CPU_CRITICAL_EXIT();

    if (p_blk == DEF_NULL) {
        RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_EMPTY);
    } else {
        RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
    }

    p_ret_blk = (void *)p_blk;

    return (p_ret_blk);
}
#endif


/*
*********************************************************************************************************
*                                          Mem_PoolBlkFree()
*
* Description : Free a memory block to memory pool.
*
* Argument(s) : p_pool  Pointer to a memory pool to free memory block.
*
*               p_blk   Pointer to a memory block address to free.
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_ALREADY_EXISTS
*                           RTOS_ERR_POOL_FULL
*
* Return(s)   : None.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Instead, use Mem_DynPoolBlkFree().
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
void  Mem_PoolBlkFree (MEM_POOL   *p_pool,
                       void       *p_blk,
                       RTOS_ERR   *p_err)
{
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, ;);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

    RTOS_ASSERT_DBG_ERR_SET((p_blk != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, ;);

                                                                /* Validate mem blk as valid pool blk addr.             */
#if ((RTOS_ARG_CHK_EXT_EN) && \
     (LIB_MEM_CFG_HEAP_SIZE > 0u))
    RTOS_ASSERT_DBG_ERR_SET((Mem_PoolBlkIsValidAddr(p_pool, p_blk) == DEF_OK), *p_err, RTOS_ERR_NULL_PTR, ;);
#endif


    CPU_CRITICAL_ENTER();                                       /* Double-free possibility if not in critical section.  */

#if RTOS_ARG_CHK_EXT_EN
    {
        CPU_SIZE_T  tbl_ix;


        for (tbl_ix = 0u; tbl_ix < p_pool->BlkNbr; tbl_ix++) {  /* Make sure blk isn't already in free list.            */
            if (p_pool->BlkFreeTbl[tbl_ix] == p_blk) {
                CPU_CRITICAL_EXIT();
                RTOS_ERR_SET(*p_err, RTOS_ERR_ALREADY_EXISTS);
                return;
            }
        }
    }
#endif
                                                                /* --------------- FREE MEM BLK TO POOL --------------- */
    if (p_pool->BlkFreeTblIx >= p_pool->BlkNbr) {
        CPU_CRITICAL_EXIT();

        RTOS_ERR_SET(*p_err, RTOS_ERR_POOL_FULL);
        return;
    }

    p_pool->BlkFreeTbl[p_pool->BlkFreeTblIx]  = p_blk;
    p_pool->BlkFreeTblIx                     += 1u;
    CPU_CRITICAL_EXIT();

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
}
#endif


/*
*********************************************************************************************************
*                                      Mem_PoolBlkGetNbrAvail()
*
* Description : Get a memory pool's remaining number of blocks that are available to allocate.
*
* Argument(s) : p_pool  Pointer to a memory pool structure.
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                               RTOS_ERR_NONE
*
* Return(s)   : Remaining memory pool blocks, if NO error(s).
*
*               0, otherwise.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*                   Mem_DynPoolBlkNbrAvailGet() should be used instead.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_HEAP_SIZE > 0u)
MEM_POOL_BLK_QTY  Mem_PoolBlkGetNbrAvail (MEM_POOL  *p_pool,
                                          RTOS_ERR  *p_err)
{
    CPU_SIZE_T  nbr_avail;
    CPU_SR_ALLOC();


    RTOS_ASSERT_DBG_ERR_PTR_VALIDATE(p_err, 0u);

                                                                /* Chk for NULL pool data ptr.                          */
    RTOS_ASSERT_DBG_ERR_SET((p_pool != DEF_NULL), *p_err, RTOS_ERR_NULL_PTR, 0u);

    CPU_CRITICAL_ENTER();
    nbr_avail = p_pool->BlkFreeTblIx;
    CPU_CRITICAL_EXIT();

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return (nbr_avail);
}
#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                        Mem_SegCreateCritical()
*
* Description : Creates a new memory segment to be used for runtime memory allocation or dynamic pools.
*
* Argument(s) : p_name          Pointer to segment name.
*
*               p_seg           Pointer to segment data. Must be allocated by caller.
*               -----           Argument validated by caller.
*
*               seg_base_addr   Segment's first byte address.
*
*               padding_align   Padding alignment (in bytes), that will be added to any allocated buffer
*                               from this memory segment. MUST be a power of 2.
*                               LIB_MEM_PADDING_ALIGN_NONE means no padding.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*               -------------   Argument validated by caller.
*
*               size            Total size of segment (in bytes).
*               ----            Argument validated by caller.
*
* Return(s)   : Pointer to segment data, if successful.
*
*               DEF_NULL, otherwise.
*
* Note(s)     : (1) This function MUST be called within a CRITICAL_SECTION.
*********************************************************************************************************
*/

static  void  Mem_SegCreateCritical(const  CPU_CHAR    *p_name,
                                           MEM_SEG     *p_seg,
                                           CPU_ADDR     seg_base_addr,
                                           CPU_SIZE_T   padding_align,
                                           CPU_SIZE_T   size)
{
    p_seg->AddrBase         =  seg_base_addr;
    p_seg->AddrEnd          = (seg_base_addr + (size - 1u));
    p_seg->AddrNext         =  seg_base_addr;
    p_seg->NextPtr          =  Mem_SegHeadPtr;
    p_seg->PaddingAlign     = (padding_align == LIB_MEM_BUF_ALIGN_AUTO) ? CPU_MIN_DATA_ALIGN_BYTES() : padding_align;

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
    p_seg->NamePtr          = p_name;
    p_seg->AllocInfoHeadPtr = DEF_NULL;
#else
    PP_UNUSED_PARAM(p_name);
#endif

    Mem_SegHeadPtr = p_seg;
}


/*
*********************************************************************************************************
*                                      Mem_SegOverlapChkCritical()
*
* Description : Checks if existing memory segment exists or overlaps with specified memory area.
*
* Argument(s) : seg_base_addr   Address of first byte of memory area.
*
*               size            Size of memory area (in bytes).
*
*               p_status        Pointer to a variable that will receive the segment status :
*
*                                   MEM_SEG_STATUS_NONE     Segment does not exist.
*                                   MEM_SEG_STATUS_EXISTS   Segment already exists.
*                                   MEM_SEG_STATUS_OVERLAPS Segment overlaps another existing segment.
*
* Return(s)   : Pointer to memory segment that overlaps.
*
*               DEF_NULL, otherwise.
*
* Note(s)     : (1) This function MUST be called within a CRITICAL_SECTION.
*********************************************************************************************************
*/

#if ((LIB_MEM_CFG_HEAP_SIZE > 0u) || \
     (RTOS_ARG_CHK_EXT_EN))
static  MEM_SEG  *Mem_SegOverlapChkCritical (CPU_ADDR         seg_base_addr,
                                             CPU_SIZE_T       size,
                                             MEM_SEG_STATUS  *p_status)
{
    MEM_SEG   *p_seg_chk;
    CPU_ADDR   seg_new_end;
    CPU_ADDR   seg_chk_start;
    CPU_ADDR   seg_chk_end;


    seg_new_end = seg_base_addr + (size - 1u);
    p_seg_chk   = Mem_SegHeadPtr;

    while (p_seg_chk != DEF_NULL) {
        seg_chk_start = p_seg_chk->AddrBase;
        seg_chk_end   = p_seg_chk->AddrEnd;

        if ((seg_base_addr == seg_chk_start) && (seg_new_end == seg_chk_end)) {
           *p_status = MEM_SEG_STATUS_EXISTS;

            return (p_seg_chk);
        } else if (((seg_base_addr >= seg_chk_start) && (seg_base_addr <= seg_chk_end)) ||
                   ((seg_base_addr <= seg_chk_start) && (seg_new_end   >= seg_chk_start))) {
           *p_status = MEM_SEG_STATUS_OVERLAPS;

            return (p_seg_chk);
        }

        p_seg_chk = p_seg_chk->NextPtr;
    }

   *p_status = MEM_SEG_STATUS_NONE;

    return (DEF_NULL);
}
#endif


/*
*********************************************************************************************************
*                                        Mem_SegAllocInternal()
*
* Description : Allocates memory from specified segment.
*
* Argument(s) : p_name          Pointer to allocated object name. Used to track allocations. May be
*                               DEF_NULL.
*
*               p_seg           Pointer to segment from which to allocate memory.
*               -----           Argument validated by caller.
*
*               size            Size of memory block to allocate (in bytes).
*
*               align           Required alignment of memory block (in bytes). MUST be a power of 2.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               padding_align   Padding alignment (in bytes), that will be added to any allocated buffer
*                               from this memory segment. MUST be a power of 2.
*                               LIB_MEM_PADDING_ALIGN_NONE means no padding.
*
*               p_bytes_reqd    Pointer to a variable that will receive the number of free bytes missing
*                               for the allocation to succeed. Set to DEF_NULL to skip calculation.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*               -----           Argument validated by caller.
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Note(s)     : None.
*********************************************************************************************************
*/

static  void  *Mem_SegAllocInternal (const  CPU_CHAR    *p_name,
                                            MEM_SEG     *p_seg,
                                            CPU_SIZE_T   size,
                                            CPU_SIZE_T   align,
                                            CPU_SIZE_T   padding_align,
                                            CPU_SIZE_T  *p_bytes_reqd,
                                            RTOS_ERR    *p_err)
{
    void  *p_blk;
    CPU_SR_ALLOC();


                                                                /* Chk for invalid sized mem req.                       */
    RTOS_ASSERT_DBG_ERR_SET((size >= 1u), *p_err, RTOS_ERR_INVALID_ARG, DEF_NULL);

                                                                /* Chk that align is a pwr of 2.                        */
    RTOS_ASSERT_DBG_ERR_SET((MATH_IS_PWR2(align) == DEF_YES) ||
                            (align               == LIB_MEM_BUF_ALIGN_AUTO), *p_err, RTOS_ERR_INVALID_ARG, DEF_NULL);

    CPU_CRITICAL_ENTER();
    p_blk = Mem_SegAllocExtCritical(p_seg,
                                    size,
                                    align,
                                    padding_align,
                                    p_bytes_reqd,
                                    p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        CPU_CRITICAL_EXIT();
        RTOS_ERR_SET(*p_err, RTOS_ERR_SEG_OVF);

        return (DEF_NULL);
    }

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)                    /* Track alloc if req'd.                                */
    Mem_SegAllocTrackCritical(p_name,
                              p_seg,
                              size,
                              p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        CPU_CRITICAL_EXIT();
        return (DEF_NULL);
    }
#else
    PP_UNUSED_PARAM(p_name);
#endif
    CPU_CRITICAL_EXIT();

    return (p_blk);
}


/*
*********************************************************************************************************
*                                       Mem_SegAllocExtCritical()
*
* Description : Allocates memory from specified segment.
*
* Argument(s) : p_seg           Pointer to segment from which to allocate memory.
*
*               size            Size of memory block to allocate (in bytes).
*
*               align           Required alignment of memory block (in bytes). MUST be a power of 2.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               padding_align   Padding alignment (in bytes), that will be added to any allocated buffer
*                               from this memory segment. MUST be a power of 2.
*                               LIB_MEM_PADDING_ALIGN_NONE means no padding.
*
*               p_bytes_reqd    Pointer to a variable that will receive the number of free bytes missing
*                               for the allocation to succeed. Set to DEF_NULL to skip calculation.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : Pointer to allocated memory block, if successful.
*
*               DEF_NULL, otherwise.
*
* Note(s)     : (1) This function MUST be called within a CRITICAL_SECTION.
*********************************************************************************************************
*/

static  void  *Mem_SegAllocExtCritical (MEM_SEG     *p_seg,
                                        CPU_SIZE_T   size,
                                        CPU_SIZE_T   align,
                                        CPU_SIZE_T   padding_align,
                                        CPU_SIZE_T  *p_bytes_reqd,
                                        RTOS_ERR    *p_err)
{
    CPU_ADDR    blk_addr;
    CPU_ADDR    addr_next;
    CPU_SIZE_T  size_rem_seg;
    CPU_SIZE_T  size_tot_blk;
    CPU_SIZE_T  blk_align     = DEF_MAX((align == LIB_MEM_BUF_ALIGN_AUTO) ? CPU_MIN_DATA_ALIGN_BYTES() : align, padding_align);


    blk_addr     = MATH_ROUND_INC_UP_PWR2(p_seg->AddrNext,      /* Compute align'ed blk addr.                           */
                                          blk_align);
    addr_next    = MATH_ROUND_INC_UP_PWR2(blk_addr + size,      /* Compute addr of next alloc.                          */
                                          padding_align);
    size_rem_seg = (p_seg->AddrEnd - p_seg->AddrNext) + 1u;
    size_tot_blk =  addr_next      - p_seg->AddrNext;           /* Compute tot blk size including align and padding.    */
    if (size_rem_seg < size_tot_blk) {                          /* If seg doesn't have enough space ...                 */
        if (p_bytes_reqd != DEF_NULL) {                         /* ... calc nbr of req'd bytes.                         */
           *p_bytes_reqd = size_tot_blk - size_rem_seg;
        }
        RTOS_ERR_SET(*p_err, RTOS_ERR_SEG_OVF);
        return (DEF_NULL);
    }

    p_seg->AddrNext = addr_next;

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

    return ((void *)blk_addr);
}


/*
*********************************************************************************************************
*                                      Mem_SegAllocTrackCritical()
*
* Description : Tracks segment allocation, adding the 'size' of the allocation under the 'p_name' entry.
*
* Argument(s) : p_name  Pointer to the name of the object. This string is not copied and its memory should
*                       remain accessible at all times.
*
*               p_seg   Pointer to segment data.
*
*               size    Allocation size (in bytes).
*
*               p_err   Pointer to the variable that will receive one of the following error code(s) from
*                       this function:
*
*                           RTOS_ERR_NONE
*                           RTOS_ERR_SEG_OVF
*
* Return(s)   : None.
*
* Note(s)     : None.
*********************************************************************************************************
*/

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
static  void  Mem_SegAllocTrackCritical (const  CPU_CHAR    *p_name,
                                                MEM_SEG     *p_seg,
                                                CPU_SIZE_T   size,
                                                RTOS_ERR    *p_err)
{
    MEM_ALLOC_INFO  *p_alloc;


                                                                /* ------- UPDATE ALLOC INFO LIST, IF POSSIBLE -------- */
    p_alloc = p_seg->AllocInfoHeadPtr;
    while (p_alloc != DEF_NULL) {
        if (p_alloc->NamePtr == p_name) {
            p_alloc->Size += size;
            RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);
            return;
        }

        p_alloc = p_alloc->NextPtr;
    }

                                                                /* --------- ADD NEW ALLOC INFO ENTRY IN LIST --------- */
                                                                /* Alloc new alloc info struct on heap.                 */
    p_alloc = (MEM_ALLOC_INFO *)Mem_SegAllocExtCritical(&Mem_SegHeap,
                                                         sizeof(MEM_ALLOC_INFO),
                                                         sizeof(CPU_ALIGN),
                                                         LIB_MEM_PADDING_ALIGN_NONE,
                                                         DEF_NULL,
                                                         p_err);
    if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
        return;
    }

    p_alloc->NamePtr = p_name;                                  /* Populate alloc info.                                 */
    p_alloc->Size    = size;

    p_alloc->NextPtr        = p_seg->AllocInfoHeadPtr;          /* Prepend new item in list.                            */
    p_seg->AllocInfoHeadPtr = p_alloc;
}
#endif


/*
*********************************************************************************************************
*                                      Mem_DynPoolCreateInternal()
*
* Description : Creates a dynamic memory pool.
*
* Argument(s) : p_name          Pointer to the pool name.
*
*               p_pool          Pointer to the pool data.
*               ------          Argument validated by caller.
*
*               p_seg           Pointer to segment from which to allocate memory.
*               -----           Argument validated by caller.
*
*               blk_size        Size of memory block to allocate from pool (in bytes). See Note #1.
*
*               blk_align       Required alignment of memory block (in bytes). MUST be a power of 2.
*                               LIB_MEM_BUF_ALIGN_AUTO will use cache line size, if cache present.
*
*               blk_qty_init    Initial number of elements to be allocated in pool.
*
*               blk_qty_max     Maximum number of elements that can be allocated from this pool. Set to
*                               LIB_MEM_BLK_QTY_UNLIMITED if no limit.
*
*               p_err           Pointer to the variable that will receive one of the following error
*                               code(s) from this function:
*               -----           Argument validated by caller.
*
*                                   RTOS_ERR_NONE
*                                   RTOS_ERR_BLK_ALLOC_CALLBACK
*                                   RTOS_ERR_SEG_OVF
*
* Return(s)   : None.
*
* Note(s)     : (1) 'blk_size' must be big enough to fit a pointer since the pointer to the next free
*                   block is stored in the block itself (only when free/unused).
*********************************************************************************************************
*/

static  void  Mem_DynPoolCreateInternal (const  CPU_CHAR      *p_name,
                                                MEM_DYN_POOL  *p_pool,
                                                MEM_SEG       *p_seg,
                                                CPU_SIZE_T     blk_size,
                                                CPU_SIZE_T     blk_align,
                                                CPU_SIZE_T     blk_qty_init,
                                                CPU_SIZE_T     blk_qty_max,
                                                RTOS_ERR      *p_err)
{
    CPU_INT08U  *p_blks;
    CPU_SIZE_T   blk_size_real;
    CPU_SIZE_T   blk_align_real;



                                                                /* Chk for invalid blk size.                            */
    RTOS_ASSERT_DBG_ERR_SET((blk_size >= 1u), *p_err, RTOS_ERR_INVALID_ARG, ;);

                                                                /* Chk for invalid blk qty.                             */
    RTOS_ASSERT_DBG_ERR_SET(((blk_qty_max  == LIB_MEM_BLK_QTY_UNLIMITED) ||
                             (blk_qty_init <= blk_qty_max)), *p_err, RTOS_ERR_INVALID_ARG, ;);

                                                                /* Chk for illegal align spec.                          */
    RTOS_ASSERT_DBG_ERR_SET(((MATH_IS_PWR2(blk_align) == DEF_YES) &&
                             (blk_align               >= CPU_MIN_DATA_ALIGN_BYTES())) ||
                            (blk_align == LIB_MEM_BUF_ALIGN_AUTO), *p_err, RTOS_ERR_INVALID_ARG, ;);

    blk_align_real = (blk_align == LIB_MEM_BUF_ALIGN_AUTO) ? CPU_MIN_DATA_ALIGN_BYTES() : blk_align;
                                                                /* Compute actual size of blks.                         */
    if (DEF_BIT_IS_SET(p_pool->Opt, MEM_DYN_POOL_OPT_PERSISTENT) == DEF_YES) {
        blk_size_real  = MATH_ROUND_INC_UP_PWR2(blk_size, CPU_MIN_DATA_ALIGN_BYTES());
        blk_size_real += sizeof(void *);                        /* Add space for next ptr.                              */
    } else {
        blk_size_real  =  DEF_MAX(blk_size, sizeof(void *));    /* Make sure enough space avail to store next ptr.      */
    }

    if (blk_qty_init != 0u) {                                   /* Alloc init blks.                                     */
        CPU_SIZE_T  i;
        CPU_SIZE_T  blk_size_align;
        CPU_SIZE_T  blk_align_worst;
        CPU_SIZE_T  blk_init_tot_size;
        CPU_ADDR    blk_next_addr;


        if (DEF_BIT_IS_CLR(p_pool->Opt, MEM_DYN_POOL_OPT_HW) == DEF_YES) {
            blk_align_worst = blk_align_real;
        } else {
            blk_align_worst = DEF_MAX(blk_align_real, p_seg->PaddingAlign);
        }

        blk_size_align    = MATH_ROUND_INC_UP_PWR2(blk_size_real,
                                                   blk_align_worst);
        blk_init_tot_size = blk_size_align * blk_qty_init;

                                                                /* Remove extra space added to last blk because of blk align.*/
        blk_init_tot_size -= (blk_size_align - blk_size_real);
        if (DEF_BIT_IS_SET(p_pool->Opt, MEM_DYN_POOL_OPT_HW) == DEF_YES) {
            blk_init_tot_size = MATH_ROUND_INC_UP_PWR2(blk_init_tot_size, p_seg->PaddingAlign);
        }

        p_blks = (CPU_INT08U *)Mem_SegAllocInternal(p_name,     /* Alloc initial blks.                                  */
                                                    p_seg,
                                                    blk_init_tot_size,
                                                    blk_align_worst,
                                                    LIB_MEM_PADDING_ALIGN_NONE,
                                                    DEF_NULL,
                                                    p_err);
        if (RTOS_ERR_CODE_GET(*p_err) != RTOS_ERR_NONE) {
            return;
        }

        p_pool->BlkFreePtr = (void *)p_blks;

        if (DEF_BIT_IS_CLR(p_pool->Opt, MEM_DYN_POOL_OPT_PERSISTENT) == DEF_YES) {
            blk_next_addr = (CPU_ADDR)p_blks;
        } else {
            blk_next_addr = (CPU_ADDR)(p_blks + MATH_ROUND_INC_UP_PWR2(blk_size, sizeof(void *)));
        }

        for (i = 1u; i < blk_qty_init; i++) {                   /* Set next addresses to each blk.                      */
           *((void **)blk_next_addr) = p_blks + blk_size_align;

            if (p_pool->AllocFnct != DEF_NULL) {
                CPU_BOOLEAN  alloc_ok;


                alloc_ok = p_pool->AllocFnct(p_pool, p_seg, p_blks, p_pool->AllocFnctArg);
                if (alloc_ok != DEF_OK) {
                    RTOS_ERR_SET_AND_LOG_ERR(*p_err, RTOS_ERR_BLK_ALLOC_CALLBACK, ("lib_mem: failed to create ", (u)blk_qty_init, " initial blocks in pool ", (s)p_name, "due to error in callback."));
                    return;
                }
            }

            blk_next_addr += blk_size_align;
            p_blks        += blk_size_align;
        }

       *((void **)blk_next_addr) = DEF_NULL;

        if (p_pool->AllocFnct != DEF_NULL) {
            CPU_BOOLEAN  alloc_ok;


            alloc_ok = p_pool->AllocFnct(p_pool, p_seg, p_blks, p_pool->AllocFnctArg);
            if (alloc_ok != DEF_OK) {
                RTOS_ERR_SET_AND_LOG_ERR(*p_err, RTOS_ERR_BLK_ALLOC_CALLBACK, ("lib_mem: failed to create ", (u)blk_qty_init, " initial blocks in pool ", (s)p_name, "due to error in callback."));
                return;
            }
        }
    } else {
        p_pool->BlkFreePtr = DEF_NULL;
    }

                                                                    /* ----------------- CREATE POOL DATA ----------------- */
    p_pool->PoolSegPtr  = p_seg;
    p_pool->BlkSize     = blk_size_real;
    p_pool->BlkAlign    = blk_align_real;
    p_pool->BlkQtyMax   = blk_qty_max;
    p_pool->BlkAllocCnt = 0u;

#if (LIB_MEM_CFG_DBG_INFO_EN == DEF_ENABLED)
    p_pool->NamePtr = p_name;
#endif

    RTOS_ERR_SET(*p_err, RTOS_ERR_NONE);

#if LOG_VRB_IS_EN()
    if (p_name == DEF_NULL) {
        LOG_VRB(("Dyn pool created successfully."));
    } else {
        LOG_VRB(("Dyn pool \"", (s)p_name, "\" created successfully."));
    }
#endif
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                      DEPRECATED LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      Mem_PoolBlkIsValidAddr()
*
* Description : Calculates if a given memory block address is valid for the memory pool.
*
* Argument(s) : p_pool   Pointer to memory pool structure to validate memory block address.
*               ------   Argument validated by caller.
*
*               p_mem    Pointer to memory block address to validate.
*               -----    Argument validated by caller.
*
* Return(s)   : DEF_YES, memory pool block address is valid.
*
*               DEF_NO, otherwise.
*
* Note(s)     : (1) This function is DEPRECATED and will be removed in a future version of this product.
*********************************************************************************************************
*/

#if ((RTOS_ARG_CHK_EXT_EN) && \
     (LIB_MEM_CFG_HEAP_SIZE > 0u))
static  CPU_BOOLEAN  Mem_PoolBlkIsValidAddr (MEM_POOL  *p_pool,
                                             void      *p_mem)
{
    CPU_ADDR  pool_offset;


    if ((p_mem < p_pool->PoolAddrStart) ||
        (p_mem > p_pool->PoolAddrEnd)) {
        return (DEF_FALSE);
    }

    pool_offset = (CPU_ADDR)p_mem - (CPU_ADDR)p_pool->PoolAddrStart;
    if (pool_offset % p_pool->BlkSize != 0u) {
        return (DEF_FALSE);
    } else {
        return (DEF_TRUE);
    }
}
#endif
