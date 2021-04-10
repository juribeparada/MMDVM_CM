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

#include "USRP2DMR.h"
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>

#define DMR_FRAME_PER      55U
#define USRP_FRAME_PER     15U

#define XLX_SLOT            2U
#define XLX_COLOR_CODE      3U

const char* DEFAULT_INI_FILE = "/etc/USRP2DMR.ini";

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2021 by AD8DP, CA6JAU, G4KLX and others";

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
				::fprintf(stdout, "USRP2DMR version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: USRP2DMR [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	// Capture SIGTERM to finish gracelessly
	if (signal(SIGTERM, sig_handler) == SIG_ERR) 
		::fprintf(stdout, "Can't catch SIGTERM\n");

	CUSRP2DMR* gateway = new CUSRP2DMR(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CUSRP2DMR::CUSRP2DMR(const std::string& configFile) :
m_callsign(),
m_usrpcs(),
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
m_usrpFrame(NULL),
m_usrpFrames(0U),
m_dmrFrame(NULL),
m_dmrFrames(0U),
m_EmbeddedLC(),
m_dmrflco(FLCO_GROUP),
m_dmrinfo(false),
m_xlxmodule(),
m_xlxConnected(false),
m_xlxReflectors(NULL),
m_xlxrefl(0U),
m_firstSync(false)
{
	m_usrpFrame = new uint8_t[400U];
	m_dmrFrame  = new uint8_t[50U];

	::memset(m_usrpFrame, 0U, 400U);
	::memset(m_dmrFrame, 0U, 50U);
}

CUSRP2DMR::~CUSRP2DMR()
{
	delete[] m_usrpFrame;
	delete[] m_dmrFrame;
}

int CUSRP2DMR::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "USRP2DMR: cannot read the .ini file\n");
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
		::fprintf(stderr, "USRP2DMR: unable to open the log file\n");
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

	std::string usrp_address      = m_conf.getUSRPAddress();
	uint16_t usrp_dstPort     = m_conf.getUSRPDstPort();
	uint16_t usrp_localPort   = m_conf.getUSRPLocalPort();
	bool usrp_debug               = m_conf.getUSRPDebug();
	
	m_conv.setUSRPGainAdjDb(m_conf.getUSRPGainAdjDb());
	m_conv.setDMRGainAdjDb(m_conf.getDMRGainAdjDb());
	
	m_usrpNetwork = new CUSRPNetwork(usrp_address, usrp_dstPort, usrp_localPort, usrp_debug);
	
	ret = m_usrpNetwork->open();
	if (!ret) {
		::LogError("Cannot open the USRP network port");
		::LogFinalise();
		return 1;
	}

	std::string fileName    = m_conf.getDMRXLXFile();
	m_xlxReflectors = new CReflectors(fileName, 60U);
	m_xlxReflectors->load();
	
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
	CStopWatch usrpWatch;
	CStopWatch dmrWatch;
	stopWatch.start();
	usrpWatch.start();
	dmrWatch.start();
	pollTimer.start();

	uint32_t usrp_cnt = 0;
	uint8_t dmr_cnt = 0;

	LogMessage("Starting USRP2DMR-%s", VERSION);

	for (; end == 0;) {
		unsigned char buffer[2000U];

		CDMRData tx_dmrdata;
		unsigned int ms = stopWatch.elapsed();

		if (m_dmrNetwork->isConnected() && !m_xlxmodule.empty() && !m_xlxConnected) {
			writeXLXLink(m_defsrcid, m_dstid, m_dmrNetwork);
			LogMessage("XLX, Linking to reflector XLX%03u, module %s", m_xlxrefl, m_xlxmodule.c_str());
			m_xlxConnected = true;
		}

		uint32_t len = 0;
		while ( (len = m_usrpNetwork->readData(m_usrpFrame, 400)) ) {
			if( !memcmp(m_usrpFrame, "USRP", 4) && (len == 32) ) {
				LogMessage("USRP received end of voice transmission, %.1f seconds", float(m_usrpFrames) / 50.0F);
				m_conv.putUSRPEOT();
				m_usrpFrames = 0U;
			}

			if( (!memcmp(m_usrpFrame, "USRP", 4)) && len == 352) {
				if( (m_usrpFrame[20] == USRP_TYPE_TEXT) && (m_usrpFrame[32] == TLV_TAG_SET_INFO) ){
					const uint32_t dmrsrc = (m_usrpFrame[34] << 16) | (m_usrpFrame[35] << 8) | m_usrpFrame[36];
					m_dmrSrc = (dmrsrc > 100000) ? dmrsrc : m_defsrcid;
					if(!m_usrpFrames){	
						m_conv.putUSRPHeader();
						LogMessage("USRP text info received as first frame");
					}
					m_usrpFrames++;
				}
				else if( (m_usrpFrame[20] == USRP_TYPE_VOICE) && (m_usrpFrame[15] == USRP_KEYUP_TRUE) ){
					if(!m_usrpFrames){
						m_dmrSrc = m_defsrcid;
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

		if (dmrWatch.elapsed() > DMR_FRAME_PER) {
			unsigned int dmrFrameType = m_conv.getDMR(m_dmrFrame);

			if(dmrFrameType == TAG_HEADER) {
				LogMessage("Sending DMR Header");
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
					m_usrpcs = netSrc;
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
						m_usrpcs = netSrc;
						m_conv.putDMRHeader();
						LogMessage("DMR late entry from %s to %s", netSrc.c_str(), netDst.c_str());

						m_dmrinfo = true;
					}

					m_conv.putDMR(dmr_frame);
					m_dmrFrames++;
				}
			}
			else {
				if(DataType == DT_VOICE_SYNC || DataType == DT_VOICE) {
					unsigned char dmr_frame[50];
					tx_dmrdata.getData(dmr_frame);
					m_conv.putDMR(dmr_frame);
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

		if (usrpWatch.elapsed() > USRP_FRAME_PER) {
			int16_t pcm[160];
			uint32_t usrpFrameType = m_conv.getUSRP(pcm);
			
			if(usrpFrameType == TAG_USRP_HEADER){
				//CUtils::dump(1U, "USRP data:", m_usrpFrame, 33U);

				const uint32_t cnt = htonl(usrp_cnt);
				const uint32_t dmrid = (htonl(m_dmrSrc)) >> 8;
				const uint32_t gwid = htonl(m_dmrSrc * 100);
				const uint32_t dstid = (htonl(m_dstid)) >> 8;
				
				memset(m_usrpFrame, 0, 352);
				memcpy(m_usrpFrame, "USRP", 4);
				memcpy(m_usrpFrame+4, &cnt, 4);
				m_usrpFrame[15] = USRP_KEYUP_FALSE;
				m_usrpFrame[20] = USRP_TYPE_TEXT;
				m_usrpFrame[32] = TLV_TAG_SET_INFO;
				m_usrpFrame[33] = 13 + m_usrpcs.size();
				memcpy(m_usrpFrame+34, &dmrid, 3);
				memcpy(m_usrpFrame+37, &gwid, 4);
				memcpy(m_usrpFrame+41, &dstid, 3);
				m_usrpFrame[44] = 2;
				m_usrpFrame[45] = m_colorcode;
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

		stopWatch.start();
		
		pollTimer.clock(ms);
		
		if(m_dmrNetwork->clock(ms)){
			m_xlxConnected = false;
		}

		if (m_xlxReflectors != NULL)
			m_xlxReflectors->clock(ms);

		if (ms < 5U) ::usleep(5U * 1000U);
	}

	m_usrpNetwork->close();
	m_dmrNetwork->close();
	delete m_dmrNetwork;
	delete m_usrpNetwork;

	if (m_xlxReflectors != NULL)
		delete m_xlxReflectors;

	::LogFinalise();

	return 0;
}

bool CUSRP2DMR::createDMRNetwork()
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

void CUSRP2DMR::writeXLXLink(unsigned int srcId, unsigned int dstId, CDMRNetwork* network)
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
