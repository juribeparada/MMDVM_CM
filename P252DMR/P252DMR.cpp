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

#include "P252DMR.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

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
#define P25_FRAME_PER      15U

#define XLX_SLOT            2U
#define XLX_COLOR_CODE      3U

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "P252DMR.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/P252DMR.ini";
#endif

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2018 by AD8DP, CA6JAU, G4KLX and others";

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
				::fprintf(stdout, "P252DMR version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: P252DMR [-v|--version] [filename]\n");
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

	CP252DMR* gateway = new CP252DMR(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CP252DMR::CP252DMR(const std::string& configFile) :
m_callsign(),
m_conf(configFile),
m_dmrNetwork(NULL),
m_dmrlookup(NULL),
m_conv(),
m_colorcode(1U),
m_srcHS(1U),
m_defsrcid(1U),
m_dstid(1U),
m_dmrpc(false),
m_dmrSrc(1U),
m_dmrDst(1U),
m_dmrLastDT(0U),
m_p25Frame(NULL),
m_dmrFrame(NULL),
m_dmrFrames(0U),
m_p25Frames(0U),
m_EmbeddedLC(),
m_dmrflco(FLCO_GROUP),
m_dmrinfo(false),
m_xlxmodule(),
m_xlxConnected(false),
m_xlxReflectors(NULL),
m_xlxrefl(0U),
m_firstSync(false)
{
	m_p25Frame = new unsigned char[200U];
	m_dmrFrame  = new unsigned char[50U];

	::memset(m_p25Frame, 0U, 200U);
	::memset(m_dmrFrame, 0U, 50U);
}

CP252DMR::~CP252DMR()
{
	delete[] m_p25Frame;
	delete[] m_dmrFrame;
}

int CP252DMR::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "P252DMR: cannot read the .ini file\n");
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
		::fprintf(stderr, "P252DMR: unable to open the log file\n");
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

	std::string p25_dstAddress   = m_conf.getP25DstAddress();
	unsigned int p25_dstPort     = m_conf.getP25DstPort();
	std::string p25_localAddress = m_conf.getP25LocalAddress();
	unsigned int p25_localPort   = m_conf.getP25LocalPort();
	bool p25_debug               = m_conf.getP25NetworkDebug();
	
	std::string fileName    = m_conf.getDMRXLXFile();
	m_xlxReflectors = new CReflectors(fileName, 60U);
	m_xlxReflectors->load();

	m_p25Network = new CP25Network(p25_localAddress, p25_localPort, p25_dstAddress, p25_dstPort, m_callsign, p25_debug);
	

	ret = m_p25Network->open();
	if (!ret) {
		::LogError("Cannot open the P25 network port");
		::LogFinalise();
		return 1;
	}

	ret = createDMRNetwork();
	if (!ret) {
		::LogError("Cannot open DMR Network");
		::LogFinalise();
		return 1;
	}

	std::string lookupFile  = m_conf.getDMRIdLookupFile();
	unsigned int reloadTime = m_conf.getDMRIdLookupTime();

	m_dmrlookup = new CDMRLookup(lookupFile, reloadTime);
	m_dmrlookup->read();

	if (m_dmrpc)
		m_dmrflco = FLCO_USER_USER;
	else
		m_dmrflco = FLCO_GROUP;

	CTimer networkWatchdog(100U, 0U, 1500U);
	CTimer pollTimer(1000U, 5U);

	std::string name = m_conf.getDescription();

	CStopWatch stopWatch;
	CStopWatch p25Watch;
	CStopWatch dmrWatch;
	stopWatch.start();
	p25Watch.start();
	dmrWatch.start();
	pollTimer.start();

	unsigned char p25_cnt = 0;
	unsigned char dmr_cnt = 0;

	LogMessage("Starting P252DMR-%s", VERSION);

	for (; end == 0;) {
		unsigned char buffer[2000U];
		unsigned int srcId = 0U;
		unsigned int dstId = 0U;

		CDMRData tx_dmrdata;
		unsigned int ms = stopWatch.elapsed();

		if (m_dmrNetwork->isConnected() && !m_xlxmodule.empty() && !m_xlxConnected) {
			writeXLXLink(m_defsrcid, m_dstid, m_dmrNetwork);
			LogMessage("XLX, Linking to reflector XLX%03u, module %s", m_xlxrefl, m_xlxmodule.c_str());
			m_xlxConnected = true;
		}

		while (m_p25Network->readData(m_p25Frame, 22U) > 0U) {
			//CUtils::dump(1U, "P25 Data", m_p25Frame, 22U);
			if (m_p25Frame[0U] != 0xF0U && m_p25Frame[0U] != 0xF1U) {
				if (m_p25Frame[0U] == 0x62U && !m_p25info) {
					LogMessage("Received P25 Header");
					m_p25Frames = 0;
					m_conv.putP25Header();
				} else if (m_p25Frame[0U] == 0x65U && !m_p25info) {
					dstId  = (m_p25Frame[1U] << 16) & 0xFF0000U;
					dstId |= (m_p25Frame[2U] << 8)  & 0x00FF00U;
					dstId |= (m_p25Frame[3U] << 0)  & 0x0000FFU;
					if(m_p25Frames != 3){
						m_p25Frames = 3;
						m_conv.putP25Header();
					}
					m_p25Dst = dstId;
					
					if(!m_xlxConnected){
						if(m_p25Dst == 20U) {
							m_dstid = m_conf.getDMRDstId();
						}
						else{
							m_dstid = m_p25Dst;
						}
					}
					
				} else if (m_p25Frame[0U] == 0x66U && !m_p25info) {
					srcId  = (m_p25Frame[1U] << 16) & 0xFF0000U;
					srcId |= (m_p25Frame[2U] << 8)  & 0x00FF00U;
					srcId |= (m_p25Frame[3U] << 0)  & 0x0000FFU;
					m_p25Src = srcId;
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

			if(dmrFrameType == TAG_HEADER) {
				LogMessage("Sending DMR Header");
				CDMRData rx_dmrdata;
				dmr_cnt = 0U;
				m_dmrSrc = m_p25Src;
				//m_dmrSrc = m_defsrcid;

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
				LogMessage("Sending DMR EOT");
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
						//m_p25Frames = 0;
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
				LogMessage("Sending DMR Data");
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
			
			if(!m_dmrSrc){
				m_dmrSrc = m_srcHS;
			}
			
			FLCO netflco = tx_dmrdata.getFLCO();
			unsigned char DataType = tx_dmrdata.getDataType();

			if (!tx_dmrdata.isMissing()) {
				networkWatchdog.start();

				if(DataType == DT_TERMINATOR_WITH_LC) {
					if (m_dmrFrames == 0U) {
						m_dmrNetwork->reset(2U);
						networkWatchdog.stop();
						m_dmrinfo = false;
						m_firstSync = false;
						break;
					}

					LogMessage("DMR received end of voice transmission, %.1f seconds", float(m_dmrFrames) / 16.667F);

					m_conv.putDMREOT();
					m_dmrNetwork->reset(2U);
					networkWatchdog.stop();
					m_dmrFrames = 0U;
					m_dmrinfo = false;
					m_firstSync = false;
				}

				if((DataType == DT_VOICE_LC_HEADER) && (DataType != m_dmrLastDT)) {
					std::string netSrc = m_dmrlookup->findCS(m_dmrSrc);
					std::string netDst = (netflco == FLCO_GROUP ? "TG " : "") + m_dmrlookup->findCS(m_dmrDst);

					m_conv.putDMRHeader();
					LogMessage("DMR header received from %s to %s", netSrc.c_str(), netDst.c_str());

					m_dmrinfo = true;

					m_dmrFrames = 0U;
					m_firstSync = false;
				}

				if(DataType == DT_VOICE_SYNC)
					m_firstSync = true;

				if((DataType == DT_VOICE_SYNC || DataType == DT_VOICE) && m_firstSync) {
					unsigned char dmr_frame[50];
					tx_dmrdata.getData(dmr_frame);

					if (!m_dmrinfo) {
						std::string netSrc = m_dmrlookup->findCS(m_dmrSrc);
						std::string netDst = (netflco == FLCO_GROUP ? "TG " : "") + m_dmrlookup->findCS(m_dmrDst);

						m_conv.putDMRHeader();
						LogMessage("DMR late entry from %s to %s", netSrc.c_str(), netDst.c_str());

						m_dmrinfo = true;
					}

					m_conv.putDMR(dmr_frame); // Add DMR frame for P25 conversion
					m_dmrFrames++;
				}
			}
			else {
				if(DataType == DT_VOICE_SYNC || DataType == DT_VOICE) {
					unsigned char dmr_frame[50];
					tx_dmrdata.getData(dmr_frame);
					m_conv.putDMR(dmr_frame); // Add DMR frame for P25 conversion
					m_dmrFrames++;
				}

				networkWatchdog.clock(ms);
				if (networkWatchdog.hasExpired()) {
					LogDebug("Network watchdog has expired, %.1f seconds", float(m_dmrFrames) / 16.667F);
					m_dmrNetwork->reset(2U);
					networkWatchdog.stop();
					m_dmrFrames = 0U;
					m_dmrinfo = false;
				}
			}
			
			m_dmrLastDT = DataType;
		}

		if (p25Watch.elapsed() > P25_FRAME_PER) {
			unsigned int p25FrameType = m_conv.getP25(m_p25Frame);
			m_p25Src = m_dmrSrc;
			
			if(!m_xlxConnected){
				m_p25Dst = m_dmrDst;
			}
			
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
		stopWatch.start();
		
		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_p25Network->writePoll();
			pollTimer.start();
		}

		if(m_dmrNetwork->clock(ms)){
			m_xlxConnected = false;
		}

		if (m_xlxReflectors != NULL)
			m_xlxReflectors->clock(ms);

		if (ms < 2U) CThread::sleep(2U);
	}

	m_p25Network->close();
	m_dmrNetwork->close();
	delete m_dmrNetwork;
	delete m_p25Network;

	if (m_xlxReflectors != NULL)
		delete m_xlxReflectors;

	::LogFinalise();

	return 0;
}

bool CP252DMR::createDMRNetwork()
{
	std::string address   = m_conf.getDMRNetworkAddress();
	m_xlxmodule           = m_conf.getDMRXLXModule();
	m_xlxrefl             = m_conf.getDMRXLXReflector();
	unsigned int port     = m_conf.getDMRNetworkPort();
	unsigned int local    = m_conf.getDMRNetworkLocal();
	std::string password  = m_conf.getDMRNetworkPassword();
	bool debug            = m_conf.getDMRNetworkDebug();
	unsigned int jitter   = m_conf.getDMRNetworkJitter();
	bool slot1            = false;
	bool slot2            = true;
	bool duplex           = false;
	HW_TYPE hwType        = HWT_MMDVM;

	m_srcHS = m_conf.getDMRId();
	m_colorcode = 1U;

	if (m_xlxmodule.empty()) {
		m_dstid = m_conf.getDMRDstId();
		m_dmrpc = m_conf.getDMRPC();
	}
	else {
		const char *xlxmod = m_xlxmodule.c_str();
		m_dstid = 4000 + xlxmod[0] - 64;
		m_dmrpc = 0;

		CReflector* reflector = m_xlxReflectors->find(m_xlxrefl);
		if (reflector == NULL)
			return false;
		
		address = reflector->m_address;
	}

	if (m_srcHS > 99999999U)
		m_defsrcid = m_srcHS / 100U;
	else if (m_srcHS > 9999999U)
		m_defsrcid = m_srcHS / 10U;
	else
		m_defsrcid = m_srcHS;
	
	LogMessage("DMR Network Parameters");
	LogMessage("    ID: %u", m_srcHS);
	LogMessage("    Default SrcID: %u", m_defsrcid);
	if (!m_xlxmodule.empty()) {
		LogMessage("    XLX Reflector: %d", m_xlxrefl);
		LogMessage("    XLX Module: %s (%d)", m_xlxmodule.c_str(), m_dstid);
	}
	else {
		LogMessage("    Startup DstID: %s%u", m_dmrpc ? "" : "TG ", m_dstid);
		LogMessage("    Address: %s", address.c_str());
	}
	LogMessage("    Port: %u", port);
	if (local > 0U)
		LogMessage("    Local: %u", local);
	else
		LogMessage("    Local: random");
	LogMessage("    Jitter: %ums", jitter);

	m_dmrNetwork = new CDMRNetwork(address, port, local, m_srcHS, password, duplex, VERSION, debug, slot1, slot2, hwType, jitter);

	std::string options = m_conf.getDMRNetworkOptions();
	if (!options.empty()) {
		LogMessage("    Options: %s", options.c_str());
		m_dmrNetwork->setOptions(options);
	}

	unsigned int rxFrequency = m_conf.getRxFrequency();
	unsigned int txFrequency = m_conf.getTxFrequency();
	unsigned int power       = m_conf.getPower();
	float latitude           = m_conf.getLatitude();
	float longitude          = m_conf.getLongitude();
	int height               = m_conf.getHeight();
	std::string location     = m_conf.getLocation();
	std::string description  = m_conf.getDescription();
	std::string url          = m_conf.getURL();

	LogMessage("Info Parameters");
	LogMessage("    Callsign: %s", m_callsign.c_str());
	LogMessage("    RX Frequency: %uHz", rxFrequency);
	LogMessage("    TX Frequency: %uHz", txFrequency);
	LogMessage("    Power: %uW", power);
	LogMessage("    Latitude: %fdeg N", latitude);
	LogMessage("    Longitude: %fdeg E", longitude);
	LogMessage("    Height: %um", height);
	LogMessage("    Location: \"%s\"", location.c_str());
	LogMessage("    Description: \"%s\"", description.c_str());
	LogMessage("    URL: \"%s\"", url.c_str());

	m_dmrNetwork->setConfig(m_callsign, rxFrequency, txFrequency, power, m_colorcode, latitude, longitude, height, location, description, url);

	bool ret = m_dmrNetwork->open();
	if (!ret) {
		delete m_dmrNetwork;
		m_dmrNetwork = NULL;
		return false;
	}

	m_dmrNetwork->enable(true);

	return true;
}

void CP252DMR::writeXLXLink(unsigned int srcId, unsigned int dstId, CDMRNetwork* network)
{
	assert(network != NULL);

	unsigned int streamId = ::rand() + 1U;

	CDMRData data;

	data.setSlotNo(XLX_SLOT);
	data.setFLCO(FLCO_USER_USER);
	data.setSrcId(srcId);
	data.setDstId(dstId);
	data.setDataType(DT_VOICE_LC_HEADER);
	data.setN(0U);
	data.setStreamId(streamId);

	unsigned char buffer[DMR_FRAME_LENGTH_BYTES];

	CDMRLC lc;
	lc.setSrcId(srcId);
	lc.setDstId(dstId);
	lc.setFLCO(FLCO_USER_USER);

	CDMRFullLC fullLC;
	fullLC.encode(lc, buffer, DT_VOICE_LC_HEADER);

	CDMRSlotType slotType;
	slotType.setColorCode(XLX_COLOR_CODE);
	slotType.setDataType(DT_VOICE_LC_HEADER);
	slotType.getData(buffer);

	CSync::addDMRDataSync(buffer, true);

	data.setData(buffer);

	for (unsigned int i = 0U; i < 3U; i++) {
		data.setSeqNo(i);
		network->write(data);
	}

	data.setDataType(DT_TERMINATOR_WITH_LC);

	fullLC.encode(lc, buffer, DT_TERMINATOR_WITH_LC);

	slotType.setDataType(DT_TERMINATOR_WITH_LC);
	slotType.getData(buffer);

	data.setData(buffer);

	for (unsigned int i = 0U; i < 2U; i++) {
		data.setSeqNo(i + 3U);
		network->write(data);
	}
}

