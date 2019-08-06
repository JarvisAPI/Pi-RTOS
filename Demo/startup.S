@ vim:ft=armv5

#include "PureFreeRTOSConfig.h"
#include <Drivers/rpi_memory.h>

.extern __bss_start
.extern __bss_end
.extern FreeRTOS_IRQ_Handler
.extern FreeRTOS_SVC_Handler
.extern rpi_cpu_irq_disable
.extern main
.extern core_init   
#if configUSE_GLOBAL_EDF == 1
.extern MultiCore_Undefined_Handler
#endif /* configUSE_GLOBAL_EDF */
.extern irq_c_handler
.extern irq_s_handler
.section .init
.globl _start
;; 
_start:
    ;@ All the following instruction should be read as:
    ;@ Load the address at symbol into the program counter.

    ldr pc,reset_handler                ;@  Processor Reset handler         -- we will have to force this on the raspi!
    ldr pc,undefined_handler            ;@  Undefined instruction handler   -- processors that don't have thumb can emulate thumb!
    ldr pc,swi_handler                  ;@  Software interrupt / TRAP (SVC) -- system SVC handler for switching to kernel mode.
    ldr pc,prefetch_handler             ;@  Prefetch/abort handler.
    ldr pc,data_handler                 ;@  Data abort handler/
    ldr pc,unused_handler               ;@  -- Historical from 26-bit addressing ARMs -- was invalid address handler.
    ldr pc,irq_handler                  ;@  IRQ handler
    ldr pc,fiq_handler                  ;@  Fast interrupt handler.

    ;@ Here we create an exception address table! This means that reset/hang/irq can be absolute addresses
reset_handler:      .word reset
#if configUSE_GLOBAL_EDF == 1
undefined_handler:  .word MultiCore_Undefined_Handler
#else /* configUSE_GLOBAL_EDF */
undefined_handler:  .word undefined_instruction
#endif /* configUSE_GLOBAL_EDF */
swi_handler:        .word FreeRTOS_SVC_Handler
prefetch_handler:   .word prefetch_abort
data_handler:       .word data_abort
unused_handler:     .word unused
irq_handler:        .word FreeRTOS_IRQ_Handler
fiq_handler:        .word fiq

reset:
    /* Disable IRQ & FIQ */
    cpsid if

    /* Check for HYP mode */
    mrs r0, cpsr_all
    and r0, r0, #0x1F
    mov r8, #0x1A
    cmp r0, r8
    beq overHyped
    b continueBoot

overHyped: 
    ldr r1, =continueBoot
    msr ELR_hyp, r1
    mrs r1, cpsr_all
    and r1, r1, #0x1f   ;@ CPSR_MODE_MASK
    orr r1, r1, #0x13   ;@ CPSR_MODE_SUPERVISOR
    msr SPSR_hyp, r1
    eret

continueBoot:
    ;@  In the reset handler, we need to copy our interrupt vector table to 0x0000, its currently at 0x8000

    mov r0,#0x8000  ;@ Store the source pointer
    mov r1,#0x0000  ;@ Store the destination pointer.

    ;@  Here we copy the branching instructions
    ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9} ;@ Load multiple values from indexed address.   ; Auto-increment R0
    stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9} ;@ Store multiple values from the indexed address.  ; Auto-increment R1

    ;@  So the branches get the correct address we also need to copy our vector table!
    ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9} ;@ Load from 4*n of regs (8) as R0 is now incremented.
    stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9} ;@ Store this extra set of data.

    cps #0x11   /* set fiq mode */
    ldr sp, =MEM_FIQ_STACK
    cps #0x12   /* set irq mode */
    ldr sp, =MEM_IRQ_STACK
    cps #0x13   /* set svc mode */
    ldr sp, =MEM_SVC_STACK
    cps #0x17   /* set abort mode */
    ldr sp, =MEM_ABORT_STACK
    cps #0x1B   /* set "undefined" mode */
    ldr sp, =MEM_ABORT_STACK
    cps #0x1F   /* set system mode */
    ldr sp, =MEM_KERNEL_STACK

    ldr r0, =__bss_start
    ldr r1, =__bss_end

    mov r2, #0

zero_loop:
    cmp r0,r1
    it lt
    strlt r2,[r0], #4
    blt zero_loop

    bl  rpi_cpu_irq_disable

    /* Enable flaoting point unit */
    mrc p15, 0, r0, c1, c0, 2
    orr r0, r0, #0x300000            @ single precision
    orr r0, r0, #0xC00000            @ double precision
    mcr p15, 0, r0, c1, c0, 2
    mov r0, #0x40000000
    fmxr fpexc,r0

    b main  ;@ We're ready?? Lets start main execution!

.section .text

.global core_wakeup
core_wakeup:
    mrc p15, 0, r0, c0, c0, 5           /* read MPIDR */
    and r0, r0, #CORES-1                /* get CPU ID */

    mov r1, #EXCEPTION_STACK_SIZE       /* calculate exception stack offset for core */
    mul r1, r0, r1
    cps #0x11                           /* set fiq mode */
    ldr r2, =MEM_FIQ_STACK
    add sp, r1, r2
    cps #0x12                           /* set irq mode */
    ldr r2, =MEM_IRQ_STACK
    add sp, r1, r2
    cps #0x13                           /* set irq mode */
    ldr r2, =MEM_SVC_STACK
    add sp, r1, r2
    cps #0x17                           /* set abort mode */
    ldr r2, =MEM_ABORT_STACK
    add sp, r1, r2
    cps #0x1B                           /* set "undefined" mode */
    add sp, r1, r2
    mov r1, #KERNEL_STACK_SIZE          /* calculate kernel stack offset for core */
    mul r1, r0, r1
    cps #0x1F                           /*  set system mode */
    ldr r2, =MEM_KERNEL_STACK
    add sp, r1, r2

    /* Enable flaoting point unit */
    mrc p15, 0, r0, c1, c0, 2
    orr r0, r0, #0x300000            @ single precision
    orr r0, r0, #0xC00000            @ double precision
    mcr p15, 0, r0, c1, c0, 2
    mov r0, #0x40000000
    fmxr fpexc,r0

    bl core_init
    bl hang

undefined_instruction:
    b undefined_instruction

prefetch_abort:
    b prefetch_abort

data_abort:
    b data_abort

unused:
    b unused

fiq:
    b fiq

hang:
    b hang

.global CPUID
CPUID:
    mrc   p15, 0, r1, c0, c0, 5          @ Read Multiprocessor Affinity Register
    and   r0, r1, #0x3                   @ Extract CPU ID bits
    bx lr

.global GETSP
GETSP:
    mov r0, pc
    bx lr

.globl PUT32
PUT32:
    str r1,[r0]
    bx lr

.globl GET32
GET32:
    ldr r0,[r0]
    bx lr

.globl dummy
dummy:
    bx lr

