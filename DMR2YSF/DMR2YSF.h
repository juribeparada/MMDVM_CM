/*
*   Copyright (C) 2016 by Jonathan Naylor G4KLX
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

#if !defined(DMR2YSF_H)
#define DMR2YSF_H

#include "DMRDefines.h"
#include "ModeConv.h"
#include "MMDVMNetwork.h"
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
#include "Thread.h"
#include "Timer.h"
#include "Sync.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"
#include "CRC.h"

#include <string>

class CTGReg {
public:
	CTGReg() :
	m_tg(),
	m_ysf()
	{
	}

	unsigned int m_tg;
	unsigned int m_ysf;
};

class CFCSReg {
public:
	CFCSReg() :
	m_id(),
	m_fcs()
	{
	}

	unsigned int m_id;
	std::string  m_fcs;
};

class CDMR2YSF
{
public:
	CDMR2YSF(const std::string& configFile);
	~CDMR2YSF();

	int run();

private:
	std::string            m_callsign;
	CConf                  m_conf;
	CMMDVMNetwork*         m_dmrNetwork;
	CYSFNetwork*           m_ysfNetwork;
	CDMRLookup*            m_lookup;
	CModeConv              m_conv;
	unsigned int           m_colorcode;
	unsigned int           m_srcid;
	unsigned int           m_defsrcid;
	unsigned int           m_dstid;
	bool                   m_dmrpc;
	std::string            m_netSrc;
	std::string            m_netDst;
	std::string            m_ysfSrc;
	unsigned char          m_dmrLastDT;
	unsigned char*         m_ysfFrame;
	unsigned char*         m_dmrFrame;
	unsigned int           m_dmrFrames;
	unsigned int           m_ysfFrames;
	CDMREmbeddedData       m_EmbeddedLC;
	FLCO                   m_dmrflco;
	bool                   m_dmrinfo;
	unsigned char*         m_config;
	unsigned int           m_configLen;
	unsigned char*         m_command;
	unsigned int           m_tgUnlink;
	std::vector<CTGReg*>   m_currTGList;
	std::vector<CFCSReg*>  m_FCSList;
	unsigned int           m_lastTG;

	void readTGList(std::string filename);
	void readFCSRoomsFile(const std::string& filename);
	unsigned int findYSFID(std::string cs, bool showdst);
	std::string getSrcYSF(const unsigned char* source);
	void connectYSF(unsigned int id);
	void sendYSFConn(unsigned int id);
	void sendYSFDisc();
	void processWiresX(const unsigned char* data, unsigned char fi, unsigned char dt, unsigned char fn, unsigned char ft);
	bool createMMDVM();

};

#endif
