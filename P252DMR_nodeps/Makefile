CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread -L. -limbe
LDFLAGS ?= -g

OBJECTS = 	BPTC19696.o Conf.o CRC.o DelayBuffer.cpp DMRData.o DMREMB.o DMREmbeddedData.o \
			DMRFullLC.o DMRLC.o DMRLookup.o DMRNetwork.o DMRSlotType.o  P25Network.o Golay2087.o \
			Golay24128.o Hamming.o Log.o ModeConv.o Mutex.o QR1676.o Reflectors.o RS129.o \
			SHA256.o StopWatch.o Sync.o Thread.o Timer.o UDPSocket.o Utils.o mbevocoder.o md380_vocoder.o firmware.o ram.o P252DMR.o

all:		P252DMR

P252DMR:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o P252DMR \
		-Xlinker --just-symbols=symbols_d02.032 \
		-Xlinker --section-start=.firmware=0x0800C000 \
		-Xlinker  --section-start=.sram=0x20000000 

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) P252DMR *.o *.d *.bak *~

firmware.o:
	objcopy \
		-I binary D002.032.img \
		--change-addresses=0x0800C000 \
		--rename-section .data=.firmware \
		-O elf32-littlearm -B arm firmware.o
ram.o:
	objcopy \
		-I binary d02032-core.img \
		--change-addresses=0x20000000 \
		--rename-section .data=.sram \
		-O elf32-littlearm -B arm ram.o
 
