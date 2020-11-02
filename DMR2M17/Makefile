CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread -lmd380_vocoder
LDFLAGS ?= -g

OBJECTS = 	BPTC19696.o Conf.o CRC.o DelayBuffer.o DMRData.o DMREMB.o DMREmbeddedData.o \
			DMRFullLC.o DMRLC.o DMRLookup.o DMRSlotType.o  MMDVMNetwork.o  M17Network.o Golay2087.o \
			Golay24128.o Hamming.o Log.o ModeConv.o Mutex.o QR1676.o RS129.o SHA256.o StopWatch.o \
			Sync.o Thread.o Timer.o UDPSocket.o Utils.o MBEVocoder.o codec2/codebooks.o codec2/kiss_fft.o \
			codec2/lpc.o codec2/nlp.o codec2/pack.o codec2/qbase.o codec2/quantise.o codec2/codec2.o DMR2M17.o 

all:		DMR2M17

DMR2M17:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o DMR2M17 -Xlinker --section-start=.firmware=0x0800C000 -Xlinker  --section-start=.sram=0x20000000

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 DMR2M17 /usr/local/bin/

clean:
		$(RM) DMR2M17 *.o *.d *.bak codec2/*.o *~

