/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by Andy Uribe CA6JAU
* 	Copyright (C) 2021 by Doug McLain AD8DP
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

 // The P25 Network section
  std::string  getCallsign() const;
  uint32_t     getDMRId() const;
  bool         getDaemon() const;
  uint32_t     getP25DstId() const;
  std::string  getP25DstAddress() const;
  uint32_t     getP25DstPort() const;
  std::string  getP25LocalAddress() const;
  uint32_t     getP25LocalPort() const;
  std::string  getP25GainAdjDb() const;
  bool         getP25NetworkDebug() const;
  
  // The USRP Network section
  std::string  getUSRPAddress() const;
  uint16_t     getUSRPDstPort() const;
  uint16_t     getUSRPLocalPort() const;
  std::string  getUSRPGainAdjDb() const;
  bool         getUSRPDebug() const;

  // The Log section
  uint32_t     getLogDisplayLevel() const;
  uint32_t     getLogFileLevel() const;
  std::string  getLogFilePath() const;
  std::string  getLogFileRoot() const;

private:
  std::string  m_file;
  std::string  m_callsign;
  uint32_t     m_dmrid;
  bool         m_daemon;
  
  std::string  m_usrpAddress;
  uint16_t     m_usrpDstPort;
  uint16_t     m_usrpLocalPort;
  std::string  m_usrpGainAdjDb;
  bool         m_usrpDebug;
  
  std::string  m_p25DstAddress;
  uint32_t     m_p25DstPort;
  std::string  m_p25LocalAddress;
  uint32_t     m_p25LocalPort;
  std::string  m_p25GainAdjDb;
  bool         m_p25NetworkDebug;


  uint32_t     m_logDisplayLevel;
  uint32_t     m_logFileLevel;
  std::string  m_logFilePath;
  std::string  m_logFileRoot;
};

#endif
