/*
*   Copyright (C) 2016,2017 by Jonathan Naylor G4KLX
*   Copyright (C) 2018 by Andy Uribe CA6JAU
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

#include "DMR2YSF.h"

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

const unsigned char CONN_RESP[] = {0x5DU, 0x41U, 0x5FU, 0x26U};

#define DMR_FRAME_PER       55U
#define YSF_FRAME_PER       90U

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "DMR2YSF.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/DMR2YSF.ini";
#endif

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2018 by CA6JAU, G4KLX, AD8DP and others";

#include <functional>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <cctype>

static bool m_killed = false;

#if !defined(_WIN32) && !defined(_WIN64)
void sig_handler(int signo)
{
	if (signo == SIGTERM) {
		m_killed = true;
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
				::fprintf(stdout, "DMR2YSF version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: DMR2YSF [-v|--version] [filename]\n");
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

	CDMR2YSF* gateway = new CDMR2YSF(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CDMR2YSF::CDMR2YSF(const std::string& configFile) :
m_callsign(),
m_conf(configFile),
m_dmrNetwork(NULL),
m_ysfNetwork(NULL),
m_conv(),
m_colorcode(1U),
m_srcid(1U),
m_defsrcid(1U),
m_dstid(1U),
m_dmrpc(false),
m_netSrc(),
m_netDst(),
m_ysfSrc(),
m_dmrLastDT(0U),
m_ysfFrame(NULL),
m_dmrFrame(NULL),
m_dmrFrames(0U),
m_ysfFrames(0U),
m_EmbeddedLC(),
m_dmrflco(FLCO_GROUP),
m_dmrinfo(false),
m_config(NULL),
m_configLen(0U),
m_command(NULL),
m_tgUnlink(4000U),
m_currTGList(),
m_FCSList(),
m_lastTG(0U)
{
	m_ysfFrame = new unsigned char[200U];
	m_dmrFrame = new unsigned char[50U];
	m_config   = new unsigned char[400U];
	m_command  = new unsigned char[300U];

	::memset(m_ysfFrame, 0U, 200U);
	::memset(m_dmrFrame, 0U, 50U);
}

CDMR2YSF::~CDMR2YSF()
{
	delete[] m_ysfFrame;
	delete[] m_dmrFrame;
	delete[] m_config;
	delete[] m_command;
}

int CDMR2YSF::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "DMR2YSF: cannot read the .ini file\n");
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
		::fprintf(stderr, "DMR2YSF: unable to open the log file\n");
		return 1;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	if (m_daemon) {
		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);
	}
#endif

	m_callsign = m_conf.getCallsign();
	m_defsrcid = m_conf.getDMRId();
	m_dstid = m_conf.getDMRDefaultDstTG();
	m_tgUnlink = m_conf.getDMRNetworkTGUnlink();
	std::string tgFile = m_conf.getDMRTGListFile();
	std::string fcsFile = m_conf.getFCSFile();

	LogInfo(HEADER1);
	LogInfo(HEADER2);
	LogInfo(HEADER3);
	LogInfo(HEADER4);

	LogInfo("General Parameters");
	LogInfo("    Default Dst TG: %u", m_dstid);
	LogInfo("    Unlink TG: %u", m_tgUnlink);
	LogInfo("    FCS Rooms File: %s", fcsFile.c_str());
	LogInfo("    TG File: %s", tgFile.c_str());

	readFCSRoomsFile(fcsFile);
	readTGList(tgFile);

	in_addr dstAddress       = CUDPSocket::lookup(m_conf.getDstAddress());
	unsigned int dstPort     = m_conf.getDstPort();
	std::string localAddress = m_conf.getLocalAddress();
	unsigned int localPort   = m_conf.getLocalPort();
	unsigned int ysfdebug    = m_conf.getDebug();

	m_ysfNetwork = new CYSFNetwork(localAddress, localPort, m_callsign, ysfdebug);
	m_ysfNetwork->setDestination(dstAddress, dstPort);

	ret = m_ysfNetwork->open();
	if (!ret) {
		::LogError("Cannot open the YSF network port");
		::LogFinalise();
		return 1;
	}

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

	m_lookup = new CDMRLookup(lookupFile, reloadTime);
	m_lookup->read();

	if (m_dmrpc)
		m_dmrflco = FLCO_USER_USER;
	else
		m_dmrflco = FLCO_GROUP;

	CTimer networkWatchdog(100U, 0U, 1500U);
	CTimer pollTimer(1000U, 5U);

	CStopWatch stopWatch;
	CStopWatch ysfWatch;
	CStopWatch dmrWatch;
	stopWatch.start();
	ysfWatch.start();
	dmrWatch.start();
	pollTimer.start();

	unsigned char ysf_cnt = 0;
	unsigned char dmr_cnt = 0;

	unsigned char gps_buffer[20U];

	LogMessage("Starting DMR2YSF-%s", VERSION);

	for (; m_killed == 0;) {
		unsigned char buffer[2000U];

		CDMRData tx_dmrdata;
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
		}

		if (dmrWatch.elapsed() > DMR_FRAME_PER) {
			unsigned int dmrFrameType = m_conv.getDMR(m_dmrFrame);

			if(dmrFrameType == TAG_HEADER) {
				CDMRData rx_dmrdata;
				dmr_cnt = 0U;

				rx_dmrdata.setSlotNo(2U);
				rx_dmrdata.setSrcId(m_srcid);
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
				CDMRLC dmrLC = CDMRLC(m_dmrflco, m_srcid, m_dstid);
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
						rx_dmrdata.setSrcId(m_srcid);
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
				rx_dmrdata.setSrcId(m_srcid);
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
				CDMRLC dmrLC = CDMRLC(m_dmrflco, m_srcid, m_dstid);
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
				rx_dmrdata.setSrcId(m_srcid);
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
					CDMRLC dmrLC = CDMRLC(m_dmrflco, m_srcid, m_dstid);
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
			unsigned int SrcId = tx_dmrdata.getSrcId();
			unsigned int DstId = tx_dmrdata.getDstId();
			
			FLCO netflco = tx_dmrdata.getFLCO();
			unsigned char DataType = tx_dmrdata.getDataType();

			if (!tx_dmrdata.isMissing()) {
				networkWatchdog.start();

				if(DataType == DT_TERMINATOR_WITH_LC && m_dmrFrames > 0U) {
					LogMessage("DMR received end of voice transmission, %.1f seconds", float(m_dmrFrames) / 16.667F);

					networkWatchdog.stop();
					m_dmrFrames = 0U;
					m_dmrinfo = false;

					if (m_dstid != m_lastTG)
						break;

					m_conv.putDMREOT();
				}

				if((DataType == DT_VOICE_LC_HEADER) && (DataType != m_dmrLastDT)) {
					
					// DT1 & DT2 without GPS info
					::memcpy(gps_buffer, dt1_temp, 10U);
					::memcpy(gps_buffer + 10U, dt2_temp, 10U);

					m_netSrc = m_lookup->findCS(SrcId);
					m_dstid = DstId;

					m_netDst = (netflco == FLCO_GROUP ? "TG " : "") + m_lookup->findCS(DstId);

					connectYSF(m_dstid);

					LogMessage("DMR audio received from %s to %s", m_netSrc.c_str(), m_netDst.c_str());

					m_dmrinfo = true;
					m_dmrFrames = 0U;

					m_netSrc.resize(YSF_CALLSIGN_LENGTH, ' ');
					m_netDst.resize(YSF_CALLSIGN_LENGTH, ' ');

					if (m_dstid != m_lastTG) {
						m_dmrLastDT = DataType;
						break;
					}

					m_conv.putDMRHeader();
				}

				if(DataType == DT_VOICE_SYNC || DataType == DT_VOICE) {
					unsigned char dmr_frame[50];

					tx_dmrdata.getData(dmr_frame);

					if (!m_dmrinfo) {
						m_netSrc = m_lookup->findCS(SrcId);
						m_dstid = DstId;

						m_netDst = (netflco == FLCO_GROUP ? "TG " : "") + m_lookup->findCS(DstId);

						LogMessage("DMR audio received from %s to %s", m_netSrc.c_str(), m_netDst.c_str());

						m_netSrc.resize(YSF_CALLSIGN_LENGTH, ' ');
						m_netDst.resize(YSF_CALLSIGN_LENGTH, ' ');

						connectYSF(m_dstid);

						m_dmrinfo = true;
					}

					m_dmrFrames++;

					if (m_dstid != m_lastTG)
						break;

					m_conv.putDMR(dmr_frame); // Add DMR frame for YSF conversion
				}
			}
			else {
				if(DataType == DT_VOICE_SYNC || DataType == DT_VOICE) {
					unsigned char dmr_frame[50];
					tx_dmrdata.getData(dmr_frame);

					m_dmrFrames++;

					if (m_dstid != m_lastTG)
						break;

					m_conv.putDMR(dmr_frame); // Add DMR frame for YSF conversion
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
				memset(csd1, '*', YSF_CALLSIGN_LENGTH);
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
			}
			else if (ysfFrameType == TAG_DATA) {

				CYSFFICH fich;
				CYSFPayload ysfPayload;
				unsigned char dch[10U];

				unsigned int fn = (ysf_cnt - 1U) % (m_conf.getFICHFrameTotal() + 1);

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

				// Send data
				m_ysfNetwork->write(m_ysfFrame);

				ysf_cnt++;
				ysfWatch.start();
			}
		}

		stopWatch.start();

		m_ysfNetwork->clock(ms);
		m_dmrNetwork->clock(ms);

		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_ysfNetwork->writePoll();
			pollTimer.start();
		}

		if (ms < 5U)
			CThread::sleep(5U);
	}

	m_ysfNetwork->close();
	m_dmrNetwork->close();

	delete m_dmrNetwork;
	delete m_ysfNetwork;

	::LogFinalise();

	return 0;
}

void CDMR2YSF::readTGList(std::string filename)
{
	// Load file with TG List
	FILE* fp = ::fopen(filename.c_str(), "rt");

	unsigned int count = 0U;

	if (fp != NULL) {
		char buffer[100U];
		while (::fgets(buffer, 100U, fp) != NULL) {
			if (buffer[0U] == '#')
				continue;

			char* p1 = ::strtok(buffer, ";\r\n");
			char* p2 = ::strtok(NULL, ";\r\n");

			if (p1 != NULL && p2 != NULL) {
				CTGReg* tgreg = new CTGReg;

				tgreg->m_tg = atoi(p1);
				int ysf_id = atoi(p2);

				if (ysf_id) {
					tgreg->m_ysf = ysf_id;
				} else {
					for (std::vector<CFCSReg*>::iterator it = m_FCSList.begin(); it != m_FCSList.end(); ++it) {
						std::string fcsname = p2;
						if (fcsname == (*it)->m_fcs) {
							LogInfo("FCS: %d, %s", (*it)->m_id, (*it)->m_fcs.c_str());
							tgreg->m_ysf = (*it)->m_id;
						}
					}
				}

				m_currTGList.push_back(tgreg);

				count++;
			}
		}

		::fclose(fp);
	}

	LogInfo("Loaded %u DMR-TG / YSF-ID pairs", count);
}

void CDMR2YSF::readFCSRoomsFile(const std::string& filename)
{
	FILE* fp = ::fopen(filename.c_str(), "rt");
	if (fp == NULL)
		return;

	unsigned int count = 0U;

	char buffer[200U];
	while (::fgets(buffer, 200, fp) != NULL) {
		if (buffer[0U] == '#')
			continue;

		char* p1 = ::strtok(buffer, ";");

		if (p1 != NULL) {
			CFCSReg* fcsreg = new CFCSReg;

			fcsreg->m_id = count + 10U;
			fcsreg->m_fcs = p1;

			m_FCSList.push_back(fcsreg);

			count++;
		}
	}

	::fclose(fp);

	LogInfo("Loaded %u FCS room descriptions", count);
}

unsigned int CDMR2YSF::findYSFID(std::string cs, bool showdst)
{
	std::string cstrim;
	bool dmrpc = false;

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

	if (m_dmrflco == FLCO_USER_USER)
		dmrpc = true;
	else if (m_dmrflco == FLCO_GROUP)
		dmrpc = false;

	if (id == 0) {
		id = m_defsrcid;
		if (showdst)
			LogMessage("DMR ID not found, using default ID: %u, DstID: %s%u", id, dmrpc ? "" : "TG ", m_dstid);
		else
			LogMessage("DMR ID not found, using default ID: %u", id);
	}
	else {
		if (showdst)
			LogMessage("DMR ID of %s: %u, DstID: %s%u", cstrim.c_str(), id, dmrpc ? "" : "TG ", m_dstid);
		else
			LogMessage("DMR ID of %s: %u", cstrim.c_str(), id);
	}

	return id;
}

std::string CDMR2YSF::getSrcYSF(const unsigned char* buffer)
{
	unsigned char temp[YSF_CALLSIGN_LENGTH + 1U];

	::memcpy(temp, buffer + 14U, YSF_CALLSIGN_LENGTH);
	temp[YSF_CALLSIGN_LENGTH] = 0U;

	std::string trimmed = reinterpret_cast<char const*>(temp);
	trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), trimmed.end());

	return trimmed;
}

void CDMR2YSF::connectYSF(unsigned int id)
{
	if (id == m_lastTG)
		return;

	if (id == m_tgUnlink)
		sendYSFDisc();

	for (std::vector<CTGReg*>::iterator it = m_currTGList.begin(); it != m_currTGList.end(); ++it) {
		if (id == (*it)->m_tg) {
			sendYSFConn((*it)->m_ysf);
			return;
		}
	}

	LogMessage("DMR TG not found in TGList");

	return;
}

void CDMR2YSF::sendYSFConn(unsigned int id)
{
	unsigned char data[] = {0x00U, 0x5DU, 0x23U, 0x5FU, 0x24U, 0x20U, 0x20U, 0x20U, 0x20U,
							0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x03U, 0x00U, 0x00U,
							0x00U, 0x00U};

	if (id > 99999U)
		return;

	sprintf((char*) (data + 5U), "%05u", id);
	data[16U] = CCRC::addCRC(data, 16U);

	// Setup net header
	::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
	::memcpy(m_ysfFrame + 4U, m_ysfNetwork->getCallsign().c_str(), YSF_CALLSIGN_LENGTH);
	::memcpy(m_ysfFrame + 14U, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
	::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);
	m_ysfFrame[34U] = 0U;

	CSync::addYSFSync(m_ysfFrame + 35U);

	// Set the FICH
	CYSFFICH fich;
	fich.setFI(YSF_FI_COMMUNICATIONS);
	fich.setCS(2U);
	fich.setFN(1U);
	fich.setFT(1U);
	fich.setDev(0U);
	fich.setMR(2U);
	fich.setDT(YSF_DT_DATA_FR_MODE);
	fich.setSQL(0U);
	fich.setSQ(0U);
	fich.encode(m_ysfFrame + 35U);

	CYSFPayload payload;
	payload.writeDataFRModeData2(data, m_ysfFrame + 35U);

	m_ysfNetwork->write(m_ysfFrame);

	LogMessage("Sending YSF connect command to reflector ID: %u", id);
}

void CDMR2YSF::sendYSFDisc()
{
	unsigned char data[] = {0x00U, 0x5DU, 0x2AU, 0x5FU, 0x24U, 0x20U, 0x20U, 0x20U, 0x20U,
							0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x20U, 0x03U, 0x4DU, 0x00U,
							0x00U, 0x00U};

	// Setup net header
	::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
	::memcpy(m_ysfFrame + 4U, m_ysfNetwork->getCallsign().c_str(), YSF_CALLSIGN_LENGTH);
	::memcpy(m_ysfFrame + 14U, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
	::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);
	m_ysfFrame[34U] = 0U;

	CSync::addYSFSync(m_ysfFrame + 35U);

	// Set the FICH
	CYSFFICH fich;
	fich.setFI(YSF_FI_COMMUNICATIONS);
	fich.setCS(2U);
	fich.setFN(1U);
	fich.setFT(1U);
	fich.setDev(0U);
	fich.setMR(2U);
	fich.setDT(YSF_DT_DATA_FR_MODE);
	fich.setSQL(0U);
	fich.setSQ(0U);
	fich.encode(m_ysfFrame + 35U);

	CYSFPayload payload;
	payload.writeDataFRModeData2(data, m_ysfFrame + 35U);

	m_ysfNetwork->write(m_ysfFrame);

	LogMessage("Sending YSF disconnect command");
}

void CDMR2YSF::processWiresX(const unsigned char* data, unsigned char fi, unsigned char dt, unsigned char fn, unsigned char ft)
{
	assert(data != NULL);

	if (dt != YSF_DT_DATA_FR_MODE)
		return;

	if (fi != YSF_FI_COMMUNICATIONS)
		return;

	CYSFPayload payload;

	if (fn == 0U)
		return;

	if (fn == 1U) {
		bool valid = payload.readDataFRModeData2(data, m_command + 0U);
		if (!valid)
			return;
	} else {
		bool valid = payload.readDataFRModeData1(data, m_command + (fn - 2U) * 40U + 20U);
		if (!valid)
			return;

		valid = payload.readDataFRModeData2(data, m_command + (fn - 2U) * 40U + 40U);
		if (!valid)
			return;
	}

	if (fn == ft) {
		bool valid = false;
		// Find the end marker
		for (unsigned int i = (fn - 1U) * 40U + 20U; i > 0U; i--) {
			if (m_command[i] == 0x03U) {
				unsigned char crc = CCRC::addCRC(m_command, i + 1U);
				if (crc == m_command[i + 1U])
					valid = true;
				break;
			}
		}

		if (!valid)
			return;

		if (::memcmp(m_command + 1U, CONN_RESP, 4U) == 0) {
			m_lastTG = m_dstid;
			LogMessage("Reflector connected OK");
			return;
		}
	}

	return;
}

bool CDMR2YSF::createMMDVM()
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
