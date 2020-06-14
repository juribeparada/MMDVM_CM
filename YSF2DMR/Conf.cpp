/*
 *   Copyright (C) 2015-2019 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018,2019 by Andy Uribe CA6JAU
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
  SECTION_DMR_NETWORK,
  SECTION_DMRID_LOOKUP,
  SECTION_LOG,
  SECTION_APRS_FI
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
m_remoteGateway(false),
m_hangTime(1000U),
m_wiresXMakeUpper(true),
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
m_daemon(false),
m_rxFrequency(0U),
m_txFrequency(0U),
m_power(0U),
m_latitude(0.0F),
m_longitude(0.0F),
m_height(0),
m_location(),
m_description(),
m_url(),
m_dmrId(0U),
m_dmrXLXFile(),
m_dmrXLXModule(),
m_dmrXLXReflector(950U),
m_dmrDstId(9990U),
m_dmrPC(true),
m_dmrNetworkAddress(),
m_dmrNetworkPort(0U),
m_dmrNetworkLocal(0U),
m_dmrNetworkPassword(),
m_dmrNetworkOptions(),
m_dmrNetworkDebug(false),
m_dmrNetworkJitterEnabled(true),
m_dmrNetworkJitter(500U),
m_dmrNetworkEnableUnlink(true),
m_dmrNetworkIDUnlink(4000U),
m_dmrNetworkPCUnlink(false),
m_dmrIdLookupFile(),
m_dmrIdLookupTime(0U),
m_logDisplayLevel(0U),
m_logFileLevel(0U),
m_logFilePath(),
m_logFileRoot(),
m_aprsEnabled(false),
m_aprsServer(),
m_aprsPort(0U),
m_aprsPassword(),
m_aprsCallsign(),
m_aprsAPIKey(),
m_aprsRefresh(120),
m_aprsDescription()
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
	  else if (::strncmp(buffer, "[DMR Network]", 13U) == 0)
		  section = SECTION_DMR_NETWORK;
	  else if (::strncmp(buffer, "[DMR Id Lookup]", 15U) == 0)
		  section = SECTION_DMRID_LOOKUP;
	  else if (::strncmp(buffer, "[Log]", 5U) == 0)
		  section = SECTION_LOG;
	  else if (::strncmp(buffer, "[aprs.fi]", 5U) == 0)
		  section = SECTION_APRS_FI;	  
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

	if (section == SECTION_YSF_NETWORK) {
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
		else if (::strcmp(key, "RemoteGateway") == 0)
			m_remoteGateway = ::atoi(value) == 1;
		else if (::strcmp(key, "HangTime") == 0)
			m_hangTime = (unsigned int)::atoi(value);
		else if (::strcmp(key, "WiresXMakeUpper") == 0)
			m_wiresXMakeUpper = ::atoi(value) == 1;
		else if (::strcmp(key, "Daemon") == 0)
			m_daemon = ::atoi(value) == 1;
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
	} else if (section == SECTION_INFO) {
		if (::strcmp(key, "TXFrequency") == 0)
			m_txFrequency = (unsigned int)::atoi(value);
		else if (::strcmp(key, "RXFrequency") == 0)
			m_rxFrequency = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Power") == 0)
			m_power = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Latitude") == 0)
			m_latitude = float(::atof(value));
		else if (::strcmp(key, "Longitude") == 0)
			m_longitude = float(::atof(value));
		else if (::strcmp(key, "Height") == 0)
			m_height = ::atoi(value);
		else if (::strcmp(key, "Location") == 0)
			m_location = value;
		else if (::strcmp(key, "Description") == 0)
			m_description = value;
		else if (::strcmp(key, "URL") == 0)
			m_url = value;
	} else if (section == SECTION_DMR_NETWORK) {
		if (::strcmp(key, "Id") == 0)
			m_dmrId = (unsigned int)::atoi(value);
		else if (::strcmp(key, "XLXFile") == 0)
			m_dmrXLXFile = value;
		else if (::strcmp(key, "XLXModule") == 0) {
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_dmrXLXModule = value;
		}
		else if (::strcmp(key, "XLXReflector") == 0)
			m_dmrXLXReflector = (unsigned int)::atoi(value);
		else if (::strcmp(key, "StartupDstId") == 0)
			m_dmrDstId = (unsigned int)::atoi(value);
		else if (::strcmp(key, "StartupPC") == 0)
			m_dmrPC = ::atoi(value) == 1;
		else if (::strcmp(key, "Address") == 0)
			m_dmrNetworkAddress = value;
		else if (::strcmp(key, "Port") == 0)
			m_dmrNetworkPort = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Local") == 0)
			m_dmrNetworkLocal = (unsigned int)::atoi(value);
		else if (::strcmp(key, "Password") == 0)
			m_dmrNetworkPassword = value;
		else if (::strcmp(key, "Options") == 0)
			m_dmrNetworkOptions = value;
		else if (::strcmp(key, "Debug") == 0)
			m_dmrNetworkDebug = ::atoi(value) == 1;
		else if (::strcmp(key, "JitterEnabled") == 0)
			m_dmrNetworkJitterEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Jitter") == 0)
			m_dmrNetworkJitter = (unsigned int)::atoi(value);
		else if (::strcmp(key, "EnableUnlink") == 0)
			m_dmrNetworkEnableUnlink = ::atoi(value) == 1;
		else if (::strcmp(key, "TGUnlink") == 0)
			m_dmrNetworkIDUnlink = (unsigned int)::atoi(value);
		else if (::strcmp(key, "PCUnlink") == 0)
			m_dmrNetworkPCUnlink = ::atoi(value) == 1;
		else if (::strcmp(key, "TGListFile") == 0)
			m_dmrTGListFile = value;
	} else if (section == SECTION_DMRID_LOOKUP) {
		if (::strcmp(key, "File") == 0)
			m_dmrIdLookupFile = value;
		else if (::strcmp(key, "Time") == 0)
			m_dmrIdLookupTime = (unsigned int)::atoi(value);
		if (::strcmp(key, "DropUnknown") == 0)
			m_dmrDropUnknown = ::atoi(value) == 1;
	} else if (section == SECTION_LOG) {
		if (::strcmp(key, "FilePath") == 0)
			m_logFilePath = value;
		else if (::strcmp(key, "FileRoot") == 0)
			m_logFileRoot = value;
		else if (::strcmp(key, "FileLevel") == 0)
			m_logFileLevel = (unsigned int)::atoi(value);
		else if (::strcmp(key, "DisplayLevel") == 0)
			m_logDisplayLevel = (unsigned int)::atoi(value);
	} else if (section == SECTION_APRS_FI) {
		if (::strcmp(key, "AprsCallsign") == 0) {
			// Convert the callsign to upper case
			for (unsigned int i = 0U; value[i] != 0; i++)
				value[i] = ::toupper(value[i]);
			m_aprsCallsign = value;
		} else if (::strcmp(key, "Enable") == 0)
			m_aprsEnabled = ::atoi(value) == 1;
		else if (::strcmp(key, "Server") == 0)
			m_aprsServer = value;
		else if (::strcmp(key, "Port") == 0)
			m_aprsPort = (unsigned int)::atoi(value);	
		else if (::strcmp(key, "Password") == 0)
			m_aprsPassword = value;
		else if (::strcmp(key, "APIKey") == 0)
			m_aprsAPIKey = value;
		else if (::strcmp(key, "Refresh") == 0)
			m_aprsRefresh = (unsigned int)::atoi(value);		
		else if (::strcmp(key, "Description") == 0)
			m_aprsDescription = value;	
	}
  }

  ::fclose(fp);

  return true;
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

bool CConf::getRemoteGateway() const
{
	return m_remoteGateway;
}

unsigned int CConf::getHangTime() const
{
	return m_hangTime;
}

bool CConf::getWiresXMakeUpper() const
{
	return m_wiresXMakeUpper;
}

bool CConf::getDaemon() const
{
	return m_daemon;
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

unsigned int CConf::getRxFrequency() const
{
	return m_rxFrequency;
}

unsigned int CConf::getTxFrequency() const
{
	return m_txFrequency;
}

unsigned int CConf::getPower() const
{
	return m_power;
}

float CConf::getLatitude() const
{
	return m_latitude;
}

float CConf::getLongitude() const
{
	return m_longitude;
}

int CConf::getHeight() const
{
	return m_height;
}

std::string CConf::getLocation() const
{
	return m_location;
}

std::string CConf::getDescription() const
{
	return m_description;
}

std::string CConf::getURL() const
{
	return m_url;
}

unsigned int CConf::getDMRId() const
{
	return m_dmrId;
}

std::string CConf::getDMRXLXFile() const
{
	return m_dmrXLXFile;
}

std::string CConf::getDMRXLXModule() const
{
	return m_dmrXLXModule;
}

unsigned int CConf::getDMRXLXReflector() const
{
	return m_dmrXLXReflector;
}

unsigned int CConf::getDMRDstId() const
{
	return m_dmrDstId;
}

bool CConf::getDMRPC() const
{
	return m_dmrPC;
}

bool CConf::getAPRSEnabled() const
{
	return m_aprsEnabled;
}

std::string CConf::getAPRSServer() const
{
	return m_aprsServer;
}

unsigned int CConf::getAPRSPort() const
{
	return m_aprsPort;
}

std::string CConf::getAPRSCallsign() const
{
	return m_aprsCallsign;
}

std::string CConf::getAPRSPassword() const
{
	return m_aprsPassword;
}

std::string CConf::getAPRSAPIKey() const
{
	return m_aprsAPIKey;
}

unsigned int CConf::getAPRSRefresh() const
{
	return m_aprsRefresh;
}

std::string CConf::getAPRSDescription() const
{
	return m_aprsDescription;
}

std::string CConf::getDMRNetworkAddress() const
{
	return m_dmrNetworkAddress;
}

unsigned int CConf::getDMRNetworkPort() const
{
	return m_dmrNetworkPort;
}

unsigned int CConf::getDMRNetworkLocal() const
{
	return m_dmrNetworkLocal;
}

std::string CConf::getDMRNetworkPassword() const
{
	return m_dmrNetworkPassword;
}

std::string CConf::getDMRNetworkOptions() const
{
	return m_dmrNetworkOptions;
}

bool CConf::getDMRNetworkDebug() const
{
	return m_dmrNetworkDebug;
}

bool CConf::getDMRNetworkJitterEnabled() const
{
	return m_dmrNetworkJitterEnabled;
}

unsigned int CConf::getDMRNetworkJitter() const
{
	return m_dmrNetworkJitter;
}

bool CConf::getDMRNetworkEnableUnlink() const
{
	return m_dmrNetworkEnableUnlink;
}

unsigned int CConf::getDMRNetworkIDUnlink() const
{
	return m_dmrNetworkIDUnlink;
}

bool CConf::getDMRNetworkPCUnlink() const
{
	return m_dmrNetworkPCUnlink;
}

std::string CConf::getDMRTGListFile() const
{
	return m_dmrTGListFile;
}

std::string CConf::getDMRIdLookupFile() const
{
	return m_dmrIdLookupFile;
}

unsigned int CConf::getDMRIdLookupTime() const
{
	return m_dmrIdLookupTime;
}

bool CConf::getDMRDropUnknown() const
{
	return m_dmrDropUnknown;
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
