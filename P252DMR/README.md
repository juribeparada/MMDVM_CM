# Description

This is the source code of P252DMR, a software for digital voice conversion from P25 to DMR digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software.  Unlike the other cross mode utilities upon which this is based, this utility performs software transcoding between IMBE 4400x2800(P25) and AMBE+2 2450x1150(DMR).

You can use this software with MMDVMHost and P25Gateway, with the default UDP ports:

MMDVMHost(P25 Mode):32010 <-> 42020:P25Gateway:42010 <-> 42012:P252DMR <-> (DMR Master server)

If you want to connect directly to a XLX reflector (with DMR support), you only need to uncomment ([DMR Network] section):

    XLXFile=XLXHosts.txt
    XLXReflector=950
    XLXModule=D

and replace XLXReflector and XLXModule according your preferences. Also, you need to configure the DMR port according the XLX reflector port, for example:

    Port=62030

StartupDstId, StartupPC and Address parameters don't care in XLX mode.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.

# Building

This utility is not built with the other cross mode ulitities, and has 2 external dependencies:

imbe_vocoder https://github.com/nostar/imbe_vocoder
md380_vocoder https://github.com/nostar/md380_vocoder

With these dependencies installed, run 'make' from the source directory.

# Crosslink configuration

You can use P252DMR to link a [P25 Reflector](https://github.com/g4klx/P25Clients) to a DMR network (without using any RF link):

P25Reflector <-> P252DMR <-> any DMR Network

Install the P252DMR software at the same server where P25Reflector is located. Configure your [DMR Network] section (P252DMR.ini) as usual, depending on your preferred DMR network. Then, you only need to match the P25Reflector UDP port (Port in [Network], P25Reflector.ini) to P25 UDP port (DstPort in [P25 Network], P252DMR.ini).

For example, a common UDP port in P25Reflector.ini:

    [Network]
    Port=41000

Then you need to configure P252DMR.ini (example):

    [P25 Network]
    Callsign=CE1ABC
    TG=10300
    DstAddress=127.0.0.1
    DstPort=41000
    LocalAddress=127.0.0.1
    LocalPort=41015
    Daemon=0
