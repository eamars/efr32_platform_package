/*
*********************************************************************************************************
*                                              Micrium OS
*                                                Common
*
*                             (c) Copyright 2017; Silicon Laboratories Inc.
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
*                                           TOOLCHAINS UTILS
*
* File : toolchains.h
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                                MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _TOOLCHAINS_H_
#define  _TOOLCHAINS_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*********************************************************************************************************
*/

#include  <rtos/common/include/rtos_opt_def.h>
#include  <rtos/common/include/rtos_path.h>


/*
*********************************************************************************************************
*********************************************************************************************************
                                                 DEFINES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      C STANDARD VERSION DEFINES
*********************************************************************************************************
*/

#define PP_C_STD_VERSION_C89         198900L
#define PP_C_STD_VERSION_C90         199000L
#define PP_C_STD_VERSION_C94         199409L
#define PP_C_STD_VERSION_C99         199901L

#if defined(__STDC_VERSION__)
    #if (__STDC_VERSION__ - 0 > 1)
        #define PP_C_STD_VERSION        __STDC_VERSION__
    #else
        #define PP_C_STD_VERSION        PP_C_STD_VERSION_C90
    #endif
#else
    #if defined(__STDC__)
        #define PP_C_STD_VERSION        PP_C_STD_VERSION_C89
    #endif
#endif

#if (PP_C_STD_VERSION - 0 >= PP_C_STD_VERSION_C89)
#define PP_C_STD_VERSION_C89_PRESENT
#endif

#if (PP_C_STD_VERSION - 0 >= PP_C_STD_VERSION_C90)
#define PP_C_STD_VERSION_C90_PRESENT
#endif

#if (PP_C_STD_VERSION - 0 >= PP_C_STD_VERSION_C94)
#define PP_C_STD_VERSION_C94_PRESENT
#endif

#if (PP_C_STD_VERSION - 0 >= PP_C_STD_VERSION_C99)
#define PP_C_STD_VERSION_C99_PRESENT
#endif


/*
*********************************************************************************************************
*                                           GENERAL MACROS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                          PP_UNUSED_PARAM()
*
* Description : Removes warning associated to a function parameter being present but not referenced in a
*               given function.
*
* Argument(s) : param   Parameter that is unused.
*
* Return(s)   : None.
*
* Note(s)     : (1) This macro can be overriden by defining it first in the compiler options.
*********************************************************************************************************
*/

#ifndef  PP_UNUSED_PARAM

#if ((RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_ARMCC)              || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CROSSCORE_BLACKFIN) || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CCS)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_GNU)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_IAR)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_MPLAB_C30)          || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_RXC)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VDSP)               || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VISUALSTUDIO))
#define  PP_UNUSED_PARAM(param)            (void)(param)
#else
#error  "Unknown toolchain. Will define PP_UNUSED_PARAM to nothing."
#define  PP_UNUSED_PARAM(param)
#endif

#endif


/*
*********************************************************************************************************
*                                             PP_ALIGN()
*
* Description : Forces variable to be aligned on specific memory multiple.
*
* Argument(s) : _variable   Variable to align.
*
*               _align      Alignment required, in bytes.
*
* Return(s)   : None.
*
* Note(s)     : (1) This macro can be overriden by defining it first in the compiler options.
*********************************************************************************************************
*/

#ifndef  PP_ALIGN

#if ((RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_ARMCC)              || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CCS)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_GNU)                || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_MPLAB_C30)          || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CROSSCORE_BLACKFIN) || \
     (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VDSP))
#define  PP_ALIGN(_variable, _align)                    _variable __attribute__ ((aligned (_align)))
#elif (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_IAR)
#define  _PP_ALIGN_STR(x)                               #x
#define  PP_ALIGN(_variable, _align)                    _Pragma(_PP_ALIGN_STR(data_alignment = _align))\
                                                        _variable
#elif (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_RXC)
#warning  "Explicit variable alignment unsupported in RXC."
#elif (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VISUALSTUDIO)
#define  PP_ALIGN(_variable, _align)                    __declspec(align(_align)) _variable
#else
#error  "Unknown toolchain. Will define PP_ALIGN to nothing."
#define  PP_ALIGN(_variable, _align)
#endif

#endif


/*
*********************************************************************************************************
*                                              PP_WEAK()
*
* Description : Declares a symbol as weak.
*
* Argument(s) : type            Symbol's type.
*
*               name            Symbol's name.
*
*               init_value      Symbol's initial value.
*
* Return(s)   : None.
*
* Note(s)     : (1) This macro can be overriden by defining it first in the compiler options.
*
*               (2) Some toolchain won't allow to have a strong and a weak definition of a given symbol
*                   in the same compilation unit.
*
*               (3) Some toolchain do not support weak symbols. In those cases, a strong definition is
*                   required.
*
*               (4) Some toolchain require the use of #pragma to declare a symbol as weak. If _Pragma is
*                   not available (i.e. C99 is disabled), a strong definition is required.
*
*               (5) A bug exists in some version of IAR where the compiler can make some optimizations using
*                   a weak definition even though a strong definition exists somewhere else. This has
*                   been observed only when the type is a structure. Keep that in mind if you use this macro.
*                   For instance, using the volatile type qualifier in the weak definition prevents this
*                   optimization.
*********************************************************************************************************
*/

#ifndef  PP_WEAK

#if (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_IAR)
                                                                /* See note (5).                                        */
#define  PP_WEAK(type, name, init_value)  __weak  type  name = init_value
#elif ((RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_ARMCC) || \
       (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_MPLAB_C30))
#define  PP_WEAK(type, name, init_value)  type  __attribute__ ((weak))  name = init_value

#elif (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_GNU)
#define  PP_WEAK(type, name, init_value)  extern  type name  __attribute__((weak))

#elif ((RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CROSSCORE_BLACKFIN) || \
       (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VDSP))
#ifdef PP_C_STD_VERSION_C99_PRESENT
#define  PP_WEAK(type, name, init_value)  _Pragma("weak_entry")  \
                                          type  name = init_value
#else
#define  PP_WEAK(type, name, init_value)  extern type name
#endif

#elif (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CCS)
#if (defined(PP_C_STD_VERSION_C99_PRESENT) && \
     defined(__TI_ARM__))
#define  _PP_WEAK_STR(x)                  #x
#define  PP_WEAK(type, name, init_value)  _Pragma(_PP_WEAK_STR(WEAK (name)))  \
                                          type  name = init_value
#else
#define  PP_WEAK(type, name, init_value)  extern type name
#endif

#elif ((RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_RXC) ||            \
       (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VISUALSTUDIO))
#define  PP_WEAK(type, name, init_value)  extern type name
#else
#define  PP_WEAK(type, name, init_value)  extern type name
#endif

#endif


/*
*********************************************************************************************************
*                                         ISR-RELATED MACROS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             PP_ISR_DECL()
*
* Description : Declare a function, indicating to the compiler it is an ISR.
*
* Argument(s) : _isr    The ISR function's name.
*
* Return(s)   : None.
*
* Note(s)     : (1) This macro can be overriden by defining it first in the compiler options.
*
*               (2) Usage is as follows:
*                   PP_ISR_DECL(My_ISR_Function);
*********************************************************************************************************
*/

#ifndef  PP_ISR_DECL

#if ((RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMUL_POSIX) || \
     (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMUL_WIN32) || \
     (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMPTY))
#define  PP_ISR_DECL(_isr)                              void _isr(void)
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_GNU)
#if    (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR)
#define  PP_ISR_DECL(_isr)                              void _isr(void) __attribute__ ((interrupt ("IRQ")))
#elif  (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
#define  PP_ISR_DECL(_isr)                              void _isr(void)
#else
#warning  "Toolchain does not define PP_ISR_DECL."
#endif
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_ARMCC)
#if   ((RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR) || \
       (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M))
#if    (__ARMCC_VERSION < 6050000)
#define  PP_ISR_DECL(_isr)                              __irq void _isr(void)
#else
#define  PP_ISR_DECL(_isr)                              void _isr(void) __attribute__ ((interrupt ("IRQ")))
#endif
#else
#warning  "Toolchain does not define PP_ISR_DECL."
#endif
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CCS)
#if    (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR)
#define  PP_ISR_DECL(_isr)                              __interrupt void _isr(void)
#elif  (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
#define  PP_ISR_DECL(_isr)                              void _isr(void)
#else
#warning  "Toolchain does not define PP_ISR_DECL."
#endif
#elif ((RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_MPLAB_C30)          || \
       (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CROSSCORE_BLACKFIN) || \
       (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VDSP))
#warning  "Toolchain does not define PP_ISR_DECL."
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_IAR)
#if    (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR)
#define  PP_ISR_DECL(_isr)                              __irq void _isr(void)
#elif  (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)
#define  PP_ISR_DECL(_isr)                              void _isr(void)
#elif  (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_RENESAS_RX)
#define  PP_ISR_DECL(_isr)                              __interrupt void _isr(void)
#else
#warning  "Toolchain does not define PP_ISR_DECL."
#endif
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_RXC)
#define  PP_ISR_DECL(_isr)                              void _isr(void)
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VISUALSTUDIO)
#define  PP_ISR_DECL(_isr)
#else
#error  "Unknown toolchain does not define PP_ISR_DECL."
#endif

#endif


/*
*********************************************************************************************************
*                                             PP_ISR_DEF()
*
* Description : Define a function, indicating to the compiler that it is an ISR.
*
* Argument(s) : _isr    The ISR function.
*
* Return(s)   : None.
*
* Note(s)     : (1) This macro can be overriden by defining it first in the compiler options.
*
*               (2) Usage is as follows:
*                   PP_ISR_DEF(My_ISR_Function)
*                   {
*                       [...]
*                   }
*********************************************************************************************************
*/

#ifndef  PP_ISR_DEF

#if ((RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMUL_POSIX) || \
     (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMUL_WIN32) || \
     (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_EMPTY))
#define  PP_ISR_DEF(_isr)                               PP_ISR_DECL(_isr)
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_GNU)
#if   ((RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR) || \
       (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M))
#define  PP_ISR_DEF(_isr)                               PP_ISR_DECL(_isr)
#else
#warning  "Toolchain does not define PP_ISR_DEF."
#endif
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_ARMCC)
#if   ((RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR) || \
       (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M))
#define  PP_ISR_DEF(_isr)                               PP_ISR_DECL(_isr)
#else
#warning  "Toolchain does not define PP_ISR_DEF."
#endif
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CCS)
#if   ((RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR) || \
       (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M))
#define  PP_ISR_DEF(_isr)                               PP_ISR_DECL(_isr)
#else
#warning  "Toolchain does not define PP_ISR_DEF."
#endif
#elif ((RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_MPLAB_C30)          || \
       (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_CROSSCORE_BLACKFIN) || \
       (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VDSP))
#warning  "Toolchain does not define PP_ISR_DEF."
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_IAR)
#if   ((RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_AR) || \
       (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_ARM_V7_M)  || \
       (RTOS_CPU_PORT_NAME == RTOS_CPU_SEL_RENESAS_RX))
#define  PP_ISR_DEF(_isr)                               PP_ISR_DECL(_isr)
#else
#warning  "Toolchain does not define PP_ISR_DEF."
#endif
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_RXC)
#define  _PP_ISR_DEF_STR(x)                             #x
#define  PP_ISR_DEF(_isr)                               _Pragma(_PP_ISR_DEF_STR(interrupt _isr))\
                                                        PP_ISR_DECL(_isr)
#elif  (RTOS_TOOLCHAIN == RTOS_TOOLCHAIN_VISUALSTUDIO)
#define  PP_ISR_DEF(_isr)
#else
#error  "Unknown toolchain does not define PP_ISR_DEF."
#endif

#endif


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif                                                      /* End of preprocessor module include.                      */
