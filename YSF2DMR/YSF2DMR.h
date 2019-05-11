/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Andy Uribe CA6JAU
*   Copyright (C) 2018 by Manuel Sanchez EA7EE
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

#if !defined(YSF2DMR_H)
#define YSF2DMR_H

#include "DMRDefines.h"
#include "ModeConv.h"
#include "DMRNetwork.h"
#include "DMREmbeddedData.h"
#include "DMRLC.h"
#include "DMRFullLC.h"
#include "DMREMB.h"
#include "DMRLookup.h"
#include "UDPSocket.h"
#include "StopWatch.h"
#include "Version.h"
#include "YSFPayload.h"
#include "YSFNetwork.h"
#include "YSFFICH.h"
#include "Reflectors.h"
#include "Thread.h"
#include "Timer.h"
#include "Sync.h"
#include "Utils.h"
#include "Conf.h"
#include "DTMF.h"
#include "GPS.h"
#include "Log.h"
#include "WiresX.h"
#include "CRC.h"
#include "APRSReader.h"

#include <string>

enum TG_STATUS {
	NONE,
	WAITING_UNLINK,
	SEND_REPLY,
	SEND_PTT
};

class CYSF2DMR
{
public:
	CYSF2DMR(const std::string& configFile);
	~CYSF2DMR();

	int run();

private:
	std::string      m_callsign;
	std::string      m_suffix;
	CConf            m_conf;
	CWiresX*         m_wiresX;
	CDMRNetwork*     m_dmrNetwork;
	CYSFNetwork*     m_ysfNetwork;
	CDMRLookup*      m_lookup;
	CModeConv        m_conv;
	unsigned int     m_colorcode;
	unsigned int     m_srcHS;
	unsigned int     m_srcid;
	unsigned int     m_defsrcid;
	unsigned int     m_dstid;
	unsigned int     m_ptt_dstid;
	bool             m_ptt_pc;
	bool             m_dmrpc;
	std::string      m_netSrc;
	std::string      m_netDst;
	std::string      m_ysfSrc;
	unsigned char    m_dmrLastDT;
	unsigned char*   m_ysfFrame;
	unsigned char*   m_dmrFrame;
	CGPS*            m_gps;
	CDTMF*           m_dtmf;
	CAPRSReader*     m_APRS;
	unsigned int     m_dmrFrames;
	unsigned int     m_ysfFrames;
	CDMREmbeddedData m_EmbeddedLC;
	std::string      m_TGList;
	FLCO             m_dmrflco;
	bool             m_dmrinfo;
	unsigned int     m_idUnlink;
	FLCO             m_flcoUnlink;
	bool             m_enableWiresX;
	std::string      m_xlxmodule;
	bool             m_xlxConnected;
	CReflectors*     m_xlxReflectors;
	unsigned int     m_xlxrefl;
	bool             m_remoteGateway;
	unsigned int     m_hangTime;
	bool             m_firstSync;
	bool             m_dropUnknown;

	bool createDMRNetwork();
	void createGPS();
	void SendDummyDMR(unsigned int srcid, unsigned int dstid, FLCO dmr_flco);
	unsigned int findYSFID(std::string cs, bool showdst);
	std::string getSrcYSF(const unsigned char* source);
	void writeXLXLink(unsigned int srcId, unsigned int dstId, CDMRNetwork* network);
};

#endif
