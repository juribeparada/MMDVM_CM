/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by Andy Uribe CA6JAU
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

#include "Conf.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

const int BUFFER_SIZE = 500;

enum SECTION {
  SECTION_NONE,
  SECTION_P25_NETWORK,
  SECTION_USRP_NETWORK,
  SECTION_LOG
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_dmrid(0U),
m_daemon(false),
m_usrpAddress(),
m_usrpDstPort(0U),
m_usrpLocalPort(0U),
m_usrpGainAdjDb(),
m_usrpDebug(false),
m_p25DstAddress(),
m_p25DstPort(0U),
m_p25LocalAddress(),
m_p25LocalPort(0U),
m_p25NetworkDebug(false),
m_logDisplayLevel(0U),
m_logFileLevel(0U),
m_logFilePath(),
m_logFileRoot()
{
}

CConf::~CConf()
{
}

bool CConf::read()
{
  FILE* fp = ::fopen(m_file.c_str(), "rt");
  if (fp == NULL) {
    ::fprintf(stderr, "Couldn't open the .ini file - %s\n", m_file.c_str());
    return false;
  }

  SECTION section = SECTION_NONE;

  char buffer[BUFFER_SIZE];
  while (::fgets(buffer, BUFFER_SIZE, fp) != NULL) {
    if (buffer[0U] == '#')
      continue;

    if (buffer[0U] == '[') {
	  if (::strncmp(buffer, "[P25 Network]", 13U) == 0)
        section = SECTION_P25_NETWORK;
	  else if (::strncmp(buffer, "[USRP Network]", 14U) == 0)
		  section = SECTION_USRP_NETWORK;
	  else if (::strncmp(buffer, "[Log]", 5U) == 0)
		  section = SECTION_LOG;
	  else
        section = SECTION_NONE;

      continue;
    }

    char* key   = ::strtok(buffer, " \t=\r\n");
    if (key == NULL)
      continue;

    char* value = ::strtok(NULL, "\r\n");
    if (value == NULL)
      continue;

    // Remove quotes from the value
    size_t len = ::strlen(value);
    if (len > 1U && *value == '"' && value[len - 1U] == '"') {
      value[len - 1U] = '\0';
      value++;
    }
	::fprintf(stderr, "CConf key:val:section == %s:%s:%d\n", key, value, section);
	if (section == SECTION_P25_NETWORK) {
		if (::strcmp(key, "Callsign") == 0)
			m_callsign = value;
		else if (::strcmp(key, "DmrId") == 0)
			m_dmrid = (unsigned int)::atoi(value);
		else if (::strcmp(key, "LocalAddress") == 0)
			m_p25LocalAddress = value;
		else if (::strcmp(key, "LocalPort") == 0)
			m_p25LocalPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "DstAddress") == 0)
			m_p25DstAddress = value;
		else if (::strcmp(key, "DstPort") == 0)
			m_p25DstPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "GainAdjustdB") == 0)
			m_p25GainAdjDb = value;
		else if (::strcmp(key, "Debug") == 0)
			m_p25NetworkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_USRP_NETWORK) {
		if (::strcmp(key, "Address") == 0)
			m_usrpAddress = value;
		else if (::strcmp(key, "DstPort") == 0)
			m_usrpDstPort = (uint32_t)::atoi(value);
		else if (::strcmp(key, "LocalPort") == 0)
			m_usrpLocalPort = (uint32_t)::atoi(value);
		else if (::strcmp(key, "GainAdjustdB") == 0)
			m_usrpGainAdjDb = value;
		else if (::strcmp(key, "Debug") == 0)
			m_usrpDebug = ::atoi(value) == 1;
	} else if (section == SECTION_LOG) {
		if (::strcmp(key, "FilePath") == 0)
			m_logFilePath = value;
		else if (::strcmp(key, "FileRoot") == 0)
			m_logFileRoot = value;
		else if (::strcmp(key, "FileLevel") == 0)
			m_logFileLevel = (uint32_t)::atoi(value);
		else if (::strcmp(key, "DisplayLevel") == 0)
			m_logDisplayLevel = (uint32_t)::atoi(value);
	}
  }

  ::fclose(fp);

  return true;
}

std::string CConf::getCallsign() const
{
  return m_callsign;
}

uint32_t CConf::getDMRId() const
{
	return m_dmrid;
}

std::string CConf::getP25DstAddress() const
{
	return m_p25DstAddress;
}

uint32_t CConf::getP25DstPort() const
{
	return m_p25DstPort;
}

std::string CConf::getP25LocalAddress() const
{
	return m_p25LocalAddress;
}

uint32_t CConf::getP25LocalPort() const
{
	return m_p25LocalPort;
}

std::string CConf::getP25GainAdjDb() const
{
	return m_p25GainAdjDb;
}

bool CConf::getP25NetworkDebug() const
{
	return m_p25NetworkDebug;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

std::string CConf::getUSRPAddress() const
{
	return m_usrpAddress;
}

uint16_t CConf::getUSRPDstPort() const
{
	return m_usrpDstPort;
}

uint16_t CConf::getUSRPLocalPort() const
{
	return m_usrpLocalPort;
}

std::string CConf::getUSRPGainAdjDb() const
{
	return m_usrpGainAdjDb;
}

bool CConf::getUSRPDebug() const
{
	return m_usrpDebug;
}

uint32_t CConf::getLogDisplayLevel() const
{
	return m_logDisplayLevel;
}

uint32_t CConf::getLogFileLevel() const
{
	return m_logFileLevel;
}

std::string CConf::getLogFilePath() const
{
  return m_logFilePath;
}

std::string CConf::getLogFileRoot() const
{
  return m_logFileRoot;
}
