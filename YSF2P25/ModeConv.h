/*
 *   Copyright (C) 2010,2014,2016 by Jonathan Naylor G4KLX
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

#include "Defines.h"
#include "YSFDefines.h"
#include "RingBuffer.h"

#if !defined(MODECONV_H)
#define MODECONV_H

class CModeConv {
public:
	CModeConv();
	~CModeConv();

	void putP25(unsigned char* data);
	void putP25Header();
	void putP25EOT();

	void putYSF(unsigned char* data);
	void putYSFHeader();
	void putYSFEOT();

	unsigned int getYSF(unsigned char* data);
	unsigned int getP25(unsigned char* data);

private:
	unsigned int m_ysfN;
	unsigned int m_p25N;
	CRingBuffer<unsigned char> m_YSF;
	CRingBuffer<unsigned char> m_P25;
	void decode(const unsigned char* data, unsigned char* imbe);
	void encode(unsigned char* data, const unsigned char* imbe);

};

#endif
