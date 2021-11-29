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

#include "M172DMR.h"
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>

#define DMR_FRAME_PER      55U
#define M17_FRAME_PER      35U
#define M17_PING_TIMEOUT    35000U

#define XLX_SLOT            2U
#define XLX_COLOR_CODE      3U

const char* DEFAULT_INI_FILE = "/etc/M172DMR.ini";

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2018 by AD8DP, CA6JAU, G4KLX and others";

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


int main(int argc, char** argv)
{
	const char* iniFile = DEFAULT_INI_FILE;
	if (argc > 1) {
		for (int currentArg = 1; currentArg < argc; ++currentArg) {
			std::string arg = argv[currentArg];
			if ((arg == "-v") || (arg == "--version")) {
				::fprintf(stdout, "M172DMR version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: M172DMR [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	// Capture SIGTERM to finish gracelessly
	if (signal(SIGTERM, sig_handler) == SIG_ERR) 
		::fprintf(stdout, "Can't catch SIGTERM\n");

	CM172DMR* gateway = new CM172DMR(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CM172DMR::CM172DMR(const std::string& configFile) :
m_callsign(),
m_m17Ref(),
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
m_m17Frame(NULL),
m_m17Frames(0U),
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
	m_m17Frame = new unsigned char[100U];
	m_dmrFrame  = new unsigned char[50U];

	::memset(m_m17Frame, 0U, 100U);
	::memset(m_dmrFrame, 0U, 50U);
}

CM172DMR::~CM172DMR()
{
	delete[] m_m17Frame;
	delete[] m_dmrFrame;
}

int CM172DMR::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "M172DMR: cannot read the .ini file\n");
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
		::fprintf(stderr, "M172DMR: unable to open the log file\n");
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
	m_m17Ref = m_conf.getM17DstName();
	char module = m_m17Ref.c_str()[m_m17Ref.find(' ')+1];

	std::string m17_dstAddress   = m_conf.getM17DstAddress();
	unsigned int m17_dstPort     = m_conf.getM17DstPort();
	std::string m17_localAddress = m_conf.getM17LocalAddress();
	unsigned int m17_localPort   = m_conf.getM17LocalPort();
	bool m17_debug               = m_conf.getM17NetworkDebug();
	
	m_conv.setM17GainAdjDb(m_conf.getM17GainAdjDb());
	
	uint16_t streamid = 0;
	unsigned char m17_src[10];
	unsigned char m17_dst[10];
	
	memcpy(m17_src, m_callsign.c_str(), 9);
	m17_src[9] = 0x00;
	encode_callsign(m17_src);
	
	std::string fileName    = m_conf.getDMRXLXFile();
	m_xlxReflectors = new CReflectors(fileName, 60U);
	m_xlxReflectors->load();
	
	m_m17Network = new CM17Network(m17_localAddress, m17_localPort, m17_dstAddress, m17_dstPort, m17_src, m17_debug);
	
	ret = m_m17Network->open();
	if (!ret) {
		::LogError("Cannot open the M17 network port");
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
	CStopWatch m17Watch;
	CStopWatch m17PingWatch;
	CStopWatch dmrWatch;
	stopWatch.start();
	m17Watch.start();
	m17PingWatch.start();
	dmrWatch.start();
	pollTimer.start();

	unsigned char m17_cnt = 0;
	unsigned char dmr_cnt = 0;

	m_m17Network->writeLink(module);

	LogMessage("Starting M172DMR-%s", VERSION);

	for (; end == 0;) {
		unsigned char buffer[2000U];

		CDMRData tx_dmrdata;
		unsigned int ms = stopWatch.elapsed();
		
		if(m17PingWatch.elapsed() > M17_PING_TIMEOUT){
			LogMessage("M17 reflector stopped responding, sending CONN...");
			pollTimer.stop();
			m17PingWatch.start();
			m_m17Network->writeLink(module);
		}

		if (m_dmrNetwork->isConnected() && !m_xlxmodule.empty() && !m_xlxConnected) {
			writeXLXLink(m_defsrcid, m_dstid, m_dmrNetwork);
			LogMessage("XLX, Linking to reflector XLX%03u, module %s", m_xlxrefl, m_xlxmodule.c_str());
			m_xlxConnected = true;
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
				}
				else{
					m_conv.putM17(m_m17Frame);
				}
				uint8_t cs[10];
				memcpy(cs, m_m17Frame+12, 6);
				decode_callsign(cs);
				std::string css((char *)cs);
				css = css.substr(0, css.find(' '));
				 
				int dmrid = m_dmrlookup->findID(css);
				if(dmrid){
					m_dmrSrc = dmrid;
				}
				m_m17Frames++;
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
			
			memset(m17_src, 0, 10);
			std::string css = m_dmrlookup->findCS(m_dmrSrc);
			memcpy(m17_src, css.c_str(), css.size());
			m17_src[css.size()] = ' ';
			m17_src[css.size()+1] = 'D';
			encode_callsign(m17_src);
			
			
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

		if (m17Watch.elapsed() > M17_FRAME_PER) {
			unsigned int m17FrameType = m_conv.getM17(m_m17Frame);
			
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

		stopWatch.start();
		
		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_m17Network->writePoll();
			pollTimer.start();
		}

		if(m_dmrNetwork->clock(ms)){
			m_xlxConnected = false;
		}

		if (m_xlxReflectors != NULL)
			m_xlxReflectors->clock(ms);

		if (ms < 5U) CThread::sleep(5U);
	}

	m_m17Network->close();
	m_dmrNetwork->close();
	delete m_dmrNetwork;
	delete m_m17Network;

	if (m_xlxReflectors != NULL)
		delete m_xlxReflectors;

	::LogFinalise();

	return 0;
}

bool CM172DMR::createDMRNetwork()
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

void CM172DMR::writeXLXLink(unsigned int srcId, unsigned int dstId, CDMRNetwork* network)
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

