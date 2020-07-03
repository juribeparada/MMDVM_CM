CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread
LDFLAGS ?= -g

OBJECTS = 	BPTC19696.o Conf.o GPS.o TCPSocket.o DTMF.o APRSWriter.o APRSWriterThread.o CRC.o \
			DelayBuffer.cpp DMRLookup.o DMREMB.o DMREmbeddedData.o APRSReader.o \
			DMRFullLC.o DMRNetwork.o DMRLC.o DMRSlotType.o DMRData.o Golay2087.o Golay24128.o \
			Hamming.o Log.o ModeConv.o Mutex.o QR1676.o Reflectors.o RS129.o StopWatch.o Sync.o \
			SHA256.o Thread.o Timer.o UDPSocket.o Utils.o WiresX.o YSFConvolution.o YSFFICH.o \
			YSFNetwork.o YSF2DMR.o YSFPayload.o

all:		YSF2DMR

YSF2DMR:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o YSF2DMR

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 YSF2DMR /usr/local/bin/

clean:
		$(RM) YSF2DMR *.o *.d *.bak *~
 
