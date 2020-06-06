#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cmath>
#include <cstring>

#include "mbevocoder.h"
#include "md380_vocoder.h"

const uint8_t  BIT_MASK_TABLE8[]  = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };
#define WRITE_BIT8(p,i,b)   p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE8[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE8[(i)&7])
#define READ_BIT8(p,i)     (p[(i)>>3] & BIT_MASK_TABLE8[(i)&7])

MBEVocoder::MBEVocoder(void)
{
}

void MBEVocoder::decode_4400(int16_t *pcm, uint8_t *imbe)
{
	int16_t frame[8U] = {0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};
	unsigned int offset = 0U;

	int16_t mask = 0x0800;
	for (unsigned int i = 0U; i < 12U; i++, mask >>= 1, offset++)
		frame[0U] |= READ_BIT8(imbe, offset) != 0x00U ? mask : 0x0000;

	mask = 0x0800;
	for (unsigned int i = 0U; i < 12U; i++, mask >>= 1, offset++)
		frame[1U] |= READ_BIT8(imbe, offset) != 0x00U ? mask : 0x0000;

	mask = 0x0800;
	for (unsigned int i = 0U; i < 12U; i++, mask >>= 1, offset++)
		frame[2U] |= READ_BIT8(imbe, offset) != 0x00U ? mask : 0x0000;

	mask = 0x0800;
	for (unsigned int i = 0U; i < 12U; i++, mask >>= 1, offset++)
		frame[3U] |= READ_BIT8(imbe, offset) != 0x00U ? mask : 0x0000;

	mask = 0x0400;
	for (unsigned int i = 0U; i < 11U; i++, mask >>= 1, offset++)
		frame[4U] |= READ_BIT8(imbe, offset) != 0x00U ? mask : 0x0000;

	mask = 0x0400;
	for (unsigned int i = 0U; i < 11U; i++, mask >>= 1, offset++)
		frame[5U] |= READ_BIT8(imbe, offset) != 0x00U ? mask : 0x0000;

	mask = 0x0400;
	for (unsigned int i = 0U; i < 11U; i++, mask >>= 1, offset++)
		frame[6U] |= READ_BIT8(imbe, offset) != 0x00U ? mask : 0x0000;

	mask = 0x0040;
	for (unsigned int i = 0U; i < 7U; i++, mask >>= 1, offset++)
		frame[7U] |= READ_BIT8(imbe, offset) != 0x00U ? mask : 0x0000;

	vocoder.imbe_decode(frame, pcm);
}

void MBEVocoder::encode_4400(int16_t *pcm, uint8_t *imbe)
{
	int16_t frame_vector[8];
	memset(imbe, 0, 9);

	vocoder.imbe_encode(frame_vector, pcm);
	//vocoder.set_gain_adjust(1.0);
	uint32_t offset = 0U;
	int16_t mask = 0x0800;
	
	for (uint32_t i = 0U; i < 12U; i++, mask >>= 1, offset++)
		WRITE_BIT8(imbe, offset, (frame_vector[0U] & mask) != 0);

	mask = 0x0800;
	for (uint32_t i = 0U; i < 12U; i++, mask >>= 1, offset++)
		WRITE_BIT8(imbe, offset, (frame_vector[1U] & mask) != 0);

	mask = 0x0800;
	for (uint32_t i = 0U; i < 12U; i++, mask >>= 1, offset++)
		WRITE_BIT8(imbe, offset, (frame_vector[2U] & mask) != 0);

	mask = 0x0800;
	for (uint32_t i = 0U; i < 12U; i++, mask >>= 1, offset++)
		WRITE_BIT8(imbe, offset, (frame_vector[3U] & mask) != 0);

	mask = 0x0400;
	for (uint32_t i = 0U; i < 11U; i++, mask >>= 1, offset++)
		WRITE_BIT8(imbe, offset, (frame_vector[4U] & mask) != 0);

	mask = 0x0400;
	for (uint32_t i = 0U; i < 11U; i++, mask >>= 1, offset++)
		WRITE_BIT8(imbe, offset, (frame_vector[5U] & mask) != 0);

	mask = 0x0400;
	for (uint32_t i = 0U; i < 11U; i++, mask >>= 1, offset++)
		WRITE_BIT8(imbe, offset, (frame_vector[6U] & mask) != 0);

	mask = 0x0040;
	for (uint32_t i = 0U; i < 7U; i++, mask >>= 1, offset++)
		WRITE_BIT8(imbe, offset, (frame_vector[7U] & mask) != 0);

}

void MBEVocoder::decode_2450(int16_t *pcm, uint8_t *ambe49)
{
	md380_decode(ambe49, pcm);
}

void MBEVocoder::encode_2450(int16_t *pcm, uint8_t *ambe49)
{
	md380_encode(ambe49, pcm);
}
