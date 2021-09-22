/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Andy Uribe CA6JAU
* 	Copyright (C) 2020 by Doug McLain AD8DP
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

#if !defined(USRP2DMR_H)
#define USRP2DMR_H

#include "DMRDefines.h"
#include "ModeConv.h"
#include "DMRNetwork.h"
#include "USRPNetwork.h"
#include "DMREmbeddedData.h"
#include "DMRLC.h"
#include "DMRFullLC.h"
#include "DMREMB.h"
#include "DMRLookup.h"
#include "Reflectors.h"
#include "UDPSocket.h"
#include "StopWatch.h"
#include "Version.h"
//#include "Thread.h"
#include "Timer.h"
#include "Sync.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"
#include "CRC.h"

#include <string>

enum TG_STATUS {
	NONE,
	WAITING_UNLINK,
	SEND_REPLY,
	SEND_PTT
};

class CUSRP2DMR
{
public:
	CUSRP2DMR(const std::string& configFile);
	~CUSRP2DMR();

	int run();

private:
	std::string      m_callsign;
	std::string       m_usrpcs;
	CConf            m_conf;
	CDMRNetwork*     m_dmrNetwork;
	CUSRPNetwork*	 m_usrpNetwork;
	CDMRLookup*      m_dmrlookup;
	CModeConv        m_conv;
	uint32_t         m_colorcode;
	uint32_t         m_srcHS;
	uint32_t         m_defsrcid;
	uint32_t         m_dstid;
	bool             m_dmrpc;
	uint32_t         m_dmrSrc;
	uint32_t         m_dmrDst;
	uint8_t          m_dmrLastDT;
	uint8_t*         m_usrpFrame;
	uint32_t         m_usrpFrames;
	uint8_t*         m_dmrFrame;
	uint32_t         m_dmrFrames;
	CDMREmbeddedData m_EmbeddedLC;
	FLCO             m_dmrflco;
	bool             m_dmrinfo;
	std::string      m_xlxmodule;
	bool             m_xlxConnected;
	CReflectors*     m_xlxReflectors;
	uint32_t         m_xlxrefl;
	bool             m_firstSync;

	bool createDMRNetwork();
	void writeXLXLink(uint32_t srcId, uint32_t dstId, CDMRNetwork* network);
};

#endif
