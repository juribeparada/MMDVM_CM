/*
 *   Copyright (C) 2010,2014,2016,2018 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by Andy Uribe CA6JAU
 *   Copyright (C) 2020 by Doug McLain AD8DP
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

#include "Defines.h"
#include "RingBuffer.h"
#include "MBEVocoder.h"
#include "codec2/codec2.h"

#if !defined(MODECONV_H)
#define MODECONV_H

class CModeConv {
public:
	CModeConv();
	~CModeConv();

	void setM17GainAdjDb(std::string dbstring);
	void putDMR(unsigned char* data);
	void putDMRHeader();
	void putDMREOT();

	void putM17(unsigned char* data);
	void putM17Header();
	void putM17EOT();

	unsigned int getM17(unsigned char* data);
	unsigned int getDMR(unsigned char* data);

private:
	unsigned int m_m17N;
	unsigned int m_dmrN;
	CRingBuffer<unsigned char> m_M17;
	CRingBuffer<unsigned char> m_DMR;
	MBEVocoder *m_mbe;
	CCodec2 *m_c2;
	uint16_t m_m17GainMultiplier;
	bool m_m17Attenuate;
	void encode(const unsigned char* in, unsigned char* out, unsigned int offset) const;
	void decode(const unsigned char* in, unsigned char* out, unsigned int offset) const;
};

#endif
