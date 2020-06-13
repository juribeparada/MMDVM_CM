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

  // The YSF Network section
  std::string  getCallsign() const;
  std::string  getDstAddress() const;
  unsigned int getDstPort() const;
  std::string  getLocalAddress() const;
  unsigned int getLocalPort() const;
  std::string  getFCSFile() const;
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
  bool         getDebug() const;

  // The DMR Network section
  unsigned int getDMRId() const;
  std::string  getDMRRptAddress() const;
  unsigned int getDMRRptPort() const;
  std::string  getDMRLocalAddress() const;
  unsigned int getDMRLocalPort() const;
  unsigned int getDMRDefaultDstTG() const;
  unsigned int getDMRNetworkTGUnlink() const;
  std::string  getDMRTGListFile() const;
  bool         getDMRDebug() const;

  // The DMR Id section
  std::string  getDMRIdLookupFile() const;
  unsigned int getDMRIdLookupTime() const;

  // The Log section
  unsigned int getLogDisplayLevel() const;
  unsigned int getLogFileLevel() const;
  std::string  getLogFilePath() const;
  std::string  getLogFileRoot() const;

private:
  std::string  m_file;
  std::string  m_callsign;
  std::string  m_dstAddress;
  unsigned int m_dstPort;
  std::string  m_localAddress;
  unsigned int m_localPort;
  std::string  m_fcsFile;
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
  std::string   m_ysfRadioID;
  bool         m_daemon;
  bool         m_debug;

  unsigned int m_dmrId;
  std::string  m_dmrRptAddress;
  unsigned int m_dmrRptPort;
  std::string  m_dmrLocalAddress;
  unsigned int m_dmrLocalPort;
  unsigned int m_dmrDefaultDstTG;
  unsigned int m_dmrNetworkTGUnlink;
  std::string  m_dmrTGListFile;
  bool         m_dmrDebug;

  std::string  m_dmrIdLookupFile;
  unsigned int m_dmrIdLookupTime;

  unsigned int m_logDisplayLevel;
  unsigned int m_logFileLevel;
  std::string  m_logFilePath;
  std::string  m_logFileRoot;
};

#endif
