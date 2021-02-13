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

#include "YSF2P25.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

// Unpacked IMBE silence
const unsigned char IMBE_SILENCE[] = {0x04U, 0x0CU, 0xFDU, 0x7BU, 0xFBU, 0x7DU, 0xF2U, 0x7BU, 0x3DU, 0x9EU, 0x44};

const unsigned char REC62[] = {
	0x62U, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x12U, 0x64U, 0x00U, 0x00U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned char REC63[] = {
	0x63U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC64[] = {
	0x64U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC65[] = {
	0x65U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC66[] = {
	0x66U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC67[] = {
	0x67U, 0xF0U, 0x9DU, 0x6AU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC68[] = {
	0x68U, 0x19U, 0xD4U, 0x26U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC69[] = {
	0x69U, 0xE0U, 0xEBU, 0x7BU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC6A[] = {
	0x6AU, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned char REC6B[] = {
	0x6BU, 0x02U, 0x02U, 0x0CU, 0x0BU, 0x12U, 0x64U, 0x00U, 0x00U, 0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
	0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned char REC6C[] = {
	0x6CU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC6D[] = {
	0x6DU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC6E[] = {
	0x6EU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC6F[] = {
	0x6FU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC70[] = {
	0x70U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC71[] = {
	0x71U, 0xACU, 0xB8U, 0xA4U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC72[] = {
	0x72U, 0x9BU, 0xDCU, 0x75U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x02U};

const unsigned char REC73[] = {
	0x73U, 0x00U, 0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

const unsigned char REC80[] = {
	0x80U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U};

#define P25_FRAME_PER       15U
#define YSF_FRAME_PER       90U

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "YSF2P25.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/YSF2P25.ini";
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
				::fprintf(stdout, "YSF2P25 version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: YSF2P25 [-v|--version] [filename]\n");
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

	CYSF2P25* gateway = new CYSF2P25(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CYSF2P25::CYSF2P25(const std::string& configFile) :
m_callsign(),
m_suffix(),
m_conf(configFile),
m_wiresX(NULL),
m_p25Network(NULL),
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
m_p25Frame(NULL),
m_dtmf(NULL),
m_p25Frames(0U),
m_ysfFrames(0U),
m_p25info(false)
{
	m_ysfFrame = new unsigned char[200U];
	m_p25Frame = new unsigned char[100U];

	::memset(m_ysfFrame, 0U, 200U);
	::memset(m_p25Frame, 0U, 100U);
}

CYSF2P25::~CYSF2P25()
{
	delete[] m_ysfFrame;
	delete[] m_p25Frame;
}

int CYSF2P25::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "YSF2P25: cannot read the .ini file\n");
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

		// If we are currently root...
		if (getuid() == 0) {
			struct passwd* user = ::getpwnam("mmdvm");
			if (user == NULL) {
				::fprintf(stderr, "Could not get the mmdvm user, exiting\n");
				return -1;
			}

			uid_t mmdvm_uid = user->pw_uid;
			gid_t mmdvm_gid = user->pw_gid;

			// Set user and group ID's to mmdvm:mmdvm
			if (setgid(mmdvm_gid) != 0) {
				::fprintf(stderr, "Could not set mmdvm GID, exiting\n");
				return -1;
			}

			if (setuid(mmdvm_uid) != 0) {
				::fprintf(stderr, "Could not set mmdvm UID, exiting\n");
				return -1;
			}

			// Double check it worked (AKA Paranoia) 
			if (setuid(0) != -1) {
				::fprintf(stderr, "It's possible to regain root - something is wrong!, exiting\n");
				return -1;
			}
		}
	}
#endif

	ret = ::LogInitialise(m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), logDisplayLevel);
	if (!ret) {
		::fprintf(stderr, "YSF2P25: unable to open the log file\n");
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
	m_defsrcid = m_conf.getP25Id();
	m_dstid = m_conf.getP25DstId();

	in_addr dstAddress       = CUDPSocket::lookup(m_conf.getDstAddress());
	unsigned int dstPort     = m_conf.getDstPort();
	std::string localAddress = m_conf.getLocalAddress();
	unsigned int localPort   = m_conf.getLocalPort();
	bool debug               = m_conf.getNetworkDebug();

	m_ysfNetwork = new CYSFNetwork(localAddress, localPort, m_callsign, debug);
	m_ysfNetwork->setDestination(dstAddress, dstPort);

	ret = m_ysfNetwork->open();
	if (!ret) {
		::LogError("Cannot open the YSF network port");
		::LogFinalise();
		return 1;
	}

	std::string p25_dstAddress   = m_conf.getP25DstAddress();
	unsigned int p25_dstPort     = m_conf.getP25DstPort();
	std::string p25_localAddress = m_conf.getP25LocalAddress();
	unsigned int p25_localPort   = m_conf.getP25LocalPort();
	bool p25_debug               = m_conf.getP25NetworkDebug();

	m_p25Network = new CP25Network(p25_localAddress, p25_localPort, p25_dstAddress, p25_dstPort, m_callsign, p25_debug);

	ret = m_p25Network->open();
	if (!ret) {
		::LogError("Cannot open the P25 network port");
		::LogFinalise();
		return 1;
	}

	std::string lookupFile  = m_conf.getDMRIdLookupFile();
	unsigned int reloadTime = m_conf.getDMRIdLookupTime();

	m_lookup = new CDMRLookup(lookupFile, reloadTime);
	m_lookup->read();

	CTimer networkWatchdog(100U, 0U, 1500U);
	CTimer pollTimer(1000U, 5U);

	std::string suffix = m_conf.getSuffix();
	std::string name = m_conf.getDescription();
	bool enableWiresX = m_conf.getEnableWiresX();
	std::string TGList = m_conf.getP25TGListFile();
	unsigned int rxFrequency = m_conf.getRxFrequency();
	unsigned int txFrequency = m_conf.getTxFrequency();

	// CWiresX Control Object
	if (enableWiresX) {
		bool makeUpper = m_conf.getWiresXMakeUpper();
		m_wiresX = new CWiresX(m_callsign, suffix, m_ysfNetwork, TGList, makeUpper);
		m_dtmf = new CDTMF;
	}

	if (m_wiresX != NULL)
		m_wiresX->setInfo(name, txFrequency, rxFrequency, m_dstid);

	CStopWatch stopWatch;
	CStopWatch ysfWatch;
	CStopWatch p25Watch;
	stopWatch.start();
	ysfWatch.start();
	p25Watch.start();
	pollTimer.start();

	unsigned char ysf_cnt = 0;
	unsigned char p25_cnt = 0;

	LogMessage("Starting YSF2P25-%s", VERSION);

	for (; end == 0;) {
		unsigned char buffer[2000U];
		unsigned int srcId = 0U;
		unsigned int dstId = 0U;

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
							sendP25PTT(m_srcid, m_dstid);
							break;

						case WXS_DX:
							if (!m_wiresX->getDstID()) {
								sendP25PTT(m_srcid, 10U);
								sendP25PTT(m_srcid, 9999U);
							} else {
								sendP25PTT(m_srcid, m_wiresX->getDstID());
							}
							break;

						case WXS_DISCONNECT:
							LogMessage("Disconnect has been requested by %s", m_ysfSrc.c_str());
							sendP25PTT(m_srcid, 9999U);
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
							sendP25PTT(m_srcid, m_dstid);
							break;

						case WXS_DISCONNECT:
							LogMessage("Disconnect via DTMF has been requested by %s", m_ysfSrc.c_str());
							sendP25PTT(m_srcid, 9999U);
							break;

						default:
							break;
					}
				}

				if ((::memcmp(buffer, "YSFD", 4U) == 0U) && (dt == YSF_DT_VOICE_FR_MODE)) {
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
			}
		}

		if (p25Watch.elapsed() > P25_FRAME_PER) {
			unsigned int p25FrameType = m_conv.getP25(m_p25Frame);

			if(p25FrameType == TAG_HEADER) {
				p25_cnt = 0U;
				p25Watch.start();
			}
			else if(p25FrameType == TAG_EOT) {
				m_p25Network->writeData(REC80, 17U);
				p25Watch.start();
			}
			else if(p25FrameType == TAG_DATA) {
				unsigned int p25step = p25_cnt % 18U;
				//CUtils::dump(1U, "P25 Data", m_p25Frame, 11U);

				if (p25_cnt > 2U) {
					switch (p25step) {
					case 0x00U:
						::memcpy(buffer, REC62, 22U);
						::memcpy(buffer + 10U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 22U);
						break;
					case 0x01U:
						::memcpy(buffer, REC63, 14U);
						::memcpy(buffer + 1U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 14U);
						break;
					case 0x02U:
						::memcpy(buffer, REC64, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						buffer[1U] = 0x00U;
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x03U:
						::memcpy(buffer, REC65, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						buffer[1U] = (m_dstid >> 16) & 0xFFU;
						buffer[2U] = (m_dstid >> 8) & 0xFFU;
						buffer[3U] = (m_dstid >> 0) & 0xFFU;
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x04U:
						::memcpy(buffer, REC66, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						buffer[1U] = (m_srcid >> 16) & 0xFFU;
						buffer[2U] = (m_srcid >> 8) & 0xFFU;
						buffer[3U] = (m_srcid >> 0) & 0xFFU;
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x05U:
						::memcpy(buffer, REC67, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x06U:
						::memcpy(buffer, REC68, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x07U:
						::memcpy(buffer, REC69, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x08U:
						::memcpy(buffer, REC6A, 16U);
						::memcpy(buffer + 4U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 16U);
						break;
					case 0x09U:
						::memcpy(buffer, REC6B, 22U);
						::memcpy(buffer + 10U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 22U);
						break;
					case 0x0AU:
						::memcpy(buffer, REC6C, 14U);
						::memcpy(buffer + 1U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 14U);
						break;
					case 0x0BU:
						::memcpy(buffer, REC6D, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x0CU:
						::memcpy(buffer, REC6E, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x0DU:
						::memcpy(buffer, REC6F, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x0EU:
						::memcpy(buffer, REC70, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						buffer[1U] = 0x80U;
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x0FU:
						::memcpy(buffer, REC71, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x10U:
						::memcpy(buffer, REC72, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x11U:
						::memcpy(buffer, REC73, 16U);
						::memcpy(buffer + 4U, m_p25Frame, 11U);
						m_p25Network->writeData(buffer, 16U);
						break;
					}
				}

				p25_cnt++;
				p25Watch.start();
			}
		}

		while (m_p25Network->readData(m_p25Frame, 22U) > 0U) {
			//CUtils::dump(1U, "P25 Data", m_p25Frame, 22U);
			if (m_p25Frame[0U] != 0xF0U && m_p25Frame[0U] != 0xF1U) {
				if (m_p25Frame[0U] == 0x62U && !m_p25info) {
					m_p25Frames = 0;
					m_conv.putP25Header();
				} else if (m_p25Frame[0U] == 0x65U && !m_p25info) {
					dstId  = (m_p25Frame[1U] << 16) & 0xFF0000U;
					dstId |= (m_p25Frame[2U] << 8)  & 0x00FF00U;
					dstId |= (m_p25Frame[3U] << 0)  & 0x0000FFU;
					m_netDst = m_lookup->findCS(dstId);
				} else if (m_p25Frame[0U] == 0x66U && !m_p25info) {
					srcId  = (m_p25Frame[1U] << 16) & 0xFF0000U;
					srcId |= (m_p25Frame[2U] << 8)  & 0x00FF00U;
					srcId |= (m_p25Frame[3U] << 0)  & 0x0000FFU;
					m_netSrc = m_lookup->findCS(srcId);
					LogMessage("Received P25 audio: Src: %s Dst: %s", m_netSrc.c_str(), m_netDst.c_str());
					m_p25info = true;
				} else if (m_p25Frame[0U] == 0x80U) {
					LogMessage("P25 received end of voice transmission, %.1f seconds", float(m_p25Frames) / 50.0F);
					m_p25info = false;
					m_conv.putP25EOT();
				}
				m_conv.putP25(m_p25Frame);
				m_p25Frames++;
			}
		}

		if (ysfWatch.elapsed() > YSF_FRAME_PER && m_p25Frames > 4U) {
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
				fich.setFI(YSF_FI_TERMINATOR);
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
				ysfWatch.start();
			}
			else if (ysfFrameType == TAG_DATA) {
				CYSFFICH fich;
				CYSFPayload ysfPayload;
				//unsigned char dch[10U];
				unsigned int fn = (ysf_cnt - 1U) % 7U;

				::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
				::memcpy(m_ysfFrame + 4U, m_ysfNetwork->getCallsign().c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 14U, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);

				// Add the YSF Sync
				CSync::addYSFSync(m_ysfFrame + 35U);

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

		if (m_wiresX != NULL)
			m_wiresX->clock(ms);

		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_ysfNetwork->writePoll();
			pollTimer.start();
		}

		if (ms < 5U)
			CThread::sleep(5U);
	}

	m_ysfNetwork->close();
	m_p25Network->close();

	delete m_p25Network;
	delete m_ysfNetwork;

	if (m_wiresX != NULL) {
		delete m_wiresX;
		delete m_dtmf;
	}

	::LogFinalise();

	return 0;
}

unsigned int CYSF2P25::findYSFID(std::string cs, bool showdst)
{
	std::string cstrim;

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
			LogMessage("Not P25 ID found, using default ID: %u, DstID: TG %u", id, m_dstid);
		else
			LogMessage("Not P25 ID found, using default ID: %u", id);
	}
	else {
		if (showdst)
			LogMessage("P25 ID of %s: %u, DstID: TG %u", cstrim.c_str(), id, m_dstid);
		else
			LogMessage("P25 ID of %s: %u", cstrim.c_str(), id);
	}

	return id;
}

std::string CYSF2P25::getSrcYSF(const unsigned char* buffer)
{
	unsigned char temp[YSF_CALLSIGN_LENGTH + 1U];

	::memcpy(temp, buffer + 14U, YSF_CALLSIGN_LENGTH);
	temp[YSF_CALLSIGN_LENGTH] = 0U;
	
	std::string trimmed = reinterpret_cast<char const*>(temp);
	trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), trimmed.end());
	
	return trimmed;
}

void CYSF2P25::sendP25PTT(unsigned int src, unsigned int dst)
{
	unsigned char buffer[20U];

	// Just send records 65 & 66 to activate linking in P25Gateway

	// Send record 65
	::memcpy(buffer, REC65, 17U);
	::memcpy(buffer + 5U, IMBE_SILENCE, 11U);
	buffer[1U] = (dst >> 16) & 0xFFU;
	buffer[2U] = (dst >> 8) & 0xFFU;
	buffer[3U] = (dst >> 0) & 0xFFU;
	m_p25Network->writeData(buffer, 17U);

	// Send record 66
	::memcpy(buffer, REC66, 17U);
	::memcpy(buffer + 5U, IMBE_SILENCE, 11U);
	buffer[1U] = (src >> 16) & 0xFFU;
	buffer[2U] = (src >> 8) & 0xFFU;
	buffer[3U] = (src >> 0) & 0xFFU;
	m_p25Network->writeData(buffer, 17U);

	// Send EOT
	m_p25Network->writeData(REC80, 17U);
}
