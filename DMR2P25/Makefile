CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread -limbe_vocoder -lmd380_vocoder
LDFLAGS ?= -g

OBJECTS = 	BPTC19696.o Conf.o CRC.o DelayBuffer.o DMRData.o DMREMB.o DMREmbeddedData.o \
			DMRFullLC.o DMRLC.o DMRLookup.o DMRSlotType.o  MMDVMNetwork.o  P25Network.o Golay2087.o \
			Golay24128.o Hamming.o Log.o ModeConv.o Mutex.o QR1676.o Reflectors.o RS129.o \
			SHA256.o StopWatch.o Sync.o Thread.o Timer.o UDPSocket.o Utils.o MBEVocoder.o DMR2P25.o

all:		DMR2P25

DMR2P25:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o DMR2P25  -Xlinker --section-start=.firmware=0x0800C000 -Xlinker  --section-start=.sram=0x20000000

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 DMR2P25 /usr/local/bin/

clean:
		$(RM) DMR2P25 *.o *.d *.bak *~

