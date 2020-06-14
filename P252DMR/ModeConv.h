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

#if !defined(MODECONV_H)
#define MODECONV_H

class CModeConv {
public:
	CModeConv();
	~CModeConv();

	void putDMR(unsigned char* data);
	void putDMRHeader();
	void putDMREOT();

	void putP25(unsigned char* data);
	void putP25Header();
	void putP25EOT();

	unsigned int getP25(unsigned char* data);
	unsigned int getDMR(unsigned char* data);

private:
	unsigned int m_p25N;
	unsigned int m_dmrN;
	CRingBuffer<unsigned char> m_P25;
	CRingBuffer<unsigned char> m_DMR;
	MBEVocoder *m_mbe;
	void encode(const unsigned char* in, unsigned char* out, unsigned int offset) const;
	void decode(const unsigned char* in, unsigned char* out, unsigned int offset) const;
};

#endif
