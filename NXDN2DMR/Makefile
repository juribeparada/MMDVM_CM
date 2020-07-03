CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread
LDFLAGS ?= -g

OBJECTS = 	BPTC19696.o Conf.o CRC.o DelayBuffer.cpp DMRData.o DMREMB.o DMREmbeddedData.o \
			DMRFullLC.o DMRLC.o DMRLookup.o DMRNetwork.o DMRSlotType.o  Golay2087.o \
			Golay24128.o Hamming.o Log.o ModeConv.o Mutex.o NXDNConvolution.o NXDNCRC.o \
			NXDNLayer3.o NXDNLICH.o NXDNLookup.o NXDNSACCH.o NXDN2DMR.o NXDNNetwork.o \
			QR1676.o Reflectors.o RS129.o SHA256.o StopWatch.o Sync.o Thread.o Timer.o \
			UDPSocket.o Utils.o 

all:		NXDN2DMR

NXDN2DMR:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o NXDN2DMR

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 NXDN2DMR /usr/local/bin/

clean:
		$(RM) NXDN2DMR *.o *.d *.bak *~
 
