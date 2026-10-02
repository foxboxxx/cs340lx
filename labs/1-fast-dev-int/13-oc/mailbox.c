#include "mailbox.h"

void mailbox_read( mailbox_channel_t channel, int value ) {
    return;
}

void mailbox_send( mailbox_channel_t channel, uint32_t msg_addr ) {
    dev_barrier();
    while(GET32(MAILBOX_BASE_STATUS) & MAILBOX_FULL) {
        // printk("full\n");
    }
    PUT32(MAILBOX_BASE_WRITE, msg_addr | channel);
    while (1) {
        while(GET32(MAILBOX_BASE_STATUS) & MAILBOX_EMPTY) {
            // printk("empty\n");
        }
        uint32_t response = GET32(MAILBOX_BASE_READ);
        if ((response & 0xF) == (uint32_t)channel) return;
    }
    dev_barrier();
}

uint64_t rpi_mailbox_send(uint32_t tag, uint32_t reply_bytes, uint32_t request_len, uint32_t input) {
    volatile uint32_t __attribute__((aligned(16))) msg[8];

    msg[0] = 8*4;        
    msg[1] = 0;           
    msg[2] = tag;               // tag
    msg[3] = reply_bytes;         
    msg[4] = request_len;           // request code [0].
    msg[5] = input;          
    msg[6] = 0;          
    msg[7] = 0;        

    mailbox_send(MAILBOXS_ARM_TO_VC, (uint32_t)msg);

    if(msg[1] != 0x80000000) panic("invalid response: got %x\n", msg[1]);
    assert(msg[4] == ((1<<31) | reply_bytes));

    if (reply_bytes == 8) {
        return ((uint64_t)msg[6] << 32) | msg[5];
    } 
    else {
        return msg[5];
    }

    return 0;

}

uint64_t rpi_get_serialnum(void) {
    return rpi_mailbox_send(SERIAL, 8, 0, 0);
}

uint32_t rpi_get_model(void) {
    return rpi_mailbox_send(BOARD_MODEL, 4, 0, 0);
}

uint32_t rpi_get_revision(void) {
    return rpi_mailbox_send(BOARD_REVISION, 4, 0, 0);
}

uint32_t rpi_get_memsize(void) {
    return rpi_mailbox_send(ARM_MEMORY, 8, 0, 0) >> 32; // top 32 bits is size, bottom 32 bits is base addr
}

uint32_t rpi_temp_get(void) {
    return rpi_mailbox_send(TEMPERATURE, 8, 0, 0) >> 32; // top 32 bits is value, bottom 32 is id
}

uint32_t rpi_clock_curhz_get(mailbox_clock_id_t clk_id) {
    return rpi_mailbox_send(CLOCK, 8, 4, clk_id) >> 32; // top 32 bits is rate in Hz, bottom 32 is id
}

uint32_t rpi_clock_maxhz_get(mailbox_clock_id_t clk_id) {
    return rpi_mailbox_send(MAX_CLOCK, 8, 4, clk_id) >> 32; // top 32 bits is rate in Hz, bottom 32 is id
}

uint32_t rpi_clock_realhz_get(mailbox_clock_id_t clk_id) {
    return rpi_mailbox_send(MEASURED_CLOCK, 8, 4, clk_id) >> 32; // top 32 bits is rate in Hz, bottom 32 is id   
}

uint32_t rpi_clock_hz_set(mailbox_clock_id_t clk_id, uint32_t rate, uint32_t turbo) {
    volatile uint32_t __attribute__((aligned(16))) msg[9];
    msg[0] = 9 * 4;             
    msg[1] = 0;                
    msg[2] = SET_CLOCK;        
    msg[3] = 12;               
    msg[4] = 12;                
    msg[5] = clk_id;            
    msg[6] = rate;               
    msg[7] = turbo;                
    msg[8] = 0;                 
    mailbox_send(MAILBOXS_ARM_TO_VC, (uint32_t)msg);
    if (msg[1] != 0x80000000) return 0; 
    return msg[6]; // return the rate
}
