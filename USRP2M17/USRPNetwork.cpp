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

#include "USRPNetwork.h"
#include "StopWatch.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>

CUSRPNetwork::CUSRPNetwork(const std::string& address, uint16_t dstPort, uint16_t localPort, bool debug) :
m_address(),
m_port(dstPort),
m_socket(localPort),
m_debug(debug)
{
	m_address = CUDPSocket::lookup(address);
	CStopWatch stopWatch;
}

CUSRPNetwork::~CUSRPNetwork()
{
}

bool CUSRPNetwork::open()
{
	LogMessage("USRP Network, Opening");
	return m_socket.open();
}

void CUSRPNetwork::close()
{
	m_socket.close();
}

uint32_t CUSRPNetwork::readData(uint8_t* data, uint32_t length)
{
	in_addr address;
	unsigned int port;
	int len = m_socket.read(data, length, address, port);
	if (len <= 0)
		return 0U;

	// Check if the data is for us
	if (m_address.s_addr != address.s_addr || port != m_port) {
		LogMessage("USRP packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return 0U;
	}

	if (m_debug)
		CUtils::dump(1U, "USRP Network Data Received", data, len);

	return len;
}

bool CUSRPNetwork::writeData(const uint8_t* data, uint32_t length)
{
	if (m_debug)
		CUtils::dump(1U, "USRP Network Data Sent", data, length);

	return m_socket.write(data, length, m_address, m_port);
}
