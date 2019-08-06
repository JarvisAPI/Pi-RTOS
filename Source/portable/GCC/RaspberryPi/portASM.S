/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved


    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/
#include "PureFreeRTOSConfig.h"
	.text
	.arm

	.set SYS_MODE,	0x1f
	.set SVC_MODE,	0x13
	.set IRQ_MODE,	0x12
	/* Variables and functions. */
	.extern ulMaxAPIPriorityMask
	.extern _freertos_vector_table
	.extern pxCurrentTCB
	.extern vTaskSwitchContext
	.extern vApplicationIRQHandler
	.extern ulPortInterruptNesting
	.extern ulPortTaskHasFPUContext
	.extern ulICCEOIR
	.extern ulPortYieldRequired
#if configUSE_SHARED_RUNTIME_STACK == 1
    .extern pxTempStack
#endif
#if configUSE_GLOBAL_EDF == 1
    .extern ulSecondaryCoreIRQHandler
    .extern xGetCoreCurrentTask
    .extern xPopCorePendingTask
    .extern core_get_mailbox_interrupt_source
    .extern vKernelCoreIRQHandler
    .extern vSecondaryCoreAfterSaveContext        
    .extern debug
    .extern debug2
    
    .global MultiCore_Undefined_Handler
    .global vPortSecondaryRestoreTaskContext
#endif
    
	.global FreeRTOS_IRQ_Handler
	.global FreeRTOS_SVC_Handler
	.global vPortRestoreTaskContext

#if configUSE_SHARED_RUNTIME_STACK == 1
    .macro srpSWITCH_STACK
    /* Switch to temporary stack */
    MOV     R0, SP
    LDR     SP, pxTempStackConst
    LDR     R1, [SP]
    LDR     SP, [R1]
    PUSH    {R0}
    .endm

    .macro srpRESTORE_STACK
    POP     {R0}
    MOV     SP, R0
    .endm
#endif
    
    .macro portSAVE_CONTEXT
    
	/* Save the LR and SPSR onto the system mode stack before switching to
	system mode to save the remaining system mode registers. */
	SRSDB	sp!, #SYS_MODE
	CPS		#SYS_MODE
	PUSH	{R0-R12, R14}
    
	/* Push the critical nesting count. */
    MRC     P15, 0, R4, C0, C0, 5          @ Read Multiprocessor Affinity Register
    AND     R4, R4, #0x3                   @ Extract CPU ID bits

	LDR		R2, ulCriticalNestingConst
    ADD     R2, R2, R4, lsl #2
	LDR		R1, [R2]
	PUSH	{R1}

	/* Does the task have a floating point context that needs saving?  If
	ulPortTaskHasFPUContext is 0 then no. */
	LDR		R2, ulPortTaskHasFPUContextConst
    ADD     R2, R2, R4, lsl #2    
	LDR		R3, [R2]
	CMP		R3, #0

	/* Save the floating point context, if any. */
	FMRXNE  R1,  FPSCR
	VPUSHNE {D0-D15}
#if configFPU_D32 == 1
	VPUSHNE	{D16-D31}
#endif /* configFPU_D32 */
	PUSHNE	{R1}

	/* Save ulPortTaskHasFPUContext itself. */
	PUSH	{R3}

	/* Save the stack pointer in the TCB. */
	LDR		R0, pxCurrentTCBConst
    ADD     R0, R0, R4, lsl #2    
	LDR		R1, [R0]
	STR		SP, [R1]

	.endm

; /**********************************************************************/

.macro portRESTORE_CONTEXT

	/* Set the SP to point to the stack of the task being restored. */
    MRC     P15, 0, R4, C0, C0, 5          @ Read Multiprocessor Affinity Register
    AND     R4, R4, #0x3                   @ Extract CPU ID bits
    
	LDR		R0, pxCurrentTCBConst
    ADD     R0, R0, R4, lsl #2
	LDR		R1, [R0]
	LDR		SP, [R1]

	/* Is there a floating point context to restore?  If the restored
	ulPortTaskHasFPUContext is zero then no. */
	LDR		R0, ulPortTaskHasFPUContextConst
    ADD     R0, R0, R4, lsl #2    
	POP		{R1}
	STR		R1, [R0]
	CMP		R1, #0

	/* Restore the floating point context, if any. */
	POPNE 	{R0}
#if configFPU_D32 == 1
	VPOPNE	{D16-D31}
#endif /* configFPU_D32 */
	VPOPNE	{D0-D15}
	VMSRNE  FPSCR, R0

	/* Restore the critical section nesting depth. */
	LDR		R0, ulCriticalNestingConst
    ADD     R0, R0, R4, lsl #2    
	POP		{R1}
	STR		R1, [R0]

	/* Restore all system mode registers other than the SP (which is already
	being used). */
	POP		{R0-R12, R14}

////
//    PUSH {R1}
  //  ADD R1, SP, #4
    //PUSH {R0-R3, R12}
    //LDR R0, [R1]
    //BL debug
    //POP {R0-R3, R12}
    //POP {R1}
////        
    

	/* Return to the task code, loading CPSR on the way. */
	RFEIA	sp!

	.endm


/*****************************************************************************
 * Global EDF MultiCore
 *****************************************************************************/

#if configUSE_GLOBAL_EDF == 1
.macro portSECONDARY_SAVE_CONTEXT 
    
	/* Save the LR and SPSR onto the system mode stack before switching to
	system mode to save the remaining system mode registers. */
	SRSDB	sp!, #SYS_MODE
	CPS		#SYS_MODE
	PUSH	{R0-R12, R14}

	/* Push the critical nesting count. */
	//LDR		R2, ulCriticalNestingConst
	//LDR		R1, [R2]
    MOV R1, #0 // TODO: Move zero for multiprocessor for now
	PUSH	{R1}

    // TODO: No floating point support for tasks in multicore global EDF.
    
    //   If floating point is enabled for a task, behavior would be undefined.
    /* Does the task have a floating point context that needs saving?  If
	ulPortTaskHasFPUContext is 0 then no. */
	//LDR		R2, ulPortTaskHasFPUContextConst
	//LDR		R3, [R2]
    MOV R3, #0 // TODO: Change to allow floating point context to be saved
	//CMP		R3, #0

	/* Save the floating point context, if any. */
	//FMRXNE  R1,  FPSCR
	//VPUSHNE {D0-D15}
    //#if configFPU_D32 == 1
	//VPUSHNE	{D16-D31}
    //#endif /* configFPU_D32 */
	//PUSHNE	{R1}
    
	/* Save ulPortTaskHasFPUContext itself. */
	PUSH	{R3}

	/* Save the stack pointer in the TCB. */
    LDR R0, xGetCoreCurrentTaskConst
    BLX R0
	STR		SP, [R0]
    
    .endm


.macro portSECONDARY_RESTORE_CONTEXT
    
	/* Set the SP to point to the stack of the task being restored. */
	LDR		R0, xPopCorePendingTaskConst
    BLX     R0    
	LDR		SP, [R0]

	/* Is there a floating point context to restore?  If the restored
	ulPortTaskHasFPUContext is zero then no. */
	//LDR		R0, ulPortTaskHasFPUContextConst
	POP		{R1}
    // TODO: Currently ignoring floating point context
    MOV     R1, #0
	//STR		R1, [R0]
	CMP		R1, #0

	/* Restore the floating point context, if any. */
	//POPNE 	{R0}
    //#if configFPU_D32 == 1
	//VPOPNE	{D16-D31}
    //#endif /* configFPU_D32 */
	//VPOPNE	{D0-D15}
	//VMSRNE  FPSCR, R0

	/* Restore the critical section nesting depth. */
    // TODO: Change so we don't ignore nesting count?
	//LDR		R0, ulCriticalNestingConst
	POP		{R1}
	//STR		R1, [R0]

	/* Restore all system mode registers other than the SP (which is already
	being used). */
	POP		{R0-R12, R14}

////
    //PUSH {R0}
    //ADD R0, SP, #4
    //PUSH {R1-R12, R14}
    //BL debug
    //POP {R1-R12, R14}
    //POP {R0}
////        
    
	/* Return to the task code, loading CPSR on the way. */
	RFEIA	sp!

	.endm

.align 4
.type MultiCore_Undefined_Handler, %function
MultiCore_Undefined_Handler:
    //PUSH {r0-r12, r14}
    //BL debug2
    //POP {r0-r12, r14}
    
    // Secondary cores need to save context of current task and then idle
    portSECONDARY_SAVE_CONTEXT

    BL vSecondaryCoreAfterSaveContext    
    
    portSECONDARY_RESTORE_CONTEXT    
    
#endif /* confiUSE_GLOBAL_EDF */   

/******************************************************************************
 * SVC handler is used to yield.
 *****************************************************************************/
.align 4
.type FreeRTOS_SVC_Handler, %function
FreeRTOS_SVC_Handler:
    
#if configUSE_SHARED_RUNTIME_STACK == 1
    PUSH    {R0, R1}
    LDR     R1, pxCurrentTCBConst
    LDR     R0, [R1]
    CMP     R0, #0
    POP     {R0, R1}    
    BEQ     SVC_Handler_after_save
#endif
        
	/* Save the context of the current task and select a new task to run. */
	portSAVE_CONTEXT
    
SVC_Handler_after_save:
#if configUSE_SHARED_RUNTIME_STACK == 1
    srpSWITCH_STACK
#endif        
	LDR R0, vTaskSwitchContextConst
	BLX	R0
#if configUSE_SHARED_RUNTIME_STACK == 1
    srpRESTORE_STACK
#endif    
    
	portRESTORE_CONTEXT


/******************************************************************************
 * vPortRestoreTaskContext is used to start the scheduler.
 *****************************************************************************/
.align 4
.type vPortRestoreTaskContext, %function
vPortRestoreTaskContext:
	/* Switch to system mode. */
	CPS		#SYS_MODE
	portRESTORE_CONTEXT

#if configUSE_GLOBAL_EDF == 1
vPortSecondaryRestoreTaskContext:
    CPS #SYS_MODE
    portSECONDARY_RESTORE_CONTEXT   
#endif /* configUSE_GLOBAL_EDF */

.align 4
.type FreeRTOS_IRQ_Handler, %function
FreeRTOS_IRQ_Handler:
	/* Return to the interrupted instruction. */
	SUB		lr, lr, #4

	/* Push the return address and SPSR. */
	PUSH	{lr}
	MRS		lr, SPSR
	PUSH	{lr}

#if configUSE_GLOBAL_EDF == 1
    // Save some registers so we can use
    PUSH {r0, r1, r2}
    // Get cpu id
    MRC p15, 0, r0, c0, c0, 5
    AND r0, r0, #0x3

    // TODO: Import the kernel core id from outside
    CMP r0, #0
    POP {r0, r1, r2}    
    // If not core 0, then we don't execute anything related to the
    // kernel.
    BEQ kernel_irq_begin
    
    PUSH {r0-r3, r12}    
    LDR r0, ulSecondaryCoreIRQHandlerConst
    BLX r0
    CMP r0, #0
    // If 0 then secondary core doesn't need to switch task, otherwise it needs to switch    
    BNE secondary_switch_before_exit

secondary_exit_without_swtich:
	POP		{r0-r3, r12}
	POP		{LR}
	MSR		SPSR_cxsf, LR
	POP		{LR}
	MOVS	PC, LR

secondary_switch_before_exit:   
	/* Restore used registers, LR-irq and SPSR before saving the context
	to the task stack. */
	POP		{r0-r3, r12}
	POP		{LR}
	MSR		SPSR_cxsf, LR    
	POP		{LR}
    
    portSECONDARY_SAVE_CONTEXT
    
    BL vSecondaryCoreAfterSaveContext
    
    portSECONDARY_RESTORE_CONTEXT
    
kernel_irq_begin:
    CPS #0x13    
    PUSH {r0-r3, r12}
    
    LDR R0, xCoreGetMailboxInterruptSourceConst
    BLX R0
    CMP R0, #0
    BEQ no_core_interrupt_to_process
    LDR R1, vKernelCoreIRQHandlerConst
    BLX R1
    
    B exit_without_switch
    
no_core_interrupt_to_process:
#else /* configUSE_GLOBAL_EDF */
    
	/* Change to supervisor mode to allow reentry. */
	CPS		#0x13

	/* Push used registers. */
	PUSH	{r0-r3, r12}

#endif /* configUSE_GLOBAL_EDF */

	/* Increment nesting count.  r3 holds the address of ulPortInterruptNesting
	for future use.  r1 holds the original ulPortInterruptNesting value for
	future use. */
    MRC p15, 0, R0, C0, C0, 5
    AND r0, r0, #0x3    
	LDR	r3, ulPortInterruptNestingConst
    ADD R3, R3, R0, lsl #2
    
	LDR		r1, [r3]
	ADD		r0, r1, #1
	STR		r0, [r3]

	/* Ensure bit 2 of the stack pointer is clear.  r2 holds the bit 2 value for
	future use. */
	MOV		r0, sp
	AND		r2, r0, #4
	SUB		sp, sp, r2

	/* Call the interrupt handler. */
	PUSH	{r0-r3, lr}
	LDR		r1, vApplicationIRQHandlerConst
	BLX		r1
	POP		{r0-r3, lr}
	ADD		sp, sp, r2

	CPSID	i
	DSB
	ISB

	/* Write to the EOI register. */
	LDR 	r0, ulICCEOIRConst
	LDR		r2, [r0]
	STR		r0, [r2]

	/* Restore the old nesting count. */
	STR		r1, [r3]

	/* A context switch is never performed if the nesting count is not 0. */
	CMP		r1, #0
	BNE		exit_without_switch

	/* Did the interrupt request a context switch?  r1 holds the address of
	ulPortYieldRequired and r0 the value of ulPortYieldRequired for future
	use. */
    MRC p15, 0, R0, C0, C0, 5
    AND r0, r0, #0x3    
	LDR R1, ulPortYieldRequiredConst
    ADD R1, R1, R0, lsl #2
    
	LDR		r0, [r1]
	CMP		r0, #0
	BNE		switch_before_exit

exit_without_switch:
	/* No context switch.  Restore used registers, LR_irq and SPSR before
	returning. */
	POP		{r0-r3, r12}
	CPS		#IRQ_MODE
	POP		{LR}
	MSR		SPSR_cxsf, LR
	POP		{LR}
	MOVS	PC, LR

switch_before_exit:
	/* A context swtich is to be performed.  Clear the context switch pending
	flag. */
	MOV		r0, #0
	STR		r0, [r1]

	/* Restore used registers, LR-irq and SPSR before saving the context
	to the task stack. */
	POP		{r0-r3, r12}
	CPS		#IRQ_MODE
	POP		{LR}
	MSR		SPSR_cxsf, LR
	POP		{LR}
    
#if configUSE_SHARED_RUNTIME_STACK == 1
    PUSH    {R0, R1}
    
    MRC p15, 0, R0, C0, C0, 5
    AND r0, r0, #0x3    
    LDR     R1, pxCurrentTCBConst
    ADD R1, R1, R0, lsl #2
    
    LDR     R0, [R1]
    CMP     R0, #0
    POP     {R0, R1}    
    BEQ     IRQ_Handler_after_save
#endif
        
	portSAVE_CONTEXT
#if configUSE_SHARED_RUNTIME_STACK == 1
IRQ_Handler_after_save:
    srpSWITCH_STACK
#endif
    
	/* Call the function that selects the new task to execute.
	vTaskSwitchContext() if vTaskSwitchContext() uses LDRD or STRD
	instructions, or 8 byte aligned stack allocated data.  LR does not need
	saving as a new LR will be loaded by portRESTORE_CONTEXT anyway. */
	LDR		R0, vTaskSwitchContextConst
	BLX		R0
#if configUSE_SHARED_RUNTIME_STACK == 1
    srpRESTORE_STACK
#endif    

	/* Restore the context of, and branch to, the task selected to execute
	next. */
	portRESTORE_CONTEXT

ulICCEOIRConst:	.word ulICCEOIR
pxCurrentTCBConst: .word pxCurrentTCB
ulCriticalNestingConst: .word ulCriticalNesting
ulPortTaskHasFPUContextConst: .word ulPortTaskHasFPUContext
vTaskSwitchContextConst: .word vTaskSwitchContext
vApplicationIRQHandlerConst: .word vApplicationIRQHandler
ulPortInterruptNestingConst: .word ulPortInterruptNesting
ulPortYieldRequiredConst: .word ulPortYieldRequired
#if configUSE_SHARED_RUNTIME_STACK == 1
pxTempStackConst:    .word pxTempStack
#endif /* configUSE_SHARED_RUNTIME_STACK */
#if configUSE_GLOBAL_EDF == 1
ulSecondaryCoreIRQHandlerConst:   .word ulSecondaryCoreIRQHandler
xGetCoreCurrentTaskConst: .word xGetCoreCurrentTask
xPopCorePendingTaskConst:   .word xPopCorePendingTask
xCoreGetMailboxInterruptSourceConst:    .word core_get_mailbox_interrupt_source
vKernelCoreIRQHandlerConst: .word vKernelCoreIRQHandler
#endif /* configUSE_GLOBAL_EDF */
.end





