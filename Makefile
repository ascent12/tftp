CFLAGS += -std=gnu99 -Wall -Wextra
CPPFLAGS +=
LDFLAGS +=

all: tftp tftpd

tftp: tftp.o network.o

tftpd: tftpd.o network.o

clean: 
	$(RM) tftp tftpd $(wildcard *.o)
