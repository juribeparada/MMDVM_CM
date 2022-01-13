#ifndef INCLUDED_AMBE_H
#define INCLUDED_AMBE_H
#ifdef __cplusplus

extern "C" {
#endif
#include "mbelib.h"
struct mbe_tones
{
  int ID;
  int AD;
  int n;
};
typedef struct mbe_tones mbe_tone;

int mbe_dequantizeAmbe2250Parms (mbe_parms * cur_mp, mbe_parms * prev_mp, const int *b);
int mbe_dequantizeAmbe2400Parms (mbe_parms * cur_mp, mbe_parms * prev_mp, const int *b);
int mbe_dequantizeAmbeTone(mbe_tone * tone, const int *u);
#ifdef __cplusplus
}
#endif
#endif /* INCLUDED_AMBE_H */
