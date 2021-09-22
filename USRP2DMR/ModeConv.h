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

	void setDMRGainAdjDb(std::string dbstring);
	void setUSRPGainAdjDb(std::string dbstring);
	void putDMR(uint8_t* data);
	void putDMRHeader();
	void putDMREOT();

	void putUSRP(int16_t* data);
	void putUSRPHeader();
	void putUSRPEOT();

	uint32_t getUSRP(int16_t* data);
	uint32_t getDMR(uint8_t* data);

private:
	uint32_t m_usrpN;
	uint32_t m_dmrN;
	CRingBuffer<int16_t> m_USRP;
	CRingBuffer<uint8_t> m_DMR;
	MBEVocoder *m_mbe;
	uint16_t m_usrpGainMultiplier;
	bool m_usrpAttenuate;
	uint16_t m_dmrGainMultiplier;
	bool m_dmrAttenuate;
	void encode(const uint8_t* in, uint8_t* out, uint32_t offset) const;
	void decode(const uint8_t* in, uint8_t* out, uint32_t offset) const;
};

#endif
