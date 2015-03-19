CFLAGS += -std=c99 -Wall -Wextra -g
CPPFLAGS += -D_POSIX_C_SOURCE=200809L
LDFLAGS +=

.PHONY: all clean

all: tftp tftpd

tftp: tftp.o network.o protocol.o

tftpd: tftpd.o network.o protocol.o

clean: 
	$(RM) tftp tftpd $(wildcard *.o)
