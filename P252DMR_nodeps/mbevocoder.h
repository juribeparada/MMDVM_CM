#ifndef INCLUDED_AMBE_ENCODER_H
#define INCLUDED_AMBE_ENCODER_H

#include <stdint.h>
#include "imbe_vocoder.h"

class MBEVocoder {
public:
	void decode_4400(int16_t *, uint8_t *);
	void encode_4400(int16_t *, uint8_t *);
	void decode_2450(int16_t *, uint8_t *);
	void encode_2450(int16_t *, uint8_t *);
	MBEVocoder(void);

private:
	imbe_vocoder vocoder;
};

#endif /* INCLUDED_AMBE_ENCODER_H */
