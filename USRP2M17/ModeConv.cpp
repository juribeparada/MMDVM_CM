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

CModeConv::CModeConv() :
m_m17N(0U),
m_usrpN(0U),
m_M17(5000U, "USRP2M17"),
m_USRP(5000U, "M172USRP"),
m_m17GainMultiplier(1),
m_m17Attenuate(false),
m_usrpGainMultiplier(3),
m_usrpAttenuate(true)
{
	m_c2 = new CCodec2(true);
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

void CModeConv::setM17GainAdjDb(std::string dbstring)
{
	float db = std::stof(dbstring);
	
	float ratio = powf(10.0, (db/10.0));
	if(db < 0){
		ratio = 1/ratio;
		m_m17Attenuate = true;
	}
	m_m17GainMultiplier = (uint16_t)roundf(ratio);
}

void CModeConv::putUSRPHeader()
{
	const uint8_t quiet[] = { 0x00u, 0x01u, 0x43u, 0x09u, 0xe4u, 0x9cu, 0x08u, 0x21u };

	m_M17.addData(&TAG_HEADER, 1U);
	m_M17.addData(quiet, 8U);
	m_m17N += 1U;
}

void CModeConv::putUSRPEOT()
{
	const uint8_t quiet[] = { 0x00u, 0x01u, 0x43u, 0x09u, 0xe4u, 0x9cu, 0x08u, 0x21u };
	
	if((m_m17N % 2) == 0){
		m_M17.addData(&TAG_DATA, 1U);
		m_M17.addData(quiet, 8U);
		m_m17N += 1U;
	}

	m_M17.addData(&TAG_EOT, 1U);
	m_M17.addData(quiet, 8U);
	m_m17N += 1U;
}

void CModeConv::putUSRP(int16_t* data)
{
	uint8_t codec2[8U];
	
	::memset(codec2, 0, sizeof(codec2));
	
	int16_t audio_adjusted[160U];
	
	for(int i = 0; i < 160; ++i){
		audio_adjusted[i] = m_usrpAttenuate ? data[i] / m_usrpGainMultiplier : data[i] * m_usrpGainMultiplier;
	}
	
	m_c2->codec2_encode(codec2, audio_adjusted);
	m_M17.addData(&TAG_DATA, 1U);
	m_M17.addData(codec2, 8U);
	m_m17N += 1U;
}

void CModeConv::putM17Header()
{
	const int16_t zero[160U] = {0};
	
	m_USRP.addData(&TAG_USRP_HEADER, 1U);
	m_USRP.addData(zero, 160U);
	m_usrpN += 1U;
}

void CModeConv::putM17EOT()
{
	const int16_t zero[160U] = {0};
	
	m_USRP.addData(&TAG_USRP_EOT, 1U);
	m_USRP.addData(zero, 160U);
	m_usrpN += 1U;
}

void CModeConv::putM17(uint8_t* data)
{
	int16_t audio[320U];
	int16_t audio_adjusted[320U];
	uint8_t codec2[8U];
	size_t s = 160;
	
	::memset(audio, 0, sizeof(audio));
	::memcpy(codec2, &data[36], 8);
	
	if((data[19] & 0x06U) == 0x04U){	//"3200 Voice";
		m_c2->codec2_set_mode(true);
		s = 160;
	}
	else{								//"1600 V/D";
		m_c2->codec2_set_mode(false);
		s = 320;
	}
	
	m_c2->codec2_decode(audio, codec2);
	
	for(size_t i = 0; i < s; ++i){
		audio_adjusted[i] = m_m17Attenuate ? audio[i] / m_m17GainMultiplier : audio[i] * m_m17GainMultiplier;
	}
	
	m_USRP.addData(&TAG_USRP_DATA, 1U);
	m_USRP.addData(audio_adjusted, 160);
	m_usrpN += 1U;
	
	int16_t *p = audio_adjusted;
	
	if(s == 160){
		::memcpy(codec2, &data[44], 8);
		m_c2->codec2_decode(audio, codec2);
		for(int i = 0; i < 160; ++i){
			audio_adjusted[i] = m_m17Attenuate ? audio[i] / m_m17GainMultiplier : audio[i] * m_m17GainMultiplier;
		}
	}
	else{
		p = &audio_adjusted[160U];
	}
	
	m_USRP.addData(&TAG_USRP_DATA, 1U);
	m_USRP.addData(p, 160U);
	m_usrpN += 1U;
	m_c2->codec2_set_mode(true);
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

uint32_t CModeConv::getM17(uint8_t* data)
{
	uint8_t tag[2U];

	tag[0U] = TAG_NODATA;
	tag[1U] = TAG_NODATA;

	if (m_m17N >= 2U) {
		m_M17.getData(tag, 1U);
		m_M17.getData(data, 8U);
		m_M17.getData(tag+1, 1U);
		m_M17.getData(data+8, 8U);
		m_m17N -= 2U;
	}
	return (tag[1U] == TAG_EOT) ? tag[1U] : tag[0];
}
