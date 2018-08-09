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
  SECTION_NXDN_NETWORK,
  SECTION_DMR_NETWORK,
  SECTION_DMRID_LOOKUP,
  SECTION_NXDNID_LOOKUP,
  SECTION_LOG
};

CConf::CConf(const std::string& file) :
m_file(file),
m_callsign(),
m_tg(20U),
m_dstAddress(),
m_dstPort(0U),
m_localAddress(),
m_localPort(0U),
m_defaultID(65519U),
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
m_dmrIdLookupFile(),
m_dmrIdLookupTime(0U),
m_nxdnIdLookupFile(),
m_nxdnIdLookupTime(0U),
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
			else if (::strncmp(buffer, "[NXDN Network]", 14U) == 0)
				section = SECTION_NXDN_NETWORK;
			else if (::strncmp(buffer, "[DMR Network]", 13U) == 0)
				section = SECTION_DMR_NETWORK;
			else if (::strncmp(buffer, "[DMR Id Lookup]", 15U) == 0)
				section = SECTION_DMRID_LOOKUP;
			else if (::strncmp(buffer, "[NXDN Id Lookup]", 16U) == 0)
				section = SECTION_NXDNID_LOOKUP;
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

		if (section == SECTION_NXDN_NETWORK) {
			if (::strcmp(key, "Callsign") == 0) {
				// Convert the callsign to upper case
				for (unsigned int i = 0U; value[i] != 0; i++)
					value[i] = ::toupper(value[i]);
				m_callsign = value;
			} else if (::strcmp(key, "TG") == 0)
				m_tg = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DstAddress") == 0)
				m_dstAddress = value;
			else if (::strcmp(key, "DstPort") == 0)
				m_dstPort = (unsigned int)::atoi(value);
			else if (::strcmp(key, "LocalAddress") == 0)
				m_localAddress = value;
			else if (::strcmp(key, "LocalPort") == 0)
				m_localPort = (unsigned int)::atoi(value);
			else if (::strcmp(key, "DefaultID") == 0)
				m_defaultID = (unsigned int)::atoi(value);
			else if (::strcmp(key, "Daemon") == 0)
				m_daemon = ::atoi(value) == 1;
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
		} else if (section == SECTION_DMRID_LOOKUP) {
			if (::strcmp(key, "File") == 0)
				m_dmrIdLookupFile = value;
			else if (::strcmp(key, "Time") == 0)
				m_dmrIdLookupTime = (unsigned int)::atoi(value);
		} else if (section == SECTION_NXDNID_LOOKUP) {
			if (::strcmp(key, "File") == 0)
				m_nxdnIdLookupFile = value;
			else if (::strcmp(key, "Time") == 0)
				m_nxdnIdLookupTime = (unsigned int)::atoi(value);
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

unsigned int CConf::getTG() const
{
	return m_tg;
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

unsigned int CConf::getDefaultID() const
{
	return m_defaultID;
}

bool CConf::getDaemon() const
{
	return m_daemon;
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

std::string CConf::getDMRIdLookupFile() const
{
	return m_dmrIdLookupFile;
}

unsigned int CConf::getDMRIdLookupTime() const
{
	return m_dmrIdLookupTime;
}

std::string CConf::getNXDNIdLookupFile() const
{
	return m_nxdnIdLookupFile;
}

unsigned int CConf::getNXDNIdLookupTime() const
{
	return m_nxdnIdLookupTime;
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
