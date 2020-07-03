CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread
LDFLAGS ?= -g

OBJECTS = 	BPTC19696.o Conf.o CRC.o DelayBuffer.cpp DMRLookup.o DMREMB.o DMREmbeddedData.o \
			DMR2YSF.o DMRFullLC.o MMDVMNetwork.o DMRLC.o DMRSlotType.o DMRData.o Golay2087.o \
			Golay24128.o Hamming.o Log.o ModeConv.o Mutex.o QR1676.o RS129.o StopWatch.o Sync.o \
			SHA256.o Thread.o Timer.o UDPSocket.o Utils.o YSFConvolution.o YSFFICH.o \
			YSFNetwork.o YSFPayload.o

all:		DMR2YSF

DMR2YSF:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o DMR2YSF

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 DMR2YSF /usr/local/bin/

clean:
		$(RM) DMR2YSF *.o *.d *.bak *~
 
