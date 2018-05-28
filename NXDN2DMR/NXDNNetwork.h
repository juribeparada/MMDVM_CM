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

#if !defined(NXDNNETWORK_H)
#define	NXDNNETWORK_H

#include "NXDNDefines.h"
#include "UDPSocket.h"

#include <cstdint>
#include <string>

class CNXDNNetwork {
public:
	CNXDNNetwork(const std::string& address, unsigned int port, const std::string& callsign, bool debug);
	~CNXDNNetwork();

	bool open();

	void setDestination(const in_addr& address, unsigned int port);
	void clearDestination();

	bool write(const unsigned char* data, unsigned int length);
	bool write(const unsigned char* data, unsigned short srcId, unsigned short dstId, bool grp);

	bool writePoll(unsigned short tg);
	bool writeUnlink(unsigned short tg);

	unsigned int read(unsigned char* data);

	void close();

private:
	CUDPSocket      m_socket;
	std::string     m_callsign;
	bool            m_debug;
	in_addr         m_address;
	unsigned int    m_port;
};

#endif
