# Description

This is the source code of USRP2DMR, which converts USRP PCM audio and DMR digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software. Typical uses are connecting DMR talkgroups and XLX reflectors to AllStar nodes and can be used with MMDVM modems in FM mode as stand alone radios.

If you want to connect directly to a XLX reflector (with DMR support), you only need to uncomment ([DMR Network] section):

    XLXFile=XLXHosts.txt
    XLXReflector=950
    XLXModule=D

and replace XLXReflector and XLXModule according your preferences. Also, you need to configure the DMR port according the XLX reflector port, for example:

    Port=62030

StartupDstId, StartupPC and Address parameters don't care in XLX mode.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.

# Building
This utility is not built with the other cross mode ulitities, and has 3 external dependencies:

md380_vocoder https://github.com/nostar/md380_vocoder

mbelib https://github.com/szechyjs/mbelib

md380_vocoder uses md380 firmware for vocoding, so this software needs to be run on an ARM based platform i.e. raspberri pi.

