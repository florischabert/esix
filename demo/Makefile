# Copyright (c) 2009, Floris Chabert, Simon Vetter. All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 
#    * Redistributions of source code must retain the above copyright notice,
#      this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright 
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AS IS'' AND ANY EXPRESS 
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO 
# EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, 
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO,PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR  
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS  SOFTWARE, 
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

SRC=$(wildcard *.c) \
	FreeRTOS/tasks.c FreeRTOS/list.c FreeRTOS/queue.c \
	FreeRTOS/portable/GCC/ARM_CM3/port.c FreeRTOS/portable/MemMang/heap_1.c
OBJ=$(SRC:.c=.o)
PROGRAM=main

CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
AR=arm-none-eabi-ar
CFLAGS = -mcpu=cortex-m3 -mthumb -g -O2 -Wall -ffreestanding -fasm \
	-fdata-sections -ffunction-sections \
	-I. -I../esix/include -IFreeRTOS/include -IFreeRTOS/portable/GCC/ARM_CM3
LDFLAGS = -mcpu=cortex-m3 --gc-sections -mthumb -nostartfiles -L../src \
	-Xlinker -T  -Xlinker link.ld

all: $(PROGRAM)

libesix:
	@echo "### -> Compiling libesix ..."
	make -C ../esix CC=$(CC) AR=$(AR) CFLAGS="-mcpu=cortex-m3 -mthumb -g -O2 -Wall"
	@echo ""

$(PROGRAM):  libesix $(OBJ)
	@echo "\n### -> Linking ..."
	$(CC) -o $(PROGRAM) $(LDFLAGS) $(OBJ) -L../esix/lib -lesix

bin: $(PROGRAM)
	arm-none-eabi-objcopy -Obinary $(PROGRAM) $(PROGRAM).bin

.PHONY: clean

clean:
	rm -f $(PROGRAM) $(PROGRAM).bin $(OBJ)
	@echo "### -> Clearing libesix..."
	make -C ../esix clean
