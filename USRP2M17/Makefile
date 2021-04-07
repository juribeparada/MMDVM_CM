CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread
LDFLAGS ?= -g

OBJECTS = 	Conf.o Log.o M17Network.o ModeConv.o StopWatch.o Timer.o UDPSocket.o USRPNetwork.o Utils.o \
			codec2/codebooks.o codec2/kiss_fft.o codec2/lpc.o codec2/nlp.o codec2/pack.o codec2/qbase.o codec2/quantise.o codec2/codec2.o USRP2M17.o 

all:		USRP2M17

USRP2M17:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o USRP2M17

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 USRP2M17 /usr/local/bin/

clean:
		$(RM) USRP2M17 *.o *.d *.bak codec2/*.o *~

