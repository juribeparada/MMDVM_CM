# Description

This is the source code of M172YSF, a software for digital voice conversion from M17 to YSF digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software.  Unlike the other cross mode utilities upon which this is based, this utility performs software transcoding between Codec2(M17) and AMBE+2 2450(YSF).

This can be used to connect a YSF reflector to an M17 Reflector

M17Reflector <-> M172YSF <-> YSFReflector

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.

# Building
This utility is not built with the other cross mode ulitities, and has an external dependency:

md380_vocoder https://github.com/nostar/md380_vocoder

md380_vocoder uses md380 firmware for vocoding, so this software needs to be run on an ARM based platform i.e. raspberri pi.

