/*
 *  * Project 25 IMBE Encoder/Decoder Fixed-Point implementation
 *   * Developed by Pavel Yazev E-mail: pyazev@gmail.com
 *    * Version 1.0 (c) Copyright 2009
 *     */
/* -*- c++ -*- */
#ifndef INCLUDED_IMBE_VOCODER_H
#define INCLUDED_IMBE_VOCODER_H

#include <cstdint>
#include "typedefs.h"

#define FRAME             160   // Number samples in frame
#define NUM_HARMS_MAX      56   // Maximum number of harmonics
#define NUM_HARMS_MIN       9   // Minimum number of harmonics
#define NUM_BANDS_MAX      12   // Maximum number of bands
#define MAX_BLOCK_LEN      10   // Maximum length of block used during spectral amplitude encoding
#define NUM_PRED_RES_BLKS   6   // Number of Prediction Residual Blocks
#define PE_LPF_ORD         21   // Order of Pitch estimation LPF 
#define PITCH_EST_FRAME   301   // Pitch estimation frame size


#define B_NUM           (NUM_HARMS_MAX - 1)


typedef struct 
{
	Word16 e_p;
	Word16 pitch;                 // Q14.2
	Word16 ref_pitch;             // Q8.8 
	Word32 fund_freq;
	Word16 num_harms;
	Word16 num_bands;
	Word16 v_uv_dsn[NUM_HARMS_MAX];
	Word16 b_vec[NUM_HARMS_MAX + 3];
	Word16 bit_alloc[B_NUM + 4];
	Word16 sa[NUM_HARMS_MAX];
	Word16 l_uv;
	Word16 div_one_by_num_harm;
	Word16 div_one_by_num_harm_sh;
} IMBE_PARAM;

typedef struct  
{
	Word16 re;
	Word16 im;
} Cmplx16;

class imbe_vocoder_impl;
class imbe_vocoder
{
public:
    imbe_vocoder(void);	// constructor
    ~imbe_vocoder();   	// destructor
    // imbe_encode compresses 160 samples (in unsigned int format)
    // outputs u[] vectors as frame_vector[]
    void imbe_encode(int16_t *frame_vector, int16_t *snd);
    
    // imbe_decode decodes IMBE codewords (frame_vector),
    // outputs the resulting 160 audio samples (snd)
    void imbe_decode(int16_t *frame_vector, int16_t *snd);
    const IMBE_PARAM* param(void);

private:
    imbe_vocoder_impl *Impl;
};
#endif /* INCLUDED_IMBE_VOCODER_H */
