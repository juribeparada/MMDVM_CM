
#ifdef __cplusplus

extern "C" {
#endif
#include <inttypes.h>

int ambe_decode_wav(int16_t *wavbuffer, //1aa8 or 1b48
		    int eighty,       //always 80
		    int16_t *bitbuffer,        //always 1c8e
		    int a4,     //0
		    int16_t a5,   //0
		    int16_t a6,   //timeslot, 0 or 1
		    int a7      //0x20011224
		    );
		    
//void ambe_init_stuff();
void md380_decode(uint8_t *, int16_t *);
void md380_encode(uint8_t *, int16_t *);

int ambe_encode_thing(int16_t *bitbuffer,
                      int a2,
                      int16_t *wavbuffer,
                      int eighty,
                      int,
                      int16_t a6, //timeslot, 0 or 1
                      int16_t a7, //2000
                      int);  // 2000c730
                      
#ifdef __cplusplus
}
#endif
