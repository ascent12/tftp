CFLAGS += -std=gnu99 -Wall -Wextra
CPPFLAGS +=
LDFLAGS +=

all: tftp tftpd

tftp: tftp.o network.o protocol.o

tftpd: tftpd.o network.o protocol.o

clean: 
	$(RM) tftp tftpd $(wildcard *.o)
