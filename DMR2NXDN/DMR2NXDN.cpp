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

#include "DMR2NXDN.h"

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#endif

#define DMR_FRAME_PER       55U
#define NXDN_FRAME_PER      75U

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "DMR2NXDN.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/DMR2NXDN.ini";
#endif

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
				::fprintf(stdout, "DMR2NXDN version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: DMR2NXDN [-v|--version] [filename]\n");
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

	CDMR2NXDN* gateway = new CDMR2NXDN(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CDMR2NXDN::CDMR2NXDN(const std::string& configFile) :
m_nxdnTG(1U),
m_conf(configFile),
m_dmrNetwork(NULL),
m_nxdnNetwork(NULL),
m_dmrlookup(NULL),
m_nxdnlookup(NULL),
m_conv(),
m_colorcode(1U),
m_defsrcid(1U),
m_dstid(1U),
m_dmrSrc(1U),
m_dmrDst(1U),
m_nxdnSrc(1U),
m_nxdnDst(1U),
m_dmrLastDT(0U),
m_nxdnFrame(NULL),
m_dmrFrame(NULL),
m_dmrFrames(0U),
m_nxdnFrames(0U),
m_EmbeddedLC(),
m_dmrflco(FLCO_GROUP),
m_dmrinfo(false),
m_nxdninfo(false),
m_config(NULL),
m_configLen(0U),
m_defaultID(65519U)
{
	m_nxdnFrame = new unsigned char[200U];
	m_dmrFrame  = new unsigned char[50U];
	m_config    = new unsigned char[400U];

	::memset(m_nxdnFrame, 0U, 200U);
	::memset(m_dmrFrame, 0U, 50U);
}

CDMR2NXDN::~CDMR2NXDN()
{
	delete[] m_nxdnFrame;
	delete[] m_dmrFrame;
	delete[] m_config;
}

int CDMR2NXDN::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "DMR2NXDN: cannot read the .ini file\n");
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
		::fprintf(stderr, "DMR2NXDN: unable to open the log file\n");
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

	m_defsrcid = m_conf.getDMRId();

	std::string gatewayAddress = m_conf.getDstAddress();
	unsigned int gatewayPort   = m_conf.getDstPort();
	std::string localAddress   = m_conf.getLocalAddress();
	unsigned int localPort     = m_conf.getLocalPort();

	m_defaultID = m_conf.getDefaultID();

	m_nxdnNetwork = new CNXDNNetwork(localAddress, localPort, gatewayAddress, gatewayPort, false);
	m_nxdnNetwork->enable(true);

	ret = m_nxdnNetwork->open();
	if (!ret) {
		::LogError("Cannot open the NXDN network port");
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

	m_dmrlookup = new CDMRLookup(lookupFile, reloadTime);
	m_dmrlookup->read();

	lookupFile  = m_conf.getNXDNIdLookupFile();
	reloadTime = m_conf.getNXDNIdLookupTime();

	m_nxdnlookup = new CNXDNLookup(lookupFile, reloadTime);
	m_nxdnlookup->read();

	m_dmrflco = FLCO_GROUP;

	CTimer networkWatchdog(100U, 0U, 1500U);

	CStopWatch stopWatch;
	CStopWatch nxdnWatch;
	CStopWatch dmrWatch;
	stopWatch.start();
	nxdnWatch.start();
	dmrWatch.start();

	unsigned char nxdn_cnt = 0;
	unsigned char dmr_cnt = 0;

	LogMessage("Starting DMR2NXDN-%s", VERSION);

	for (; m_killed == 0;) {
		unsigned char buffer[2000U];

		CDMRData tx_dmrdata;
		unsigned int ms = stopWatch.elapsed();

		while (m_nxdnNetwork->read(buffer)) {
			CNXDNLICH lich;
			bool grp = true;
			bool end = false;

			if ((buffer[0U] == 0x81U || buffer[0U] == 0x83U) && (buffer[5U] == 0x01U || buffer[5U] == 0x08U)) {
				grp = (buffer[7U] & 0x20U) == 0x20U;
				end = (buffer[5U] & 0x08) == 0x08;

				m_nxdnSrc  = (buffer[8U] << 8) & 0xFF00U;
				m_nxdnSrc |= (buffer[9U] << 0) & 0x00FFU;

				m_nxdnDst  = (buffer[10U] << 8) & 0xFF00U;
				m_nxdnDst |= (buffer[11U] << 0) & 0x00FFU;

				// CUtils::dump(1U, "NXDN Header/EOT:", buffer, 33U);
			}

			lich.setRaw(buffer[0U]);
			unsigned char usc = lich.getFCT();
			unsigned char opt = lich.getOption();

			if (usc == NXDN_LICH_USC_SACCH_NS) {
				if (end) {
					LogMessage("NXDN received end of voice transmission, %.1f seconds", float(m_nxdnFrames) / 12.5F);
					m_conv.putNXDNEOT();
					m_nxdnFrames = 0U;
					m_nxdninfo = false;
				} else {
					std::string netSrc = m_nxdnlookup->findCS(m_nxdnSrc);
					std::string netDst = m_nxdnlookup->findCS(m_nxdnDst);
					LogMessage("Received NXDN header from %s to %s%s", netSrc.c_str(), grp ? "TG " : "", netDst.c_str());

					m_conv.putNXDNHeader();
					m_nxdnFrames = 0U;
					m_nxdninfo = true;
				}
			} else {
				if (opt == NXDN_LICH_STEAL_NONE) {
					if (!m_nxdninfo) {
						std::string netSrc = m_nxdnlookup->findCS(m_nxdnSrc);
						std::string netDst = m_nxdnlookup->findCS(m_nxdnDst);
						LogMessage("Received NXDN late entry from %s to %s%s", netSrc.c_str(), grp ? "TG " : "", netDst.c_str());
						m_conv.putNXDNHeader();
						m_nxdninfo = true;
					}

					m_conv.putNXDN(buffer);
					m_nxdnFrames++;
				}
			}
		}

		if (dmrWatch.elapsed() > DMR_FRAME_PER) {
			unsigned int dmrFrameType = m_conv.getDMR(m_dmrFrame);

			if(dmrFrameType == TAG_HEADER) {
				CDMRData rx_dmrdata;
				dmr_cnt = 0U;
				m_dmrSrc = findDMRID(m_nxdnSrc);
				m_dstid = m_nxdnDst;

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

		if (nxdnWatch.elapsed() > NXDN_FRAME_PER) {
			unsigned int nxdnFrameType = m_conv.getNXDN(m_nxdnFrame);

			if(nxdnFrameType == TAG_HEADER) {
				nxdn_cnt = 0U;
				m_nxdnSrc = findNXDNID(m_dmrSrc);
				m_nxdnTG = m_dmrDst;

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
				layer3.setSourceUnitId(m_nxdnSrc & 0xFFFF);
				layer3.setDestinationGroupId(m_nxdnTG & 0xFFFF);
				layer3.setGroup(true);
				layer3.setDataBlocks(0U);
				layer3.getData(layer3data);

				::memcpy(m_nxdnFrame + 5U, layer3data, 14U);
				::memcpy(m_nxdnFrame + 5U + 14U, layer3data, 14U);

				m_nxdnNetwork->write(m_nxdnFrame, NNMT_VOICE_HEADER);

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
				layer3.setSourceUnitId(m_nxdnSrc & 0xFFFF);
				layer3.setDestinationGroupId(m_nxdnTG & 0xFFFF);
				layer3.setGroup(true);
				layer3.setDataBlocks(0U);
				layer3.getData(layer3data);

				::memcpy(m_nxdnFrame + 5U, layer3data, 14U);
				::memcpy(m_nxdnFrame + 5U + 14U, layer3data, 14U);

				m_nxdnNetwork->write(m_nxdnFrame, NNMT_VOICE_TRAILER);

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
				layer3.setSourceUnitId(m_nxdnSrc & 0xFFFF);
				layer3.setDestinationGroupId(m_nxdnTG & 0xFFFF);
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
				m_nxdnNetwork->write(m_nxdnFrame, NNMT_VOICE_BODY);
				
				nxdn_cnt++;
				nxdnWatch.start();
			}
		}

		stopWatch.start();

		m_dmrNetwork->clock(ms);
		m_nxdnNetwork->clock(ms);

		if (ms < 5U)
			CThread::sleep(5U);
	}

	m_nxdnNetwork->close();
	m_dmrNetwork->close();
	delete m_dmrNetwork;
	delete m_nxdnNetwork;

	::LogFinalise();

	return 0;
}

unsigned int CDMR2NXDN::findNXDNID(unsigned int dmrid)
{
	std::string dmrCS = m_dmrlookup->findCS(dmrid);
	unsigned int nxdnID = m_nxdnlookup->findID(dmrCS);

	if (nxdnID == 0)
		nxdnID = truncID(dmrid);
	else
		LogMessage("NXDN ID of %s: %u", dmrCS.c_str(), nxdnID);

	return nxdnID;
}

unsigned int CDMR2NXDN::findDMRID(unsigned int nxdnid)
{
	std::string nxdnCS = m_nxdnlookup->findCS(nxdnid);
	unsigned int dmrID = m_dmrlookup->findID(nxdnCS);

	if (dmrID == 0)
		dmrID = m_defsrcid;
	else
		LogMessage("DMR ID of %s: %u", nxdnCS.c_str(), dmrID);

	return dmrID;
}

unsigned int CDMR2NXDN::truncID(unsigned int id)
{
	char temp[20];

	snprintf(temp, 8, "%07d", id);
	unsigned int newid = atoi(temp + 2);

	if (newid > 65519 || newid == 0)
		newid = m_defaultID;

	return newid;
}

bool CDMR2NXDN::createMMDVM()
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
