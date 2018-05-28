/*
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Manuel Sanchez EA7EE
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

#ifndef	APRSReader_H
#define	APRSReader_H

#include "TCPSocket.h"
#include "Thread.h"
#include "Mutex.h"

#include <string>
#include <unordered_map>

class CAPRSReader : public CThread  {
public:
	CAPRSReader(std::string ApiKey, int refres_time);
	virtual ~CAPRSReader();

	virtual void entry();
	
	bool findCall(std::string cs, int *latitude, int *longitude);
    void formatGPS(unsigned char *buffer, int latitude, int longitude);
	void stop();
	bool load_call();

private:
	std::string m_ApiKey;
	std::string m_cs;
	bool m_stop;
	bool m_new_callsign;
	unsigned int  m_refres_time;
	std::unordered_map<std::string, int> m_lat_table;
	std::unordered_map<std::string, int> m_lon_table;
	std::unordered_map<std::string, unsigned int> m_time_table;
};

#endif
