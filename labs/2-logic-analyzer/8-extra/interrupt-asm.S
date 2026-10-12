#include "rpi-asm.h"

@ just use the default int stack address.
#define INT_STACK INT_STACK_ADDR



@ <default_vec_ints> jumps to this trampoline when we get 
@ an interrupt.    
@
@ the trampoline sets up the interrupt stack, saves state 
@ and calls the C routine <int_vector>.  when that returns 
@ it restores state and jumps back to interrupted instruction.
@
@ this is the only assembly you have to modify (initially).
interrupt:
        sub   lr, lr, #4               @ correct interrupt pc
        mov sp, #INT_STACK             @ load the stack pointer
        push  {r0-r12,lr}              @ push all regs (trim this to caller)
        mov   r0, lr                   @ pass exception pc as arg0
        bl    int_vector               @ call our interrupt handler.
        pop   {r0-r12,lr}              @ pop all the regs
        movs    pc, lr                 @ resume back at exception location.

.align 5; 
.globl default_vec_ints
default_vec_ints: 
    b reset
    b undef
    b syscall
    b prefetch_abort
    b data_abort
    b reset
    @ b interrupt
    @ b int_vector
    b reset
    str event0_val, [event0]
    mcr p15, 0, event0_val, c7, c10, 4
    nop
    nop
    nop
    nop
    nop
    nop

    @ldr r0, [fiq_cnt_reg]
    @add r0, r0, #1
    @str r0, [fiq_cnt_reg]
    add fiq_cnt, fiq_cnt, #1
    msr cpsr, r3
    mrc p15, 0, r0, c15, c12, 1
    sub r0, r0, r2
    @# add cnt, cnt, #1
    bx lr

    8060:	e92d000f 	push	{r0, r1, r2, r3}
    8064:	ee1f1f3c 	mrc	15, 0, r1, cr15, cr12, {1}
    8068:	e3a02000 	mov	r2, #0, 0
    806c:	ee072fba 	mcr	15, 0, r2, cr7, cr10, {5}
    8070:	ee1d3f90 	mrc	15, 0, r3, cr13, cr0, {4}
    8074:	e5930000 	ldr	r0, [r3]
    8078:	ee1d3f50 	mrc	15, 0, r3, cr13, cr0, {2}
    807c:	e8830003 	stm	r3, {r0, r1}
    8080:	e2833008 	add	r3, r3, #8, 0
    8084:	ee0d3f50 	mcr	15, 0, r3, cr13, cr0, {2}
    8088:	ee072fba 	mcr	15, 0, r2, cr7, cr10, {5}
    808c:	ee1d3f70 	mrc	15, 0, r3, cr13, cr0, {3}
    8090:	e3a01302 	mov	r1, #134217728	; 0x8000000
    8094:	e5831000 	str	r1, [r3]
    8098:	ee072fba 	mcr	15, 0, r2, cr7, cr10, {5}
    809c:	e8bd000f 	pop	{r0, r1, r2, r3}
    80a0:	e25ef004 	subs	pc, lr, #4, 0

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ these are trampolines for unexpected exceptions: they just
@ die with an error.  you don't have to modify them.

#define MK_TRAMPOLINE(name, offset, fn)         \
    name:                                       \
        sub   lr, lr, # (offset);               \
        mov sp, #INT_STACK;               \
        push  {r0-r12,lr};                \
        mov   r0, lr;                     \
        bl    fn;                         \
        pop   {r0-r12,lr};                \
        movs    pc, lr


@ call the default handlers and blow up.  override if you
@ want to do something fancier.
MK_TRAMPOLINE(reset,            4, reset_vector)
MK_TRAMPOLINE(undef,            4, undefined_instruction_vector)
MK_TRAMPOLINE(prefetch_abort,   4, prefetch_abort_vector)
MK_TRAMPOLINE(data_abort,       8, data_abort_vector)
MK_TRAMPOLINE(syscall,          4, syscall_vector)
MK_FN(interrupt_setup_stack)
    @ switch to IRQ mode
    CPS #IRQ_MODE
    prefetch_flush(r3);

    @ set the IRQ stack
    mov sp, #INT_STACK

    @ switch back to super mode.
    CPS #SUPER_MODE
    prefetch_flush(r3);
    bx lr

MK_FN(fiq_setup_stack)
    @ switch to IRQ mode
    CPS #FIQ_MODE
    prefetch_flush(r3);

    @ set the IRQ stack
    mov sp, #INT_STACK

    @ switch back to super mode.
    CPS #SUPER_MODE
    prefetch_flush(r3);
    bx lr


.align 5;
.globl fiq_init
fiq_init:
    CPS #17  @ switch to fiq mode
    prefetch_flush(r3)
    mov event0, r0
    mov event0_val, r1
    mov one, #0
    @mov cnt, #0
    mov fiq_cnt_reg, r2
    mov fiq_cnt, #0
    CPS #19 @ bac to super
    prefetch_flush(r3)
    bx lr
