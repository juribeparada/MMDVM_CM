/*
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
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

#include "DMR2P25.h"

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>

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

#define DMR_FRAME_PER       55U
#define P25_FRAME_PER       15U

const char* DEFAULT_INI_FILE = "/etc/DMR2P25.ini";

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2018 by CA6JAU, G4KLX and others";

#include <functional>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <cctype>

static bool m_killed = false;

void sig_handler(int signo)
{
	if (signo == SIGTERM) {
		m_killed = true;
		::fprintf(stdout, "Received SIGTERM\n");
	}
}

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "DMR2P25 version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: DMR2P25 [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	// Capture SIGTERM to finish gracelessly
	if (signal(SIGTERM, sig_handler) == SIG_ERR) 
		::fprintf(stdout, "Can't catch SIGTERM\n");

	CDMR2P25* gateway = new CDMR2P25(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CDMR2P25::CDMR2P25(const std::string& configFile) :
m_callsign(),
m_conf(configFile),
m_dmrNetwork(NULL),
m_p25Network(NULL),
m_dmrlookup(NULL),
m_conv(),
m_colorcode(1U),
m_dstid(1U),
m_dmrSrc(1U),
m_dmrDst(1U),
m_dmrLastDT(0U),
m_dmrFrame(NULL),
m_dmrFrames(0U),
m_p25Src(1U),
m_p25Dst(1U),
m_p25Frame(NULL),
m_p25Frames(0U),
m_p25info(false),
m_EmbeddedLC(),
m_dmrflco(FLCO_GROUP),
m_dmrinfo(false),
m_config(NULL),
m_configLen(0U)
{
	m_p25Frame = new unsigned char[100U];
	m_dmrFrame  = new unsigned char[50U];
	m_config    = new unsigned char[400U];

	::memset(m_p25Frame, 0U, 100U);
	::memset(m_dmrFrame, 0U, 50U);
}

CDMR2P25::~CDMR2P25()
{
	delete[] m_p25Frame;
	delete[] m_dmrFrame;
	delete[] m_config;
}

int CDMR2P25::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "DMR2P25: cannot read the .ini file\n");
		return 1;
	}

	setlocale(LC_ALL, "C");

	unsigned int logDisplayLevel = m_conf.getLogDisplayLevel();

	if(m_conf.getDaemon())
		logDisplayLevel = 0U;

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

	ret = ::LogInitialise(m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), logDisplayLevel);
	if (!ret) {
		::fprintf(stderr, "DMR2P25: unable to open the log file\n");
		return 1;
	}

	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}

	LogInfo(HEADER1);
	LogInfo(HEADER2);
	LogInfo(HEADER3);
	LogInfo(HEADER4);
	
	m_callsign = m_conf.getCallsign();

	std::string p25_dstAddress   = m_conf.getP25DstAddress();
	unsigned int p25_dstPort     = m_conf.getP25DstPort();
	std::string p25_localAddress = m_conf.getP25LocalAddress();
	unsigned int p25_localPort   = m_conf.getP25LocalPort();
	bool p25_debug               = m_conf.getP25NetworkDebug();
	::fprintf(stderr, "%s : %s\n", p25_dstAddress.c_str(), p25_localAddress.c_str());
	 
	m_p25Network = new CP25Network(p25_localAddress, p25_localPort, p25_dstAddress, p25_dstPort, m_callsign, p25_debug);

	ret = m_p25Network->open();
	if (!ret) {
		::LogError("Cannot open the P25 network port");
		::LogFinalise();
		return 1;
	}
	m_p25Network->writePoll();
	ret = createMMDVM();
	if (!ret)
		return 1;

	LogMessage("Waiting for MMDVM to connect.....");

	while (!m_killed) {
		m_configLen = m_dmrNetwork->getConfig(m_config);
		if (m_configLen > 0U && m_dmrNetwork->getId() > 1000U)
			break;

		m_dmrNetwork->clock(10U);

		CThread::sleep(10U);
	}

	if (m_killed) {
		m_dmrNetwork->close();
		delete m_dmrNetwork;
		return 0;
	}

	LogMessage("MMDVM has connected");

	std::string lookupFile  = m_conf.getDMRIdLookupFile();
	unsigned int reloadTime = m_conf.getDMRIdLookupTime();

	m_dmrlookup = new CDMRLookup(lookupFile, reloadTime);
	m_dmrlookup->read();

	m_dmrflco = FLCO_GROUP;

	CTimer networkWatchdog(100U, 0U, 1500U);

	CStopWatch stopWatch;
	CStopWatch p25Watch;
	CStopWatch dmrWatch;
	stopWatch.start();
	p25Watch.start();
	dmrWatch.start();

	unsigned char p25_cnt = 0;
	unsigned char dmr_cnt = 0;

	LogMessage("Starting DMR2P25-%s", VERSION);

	for (; m_killed == 0;) {
		unsigned char buffer[2000U];

		CDMRData tx_dmrdata;
		unsigned int ms = stopWatch.elapsed();

		if (p25Watch.elapsed() > P25_FRAME_PER) {
			unsigned int p25FrameType = m_conv.getP25(m_p25Frame);
			m_p25Src = m_dmrSrc;
			m_p25Dst = m_dmrDst;

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
						buffer[1U] = (m_p25Dst >> 16) & 0xFFU;
						buffer[2U] = (m_p25Dst >> 8) & 0xFFU;
						buffer[3U] = (m_p25Dst >> 0) & 0xFFU;
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x04U:
						::memcpy(buffer, REC66, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						buffer[1U] = (m_p25Src >> 16) & 0xFFU;
						buffer[2U] = (m_p25Src >> 8) & 0xFFU;
						buffer[3U] = (m_p25Src >> 0) & 0xFFU;
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
					m_p25Dst  = (m_p25Frame[1U] << 16) & 0xFF0000U;
					m_p25Dst |= (m_p25Frame[2U] << 8)  & 0x00FF00U;
					m_p25Dst |= (m_p25Frame[3U] << 0)  & 0x0000FFU;
				} else if (m_p25Frame[0U] == 0x66U && !m_p25info) {
					m_p25Src  = (m_p25Frame[1U] << 16) & 0xFF0000U;
					m_p25Src |= (m_p25Frame[2U] << 8)  & 0x00FF00U;
					m_p25Src |= (m_p25Frame[3U] << 0)  & 0x0000FFU;
					LogMessage("Received P25 audio: Src: %d Dst: %d", m_p25Src, m_p25Dst);
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

		if (dmrWatch.elapsed() > DMR_FRAME_PER && m_p25Frames > 4U) {
			unsigned int dmrFrameType = m_conv.getDMR(m_dmrFrame);
			m_dmrSrc = m_p25Src;
			m_dstid = m_p25Dst;
			if(dmrFrameType == TAG_HEADER) {
				CDMRData rx_dmrdata;
				dmr_cnt = 0U;

				rx_dmrdata.setSlotNo(2U);
				rx_dmrdata.setSrcId(m_dmrSrc);
				rx_dmrdata.setDstId(m_dstid);
				rx_dmrdata.setFLCO(m_dmrflco);
				rx_dmrdata.setN(0U);
				rx_dmrdata.setSeqNo(0U);
				rx_dmrdata.setBER(0U);
				rx_dmrdata.setRSSI(0U);
				rx_dmrdata.setDataType(DT_VOICE_LC_HEADER);

				// Add sync
				CSync::addDMRDataSync(m_dmrFrame, 0);

				// Add SlotType
				CDMRSlotType slotType;
				slotType.setColorCode(m_colorcode);
				slotType.setDataType(DT_VOICE_LC_HEADER);
				slotType.getData(m_dmrFrame);

				// Full LC
				CDMRLC dmrLC = CDMRLC(m_dmrflco, m_dmrSrc, m_dstid);
				CDMRFullLC fullLC;
				fullLC.encode(dmrLC, m_dmrFrame, DT_VOICE_LC_HEADER);
				m_EmbeddedLC.setLC(dmrLC);
				
				rx_dmrdata.setData(m_dmrFrame);
				//CUtils::dump(1U, "DMR data:", m_dmrFrame, 33U);

				for (unsigned int i = 0U; i < 3U; i++) {
					rx_dmrdata.setSeqNo(dmr_cnt);
					m_dmrNetwork->write(rx_dmrdata);
					dmr_cnt++;
				}

				dmrWatch.start();
			}
			else if(dmrFrameType == TAG_EOT) {
				CDMRData rx_dmrdata;
				unsigned int n_dmr = (dmr_cnt - 3U) % 6U;
				unsigned int fill = (6U - n_dmr);
				
				if (n_dmr) {
					for (unsigned int i = 0U; i < fill; i++) {

						CDMREMB emb;
						CDMRData rx_dmrdata;

						rx_dmrdata.setSlotNo(2U);
						rx_dmrdata.setSrcId(m_dmrSrc);
						rx_dmrdata.setDstId(m_dstid);
						rx_dmrdata.setFLCO(m_dmrflco);
						rx_dmrdata.setN(n_dmr);
						rx_dmrdata.setSeqNo(dmr_cnt);
						rx_dmrdata.setBER(0U);
						rx_dmrdata.setRSSI(0U);
						rx_dmrdata.setDataType(DT_VOICE);

						::memcpy(m_dmrFrame, DMR_SILENCE_DATA, DMR_FRAME_LENGTH_BYTES);

						// Generate the Embedded LC
						unsigned char lcss = m_EmbeddedLC.getData(m_dmrFrame, n_dmr);

						// Generate the EMB
						emb.setColorCode(m_colorcode);
						emb.setLCSS(lcss);
						emb.getData(m_dmrFrame);

						rx_dmrdata.setData(m_dmrFrame);

						//CUtils::dump(1U, "DMR data:", m_dmrFrame, 33U);
						m_dmrNetwork->write(rx_dmrdata);

						n_dmr++;
						dmr_cnt++;
					}
				}

				rx_dmrdata.setSlotNo(2U);
				rx_dmrdata.setSrcId(m_dmrSrc);
				rx_dmrdata.setDstId(m_dstid);
				rx_dmrdata.setFLCO(m_dmrflco);
				rx_dmrdata.setN(n_dmr);
				rx_dmrdata.setSeqNo(dmr_cnt);
				rx_dmrdata.setBER(0U);
				rx_dmrdata.setRSSI(0U);
				rx_dmrdata.setDataType(DT_TERMINATOR_WITH_LC);

				// Add sync
				CSync::addDMRDataSync(m_dmrFrame, 0);

				// Add SlotType
				CDMRSlotType slotType;
				slotType.setColorCode(m_colorcode);
				slotType.setDataType(DT_TERMINATOR_WITH_LC);
				slotType.getData(m_dmrFrame);

				// Full LC
				CDMRLC dmrLC = CDMRLC(m_dmrflco, m_dmrSrc, m_dstid);
				CDMRFullLC fullLC;
				fullLC.encode(dmrLC, m_dmrFrame, DT_TERMINATOR_WITH_LC);

				rx_dmrdata.setData(m_dmrFrame);
				//CUtils::dump(1U, "DMR data:", m_dmrFrame, 33U);
				m_dmrNetwork->write(rx_dmrdata);

				dmrWatch.start();
			}
			else if(dmrFrameType == TAG_DATA) {
				CDMREMB emb;
				CDMRData rx_dmrdata;
				unsigned int n_dmr = (dmr_cnt - 3U) % 6U;

				rx_dmrdata.setSlotNo(2U);
				rx_dmrdata.setSrcId(m_dmrSrc);
				rx_dmrdata.setDstId(m_dstid);
				rx_dmrdata.setFLCO(m_dmrflco);
				rx_dmrdata.setN(n_dmr);
				rx_dmrdata.setSeqNo(dmr_cnt);
				rx_dmrdata.setBER(0U);
				rx_dmrdata.setRSSI(0U);
			
				if (!n_dmr) {
					rx_dmrdata.setDataType(DT_VOICE_SYNC);
					// Add sync
					CSync::addDMRAudioSync(m_dmrFrame, 0U);
					// Prepare Full LC data
					CDMRLC dmrLC = CDMRLC(m_dmrflco, m_dmrSrc, m_dstid);
					// Configure the Embedded LC
					m_EmbeddedLC.setLC(dmrLC);
				}
				else {
					rx_dmrdata.setDataType(DT_VOICE);
					// Generate the Embedded LC
					unsigned char lcss = m_EmbeddedLC.getData(m_dmrFrame, n_dmr);
					// Generate the EMB
					emb.setColorCode(m_colorcode);
					emb.setLCSS(lcss);
					emb.getData(m_dmrFrame);
				}

				rx_dmrdata.setData(m_dmrFrame);
				
				//CUtils::dump(1U, "DMR data:", m_dmrFrame, 33U);
				m_dmrNetwork->write(rx_dmrdata);

				dmr_cnt++;
				dmrWatch.start();
			}
		}

		while (m_dmrNetwork->read(tx_dmrdata) > 0U) {
			m_dmrSrc = tx_dmrdata.getSrcId();
			m_dmrDst = tx_dmrdata.getDstId();
			
			FLCO netflco = tx_dmrdata.getFLCO();
			unsigned char DataType = tx_dmrdata.getDataType();

			if (!tx_dmrdata.isMissing()) {
				networkWatchdog.start();

				if(DataType == DT_TERMINATOR_WITH_LC && m_dmrFrames > 0U) {
					LogMessage("DMR received end of voice transmission, %.1f seconds", float(m_dmrFrames) / 16.667F);

					m_conv.putDMREOT();
					networkWatchdog.stop();
					m_dmrFrames = 0U;
					m_dmrinfo = false;
				}

				if((DataType == DT_VOICE_LC_HEADER) && (DataType != m_dmrLastDT)) {
					std::string netSrc = m_dmrlookup->findCS(m_dmrSrc);
					std::string netDst = (netflco == FLCO_GROUP ? "TG " : "") + m_dmrlookup->findCS(m_dmrDst);

					m_conv.putDMRHeader();
					LogMessage("DMR header received from %s to %s", netSrc.c_str(), netDst.c_str());

					m_dmrinfo = true;

					m_dmrFrames = 0U;
				}

				if(DataType == DT_VOICE_SYNC || DataType == DT_VOICE) {
					unsigned char dmr_frame[50];
					tx_dmrdata.getData(dmr_frame);
					m_conv.putDMR(dmr_frame); // Add DMR frame for NXDN conversion
					m_dmrFrames++;
				}
			}
			else {
				if(DataType == DT_VOICE_SYNC || DataType == DT_VOICE) {
					unsigned char dmr_frame[50];
					tx_dmrdata.getData(dmr_frame);

					if (!m_dmrinfo) {
						std::string netSrc = m_dmrlookup->findCS(m_dmrSrc);
						std::string netDst = (netflco == FLCO_GROUP ? "TG " : "") + m_dmrlookup->findCS(m_dmrDst);

						m_conv.putDMRHeader();
						LogMessage("DMR late entry from %s to %s", netSrc.c_str(), netDst.c_str());

						m_dmrinfo = true;
					}

					m_conv.putDMR(dmr_frame); // Add DMR frame for NXDN conversion
					m_dmrFrames++;
				}

				networkWatchdog.clock(ms);
				if (networkWatchdog.hasExpired()) {
					LogDebug("Network watchdog has expired, %.1f seconds", float(m_dmrFrames) / 16.667F);
					networkWatchdog.stop();
					m_dmrFrames = 0U;
					m_dmrinfo = false;
				}
			}
			
			m_dmrLastDT = DataType;
		}

		stopWatch.start();

		m_dmrNetwork->clock(ms);
		
	}

	m_p25Network->close();
	m_dmrNetwork->close();
	delete m_dmrNetwork;
	delete m_p25Network;

	::LogFinalise();

	return 0;
}

bool CDMR2P25::createMMDVM()
{
	std::string rptAddress   = m_conf.getDMRRptAddress();
	unsigned int rptPort     = m_conf.getDMRRptPort();
	std::string localAddress = m_conf.getDMRLocalAddress();
	unsigned int localPort   = m_conf.getDMRLocalPort();
	bool debug               = m_conf.getDMRDebug();

	LogInfo("MMDVM Network Parameters");
	LogInfo("    Rpt Address: %s", rptAddress.c_str());
	LogInfo("    Rpt Port: %u", rptPort);
	LogInfo("    Local Address: %s", localAddress.c_str());
	LogInfo("    Local Port: %u", localPort);

	m_dmrNetwork = new CMMDVMNetwork(rptAddress, rptPort, localAddress, localPort, debug);

	bool ret = m_dmrNetwork->open();
	if (!ret) {
		delete m_dmrNetwork;
		m_dmrNetwork = NULL;
		return false;
	}

	return true;
}
