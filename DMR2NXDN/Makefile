CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread
LDFLAGS ?= -g

OBJECTS = 	BPTC19696.o Conf.o CRC.o DMRData.o DMREMB.o DMREmbeddedData.o \
			DMRFullLC.o DMRLC.o DMRLookup.o DMR2NXDN.o DMRSlotType.o  Golay2087.o \
			Golay24128.o Hamming.o Log.o MMDVMNetwork.o ModeConv.o Mutex.o \
			NXDNConvolution.o NXDNCRC.o NXDNLayer3.o NXDNLICH.o NXDNLookup.o \
			NXDNSACCH.o  NXDNNetwork.o QR1676.o RS129.o SHA256.o StopWatch.o Sync.o \
			Thread.o Timer.o UDPSocket.o Utils.o 

all:		DMR2NXDN

DMR2NXDN:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o DMR2NXDN

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 DMR2NXDN /usr/local/bin/

clean:
		$(RM) DMR2NXDN *.o *.d *.bak *~
 
