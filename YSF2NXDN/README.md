# Description

This is the source code of YSF2NXDN, a software for digital voice conversion from Yaesu System Fusion to NXDN digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software.

You can use this software and YSFGateway at the same time, with the default YSF UDP ports (42000 and 42014). In this case, you can select the pseudo "YSF2NXDN" reflector in the Wires-X list provided by YSFGateway:

MMDVMHost <-> YSFGateway <-> YSF2NXDN <-> NXDNGateway

Also, you can connect directly with MMDVMHost, changing the following ports in [YSF Network] section (YSF2NXDN.ini):

    DstPort=3200
    LocalPort=4200

MMDVMHost <-> YSF2NXDN <-> NXDNGateway

You can use the Wires-X function of your radio to select any NXDN TG ID. In this case, you need to connect YSF2NXDN directly to MMDVMHost in order to process correctly all Wires-X commands.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.
