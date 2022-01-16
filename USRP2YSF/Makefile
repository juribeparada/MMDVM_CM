CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x
LIBS    = -lm -lmd380_vocoder
LDFLAGS ?= -g

OBJECTS = 	Conf.o CRC.o USRPNetwork.o Golay24128.o Log.o MBEVocoder.o ModeConv.o Mutex.o StopWatch.o Timer.o \
			UDPSocket.o Utils.o YSFConvolution.o YSFFICH.o YSFNetwork.o YSFPayload.o USRP2YSF.o 

all:		USRP2YSF

USRP2YSF:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o USRP2YSF  -Xlinker --section-start=.firmware=0x0800C000 -Xlinker  --section-start=.sram=0x20000000

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 USRP2YSF /usr/local/bin/

clean:
		$(RM) USRP2YSF *.o *.d *.bak *~

