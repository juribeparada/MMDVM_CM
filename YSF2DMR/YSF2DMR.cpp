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

#include "YSF2DMR.h"

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

#define XLX_SLOT            2U
#define XLX_COLOR_CODE      3U

#if defined(_WIN32) || defined(_WIN64)
const char* DEFAULT_INI_FILE = "YSF2DMR.ini";
#else
const char* DEFAULT_INI_FILE = "/etc/YSF2DMR.ini";
#endif

const char* HEADER1 = "This software is for use on amateur radio networks only,";
const char* HEADER2 = "it is to be used for educational purposes only. Its use on";
const char* HEADER3 = "commercial networks is strictly prohibited.";
const char* HEADER4 = "Copyright(C) 2018,2019 by CA6JAU, EA7EE, G4KLX, AD8DP and others";

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
				::fprintf(stdout, "YSF2DMR version %s\n", VERSION);
				return 0;
			} else if (arg.substr(0, 1) == "-") {
				::fprintf(stderr, "Usage: YSF2DMR [-v|--version] [filename]\n");
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

	CYSF2DMR* gateway = new CYSF2DMR(std::string(iniFile));

	int ret = gateway->run();

	delete gateway;

	return ret;
}

CYSF2DMR::CYSF2DMR(const std::string& configFile) :
m_callsign(),
m_suffix(),
m_conf(configFile),
m_wiresX(NULL),
m_dmrNetwork(NULL),
m_ysfNetwork(NULL),
m_lookup(NULL),
m_conv(),
m_colorcode(1U),
m_srcHS(1U),
m_srcid(1U),
m_defsrcid(1U),
m_dstid(1U),
m_ptt_dstid(1U),
m_ptt_pc(false),
m_dmrpc(false),
m_netSrc(),
m_netDst(),
m_ysfSrc(),
m_dmrLastDT(0U),
m_ysfFrame(NULL),
m_dmrFrame(NULL),
m_gps(NULL),
m_dtmf(NULL),
m_APRS(NULL),
m_dmrFrames(0U),
m_ysfFrames(0U),
m_EmbeddedLC(),
m_TGList(),
m_dmrflco(FLCO_GROUP),
m_dmrinfo(false),
m_idUnlink(4000U),
m_flcoUnlink(FLCO_GROUP),
m_enableWiresX(false),
m_xlxmodule(),
m_xlxConnected(false),
m_xlxReflectors(NULL),
m_xlxrefl(0U),
m_remoteGateway(false),
m_hangTime(1000U),
m_firstSync(false)
{
	m_ysfFrame = new unsigned char[200U];
	m_dmrFrame = new unsigned char[50U];

	::memset(m_ysfFrame, 0U, 200U);
	::memset(m_dmrFrame, 0U, 50U);
}

CYSF2DMR::~CYSF2DMR()
{
	delete[] m_ysfFrame;
	delete[] m_dmrFrame;
}

int CYSF2DMR::run()
{
	bool ret = m_conf.read();
	if (!ret) {
		::fprintf(stderr, "YSF2DMR: cannot read the .ini file\n");
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
		::fprintf(stderr, "YSF2DMR: unable to open the log file\n");
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
	m_suffix   = m_conf.getSuffix();

	m_remoteGateway = m_conf.getRemoteGateway();
	m_hangTime = m_conf.getHangTime();

	bool debug               = m_conf.getDMRNetworkDebug();
	in_addr dstAddress       = CUDPSocket::lookup(m_conf.getDstAddress());
	unsigned int dstPort     = m_conf.getDstPort();
	std::string localAddress = m_conf.getLocalAddress();
	unsigned int localPort   = m_conf.getLocalPort();

	std::string fileName    = m_conf.getDMRXLXFile();
	m_xlxReflectors = new CReflectors(fileName, 60U);
	m_xlxReflectors->load();

	m_ysfNetwork = new CYSFNetwork(localAddress, localPort, m_callsign, debug);
	m_ysfNetwork->setDestination(dstAddress, dstPort);

	LogInfo("General Parameters");
	LogInfo("    Remote Gateway: %s", m_remoteGateway ? "yes" : "no");
	LogInfo("    Hang Time: %u ms", m_hangTime);

	ret = m_ysfNetwork->open();
	if (!ret) {
		::LogError("Cannot open the YSF network port");
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

	m_lookup = new CDMRLookup(lookupFile, reloadTime);
	m_lookup->read();
	m_dropUnknown = m_conf.getDMRDropUnknown();

	if (m_dmrpc)
		m_dmrflco = FLCO_USER_USER;
	else
		m_dmrflco = FLCO_GROUP;

	CTimer networkWatchdog(100U, 0U, 1500U);
	CTimer pollTimer(1000U, 5U);
	CTimer ysfWatchdog(1000U, 0U, 500U);

	// CWiresX Control Object
	if (m_enableWiresX) {
		bool makeUpper = m_conf.getWiresXMakeUpper();
		m_wiresX = new CWiresX(m_callsign, m_suffix, m_ysfNetwork, m_TGList, makeUpper);
		m_dtmf = new CDTMF;
	}

	std::string name = m_conf.getDescription();
	unsigned int rxFrequency = m_conf.getRxFrequency();
	unsigned int txFrequency = m_conf.getTxFrequency();
	int reflector = m_conf.getDMRDstId();

	if (m_wiresX != NULL)
		m_wiresX->setInfo(name, txFrequency, rxFrequency, reflector);

	if (m_conf.getAPRSEnabled()) {
		createGPS();
		m_APRS = new CAPRSReader(m_conf.getAPRSAPIKey(), m_conf.getAPRSRefresh());
	}
	
	CStopWatch TGChange;
	CStopWatch stopWatch;
	CStopWatch ysfWatch;
	CStopWatch dmrWatch;
	stopWatch.start();
	ysfWatch.start();
	dmrWatch.start();
	pollTimer.start();
	ysfWatchdog.stop();

	unsigned char ysf_cnt = 0;
	unsigned char dmr_cnt = 0;

	LogMessage("Starting YSF2DMR-%s", VERSION);

	bool enableUnlink = m_conf.getDMRNetworkEnableUnlink();
	bool unlinkReceived = false;

	TG_STATUS TG_connect_state = NONE;
	unsigned char gps_buffer[20U];

	unsigned int tglistOpt = 0; 

	for (; end == 0;) {
		unsigned char buffer[2000U];

		CDMRData tx_dmrdata;
		unsigned int ms = stopWatch.elapsed();

		if (m_dmrNetwork->isConnected() && !m_xlxmodule.empty() && !m_xlxConnected) {
			writeXLXLink(m_srcid, m_dstid, m_dmrNetwork);
			LogMessage("XLX, Linking to reflector XLX%03u, module %s", m_xlxrefl, m_xlxmodule.c_str());
			m_xlxConnected = true;
		}
		else if (!m_dmrNetwork->isConnected() && !m_xlxmodule.empty() && m_xlxConnected) {
			LogMessage("XLX, Disconnected from reflector XLX%03u, module %s", m_xlxrefl, m_xlxmodule.c_str());
			m_xlxConnected = false;
		}

		if (m_wiresX != NULL) {
			switch (TG_connect_state) {
				case WAITING_UNLINK:
					if (unlinkReceived) {
						//LogMessage("Unlink Received");
						TGChange.start();
						TG_connect_state = SEND_REPLY;
						unlinkReceived = false;
					}
					break;
				case SEND_REPLY:
					if (TGChange.elapsed() > 600) {
						TGChange.start();
						TG_connect_state = SEND_PTT;
						m_wiresX->sendConnectReply(m_dstid);
					}
					break;
				case SEND_PTT:
					if (TGChange.elapsed() > 600) {
						TGChange.start();
						TG_connect_state = NONE;
						if (m_ptt_dstid) {
							LogMessage("Sending PTT: Src: %s Dst: %s%d", m_ysfSrc.c_str(), m_ptt_pc ? "" : "TG ", m_ptt_dstid);
							SendDummyDMR(m_srcid, m_ptt_dstid, m_ptt_pc ? FLCO_USER_USER : FLCO_GROUP);
						}
					}
					break;
				default: 
					break;
			}

			if ((TG_connect_state != NONE) && (TGChange.elapsed() > 12000)) {
				LogMessage("Timeout changing TG");
				TG_connect_state = NONE;
			}
		}

		while (m_ysfNetwork->read(buffer) > 0U) {
			CYSFFICH fich;
			bool valid = fich.decode(buffer + 35U);

			if (valid) {
				unsigned char fi = fich.getFI();
				unsigned char dt = fich.getDT();
				unsigned char fn = fich.getFN();
				unsigned char ft = fich.getFT();
				
				if (m_wiresX != NULL) {
					WX_STATUS status = m_wiresX->process(buffer + 35U, buffer + 14U, fi, dt, fn, ft);
					m_ysfSrc = getSrcYSF(buffer);

					switch (status) {
						case WXS_CONNECT:
							m_srcid = findYSFID(m_ysfSrc, false);

							m_ptt_dstid = m_wiresX->getDstID();
							tglistOpt = m_wiresX->getOpt(m_ptt_dstid);

							switch (tglistOpt) {
								case 0:
									m_ptt_pc = false;
									m_dstid = m_wiresX->getFullDstID();
									m_ptt_dstid = m_dstid;
									m_dmrflco = FLCO_GROUP;
									LogMessage("Connect to TG %d has been requested by %s", m_dstid, m_ysfSrc.c_str());
									break;
							
								case 1:
									m_ptt_pc = true;
									m_dstid = 9U;
									m_dmrflco = FLCO_GROUP;
									LogMessage("Connect to REF %d has been requested by %s", m_ptt_dstid, m_ysfSrc.c_str());
									break;
								
								case 2:
									m_ptt_dstid = 0;
									m_ptt_pc = true;
									m_dstid = m_wiresX->getFullDstID();
									m_dmrflco = FLCO_USER_USER;
									LogMessage("Connect to %d has been requested by %s", m_dstid, m_ysfSrc.c_str());
									break;
							
								default:
									m_ptt_pc = false;
									m_dstid = m_wiresX->getFullDstID();
									m_ptt_dstid = m_dstid;
									m_dmrflco = FLCO_GROUP;
									LogMessage("Connect to TG %d has been requested by %s", m_dstid, m_ysfSrc.c_str());
									break;
							}

							if (enableUnlink && (tglistOpt != 2) && (m_ptt_dstid != m_idUnlink) && (m_ptt_dstid != 5000)) {
								LogMessage("Sending DMR Disconnect: Src: %s Dst: %s%d", m_ysfSrc.c_str(), m_flcoUnlink == FLCO_GROUP ? "TG " : "", m_idUnlink);

								SendDummyDMR(m_srcid, m_idUnlink, m_flcoUnlink);

								unlinkReceived = false;
								TG_connect_state = WAITING_UNLINK;
							} else 
								TG_connect_state = SEND_REPLY;

							TGChange.start();
							break;

						case WXS_DX:
							break;

						case WXS_DISCONNECT:
							LogMessage("Disconnect has been requested by %s", m_ysfSrc.c_str());

							m_srcid = findYSFID(m_ysfSrc, false);
							m_ptt_dstid = 9U;
							m_ptt_pc = false;
							m_dstid = 9U;
							m_dmrflco = FLCO_GROUP;

							SendDummyDMR(m_srcid, m_idUnlink, m_flcoUnlink);

							TG_connect_state = WAITING_UNLINK;

							TGChange.start();
							break;

						default:
							break;
					}

					status = WXS_NONE;

					if (dt == YSF_DT_VD_MODE2)
						status = m_dtmf->decodeVDMode2(buffer + 35U, (buffer[34U] & 0x01U) == 0x01U);

					switch (status) {
						case WXS_CONNECT:
							m_srcid = findYSFID(m_ysfSrc, false);

							m_ptt_dstid = m_dtmf->getDstID();
							tglistOpt = m_wiresX->getOpt(m_ptt_dstid);

							switch (tglistOpt) {
								case 0:
									m_ptt_pc = false;
									m_dstid = m_wiresX->getFullDstID();
									m_ptt_dstid = m_dstid;
									m_dmrflco = FLCO_GROUP;
									LogMessage("Connect to TG %d has been requested by %s", m_dstid, m_ysfSrc.c_str());
									break;
							
								case 1:
									m_ptt_pc = true;
									m_dstid = 9U;
									m_dmrflco = FLCO_GROUP;
									LogMessage("Connect to REF %d has been requested by %s", m_ptt_dstid, m_ysfSrc.c_str());
									break;
								
								case 2:
									m_ptt_dstid = 0;
									m_ptt_pc = true;
									m_dstid = m_wiresX->getFullDstID();
									m_dmrflco = FLCO_USER_USER;
									LogMessage("Connect to %d has been requested by %s", m_dstid, m_ysfSrc.c_str());
									break;
							
								default:
									m_ptt_pc = false;
									m_dstid = m_wiresX->getFullDstID();
									m_ptt_dstid = m_dstid;
									m_dmrflco = FLCO_GROUP;
									LogMessage("Connect to TG %d has been requested by %s", m_dstid, m_ysfSrc.c_str());
									break;
							}

							LogMessage("Connect to %s%d via DTMF has been requested by %s", m_ptt_pc ? "" : "TG ", m_ptt_dstid, m_ysfSrc.c_str());

							if (enableUnlink && (tglistOpt != 2) && (m_ptt_dstid != m_idUnlink) && (m_ptt_dstid != 5000)) {
								LogMessage("Sending DMR Disconnect: Src: %s Dst: %s%d", m_ysfSrc.c_str(), m_flcoUnlink == FLCO_GROUP ? "TG " : "", m_idUnlink);

								SendDummyDMR(m_srcid, m_idUnlink, m_flcoUnlink);
							
								unlinkReceived = false;
								TG_connect_state = WAITING_UNLINK;
							} else
								TG_connect_state = SEND_REPLY;

							TGChange.start();
							break;

						case WXS_DISCONNECT:
							LogMessage("Disconnect via DTMF has been requested by %s", m_ysfSrc.c_str());

							m_srcid = findYSFID(m_ysfSrc, false);
							m_ptt_dstid = 9U;
							m_ptt_pc = false;
							m_dstid = 9U;
							m_dmrflco = FLCO_GROUP;

							SendDummyDMR(m_srcid, m_idUnlink, m_flcoUnlink);

							TG_connect_state = WAITING_UNLINK;
							TGChange.start();
							break;

						default:
							break;
					}
				}

				if ((::memcmp(buffer, "YSFD", 4U) == 0U) && (dt == YSF_DT_VD_MODE2)) {
					CYSFPayload ysfPayload;

					if (fi == YSF_FI_HEADER) {
						if (ysfPayload.processHeaderData(buffer + 35U)) {
							ysfWatchdog.start();
							std::string ysfSrc = ysfPayload.getSource();
							std::string ysfDst = ysfPayload.getDest();
							LogMessage("Received YSF Header: Src: %s Dst: %s", ysfSrc.c_str(), ysfDst.c_str());
							
							m_dmrNetwork->reset(2U);	// OE1KBC fix
							
							m_srcid = findYSFID(ysfSrc, true);
							if (m_dropUnknown == 0 || m_srcid != 0) {
								ysfWatchdog.start();
								m_dmrNetwork->reset(2U);	// OE1KBC fix
								 m_conv.putYSFHeader();
								m_ysfFrames = 0U;
							}
							else
							{
								LogMessage("Dropped source without DMR ID: %s", ysfSrc.c_str());
							}
						}
					} else if (fi == YSF_FI_TERMINATOR) {
						if (m_dropUnknown == 0 || m_srcid != 0) {
							ysfWatchdog.stop();
							int extraFrames = (m_hangTime / 100U) - m_ysfFrames - 2U;
							for (int i = 0U; i < extraFrames; i++)
								m_conv.putDummyYSF();
							LogMessage("YSF received end of voice transmission, %.1f seconds", float(m_ysfFrames) / 10.0F);
							m_conv.putYSFEOT();
							m_ysfFrames = 0U;
						}
					} else if (fi == YSF_FI_COMMUNICATIONS) {
						if (m_dropUnknown == 0 || m_srcid != 0) {
							ysfWatchdog.start();
							m_conv.putYSF(buffer + 35U);
							m_ysfFrames++;
						}
					}
				}

				if (m_gps != NULL)
					m_gps->data(buffer + 14U, buffer + 35U, fi, dt, fn, ft, m_dstid);
				
			}

			if ((buffer[34U] & 0x01U) == 0x01U) {
				if (m_gps != NULL)
					m_gps->reset();
				if (m_dtmf != NULL)
					m_dtmf->reset();
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

				if(DataType == DT_TERMINATOR_WITH_LC) {
					if (m_dmrFrames == 0U) {
						m_dmrNetwork->reset(2U);
						networkWatchdog.stop();
						m_dmrinfo = false;
						m_firstSync = false;
						break;
					}

					LogMessage("DMR received end of voice transmission, %.1f seconds", float(m_dmrFrames) / 16.667F);

					if (SrcId == 4000)
						unlinkReceived = true;

					m_conv.putDMREOT();
					m_dmrNetwork->reset(2U);
					networkWatchdog.stop();
					m_dmrFrames = 0U;
					m_dmrinfo = false;
					m_firstSync = false;
				}

				if((DataType == DT_VOICE_LC_HEADER) && (DataType != m_dmrLastDT)) {
					
					// DT1 & DT2 without GPS info
					::memcpy(gps_buffer, dt1_temp, 10U);
					::memcpy(gps_buffer + 10U, dt2_temp, 10U);

					if (SrcId == 9990U)
						m_netSrc = "PARROT";
					else if (SrcId == 9U)
						m_netSrc = "LOCAL";
					else if (SrcId == 4000U)
						m_netSrc = "UNLINK";
					else
						m_netSrc = m_lookup->findCS(SrcId);

					m_netDst = (netflco == FLCO_GROUP ? "TG " : "") + m_lookup->findCS(DstId);

					m_conv.putDMRHeader();
					LogMessage("DMR audio received from %s to %s", m_netSrc.c_str(), m_netDst.c_str());

					m_dmrinfo = true;

					if (m_lookup->exists(SrcId) && (m_APRS != NULL)) {
						int lat, lon, resp;
						resp = m_APRS->findCall(m_netSrc, &lat, &lon);

						//LogMessage("Searching GPS Position of %s in aprs.fi", m_netSrc.c_str());

						if (resp) {
							LogMessage("GPS Position of %s Lat: %0.3f, Lon: %0.3f", m_netSrc.c_str(), (float)lat / 1000.0, (float)lon / 1000.0);
							m_APRS->formatGPS(gps_buffer, lat, lon);
						}
						// else
						//	LogMessage("GPS Position not available");
					}

					m_netSrc.resize(YSF_CALLSIGN_LENGTH, ' ');
					m_netDst.resize(YSF_CALLSIGN_LENGTH, ' ');
					
					m_dmrFrames = 0U;
					m_firstSync = false;
				}

				if(DataType == DT_VOICE_SYNC)
					m_firstSync = true;

				if((DataType == DT_VOICE_SYNC || DataType == DT_VOICE) && m_firstSync) {
					unsigned char dmr_frame[50];

					tx_dmrdata.getData(dmr_frame);

					if (!m_dmrinfo) {
						if (SrcId == 9990U)
							m_netSrc = "PARROT";
						else if (SrcId == 9U)
							m_netSrc = "LOCAL";
						else if (SrcId == 4000U)
							m_netSrc = "UNLINK";
						else
							m_netSrc = m_lookup->findCS(SrcId);

						m_netDst = (netflco == FLCO_GROUP ? "TG " : "") + m_lookup->findCS(DstId);

						LogMessage("DMR audio late entry received from %s to %s", m_netSrc.c_str(), m_netDst.c_str());

						if (m_lookup->exists(SrcId) && (m_APRS != NULL)) {
							int lat, lon, resp;
							resp = m_APRS->findCall(m_netSrc, &lat, &lon);

							//LogMessage("Searching GPS Position of %s in aprs.fi", m_netSrc.c_str());

							if (resp) {
								LogMessage("GPS Position of %s Lat: %0.3f, Lon: %0.3f", m_netSrc.c_str(), (float)lat / 1000.0, (float)lon / 1000.0);
								m_APRS->formatGPS(gps_buffer, lat, lon);
							}
							// else
							//	LogMessage("GPS Position not available");
						}

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
					m_dmrNetwork->reset(2U);
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

				// Send data to MMDVMHost
				m_ysfNetwork->write(m_ysfFrame);

				ysf_cnt++;
				ysfWatch.start();
			}
		}

		stopWatch.start();

		m_ysfNetwork->clock(ms);
		m_dmrNetwork->clock(ms);

		if (m_wiresX != NULL)
			m_wiresX->clock(ms);

		if (m_gps != NULL)
			m_gps->clock(ms);

		pollTimer.clock(ms);
		if (pollTimer.isRunning() && pollTimer.hasExpired()) {
			m_ysfNetwork->writePoll();
			pollTimer.start();
		}

		ysfWatchdog.clock(ms);
		if (ysfWatchdog.isRunning() && ysfWatchdog.hasExpired()) {
			int extraFrames = (m_hangTime / 100U) - m_ysfFrames;
			for (int i = 0U; i < extraFrames; i++)
				m_conv.putDummyYSF();
			ysfWatchdog.stop();
		}

		if (m_xlxReflectors != NULL)
			m_xlxReflectors->clock(ms);

		if (ms < 5U)
			CThread::sleep(5U);
	}

	m_ysfNetwork->close();
	m_dmrNetwork->close();
	
	if (m_APRS != NULL) {
		m_APRS->stop();
		delete m_APRS;
	}
	
	if (m_gps != NULL) {
		m_gps->close();
		delete m_gps;
	}
	
	delete m_dmrNetwork;
	delete m_ysfNetwork;

	if (m_wiresX != NULL) {
		delete m_wiresX;
		delete m_dtmf;
	}

	if (m_xlxReflectors != NULL)
		delete m_xlxReflectors;

	::LogFinalise();

	return 0;
}

void CYSF2DMR::createGPS()
{
	std::string hostname = m_conf.getAPRSServer();
	unsigned int port    = m_conf.getAPRSPort();
	std::string password = m_conf.getAPRSPassword();
	std::string callsign = m_conf.getAPRSCallsign();
	std::string desc     = m_conf.getAPRSDescription();

	if (callsign.empty())
		callsign = m_callsign;

	LogMessage("APRS Parameters");
	LogMessage("    Callsign: %s", callsign.c_str());
	LogMessage("    Server: %s", hostname.c_str());
	LogMessage("    Port: %u", port);
	LogMessage("    Passworwd: %s", password.c_str());
	LogMessage("    Description: %s", desc.c_str());

	m_gps = new CGPS(callsign, m_suffix, password, hostname, port);

	unsigned int txFrequency = m_conf.getTxFrequency();
	unsigned int rxFrequency = m_conf.getRxFrequency();
	float latitude           = m_conf.getLatitude();
	float longitude          = m_conf.getLongitude();
	int height               = m_conf.getHeight();

	m_gps->setInfo(txFrequency, rxFrequency, latitude, longitude, height, desc);

	bool ret = m_gps->open();
	if (!ret) {
		delete m_gps;
		LogMessage("Error starting GPS");
		m_gps = NULL;
	}
}

void CYSF2DMR::SendDummyDMR(unsigned int srcid,unsigned int dstid, FLCO dmr_flco)
{
	CDMRData dmrdata;
	CDMRSlotType slotType;
	CDMRFullLC fullLC;

	int dmr_cnt = 0U;

	// Generate DMR LC for header and TermLC frames
	CDMRLC dmrLC = CDMRLC(dmr_flco, srcid, dstid);

	// Build DMR header
	dmrdata.setSlotNo(2U);
	dmrdata.setSrcId(srcid);
	dmrdata.setDstId(dstid);
	dmrdata.setFLCO(dmr_flco);
	dmrdata.setN(0U);
	dmrdata.setSeqNo(0U);
	dmrdata.setBER(0U);
	dmrdata.setRSSI(0U);
	dmrdata.setDataType(DT_VOICE_LC_HEADER);

	// Add sync
	CSync::addDMRDataSync(m_dmrFrame, 0);

	// Add SlotType
	slotType.setColorCode(m_colorcode);
	slotType.setDataType(DT_VOICE_LC_HEADER);
	slotType.getData(m_dmrFrame);

	// Full LC
	fullLC.encode(dmrLC, m_dmrFrame, DT_VOICE_LC_HEADER);

	dmrdata.setData(m_dmrFrame);

	// Send DMR header
	for (unsigned int i = 0U; i < 3U; i++) {
		dmrdata.setSeqNo(dmr_cnt);
		m_dmrNetwork->write(dmrdata);
		dmr_cnt++;
	}

	// Build DMR TermLC
	dmrdata.setSeqNo(dmr_cnt);
	dmrdata.setDataType(DT_TERMINATOR_WITH_LC);

	// Add sync
	CSync::addDMRDataSync(m_dmrFrame, 0);

	// Add SlotType
	slotType.setColorCode(m_colorcode);
	slotType.setDataType(DT_TERMINATOR_WITH_LC);
	slotType.getData(m_dmrFrame);

	// Full LC for TermLC frame
	fullLC.encode(dmrLC, m_dmrFrame, DT_TERMINATOR_WITH_LC);

	dmrdata.setData(m_dmrFrame);

	// Send DMR TermLC
	m_dmrNetwork->write(dmrdata);
}

unsigned int CYSF2DMR::findYSFID(std::string cs, bool showdst)
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
		if (m_dropUnknown == 0) id = m_defsrcid;
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

std::string CYSF2DMR::getSrcYSF(const unsigned char* buffer)
{
	unsigned char temp[YSF_CALLSIGN_LENGTH + 1U];

	::memcpy(temp, buffer + 14U, YSF_CALLSIGN_LENGTH);
	temp[YSF_CALLSIGN_LENGTH] = 0U;
	
	std::string trimmed = reinterpret_cast<char const*>(temp);
	trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), trimmed.end());
	
	return trimmed;
}

bool CYSF2DMR::createDMRNetwork()
{
	std::string address  = m_conf.getDMRNetworkAddress();
	m_xlxmodule          = m_conf.getDMRXLXModule();
	m_xlxrefl            = m_conf.getDMRXLXReflector();
	unsigned int port    = m_conf.getDMRNetworkPort();
	unsigned int local   = m_conf.getDMRNetworkLocal();
	std::string password = m_conf.getDMRNetworkPassword();
	bool debug           = m_conf.getDMRNetworkDebug();
	unsigned int jitter  = m_conf.getDMRNetworkJitter();
	bool slot1           = false;
	bool slot2           = true;
	bool duplex          = false;
	HW_TYPE hwType       = HWT_MMDVM;

	m_srcHS = m_conf.getDMRId();
	m_colorcode = 1U;
	m_TGList = m_conf.getDMRTGListFile();
	m_idUnlink = m_conf.getDMRNetworkIDUnlink();
	bool pcUnlink = m_conf.getDMRNetworkPCUnlink();
	m_enableWiresX = m_conf.getEnableWiresX();

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

	if (pcUnlink)
		m_flcoUnlink = FLCO_USER_USER;
	else
		m_flcoUnlink = FLCO_GROUP;

	if (m_srcHS > 99999999U)
		m_defsrcid = m_srcHS / 100U;
	else if (m_srcHS > 9999999U)
		m_defsrcid = m_srcHS / 10U;
	else
		m_defsrcid = m_srcHS;

	m_srcid = m_defsrcid;
	bool enableUnlink = m_conf.getDMRNetworkEnableUnlink();
	
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
	LogMessage("    Send %s%u Disconect: %s", pcUnlink ? "" : "TG ", m_idUnlink, (enableUnlink) ? "Yes":"No");
	LogMessage("    TGList file: %s", m_TGList.c_str());
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

void CYSF2DMR::writeXLXLink(unsigned int srcId, unsigned int dstId, CDMRNetwork* network)
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

