#include "rpi.h"

// from https://www.valvers.com/open-software/raspberry-pi/bare-metal-programming-in-c-part-5/#mailboxes

#define MAILBOX_BASE_READ   0x2000B880
#define MAILBOX_BASE_WRITE  0x2000B8A0
#define MAILBOX_BASE_STATUS 0x2000B898

// tags
#define SERIAL              0x00010004
#define BOARD_MODEL         0x00010001
#define BOARD_REVISION      0x00010002
#define ARM_MEMORY          0x00010005
#define TEMPERATURE         0x00030006
#define CLOCK               0x00030002
#define MAX_CLOCK           0x00030004
#define MEASURED_CLOCK      0x00030047
#define SET_CLOCK           0x00038002

// settings
#define SKIP_TURBO          1

typedef enum {
    MAILBOX_POWER_MANAGEMENT = 0,
    MAILBOX_FRAMEBUFFER, // 1
    MAILBOX_VIRTUAL_UART, // 2
    MAILBOX_VCHIQ, // 3
    MAILBOX_LEDS, // 4
    MAILBOX_BUTTONS, // 5
    MAILBOX_TOUCHSCREEN, // 6
    MAILBOX_UNUSED, // 7
    MAILBOXS_ARM_TO_VC, // 8 IMPORTANT
    MAILBOX_VC_TO_ARM, // 9 IMPORTANT
} mailbox_channel_t;

enum mailbox_status_reg_bits {
    MAILBOX_FULL  = 0x80000000, // 1 << 31
    MAILBOX_EMPTY = 0x40000000, // 1 << 30
};

typedef enum {
    reserved = 0x0,
    EMMC,
    UART,
    ARM,
    CORE,
    V3D,
    H264,
    ISP,
    SDRAM,
    PIXEL,
    PWM,
    HEVC,
    EMMC2,
    M2MC,
    PIXEL_BVB,
} mailbox_clock_id_t;

// extern void mailbox_send( mailbox_channel_t channel, uint32_t msg_addr );
uint64_t rpi_get_serialnum(void);
uint32_t rpi_get_model(void);
uint32_t rpi_get_revision(void);
uint32_t rpi_get_memsize(void);
uint32_t rpi_temp_get(void);
uint32_t rpi_clock_curhz_get(mailbox_clock_id_t clk_id);
uint32_t rpi_clock_maxhz_get(mailbox_clock_id_t clk_id);
uint32_t rpi_clock_realhz_get(mailbox_clock_id_t clk_id);
uint32_t rpi_clock_hz_set(mailbox_clock_id_t clk_id, uint32_t rate, uint32_t turbo);
