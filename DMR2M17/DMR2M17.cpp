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

#include "DMR2M17.h"
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>

#define DMR_FRAME_PER       55U
#define M17_FRAME_PER       35U
#define M17_PING_TIMEOUT    35000U

const char* DEFAULT_INI_FILE = "/etc/DMR2M17.ini";

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
				::fprintf(stdout, "DMR2M17 version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: DMR2M17 [-v|--version] [filename]\n");
				return 1;
			} else {
				iniFile = argv[currentArg];
			}
		}
	}

	// Capture SIGTERM to finish gracelessly
	if (signal(SIGTERM, sig_handler) == SIG_ERR) 
		::fprintf(stdout, "Can't catch SIGTERM\n");

	CDMR2M17* gateway = new CDMR2M17(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CDMR2M17::CDMR2M17(const std::string& configFile) :
m_callsign(),
m_m17Ref(),
m_conf(configFile),
m_dmrNetwork(NULL),
m_m17Network(NULL),
m_dmrlookup(NULL),
m_conv(),
m_colorcode(1U),
m_dstid(1U),
m_dmrSrc(1U),
m_dmrDst(1U),
m_dmrLastDT(0U),
m_dmrFrame(NULL),
m_dmrFrames(0U),
m_m17Src(),
m_m17Dst(),
m_m17Frame(NULL),
m_m17Frames(0U),
m_EmbeddedLC(),
m_dmrflco(FLCO_GROUP),
m_dmrinfo(false),
m_config(NULL),
m_configLen(0U)
{
	m_m17Frame = new unsigned char[100U];
	m_dmrFrame  = new unsigned char[50U];
	m_config    = new unsigned char[400U];

	::memset(m_m17Frame, 0U, 100U);
	::memset(m_dmrFrame, 0U, 50U);
}

CDMR2M17::~CDMR2M17()
{
	delete[] m_m17Frame;
	delete[] m_dmrFrame;
	delete[] m_config;
}

int CDMR2M17::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "DMR2M17: cannot read the .ini file\n");
		return 1;
	}

	//setlocale(LC_ALL, "C");

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
		::fprintf(stderr, "DMR2M17: unable to open the log file\n");
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
	
	m_m17Network = new CM17Network(m17_localAddress, m17_localPort, m17_dstAddress, m17_dstPort, m17_src, m17_debug);
	
	ret = m_m17Network->open();
	if (!ret) {
		::LogError("Cannot open the M17 network port");
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

	m_dmrflco = FLCO_GROUP;

	CTimer networkWatchdog(100U, 0U, 1500U);
	CTimer pollTimer(1000U, 8U);
	CStopWatch stopWatch;
	CStopWatch m17Watch;
	CStopWatch m17PingWatch;
	CStopWatch dmrWatch;
	
	pollTimer.start();
	stopWatch.start();
	m17Watch.start();
	m17PingWatch.start();
	dmrWatch.start();

	unsigned short m17_cnt = 0;
	unsigned char dmr_cnt = 0;
	
	m_m17Network->writeLink(module);
	
	LogMessage("Starting DMR2M17-%s", VERSION);

	for (; m_killed == 0;) {
		unsigned char buffer[2000U];
		memset(buffer, 0, sizeof(buffer));
		
		CDMRData tx_dmrdata;
		unsigned int ms = stopWatch.elapsed();
		
		if(m17PingWatch.elapsed() > M17_PING_TIMEOUT){
			LogMessage("M17 reflector stopped responding, sending CONN...");
			pollTimer.stop();
			m17PingWatch.start();
			m_m17Network->writeLink(module);
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
				CDMRData rx_dmrdata;
				dmr_cnt = 0U;

				rx_dmrdata.setSlotNo(2U);
				rx_dmrdata.setSrcId(m_dmrSrc);
				rx_dmrdata.setDstId(m_dmrDst);
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
				CDMRLC dmrLC = CDMRLC(m_dmrflco, m_dmrSrc, m_dmrDst);
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
						rx_dmrdata.setDstId(m_dmrDst);
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
				rx_dmrdata.setDstId(m_dmrDst);
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
				CDMRLC dmrLC = CDMRLC(m_dmrflco, m_dmrSrc, m_dmrDst);
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
				rx_dmrdata.setDstId(m_dmrDst);
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
					CDMRLC dmrLC = CDMRLC(m_dmrflco, m_dmrSrc, m_dmrDst);
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
			
			memset(m17_src, 0, 10);
			std::string css = m_dmrlookup->findCS(m_dmrSrc);
			memcpy(m17_src, css.c_str(), css.size());
			m17_src[css.size()] = ' ';
			m17_src[css.size()+1] = 'D';
			encode_callsign(m17_src);
			//fprintf(stderr, "M17 Callsign info %s : %s : %d\n", m17_src, css.c_str(), m_dmrSrc);
			
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
					m_conv.putDMR(dmr_frame);
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

					m_conv.putDMR(dmr_frame);
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

		stopWatch.start();

		m_dmrNetwork->clock(ms);
		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_m17Network->writePoll();
			pollTimer.start();
		}

		if (ms < 5U) CThread::sleep(5U);
	}

	m_m17Network->close();
	m_dmrNetwork->close();
	delete m_dmrNetwork;
	delete m_m17Network;

	::LogFinalise();

	return 0;
}

bool CDMR2M17::createMMDVM()
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
