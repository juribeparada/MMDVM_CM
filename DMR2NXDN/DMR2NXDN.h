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

#if !defined(DMR2NXDN_H)
#define DMR2NXDN_H

#include "DMRDefines.h"
#include "NXDNDefines.h"
#include "ModeConv.h"
#include "MMDVMNetwork.h"
#include "DMREmbeddedData.h"
#include "DMRLC.h"
#include "DMRFullLC.h"
#include "DMREMB.h"
#include "DMRLookup.h"
#include "NXDNConvolution.h"
#include "NXDNCRC.h"
#include "NXDNLayer3.h"
#include "NXDNLICH.h"
#include "NXDNLookup.h"
#include "NXDNSACCH.h"
#include "NXDNNetwork.h"
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

class CDMR2NXDN
{
public:
	CDMR2NXDN(const std::string& configFile);
	~CDMR2NXDN();

	int run();

private:
	unsigned int     m_nxdnTG;
	CConf            m_conf;
	CMMDVMNetwork*   m_dmrNetwork;
	CNXDNNetwork*    m_nxdnNetwork;
	CDMRLookup*      m_dmrlookup;
	CNXDNLookup*     m_nxdnlookup;
	CModeConv        m_conv;
	unsigned int     m_colorcode;
	unsigned int     m_defsrcid;
	unsigned int     m_dstid;
	unsigned int     m_dmrSrc;
	unsigned int     m_dmrDst;
	unsigned int     m_nxdnSrc;
	unsigned int     m_nxdnDst;
	unsigned char    m_dmrLastDT;
	unsigned char*   m_nxdnFrame;
	unsigned char*   m_dmrFrame;
	unsigned int     m_dmrFrames;
	unsigned int     m_nxdnFrames;
	CDMREmbeddedData m_EmbeddedLC;
	FLCO             m_dmrflco;
	bool             m_dmrinfo;
	bool             m_nxdninfo;
	unsigned char*   m_config;
	unsigned int     m_configLen;
	unsigned int     m_defaultID;

	unsigned int findNXDNID(unsigned int dmrid);
	unsigned int findDMRID(unsigned int nxdnid);
	unsigned int truncID(unsigned int id);
	bool createMMDVM();

};

#endif
