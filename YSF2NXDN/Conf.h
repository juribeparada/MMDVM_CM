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

#if !defined(CONF_H)
#define	CONF_H

#include <string>
#include <vector>

class CConf
{
public:
  CConf(const std::string& file);
  ~CConf();

  bool read();

  // The Info section
  unsigned int getRxFrequency() const;
  unsigned int getTxFrequency() const;
  float        getLatitude() const;
  float        getLongitude() const;
  int          getHeight() const;
  std::string  getDescription() const;

  // The YSF Network section
  std::string  getCallsign() const;
  std::string  getSuffix() const;
  std::string  getDstAddress() const;
  unsigned int getDstPort() const;
  std::string  getLocalAddress() const;
  unsigned int getLocalPort() const;
  bool         getEnableWiresX() const;
  bool         getWiresXMakeUpper() const;
  unsigned char getFICHCallSign() const;
  unsigned char getFICHCallMode() const;
  unsigned char getFICHFrameTotal() const;
  unsigned char getFICHMessageRoute() const;
  unsigned char getFICHVOIP() const;
  unsigned char getFICHDataType() const;
  unsigned char getFICHSQLType() const;
  unsigned char getFICHSQLCode() const;
  std::vector<unsigned char> getYsfDT1();
  std::vector<unsigned char> getYsfDT2();
  std::string  getYsfRadioID();
  bool         getDaemon() const;

  // The NXDN Network section
  unsigned int getNXDNId() const;
  unsigned int getNXDNDstId() const;
  std::string  getNXDNDstAddress() const;
  unsigned int getNXDNDstPort() const;
  std::string  getNXDNLocalAddress() const;
  unsigned int getNXDNLocalPort() const;
  std::string  getNXDNTGListFile() const;
  bool         getNXDNNetworkDebug() const;

  // The NXDN Id section
  std::string  getNXDNIdLookupFile() const;
  unsigned int getNXDNIdLookupTime() const;

  // The Log section
  unsigned int getLogDisplayLevel() const;
  unsigned int getLogFileLevel() const;
  std::string  getLogFilePath() const;
  std::string  getLogFileRoot() const;

  // The aprs.fi section
  bool         getAPRSEnabled() const;
  std::string  getAPRSServer() const;
  unsigned int getAPRSPort() const;
  std::string  getAPRSPassword() const;
  std::string  getAPRSAPIKey() const;
  unsigned int getAPRSRefresh() const;  
  std::string  getAPRSDescription() const;  

private:
  std::string  m_file;

  std::string  m_callsign;
  std::string  m_suffix;
  std::string  m_dstAddress;
  unsigned int m_dstPort;
  std::string  m_localAddress;
  unsigned int m_localPort;
  bool         m_enableWiresX;
  bool         m_wiresXMakeUpper;
  unsigned char m_fichCallSign;
  unsigned char m_fichCallMode;
  unsigned char m_fichFrameTotal;
  unsigned char m_fichMessageRoute;
  unsigned char m_fichVOIP;
  unsigned char m_fichDataType;
  unsigned char m_fichSQLType;
  unsigned char m_fichSQLCode;
  std::vector<unsigned char> m_ysfDT1;
  std::vector<unsigned char> m_ysfDT2;
  std::string  m_ysfRadioID;
  bool         m_daemon;

  unsigned int m_rxFrequency;
  unsigned int m_txFrequency;
  float        m_latitude;
  float        m_longitude;
  int          m_height;
  std::string  m_description;

  unsigned int m_nxdnId;
  unsigned int m_nxdnDstId;
  std::string  m_nxdnDstAddress;
  unsigned int m_nxdnDstPort;
  std::string  m_nxdnLocalAddress;
  unsigned int m_nxdnLocalPort;
  std::string  m_nxdnTGListFile;
  bool         m_nxdnNetworkDebug;

  std::string  m_nxdnIdLookupFile;
  unsigned int m_nxdnIdLookupTime;

  unsigned int m_logDisplayLevel;
  unsigned int m_logFileLevel;
  std::string  m_logFilePath;
  std::string  m_logFileRoot;
  
  bool         m_aprsEnabled;
  std::string  m_aprsServer;
  unsigned int m_aprsPort;
  std::string  m_aprsPassword;
  std::string  m_aprsAPIKey;
  unsigned int m_aprsRefresh;
  std::string  m_aprsDescription;

};

#endif
