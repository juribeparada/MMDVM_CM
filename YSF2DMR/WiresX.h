/*
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Manuel Sanchez EA7EE
*   Copyright (C) 2018,2019 by Andy Uribe CA6JAU
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

#if !defined(WIRESX_H)
#define	WIRESX_H

#include "YSFNetwork.h"
#include "DMRNetwork.h"
#include "Thread.h"
#include "Timer.h"
#include "StopWatch.h"
#include "RingBuffer.h"

#include <vector>
#include <string>

enum WX_STATUS {
	WXS_NONE,
	WXS_CONNECT,
	WXS_DISCONNECT,
	WXS_DX,
	WXS_ALL,
	WXS_FAIL
};

enum WXSI_STATUS {
	WXSI_NONE,
	WXSI_DX,
	WXSI_CONNECT,
	WXSI_DISCONNECT,
	WXSI_ALL,
	WXSI_SEARCH,
	WXSI_CATEGORY
};

class CTGReg {
public:
	CTGReg() :
	m_id(),
	m_opt(),
	m_name(),
	m_desc()
	{
	}

	std::string  m_id;
	std::string  m_opt;
	std::string  m_name;
	std::string  m_desc;
};

class CWiresX {
public:
	CWiresX(const std::string& callsign, const std::string& suffix, CYSFNetwork* network, std::string tgfile, bool makeUpper);
	~CWiresX();

	bool start();

	WX_STATUS process(const unsigned char* data, const unsigned char* source, unsigned char fi, unsigned char dt, unsigned char fn, unsigned char ft);

	unsigned int getDstID();
	unsigned int getOpt(unsigned int id);
	unsigned int getFullDstID();

	CTGReg* findById(unsigned int id);
	std::vector<CTGReg*>& TGSearch(const std::string& name);

	void processConnect(int reflector);
	void processDisconnect(const unsigned char* source = NULL);
	void setInfo(const std::string& name, unsigned int txFrequency, unsigned int rxFrequency, int reflector);
	void sendConnectReply(unsigned int reflector);
	void sendDisconnectReply();
	void clock(unsigned int ms);

private:
	std::string          m_callsign;
	std::string          m_node;
	std::string          m_id;
	std::string          m_name;
	unsigned int         m_txFrequency;
	unsigned int         m_rxFrequency;
	unsigned int         m_dstID;
	unsigned int         m_fulldstID;
	CYSFNetwork*         m_network;
	unsigned char*       m_command;
	CTimer               m_timer;
	unsigned char        m_seqNo;
	unsigned char*       m_header;
	unsigned char*       m_csd1;
	unsigned char*       m_csd2;
	unsigned char*       m_csd3;
	WXSI_STATUS          m_status;
	unsigned int         m_start;
	std::string          m_search;
	std::vector<CTGReg*> m_currTGList;
	std::vector<CTGReg*> m_TGSearch;
	std::vector<CTGReg*> m_category;
	bool                 m_makeUpper;
	CStopWatch           m_txWatch;
	CRingBuffer<unsigned char> m_bufferTX;

	WX_STATUS processConnect(const unsigned char* source, const unsigned char* data);
	void processDX(const unsigned char* source);
	void processAll(const unsigned char* source, const unsigned char* data);
	void processCategory(const unsigned char* source, const unsigned char* data);

	void sendDXReply();
	void sendAllReply();
	void sendSearchReply();
	void sendSearchNotFoundReply();
	void sendCategoryReply();

	void createReply(const unsigned char* data, unsigned int length);
	void writeData(const unsigned char* data);
	unsigned char calculateFT(unsigned int length, unsigned int offset) const;
};

#endif
