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

  // The M17 Network section
  std::string  getCallsign() const;
  bool         getDaemon() const;
  std::string  getM17Name() const;
  std::string  getM17Address() const;
  uint16_t     getM17DstPort() const;
  uint16_t     getM17LocalPort() const;
  std::string  getM17GainAdjDb() const;
  bool         getM17Debug() const;
  
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
  bool         m_daemon;
  
  std::string  m_usrpAddress;
  uint16_t     m_usrpDstPort;
  uint16_t     m_usrpLocalPort;
  std::string  m_usrpGainAdjDb;
  bool         m_usrpDebug;
  
  std::string  m_m17Name;
  std::string  m_m17Address;
  uint16_t     m_m17DstPort;
  uint16_t     m_m17LocalPort;
  std::string  m_m17GainAdjDb;
  bool         m_m17Debug;

  uint32_t     m_logDisplayLevel;
  uint32_t     m_logFileLevel;
  std::string  m_logFilePath;
  std::string  m_logFileRoot;
};

#endif
