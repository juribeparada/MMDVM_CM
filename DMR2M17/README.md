# Description

This is the source code of DMR2M17, a software for digital voice conversion from DMR to M17 digital mode, based on Jonathan G4KLX's [MMDVM](https://github.com/g4klx) software. Unlike the other cross mode utilities upon which this is based, this utility performs software transcoding between Codec2(M17) and AMBE+2 2450x1150(DMR).

You can use this software with MMDVMHost with the default UDP ports:

MMDVMHost(DMR Mode):62032 <-> 62037:DMR2M17:32010 <-> 17000:M17Reflector (mrefd)

This software is licenced under the GPL v2 and is intended for amateur and educational use only. Use of this software for commercial purposes is strictly forbidden.

# PiStar specific notes

An entry needs to be added to /root/DMR_Hosts.txt:
```
DMR2M17				0000	127.0.0.4			none		62037
```
And a custom firewall rule needs to be added by creating a file called /root/ipv4.fw and adding the line:
```
iptables -A OUTPUT -p udp --dport 17000 -j ACCEPT #M17 Outgoing
```

# Building

This utility is not built with the other cross mode ulitities, and has 1 external dependency:

md380_vocoder https://github.com/nostar/md380_vocoder

With this dependency installed, run 'make' from the source directory.
