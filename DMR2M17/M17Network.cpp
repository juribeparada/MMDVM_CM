/*
 *   Copyright (C) 2009-2014,2016 by Jonathan Naylor G4KLX
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

#include "M17Network.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>


CM17Network::CM17Network(const std::string& localAddress, unsigned int localPort, const std::string& gatewayAddress, unsigned int gatewayPort, unsigned char* callsign, bool debug) :
m_address(),
m_port(gatewayPort),
//m_socket(localAddress, localPort),
m_socket(localPort),
m_debug(debug)
{
	memcpy(m_callsign, callsign, 6);
	m_address = CUDPSocket::lookup(gatewayAddress);
}

CM17Network::~CM17Network()
{
}

bool CM17Network::open()
{
	LogInfo("Opening M17 network connection");

	return m_socket.open();
}

bool CM17Network::writeData(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (m_debug)
		CUtils::dump(1U, "M17 Network Data Sent", data, length);

	return m_socket.write(data, length, m_address, m_port);
}

bool CM17Network::writePoll()
{
	unsigned char data[10U];
	
	memcpy(data, "PONG", 4);
	memcpy(data+4, m_callsign, 6);

	if (m_debug)
		CUtils::dump(1U, "M17 Network Pong Sent", data, 10U);

	return m_socket.write(data, 10U, m_address, m_port);
}

bool CM17Network::writeLink(char m)
{
	unsigned char data[11U];
	
	memcpy(data, "CONN", 4);
	memcpy(data+4, m_callsign, 6);
	data[10U] = m;
	if (m_debug)
		CUtils::dump(1U, "M17 Network Link Sent", data, 11U);

	//LogInfo("writeLink add:port == %x, %x", m_address.s_addr, m_port);
	return m_socket.write(data, 11U, m_address, m_port);
}

bool CM17Network::writeUnlink()
{
	unsigned char data[10U];
	
	memcpy(data, "DISC", 4);
	memcpy(data+4, m_callsign, 6);

	if (m_debug)
		CUtils::dump(1U, "M17 Network Unlink Sent", data, 10U);

	return m_socket.write(data, 10U, m_address, m_port);
}

unsigned int CM17Network::readData(unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	in_addr address;
	unsigned int port;
	int len = m_socket.read(data, length, address, port);
	if (len <= 0)
		return 0U;

	// Check if the data is for us
	if (m_address.s_addr != address.s_addr || port != m_port) {
		LogMessage("M17 packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return 0U;
	}

	if (m_debug)
		CUtils::dump(1U, "M17 Network Data Received", data, len);

	return len;
}

void CM17Network::close()
{
	m_socket.close();

	LogInfo("Closing P25 network connection");
}
