CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x -pthread
LIBS    = -lm -lmd380_vocoder
LDFLAGS ?= -g

OBJECTS =   Conf.o CRC.o Golay24128.o Log.o ModeConv.o DSTARNetwork.o SerialController.o \
			StopWatch.o Timer.o UDPSocket.o Utils.o YSFConvolution.o YSFFICH.o YSFNetwork.o \
			YSFPayload.o DSTAR2YSF.o

all:		DSTAR2YSF

DSTAR2YSF:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o DSTAR2YSF -Xlinker --section-start=.firmware=0x0800C000 -Xlinker  --section-start=.sram=0x20000000

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

clean:
		$(RM) DSTAR2YSF *.o *.d *.bak *~
 
