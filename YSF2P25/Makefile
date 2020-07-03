CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread
LDFLAGS ?= -g

OBJECTS = 	Conf.o CRC.o DMRLookup.o DTMF.o Golay24128.o Hamming.o Log.o ModeConv.o Mutex.o \
			P25Network.o StopWatch.o Sync.o Thread.o Timer.o UDPSocket.o Utils.o WiresX.o \
			YSF2P25.o YSFConvolution.o YSFFICH.o YSFNetwork.o YSFPayload.o

all:		YSF2P25

YSF2P25:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o YSF2P25

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 YSF2P25 /usr/local/bin/

clean:
		$(RM) YSF2P25 *.o *.d *.bak *~
 
