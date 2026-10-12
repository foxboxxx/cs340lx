#include "rpi-asm.h"

@ just use the default int stack address.
#define INT_STACK INT_STACK_ADDR
#define event0 r8
#define event0_val r9
#define trp r10
#define lev r11
#define cyc r12
#define fiq_count sp

@ <default_vec_ints> jumps to this trampoline when we get 
@ an interrupt.    
@
@ the trampoline sets up the interrupt stack, saves state 
@ and calls the C routine <int_vector>.  when that returns 
@ it restores state and jumps back to interrupted instruction.
@
@ this is the only assembly you have to modify (initially).
.align 5; 
.globl default_vec_ints
default_vec_ints: 
    b reset
    b undef
    b syscall
    b prefetch_abort
    b data_abort
    b reset
    b reset
    @b fiq_handler
    mrc p15, 0, cyc, c15, c12, 1 @ cycle counter
    str event0_val, [event0] @ clear event
    ldr lev, [event0, #-12] 
    stmia trp!, {lev, cyc}
    mcr p15, 0, trp, c13, c0, 2
    add fiq_count, fiq_count, #1
    subs pc, lr, #4
.globl prefetch_end
prefetch_end:
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ these are trampolines for unexpected exceptions: they just
@ die with an error.  you don't have to modify them.
MK_FN(fiq_init)
    @ switch to IRQ mode
    CPS #FIQ_MODE
    prefetch_flush(r3);
    ldr event0, =0x20200040
    mov event0_val, r1
    mov trp, r0
    mov fiq_count, #0
    @ set the IRQ stack
    mcr p15, 0, r0, c13, c0, 2

    @ switch back to super mode.
    CPS #SUPER_MODE
    prefetch_flush(r3);
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
MK_FN(fiq_count_get)
  CPS #FIQ_MODE
  prefetch_flush(r3);
  mov r0, fiq_count
  CPS #SUPER_MODE
  prefetch_flush(r3);
  bx lr


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


