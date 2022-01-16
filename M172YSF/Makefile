CC      ?= gcc
CXX     ?= g++
CFLAGS  ?= -g -O3 -Wall -std=c++0x
LIBS    = -lm -lmd380_vocoder -lmbe -limbe_vocoder
LDFLAGS ?= -g

OBJECTS = 	Conf.o CRC.o M17Network.o Golay24128.o Log.o MBEVocoder.o ModeConv.o Mutex.o StopWatch.o Timer.o UDPSocket.o Utils.o YSFConvolution.o YSFFICH.o YSFNetwork.o YSFPayload.o \
			codec2/codebooks.o codec2/kiss_fft.o codec2/lpc.o codec2/nlp.o codec2/pack.o codec2/qbase.o codec2/quantise.o codec2/codec2.o M172YSF.o 

all:		M172YSF

M172YSF:	$(OBJECTS)
		$(CXX) $(OBJECTS) $(CFLAGS) $(LIBS) -o M172YSF  -Xlinker --section-start=.firmware=0x0800C000 -Xlinker  --section-start=.sram=0x20000000

%.o: %.cpp
		$(CXX) $(CFLAGS) -c -o $@ $<

install:
		install -m 755 M172YSF /usr/local/bin/

clean:
		$(RM) M172YSF *.o *.d *.bak *~

