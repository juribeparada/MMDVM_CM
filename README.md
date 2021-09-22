# Description

This is the source code of several tools for cross-mode conversion for some digital voice protocols, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software.

Please see individual README at each subdirectory for more information.

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.

# Transcoding cross mode utilities

This fork of MMDVM_CM includes transcoding cross mode utilities that use md380 firmware to encode/decode AMBE+2 2450x1150 used by DMR/YSF/NXDN.  Because of this, these utilties must be run on an ARM platform or via an ARM emulator.  RPi 2, 3, 4 are confirmed to work, using both RaspiOS and PiStar (which is based on RaspiOS Lite).  RPi Zero, 1, or other clones are not supported.  DSTAR2xxx utilties are still a work in progress and are not currently functional.  When finished, these utilties will use 1 and only 1 USB AMBE vocoder device for the AMBE+ 2400x1200 along with either md380, imbe, or codec2 vocders for the other side, depending on the mode.

The USRP2xxx utilties connect the various modes to an AllStar node or AllStar enabled repeater via USRP.  These are a work in progress and should be considered experimental.

