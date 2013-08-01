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

LIB=libesix.a
SRC=$(wildcard src/*.c) $(wildcard src/glue/*.c)
TESTS=$(wildcard tests/*.c)
PCAP=$(wildcard pcap/*.c)
CFLAGS += -Iinclude -Wall -g -DESIX_DEBUG

%.o: %.c
	@echo CC $@
	@$(CC) $(CFLAGS) -MD -MF $(@:.o=.deps) -o $@ -c $<

$(LIB): $(SRC:.c=.o)
	@echo AR $@
	@$(AR) rcs $@ $^

test: $(TESTS:.c=.o) $(LIB)
	@echo LD $@
	@$(CC) -o $@ $^

esixd: $(PCAP:.c=.o) $(LIB)
	@echo LD $@
	@$(CC) -o $@ $^ -lpcap

freertos: $(LIB)
	@$(MAKE) -C freertos

-include $(SRC:%.c=%.deps) $(TESTS:%.c=%.deps) $(PCAP:%.c=%.deps)

clean:
	@$(RM) $(LIB) $(SRC:.c=.o) $(SRC:.c=.deps) test $(TESTS:.c=.o) $(TESTS:.c=.deps) esixd $(PCAP:.c=.o) $(PCAP:.c=.deps)

.PHONY: clean freertos
