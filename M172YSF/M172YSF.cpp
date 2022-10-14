/* 
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Andy Uribe CA6JAU
* 	Copyright (C) 2020 by Doug McLain AD8DP
* 	Copyright (C) 2022 by Dave Behnke AC8ZD
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

#include "M172YSF.h"
#include <thread>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>

#define YSF_FRAME_PER       90U
#define M17_FRAME_PER      35U
#define M17_PING_TIMEOUT    35000U

const char* DEFAULT_INI_FILE = "/etc/M172DMR.ini";

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2022 by AC8ZD, AD8DP, CA6JAU, G4KLX and others";

#define M17CHARACTERS " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-/."

#include <functional>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <cctype>

int end = 0;

void sig_handler(int signo)
{
	if (signo == SIGTERM) {
		end = 1;
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

//trim is necessary over usrp, especially USRP2M17, since people put wonky
//calls in their radio like AC8ZD/DAVE. By default, callsigns coming in from YSF
//are 10 characters and padded with spaces if callsign isn't that long.
//to make it extra M17 friendly, we ensure the callsign is no longer than 8 characters.
std::string trim_callsign(const std::string s) {
    const std::string ACCEPTABLECHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    size_t start = s.find_first_not_of(ACCEPTABLECHARS);
    if (start > 8) {
        start = 8;
    }
    return s.substr(0, start);
}

//pad is necessary for the reverse back to ysf, the callsign needs to be 10 characters
//spaces need to be placed if all 10 characters not used.
void pad_callsign(std::string &str, const size_t num, const char paddingChar = ' ')
{
    if(num > str.size())
        str.insert(str.size(), num - str.size(), paddingChar);
}

int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "M172YSF version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: M172YSF [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	// Capture SIGTERM to finish gracelessly
	if (signal(SIGTERM, sig_handler) == SIG_ERR) 
		::fprintf(stdout, "Can't catch SIGTERM\n");

	CM172YSF* gateway = new CM172YSF(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CM172YSF::CM172YSF(const std::string& configFile) :
m_callsign(),
m_m17Ref(),
m_conf(configFile),
m_conv(),
m_m17Frame(NULL),
m_m17Frames(0U)
{
	m_m17Frame = new unsigned char[100U];
	m_ysfFrame  = new unsigned char[200U];

	::memset(m_m17Frame, 0U, 100U);
	::memset(m_ysfFrame, 0U, 200U);
}

CM172YSF::~CM172YSF()
{
	delete[] m_m17Frame;
	delete[] m_ysfFrame;
}

int CM172YSF::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "M172YSF: cannot read the .ini file\n");
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
		::fprintf(stderr, "M172YSF: unable to open the log file\n");
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
	bool debug = m_conf.getDebug();
	
	char suffix = m_conf.getM17CallsignSuffix();
	m_m17Ref = m_conf.getM17DstName();
	char module = m_m17Ref.c_str()[m_m17Ref.find(' ')+1];
	std::string m17_dstAddress   = m_conf.getM17DstAddress();
	unsigned int m17_dstPort     = m_conf.getM17DstPort();
	std::string m17_localAddress = m_conf.getM17LocalAddress();
	unsigned int m17_localPort   = m_conf.getM17LocalPort();
	
	m_conv.setM17GainAdjDb(m_conf.getM17GainAdjDb());
	
	uint16_t streamid = 0;
	unsigned char m17_src[10];
	unsigned char m17_dst[10];
	
	memset(m17_src, ' ', 9);
	memcpy(m17_src, m_callsign.c_str(), m_callsign.size());
	m17_src[8] = suffix;
	m17_src[9] = 0x00;
	encode_callsign(m17_src);
	
	m_m17Network = new CM17Network(m17_localAddress, m17_localPort, m17_dstAddress, m17_dstPort, m17_src, debug);
	
	ret = m_m17Network->open();
	if (!ret) {
		::LogError("Cannot open the M17 network port");
		::LogFinalise();
		return 1;
	}
	
	in_addr ysf_dstAddress       = CUDPSocket::lookup(m_conf.getYSFDstAddress());
	unsigned int ysf_dstPort     = m_conf.getYSFDstPort();
	std::string ysf_localAddress = m_conf.getYSFLocalAddress();
	unsigned int ysf_localPort   = m_conf.getYSFLocalPort();

	m_ysfNetwork = new CYSFNetwork(ysf_localAddress, ysf_localPort, m_callsign, debug);
	m_ysfNetwork->setDestination(ysf_dstAddress, ysf_dstPort);

	ret = m_ysfNetwork->open();
	if (!ret) {
		::LogError("Cannot open the YSF network port");
		::LogFinalise();
		return 1;
	}
	
	CTimer networkWatchdog(100U, 0U, 1500U);
	CTimer pollTimer(1000U, 5U);

	CStopWatch stopWatch;
	CStopWatch m17Watch;
	CStopWatch m17PingWatch;
	CStopWatch ysfWatch;
	stopWatch.start();
	m17Watch.start();
	m17PingWatch.start();
	ysfWatch.start();
	pollTimer.start();

	unsigned char m17_cnt = 0;
	unsigned char ysf_cnt = 0;

	m_m17Network->writeLink(module);
	m_ysfNetwork->writePoll();
	 
	LogMessage("Starting M172YSF-%s", VERSION);

	for (; end == 0;) {
		unsigned char buffer[2000U];
		unsigned int ms = stopWatch.elapsed();
		
		if(m17PingWatch.elapsed() > M17_PING_TIMEOUT){
			LogMessage("M17 reflector stopped responding, sending CONN...");
			pollTimer.stop();
			m17PingWatch.start();
			m_m17Network->writeLink(module);
		}

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
					LogMessage("M17 received end of voice transmission, %.1f seconds", float(m_m17Frames) / 25.0F);
					m_conv.putM17EOT();
					m_m17cs.clear();
				}
				else{
					m_conv.putM17(m_m17Frame);
				}
				uint8_t cs[10];
				memcpy(cs, m_m17Frame+12, 6);
				decode_callsign(cs);
				std::string css((char *)cs);
				css = css.substr(0, css.find(' '));
				pad_callsign(css, 10, ' ');
				m_m17cs = css;
				m_m17Frames++;
			}
		}
		
		while (m_ysfNetwork->read(buffer) > 0U) {
			CYSFFICH fich;
			bool valid = fich.decode(buffer + 35U);

			if (valid) {
				unsigned char fi = fich.getFI();
				unsigned char dt = fich.getDT();
				//unsigned char fn = fich.getFN();
				//unsigned char ft = fich.getFT();

				if (::memcmp(buffer, "YSFD", 4U) == 0U) {
					//processWiresX(buffer + 35U, fi, dt, fn, ft);

					if (dt == YSF_DT_VD_MODE2) {
						CYSFPayload ysfPayload;

						if (fi == YSF_FI_HEADER) {
							if (ysfPayload.processHeaderData(buffer + 35U)) {
								std::string ysfSrcRaw = ysfPayload.getSource();
								std::string ysfSrc = trim_callsign(ysfSrcRaw);
								std::string ysfDst = ysfPayload.getDest();
								LogMessage("Received YSF Header: Raw Src: \"%s\" Src: \"%s\" Dst: \"%s\"", ysfSrcRaw.c_str(), ysfSrc.c_str(), ysfDst.c_str());
								m_conv.putYSFHeader();
								m_ysfcs = ysfSrc;
							}
						} else if (fi == YSF_FI_TERMINATOR) {
							LogMessage("YSF received end of voice transmission");
							m_conv.putYSFEOT();
						} else if (fi == YSF_FI_COMMUNICATIONS) {
							m_conv.putYSF(buffer + 35U);
						}
					}
				}
			}
		}

		if (m17Watch.elapsed() > M17_FRAME_PER) {
			uint32_t m17FrameType = m_conv.getM17(m_m17Frame);
			
			if( (m_ysfcs.size()) > 3 && (m_ysfcs.size() < 8) ){
				memset(m17_src, ' ', 9);
				memcpy(m17_src, m_ysfcs.c_str(), m_ysfcs.size());
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
				//CUtils::dump(1U, "M17 Data", m_p25Frame, 11U);
				m17_cnt++;
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
		
		if (ysfWatch.elapsed() > YSF_FRAME_PER) {
			unsigned int ysfFrameType = m_conv.getYSF(m_ysfFrame + 35U);

			//fprintf(stderr, "type:ms %d:%d\n", ysfFrameType, ysfWatch.elapsed());
			
			if(ysfFrameType == TAG_HEADER) {
				ysf_cnt = 0U;

				::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
				::memcpy(m_ysfFrame + 4U, m_callsign.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 14U, m_m17cs.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);
				m_ysfFrame[34U] = 0U; // Net frame counter

				::memcpy(m_ysfFrame + 35U, YSF_SYNC_BYTES, YSF_SYNC_LENGTH_BYTES);

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
				memset(csd1, '*', YSF_CALLSIGN_LENGTH);
 				memset(csd1, '*', YSF_CALLSIGN_LENGTH/2);
 				memcpy(csd1 + YSF_CALLSIGN_LENGTH/2, m_conf.getYsfRadioID().c_str(), YSF_CALLSIGN_LENGTH/2);
				memcpy(csd1 + YSF_CALLSIGN_LENGTH, m_m17cs.c_str(), YSF_CALLSIGN_LENGTH);
				memset(csd2, ' ', YSF_CALLSIGN_LENGTH + YSF_CALLSIGN_LENGTH);

				CYSFPayload payload;
				payload.writeHeader(m_ysfFrame + 35U, csd1, csd2);

				m_ysfNetwork->write(m_ysfFrame);

				ysf_cnt++;
				//ysfWatch.start();
			}
			else if (ysfFrameType == TAG_EOT) {

				::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
				::memcpy(m_ysfFrame + 4U, m_callsign.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 14U, m_m17cs.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);
				m_ysfFrame[34U] = ysf_cnt; // Net frame counter

				::memcpy(m_ysfFrame + 35U, YSF_SYNC_BYTES, YSF_SYNC_LENGTH_BYTES);

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
				memcpy(csd1 + YSF_CALLSIGN_LENGTH, m_m17cs.c_str(), YSF_CALLSIGN_LENGTH);
				memset(csd2, ' ', YSF_CALLSIGN_LENGTH + YSF_CALLSIGN_LENGTH);

				CYSFPayload payload;
				payload.writeHeader(m_ysfFrame + 35U, csd1, csd2);

				m_ysfNetwork->write(m_ysfFrame);
			}
			else if (ysfFrameType == TAG_DATA) {

				CYSFFICH fich;
				CYSFPayload ysfPayload;
				unsigned char dch[10U];

				unsigned int fn = (ysf_cnt - 1U) % (m_conf.getFICHFrameTotal() + 1);

				::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
				::memcpy(m_ysfFrame + 4U, m_callsign.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 14U, m_m17cs.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);

				::memcpy(m_ysfFrame + 35U, YSF_SYNC_BYTES, YSF_SYNC_LENGTH_BYTES);

				switch (fn) {
					case 0:
						memset(dch, '*', YSF_CALLSIGN_LENGTH/2);
 						memcpy(dch + YSF_CALLSIGN_LENGTH/2, m_conf.getYsfRadioID().c_str(), YSF_CALLSIGN_LENGTH/2);
 						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, dch);
						break;
					case 1:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (unsigned char*)m_m17cs.c_str());
						break;
					case 2:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (unsigned char*)m_m17cs.c_str());
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

				// Send data
				m_ysfNetwork->write(m_ysfFrame);

				ysf_cnt++;
				//ysfWatch.start();
			}
			ysfWatch.start();
		}

		stopWatch.start();
		m_ysfNetwork->clock(ms);
		pollTimer.clock(ms);
		
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_m17Network->writePoll();
			m_ysfNetwork->writePoll();
			pollTimer.start();
		}

		if (ms < 5U) ::usleep(5 * 1000);
	}

	m_m17Network->close();
	m_ysfNetwork->close();
	delete m_ysfNetwork;
	delete m_m17Network;

	::LogFinalise();

	return 0;
}

