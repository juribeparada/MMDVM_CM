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

#if !defined(USRP2YSF_H)
#define USRP2YSF_H

#include "ModeConv.h"
#include "USRPNetwork.h"
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

class CUSRP2YSF
{
public:
	CUSRP2YSF(const std::string& configFile);
	~CUSRP2YSF();

	int run();

private:
	std::string      m_callsign;
	std::string      m_usrpcs;
	CConf            m_conf;
	CYSFNetwork*     m_ysfNetwork;
	CUSRPNetwork*    m_usrpNetwork;
	CModeConv        m_conv;
	uint8_t*         m_usrpFrame;
	uint32_t         m_usrpFrames;
	unsigned char*   m_ysfFrame;
};

#endif
