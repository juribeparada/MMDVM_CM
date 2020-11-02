/*
 *   Copyright (C) 2015,2016,2017 by Jonathan Naylor G4KLX
 *   Copyright (C) 2018 by Andy Uribe CA6JAU
* 	Copyright (C) 2020 by Doug McLain AD8DP
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
  unsigned int getM17DstId() const;
  std::string  getM17DstName() const;
  std::string  getM17DstAddress() const;
  unsigned int getM17DstPort() const;
  std::string  getM17LocalAddress() const;
  unsigned int getM17LocalPort() const;
  std::string  getM17GainAdjDb() const;
  bool         getM17NetworkDebug() const;
  
  // The DMR Network section
  unsigned int getDMRId() const;
  std::string  getDMRRptAddress() const;
  unsigned int getDMRRptPort() const;
  std::string  getDMRLocalAddress() const;
  unsigned int getDMRLocalPort() const;
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
  bool         m_daemon;
  
  unsigned int m_dmrId;
  std::string  m_dmrRptAddress;
  unsigned int m_dmrRptPort;
  std::string  m_dmrLocalAddress;
  unsigned int m_dmrLocalPort;
  bool         m_dmrDebug;
  
  unsigned int m_m17DstId;
  std::string  m_m17DstName;
  std::string  m_m17DstAddress;
  unsigned int m_m17DstPort;
  std::string  m_m17LocalAddress;
  unsigned int m_m17LocalPort;
  std::string  m_m17GainAdjDb;
  bool         m_m17NetworkDebug;

  std::string  m_dmrIdLookupFile;
  unsigned int m_dmrIdLookupTime;

  unsigned int m_logDisplayLevel;
  unsigned int m_logFileLevel;
  std::string  m_logFilePath;
  std::string  m_logFileRoot;

};

#endif
