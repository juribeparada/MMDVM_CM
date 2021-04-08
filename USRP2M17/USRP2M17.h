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

#if !defined(USRP2M17_H)
#define USRP2M17_H

#include "ModeConv.h"
#include "USRPNetwork.h"
#include "M17Network.h"
#include "UDPSocket.h"
#include "StopWatch.h"
#include "Version.h"
#include "Timer.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"

#include <string>

class CUSRP2M17
{
public:
	CUSRP2M17(const std::string& configFile);
	~CUSRP2M17();

	int run();

private:
	std::string      m_callsign;
	std::string		 m_m17Ref;
	std::string      m_usrpcs;
	CConf            m_conf;
	CUSRPNetwork*    m_usrpNetwork;
	CM17Network*     m_m17Network;
	CModeConv        m_conv;
	std::string    	 m_m17Src;
	std::string      m_m17Dst;
	uint8_t*         m_m17Frame;
	uint32_t         m_m17Frames;
	uint8_t*         m_usrpFrame;
	uint32_t         m_usrpFrames;
};

#endif
