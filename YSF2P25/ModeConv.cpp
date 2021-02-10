/*
 *   Copyright (C) 2010,2014,2016 and 2018 by Jonathan Naylor G4KLX
 *   Copyright (C) 2016 Mathias Weyland, HB9FRV
 *   Copyright (C) 2018 by Andy Uribe CA6JAU
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

#include "ModeConv.h"
#include "Golay24128.h"
#include "Hamming.h"
#include "Utils.h"

#include "Log.h"

#include <cstdio>
#include <cassert>

const unsigned int IMBE_INTERLEAVE[] = {
	0,  7, 12, 19, 24, 31, 36, 43, 48, 55, 60, 67, 72, 79, 84, 91,  96, 103, 108, 115, 120, 127, 132, 139,
	1,  6, 13, 18, 25, 30, 37, 42, 49, 54, 61, 66, 73, 78, 85, 90,  97, 102, 109, 114, 121, 126, 133, 138,
	2,  9, 14, 21, 26, 33, 38, 45, 50, 57, 62, 69, 74, 81, 86, 93,  98, 105, 110, 117, 122, 129, 134, 141,
	3,  8, 15, 20, 27, 32, 39, 44, 51, 56, 63, 68, 75, 80, 87, 92,  99, 104, 111, 116, 123, 128, 135, 140,
	4, 11, 16, 23, 28, 35, 40, 47, 52, 59, 64, 71, 76, 83, 88, 95, 100, 107, 112, 119, 124, 131, 136, 143,
	5, 10, 17, 22, 29, 34, 41, 46, 53, 58, 65, 70, 77, 82, 89, 94, 101, 106, 113, 118, 125, 130, 137, 142};

// Unpacked IMBE silence
const unsigned char IMBE_SILENCE[] = {0x04U, 0x0CU, 0xFDU, 0x7BU, 0xFBU, 0x7DU, 0xF2U, 0x7BU, 0x3DU, 0x9EU, 0x44};

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CModeConv::CModeConv() :
m_ysfN(0U),
m_p25N(0U),
m_YSF(5000U, "P252YSF"),
m_P25(5000U, "YSF2P25")
{
}

CModeConv::~CModeConv()
{
}

void CModeConv::putP25(unsigned char* data)
{
	assert(data != NULL);

	unsigned char imbe[20U];

	switch (data[0U]) {
	case 0x62U:
		::memcpy(imbe, data + 10U, 11U);
		break;
	case 0x63U:
		::memcpy(imbe, data + 1U, 11U);
		break;
	case 0x64U:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x65U:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x66U:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x67U:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x68U:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x69U:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x6AU:
		::memcpy(imbe, data + 4U, 11U);
		break;
	case 0x6BU:
		::memcpy(imbe, data + 10U, 11U);
		break;
	case 0x6CU:
		::memcpy(imbe, data + 1U, 11U);
		break;
	case 0x6DU:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x6EU:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x6FU:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x70U:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x71U:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x72U:
		::memcpy(imbe, data + 5U, 11U);
		break;
	case 0x73U:
		::memcpy(imbe, data + 4U, 11U);
		break;
	default:
		break;
	}

	m_YSF.addData(&TAG_DATA, 1U);
	m_YSF.addData(imbe, 11U);
	m_ysfN += 1U;

	//CUtils::dump(1U, "P25 IMBE unpacked:", imbe, 11U);
}

void CModeConv::putP25Header()
{
	unsigned char vch[11U];

	::memset(vch, 0, 11U);

	m_YSF.addData(&TAG_HEADER, 1U);
	m_YSF.addData(vch, 11U);
	m_ysfN += 1U;
}

void CModeConv::putP25EOT()
{
	unsigned char imbe[11U];

	::memset(imbe, 0, 11U);
	
	unsigned int fill = 5U - (m_ysfN % 5U);
	for (unsigned int i = 0U; i < fill; i++) {
		m_YSF.addData(&TAG_DATA, 1U);
		m_YSF.addData(IMBE_SILENCE, 11U);
		m_ysfN += 1U;
	}

	m_YSF.addData(&TAG_EOT, 1U);
	m_YSF.addData(imbe, 11U);
	m_ysfN += 1U;
}

void CModeConv::putYSF(unsigned char* data)
{
	assert(data != NULL);

	unsigned char vch[18U];
	unsigned char imbe[11U];

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	unsigned int offset = 0U;

	// We have a total of 5 VCH sections, iterate through each
	for (unsigned int j = 0U; j < 5U; j++, offset += 18U) {
		::memcpy(vch, data + offset, 18U);
		decode(vch, imbe);

		//CUtils::dump(1U, "YSF IMBE unpacked:", imbe, 11U);

		m_P25.addData(&TAG_DATA, 1U);
		m_P25.addData(imbe, 11U);
		m_p25N += 1U;
	}
}

void CModeConv::putYSFHeader()
{
	unsigned char imbe[11U];

	::memset(imbe, 0U, 11U);

	m_P25.addData(&TAG_HEADER, 1U);
	m_P25.addData(imbe, 11U);
	m_p25N += 1U;
}

void CModeConv::putYSFEOT()
{
	unsigned char imbe[11U];

	::memset(imbe, 0U, 11U);

	m_P25.addData(&TAG_EOT, 1U);
	m_P25.addData(imbe, 11U);
	m_p25N += 1U;
}

unsigned int CModeConv::getP25(unsigned char* data)
{
	unsigned char tag[1U];

	tag[0U] = TAG_NODATA;

	if (m_p25N >= 1U) {
		m_P25.peek(tag, 1U);

		if (tag[0U] != TAG_DATA) {
			m_P25.getData(tag, 1U);
			m_P25.getData(data, 11U);
			m_p25N -= 1U;
			return tag[0U];
		}
	}

	if (m_p25N >= 1U) {
		m_P25.getData(tag, 1U);
		m_P25.getData(data, 11U);
		m_p25N -= 1U;

		return TAG_DATA;
	}
	else
		return TAG_NODATA;
}

unsigned int CModeConv::getYSF(unsigned char* data)
{
	unsigned char tag[1U];
	unsigned char imbe[11U];

	tag[0U] = TAG_NODATA;

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;
	
	if (m_ysfN >= 1U) {
		m_YSF.peek(tag, 1U);

		if (tag[0U] != TAG_DATA) {
			m_YSF.getData(tag, 1U);
			m_YSF.getData(data, 11U);
			m_ysfN -= 1U;
			if(tag[0U] == TAG_EOT) {
				m_YSF.clear();
				m_ysfN = 0;
			}
			return tag[0U];
		}
	}

	if (m_ysfN >= 5U) {
		m_YSF.getData(tag, 1U);
		m_YSF.getData(imbe, 11U);
		encode(data, imbe);
		m_ysfN -= 1U;

		data += 18U;
		m_YSF.getData(tag, 1U);
		m_YSF.getData(imbe, 11U);
		encode(data, imbe);
		m_ysfN -= 1U;

		data += 18U;
		m_YSF.getData(tag, 1U);
		m_YSF.getData(imbe, 11U);
		encode(data, imbe);
		m_ysfN -= 1U;

		data += 18U;
		m_YSF.getData(tag, 1U);
		m_YSF.getData(imbe, 11U);
		encode(data, imbe);
		m_ysfN -= 1U;

		data += 18U;
		m_YSF.getData(tag, 1U);
		m_YSF.getData(imbe, 11U);
		encode(data, imbe);
		m_ysfN -= 1U;

		return TAG_DATA;
	}
	else
		return TAG_NODATA;
}

void CModeConv::decode(const unsigned char* data, unsigned char* imbe)
{
	bool bit[144U];

	// De-interleave
	for (unsigned int i = 0U; i < 144U; i++) {
		unsigned int n = IMBE_INTERLEAVE[i];
		bit[i] = READ_BIT(data, n);
	}

	// now ..

	// 12 voice bits     0
	// 11 golay bits     12
	//
	// 12 voice bits     23
	// 11 golay bits     35
	//
	// 12 voice bits     46
	// 11 golay bits     58
	//
	// 12 voice bits     69
	// 11 golay bits     81
	//
	// 11 voice bits     92
	//  4 hamming bits   103
	//
	// 11 voice bits     107
	//  4 hamming bits   118
	//
	// 11 voice bits     122
	//  4 hamming bits   133
	//
	//  7 voice bits     137

	// c0
	unsigned int c0data = 0U;
	for (unsigned int i = 0U; i < 12U; i++)
		c0data = (c0data << 1) | (bit[i] ? 0x01U : 0x00U);

	bool prn[114U];

	// Create the whitening vector and save it for future use
	unsigned int p = 16U * c0data;
	for (unsigned int i = 0U; i < 114U; i++) {
		p = (173U * p + 13849U) % 65536U;
		prn[i] = p >= 32768U;
	}

	// De-whiten some bits
	for (unsigned int i = 0U; i < 114U; i++)
		bit[i + 23U] ^= prn[i];

	unsigned int offset = 0U;
	for (unsigned int i = 0U; i < 12U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 0U]);
	for (unsigned int i = 0U; i < 12U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 23U]);
	for (unsigned int i = 0U; i < 12U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 46U]);
	for (unsigned int i = 0U; i < 12U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 69U]);
	for (unsigned int i = 0U; i < 11U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 92U]);
	for (unsigned int i = 0U; i < 11U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 107U]);
	for (unsigned int i = 0U; i < 11U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 122U]);
	for (unsigned int i = 0U; i < 7U; i++, offset++)
		WRITE_BIT(imbe, offset, bit[i + 137U]);
}

void CModeConv::encode(unsigned char* data, const unsigned char* imbe)
{
	assert(data != NULL);
	assert(imbe != NULL);

	bool bTemp[144U];
	bool* bit = bTemp;

	// c0
	unsigned int c0 = 0U;
	for (unsigned int i = 0U; i < 12U; i++) {
		bool b = READ_BIT(imbe, i);
		c0 = (c0 << 1) | (b ? 0x01U : 0x00U);
	}
	unsigned int g2 = CGolay24128::encode23127(c0);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c1
	unsigned int c1 = 0U;
	for (unsigned int i = 12U; i < 24U; i++) {
		bool b = READ_BIT(imbe, i);
		c1 = (c1 << 1) | (b ? 0x01U : 0x00U);
	}
	g2 = CGolay24128::encode23127(c1);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c2
	unsigned int c2 = 0;
	for (unsigned int i = 24U; i < 36U; i++) {
		bool b = READ_BIT(imbe, i);
		c2 = (c2 << 1) | (b ? 0x01U : 0x00U);
	}
	g2 = CGolay24128::encode23127(c2);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c3
	unsigned int c3 = 0U;
	for (unsigned int i = 36U; i < 48U; i++) {
		bool b = READ_BIT(imbe, i);
		c3 = (c3 << 1) | (b ? 0x01U : 0x00U);
	}
	g2 = CGolay24128::encode23127(c3);
	for (int i = 23; i >= 0; i--) {
		bit[i] = (g2 & 0x01U) == 0x01U;
		g2 >>= 1;
	}
	bit += 23U;

	// c4
	for (unsigned int i = 0U; i < 11U; i++)
		bit[i] = READ_BIT(imbe, i + 48U);
	CHamming::encode15113_1(bit);
	bit += 15U;

	// c5
	for (unsigned int i = 0U; i < 11U; i++)
		bit[i] = READ_BIT(imbe, i + 59U);
	CHamming::encode15113_1(bit);
	bit += 15U;

	// c6
	for (unsigned int i = 0U; i < 11U; i++)
		bit[i] = READ_BIT(imbe, i + 70U);
	CHamming::encode15113_1(bit);
	bit += 15U;

	// c7
	for (unsigned int i = 0U; i < 7U; i++)
		bit[i] = READ_BIT(imbe, i + 81U);

	bool prn[114U];

	// Create the whitening vector and save it for future use
	unsigned int p = 16U * c0;
	for (unsigned int i = 0U; i < 114U; i++) {
		p = (173U * p + 13849U) % 65536U;
		prn[i] = p >= 32768U;
	}

	// Whiten some bits
	for (unsigned int i = 0U; i < 114U; i++)
		bTemp[i + 23U] ^= prn[i];

	// Interleave
	for (unsigned int i = 0U; i < 144U; i++) {
		unsigned int n = IMBE_INTERLEAVE[i];
		WRITE_BIT(data, n, bTemp[i]);
	}
}
