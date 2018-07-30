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

#if !defined(YSF2NXDN_H)
#define YSF2NXDN_H

#include "NXDNDefines.h"
#include "ModeConv.h"
#include "NXDNLookup.h"
#include "UDPSocket.h"
#include "StopWatch.h"
#include "Version.h"
#include "YSFPayload.h"
#include "YSFNetwork.h"
#include "NXDNConvolution.h"
#include "NXDNCRC.h"
#include "NXDNLayer3.h"
#include "NXDNLICH.h"
#include "NXDNSACCH.h"
#include "NXDNNetwork.h"
#include "YSFFICH.h"
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

class CYSF2NXDN
{
public:
	CYSF2NXDN(const std::string& configFile);
	~CYSF2NXDN();

	int run();

private:
	std::string      m_callsign;
	std::string      m_suffix;
	CConf            m_conf;
	CWiresX*         m_wiresX;
	CNXDNNetwork*    m_nxdnNetwork;
	CYSFNetwork*     m_ysfNetwork;
	CNXDNLookup*     m_lookup;
	CModeConv        m_conv;
	unsigned int     m_srcid;
	unsigned int     m_defsrcid;
	unsigned int     m_dstid;
	std::string      m_netSrc;
	std::string      m_netDst;
	std::string      m_ysfSrc;
	unsigned char*   m_ysfFrame;
	unsigned char*   m_nxdnFrame;
	CGPS*            m_gps;
	CDTMF*           m_dtmf;
	CAPRSReader*     m_APRS;
	unsigned int     m_nxdnFrames;
	unsigned int     m_ysfFrames;
	bool             m_nxdninfo;

	void sendNXDNPTT(unsigned int src, unsigned int dst);
	void createGPS();
	unsigned int findYSFID(std::string cs, bool showdst);
	std::string getSrcYSF(const unsigned char* source);
};

#endif
