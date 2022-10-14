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

#include "USRP2M17.h"
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>

#define USRP_FRAME_PER      15U
#define M17_FRAME_PER       35U
#define	M17_PING_TIMEOUT	35000U

const char* DEFAULT_INI_FILE = "/etc/USRP2M17.ini";

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2021 by AD8DP";

#define M17CHARACTERS " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/."

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

void encode_callsign(uint8_t *callsign)
{
	const std::string m17_alphabet(M17CHARACTERS);
	char cs[10];
	memset(cs, 0, sizeof(cs));
	memcpy(cs, callsign, strlen((char *)callsign));
	uint64_t encoded = 0;
	for(int i = std::strlen((char *)callsign)-1; i >= 0; i--) {
		auto pos = m17_alphabet.find(cs[i]);
		if (pos == std::string::npos) {
			pos = 0;
		}
		encoded *= 40;
		encoded += pos;
	}
	for (int i=0; i<6; i++) {
		callsign[i] = (encoded >> (8*(5-i)) & 0xFFU);
	}
}

void decode_callsign(uint8_t *callsign)
{
	const std::string m17_alphabet(M17CHARACTERS);
	uint8_t code[6];
	uint64_t coded = callsign[0];
	for (int i=1; i<6; i++)
		coded = (coded << 8) | callsign[i];
	if (coded > 0xee6b27ffffffu) {
		//std::cerr << "Callsign code is too large, 0x" << std::hex << coded << std::endl;
		return;
	}
	memcpy(code, callsign, 6);
	memset(callsign, 0, 10);
	int i = 0;
	while (coded) {
		callsign[i++] = m17_alphabet[coded % 40];
		coded /= 40;
	}
}

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "USRP2M17 version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: USRP2M17 [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	// Capture SIGTERM to finish gracelessly
	if (signal(SIGTERM, sig_handler) == SIG_ERR) 
		::fprintf(stdout, "Can't catch SIGTERM\n");

	CUSRP2M17* gateway = new CUSRP2M17(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CUSRP2M17::CUSRP2M17(const std::string& configFile) :
m_callsign(),
m_m17Ref(),
m_usrpcs(),
m_conf(configFile),
m_usrpNetwork(NULL),
m_m17Network(NULL),
m_conv(),
m_m17Src(),
m_m17Dst(),
m_m17Frame(NULL),
m_m17Frames(0U),
m_usrpFrame(NULL),
m_usrpFrames(0U)
{
	m_m17Frame = new uint8_t[100U];
	m_usrpFrame  = new uint8_t[400U];

	::memset(m_m17Frame, 0U, 100U);
	::memset(m_usrpFrame, 0U, 400U);
}

CUSRP2M17::~CUSRP2M17()
{
	delete[] m_m17Frame;
	delete[] m_usrpFrame;
}

int CUSRP2M17::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "USRP2M17: cannot read the .ini file\n");
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
		::fprintf(stderr, "USRP2M17: unable to open the log file\n");
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
	m_m17Ref = m_conf.getM17Name();
	char module = m_m17Ref.c_str()[m_m17Ref.find(' ')+1];

	std::string m17_address      = m_conf.getM17Address();
	uint16_t m17_dstPort     = m_conf.getM17DstPort();
	uint16_t m17_localPort   = m_conf.getM17LocalPort();
	bool m17_debug               = m_conf.getM17Debug();
	
	m_conv.setM17GainAdjDb(m_conf.getM17GainAdjDb());
	
	uint16_t streamid = 0;
	uint8_t m17_src[10];
	uint8_t m17_dst[10];
	
	memcpy(m17_src, m_callsign.c_str(), 9);
	m17_src[9] = 0x00;
	encode_callsign(m17_src);
	
	m_m17Network = new CM17Network(m17_address, m17_dstPort, m17_localPort, m17_src, m17_debug);
	
	ret = m_m17Network->open();
	if (!ret) {
		::LogError("Cannot open the M17 network port");
		::LogFinalise();
		return 1;
	}
	
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
	CStopWatch m17Watch;
	CStopWatch m17PingWatch;
	CStopWatch usrpWatch;
	
	pollTimer.start();
	stopWatch.start();
	m17Watch.start();
	m17PingWatch.start();
	usrpWatch.start();

	uint16_t m17_cnt = 0;
	uint32_t usrp_cnt = 0;
	
	m_m17Network->writeLink(module);
	
	LogMessage("Starting USRP2M17-%s", VERSION);

	for (; m_killed == 0;) {
		uint8_t buffer[2000U];
		memset(buffer, 0, sizeof(buffer));
		
		uint32_t ms = stopWatch.elapsed();
		
		if(m17PingWatch.elapsed() > M17_PING_TIMEOUT){
			LogMessage("M17 reflector stopped responding, sending CONN...");
			pollTimer.stop();
			m17PingWatch.start();
			m_m17Network->writeLink(module);
		}

		if (m17Watch.elapsed() > M17_FRAME_PER) {
			uint32_t m17FrameType = m_conv.getM17(m_m17Frame);
			
			if( (m_usrpcs.size()) > 3 && (m_usrpcs.size() < 8) ){
				memset(m17_src, ' ', 9);
				memcpy(m17_src, m_usrpcs.c_str(), m_usrpcs.size());
				m17_src[8] = 'D';
				m17_src[9] = 0x00;
				encode_callsign(m17_src);
			}
			else{
				memcpy(m17_src, m_callsign.c_str(), 9);
				m17_src[9] = 0x00;
				encode_callsign(m17_src);
			}
			
			if(m17FrameType == TAG_HEADER) {
				m17_cnt = 0U;
				m17Watch.start();
				
				streamid = static_cast<uint16_t>((::rand() & 0xFFFF));
				memcpy(m17_dst, m_m17Ref.c_str(), m_m17Ref.size());
				m17_dst[9] = 0x00;
				encode_callsign(m17_dst);
				
				memcpy(buffer, "M17 ", 4);
				memcpy(buffer+4, &streamid, 2);
				memcpy(buffer+6, m17_dst, 6);
				memcpy(buffer+12, m17_src, 6);
				buffer[19] = 0x05;
				memcpy(buffer+36, m_m17Frame, 16);
				m_m17Network->writeData(buffer, 54U);
			}
			else if(m17FrameType == TAG_EOT) {
				m17_cnt |= 0x8000;
				memcpy(buffer, "M17 ", 4);
				memcpy(buffer+4, &streamid, 2);
				memcpy(buffer+6, m17_dst, 6);
				memcpy(buffer+12, m17_src, 6);
				buffer[19] = 0x05;
				buffer[34] = m17_cnt >> 8;
				buffer[35] = m17_cnt & 0xff;
				memcpy(buffer+36, m_m17Frame, 16);
				m_m17Network->writeData(buffer, 54U);
				m17Watch.start();
			}
			else if(m17FrameType == TAG_DATA) {
				m17_cnt++;
				// Roll counter before it sets the EOT bit
				if (m17_cnt > 0x7ff) {
					m17_cnt = 1;
				}
				memcpy(buffer, "M17 ", 4);
				memcpy(buffer+4, &streamid, 2);
				memcpy(buffer+6, m17_dst, 6);
				memcpy(buffer+12, m17_src, 6);
				buffer[19] = 0x05;
				buffer[34] = m17_cnt >> 8;
				buffer[35] = m17_cnt & 0xff;
				memcpy(buffer+36, m_m17Frame, 16);
				m_m17Network->writeData(buffer, 54U);
				m17Watch.start();
			}
		}
		uint32_t len = 0;
		while (m_m17Network->readData(m_m17Frame, 54U) > 0U) {
			if (!memcmp(m_m17Frame, "PING", 4)) {
				m17PingWatch.start();
			}
			if (!memcmp(m_m17Frame, "ACKN", 4)) {
				LogMessage("Received ACKN from reflector");
				if(!pollTimer.isRunning()){
					pollTimer.start();
				}
				m17PingWatch.start();
			}
			if (!memcmp(m_m17Frame, "NACK", 4)) {
				LogMessage("Received NACK from reflector");
				pollTimer.stop();
				m17PingWatch.start();
			}
			
			if (!memcmp(m_m17Frame, "M17 ", 4)) {
				if (m_m17Frame[34] == 0 && m_m17Frame[35] == 0) {
					m_m17Frames = 0;
					m_conv.putM17Header();
				} 
				else if (m_m17Frame[34U] & 0x80U) {
					m_conv.putM17EOT();
					LogMessage("M17 received end of voice transmission, %.1f seconds", float(m_m17Frames) / 25.0F);
				}
				else{
					m_conv.putM17(m_m17Frame);
				}
				uint8_t cs[10];
				memcpy(cs, m_m17Frame+12, 6);
				decode_callsign(cs);
				std::string css((char *)cs);
				m_usrpcs = css.substr(0, css.find(' '));
				 
				m_m17Frames++;
			}
		}

		if (usrpWatch.elapsed() > USRP_FRAME_PER) {
			int16_t pcm[160];
			uint32_t usrpFrameType = m_conv.getUSRP(pcm);
			
			if(usrpFrameType == TAG_USRP_HEADER){
				//CUtils::dump(1U, "USRP data:", m_usrpFrame, 33U);

				const uint32_t cnt = htonl(usrp_cnt);
				memset(m_usrpFrame, 0, 352);
				memcpy(m_usrpFrame, "USRP", 4);
				memcpy(m_usrpFrame+4, &cnt, 4);
				m_usrpFrame[15] = USRP_KEYUP_FALSE;
				m_usrpFrame[20] = USRP_TYPE_TEXT;
				m_usrpFrame[32] = TLV_TAG_SET_INFO;
				m_usrpFrame[33] = 13 + m_usrpcs.size();
				
				memcpy(m_usrpFrame+46, m_usrpcs.c_str(), m_usrpcs.size());
				
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
				m_usrpFrame[15] = USRP_KEYUP_FALSE;
				
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
				m_usrpFrame[15] = USRP_KEYUP_TRUE;
				
				for(int i = 0; i < 320; i+=2){
					m_usrpFrame[32+i] = pcm[(i/2)] & 0xff;
					m_usrpFrame[32+i+1] = pcm[(i/2)] >> 8;
				}
				
				m_usrpNetwork->writeData(m_usrpFrame, 352);
				usrp_cnt++;
				usrpWatch.start();
			}
		}
		len = 0;
		while ( (len = m_usrpNetwork->readData(m_usrpFrame, 400)) ) {
			if(!memcmp(m_usrpFrame, "USRP", 4) && (len == 32)) {
				LogMessage("USRP received end of voice transmission, %.1f seconds", float(m_usrpFrames) / 50.0F);
				m_conv.putUSRPEOT();
				m_usrpcs.clear();
				m_usrpFrames = 0U;
			}

			if( (!memcmp(m_usrpFrame, "USRP", 4)) && len == 352) {
				if( (m_usrpFrame[20] == USRP_TYPE_TEXT) && (m_usrpFrame[32] == TLV_TAG_SET_INFO) ){
					m_usrpcs = (char *)(m_usrpFrame + 46);
					
					if(!m_usrpFrames){	
						m_conv.putUSRPHeader();
						LogMessage("USRP text info received first frame, callsign=%s (%d bytes)", m_usrpcs.c_str(), m_usrpcs.size());
					}
					m_usrpFrames++;
				}
				else if( (m_usrpFrame[20] == USRP_TYPE_VOICE) && (m_usrpFrame[15] == USRP_KEYUP_TRUE) ){
					if(!m_usrpFrames){
						m_usrpcs.clear();
						m_conv.putUSRPHeader();
						LogMessage("USRP voice received as first frame");
					}
					int16_t pcm[160];
					for(int i = 0; i < 160; ++i){
						pcm[i] = (m_usrpFrame[32+(i*2)+1] << 8) | m_usrpFrame[32+(i*2)];
					}
					m_conv.putUSRP(pcm);
					m_usrpFrames++;
				}
			}
		}

		stopWatch.start();
		pollTimer.clock(ms);
		
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_m17Network->writePoll();
			pollTimer.start();
		}

		if (ms < 5U) ::usleep(5U * 1000U);
	}

	m_m17Network->close();
	m_usrpNetwork->close();
	delete m_usrpNetwork;
	delete m_m17Network;

	::LogFinalise();

	return 0;
}
