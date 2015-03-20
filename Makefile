# Scott Anderson
# #1240847

CFLAGS += -std=c99 -Wall -Wextra -g
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
LDFLAGS +=

.PHONY: all clean

all: Tftp Tftpd

Tftp: Tftp.o network.o protocol.o

Tftpd: Tftpd.o network.o protocol.o

clean: 
	$(RM) Tftp Tftpd $(wildcard *.o)
