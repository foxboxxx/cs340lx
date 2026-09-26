// Simple code to measure the time it takes to trigger and
// return from a GPIO interrupt.
//
// The file "interrupt-asm.S" has the support assembly code.
//
// You should put a loopback jumper between <in_pin> and 
// <out_pin> (definitions below).
//
// Lab: make this code much faster / accurate.
#include "rpi.h"
#include "timer-interrupt.h"
#include "cycle-count.h"
#include "vector-base.h"
#include "gpio-raw.h"
/* #include "asm-helpers.h" */

/* cp_asm_raw(cp15_scratch2, p15, 0, c13, c0, 3) */
/* cp_asm_raw(cp15_scratch1, p15, 0, c13, c0, 2) */

// Can change these pins to whatever you want.  
// 
// NOTE: some pin numbers let the compiler generate faster 
// code b/c it can load derived constants in fewer instructions
// [useful side quest: write some code to check this claim!]
enum { out_pin = 26, in_pin = 27 };

/* enum { */
/*     GPEDS0=0x20200040, */
/*     GPREN0=0x2020004c, */
/*     GPFEN0=0x20200058, */
/*     GPAREN0=0x2020007c, */
/*     GPAFEN0=0x20200088, */
/* }; */


// counters: only modified by the interrupt handler.
// since they are read by non-interrupt code, we must
// either use memory barriers or mark them as volatile.
/* static volatile unsigned n_rising_edge, n_falling_edge; */
/* static volatile unsigned n_interrupt; */

// interrupt handler: should only be called on gpio 
// transitions from 0->1 or 1->0, nothing else (no timer, 
// etc).  
// all it does:
//  1. increment appropriate counter;
//  2. clear the interrupt;
//  3. return.

/* enum { */
/*     IRQ_Base            = 0x2000b200, */
/*     IRQ_basic_pending   = IRQ_Base+0x00,    // 0x200 */
/*     IRQ_pending_1       = IRQ_Base+0x04,    // 0x204 */
/*     IRQ_pending_2       = IRQ_Base+0x08,    // 0x208 */
/*     IRQ_FIQ_control     = IRQ_Base+0x0c,    // 0x20c */
/*     IRQ_Enable_1        = IRQ_Base+0x10,    // 0x210 */
/*     IRQ_Enable_2        = IRQ_Base+0x14,    // 0x214 */
/*     IRQ_Enable_Basic    = IRQ_Base+0x18,    // 0x218 */
/*     IRQ_Disable_1       = IRQ_Base+0x1c,    // 0x21c */
/*     IRQ_Disable_2       = IRQ_Base+0x20,    // 0x220 */
/*     IRQ_Disable_Basic   = IRQ_Base+0x24,    // 0x224 */
/*     GPIO_INT0 = 49, */
/* }; */

void x_gpio_fiq_rising_edge(unsigned pin) {
    volatile unsigned* gpren0 = (volatile unsigned*)(0x2020004c);
    *gpren0 = *gpren0 | (0b1 << (pin & 31));
    *((volatile unsigned*)(0x2000b20c)) = 49 | (1 << 7);
}
void x_gpio_fiq_falling_edge(unsigned pin) {
    volatile unsigned* gpfen0 = (volatile unsigned*)(0x20200058);
    *gpfen0 = *gpfen0 | (0b1 << (pin & 31));
    *((volatile unsigned*)(0x2000b20c)) = 49 | (1 << 7);
}

void x_gpio_fiq_async_rising_edge(unsigned pin) {
    volatile unsigned* gparen0 = (volatile unsigned*)(0x2020007c);
    *gparen0 = *gparen0 | (0b1 << (pin & 31));
    /* OR32(GPAFEN0, 1 << (pin & 31)); */
    // from page 116. Bit 7 is enbale, lower 7 bits are source
    /* PUT32(IRQ_FIQ_control, (1 << 7) | GPIO_INT0); */
    *((volatile unsigned*)(0x2000b20c)) = 49 | (1 << 7); 
}

void x_gpio_fiq_async_falling_edge(unsigned pin) {
    volatile unsigned* gpafen0 = (volatile unsigned*)(0x20200088);
    *gpafen0 = *gpafen0 | (0b1 << (pin & 31));
    *((volatile unsigned*)(0x2000b20c)) = 49 | (1 << 7);
}

__attribute__((aligned(32))) void int_vector(uint32_t pc) {
        /* n_interrupt++; */
    cp15_scratch1_set_raw(0);
    raw_gpio_event_clear(in_pin);
}


/*
 00008060 <int_vector>:
    8060:	e3a03000 	mov	r3, #0, 0
    8064:	ee0d3f50 	mcr	15, 0, r3, cr13, cr0, {2}
    8068:	ee1d3f70 	mrc	15, 0, r3, cr13, cr0, {3}
    806c:	e3a02302 	mov	r2, #134217728	; 0x8000000
    8070:	e5832000 	str	r2, [r3]
    8074:	e12fff1e 	bx	lr
 */ 

// driver that triggers and measures the interrupts
// caused by writing to GPIO <pin>.
void test_cost(unsigned pin) { 
    // initial state.
    assert(raw_gpio_read(in_pin) == 0);
    asm volatile("cpsie f");

    float sum = 0;
    uint32_t c,e;
    for(int i = 0; i < 10; i++) {
        // measure the cost of a rising edge interrupt.
        // by reading cycle counter and spinning until the
        // rising edge count increases (i.e., an interrupt
        // occured).
        cp15_scratch1_set_raw(1);
        asm volatile(".align 5");
        c = cycle_cnt_read();

        raw_gpio_set_on(pin);
        while (cp15_scratch1_get())
            ;

        e = cycle_cnt_read();
        output("%d: rising\t= %d cycles\n", i*2, e-c);
        sum += e-c;

        // measure the cost of a falling edge interrupt.
        // by reading cycle counter and spinning until the
        // falling edge count increases (i.e., an interrupt
        // occured).
        cp15_scratch1_set_raw(1);
        asm volatile(".align 5");
        c = cycle_cnt_read();

        let f = cp15_scratch1_get();
        raw_gpio_set_off(pin);
        while (cp15_scratch1_get())
            ;

        e = cycle_cnt_read();
        output("%d: falling\t= %d cycles\n", i*2+1, e-c);
        sum += e-c;
    }
    output("ave cost = %f\n", sum / 20);
}

void notmain() {
    cp15_scratch2_set_raw(gpio_eds0);
    //*****************************************************
    // 1. setup pins and check that loopback works.
    gpio_set_output(out_pin);
    gpio_set_input(in_pin);

    // make sure there is a jumper b/n <in_pin> and <out_pin>
    gpio_write(out_pin, 1);
    if(gpio_read(in_pin) != 1)
        panic("connect jumper from pin %d to pin %d\n", 
                                    in_pin, out_pin);
    gpio_write(out_pin, 0);
    if(gpio_read(in_pin) != 0)
        panic("connect jumper from pin %d to pin %d\n", 
                                    in_pin, out_pin);

    //*****************************************************
    // 2. setup interrupts in our standard way.
    /* extern uint32_t default_vec_ints[]; */

    // setup interrupts.  you've seen this code
    // before.  (we're assuming ints are off.)
    /* dev_barrier(); */
    /* PUT32(IRQ_Disable_1, 0xffffffff); */
    /* PUT32(IRQ_Disable_2, 0xffffffff); */
    /* vector_base_set(default_vec_ints); */
    /**/
    /* // setup interrupts on both rising and falling edges. */
    /* gpio_int_rising_edge(in_pin); */
    /* gpio_int_falling_edge(in_pin); */

    extern uint32_t fiq_ints[];
    vector_base_set(fiq_ints);
    output("assigned fiq_ints\n");

    extern void fiq_init(unsigned gpio_eds0, unsigned in_pin);

    fiq_init(gpio_eds0, 0b1 << in_pin);

    x_gpio_fiq_rising_edge(in_pin);
    x_gpio_fiq_falling_edge(in_pin);

    // the above sample twice triggering (for a stable
    // signal) --- in theory these should be faster.
    // gpio_int_async_rising_edge(in_pin);
    // gpio_int_async_falling_edge(in_pin);

    // clear any existent GPIO event so that we don't 
    // get a delayed interrupt.
    gpio_event_clear(in_pin);

    // now we are live!
    enable_interrupts();

    //*****************************************************
    // 3. run the test.

    // leave this off initially so its easier to see the effect
    // of speed improvements.
    // caches_enable();

    output("caches off\n");
    test_cost(out_pin);
    output("caches on\n");
    caches_enable();
    test_cost(out_pin);
    test_cost(out_pin);
    output("bp on\n");
    uint32_t r;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(r));
    r &= ~(1 << 11);
    asm volatile("mcr p15, 0, %0, c1, c0, 0" : : "r"(r));
    /* PREFETCH_FLUSH(r1); */
    asm volatile("mcr p15, 0, r1, c7, c5, 4"::);
    test_cost(out_pin);
    return;
}
