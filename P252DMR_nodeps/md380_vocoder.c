#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

#include "md380_vocoder.h"

extern int ambe_inbuffer, ambe_outbuffer0, ambe_outbuffer1, ambe_mystery;
extern int ambe_outbuffer, wav_inbuffer0, wav_inbuffer1, ambe_en_mystery;

int ambe_encode_thing2(int16_t *bitbuffer,
                       int a2,
                       int16_t *wavbuffer,
                       int eighty,
                       int a5,
                       int16_t a6, //timeslot, 0 or 1
                       int16_t a7, //2000
                       uint32_t a8){  // 2000c730
    ambe_encode_thing(bitbuffer,a2,wavbuffer, eighty, a5,a6,a7,a8);
}

void md380_encode(uint8_t *ambe49, int16_t *pcm)
{
	int16_t *inbuf0 = (int16_t*) &wav_inbuffer0;
	int16_t *inbuf1 = (int16_t*) &wav_inbuffer1;
	int16_t *ambe   = (int16_t*) &ambe_outbuffer;
	
	memset(ambe,0,50);

	for (int i=0;i<80;i++) inbuf0[i] = pcm[i];
	for (int i=0;i<80;i++) inbuf1[i] = pcm[i+80];

	ambe_encode_thing2(ambe, 0, inbuf0, 0x50, 0x1840, 0, 0x2000, (int) &ambe_en_mystery);
	ambe_encode_thing2(ambe, 0, inbuf1,0x50, 0x1840, 0x1, 0x2000, (int) &ambe_en_mystery);

	for(int i = 0; i < 6; ++i){
		for(int j = 0; j < 8; ++j){
			ambe49[i] |= (ambe[(i * 8) + (7 - j)] << j);
		}
	}
    ambe49[6] = ambe[48] ? 0x80 : 0;
}

void md380_decode(uint8_t *ambe49, int16_t *pcm)
{
	int16_t *ambe=(int16_t*) &ambe_inbuffer;
	int16_t *outbuf0=(int16_t*) &ambe_outbuffer0; 
	int16_t *outbuf1=(int16_t*) &ambe_outbuffer1;

    int ambei=0;
    for(int i=0;i<6;i++){
      for(int j=0;j<8;j++){
        ambe[ambei++]=(ambe49[i]>>(7-j))&1;
      }
    }
    ambe[ambei++]=ambe49[6] ? 1 : 0;

    ambe_decode_wav(outbuf0, 80, ambe, 0, 0, 0, (int) &ambe_mystery);
    ambe_decode_wav(outbuf1, 80, ambe, 0, 0, 1, (int) &ambe_mystery);
    memmove(pcm, outbuf0, 160);
    memmove(&pcm[80], outbuf1, 160);
}
