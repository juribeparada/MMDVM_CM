# Description

This is the source code of YSF2P25, a software for digital voice conversion from Yaesu System Fusion using Voice FR (VW) mode to P25, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software.

You can use this software and YSFGateway at the same time, with the default YSF UDP ports (42000 and 42015). In this case, you can select the pseudo "YSF2P25" reflector in the Wires-X list provided by YSFGateway. Don't forget to exit from Wires-X mode and change to VW mode before to talk:

MMDVMHost <-> YSFGateway <-> YSF2P25 <-> P25Gateway

Also, you can connect directly with MMDVMHost, changing the following ports in [YSF Network] section (YSF2P25.ini):

    DstPort=3200
    LocalPort=4200

MMDVMHost <-> YSF2P25 <-> P25Gateway

You can use the Wires-X function of your radio to select any P25 TG ID. In this case, you need to connect YSF2P25 directly to MMDVMHost in order to process correctly all Wires-X commands. Once you connect to desired P25 TG, you also need to exit from Wires-X mode and change to VW mode before to talk.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.
