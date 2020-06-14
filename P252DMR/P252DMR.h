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

#if !defined(P252DMR_H)
#define P252DMR_H

#include "DMRDefines.h"
#include "ModeConv.h"
#include "DMRNetwork.h"
#include "P25Network.h"
#include "DMREmbeddedData.h"
#include "DMRLC.h"
#include "DMRFullLC.h"
#include "DMREMB.h"
#include "DMRLookup.h"
#include "Reflectors.h"
#include "UDPSocket.h"
#include "StopWatch.h"
#include "Version.h"
#include "Thread.h"
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

class CP252DMR
{
public:
	CP252DMR(const std::string& configFile);
	~CP252DMR();

	int run();

private:
	std::string      m_callsign;
	CConf            m_conf;
	CDMRNetwork*     m_dmrNetwork;
	CP25Network*	 m_p25Network;
	CDMRLookup*      m_dmrlookup;
	CModeConv        m_conv;
	unsigned int     m_colorcode;
	unsigned int     m_srcHS;
	unsigned int     m_defsrcid;
	unsigned int     m_dstid;
	bool             m_dmrpc;
	unsigned int     m_dmrSrc;
	unsigned int     m_dmrDst;
	unsigned int     m_p25Src;
	unsigned int     m_p25Dst;
	unsigned char    m_dmrLastDT;
	unsigned char*   m_p25Frame;
	unsigned char*   m_dmrFrame;
	unsigned int     m_dmrFrames;
	unsigned int     m_p25Frames;
	bool			 m_p25info;
	CDMREmbeddedData m_EmbeddedLC;
	FLCO             m_dmrflco;
	bool             m_dmrinfo;
	std::string      m_xlxmodule;
	bool             m_xlxConnected;
	CReflectors*     m_xlxReflectors;
	unsigned int     m_xlxrefl;
	bool             m_firstSync;

	bool createDMRNetwork();
	void writeXLXLink(unsigned int srcId, unsigned int dstId, CDMRNetwork* network);
};

#endif
