# Description

This is the source code of DMR2P25, a software for digital voice conversion from DMR to P25 Phase 1 digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software. Unlike the other cross mode utilities upon which this is based, this utility performs software transcoding between IMBE 4400x2800(P25) and AMBE+2 2450x1150(DMR).

You can use this software with MMDVMHost and P25Gateway, with the default UDP ports:

MMDVMHost(DMR Mode):62032 <-> 62037:DMR2P25:32010 <-> 42020:P25Gateway:42010 <-> (P25Reflector)

Program your DMR radio with P25 TG numbers.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.

# Building

This utility is not built with the other cross mode ulitities, and has 2 external dependencies:

imbe_vocoder https://github.com/nostar/imbe_vocoder

md380_vocoder https://github.com/nostar/md380_vocoder

With these dependencies installed, run 'make' from the source directory.
