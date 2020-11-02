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
#include "SerialController.h"
#include "YSFDefines.h"
#include "RingBuffer.h"
#include <thread>


const unsigned char TAG_HEADER = 0x00U;
const unsigned char TAG_DATA   = 0x01U;
const unsigned char TAG_LOST   = 0x02U;
const unsigned char TAG_EOT    = 0x03U;
const unsigned char TAG_NODATA = 0x04U;

#if !defined(MODECONV_H)
#define MODECONV_H

class CModeConv {
public:
	CModeConv(std::string device = "/dev/null");
	~CModeConv();

	void putDSTAR(unsigned char* bytes);
	void putDSTARHeader();
	void putDSTAREOT();
	unsigned int getDSTAR(unsigned char* bytes);
	
	void putYSF(unsigned char* bytes);
	void putYSFHeader();
	void putYSFEOT();

	unsigned int getYSF(unsigned char* bytes);

private:
	void encodeYSF(int16_t *pcm, uint8_t *vch);
	void decode_2400(int16_t *, uint8_t *);
	void encode_2400(int16_t *, uint8_t *);
	void vocoder_thread_fn();
	
	unsigned int m_dstarN;
	unsigned int m_ysfN;
	CSerialController *vocoder;
	CRingBuffer<unsigned char> m_DSTAR;
	CRingBuffer<unsigned char> m_YSF;
	bool m_dvsiEncode;
	std::thread *vocoder_thread;
	std::atomic_flag m_lock;
	
};

#endif
