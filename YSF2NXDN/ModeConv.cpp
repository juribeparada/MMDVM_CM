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
#include "YSFConvolution.h"
#include "CRC.h"
#include "Utils.h"

#include "Log.h"

#include <cstdio>
#include <cassert>

const unsigned char BIT_MASK_TABLE[] = {0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U};

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

const unsigned int INTERLEAVE_TABLE_26_4[] = {
	0U, 4U,  8U, 12U, 16U, 20U, 24U, 28U, 32U, 36U, 40U, 44U, 48U, 52U, 56U, 60U, 64U, 68U, 72U, 76U, 80U, 84U, 88U, 92U, 96U, 100U,
	1U, 5U,  9U, 13U, 17U, 21U, 25U, 29U, 33U, 37U, 41U, 45U, 49U, 53U, 57U, 61U, 65U, 69U, 73U, 77U, 81U, 85U, 89U, 93U, 97U, 101U,
	2U, 6U, 10U, 14U, 18U, 22U, 26U, 30U, 34U, 38U, 42U, 46U, 50U, 54U, 58U, 62U, 66U, 70U, 74U, 78U, 82U, 86U, 90U, 94U, 98U, 102U,
	3U, 7U, 11U, 15U, 19U, 23U, 27U, 31U, 35U, 39U, 43U, 47U, 51U, 55U, 59U, 63U, 67U, 71U, 75U, 79U, 83U, 87U, 91U, 95U, 99U, 103U};

const unsigned char WHITENING_DATA[] = {0x93U, 0xD7U, 0x51U, 0x21U, 0x9CU, 0x2FU, 0x6CU, 0xD0U, 0xEFU, 0x0FU,
										0xF8U, 0x3DU, 0xF1U, 0x73U, 0x20U, 0x94U, 0xEDU, 0x1EU, 0x7CU, 0xD8U};

const unsigned char AMBE_SILENCE[] = {0xF8U, 0x01U, 0xA9U, 0x9FU, 0x8CU, 0xE0U, 0x80U};
const unsigned char YSF_SILENCE[] = {0x7BU, 0xB2U, 0x8EU, 0x43U, 0x36U, 0xE4U, 0xA2U, 0x39U, 0x78U, 0x49U, 0x33U, 0x68U, 0x33U};

CModeConv::CModeConv() :
m_ysfN(0U),
m_nxdnN(0U),
m_YSF(5000U, "NXDN2YSF"),
m_NXDN(5000U, "YSF2NXDN")
{
}

CModeConv::~CModeConv()
{
}

void CModeConv::putNXDN(unsigned char* data)
{
	assert(data != NULL);

	data += 5U;

	unsigned int dat_a1 = 0U, dat_b1 = 0U, dat_c1 = 0U;
	unsigned int dat_a2 = 0U, dat_b2 = 0U, dat_c2 = 0U;
	unsigned int dat_a3 = 0U, dat_b3 = 0U, dat_c3 = 0U;
	unsigned int dat_a4 = 0U, dat_b4 = 0U, dat_c4 = 0U;

	unsigned int MASK = 0x800U;
	for (unsigned int i = 0U; i < 12U; i++, MASK >>= 1) {
		if (READ_BIT(data, i + 0U))
			dat_a1 |= MASK;
		if (READ_BIT(data, i + 12U))
			dat_b1 |= MASK;
		if (READ_BIT(data, i + 49U))
			dat_a2 |= MASK;
		if (READ_BIT(data, i + 61U))
			dat_b2 |= MASK;
		if (READ_BIT(data, i + 112U))
			dat_a3 |= MASK;
		if (READ_BIT(data, i + 124U))
			dat_b3 |= MASK;
		if (READ_BIT(data, i + 161U))
			dat_a4 |= MASK;
		if (READ_BIT(data, i + 173U))
			dat_b4 |= MASK;
	}

	MASK = 0x1000000U;
	for (unsigned int i = 0U; i < 25U; i++, MASK >>= 1) {
		if (READ_BIT(data, i + 24U))
			dat_c1 |= MASK;
		if (READ_BIT(data, i + 73U))
			dat_c2 |= MASK;
		if (READ_BIT(data, i + 136U))
			dat_c3 |= MASK;
		if (READ_BIT(data, i + 185U))
			dat_c4 |= MASK;
	}

	putAMBE2YSF(dat_a1, dat_b1, dat_c1);
	putAMBE2YSF(dat_a2, dat_b2, dat_c2);
	putAMBE2YSF(dat_a3, dat_b3, dat_c3);
	putAMBE2YSF(dat_a4, dat_b4, dat_c4);
}

void CModeConv::putAMBE2YSF(unsigned int dat_a, unsigned int dat_b, unsigned int dat_c)
{
	unsigned char vch[13U];
	unsigned char ysfFrame[13U];
	::memset(vch, 0U, 13U);
	::memset(ysfFrame, 0, 13U);

	for (unsigned int i = 0U; i < 12U; i++) {
		bool s = (dat_a << (20U + i)) & 0x80000000U;
		WRITE_BIT(vch, 3*i + 0U, s);
		WRITE_BIT(vch, 3*i + 1U, s);
		WRITE_BIT(vch, 3*i + 2U, s);
	}
	
	for (unsigned int i = 0U; i < 12U; i++) {
		bool s = (dat_b << (20U + i)) & 0x80000000U;
		WRITE_BIT(vch, 3*(i + 12U) + 0U, s);
		WRITE_BIT(vch, 3*(i + 12U) + 1U, s);
		WRITE_BIT(vch, 3*(i + 12U) + 2U, s);
	}
	
	for (unsigned int i = 0U; i < 3U; i++) {
		bool s = (dat_c << (7U + i)) & 0x80000000U;
		WRITE_BIT(vch, 3*(i + 24U) + 0U, s);
		WRITE_BIT(vch, 3*(i + 24U) + 1U, s);
		WRITE_BIT(vch, 3*(i + 24U) + 2U, s);
	}

	for (unsigned int i = 0U; i < 22U; i++) {
		bool s = (dat_c << (10U + i)) & 0x80000000U;
		WRITE_BIT(vch, i + 81U, s);
	}
	
	WRITE_BIT(vch, 103U, 0U);

	// Scramble
	for (unsigned int i = 0U; i < 13U; i++)
		vch[i] ^= WHITENING_DATA[i];

	// Interleave
	for (unsigned int i = 0U; i < 104U; i++) {
		unsigned int n = INTERLEAVE_TABLE_26_4[i];
		bool s = READ_BIT(vch, i);
		WRITE_BIT(ysfFrame, n, s);
	}

	m_YSF.addData(&TAG_DATA, 1U);
	m_YSF.addData(ysfFrame, 13U);
	//CUtils::dump(1U, "VCH V/D type 2:", ysfFrame, 13U);
	
	m_ysfN += 1U;
}

void CModeConv::putYSF(unsigned char* data)
{
	unsigned char v_tmp[7U];

	assert(data != NULL);

	::memset(v_tmp, 0, 7U);

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;

	unsigned int offset = 40U; // DCH(0)

	// We have a total of 5 VCH sections, iterate through each
	for (unsigned int j = 0U; j < 5U; j++, offset += 144U) {

		unsigned char vch[13U];
		unsigned int dat_a = 0U;
		unsigned int dat_b = 0U;
		unsigned int dat_c = 0U;

		// Deinterleave
		for (unsigned int i = 0U; i < 104U; i++) {
			unsigned int n = INTERLEAVE_TABLE_26_4[i];
			bool s = READ_BIT(data, offset + n);
			WRITE_BIT(vch, i, s);
		}

		// "Un-whiten" (descramble)
		for (unsigned int i = 0U; i < 13U; i++)
			vch[i] ^= WHITENING_DATA[i];
	
		for (unsigned int i = 0U; i < 12U; i++) {
			dat_a <<= 1U;
			if (READ_BIT(vch, 3U*i + 1U))
				dat_a |= 0x01U;;
		}
		
		for (unsigned int i = 0U; i < 12U; i++) {
			dat_b <<= 1U;
			if (READ_BIT(vch, 3U*(i + 12U) + 1U))
				dat_b |= 0x01U;;
		}
		
		for (unsigned int i = 0U; i < 3U; i++) {
			dat_c <<= 1U;
			if (READ_BIT(vch, 3U*(i + 24U) + 1U))
				dat_c |= 0x01U;;
		}

		for (unsigned int i = 0U; i < 22U; i++) {
			dat_c <<= 1U;
			if (READ_BIT(vch, i + 81U))
				dat_c |= 0x01U;;
		}

		for (unsigned int i = 0U; i < 12U; i++) {
			bool s1 = (dat_a << (i + 20U)) & 0x80000000;
			bool s2 = (dat_b << (i + 20U)) & 0x80000000;
			WRITE_BIT(v_tmp, i, s1);
			WRITE_BIT(v_tmp, i + 12U, s2);
		}

		for (unsigned int i = 0U; i < 25U; i++) {
			bool s = (dat_c << (i + 7U)) & 0x80000000;
			WRITE_BIT(v_tmp, i + 24U, s);
		}

		m_NXDN.addData(&TAG_DATA, 1U);
		m_NXDN.addData(v_tmp, 7U);

		//CUtils::dump(1U, "NXDN Voice:", v_tmp, 7U);

		m_nxdnN += 1U;
	}
}

void CModeConv::putNXDNHeader()
{
	unsigned char vch[13U];

	::memset(vch, 0, 13U);

	m_YSF.addData(&TAG_HEADER, 1U);
	m_YSF.addData(vch, 13U);
	m_ysfN += 1U;
}

void CModeConv::putNXDNEOT()
{
	unsigned char vch[13U];

	::memset(vch, 0, 13U);
	
	unsigned int fill = 5U - (m_ysfN % 5U);
	for (unsigned int i = 0U; i < fill; i++) {
		m_YSF.addData(&TAG_DATA, 1U);
		m_YSF.addData(YSF_SILENCE, 13U);
		m_ysfN += 1U;
	}

	m_YSF.addData(&TAG_EOT, 1U);
	m_YSF.addData(vch, 13U);
	m_ysfN += 1U;
}

void CModeConv::putYSFHeader()
{
	unsigned char v_nxdn[7U];

	::memset(v_nxdn, 0U, 7U);

	m_NXDN.addData(&TAG_HEADER, 1U);
	m_NXDN.addData(v_nxdn, 7U);
	m_nxdnN += 1U;
}

void CModeConv::putYSFEOT()
{
	unsigned char v_nxdn[7U];

	::memset(v_nxdn, 0U, 7U);
	
	unsigned int fill = 4U - (m_nxdnN % 4U);
	for (unsigned int i = 0U; i < fill; i++) {
		m_NXDN.addData(&TAG_DATA, 1U);
		m_NXDN.addData(AMBE_SILENCE, 7U);
		m_nxdnN += 1U;
	}

	m_NXDN.addData(&TAG_EOT, 1U);
	m_NXDN.addData(v_nxdn, 7U);
	m_nxdnN += 1U;
}

unsigned int CModeConv::getNXDN(unsigned char* data)
{
	unsigned char tmp[7U];
	unsigned char tag[1U];

	tag[0U] = TAG_NODATA;

	if (m_nxdnN >= 1U) {
		m_NXDN.peek(tag, 1U);

		if (tag[0U] != TAG_DATA) {
			m_NXDN.getData(tag, 1U);
			m_NXDN.getData(data, 7U);
			m_nxdnN -= 1U;
			return tag[0U];
		}
	}

	if (m_nxdnN >= 4U) {
		data += 5U;

		m_NXDN.getData(tag, 1U);
		m_NXDN.getData(tmp, 7U);
		for (unsigned int i = 0U; i < 49U; i++) {
			bool s = READ_BIT(tmp, i);
			WRITE_BIT(data, i + 0U, s);
		}

		m_NXDN.getData(tag, 1U);
		m_NXDN.getData(tmp, 7U);
		for (unsigned int i = 0U; i < 49U; i++) {
			bool s = READ_BIT(tmp, i);
			WRITE_BIT(data, i + 49U, s);
		}

		m_NXDN.getData(tag, 1U);
		m_NXDN.getData(tmp, 7U);
		for (unsigned int i = 0U; i < 49U; i++) {
			bool s = READ_BIT(tmp, i);
			WRITE_BIT(data, i + 112U, s);
		}

		m_NXDN.getData(tag, 1U);
		m_NXDN.getData(tmp, 7U);
		for (unsigned int i = 0U; i < 49U; i++) {
			bool s = READ_BIT(tmp, i);
			WRITE_BIT(data, i + 161U, s);
		}

		m_nxdnN -= 4U;

		return TAG_DATA;
	}
	else
		return TAG_NODATA;
}

unsigned int CModeConv::getYSF(unsigned char* data)
{
	unsigned char tag[1U];

	tag[0U] = TAG_NODATA;

	data += YSF_SYNC_LENGTH_BYTES + YSF_FICH_LENGTH_BYTES;
	
	if (m_ysfN >= 1U) {
		m_YSF.peek(tag, 1U);

		if (tag[0U] != TAG_DATA) {
			m_YSF.getData(tag, 1U);
			m_YSF.getData(data, 13U);
			m_ysfN -= 1U;
			return tag[0U];
		}
	}

	if (m_ysfN >= 5U) {
		data += 5U;
		m_YSF.getData(tag, 1U);
		m_YSF.getData(data, 13U);
		m_ysfN -= 1U;

		data += 18U;
		m_YSF.getData(tag, 1U);
		m_YSF.getData(data, 13U);
		m_ysfN -= 1U;

		data += 18U;
		m_YSF.getData(tag, 1U);
		m_YSF.getData(data, 13U);
		m_ysfN -= 1U;

		data += 18U;
		m_YSF.getData(tag, 1U);
		m_YSF.getData(data, 13U);
		m_ysfN -= 1U;

		data += 18U;
		m_YSF.getData(tag, 1U);
		m_YSF.getData(data, 13U);
		m_ysfN -= 1U;

		return TAG_DATA;
	}
	else
		return TAG_NODATA;
}
