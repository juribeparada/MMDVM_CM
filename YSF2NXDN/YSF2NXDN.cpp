/*
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
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

#include "YSF2NXDN.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

// DT1 and DT2, suggested by Manuel EA7EE
const unsigned char dt1_temp[] = {0x31, 0x22, 0x62, 0x5F, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00};
const unsigned char dt2_temp[] = {0x00, 0x00, 0x00, 0x00, 0x6C, 0x20, 0x1C, 0x20, 0x03, 0x08};

#define NXDN_FRAME_PER      75U
#define YSF_FRAME_PER       90U

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "YSF2NXDN.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/YSF2NXDN.ini";
#endif

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2018,2019 by CA6JAU, G4KLX and others";

#include <functional>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <cctype>

int end = 0;

#if !defined(_WIN32) && !defined(_WIN64)
void sig_handler(int signo)
{
	if (signo == SIGTERM) {
		end = 1;
		::fprintf(stdout, "Received SIGTERM\n");
	}
}
#endif

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "YSF2NXDN version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: YSF2NXDN [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

#if !defined(_WIN32) && !defined(_WIN64)
	// Capture SIGTERM to finish gracelessly
	if (signal(SIGTERM, sig_handler) == SIG_ERR) 
		::fprintf(stdout, "Can't catch SIGTERM\n");
#endif

	CYSF2NXDN* gateway = new CYSF2NXDN(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CYSF2NXDN::CYSF2NXDN(const std::string& configFile) :
m_callsign(),
m_suffix(),
m_conf(configFile),
m_wiresX(NULL),
m_nxdnNetwork(NULL),
m_ysfNetwork(NULL),
m_lookup(NULL),
m_conv(),
m_srcid(1U),
m_defsrcid(1U),
m_dstid(1U),
m_netSrc(),
m_netDst(),
m_ysfSrc(),
m_ysfFrame(NULL),
m_nxdnFrame(NULL),
m_gps(NULL),
m_dtmf(NULL),
m_APRS(NULL),
m_nxdnFrames(0U),
m_ysfFrames(0U),
m_nxdninfo(false)
{
	m_ysfFrame  = new unsigned char[200U];
	m_nxdnFrame = new unsigned char[200U];

	::memset(m_ysfFrame, 0U, 200U);
	::memset(m_nxdnFrame, 0U, 200U);
}

CYSF2NXDN::~CYSF2NXDN()
{
	delete[] m_ysfFrame;
	delete[] m_nxdnFrame;
}

int CYSF2NXDN::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "YSF2NXDN: cannot read the .ini file\n");
		return 1;
	}

	setlocale(LC_ALL, "C");

	unsigned int logDisplayLevel = m_conf.getLogDisplayLevel();

#if !defined(_WIN32) && !defined(_WIN64)
	if(m_conf.getDaemon())
		logDisplayLevel = 0U;
#endif

#if !defined(_WIN32) && !defined(_WIN64)
	bool m_daemon = m_conf.getDaemon();
	if (m_daemon) {
		// Create new process
		pid_t pid = ::fork();
		if (pid == -1) {
			::fprintf(stderr, "Couldn't fork() , exiting\n");
			return -1;
		} else if (pid != 0)
			exit(EXIT_SUCCESS);

		// Create new session and process group
		if (::setsid() == -1) {
			::fprintf(stderr, "Couldn't setsid(), exiting\n");
			return -1;
		}

		// Set the working directory to the root directory
		if (::chdir("/") == -1) {
			::fprintf(stderr, "Couldn't cd /, exiting\n");
			return -1;
		}

		//If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return -1;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			//Set user and group ID's to mmdvm:mmdvm
			if (setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return -1;
			}

			if (setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return -1;
			}

			//Double check it worked (AKA Paranoia) 
			if (setuid(0) != -1) {
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return -1;
			}
		}
	}
#endif

	ret = ::LogInitialise(m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), logDisplayLevel);
	if (!ret) {
		::fprintf(stderr, "YSF2NXDN: unable to open the log file\n");
		return 1;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}
#endif

	LogInfo(HEADER1);
	LogInfo(HEADER2);
	LogInfo(HEADER3);
	LogInfo(HEADER4);

	m_callsign = m_conf.getCallsign();
	m_suffix   = m_conf.getSuffix();
	m_defsrcid = m_conf.getNXDNId();
	m_dstid    = m_conf.getNXDNDstId();

	bool debug               = m_conf.getNXDNNetworkDebug();
	in_addr dstAddress       = CUDPSocket::lookup(m_conf.getDstAddress());
	unsigned int dstPort     = m_conf.getDstPort();
	std::string localAddress = m_conf.getLocalAddress();
	unsigned int localPort   = m_conf.getLocalPort();

	m_ysfNetwork = new CYSFNetwork(localAddress, localPort, m_callsign, debug);
	m_ysfNetwork->setDestination(dstAddress, dstPort);

	ret = m_ysfNetwork->open();
	if (!ret) {
		::LogError("Cannot open the YSF network port");
		::LogFinalise();
		return 1;
	}

	std::string nxdn_dstAddress    = m_conf.getNXDNDstAddress();
	unsigned int nxdn_dstPort      = m_conf.getNXDNDstPort();
	std::string nxdn_localAddress  = m_conf.getNXDNLocalAddress();
	unsigned int nxdn_localPort    = m_conf.getNXDNLocalPort();

	m_nxdnNetwork = new CNXDNNetwork(nxdn_localAddress, nxdn_localPort, nxdn_dstAddress, nxdn_dstPort, debug);

	ret = m_nxdnNetwork->open();
	if (!ret) {
		::LogError("Cannot open the NXDN network port");
		::LogFinalise();
		return 1;
	}

	std::string lookupFile  = m_conf.getNXDNIdLookupFile();
	unsigned int reloadTime = m_conf.getNXDNIdLookupTime();

	m_lookup = new CNXDNLookup(lookupFile, reloadTime);
	m_lookup->read();

	CTimer networkWatchdog(100U, 0U, 1500U);
	CTimer pollTimer(1000U, 5U);

	std::string name = m_conf.getDescription();
	bool enableWiresX = m_conf.getEnableWiresX();
	std::string TGList = m_conf.getNXDNTGListFile();
	unsigned int rxFrequency = m_conf.getRxFrequency();
	unsigned int txFrequency = m_conf.getTxFrequency();

	// CWiresX Control Object
	if (enableWiresX) {
		bool makeUpper = m_conf.getWiresXMakeUpper();
		m_wiresX = new CWiresX(m_callsign, m_suffix, m_ysfNetwork, TGList, makeUpper);
		m_dtmf = new CDTMF;
	}

	if (m_wiresX != NULL)
		m_wiresX->setInfo(name, txFrequency, rxFrequency, m_dstid);

	if (m_conf.getAPRSEnabled()) {
		createGPS();
		m_APRS = new CAPRSReader(m_conf.getAPRSAPIKey(), m_conf.getAPRSRefresh());
	}
	
	CStopWatch TGChange;
	CStopWatch stopWatch;
	CStopWatch ysfWatch;
	CStopWatch nxdnWatch;
	stopWatch.start();
	ysfWatch.start();
	nxdnWatch.start();
	pollTimer.start();

	unsigned char ysf_cnt = 0;
	unsigned char nxdn_cnt = 0;

	LogMessage("Starting YSF2NXDN-%s", VERSION);

	unsigned char gps_buffer[20U];

	for (; end == 0;) {
		unsigned char buffer[2000U];

		unsigned int ms = stopWatch.elapsed();

		while (m_ysfNetwork->read(buffer) > 0U) {
			CYSFFICH fich;
			bool valid = fich.decode(buffer + 35U);

			if (valid) {
				unsigned char fi = fich.getFI();
				unsigned char dt = fich.getDT();
				unsigned char fn = fich.getFN();
				unsigned char ft = fich.getFT();
				
				if (m_wiresX != NULL) {
					WX_STATUS status = m_wiresX->process(buffer + 35U, buffer + 14U, fi, dt, fn, ft);
					m_ysfSrc = getSrcYSF(buffer);

					switch (status) {
						case WXS_CONNECT:
							m_srcid = findYSFID(m_ysfSrc, false);
							m_dstid = m_wiresX->getDstID();
							LogMessage("Connect to TG %d has been requested by %s", m_dstid, m_ysfSrc.c_str());
							sendNXDNPTT(m_srcid, m_dstid);
							break;

						case WXS_DX:
							break;

						case WXS_DISCONNECT:
							LogMessage("Disconnect has been requested by %s", m_ysfSrc.c_str());
							sendNXDNPTT(m_srcid, 9999U);
							break;

						default:
							break;
					}

					status = WXS_NONE;

					if (dt == YSF_DT_VD_MODE2)
						status = m_dtmf->decodeVDMode2(buffer + 35U, (buffer[34U] & 0x01U) == 0x01U);

					switch (status) {
						case WXS_CONNECT:
							m_srcid = findYSFID(m_ysfSrc, false);
							m_dstid = m_dtmf->getDstID();
							LogMessage("Connect to TG %d has been requested by %s", m_dstid, m_ysfSrc.c_str());
							sendNXDNPTT(m_srcid, m_dstid);
							break;

						case WXS_DISCONNECT:
							LogMessage("Disconnect via DTMF has been requested by %s", m_ysfSrc.c_str());
							sendNXDNPTT(m_srcid, 9999U);
							break;

						default:
							break;
					}
				}

				if ((::memcmp(buffer, "YSFD", 4U) == 0U) && (dt == YSF_DT_VD_MODE2)) {
					CYSFPayload ysfPayload;

					if (fi == YSF_FI_HEADER) {
						if (ysfPayload.processHeaderData(buffer + 35U)) {
							std::string ysfSrc = ysfPayload.getSource();
							std::string ysfDst = ysfPayload.getDest();
							LogMessage("Received YSF Header: Src: %s Dst: %s", ysfSrc.c_str(), ysfDst.c_str());
							m_srcid = findYSFID(ysfSrc, true);
							m_conv.putYSFHeader();
							m_ysfFrames = 0U;
						}
					} else if (fi == YSF_FI_TERMINATOR) {
						LogMessage("YSF received end of voice transmission, %.1f seconds", float(m_ysfFrames) / 10.0F);
						m_conv.putYSFEOT();
						m_ysfFrames = 0U;
					} else if (fi == YSF_FI_COMMUNICATIONS) {
						m_conv.putYSF(buffer + 35U);
						m_ysfFrames++;
					}
				}

				if (m_gps != NULL)
					m_gps->data(buffer + 14U, buffer + 35U, fi, dt, fn, ft);

			}

			if ((buffer[34U] & 0x01U) == 0x01U) {
				if (m_gps != NULL)
					m_gps->reset();
				if (m_dtmf != NULL)
					m_dtmf->reset();
			}
		}

		if (nxdnWatch.elapsed() > NXDN_FRAME_PER) {
			unsigned int nxdnFrameType = m_conv.getNXDN(m_nxdnFrame);

			if(nxdnFrameType == TAG_HEADER) {
				nxdn_cnt = 0U;

				CNXDNLICH lich;
				lich.setRFCT(NXDN_LICH_RFCT_RDCH);
				lich.setFCT(NXDN_LICH_USC_SACCH_NS);
				lich.setOption(NXDN_LICH_STEAL_FACCH);
				lich.setDirection(NXDN_LICH_DIRECTION_INBOUND);
				m_nxdnFrame[0U] = lich.getRaw();

				CNXDNSACCH sacch;
				sacch.setRAN(0x01);
				sacch.setStructure(NXDN_SR_SINGLE);
				sacch.setData(SACCH_IDLE);
				sacch.getRaw(m_nxdnFrame + 1U);

				unsigned char layer3data[25U];
				CNXDNLayer3 layer3;
				layer3.setMessageType(NXDN_MESSAGE_TYPE_VCALL);
				layer3.setSourceUnitId(m_srcid & 0xFFFF);
				layer3.setDestinationGroupId(m_dstid & 0xFFFF);
				layer3.setGroup(true);
				layer3.setDataBlocks(0U);
				layer3.getData(layer3data);

				::memcpy(m_nxdnFrame + 5U, layer3data, 14U);
				::memcpy(m_nxdnFrame + 5U + 14U, layer3data, 14U);

				m_nxdnNetwork->write(m_nxdnFrame, false);

				nxdnWatch.start();
			}
			else if (nxdnFrameType == TAG_EOT) {
				CNXDNLICH lich;
				lich.setRFCT(NXDN_LICH_RFCT_RDCH);
				lich.setFCT(NXDN_LICH_USC_SACCH_NS);
				lich.setOption(NXDN_LICH_STEAL_FACCH);
				lich.setDirection(NXDN_LICH_DIRECTION_INBOUND);
				m_nxdnFrame[0U] = lich.getRaw();

				CNXDNSACCH sacch;
				sacch.setRAN(0x01);
				sacch.setStructure(NXDN_SR_SINGLE);
				sacch.setData(SACCH_IDLE);
				sacch.getRaw(m_nxdnFrame + 1U);

				unsigned char layer3data[25U];
				CNXDNLayer3 layer3;
				layer3.setMessageType(NXDN_MESSAGE_TYPE_TX_REL);
				layer3.setSourceUnitId(m_srcid & 0xFFFF);
				layer3.setDestinationGroupId(m_dstid & 0xFFFF);
				layer3.setGroup(true);
				layer3.setDataBlocks(0U);
				layer3.getData(layer3data);

				::memcpy(m_nxdnFrame + 5U, layer3data, 14U);
				::memcpy(m_nxdnFrame + 5U + 14U, layer3data, 14U);

				m_nxdnNetwork->write(m_nxdnFrame, false);

				nxdn_cnt = 0U;
			}
			else if (nxdnFrameType == TAG_DATA) {
				CNXDNLICH lich;
				lich.setRFCT(NXDN_LICH_RFCT_RDCH);
				lich.setFCT(NXDN_LICH_USC_SACCH_SS);
				lich.setOption(NXDN_LICH_STEAL_NONE);
				lich.setDirection(NXDN_LICH_DIRECTION_INBOUND);
				m_nxdnFrame[0U] = lich.getRaw();

				CNXDNSACCH sacch;
				CNXDNLayer3 layer3;
				unsigned char message[3U];

				layer3.setMessageType(NXDN_MESSAGE_TYPE_VCALL);
				layer3.setSourceUnitId(m_srcid & 0xFFFF);
				layer3.setDestinationGroupId(m_dstid & 0xFFFF);
				layer3.setGroup(true);
				layer3.setDataBlocks(0U);

				switch (nxdn_cnt % 4) {
					case 0:
						sacch.setStructure(NXDN_SR_1_4);
						layer3.encode(message, 18U, 0U);
						sacch.setData(message);
						break;
					case 1:
						sacch.setStructure(NXDN_SR_2_4);
						layer3.encode(message, 18U, 18U);
						sacch.setData(message);
						break;
					case 2:
						sacch.setStructure(NXDN_SR_3_4);
						layer3.encode(message, 18U, 36U);
						sacch.setData(message);
						break;
					case 3:
						sacch.setStructure(NXDN_SR_4_4);
						layer3.encode(message, 18U, 54U);
						sacch.setData(message);
						break;
				}

				sacch.setRAN(0x01);
				sacch.getRaw(m_nxdnFrame + 1U);

				// Send data to MMDVMHost
				m_nxdnNetwork->write(m_nxdnFrame, false);

				nxdn_cnt++;
				nxdnWatch.start();
			}
		}

		unsigned int grp = 0;
		unsigned int srcId = 0;
		unsigned int dstId = 0;

		while (m_nxdnNetwork->read(m_nxdnFrame) > 0U) {
			//CUtils::dump(1U, "NXDN Net:", m_nxdnFrame, 33U);
			if ((m_nxdnFrame[0U] == 0x81U || m_nxdnFrame[0U] == 0x83U) && (m_nxdnFrame[5U] == 0x01U || m_nxdnFrame[5U] == 0x08U)) {
				grp = (m_nxdnFrame[7U] & 0x20U) == 0x20U;

				srcId  = (m_nxdnFrame[8U] << 8) & 0xFF00U;
				srcId |= (m_nxdnFrame[9U] << 0) & 0x00FFU;

				dstId  = (m_nxdnFrame[10U] << 8) & 0xFF00U;
				dstId |= (m_nxdnFrame[11U] << 0) & 0x00FFU;

				if (grp && m_dstid == dstId) {
					if (m_nxdnFrame[5U] == 0x01) {
						// DT1 & DT2 without GPS info
						::memcpy(gps_buffer, dt1_temp, 10U);
						::memcpy(gps_buffer + 10U, dt2_temp, 10U);

						m_netSrc = m_lookup->findCS(srcId);
						//m_netDst = m_lookup->findCS(dstId);
						m_netDst = "TG " + std::to_string(dstId);
                                                LogMessage("Received NXDN Header: Src: %s Dst: %s", m_netSrc.c_str(), m_netDst.c_str());

						m_conv.putNXDNHeader();
						m_nxdnFrames = 0;
						m_nxdninfo = true;

						m_netSrc.resize(YSF_CALLSIGN_LENGTH, ' ');
						m_netDst.resize(YSF_CALLSIGN_LENGTH, ' ');
					}
					else if (m_nxdnFrame[5U] == 0x08) {
						LogMessage("NXDN received end of voice transmission, %.1f seconds", float(m_nxdnFrames) / 12.5F);
						m_conv.putNXDNEOT();
						m_nxdnFrames = 0;
						m_nxdninfo = false;
					}
				}
			}
			else {
				if (!m_nxdninfo) {
					// DT1 & DT2 without GPS info
					::memcpy(gps_buffer, dt1_temp, 10U);
					::memcpy(gps_buffer + 10U, dt2_temp, 10U);

					m_netSrc = m_lookup->findCS(srcId);
					m_netDst = m_lookup->findCS(dstId);
					LogMessage("Received NXDN entry late: Src: %s Dst: %s", m_netSrc.c_str(), m_netDst.c_str());

					m_conv.putNXDNHeader();
					m_nxdnFrames = 0;
					m_nxdninfo = true;

					m_netSrc.resize(YSF_CALLSIGN_LENGTH, ' ');
					m_netDst.resize(YSF_CALLSIGN_LENGTH, ' ');
				}

				m_conv.putNXDN(m_nxdnFrame);
				m_nxdnFrames++;
			}
		}

		if (ysfWatch.elapsed() > YSF_FRAME_PER) {
			unsigned int ysfFrameType = m_conv.getYSF(m_ysfFrame + 35U);

			if(ysfFrameType == TAG_HEADER) {
				ysf_cnt = 0U;

				::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
				::memcpy(m_ysfFrame + 4U, m_ysfNetwork->getCallsign().c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 14U, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);
				m_ysfFrame[34U] = 0U; // Net frame counter

				CSync::addYSFSync(m_ysfFrame + 35U);

				// Set the FICH
				CYSFFICH fich;
				fich.setFI(YSF_FI_HEADER);
				fich.setCS(m_conf.getFICHCallSign());
 				fich.setCM(m_conf.getFICHCallMode());
 				fich.setBN(0U);
 				fich.setBT(0U);
				fich.setFN(0U);
				fich.setFT(m_conf.getFICHFrameTotal());
				fich.setDev(0U);
				fich.setMR(m_conf.getFICHMessageRoute());
 				fich.setVoIP(m_conf.getFICHVOIP());
 				fich.setDT(m_conf.getFICHDataType());
 				fich.setSQL(m_conf.getFICHSQLType());
 				fich.setSQ(m_conf.getFICHSQLCode());
				fich.encode(m_ysfFrame + 35U);

				unsigned char csd1[20U], csd2[20U];
				memset(csd1, '*', YSF_CALLSIGN_LENGTH/2);
				memcpy(csd1 + YSF_CALLSIGN_LENGTH/2, m_conf.getYsfRadioID().c_str(), YSF_CALLSIGN_LENGTH/2);
				memcpy(csd1 + YSF_CALLSIGN_LENGTH, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
				memset(csd2, ' ', YSF_CALLSIGN_LENGTH + YSF_CALLSIGN_LENGTH);

				CYSFPayload payload;
				payload.writeHeader(m_ysfFrame + 35U, csd1, csd2);

				m_ysfNetwork->write(m_ysfFrame);

				ysf_cnt++;
				ysfWatch.start();
			}
			else if (ysfFrameType == TAG_EOT) {
				::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
				::memcpy(m_ysfFrame + 4U, m_ysfNetwork->getCallsign().c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 14U, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);
				m_ysfFrame[34U] = ysf_cnt; // Net frame counter

				CSync::addYSFSync(m_ysfFrame + 35U);

				// Set the FICH
				CYSFFICH fich;
                                fich.setFI(YSF_FI_HEADER);
                                fich.setCS(m_conf.getFICHCallSign());
                                fich.setCM(m_conf.getFICHCallMode());
                                fich.setBN(0U);
                                fich.setBT(0U);
                                fich.setFN(0U);
                                fich.setFT(m_conf.getFICHFrameTotal());
                                fich.setDev(0U);
                                fich.setMR(m_conf.getFICHMessageRoute());
                                fich.setVoIP(m_conf.getFICHVOIP());
                                fich.setDT(m_conf.getFICHDataType());
                                fich.setSQL(m_conf.getFICHSQLType());
                                fich.setSQ(m_conf.getFICHSQLCode());
                                fich.encode(m_ysfFrame + 35U);

                                unsigned char csd1[20U], csd2[20U];
                                memset(csd1, '*', YSF_CALLSIGN_LENGTH/2);
                                memcpy(csd1 + YSF_CALLSIGN_LENGTH/2, m_conf.getYsfRadioID().c_str(), YSF_CALLSIGN_LENGTH/2);
                                memcpy(csd1 + YSF_CALLSIGN_LENGTH, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
                                memset(csd2, ' ', YSF_CALLSIGN_LENGTH + YSF_CALLSIGN_LENGTH);

				CYSFPayload payload;
				payload.writeHeader(m_ysfFrame + 35U, csd1, csd2);

				m_ysfNetwork->write(m_ysfFrame);
			}
			else if (ysfFrameType == TAG_DATA) {
				CYSFFICH fich;
				CYSFPayload ysfPayload;
				unsigned char dch[10U];

				unsigned int fn = (ysf_cnt - 1U) % 7U;

				::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
				::memcpy(m_ysfFrame + 4U, m_ysfNetwork->getCallsign().c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 14U, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);

				// Add the YSF Sync
				CSync::addYSFSync(m_ysfFrame + 35U);

				switch (fn) {
					case 0:
						memset(dch, '*', YSF_CALLSIGN_LENGTH/2);
 						memcpy(dch + YSF_CALLSIGN_LENGTH/2, m_conf.getYsfRadioID().c_str(), YSF_CALLSIGN_LENGTH/2);
 						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, dch);
						break;
					case 1:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (unsigned char*)m_netSrc.c_str());
						break;
					case 2:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (unsigned char*)m_netDst.c_str());
						break;
					case 5:
						memset(dch, ' ', YSF_CALLSIGN_LENGTH/2);
 						memcpy(dch + YSF_CALLSIGN_LENGTH/2, m_conf.getYsfRadioID().c_str(), YSF_CALLSIGN_LENGTH/2);
 						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, dch);	// Rem3/4
 						break;
					case 6: {
							unsigned char dt1[10U] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
							for (unsigned int i = 0U; i < m_conf.getYsfDT1().size() && i < 10U; i++)
								dt1[i] = m_conf.getYsfDT1()[i];
							ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, dt1);
						}
						break;
					case 7: {
							unsigned char dt2[10U] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
							for (unsigned int i = 0U; i < m_conf.getYsfDT2().size() && i < 10U; i++)
								dt2[i] = m_conf.getYsfDT2()[i];
							ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, dt2);
						}
						break;
					default:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (const unsigned char*)"          ");
				}

				// Set the FICH
				fich.setFI(YSF_FI_COMMUNICATIONS);
				fich.setCS(m_conf.getFICHCallSign());
 				fich.setCM(m_conf.getFICHCallMode());
 				fich.setBN(0U);
 				fich.setBT(0U);
 				fich.setFN(fn);
				fich.setFT(m_conf.getFICHFrameTotal());
				fich.setDev(0U);
				fich.setMR(m_conf.getFICHMessageRoute());
 				fich.setVoIP(m_conf.getFICHVOIP());
 				fich.setDT(m_conf.getFICHDataType());
 				fich.setSQL(m_conf.getFICHSQLType());
 				fich.setSQ(m_conf.getFICHSQLCode());
				fich.encode(m_ysfFrame + 35U);

				// Net frame counter
				m_ysfFrame[34U] = (ysf_cnt & 0x7FU) << 1;

				// Send data to MMDVMHost
				m_ysfNetwork->write(m_ysfFrame);

				ysf_cnt++;
				ysfWatch.start();
			}
		}

		stopWatch.start();

		m_ysfNetwork->clock(ms);
		m_nxdnNetwork->clock(ms);

		if (m_wiresX != NULL)
			m_wiresX->clock(ms);

		if (m_gps != NULL)
			m_gps->clock(ms);

		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_ysfNetwork->writePoll();
			pollTimer.start();
		}

		if (ms < 5U)
			CThread::sleep(5U);
	}

	m_ysfNetwork->close();
	m_nxdnNetwork->close();

	if (m_APRS != NULL) {
		m_APRS->stop();
		delete m_APRS;
	}

	if (m_gps != NULL) {
		m_gps->close();
		delete m_gps;
	}

	delete m_nxdnNetwork;
	delete m_ysfNetwork;

	if (m_wiresX != NULL) {
		delete m_wiresX;
		delete m_dtmf;
	}

	::LogFinalise();

	return 0;
}

void CYSF2NXDN::createGPS()
{
	std::string hostname = m_conf.getAPRSServer();
	unsigned int port    = m_conf.getAPRSPort();
	std::string password = m_conf.getAPRSPassword();
	std::string desc     = m_conf.getAPRSDescription();

	LogMessage("APRS Parameters");
	LogMessage("    Server: %s", hostname.c_str());
	LogMessage("    Port: %u", port);
	LogMessage("    Passworwd: %s", password.c_str());
	LogMessage("    Description: %s", desc.c_str());

	m_gps = new CGPS(m_callsign, m_suffix, password, hostname, port);

	unsigned int txFrequency = m_conf.getTxFrequency();
	unsigned int rxFrequency = m_conf.getRxFrequency();
	float latitude           = m_conf.getLatitude();
	float longitude          = m_conf.getLongitude();
	int height               = m_conf.getHeight();

	m_gps->setInfo(txFrequency, rxFrequency, latitude, longitude, height, desc);

	bool ret = m_gps->open();
	if (!ret) {
		delete m_gps;
		LogMessage("Error starting GPS");
		m_gps = NULL;
	}
}

unsigned int CYSF2NXDN::findYSFID(std::string cs, bool showdst)
{
	std::string cstrim;
	bool nxdnpc = false;

	int first = cs.find_first_not_of(' ');
	int mid1 = cs.find_last_of('-');
	int mid2 = cs.find_last_of('/');
	int last = cs.find_last_not_of(' ');

	if (mid1 == -1 && mid2 == -1 && first == -1 && last == -1)
		cstrim = "N0CALL";
	else if (mid1 == -1 && mid2 == -1)
		cstrim = cs.substr(first, (last - first + 1));
	else if (mid1 > first)
		cstrim = cs.substr(first, (mid1 - first));
	else if (mid2 > first)
		cstrim = cs.substr(first, (mid2 - first));
	else
		cstrim = "N0CALL";

	unsigned int id = m_lookup->findID(cstrim);

	if (id == 0) {
		id = m_defsrcid;
		if (showdst)
			LogMessage("Not NXDN ID found, using default ID: %u, DstID: %s%u", id, nxdnpc ? "" : "TG ", m_dstid);
		else
			LogMessage("Not NXDN ID found, using default ID: %u", id);
	}
	else {
		if (showdst)
			LogMessage("NXDN ID of %s: %u, DstID: %s%u", cstrim.c_str(), id, nxdnpc ? "" : "TG ", m_dstid);
		else
			LogMessage("NXDN ID of %s: %u", cstrim.c_str(), id);
	}

	return id;
}

std::string CYSF2NXDN::getSrcYSF(const unsigned char* buffer)
{
	unsigned char temp[YSF_CALLSIGN_LENGTH + 1U];

	::memcpy(temp, buffer + 14U, YSF_CALLSIGN_LENGTH);
	temp[YSF_CALLSIGN_LENGTH] = 0U;
	
	std::string trimmed = reinterpret_cast<char const*>(temp);
	trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), trimmed.end());
	
	return trimmed;
}

void CYSF2NXDN::sendNXDNPTT(unsigned int src, unsigned int dst)
{
	// Send NXDN Header
	CNXDNLICH lich;
	lich.setRFCT(NXDN_LICH_RFCT_RDCH);
	lich.setFCT(NXDN_LICH_USC_SACCH_NS);
	lich.setOption(NXDN_LICH_STEAL_FACCH);
	lich.setDirection(NXDN_LICH_DIRECTION_INBOUND);
	m_nxdnFrame[0U] = lich.getRaw();

	CNXDNSACCH sacch;
	sacch.setRAN(0x01);
	sacch.setStructure(NXDN_SR_SINGLE);
	sacch.setData(SACCH_IDLE);
	sacch.getRaw(m_nxdnFrame + 1U);

	unsigned char layer3data[25U];
	CNXDNLayer3 layer3;
	layer3.setMessageType(NXDN_MESSAGE_TYPE_VCALL);
	layer3.setSourceUnitId(src & 0xFFFF);
	layer3.setDestinationGroupId(dst & 0xFFFF);
	layer3.setGroup(true);
	layer3.setDataBlocks(0U);
	layer3.getData(layer3data);

	::memcpy(m_nxdnFrame + 5U, layer3data, 14U);
	::memcpy(m_nxdnFrame + 5U + 14U, layer3data, 14U);

	m_nxdnNetwork->write(m_nxdnFrame, false);

	// Send NXDN EOT
	lich.setRFCT(NXDN_LICH_RFCT_RDCH);
	lich.setFCT(NXDN_LICH_USC_SACCH_NS);
	lich.setOption(NXDN_LICH_STEAL_FACCH);
	lich.setDirection(NXDN_LICH_DIRECTION_INBOUND);
	m_nxdnFrame[0U] = lich.getRaw();

	sacch.setRAN(0x01);
	sacch.setStructure(NXDN_SR_SINGLE);
	sacch.setData(SACCH_IDLE);
	sacch.getRaw(m_nxdnFrame + 1U);

	layer3.setMessageType(NXDN_MESSAGE_TYPE_TX_REL);
	layer3.setSourceUnitId(src & 0xFFFF);
	layer3.setDestinationGroupId(dst & 0xFFFF);
	layer3.setGroup(true);
	layer3.setDataBlocks(0U);
	layer3.getData(layer3data);

	::memcpy(m_nxdnFrame + 5U, layer3data, 14U);
	::memcpy(m_nxdnFrame + 5U + 14U, layer3data, 14U);

	m_nxdnNetwork->write(m_nxdnFrame, false);
}

