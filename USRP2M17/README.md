# Description

This is the source code of USRP2M17, which converts USRP PCM audio and M17 digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software. Typical uses are connecting M17 reflectors to AllStar nodes and can be used with MMDVM modems in FM mode as stand alone radios.

# Configuration
M17 and USRP both have a configuration value 'GainAdjustDB".  Thru trial and error I have found the best balance of audio levels which are set as the defaults in the provided USRP2M17.ini file.  For AllStar connections, USRP address and ports are the values defined in your USRP channel based node.

# Building
run 'make' from the source directory.
