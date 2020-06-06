# Description

This is the source code of NXDN2DMR, a software for digital voice conversion from NXDN to DMR digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software.

You have to use this software with NXDNGateway, with the default NXDN UDP ports (14050 and 42022). In this case, you can select the pseudo TG20 to connect to NXDN2DMR software.

If you want to connect directly to a XLX reflector (with DMR support), you only need to uncomment ([DMR Network] section):

    XLXFile=XLXHosts.txt
    XLXReflector=950
    XLXModule=D

and replace XLXReflector and XLXModule according your preferences. Also, you need to configure the DMR port according the XLX reflector port, for example:

    Port=62030

StartupDstId, StartupPC and Address parameters don't care in XLX mode.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.

# Crosslink configuration

You can use NXDN2DMR to link a [NXDN Reflector](https://github.com/g4klx/NXDNClients) to a DMR network (without using any RF link):

NXDNReflector <-> NXDN2DMR <-> any DMR Network

Install the NXDN2DMR software at the same server where NXDNReflector is located. Configure your [DMR Network] section (NXDN2DMR.ini) as usual, depending on your preferred DMR network. Then, you only need to match the NXDNReflector UDP port (Port in [Network], NXDNReflector.ini) to NXDN UDP port (DstPort in [NXDN Network], NXDN2DMR.ini).

For example, a common UDP port in NXDNReflector.ini:

    [Network]
    Port=41400

Then you need to configure NXDN2DMR.ini (example):

    [NXDN Network]
    Callsign=CE1ABC
    TG=10300
    DstAddress=127.0.0.1
    DstPort=41400
    LocalAddress=127.0.0.1
    LocalPort=41412
    Daemon=0
