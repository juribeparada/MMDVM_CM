CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread -lmd380_vocoder -lmbe -limbe_vocoder
LDFLAGS ?= -g

OBJECTS = 	BPTC19696.o Conf.o CRC.o DelayBuffer.o DMRData.o DMREMB.o DMREmbeddedData.o \
			DMRFullLC.o DMRLC.o DMRLookup.o DMRNetwork.o DMRSlotType.o M17Network.o Golay2087.o \
			Golay24128.o Hamming.o Log.o mbeenc.o ambe.o MBEVocoder.o ModeConv.o Mutex.o QR1676.o RS129.o SHA256.o StopWatch.o \
			Sync.o Thread.o Timer.o UDPSocket.o Utils.o Reflectors.o codec2/codebooks.o codec2/kiss_fft.o \
			codec2/lpc.o codec2/nlp.o codec2/pack.o codec2/qbase.o codec2/quantise.o codec2/codec2.o M172DMR.o 

all:		M172DMR

M172DMR:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o M172DMR  -Xlinker --section-start=.firmware=0x0800C000 -Xlinker  --section-start=.sram=0x20000000

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 M172DMR /usr/local/bin/

clean:
		$(RM) M172DMR *.o *.d *.bak *~

