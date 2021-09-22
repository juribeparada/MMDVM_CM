/* 
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Andy Uribe CA6JAU
* 	Copyright (C) 2020 by Doug McLain AD8DP
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <cstring>
#include "MBEVocoder.h"


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
