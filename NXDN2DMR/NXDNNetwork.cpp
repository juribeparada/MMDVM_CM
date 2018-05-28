/*
 *   Copyright (C) 2009-2014,2016,2017,2018 by Jonathan Naylor G4KLX
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

#include "NXDNNetwork.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>

const unsigned int BUFFER_LENGTH = 200U;

CNXDNNetwork::CNXDNNetwork(const std::string& address, unsigned int port, const std::string& callsign, bool debug) :
m_socket(address, port),
m_callsign(callsign),
m_debug(debug),
m_address(),
m_port(0U)
{
	m_callsign.resize(10U, ' ');
}

CNXDNNetwork::~CNXDNNetwork()
{
}

bool CNXDNNetwork::open()
{
	LogMessage("Opening NXDN network connection");

	return m_socket.open();
}

void CNXDNNetwork::setDestination(const in_addr& address, unsigned int port)
{
	m_address = address;
	m_port    = port;
}

void CNXDNNetwork::clearDestination()
{
	m_address.s_addr = INADDR_NONE;
	m_port           = 0U;
}

bool CNXDNNetwork::write(const unsigned char* data, unsigned int length)
{
	assert(data != NULL);
	assert(length > 0U);

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Data Sent", data, length);

	return m_socket.write(data, length, m_address, m_port);
}

bool CNXDNNetwork::write(const unsigned char* data, unsigned short srcId, unsigned short dstId, bool grp)
{
	assert(data != NULL);

	unsigned char buffer[50U];

	buffer[0U] = 'N';
	buffer[1U] = 'X';
	buffer[2U] = 'D';
	buffer[3U] = 'N';
	buffer[4U] = 'D';

	buffer[5U] = (srcId >> 8) & 0xFFU;
	buffer[6U] = (srcId >> 0) & 0xFFU;

	buffer[7U] = (dstId >> 8) & 0xFFU;
	buffer[8U] = (dstId >> 0) & 0xFFU;

	buffer[9U] = 0x00U;
	buffer[9U] |= grp ? 0x01U : 0x00U;

	if (data[0U] == 0x81U || data[0U] == 0x83U) {
		buffer[9U] |= data[5U] == 0x01U ? 0x04U : 0x00U;
		buffer[9U] |= data[5U] == 0x08U ? 0x08U : 0x00U;
	}

	::memcpy(buffer + 10U, data, 33U);

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Data Sent", buffer, 43U);

	return m_socket.write(buffer, 43U, m_address, m_port);
}

unsigned int CNXDNNetwork::read(unsigned char* data)
{
	assert(data != NULL);

	in_addr address;
	unsigned int port;

	int len = m_socket.read(data, BUFFER_LENGTH, address, port);
	if (len <= 0)
		return 0U;

	// Invalid packet type?
	if (::memcmp(data, "NXDN", 4U) != 0)
		return 0U;

	if (len != 17 && len != 43)
		return 0U;

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Data Received", data, len);

	return len;
}

bool CNXDNNetwork::writePoll(unsigned short tg)
{
	unsigned char data[20U];

	data[0U] = 'N';
	data[1U] = 'X';
	data[2U] = 'D';
	data[3U] = 'N';
	data[4U] = 'P';

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 5U] = m_callsign.at(i);

	data[15U] = (tg >> 8) & 0xFFU;
	data[16U] = (tg >> 0) & 0xFFU;

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Poll Sent", data, 17U);

	return m_socket.write(data, 17U, m_address, m_port);
}

bool CNXDNNetwork::writeUnlink(unsigned short tg)
{
	unsigned char data[20U];

	data[0U] = 'N';
	data[1U] = 'X';
	data[2U] = 'D';
	data[3U] = 'N';
	data[4U] = 'U';

	for (unsigned int i = 0U; i < 10U; i++)
		data[i + 5U] = m_callsign.at(i);

	data[15U] = (tg >> 8) & 0xFFU;
	data[16U] = (tg >> 0) & 0xFFU;

	if (m_debug)
		CUtils::dump(1U, "NXDN Network Unlink Sent", data, 17U);

	return m_socket.write(data, 17U, m_address, m_port);
}

void CNXDNNetwork::close()
{
	m_socket.close();

	LogMessage("Closing NXDN network connection");
}
