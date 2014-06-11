#TOOL_DIR := /pool/arm/toolchain/root/usr/bin
#CROSS_PREFIX := arm-none-linux-gnueabi-
#CC := $(TOOL_DIR)/$(CROSS_PREFIX)gcc
CC := gcc
CFLAGS := -Wall

all: lps331 lps331-get
lps331: lps331.o
lps331.o: lps331.c
lps331-get: lps331-get.o
lps331-get.o: lps331-get.c
clean:
	rm -f lps331 lps331-get *.o *~
.PHONY: clean

