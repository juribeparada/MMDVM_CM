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

  std::string  getCallsign() const;
  bool         getDaemon() const;
  bool 		   getDebug() const;
  
  // The USRP Network section
  std::string  getUSRPAddress() const;
  uint16_t     getUSRPDstPort() const;
  uint16_t     getUSRPLocalPort() const;
  std::string  getUSRPGainAdjDb() const;
  
// The YSF Network section
  std::string  getYSFDstAddress() const;
  unsigned int getYSFDstPort() const;
  std::string  getYSFLocalAddress() const;
  unsigned int getYSFLocalPort() const;
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
  std::string  getYSFGainAdjDb() const;
  
  // The Log section
  unsigned int getLogDisplayLevel() const;
  unsigned int getLogFileLevel() const;
  std::string  getLogFilePath() const;
  std::string  getLogFileRoot() const;

private:
  std::string  m_file;
  bool         m_daemon;
  std::string  m_callsign;
  bool		   m_debug;
  
  std::string  m_YSFDstAddress;
  unsigned int m_YSFDstPort;
  std::string  m_YSFLocalAddress;
  unsigned int m_YSFLocalPort;
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
  std::string  m_ysfGainAdjDb;
  
  std::string  m_usrpAddress;
  uint16_t     m_usrpDstPort;
  uint16_t     m_usrpLocalPort;
  std::string  m_usrpGainAdjDb;
  bool         m_usrpDebug;

  unsigned int m_logDisplayLevel;
  unsigned int m_logFileLevel;
  std::string  m_logFilePath;
  std::string  m_logFileRoot;

};

#endif
