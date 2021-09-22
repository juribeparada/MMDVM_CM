CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lpthread -limbe_vocoder
LDFLAGS ?= -g

OBJECTS = 	Conf.o Log.o MBEVocoder.o ModeConv.o P25Network.o StopWatch.o Timer.o UDPSocket.o USRPNetwork.o Utils.o USRP2P25.o 

all:		USRP2P25

USRP2P25:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o USRP2P25

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 USRP2P25 /usr/local/bin/

clean:
		$(RM) USRP2P25 *.o *.d *.bak *~

