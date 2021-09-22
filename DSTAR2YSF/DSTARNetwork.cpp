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

#include "DSTARNetwork.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CDSTARNetwork::CDSTARNetwork(const std::string& localAddress, unsigned int localPort, const std::string& gatewayAddress, unsigned int gatewayPort, const std::string& callsign, bool debug) :
m_callsign(callsign),
m_address(),
m_port(gatewayPort),
m_socket(localAddress, localPort),
m_debug(debug)
{
	m_callsign.resize(10U, ' ');
	m_address = CUDPSocket::lookup(gatewayAddress);
}

CDSTARNetwork::~CDSTARNetwork()
{
}

bool CDSTARNetwork::open()
{
	LogInfo("Opening DSTAR network connection");

	return m_socket.open();
}

bool CDSTARNetwork::writeHeader(const unsigned char* header, unsigned int length)
{
	assert(header != NULL);

	unsigned char buffer[50U];

	buffer[0] = 'D';
	buffer[1] = 'S';
	buffer[2] = 'R';
	buffer[3] = 'P';
	buffer[4] = 0x20U;

	// Create a random id for this transmission
	//std::uniform_int_distribution<uint16_t> dist(0x0001, 0xfffe);
	//m_outId = 0x1234;//dist(m_random);
	m_outId = rand() % 65536; 

	buffer[5] = m_outId / 256U;	// Unique session id
	buffer[6] = m_outId % 256U;

	buffer[7] = 0U;

	::memcpy(buffer + 8U, header, length);

	m_outSeq = 0U;

	

	for (unsigned int i = 0U; i < 2U; i++) {
		bool ret = m_socket.write(buffer, 49U, m_address, m_port);
		if (m_debug)
			CUtils::dump(1U, "D-Star Network Header Sent", buffer, 49U);
		
		if (!ret)
			return false;
	}

	return true;
}

bool CDSTARNetwork::writeData(const unsigned char* data, unsigned int length, bool end)
{
	assert(data != NULL);

	unsigned char buffer[30U];

	buffer[0] = 'D';
	buffer[1] = 'S';
	buffer[2] = 'R';
	buffer[3] = 'P';
	buffer[4] = 0x21U;
	buffer[5] = m_outId / 256U;	// Unique session id
	buffer[6] = m_outId % 256U;

	// If this is a data sync, reset the sequence to zero
	if (data[9] == 0x55 && data[10] == 0x2D && data[11] == 0x16)
		m_outSeq = 0U;

	buffer[7] = m_outSeq;
	if (end)
		buffer[7] |= 0x40U;			// End of data marker

	buffer[8] = 0;

	m_outSeq++;
	if (m_outSeq > 0x14U)
		m_outSeq = 0U;

	::memcpy(buffer + 9U, data, length);

	if (m_debug)
		CUtils::dump(1U, "D-Star Network Data Sent", buffer, length + 9U);

	return m_socket.write(buffer, length + 9U, m_address, m_port);
}

bool CDSTARNetwork::writePoll()
{
	unsigned char data[15U];

	data[0U] = 0xF0U;

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 1U] = m_callsign.at(i);

	if (m_debug)
		CUtils::dump(1U, "DSTAR Network Poll Sent", data, 11U);

	return m_socket.write(data, 11U, m_address, m_port);
}

unsigned int CDSTARNetwork::readData(unsigned char* data, unsigned int length)
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
		LogMessage("DSTAR packet received from an invalid source, %08X != %08X and/or %u != %u", m_address.s_addr, address.s_addr, m_port, port);
		return 0U;
	}

	if (m_debug)
		CUtils::dump(1U, "DSTAR Network Data Received", data, len);

	return len;
}

void CDSTARNetwork::close()
{
	m_socket.close();

	LogInfo("Closing DSTAR network connection");
}
