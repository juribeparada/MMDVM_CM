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

#include "DSTAR2YSF.h"

#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <thread>
#include <pwd.h>

#define BUFSIZE 1024

#define DSTAR_FRAME_PER     15U
#define YSF_FRAME_PER       90U

const unsigned char CONN_RESP[] = {0x5DU, 0x41U, 0x5FU, 0x26U};
const char* DEFAULT_INI_FILE = "/etc/DSTAR2YSF.ini";

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

void sig_handler(int signo)
{
	if (signo == SIGTERM) {
		end = 1;
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
				::fprintf(stdout, "DSTAR2YSF version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: DSTAR2YSF [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	// Capture SIGTERM to finish gracelessly
	if (signal(SIGTERM, sig_handler) == SIG_ERR) 
		::fprintf(stdout, "Can't catch SIGTERM\n");

	CDSTAR2YSF* gateway = new CDSTAR2YSF(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CDSTAR2YSF::CDSTAR2YSF(const std::string& configFile) :
m_conf(configFile),
m_conv("/dev/ttyUSB0")
{
	m_dstarFrame = new unsigned char[200U];
	m_ysfFrame = new unsigned char[200U];
	::memset(m_dstarFrame, 0U, 200U);
	::memset(m_ysfFrame, 0U, 200U);
}

CDSTAR2YSF::~CDSTAR2YSF()
{
	delete[] m_dstarFrame;
}

int CDSTAR2YSF::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "DSTAR2YSF: cannot read the .ini file\n");
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

	ret = ::LogInitialise(m_conf.getLogFilePath(), m_conf.getLogFileRoot(), m_conf.getLogFileLevel(), logDisplayLevel);
	if (!ret) {
		::fprintf(stderr, "DSTAR2YSF: unable to open the log file\n");
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
	std::string mycall = m_conf.getMycall();
	std::string urcall = m_conf.getUrcall();
	std::string rptr1 = m_conf.getRptr1();
	std::string rptr2 = m_conf.getRptr2();
	std::string suffix = m_conf.getSuffix();
	
	char usertxt[20];
	::memset(usertxt, 0x20, 20);
	::memcpy(usertxt, m_conf.getUserTxt().c_str(), (m_conf.getUserTxt().size() <= 20) ? m_conf.getUserTxt().size() : 20);

	std::string dstar_dstAddress   = m_conf.getDSTARDstAddress();
	unsigned int dstar_dstPort     = m_conf.getDSTARDstPort();
	std::string dstar_localAddress = m_conf.getDSTARLocalAddress();
	unsigned int dstar_localPort   = m_conf.getDSTARLocalPort();
	bool dstar_debug               = m_conf.getDSTARNetworkDebug();

	m_dstarNetwork = new CDSTARNetwork(dstar_localAddress, dstar_localPort, dstar_dstAddress, dstar_dstPort, mycall, dstar_debug);
	
	ret = m_dstarNetwork->open();
	if (!ret) {
		::LogError("Cannot open the DSTAR network port");
		::LogFinalise();
		return 1;
	}
	
	in_addr dstAddress       = CUDPSocket::lookup(m_conf.getDstAddress());
	unsigned int dstPort     = m_conf.getDstPort();
	std::string localAddress = m_conf.getLocalAddress();
	unsigned int localPort   = m_conf.getLocalPort();
	unsigned int ysfdebug    = m_conf.getYSFDebug();

	m_ysfNetwork = new CYSFNetwork(localAddress, localPort, m_callsign, ysfdebug);
	m_ysfNetwork->setDestination(dstAddress, dstPort);

	ret = m_ysfNetwork->open();
	if (!ret) {
		::LogError("Cannot open the YSF network port");
		::LogFinalise();
		return 1;
	}

	CTimer pollTimer(1000U, 5U);
	
	CStopWatch stopWatch;
	CStopWatch dstarWatch;
	CStopWatch ysfWatch;
	stopWatch.start();
	dstarWatch.start();
	ysfWatch.start();
	pollTimer.start();

	unsigned char dstar_cnt = 0;
	unsigned char ysf_cnt = 0;

	LogMessage("Starting DSTAR2YSF-%s", VERSION);
	
	for (; end == 0;) {
		unsigned char buffer[2000U];
		unsigned char data[41U];
		int16_t pcm[160];
		memset(pcm, 0, sizeof(pcm));
		unsigned int ms = stopWatch.elapsed();
		
		while (m_ysfNetwork->read(buffer) > 0U) {
			CYSFFICH fich;
			bool valid = fich.decode(buffer + 35U);

			if (valid) {
				unsigned char fi = fich.getFI();
				unsigned char dt = fich.getDT();
				unsigned char fn = fich.getFN();
				unsigned char ft = fich.getFT();

				if (::memcmp(buffer, "YSFD", 4U) == 0U) {
					processWiresX(buffer + 35U, fi, dt, fn, ft);

					if (dt == YSF_DT_VD_MODE2) {
						CYSFPayload ysfPayload;

						if (fi == YSF_FI_HEADER) {
							if (ysfPayload.processHeaderData(buffer + 35U)) {
								std::string ysfSrc = ysfPayload.getSource();
								std::string ysfDst = ysfPayload.getDest();
								LogMessage("Received YSF Header: Src: %s Dst: %s", ysfSrc.c_str(), ysfDst.c_str());
								m_conv.putYSFHeader();
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
		if (dstarWatch.elapsed() > DSTAR_FRAME_PER) {
			unsigned int dstarFrameType = m_conv.getDSTAR(m_dstarFrame);
			
			fprintf(stderr, "type:ms %d:%d\n", dstarFrameType, dstarWatch.elapsed());
			
			if(dstarFrameType == TAG_HEADER) {
				data[0] = 0;
				data[1] = 0;
				data[2] = 0;
				
				::memcpy(data+3, rptr2.c_str(), rptr2.size());
				::memset(data+3+rptr2.size(), 0x20, 8-rptr2.size());
				
				::memcpy(data+11, rptr1.c_str(), rptr1.size());
				::memset(data+11+rptr1.size(), 0x20, 8-rptr1.size());
				
				::memcpy(data+19, urcall.c_str(), urcall.size());
				::memset(data+19+urcall.size(), 0x20, 8-urcall.size());
				
				::memcpy(data+27, mycall.c_str(), mycall.size());
				::memset(data+27+mycall.size(), 0x20, 8-mycall.size());
				
				::memcpy(data+35, suffix.c_str(), 4);
				
				data[39] = 0xea;
				data[40] = 0x7e;
				m_dstarNetwork->writeHeader(data, 41U);
				//m_dstarNetwork->writeHeader(data, 41U);
				dstar_cnt = 0U;
				//dstarWatch.start();
				
			}
			else if(dstarFrameType == TAG_EOT) {
				::memcpy(data, m_dstarFrame, 9);
				if(dstar_cnt % 0x15 == 0){
					data[9] = 0x55;
					data[10] = 0x2d;
					data[11] = 0x16;
				}
				else{
					data[9] = 0;
					data[10] = 0;
					data[11] = 0;
				}
				m_dstarNetwork->writeData(data, 12U, true);
				//dstarWatch.start();
			}
			else if(dstarFrameType == TAG_DATA) {
				::memcpy(data, m_dstarFrame, 9);
				switch(dstar_cnt){
				case 0:
					data[9]  = 0x55;
					data[10] = 0x2d;
					data[11] = 0x16;
					break;
				case 1:
					data[9]  = 0x40 ^ 0x70;
					data[10] = usertxt[0] ^ 0x4f;
					data[11] = usertxt[1] ^ 0x93;
					break;
				case 2:
					data[9]  = usertxt[2] ^ 0x70;
					data[10] = usertxt[3] ^ 0x4f;
					data[11] = usertxt[4] ^ 0x93;
					break;
				case 3:
					data[9]  = 0x41 ^ 0x70;
					data[10] = usertxt[5] ^ 0x4f;
					data[11] = usertxt[6] ^ 0x93;
					break;
				case 4:
					data[9]  = usertxt[7] ^ 0x70;
					data[10] = usertxt[8] ^ 0x4f;
					data[11] = usertxt[9] ^ 0x93;
					break;
				case 5:
					data[9]  = 0x42 ^ 0x70;
					data[10] = usertxt[10] ^ 0x4f;
					data[11] = usertxt[11] ^ 0x93;
					break;
				case 6:
					data[9]  = usertxt[12] ^ 0x70;
					data[10] = usertxt[13] ^ 0x4f;
					data[11] = usertxt[14] ^ 0x93;
					break;
				case 7:
					data[9]  = 0x43 ^ 0x70;
					data[10] = usertxt[15] ^ 0x4f;
					data[11] = usertxt[16] ^ 0x93;
					break;
				case 8:
					data[9]  = usertxt[17] ^ 0x70;
					data[10] = usertxt[18] ^ 0x4f;
					data[11] = usertxt[19] ^ 0x93;
					break;
				default:
					data[9]  = 0x16;
					data[10] = 0x29;
					data[11] = 0xf5;
					break;
				}
				
				m_dstarNetwork->writeData(data, 12U, false);
				//CUtils::dump(1U, "P25 Data", m_p25Frame, 11U);
				(dstar_cnt >= 0x14) ? dstar_cnt = 0 : ++dstar_cnt;
				//dstarWatch.start();
			}
			dstarWatch.start();
		}
		
		if (m_dstarNetwork->readData(m_dstarFrame, 49U) > 0U) {
			if(::memcmp("DSRP ", m_dstarFrame, 5) == 0){
				m_conv.putDSTARHeader();
			}
			if(::memcmp("DSRP!", m_dstarFrame, 5) == 0){
				m_conv.putDSTAR(m_dstarFrame + 9);
			}
			//CUtils::dump(1U, "DSTAR Data", m_dstarFrame, 49U);
		}
		
		if (ysfWatch.elapsed() > YSF_FRAME_PER) {
			unsigned int ysfFrameType = m_conv.getYSF(m_ysfFrame + 35U);

			//fprintf(stderr, "type:ms %d:%d\n", ysfFrameType, ysfWatch.elapsed());
			
			if(ysfFrameType == TAG_HEADER) {
				ysf_cnt = 0U;

				::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
				::memcpy(m_ysfFrame + 4U, m_callsign.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 14U, m_callsign.c_str(), YSF_CALLSIGN_LENGTH);
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
				memcpy(csd1 + YSF_CALLSIGN_LENGTH, m_callsign.c_str(), YSF_CALLSIGN_LENGTH);
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
				::memcpy(m_ysfFrame + 14U, m_callsign.c_str(), YSF_CALLSIGN_LENGTH);
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
				memcpy(csd1 + YSF_CALLSIGN_LENGTH, m_callsign.c_str(), YSF_CALLSIGN_LENGTH);
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
				::memcpy(m_ysfFrame + 14U, m_callsign.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);

				::memcpy(m_ysfFrame + 35U, YSF_SYNC_BYTES, YSF_SYNC_LENGTH_BYTES);

				switch (fn) {
					case 0:
						memset(dch, '*', YSF_CALLSIGN_LENGTH/2);
 						memcpy(dch + YSF_CALLSIGN_LENGTH/2, m_conf.getYsfRadioID().c_str(), YSF_CALLSIGN_LENGTH/2);
 						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, dch);
						break;
					case 1:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (unsigned char*)m_callsign.c_str());
						break;
					case 2:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (unsigned char*)m_callsign.c_str());
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
			//m_dstarNetwork->writePoll();
			pollTimer.start();
		}

		if (ms < 5U)
			::usleep(5 * 1000);
	}

	m_dstarNetwork->close();

	delete m_dstarNetwork;

	::LogFinalise();

	return 0;
}

void CDSTAR2YSF::processWiresX(const unsigned char* data, unsigned char fi, unsigned char dt, unsigned char fn, unsigned char ft)
{
	assert(data != NULL);
	unsigned char command[300U];

	if (dt != YSF_DT_DATA_FR_MODE)
		return;

	if (fi != YSF_FI_COMMUNICATIONS)
		return;

	CYSFPayload payload;

	if (fn == 0U)
		return;

	if (fn == 1U) {
		bool valid = payload.readDataFRModeData2(data, command + 0U);
		if (!valid)
			return;
	} else {
		bool valid = payload.readDataFRModeData1(data, command + (fn - 2U) * 40U + 20U);
		if (!valid)
			return;

		valid = payload.readDataFRModeData2(data, command + (fn - 2U) * 40U + 40U);
		if (!valid)
			return;
	}

	if (fn == ft) {
		bool valid = false;
		// Find the end marker
		for (unsigned int i = (fn - 1U) * 40U + 20U; i > 0U; i--) {
			if (command[i] == 0x03U) {
				unsigned char crc = CCRC::addCRC(command, i + 1U);
				if (crc == command[i + 1U])
					valid = true;
				break;
			}
		}

		if (!valid)
			return;

		if (::memcmp(command + 1U, CONN_RESP, 4U) == 0) {
			LogMessage("Reflector connected OK");
			return;
		}
	}

	return;
}

