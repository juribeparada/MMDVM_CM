/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
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

#include "Conf.h"
#include "Log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

const int BUFFER_SIZE = 500;

enum SECTION {
  SECTION_NONE,
  SECTION_INFO,
  SECTION_M17_NETWORK,
  SECTION_YSF_NETWORK,
  SECTION_LOG
};

CConf::CConf(const std::string& file) :
m_file(file),
m_daemon(false),
m_callsign(),
m_debug(false),
m_YSFDstAddress(),
m_YSFDstPort(0U),
m_YSFLocalAddress(),
m_YSFLocalPort(0U),
m_fcsFile(),
m_fichCallSign(2U),
m_fichCallMode(0U),
m_fichFrameTotal(6U),
m_fichMessageRoute(0U),
m_fichVOIP(0U),
m_fichDataType(2U),
m_fichSQLType(0U),
m_fichSQLCode(0U),
m_ysfDT1(),
m_ysfDT2(),
m_ysfRadioID("*****"),
m_m17CallsignSuffix('D'),
m_m17DstId(0U),
m_m17DstName(),
m_m17DstAddress(),
m_m17DstPort(0U),
m_m17LocalAddress(),
m_m17LocalPort(0U),
m_m17GainAdjDb(),
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
			if (::strncmp(buffer, "[Info]", 6U) == 0)
				section = SECTION_INFO;
			else if (::strncmp(buffer, "[M17 Network]", 13U) == 0)
				section = SECTION_M17_NETWORK;
			else if (::strncmp(buffer, "[YSF Network]", 13U) == 0)
				section = SECTION_YSF_NETWORK;
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
		char *t;
		if (len > 1U && *value == '"' && value[len - 1U] == '"') {
			value[len - 1U] = '\0';
			value++;
		}
		
		if (section == SECTION_INFO) {
			if (::strcmp(key, "Callsign") == 0) {
				// Convert the callsign to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_callsign = value;
			}
			else if (::strcmp(key, "Daemon") == 0)
				m_daemon = ::atoi(value) == 1;
			else if (::strcmp(key, "Debug") == 0)
				m_debug = ::atoi(value) == 1;
		} else if (section == SECTION_YSF_NETWORK) {
			if (::strcmp(key, "DstAddress") == 0)
				m_YSFDstAddress = value;
			else if (::strcmp(key, "DstPort") == 0)
				m_YSFDstPort = (unsigned int)::atoi(value);
			else if (::strcmp(key, "LocalAddress") == 0)
				m_YSFLocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_YSFLocalPort = (unsigned int)::atoi(value);
			else if (::strcmp(key, "FCSRooms") == 0)
				m_fcsFile = value;
			else if (::strcmp(key, "RadioID") == 0)
				m_ysfRadioID = value;
			else if (::strcmp(key, "FICHCallsign") == 0)
				m_fichCallSign = ::atoi(value);
			else if (::strcmp(key, "FICHCallMode") == 0)
				m_fichCallMode = ::atoi(value);
			else if (::strcmp(key, "FICHFrameTotal") == 0)
				m_fichFrameTotal = ::atoi(value);
			else if (::strcmp(key, "FICHMessageRoute") == 0)
				m_fichMessageRoute = ::atoi(value);
			else if (::strcmp(key, "FICHVOIP") == 0)
				m_fichVOIP = ::atoi(value);
			else if (::strcmp(key, "FICHDataType") == 0)
				m_fichDataType = ::atoi(value);
			else if (::strcmp(key, "FICHSQLType") == 0)
				m_fichSQLType = ::atoi(value);
			else if (::strcmp(key, "FICHSQLCode") == 0)
				m_fichSQLCode = ::atoi(value);
			else if (::strcmp(key, "DT1") == 0){
				while ((t = strtok_r(value, ",", &value)) != NULL)
					m_ysfDT1.push_back(::atoi(t));
			} else if (::strcmp(key, "DT2") == 0){
				while ((t = strtok_r(value, ",", &value)) != NULL)
					m_ysfDT2.push_back(::atoi(t));
			}
		} else if (section == SECTION_M17_NETWORK) {
			if (::strcmp(key, "CallsignSuffix") == 0)
				m_m17CallsignSuffix = (char)value[0];
			else if (::strcmp(key, "StartupDstId") == 0)
				m_m17DstId = (unsigned int)::atoi(value);
			else if (::strcmp(key, "LocalAddress") == 0)
				m_m17LocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_m17LocalPort = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DstName") == 0)
				m_m17DstName = value;
			else if (::strcmp(key, "DstAddress") == 0)
				m_m17DstAddress = value;
			else if (::strcmp(key, "DstPort") == 0)
				m_m17DstPort = (unsigned int)::atoi(value);
			else if (::strcmp(key, "GainAdjustdB") == 0)
				m_m17GainAdjDb = value;
		} else if (section == SECTION_LOG) {
			if (::strcmp(key, "FilePath") == 0)
				m_logFilePath = value;
			else if (::strcmp(key, "FileRoot") == 0)
				m_logFileRoot = value;
			else if (::strcmp(key, "FileLevel") == 0)
				m_logFileLevel = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DisplayLevel") == 0)
				m_logDisplayLevel = (unsigned int)::atoi(value);
		}
	}

	::fclose(fp);

	return true;
}

std::string CConf::getCallsign() const
{
	return m_callsign;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

bool CConf::getDebug() const
{
	return m_debug;
}

char CConf::getM17CallsignSuffix() const
{
	return m_m17CallsignSuffix;
}

std::string CConf::getM17DstName() const
{
	return m_m17DstName;
}

std::string CConf::getM17DstAddress() const
{
	return m_m17DstAddress;
}

unsigned int CConf::getM17DstPort() const
{
	return m_m17DstPort;
}

std::string CConf::getM17LocalAddress() const
{
	return m_m17LocalAddress;
}

unsigned int CConf::getM17LocalPort() const
{
	return m_m17LocalPort;
}

std::string CConf::getM17GainAdjDb() const
{
	return m_m17GainAdjDb;
}

std::string CConf::getYSFDstAddress() const
{
	return m_YSFDstAddress;
}

unsigned int CConf::getYSFDstPort() const
{
	return m_YSFDstPort;
}

std::string CConf::getYSFLocalAddress() const
{
	return m_YSFLocalAddress;
}

unsigned int CConf::getYSFLocalPort() const
{
	return m_YSFLocalPort;
}

std::string CConf::getFCSFile() const
{
	return m_fcsFile;
}

unsigned char CConf::getFICHCallSign() const
{
 	return m_fichCallSign;
}

unsigned char CConf::getFICHCallMode() const
{
 	return m_fichCallMode;
}

unsigned char CConf::getFICHFrameTotal() const
{
 	return m_fichFrameTotal;
}

unsigned char CConf::getFICHMessageRoute() const
{
 	return m_fichMessageRoute;
}

unsigned char CConf::getFICHVOIP() const
{
 	return m_fichVOIP;
}

unsigned char CConf::getFICHDataType() const
{
 	return m_fichDataType;
}

unsigned char CConf::getFICHSQLType() const
{
 	return m_fichSQLType;
}

unsigned char CConf::getFICHSQLCode() const
{
 	return m_fichSQLCode;
}

std::vector<unsigned char> CConf::getYsfDT1()
{
 	return m_ysfDT1;
}

std::vector<unsigned char> CConf::getYsfDT2()
{
 	return m_ysfDT2;
}

std::string CConf::getYsfRadioID()
{
 	return m_ysfRadioID;
}

unsigned int CConf::getLogDisplayLevel() const
{
	return m_logDisplayLevel;
}

unsigned int CConf::getLogFileLevel() const
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
