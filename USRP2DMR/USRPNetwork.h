/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
 *   Copyright (C) 2021 by Doug McLain AD8DP
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

#if !defined(USRPNetwork_H)
#define	USRPNetwork_H

#include "UDPSocket.h"
#include "Timer.h"

#include <string>
#include <cstdint>

class CUSRPNetwork
{
public:
	CUSRPNetwork(const std::string& address, uint16_t dstPort, uint16_t localPort, bool debug);
	~CUSRPNetwork();

	uint32_t getConfig(uint8_t* config) const;
	bool open();
	bool writeData(const uint8_t* data, uint32_t length);
	uint32_t readData(uint8_t* data, uint32_t length);
	void close();
private: 
	in_addr		m_address;
	uint16_t	m_port;
	CUDPSocket	m_socket;
	bool		m_debug;
};

#endif
