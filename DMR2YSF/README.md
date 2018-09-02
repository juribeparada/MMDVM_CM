# Description

This is the source code of DMR2YSF, a software for digital voice conversion from DMR to Yaesu System Fusion digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software.

You can use this software with MMDVMHost and YSFGateway, with the default UDP ports. Select the YSF or FCS target reflector from the Startup reflector in [Network] section, YSFGateway.ini.

MMDVMHost <-> DMR2YSF <-> YSFGateway

You can select a YSF reflector from your DMR radio, associating each radio TG with a specific YSF reflector. Please edit the file TG-YSFList.txt and enter only your preferred DMR ID - YSF reflector mapping list. YSF reflectors are identified by ID numbers, and FCS rooms by names.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.
