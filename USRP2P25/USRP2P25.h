/*
* 	Copyright (C) 2021 by Doug McLain AD8DP
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

#if !defined(USRP2P25_H)
#define USRP2P25_H

#include "ModeConv.h"
#include "USRPNetwork.h"
#include "P25Network.h"
#include "UDPSocket.h"
#include "StopWatch.h"
#include "Version.h"
#include "Timer.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"

#include <string>

class CUSRP2P25
{
public:
	CUSRP2P25(const std::string& configFile);
	~CUSRP2P25();

	int run();

private:
	std::string      m_callsign;
	CConf            m_conf;
	CUSRPNetwork*    m_usrpNetwork;
	CP25Network*     m_p25Network;
	CModeConv        m_conv;
	uint32_t         m_dmrid;
	uint32_t         m_p25Src;
	uint32_t         m_p25Dst;
	uint8_t*         m_p25Frame;
	uint32_t         m_p25Frames;
	bool             m_p25info;
	uint8_t*         m_usrpFrame;
	uint32_t         m_usrpFrames;
};

#endif
