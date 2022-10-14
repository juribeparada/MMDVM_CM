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

#if !defined(M172YSF_H)
#define M172YSF_H

#include "ModeConv.h"
#include "M17Network.h"
#include "YSFPayload.h"
#include "YSFNetwork.h"
#include "YSFFICH.h"
#include "UDPSocket.h"
#include "StopWatch.h"
#include "Version.h"
#include "Timer.h"
#include "Utils.h"
#include "Conf.h"
#include "Log.h"
#include "CRC.h"

#include <string>

class CM172YSF
{
public:
	CM172YSF(const std::string& configFile);
	~CM172YSF();

	int run();

private:
	std::string      m_callsign;
	std::string      m_ysfcs;
	std::string 	 m_m17cs;
	std::string 	 m_m17Ref;
	CConf            m_conf;
	CYSFNetwork*     m_ysfNetwork;
	CM17Network*	 m_m17Network;
	CModeConv        m_conv;
	std::string      m_m17Src;
	std::string	     m_m17Dst;
	unsigned char*   m_m17Frame;
	unsigned int     m_m17Frames;
	unsigned char*   m_ysfFrame;
};

#endif
