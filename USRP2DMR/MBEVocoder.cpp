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
#include <md380_vocoder.h>
#include "MBEVocoder.h"

const uint8_t  BIT_MASK_TABLE8[]  = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };
#define WRITE_BIT8(p,i,b)   p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE8[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE8[(i)&7])
#define READ_BIT8(p,i)     (p[(i)>>3] & BIT_MASK_TABLE8[(i)&7])

MBEVocoder::MBEVocoder(void)
{
	m_mbeenc = new MBEEncoder();
	m_mbeenc->set_dmr_mode();
	m_mbeenc->set_gain_adjust(2.5);
	md380_init();
}

void MBEVocoder::decode_2450(int16_t *pcm, uint8_t *ambe49)
{
	md380_decode(ambe49, pcm);
}

void MBEVocoder::encode_2450(int16_t *pcm, uint8_t *ambe49)
{
	md380_encode(ambe49, pcm);
}

void MBEVocoder::encode_dmr(int16_t *pcm, uint8_t *ambe)
{
	m_mbeenc->encode(pcm, ambe);
}
