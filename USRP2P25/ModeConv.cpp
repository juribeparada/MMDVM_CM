/*
 *   Copyright (C) 2010,2014,2016,2018 by Jonathan Naylor G4KLX
 *   Copyright (C) 2016 Mathias Weyland, HB9FRV
 *   Copyright (C) 2018 by Andy Uribe CA6JAU
 * 	 Copyright (C) 2021 by Doug McLain AD8DP
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
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cstring>
#include <cmath>

const uint8_t IMBE_SILENCE[] = {0x04U, 0x0CU, 0xFDU, 0x7BU, 0xFBU, 0x7DU, 0xF2U, 0x7BU, 0x3DU, 0x9EU, 0x44};

CModeConv::CModeConv() :
m_p25N(0U),
m_usrpN(0U),
m_P25(5000U, "USRP2P25"),
m_USRP(5000U, "P252USRP"),
m_p25GainMultiplier(1),
m_p25Attenuate(false),
m_usrpGainMultiplier(3),
m_usrpAttenuate(true)
{
	m_mbe = new MBEVocoder();
}

CModeConv::~CModeConv()
{
}

void CModeConv::setUSRPGainAdjDb(std::string dbstring)
{
	float db = std::stof(dbstring);
	
	float ratio = powf(10.0, (db/10.0));
	if(db < 0){
		ratio = 1/ratio;
		m_usrpAttenuate = true;
	}
	m_usrpGainMultiplier = (uint16_t)roundf(ratio);
}

void CModeConv::setP25GainAdjDb(std::string dbstring)
{
	float db = std::stof(dbstring);
	
	float ratio = powf(10.0, (db/10.0));
	if(db < 0){
		ratio = 1/ratio;
		m_p25Attenuate = true;
	}
	m_p25GainMultiplier = (uint16_t)roundf(ratio);
}

void CModeConv::putUSRPHeader()
{
	m_P25.addData(&TAG_HEADER, 1U);
	m_P25.addData(IMBE_SILENCE, 11U);
	m_p25N += 1U;
}

void CModeConv::putUSRPEOT()
{
	m_P25.addData(&TAG_EOT, 1U);
	m_P25.addData(IMBE_SILENCE, 11U);
	m_p25N += 1U;
}

void CModeConv::putUSRP(int16_t* data)
{
	uint8_t imbe[11U];
	int16_t audio_adjusted[160U];
	
	for(int i = 0; i < 160; ++i){
		audio_adjusted[i] = m_usrpAttenuate ? data[i] / m_usrpGainMultiplier : data[i] * m_usrpGainMultiplier;
	}
	
	m_mbe->encode_4400(audio_adjusted, imbe);
	m_P25.addData(&TAG_DATA, 1U);
	m_P25.addData(imbe, 11U);
	//CUtils::dump(1U, "NXDN Voice:", data, 9U);
	m_p25N += 1U;
}

void CModeConv::putP25Header()
{
	const int16_t zero[160U] = {0};
	
	m_USRP.addData(&TAG_USRP_HEADER, 1U);
	m_USRP.addData(zero, 160U);
	m_usrpN += 1U;
}

void CModeConv::putP25EOT()
{
	const int16_t zero[160U] = {0};
	
	m_USRP.addData(&TAG_USRP_EOT, 1U);
	m_USRP.addData(zero, 160U);
	m_usrpN += 1U;
}

void CModeConv::putP25(uint8_t* data)
{
	int16_t audio[160U];
	int16_t audio_adjusted[160U];
	uint8_t imbe[11U];
	
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
	
	m_mbe->decode_4400(audio, imbe);
	
	for(int i = 0; i < 160; ++i){
		audio_adjusted[i] = m_p25Attenuate ? audio[i] / m_p25GainMultiplier : audio[i] * m_p25GainMultiplier;
	}
	
	m_USRP.addData(&TAG_USRP_DATA, 1U);
	m_USRP.addData(audio_adjusted, 160U);
	m_usrpN += 1U;
	
}

uint32_t CModeConv::getUSRP(int16_t* data)
{
	int16_t tag[1U];
	
	tag[0] = TAG_USRP_NODATA;

	if(m_usrpN){
		m_USRP.getData(tag, 1U);
		m_USRP.getData(data, 160U);
		--m_usrpN;
	}
	
	return tag[0];
}

uint32_t CModeConv::getP25(uint8_t* data)
{
	uint8_t tag[1U];

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
