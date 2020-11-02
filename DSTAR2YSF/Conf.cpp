/*
 *   Copyright (C) 2015-2019 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018,2019 by Andy Uribe CA6JAU
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
  SECTION_DSTAR_NETWORK,
  SECTION_YSF_NETWORK,
  SECTION_LOG
};

CConf::CConf(const std::string& file) :
m_file(file),
m_mycall(),
m_urcall(),
m_rptr1(),
m_rptr2(),
m_suffix(),
m_userTxt(),
m_dstarDstAddress(),
m_dstarDstPort(0U),
m_dstarLocalAddress(),
m_dstarLocalPort(0U),
m_daemon(false),
m_dstarNetworkDebug(false),
m_callsign(),
m_dstAddress(),
m_dstPort(0U),
m_localAddress(),
m_localPort(0U),
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
m_ysfDebug(false),
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
	  else if (::strncmp(buffer, "[DSTAR Network]", 13U) == 0)
		section = SECTION_DSTAR_NETWORK;
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
		if (::strcmp(key, "Mycall") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_mycall = value;
		}
		else if (::strcmp(key, "Urcall") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_urcall = value;
		}
		else if (::strcmp(key, "Rptr1") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_rptr1 = value;
		}
		else if (::strcmp(key, "Rptr2") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_rptr2 = value;
		}
		else if (::strcmp(key, "Suffix") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_suffix = value;
		}
		else if (::strcmp(key, "UserText") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_userTxt = value;
		}
		else if(::strcmp(key, "VocoderDevice") == 0) {
			m_vocoderDevice = value;
		}
	} else if (section == SECTION_DSTAR_NETWORK) {
			if (::strcmp(key, "DstAddress") == 0)
				m_dstarDstAddress = value;
			else if (::strcmp(key, "DstPort") == 0)
				m_dstarDstPort = (unsigned int)::atoi(value);
			else if (::strcmp(key, "LocalAddress") == 0)
				m_dstarLocalAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_dstarLocalPort = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Daemon") == 0)
				m_daemon = ::atoi(value) == 1;
			else if (::strcmp(key, "Debug") == 0)
				m_dstarNetworkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_YSF_NETWORK) {
		if (::strcmp(key, "Callsign") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_callsign = value;
		} else if (::strcmp(key, "GatewayAddress") == 0)
			m_dstAddress = value;
		else if (::strcmp(key, "GatewayPort") == 0)
			m_dstPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "LocalAddress") == 0)
			m_localAddress = value;
		else if (::strcmp(key, "LocalPort") == 0)
			m_localPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "FCSRooms") == 0)
			m_fcsFile = value;
		else if (::strcmp(key, "RadioID") == 0)
 			m_ysfRadioID = value;
 		else if (::strcmp(key, "Debug") == 0)
				m_ysfDebug = ::atoi(value) == 1;
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

std::string CConf::getMycall() const
{
  return m_mycall;
}

std::string CConf::getUrcall() const
{
  return m_urcall;
}

std::string CConf::getRptr1() const
{
  return m_rptr1;
}

std::string CConf::getRptr2() const
{
  return m_rptr2;
}

std::string CConf::getSuffix() const
{
  return m_suffix;
}

std::string CConf::getUserTxt() const
{
  return m_userTxt;
}

std::string CConf::getVocoderDevice() const
{
  return m_vocoderDevice;
}

std::string CConf::getDSTARDstAddress() const
{
	return m_dstarDstAddress;
}

unsigned int CConf::getDSTARDstPort() const
{
	return m_dstarDstPort;
}

std::string CConf::getDSTARLocalAddress() const
{
	return m_dstarLocalAddress;
}

unsigned int CConf::getDSTARLocalPort() const
{
	return m_dstarLocalPort;
}

bool CConf::getDaemon() const
{
	return m_daemon;
}

bool CConf::getDSTARNetworkDebug() const
{
	return m_dstarNetworkDebug;
}

std::string CConf::getCallsign() const
{
  return m_callsign;
}

std::string CConf::getDstAddress() const
{
	return m_dstAddress;
}

unsigned int CConf::getDstPort() const
{
	return m_dstPort;
}

std::string CConf::getLocalAddress() const
{
	return m_localAddress;
}

unsigned int CConf::getLocalPort() const
{
	return m_localPort;
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

bool CConf::getYSFDebug() const
{
	return m_ysfDebug;
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
