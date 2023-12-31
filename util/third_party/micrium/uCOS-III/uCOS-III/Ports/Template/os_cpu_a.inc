;********************************************************************************************************
;                                               uC/OS-III
;                                         The Real-Time Kernel
;
;                          (c) Copyright 2009-2016; Micrium, Inc.; Weston, FL
;                    All rights reserved.  Protected by international copyright laws.
;
;                                       ASSEMBLY LANGUAGE MACROS
;
;                                      $$$$ Processor Name
;                                      $$$$ Compiler/Assembler Name
;
; Filename     : os_cpu_a.asm
; Version      : $$$$ V3.06.01
; By           : $$$$ JJL
;********************************************************************************************************

;********************************************************************************************************
;                                           PUBLIC FUNCTIONS
;
; $$$$ Declare the global variables as well as public functions here using the proper assembler directive
;********************************************************************************************************

;$$$$    extern     _OSIntNestingCtr            ; Declared as 'CPU_INT08U'
;$$$$    extern     _OSTCBCurPtr                ; Declared as 'OS_TCB *'  
    
;$$$$    extern     _OSTimeTick					; OSTimeTick()
;$$$$    extern     _OSIntExit                  ; OSIntExit()
    
;/*$PAGE*/
;********************************************************************************************************
;                                             OS_CTX_SAVE
;
; Description : This MACRO saves the CPU registers (i.e. CPU context) onto the current task's stack using 
;               the same order as they were saved in OSTaskStkInit().
;********************************************************************************************************

OS_CTX_SAVE  MACRO
;$$$$ 
    
    ENDM
    
;/*$PAGE*/
;********************************************************************************************************
;                                           OS_CTX_RESTORE
;
; Description : This MACRO restores the CPU registers (i.e. context) from the new task's stack in the 
;               reverse order of OS_CTX_SAVE (see above)
;********************************************************************************************************

OS_CTX_RESTORE  MACRO
;$$$$
    
    ENDM
    
;/*$PAGE*/
;********************************************************************************************************
;                                            OS_ISR_ENTER
;
; Description : Interrupt service routine prologue for kernel-aware handler.
;               This macro implements the following code in assembly language:
;
;                  OS_ISR_ENTER
;                      OS_CTX_SAVE                   ; Call the macro: OS_CTX_SAVE
;                      OSIntNestingCtr++;
;                      if (OSIntNestingCtr == 1) {
;                          OSTCBCurPtr->StkPtr = SP;
;                      }
;
;               This MACRO is to be used by your assembly language based ISRs as follows:
;
;                  MyISR
;                      OS_ISR_ENTER
;                      ISR Body here
;                      OS_ISR_EXIT
;********************************************************************************************************

OS_ISR_ENTER  MACRO
    OS_CTX_SAVE                                      ; Save processor registers on the stack
;$$$$                                                ; Implement: 'OSIntNestingCtr++;' 
;$$$$                                                ; Implement: 'if (OSIntNestingCtr == 1)'
;$$$$                                                ; Implement: '    OSTCBCurPtr->StkPtr = SP'

    ENDM
    
;/*$PAGE*/
;********************************************************************************************************
;                                             OS_ISR_EXIT
;
; Description : Interrupt service routine epilog for kernel-aware handler.
;               This macro implements the following code in assembly language:
;
;                  OS_ISR_EXIT:
;                      OSIntExit();					 ; Call the C function: OSIntExit();
;                      OS_CTX_RESTORE       		 ; Call the macro: OS_CTX_RESTORE
;					   Return from interrupt         ; CPU instruction to return from interrupt/exception
;********************************************************************************************************

OS_ISR_EXIT  MACRO
;$$$$                                                ; Call 'OSIntExit()'
    OS_CTX_RESTORE                                   ; Restore processor registers from stack
;$$$$                                                ; CPU instruction to return from Interrupt/exception
    							                     
    ENDM