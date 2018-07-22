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

#define DMR_FRAME_PER       55U
#define YSF_FRAME_PER       90U

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "DMR2YSF.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/DMR2YSF.ini";
#endif

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
m_srcid(0U),
m_dstid(0U),
m_dmrLastDT(0U),
m_dmrFrames(0U),
m_ysfFrames(0U),
m_dmrinfo(false),
m_config(NULL),
m_configLen(0U)
{
	::memset(m_ysfFrame, 0U, 200U);
	::memset(m_dmrFrame, 0U, 50U);

	m_config = new unsigned char[400U];
}

CDMR2YSF::~CDMR2YSF()
{
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

		::close(STDIN_FILENO);
		::close(STDOUT_FILENO);
		::close(STDERR_FILENO);

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

	m_callsign = m_conf.getCallsign();
	m_defsrcid = m_conf.getDMRId();

	in_addr dstAddress       = CUDPSocket::lookup(m_conf.getDstAddress());
	unsigned int dstPort     = m_conf.getDstPort();
	std::string localAddress = m_conf.getLocalAddress();
	unsigned int localPort   = m_conf.getLocalPort();

	m_ysfNetwork = new CYSFNetwork(localAddress, localPort, m_callsign, false);
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

					m_conv.putDMREOT();
					networkWatchdog.stop();
					m_dmrFrames = 0U;
					m_dmrinfo = false;
				}

				if((DataType == DT_VOICE_LC_HEADER) && (DataType != m_dmrLastDT)) {
					
					// DT1 & DT2 without GPS info
					::memcpy(gps_buffer, dt1_temp, 10U);
					::memcpy(gps_buffer + 10U, dt2_temp, 10U);

					m_netSrc = m_lookup->findCS(SrcId);
					m_dstid = DstId;

					m_netDst = (netflco == FLCO_GROUP ? "TG " : "") + m_lookup->findCS(DstId);

					m_conv.putDMRHeader();
					LogMessage("DMR audio received from %s to %s", m_netSrc.c_str(), m_netDst.c_str());

					m_dmrinfo = true;

					m_netSrc.resize(YSF_CALLSIGN_LENGTH, ' ');
					m_netDst.resize(YSF_CALLSIGN_LENGTH, ' ');
					
					m_dmrFrames = 0U;
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

						m_dmrinfo = true;
					}

					m_conv.putDMR(dmr_frame); // Add DMR frame for YSF conversion
					m_dmrFrames++;
				}
			}
			else {
				if(DataType == DT_VOICE_SYNC || DataType == DT_VOICE) {
					unsigned char dmr_frame[50];
					tx_dmrdata.getData(dmr_frame);
					m_conv.putDMR(dmr_frame); // Add DMR frame for YSF conversion
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
				fich.setCS(2U);
				fich.setFN(0U);
				fich.setFT(7U);
				fich.setDev(0U);
				fich.setMR(2U);
				fich.setDT(YSF_DT_VD_MODE2);
				fich.setSQL(0U);
				fich.setSQ(0U);
				fich.encode(m_ysfFrame + 35U);

				unsigned char csd1[20U], csd2[20U];
				memset(csd1, '*', YSF_CALLSIGN_LENGTH);
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
				fich.setCS(2U);
				fich.setFN(0U);
				fich.setFT(7U);
				fich.setDev(0U);
				fich.setMR(2U);
				fich.setDT(YSF_DT_VD_MODE2);
				fich.setSQL(0U);
				fich.setSQ(0U);
				fich.encode(m_ysfFrame + 35U);

				unsigned char csd1[20U], csd2[20U];
				memset(csd1, '*', YSF_CALLSIGN_LENGTH);
				memcpy(csd1 + YSF_CALLSIGN_LENGTH, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
				memset(csd2, ' ', YSF_CALLSIGN_LENGTH + YSF_CALLSIGN_LENGTH);

				CYSFPayload payload;
				payload.writeHeader(m_ysfFrame + 35U, csd1, csd2);

				m_ysfNetwork->write(m_ysfFrame);
			}
			else if (ysfFrameType == TAG_DATA) {

				CYSFFICH fich;
				CYSFPayload ysfPayload;

				unsigned int fn = (ysf_cnt - 1U) % 8U;

				::memcpy(m_ysfFrame + 0U, "YSFD", 4U);
				::memcpy(m_ysfFrame + 4U, m_ysfNetwork->getCallsign().c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 14U, m_netSrc.c_str(), YSF_CALLSIGN_LENGTH);
				::memcpy(m_ysfFrame + 24U, "ALL       ", YSF_CALLSIGN_LENGTH);

				// Add the YSF Sync
				CSync::addYSFSync(m_ysfFrame + 35U);

				switch (fn) {
					case 0:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (const unsigned char*)"**********");
						break;
					case 1:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (const unsigned char*)m_netSrc.c_str());
						break;
					case 2:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (const unsigned char*)m_netDst.c_str());
						break;
					case 6:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, gps_buffer);
						break;
					case 7:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, gps_buffer+10U);
						break;
					default:
						ysfPayload.writeVDMode2Data(m_ysfFrame + 35U, (const unsigned char*)"          ");
				}
				
				// Set the FICH
				fich.setFI(YSF_FI_COMMUNICATIONS);
				fich.setCS(2U);
				fich.setFN(fn);
				fich.setFT(7U);
				fich.setDev(0U);
				fich.setMR(YSF_MR_BUSY);
				fich.setDT(YSF_DT_VD_MODE2);
				fich.setSQL(0U);
				fich.setSQ(0U);
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
			LogMessage("Not DMR ID found, using default ID: %u, DstID: %s%u", id, dmrpc ? "" : "TG ", m_dstid);
		else
			LogMessage("Not DMR ID found, using default ID: %u", id);
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
