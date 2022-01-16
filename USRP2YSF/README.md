# Description

This is the source code of USRP2YSF, which converts USRP PCM audio and YSF(N) digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software. Typical uses are connecting YSF reflectors to AllStar nodes and can be used with MMDVM modems in FM mode as stand alone radios.

# Configuration
YSF and USRP both have a configuration value 'GainAdjustDB".  Thru trial and error I have found the best balance of audio levels which are set as the defaults in the provided USRP2YSF.ini file.  For AllStar connections, USRP address and ports are the values defined in your USRP channel based node.

# Building
This utility is not built with the other cross mode ulitities, and has an external dependency:

md380_vocoder https://github.com/nostar/md380_vocoder

md380_vocoder uses md380 firmware for vocoding, so this software needs to be run on an ARM based platform i.e. raspberri pi.
