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
#include "Utils.h"
#include "Log.h"
#include <cstdio>
#include <cassert>
#include <deque>
#include <mutex>
#include <md380_vocoder.h>

const unsigned int INTERLEAVE_TABLE_26_4[] = {
	0U, 4U,  8U, 12U, 16U, 20U, 24U, 28U, 32U, 36U, 40U, 44U, 48U, 52U, 56U, 60U, 64U, 68U, 72U, 76U, 80U, 84U, 88U, 92U, 96U, 100U,
	1U, 5U,  9U, 13U, 17U, 21U, 25U, 29U, 33U, 37U, 41U, 45U, 49U, 53U, 57U, 61U, 65U, 69U, 73U, 77U, 81U, 85U, 89U, 93U, 97U, 101U,
	2U, 6U, 10U, 14U, 18U, 22U, 26U, 30U, 34U, 38U, 42U, 46U, 50U, 54U, 58U, 62U, 66U, 70U, 74U, 78U, 82U, 86U, 90U, 94U, 98U, 102U,
	3U, 7U, 11U, 15U, 19U, 23U, 27U, 31U, 35U, 39U, 43U, 47U, 51U, 55U, 59U, 63U, 67U, 71U, 75U, 79U, 83U, 87U, 91U, 95U, 99U, 103U};

const unsigned char WHITENING_DATA[] = {0x93U, 0xD7U, 0x51U, 0x21U, 0x9CU, 0x2FU, 0x6CU, 0xD0U, 0xEFU, 0x0FU,
										0xF8U, 0x3DU, 0xF1U, 0x73U, 0x20U, 0x94U, 0xEDU, 0x1EU, 0x7CU, 0xD8U};

const uint8_t  BIT_MASK_TABLE[]  = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };
#define WRITE_BIT(p,i,b)   p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)     (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

std::mutex m;

CModeConv::CModeConv(std::string device) :
m_dstarN(0U),
m_ysfN(0U),
m_DSTAR(5000U, "YSF2DSTAR"),
m_YSF(5000U, "DSTAR2YSF")
{
	uint8_t buf[512];
	::memset(buf, 0, sizeof(buf));
	const uint8_t AMBE2000_2400_1200[17] = {0x61, 0x00, 0x0d, 0x00, 0x0a, 0x01U, 0x30U, 0x07U, 0x63U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x48U};
	fprintf(stderr, "Creating vocoder on %s\n", device.c_str());
	vocoder = new CSerialController(device, SERIAL_460800);
	fprintf(stderr, "Opening vocoder on %s\n", device.c_str());
	vocoder->open();
	fprintf(stderr, "Opened vocoder on %s\n", device.c_str());
	int lw = vocoder->write(AMBE2000_2400_1200, sizeof(AMBE2000_2400_1200));
	int lr = vocoder->read(buf, 6);
	
	fprintf(stderr, "AMBEHW:%d:%d:   ", lw, lr);
	for(int i = 0; i < lr; ++i){
		fprintf(stderr, "%02x ", buf[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
	vocoder_thread = new std::thread(&CModeConv::vocoder_thread_fn, this);
}

CModeConv::~CModeConv()
{
}

void CModeConv::vocoder_thread_fn()
{
	uint8_t dvsi_rx[512];
	uint8_t ambe[9];
	uint8_t vch[13];
	int16_t pcm[160U];
	uint8_t pcm_bytes[326U];
	int lr;
	
	const char dvsi_pcm_header[] = {0x61, 0x01, 0x42, 0x02, 0x00, 0xa0};
	const char dvsi_pkt_header[] = {0x61, 0x00, 0x0b, 0x01, 0x01, 0x48};
	std::deque<uint8_t> d;
	d.clear();
	::memset(dvsi_rx, 0, 512);
	//::memset(pcm, 0, sizeof(pcm));
	fprintf(stderr, "CModeConv::vocoder_thread_fn() started queue size == %d\n", d.max_size());
	while(1){
		lr = vocoder->read(dvsi_rx, sizeof(dvsi_rx));
		
		//fprintf(stderr, "AMBEHW:%d:  ", lr);
		for(int i = 0; i < lr; ++i){
			d.push_back(dvsi_rx[i]);
			//fprintf(stderr, "%02x ", dvsi_rx[i]);
		}
		//fprintf(stderr, "\n");
		//fflush(stderr);
		/*
		fprintf(stderr, "QUEUE %d:  ", d.size());
		for(std::deque<uint8_t>::iterator i = d.begin(); i < d.end(); ++i){
			fprintf(stderr, "%02x ", *i);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
		*/
		if((::memcmp(&(*(d.begin())), dvsi_pcm_header, 6) == 0) && d.size() > 325){
			d.erase(d.begin(), d.begin() + 6);
			
			std::deque<uint8_t>::iterator p_rx = d.begin();
			for(int i = 0; i < 160; ++i){
				pcm[i] = ((p_rx[(i*2)] << 8) & 0xff00) | (p_rx[(i*2)+1] & 0xff);
			}
			encodeYSF(pcm, vch);
			m_YSF.addData(&TAG_DATA, 1U);
			m_YSF.addData(vch, 13U);
			m_ysfN += 1U;
			d.erase(d.begin(), d.begin() + 320);
		}
		
		if(d.size() > 14){
			int j = 0;
			for(std::deque<uint8_t>::iterator i = d.begin()+6; i < d.begin()+15; ++i){
				ambe[j++] = *i;
			}
			try {
				//m.lock();
				m_DSTAR.addData(&TAG_DATA, 1U);
				m_DSTAR.addData(ambe, 9U);
				m_dstarN += 1;
				//m.unlock();
			}
			catch(std::logic_error&){
				fprintf(stderr, "Exception\n");
			}
				//m_mutex.unlock();
			d.erase(d.begin(), d.begin() + 15U);
			//}
			//else{
			//	fprintf(stderr, "read thread try_lock() failed\n");
			//}
		}
		/*
		if(m_dvsiEncode){
			fprintf(stderr, "CModeConv::vocoder_thread_fn() encode\n");
			lr = vocoder->read(dvsi_rx, sizeof(dvsi_rx));
			//fprintf(stderr, "CModeConv::vocoder_thread_fn() encoding lr == %d\n", lr);
			//for(int i = 0; i < lr; ++i){
			//	d.push(dvsi_rx[i]);
			//}
			if(lr != 15){
				fprintf(stderr, "vocoder returned %d\n", lr);
				//continue;
			}
			//if(d.size() > 14){
				//d.pop();
				//d.pop();
				//d.pop();
				//d.pop();
				//d.pop();
				//d.pop();
				//for(int i = 0; i < 9; ++i){
				//	ambe[i] = d.front();
				//	d.pop();
				//}
				//::memcpy(pcm, pcm_bytes, 320);
				m_DSTAR.addData(&TAG_DATA, 1U);
				//m_DSTAR.addData(ambe, 9U);
				m_DSTAR.addData(dvsi_rx+6, 9U);
				m_dstarN += 1;
			//}
		}
		else{
			fprintf(stderr, "CModeConv::vocoder_thread_fn() decode\n");
			lr = vocoder->read(pcm_bytes, sizeof(pcm_bytes));
			fprintf(stderr, "CModeConv::vocoder_thread_fn() decoding lr == %d\n", lr);
			//for(int i = 0; i < lr; ++i){
			//	pcm_q.push(pcm_bytes[i]);
			//}
			if(lr != 326){
				fprintf(stderr, "vocoder returned %d\n", lr);
				continue;
			}
			//if(pcm_q.size() > 325){
				uint8_t *p_rx = pcm_bytes + 6;
				for(int i = 0; i < 160; ++i){
					pcm[i] = ((p_rx[(i*2)] << 8) & 0xff00) | (p_rx[(i*2)+1] & 0xff);
				}
				
				encodeYSF(pcm, vch);
				m_YSF.addData(&TAG_DATA, 1U);
				m_YSF.addData(vch, 13U);
				m_ysfN += 1U;
			//}
		}
		
		fprintf(stderr, "AMBEHW %d:  ", lr);
		for(int i = 0; i < lr; ++i){
			fprintf(stderr, "%02x ", pcm_bytes[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
		*/
	}
}

void CModeConv::encodeYSF(int16_t *pcm, uint8_t *vch)
{
	static const uint8_t scramble_code[180] = {
	1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1,
	0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1,
	1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1,
	0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 0,
	1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 1,
	1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1,
	0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0,
	1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0,
	0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0
	};
	unsigned char ambe49[49];
	unsigned char ambe[7];
	uint8_t buf[104];
	uint8_t result[104];
	
	memset(vch, 0, 13);
	memset(ambe, 0, 7);
	
	md380_encode(ambe, pcm);
	
	for(int i = 0; i < 6; ++i){
		for(int j = 0; j < 8; j++){
			ambe49[j+(8*i)] = (1 & (ambe[i] >> (7 - j)));
		}
	}
	ambe49[48] = (1 & (ambe[6] >> 7));

	for (int i=0; i<27; i++) {
		buf[0+i*3] = ambe49[i];
		buf[1+i*3] = ambe49[i];
		buf[2+i*3] = ambe49[i];
	}
	memcpy(buf+81, ambe49+27, 22);
	buf[103] = 0;
	for (int i=0; i<104; i++) {
		buf[i] = buf[i] ^ scramble_code[i];
	}

	int x=4;
	int y=26;
	for (int i=0; i<x; i++) {
		for (int j=0; j<y; j++) {
			result[i+j*x] = buf[j+i*y];
		}
	}
	for(int i = 0; i < 13; ++i){
		for(int j = 0; j < 8; ++j){
			vch[i] |= (result[(i*8)+j] << (7-j));
		}
	}
}

void CModeConv::putDSTARHeader()
{
	uint8_t vch[13];
	::memset(vch, 0, sizeof(vch));
	m_YSF.addData(&TAG_HEADER, 1U);
	m_YSF.addData(vch, 13U);
	m_ysfN += 1U;
}

void CModeConv::putDSTAREOT()
{
	uint8_t vch[13];
	::memset(vch, 0, sizeof(vch));
	m_YSF.addData(&TAG_EOT, 1U);
	m_YSF.addData(vch, 13U);
	m_ysfN += 1U;
}

void CModeConv::putDSTAR(unsigned char* ambe)
{
	assert(ambe != NULL);

	uint8_t vch[13];
	int16_t pcm[320U];
	::memset(pcm, 0, sizeof(pcm));
	
	decode_2400(pcm, ambe);
	encodeYSF(pcm, vch);
	m_YSF.addData(&TAG_DATA, 1U);
	m_YSF.addData(vch, 13U);
	m_ysfN += 1U;
}

unsigned int CModeConv::getDSTAR(unsigned char* data)
{
	unsigned char tag[1U];

	tag[0U] = TAG_NODATA;

	if (m_dstarN >= 1U) {
		try {
			//m.lock();
			m_DSTAR.getData(tag, 1U);
			m_DSTAR.getData(data, 9U);
			m_dstarN -= 1U;
			//m.unlock();
		}
		catch(std::logic_error&){
			fprintf(stderr, "Exception\n");
		}
			return tag[0U];
	}
	else
		return TAG_NODATA;
}

void CModeConv::putYSFHeader()
{
	uint8_t ambe[9U];
	::memset(ambe, 0, 9);
	m_DSTAR.addData(&TAG_HEADER, 1U);
	m_DSTAR.addData(ambe, 9U);
	m_dstarN += 1U;
}

void CModeConv::putYSFEOT()
{
	uint8_t ambe[9U];
	::memset(ambe, 0, 9);
	m_DSTAR.addData(&TAG_EOT, 1U);
	m_DSTAR.addData(ambe, 9U);
	m_dstarN += 1U;
}

void CModeConv::putYSF(unsigned char* data)
{
	unsigned char v_tmp[7U];
	uint8_t ambe[9U];
	int16_t pcm[160];

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
		md380_decode(v_tmp, pcm);
		encode_2400(pcm, ambe);
		//m_DSTAR.addData(&TAG_DATA, 1U);
		//m_DSTAR.addData(ambe, 9U);
		//m_dstarN += 1;
	}
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

void CModeConv::decode_2400(int16_t *pcm, uint8_t *ambe)
{
	uint8_t dvsi_tx[15] = {0x61, 0x00, 0x0b, 0x01, 0x01, 0x48};
	uint8_t dvsi_rx[326];

	::memcpy(dvsi_tx + 6, ambe, 9);
	m_dvsiEncode = false;
	int lw = vocoder->write(dvsi_tx, 15);
	/*
	int lr = vocoder->read(dvsi_rx, sizeof(dvsi_rx));
	
	if(lr != 326){
		fprintf(stderr, "vocoder returned %d:%d\n", lr, lw);
		return;
	}
	
	uint8_t *p_rx = dvsi_rx + 6;
	for(int i = 0; i < 160; ++i){
		pcm[i] = ((p_rx[(i*2)] << 8) & 0xff00) | (p_rx[(i*2)+1] & 0xff);
	}
	//::memcpy(pcm, dvsi_rx + 6, 320);
	fprintf(stderr, "AMBEHW:%d:%d:  ", lw, lr);
	for(int i = 0; i < lr; ++i){
		fprintf(stderr, "%02x ", dvsi_rx[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
	*/
}

void CModeConv::encode_2400(int16_t *pcm, uint8_t *ambe)
{
	uint8_t dvsi_tx[326] = {0x61, 0x01, 0x42, 0x02, 0x00, 0xa0};
	uint8_t dvsi_rx[15];
	
	uint8_t *p_tx = dvsi_tx + 6;
	for(int i = 0; i < 160; ++i){
		p_tx[(i*2)+1] = pcm[i] & 0xff;
		p_tx[(i*2)] = pcm[i] >> 8;
	}
	
	//::memcpy(dvsi_tx + 6, pcm, 320);
	m_dvsiEncode = true;
	int lw = vocoder->write(dvsi_tx, sizeof(dvsi_tx));
	
	//int lr = vocoder->read(dvsi_rx, sizeof(dvsi_rx));
	
	//::memcpy(ambe, dvsi_rx + 6, 9);
	/*
	fprintf(stderr, "PCM:%d: ", lw);
	for(int i = 0; i < lw; ++i){
		fprintf(stderr, "%02x ", dvsi_tx[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
	*/
}
