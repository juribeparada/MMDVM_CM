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
  SECTION_YSF_NETWORK,
  SECTION_P25_NETWORK,
  SECTION_DMRID_LOOKUP,
  SECTION_LOG
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_suffix(),
m_dstAddress(),
m_dstPort(0U),
m_localAddress(),
m_localPort(0U),
m_enableWiresX(false),
m_wiresXMakeUpper(true),
m_fichCallSign(2U),
m_fichCallMode(0U),
m_fichFrameTotal(6U),
m_fichMessageRoute(0U),
m_fichVOIP(0U),
m_fichDataType(3U),
m_fichSQLType(0U),
m_fichSQLCode(0U),
m_ysfDT1(),
m_ysfDT2(),
m_ysfRadioID("*****"),
m_daemon(false),
m_networkDebug(false),
m_rxFrequency(0U),
m_txFrequency(0U),
m_description(),
m_p25Id(0U),
m_p25DstId(0U),
m_p25DstAddress(),
m_p25DstPort(0U),
m_p25LocalAddress(),
m_p25LocalPort(0U),
m_p25TGListFile(),
m_p25NetworkDebug(false),
m_dmrIdLookupFile(),
m_dmrIdLookupTime(0U),
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
	  else if (::strncmp(buffer, "[YSF Network]", 13U) == 0)
		section = SECTION_YSF_NETWORK;
	  else if (::strncmp(buffer, "[P25 Network]", 13U) == 0)
		section = SECTION_P25_NETWORK;
	  else if (::strncmp(buffer, "[DMR Id Lookup]", 15U) == 0)
		section = SECTION_DMRID_LOOKUP;
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
		if (::strcmp(key, "TXFrequency") == 0)
			m_txFrequency = (unsigned int)::atoi(value);
		else if (::strcmp(key, "RXFrequency") == 0)
			m_rxFrequency = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Description") == 0)
			m_description = value;
	} else if (section == SECTION_YSF_NETWORK) {
		if (::strcmp(key, "Callsign") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_callsign = value;
		} else if (::strcmp(key, "Suffix") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_suffix = value;
		} else if (::strcmp(key, "DstAddress") == 0)
			m_dstAddress = value;
		else if (::strcmp(key, "DstPort") == 0)
			m_dstPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "LocalAddress") == 0)
			m_localAddress = value;
		else if (::strcmp(key, "LocalPort") == 0)
			m_localPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "EnableWiresX") == 0)
			m_enableWiresX = ::atoi(value) == 1;
		else if (::strcmp(key, "WiresXMakeUpper") == 0)
			m_wiresXMakeUpper = ::atoi(value) == 1;
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
		else if (::strcmp(key, "Daemon") == 0)
			m_daemon = ::atoi(value) == 1;
		else if (::strcmp(key, "Debug") == 0)
			m_networkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_P25_NETWORK) {
		if (::strcmp(key, "Id") == 0)
			m_p25Id = (unsigned int)::atoi(value);
		else if (::strcmp(key, "StartupDstId") == 0)
			m_p25DstId = (unsigned int)::atoi(value);
		else if (::strcmp(key, "DstAddress") == 0)
			m_p25DstAddress = value;
		else if (::strcmp(key, "DstPort") == 0)
			m_p25DstPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "LocalAddress") == 0)
			m_p25LocalAddress = value;
		else if (::strcmp(key, "LocalPort") == 0)
			m_p25LocalPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "TGListFile") == 0)
			m_p25TGListFile = value;
		else if (::strcmp(key, "Debug") == 0)
			m_p25NetworkDebug = ::atoi(value) == 1;
	} else if (section == SECTION_DMRID_LOOKUP) {
		if (::strcmp(key, "File") == 0)
			m_dmrIdLookupFile = value;
		else if (::strcmp(key, "Time") == 0)
			m_dmrIdLookupTime = (unsigned int)::atoi(value);
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

unsigned int CConf::getRxFrequency() const
{
	return m_rxFrequency;
}

unsigned int CConf::getTxFrequency() const
{
	return m_txFrequency;
}

std::string CConf::getDescription() const
{
	return m_description;
}

std::string CConf::getCallsign() const
{
  return m_callsign;
}

std::string CConf::getSuffix() const
{
	return m_suffix;
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

bool CConf::getEnableWiresX() const
{
	return m_enableWiresX;
}

bool CConf::getWiresXMakeUpper() const
{
	return m_wiresXMakeUpper;
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

bool CConf::getDaemon() const
{
	return m_daemon;
}

bool CConf::getNetworkDebug() const
{
	return m_networkDebug;
}

unsigned int CConf::getP25Id() const
{
	return m_p25Id;
}

unsigned int CConf::getP25DstId() const
{
	return m_p25DstId;
}

std::string CConf::getP25DstAddress() const
{
	return m_p25DstAddress;
}

unsigned int CConf::getP25DstPort() const
{
	return m_p25DstPort;
}

std::string CConf::getP25LocalAddress() const
{
	return m_p25LocalAddress;
}

unsigned int CConf::getP25LocalPort() const
{
	return m_p25LocalPort;
}

std::string CConf::getP25TGListFile() const
{
	return m_p25TGListFile;
}

bool CConf::getP25NetworkDebug() const
{
	return m_p25NetworkDebug;
}

std::string CConf::getDMRIdLookupFile() const
{
	return m_dmrIdLookupFile;
}

unsigned int CConf::getDMRIdLookupTime() const
{
	return m_dmrIdLookupTime;
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
