#include "rpi-asm.h"

@ just use the default libpi interrupt stack address.
#define INT_STACK INT_STACK_ADDR

@ register r8 holds the event clear address
#define event0      r8
@ register r9 holds the value (1<<in_pin) that 
@ we write to event clear address
#define event0_val  r9
@ register r10 holds the value 1 that we write to 
@ the scratch register to indicate an interrupt happened
#define one         r10


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

    @ str r9, [r8]
    @ mcr 15, 0, sl, c13, c0, 2
    @ subs pc, lr, #4

    str event0_val, [event0]
    msr cpsr_c, r3
    mrc p15, 0, r0, c15, c12, 1
    sub r0, r0, r2
    bx lr
.align 5;
.globl measure_int_asm
measure_int_asm:
    mrs r3, cpsr
    mrc p15, 0, r2, c15, c12, 1
    str r1, [r0]
loop:
    b loop

.globl prefetch_end
prefetch_end:
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ these are trampolines for unexpected exceptions: they just
@ die with an error.  you don't have to modify them.

.align 5;
.globl fiq_init
fiq_init:
    CPS #17  @ switch to fiq mode
    mov event0, r0
    mov event0_val, r1
    mov one, #0
    CPS #19 @ bac to super
    bx lr

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
