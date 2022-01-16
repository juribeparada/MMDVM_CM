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

	void setM17GainAdjDb(std::string);
	void putM17(unsigned char*);
	void putM17Header();
	void putM17EOT();
	
	void putYSF(unsigned char*);
	void putYSFHeader();
	void putYSFEOT();

	unsigned int getM17(unsigned char*);
	unsigned int getYSF(unsigned char*);

private:
	unsigned int m_m17N;
	unsigned int m_ysfN;
	CRingBuffer<unsigned char> m_M17;
	CRingBuffer<unsigned char> m_YSF;
	MBEVocoder *m_mbe;
	CCodec2 *m_c2;
	uint16_t m_m17GainMultiplier;
	bool m_m17Attenuate;
	void encodeYSF(int16_t *, uint8_t *);
};

#endif
