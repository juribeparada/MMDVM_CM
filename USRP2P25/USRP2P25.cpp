/*
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

#include "USRP2P25.h"
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>

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

#define USRP_FRAME_PER      15U
#define P25_FRAME_PER       15U

const char* DEFAULT_INI_FILE = "/etc/USRP2P25.ini";

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2021 by AD8DP";


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
				::fprintf(stdout, "USRP2P25 version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: USRP2P25 [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	// Capture SIGTERM to finish gracelessly
	if (signal(SIGTERM, sig_handler) == SIG_ERR) 
		::fprintf(stdout, "Can't catch SIGTERM\n");

	CUSRP2P25* gateway = new CUSRP2P25(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CUSRP2P25::CUSRP2P25(const std::string& configFile) :
m_callsign(),
m_conf(configFile),
m_usrpNetwork(NULL),
m_p25Network(NULL),
m_conv(),
m_dmrid(1U),
m_p25Src(1U),
m_p25Dst(1U),
m_p25Frame(NULL),
m_p25Frames(0U),
m_p25info(false),
m_usrpFrame(NULL),
m_usrpFrames(0U)
{
	m_p25Frame = new uint8_t[100U];
	m_usrpFrame  = new uint8_t[400U];

	::memset(m_p25Frame, 0U, 100U);
	::memset(m_usrpFrame, 0U, 400U);
}

CUSRP2P25::~CUSRP2P25()
{
	delete[] m_p25Frame;
	delete[] m_usrpFrame;
}

int CUSRP2P25::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "USRP2P25: cannot read the .ini file\n");
		return 1;
	}

	//setlocale(LC_ALL, "C");

	uint32_t logDisplayLevel = m_conf.getLogDisplayLevel();

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
		::fprintf(stderr, "USRP2P25: unable to open the log file\n");
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
	m_dmrid = m_conf.getDMRId();
	
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
	
	std::string usrp_address      = m_conf.getUSRPAddress();
	uint16_t usrp_dstPort     = m_conf.getUSRPDstPort();
	uint16_t usrp_localPort   = m_conf.getUSRPLocalPort();
	bool usrp_debug               = m_conf.getUSRPDebug();
	
	m_conv.setUSRPGainAdjDb(m_conf.getUSRPGainAdjDb());
	
	m_usrpNetwork = new CUSRPNetwork(usrp_address, usrp_dstPort, usrp_localPort, usrp_debug);
	
	ret = m_usrpNetwork->open();
	if (!ret) {
		::LogError("Cannot open the USRP network port");
		::LogFinalise();
		return 1;
	}

	CTimer networkWatchdog(100U, 0U, 1500U);
	CTimer pollTimer(1000U, 8U);
	CStopWatch stopWatch;
	CStopWatch p25Watch;
	CStopWatch usrpWatch;
	
	pollTimer.start();
	stopWatch.start();
	p25Watch.start();
	usrpWatch.start();

	uint16_t p25_cnt = 0;
	uint32_t usrp_cnt = 0;
	
	
	LogMessage("Starting USRP2P25-%s", VERSION);

	for (; m_killed == 0;) {
		uint8_t buffer[2000U];
		memset(buffer, 0, sizeof(buffer));
		
		uint32_t ms = stopWatch.elapsed();

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
						buffer[1U] = (m_p25Dst >> 16) & 0xFFU;
						buffer[2U] = (m_p25Dst >> 8) & 0xFFU;
						buffer[3U] = (m_p25Dst >> 0) & 0xFFU;
						m_p25Network->writeData(buffer, 17U);
						break;
					case 0x04U:
						::memcpy(buffer, REC66, 17U);
						::memcpy(buffer + 5U, m_p25Frame, 11U);
						buffer[1U] = (m_dmrid >> 16) & 0xFFU;
						buffer[2U] = (m_dmrid >> 8) & 0xFFU;
						buffer[3U] = (m_dmrid >> 0) & 0xFFU;
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
		
		if ( (usrpWatch.elapsed() > USRP_FRAME_PER) && (m_p25Frames > 4U) ) {
			int16_t pcm[160];
			uint32_t usrpFrameType = m_conv.getUSRP(pcm);
			
			if(usrpFrameType == TAG_USRP_HEADER){
				//CUtils::dump(1U, "USRP data:", m_usrpFrame, 33U);

				const uint32_t cnt = htonl(usrp_cnt);
				//const uint32_t cnt = 
				memset(m_usrpFrame, 0, 352);
				memcpy(m_usrpFrame, "USRP", 4);
				memcpy(m_usrpFrame+4, &cnt, 4);
				m_usrpFrame[20] = 2;
				//memcpy(m_usrpFrame+46, m_usrpcs.c_str(), m_usrpcs.size());
				
				m_usrpNetwork->writeData(m_usrpFrame, 352);
				usrp_cnt++;
				usrpWatch.start();
			}
			
			if(usrpFrameType == TAG_USRP_EOT){
				//CUtils::dump(1U, "USRP data:", m_usrpFrame, 33U);
				const uint32_t cnt = htonl(usrp_cnt);
				memcpy(m_usrpFrame, "USRP", 4);
				memset(m_usrpFrame+4, 0, 28);
				memcpy(m_usrpFrame+4, &cnt, 4);
				m_usrpFrame[15] = 0;
				
				m_usrpNetwork->writeData(m_usrpFrame, 32);
				usrp_cnt++;
				usrpWatch.start();
			}
			
			if(usrpFrameType == TAG_USRP_DATA){
				//CUtils::dump(1U, "USRP data:", m_usrpFrame, 33U);
				const uint32_t cnt = htonl(usrp_cnt);
				memcpy(m_usrpFrame, "USRP", 4);
				memset(m_usrpFrame+4, 0, 28);
				memcpy(m_usrpFrame+4, &cnt, 4);
				m_usrpFrame[15] = 1;
				
				for(int i = 0; i < 320; i+=2){
					m_usrpFrame[32+i] = pcm[(i/2)] & 0xff;
					m_usrpFrame[32+i+1] = pcm[(i/2)] >> 8;
				}
				
				m_usrpNetwork->writeData(m_usrpFrame, 352);
				usrp_cnt++;
				usrpWatch.start();
			}
		}
		uint32_t len = 0;
		while ( (len = m_usrpNetwork->readData(m_usrpFrame, 400)) ) {
			if((len == 32) && !memcmp(m_usrpFrame, "USRP", 4)) {
				LogMessage("USRP received end of voice transmission, %.1f seconds", float(m_usrpFrames) / 50.0F);
				m_conv.putUSRPEOT();
				m_usrpFrames = 0U;
			}

			if( (!memcmp(m_usrpFrame, "USRP", 4)) && len == 352) {
				if(!m_usrpFrames){	
					m_conv.putUSRPHeader();
					LogMessage("USRP first frame received");
				}
				int16_t pcm[160];
				for(int i = 0; i < 160; ++i){
					pcm[i] = (m_usrpFrame[32+(i*2)+1] << 8) | m_usrpFrame[32+(i*2)];
				}
				m_conv.putUSRP(pcm);
				m_usrpFrames++;
			}
		}

		stopWatch.start();
		pollTimer.clock(ms);
		
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_p25Network->writePoll();
			pollTimer.start();
		}

		if (ms < 5U) ::usleep(5U * 1000U);
	}

	m_p25Network->close();
	m_usrpNetwork->close();
	delete m_usrpNetwork;
	delete m_p25Network;

	::LogFinalise();

	return 0;
}
