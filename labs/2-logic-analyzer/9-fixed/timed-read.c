// [NOTE: this code should remind you alot about 1-fast-dev-int.
// its good practice to speed run again.]
//
// Simple code to measure the time it takes to record timed
// reads.
//
// you can use these to make a digital scope, or make bit banged
// digital protocols.  dual of observing for measurement and 
// observing for information.
//
// The file "interrupt-asm.S" has the support assembly code.
//
// You should put a loopback jumper between <in_pin> and 
// <out_pin> (definitions below).
//
// Lab: make this code much faster / accurate.
#define __NDEBUG__ 1
#include "rpi.h"
#include "timer-interrupt.h"
#include "cycle-count.h"
#include "vector-base.h"
#include "gpio-raw.h"
#include "cache-support.h"
#include "mailbox.h"
#include "vm-enable.h"
#ifdef __NDEBUG__ 
#undef assert
#define assert(bool) do { } while(0)
#endif
#include "asm-helpers.h"
cp_asm_raw(cp15_scratch3, p15, 0, c13, c0, 4)
cp_asm_raw(cp15_scratch2, p15, 0, c13, c0, 3)
cp_asm_raw(cp15_scratch1, p15, 0, c13, c0, 2)

// raw timed reads: gives the cycle count (absolute) and the raw
// level.  post process this into a timed log(?)
typedef struct { 
    uint32_t lev;   // raw lev
    uint32_t cyc;   // raw cycles
} timed_read_t;

// not sure if you should check the end.
// probably should just do a watchpoint.
enum { MAX_READS = 128 };
static timed_read_t t_reads[MAX_READS], *t_end = &t_reads[MAX_READS];
static timed_read_t * volatile tr = &t_reads[0];

static inline timed_read_t * tr_get(void) { return (void*)cp15_scratch1_get(); }
static inline void tr_set(timed_read_t *t) { cp15_scratch1_set_raw((uint32_t)t); }

static inline volatile uint32_t *event0_get(void) { return (void*)cp15_scratch2_get(); }
static inline void event0_set(uint32_t event0) { cp15_scratch2_set_raw(event0); }

static inline volatile uint32_t *level0_get(void) { return (void*)cp15_scratch3_get(); }
static inline void level0_set(uint32_t level0) { cp15_scratch3_set_raw(level0); }

// Can change these pins to whatever you want.  
// 
// NOTE: some pin numbers let the compiler generate faster 
// code b/c it can load derived constants in fewer instructions
// [useful side quest: write some code to check this claim!]
enum { out_pin = 26, in_pin = 27 };

void fiq_init(timed_read_t* tr, uint32_t pin);

static void int_init(void) {
    /* tr_set(tr = &t_reads[0]); */
    /* level0_set(gpio_lev0); */
    /* event0_set(gpio_eds0); */
    fiq_init(tr = &t_reads[0], 1 << in_pin);

}

static inline timed_read_t * volatile read_cur(void) {
    return tr_get();
    /* return tr; */
}

static inline void dmb_raw(void) {
    uint32_t r0 = 0;
    asm volatile("mcr p15, 0, %0, c7, c10, 5" :: "r"(r0));
}

/* static void int_init(void) { */
/*     tr = &t_reads[0]; */
/* } */

// a bit fancier than the device interrupt since we 
// are recording a log.
/* __attribute__((aligned(32)))  */
__attribute__((interrupt("FIQ"), aligned(32)))
void fiq_handler(void) {
    uint32_t cycle = cycle_cnt_read();

    // we don't know what the user code was doing
    dmb_raw();
    uint32_t lev = *level0_get();

    timed_read_t* tr = tr_get();
    *tr =  (timed_read_t) { .cyc = cycle, .lev = lev };
    tr_set(tr+1);

    assert(tr < t_end);

    // gpio and event clear may need device barrier?
    dmb_raw();

    *event0_get() = 1 << in_pin;

    // we don't know what the user code was doing
    dmb_raw();
}

// driver that triggers and measures the interrupts
// caused by writing to GPIO <pin>.
void test_cost(unsigned pin) { 
    int_init();
    // initial state.
    assert(raw_gpio_read(in_pin) == 0);

    float sum = 0;
    uint32_t c,e;
    enum { N = 20 };

    for(int i = 0; i < N/2; i++) {
        // measure the cost of a rising edge interrupt.
        // by reading cycle counter and spinning until the
        // rising edge count increases (i.e., an interrupt
        // occured).
        timed_read_t* r = read_cur();
        asm volatile(".align 5");
        c = cycle_cnt_read();
        raw_gpio_set_on(pin);
        while(r == read_cur())
            ;
        e = cycle_cnt_read();
        // compute what the level was
        long unsigned int lvl = (r->lev >> in_pin) &1;

        output("%d: rising  [lev=%d]\t= %d total cycles [%d until int ran]\n", 
            i*2, 
            lvl,
            e-c,
            r->cyc -c);
        sum += e-c;
        /* if(lvl != 1) */
        /*     panic("LOG error: level not 1 on a rising interrupt\n"); */
        assert(lvl == 1);

        // measure the cost of a falling edge interrupt.
        // by reading cycle counter and spinning until the
        // falling edge count increases (i.e., an interrupt
        // occured).
        r = read_cur();
        asm volatile(".align 5");
        c = cycle_cnt_read();
        raw_gpio_set_off(pin);
        while(r == read_cur())
            ;
        e = cycle_cnt_read();

        // compute what the level was
        lvl = (r->lev >> in_pin) &1;

        output("%d: falling [lev=%d]\t= %d total cycles [%d until int ran]\n", 
            i*2+1, lvl, e-c, r->cyc - c);
        sum += e-c;
        /* if(lvl != 0) */
        /*     panic("LOG error: level not 0 on a falling interrupt\n"); */
        assert(lvl == 0);
    }
    output("ave cost = %f\n", sum / 20);
}

// all this is the same as 1-fast-dev-int.
void notmain() {
    //*****************************************************
    // 1. setup pins and check that loopback works.
    raw_gpio_set_output(out_pin);
    raw_gpio_set_input(in_pin);

    // make sure there is a jumper b/n <in_pin> and <out_pin>
    raw_gpio_write(out_pin, 1);
    delay_us(10);
    /* if(raw_gpio_read(in_pin) != 1) */
    /*     panic("connect jumper from pin %d to pin %d\n",  */
    /*                                 in_pin, out_pin); */
    assert(raw_gpio_read(in_pin) == 1);
    raw_gpio_write(out_pin, 0);
    delay_us(10);
    /* if(raw_gpio_read(in_pin) != 0) */
    /*     panic("connect jumper from pin %d to pin %d\n",  */
    /*                                 in_pin, out_pin); */
    assert(raw_gpio_read(in_pin) == 0);

    //*****************************************************
    // 2. setup interrupts in our standard way.
    extern uint32_t default_vec_ints[];

    // setup interrupts.  you've seen this code
    // before.  (we're assuming ints are off.)
    extern void interrupt_setup_stack(void);
    /* extern void fiq_setup_stack(void); */
    /* fiq_setup_stack(); */
    /* interrupt_setup_stack(); */
    dmb_raw();
    PUT32(IRQ_Disable_1, 0xffffffff);
    PUT32(IRQ_Disable_2, 0xffffffff);
    dmb_raw();
    vector_base_set(default_vec_ints);

    // setup interrupts on both rising and falling edges.
    /* gpio_int_rising_edge(in_pin); */
    /* gpio_int_falling_edge(in_pin); */
    /* gpio_fiq_rising_edge(in_pin); */
    /* gpio_fiq_falling_edge(in_pin); */
    gpio_fiq_async_rising_edge(in_pin);
    gpio_fiq_async_falling_edge(in_pin);

    // the above sample twice triggering (for a stable
    // signal) --- in theory these should be faster.
    // gpio_int_async_rising_edge(in_pin);
    // gpio_int_async_falling_edge(in_pin);

    // clear any existent GPIO event so that we don't 
    // get a delayed interrupt.
    raw_gpio_event_clear(in_pin);

    output("About to go live.\n");
    enable_interrupts();
    asm volatile("cpsie f");
    // now we are live!

    //*****************************************************
    // 3. run the test.

    // leave this off initially so its easier to see the effect
    // of speed improvements.
    // caches_enable();

    output("no cahce!\n");
    test_cost(out_pin);

    output("caches on + prefetch\n");
    caches_enable();
    /* btc_off(); */
    extern uint32_t prefetch_end[];
    prefetch_inst((void*)test_cost, prefetch_end);
    test_cost(out_pin);
    test_cost(out_pin);

    output("vm on + caches + prefetch + btc off");
    vm_on_cache();
    btc_off();
    prefetch_inst((void*)test_cost, prefetch_end);
    test_cost(out_pin);
    test_cost(out_pin);

    output("Done!\n");
    return;
}
