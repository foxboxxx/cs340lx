#include "rpi-asm.h"

@ just use the default libpi interrupt stack address.
#define INT_STACK INT_STACK_ADDR

@ <default_vec_ints> jumps to this trampoline when we get 
@ an interrupt.    
@
@ the trampoline sets up the interrupt stack, saves state 
@ and calls the C routine <int_vector>.  when that returns 
@ it restores state and jumps back to interrupted instruction.
@
@ this is the only assembly you have to modify (initially).
.align 5;
interrupt:
        sub   lr, lr, #4               @ correct interrupt pc
        mov sp, #INT_STACK             @ load the stack pointer
        push  {r2, r3}              @ push all regs (trim this to caller)
        mov	r3, #0, 0
        mcr	15, 0, r3, cr13, c0, 2
        mrc	15, 0, r3, cr13, c0, 3
        mov	r2, #134217728 @ 0x8000000
        str	r2, [r3]
        pop   {r2, r3}              @ pop all the regs
        movs    pc, lr                 @ resume back at exception location.

.align 5; 
.globl default_vec_ints
default_vec_ints: 
    @ see below: these jump to default libpi handlers and panic.
    b reset
    b undef
    b syscall
    b prefetch_abort
    b data_abort
    b reset
    @ the only exception/int we expect: branches to the
    @ trampoline above.
    @ b interrupt  
    mov sp, #INT_STACK             @ load the stack pointer
    push  {r2, r3}              @ push all regs (trim this to caller)
    mov	r3, #0, 0
    mcr	15, 0, r3, cr13, c0, 2
    mrc	15, 0, r3, cr13, c0, 3
    mov	r2, #134217728 @ 0x8000000
    str	r2, [r3]
    pop   {r2, r3}              @ pop all the regs
    subs pc, lr, #4

@ register r8 holds the event clear address
#define event0      r8
@ register r9 holds the value (1<<in_pin) that 
@ we write to event clear address
#define event0_val  r9
@ register r10 holds the value 1 that we write to 
@ the scratch register to indicate an interrupt happened
#define one         r10

.align 5;
.globl fiq_init
fiq_init:
    CPS #17  @ switch to fiq mode
    mov event0, r0
    mov event0_val, r1
    mov one, #0
    CPS #19
    bx lr

.align 5;
.globl fiq_ints
fiq_ints: 
    @ see below: these jump to default libpi handlers and panic.
    b reset
    b undef
    b syscall
    b prefetch_abort
    b data_abort
    b reset
    b reset
    str r9, [r8]
    mcr 15, 0, sl, c13, c0, 2
    subs pc, lr, #4
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


@ call the default libpi handlers, which panic with an error 
@ message.  override if you want to do something fancier.
MK_TRAMPOLINE(reset,            4, reset_vector)
MK_TRAMPOLINE(undef,            4, undefined_instruction_vector)
MK_TRAMPOLINE(prefetch_abort,   4, prefetch_abort_vector)
MK_TRAMPOLINE(data_abort,       8, data_abort_vector)
MK_TRAMPOLINE(syscall,          4, syscall_vector)
