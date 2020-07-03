CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread
LDFLAGS ?= -g

OBJECTS =   APRSReader.o APRSWriter.o APRSWriterThread.o Conf.o CRC.o DTMF.o Golay24128.o \
			GPS.o Log.o ModeConv.o Mutex.o NXDNConvolution.o NXDNCRC.o NXDNLayer3.o NXDNLICH.o \
			NXDNLookup.o NXDNNetwork.o NXDNSACCH.o SHA256.o StopWatch.o Sync.o TCPSocket.o \
			Thread.o Timer.o UDPSocket.o Utils.o WiresX.o YSF2NXDN.o YSFConvolution.o YSFFICH.o \
			YSFNetwork.o YSFPayload.o

all:		YSF2NXDN

YSF2NXDN:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o YSF2NXDN

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 YSF2NXDN /usr/local/bin/

clean:
		$(RM) YSF2NXDN *.o *.d *.bak *~
 
