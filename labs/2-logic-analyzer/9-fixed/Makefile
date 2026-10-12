PROGS = timed-read.c
COMMON_SRC = interrupt-asm.S mailbox.c 

#CFLAGS += -fno-toplevel-reorder
#CFLAGS += -I$(CS340LX_2026_PATH)/lib/libvm-ident0.0/includes
# CFLAGS += -I/Users/joe/Documents/github/cs340lx-26aut/lib/libvm-ident0.0/vm-ident.h
CFLAGS += -fno-toplevel-reorder
  CFLAGS += -I$(CS340LX_2026_PATH)/lib/libvm-ident0.0/includes
  CFLAGS += -I$(CS340LX_2026_PATH)/lib/libvm-ident0.0
  LIBS   += $(CS340LX_2026_PATH)/lib/libvm-ident0.0/libvm-ident.a

# if you need to override the install
BOOTLOADER = my-install

# OPT_LEVEL = our default optimization level.
OPT_LEVEL = -O1
OPT_LEVEL = -O3
OPT_LEVEL = -Ofast
#OPT_LEVEL = -Og

# automatically run the code
RUN = 1

include $(CS340LX_2026_PATH)/libpi/mk/Makefile.robust-v3
