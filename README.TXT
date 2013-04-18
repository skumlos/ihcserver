IHCServer v0.2b

1. Introduction

This software is meant for monitoring and controlling LK (Lauritz Knudsen) A/S's
IHC (Intelligent Home Control) system, or at least the I/Os. It is only tested
with the first Visual controller (the one with the RS-232 port on the front).

It works by monitoring the RS-485 bus that is available on the controller, also
used for the voice modem and other LK IHC peripherals.

The controller issues a command addressed to the PC which gives "permission" to
act. IHCServer then issues commands back to the controller. IHCServer updates
its models of the I/O, alternating between updating INPUT, then OUTPUT on the 
next permission. If there are any pending requests, like setting an output,
IHCServer will issue these, and then continue updating.

2. Software overview

The software consists of 2 main parts, a IHC interface which does the actual
talking to the controller, and the server that can be communicated with via
sockets and JSON encoded packets. For the JSON packet stuff, the cajun JSON
C++ library is used. The library is isolated under the 3rdparty directory.

The software is written for Linux, and requires pthreads and openssl for 
running. It has been tested on Ubuntu 12.10 on a Avalue FPC-08W04 that has 
onboard RS-485.

3. IHC Protocol (AFAIK)

A packet consists of an STX (0x02), an address (fx. 0x1C, for PC), a command 
(like 0x30 for permission), possible data packet depending on the command, 
ETB (0x17) and a CRC that are all the bytes in a packet added up (in 1 byte,
meaning the value should be AND'ed with 0xFF).

Both the output and the input packets data consists of 16 data bytes, where 
each data byte in an output packet represent the state of an output module 
(since an output module has 8 outputs). Thus getting the state of an output 
is as simple as AND'ing the byte value with 0x1 shifted left the output 
number times, eg. BYTEVALUE & (0x1 << OUTPUTNUMBER).

The inputs are the same, except one input module has 16 inputs, thus two bytes
represent a modules inputs state.  

4. Sources

http://www.itp.uzh.ch/~dpotter/howto/daemonize
http://ihcsoftwarekit.sourceforge.net
http://sourceforge.net/projects/cajun-jsonapi
http://beej.us/guide/bgnet/
http://www.tooms.dk (Web forum)

5. Changelog

Since 0.2:

Fixed module toggling

Since 0.1:

Added description layer so I/O's can be named.

Fixed obvious error in receiver thread with message size calculation.

Fixed potential segfaults regarding iterators in IHCServer.

Added alarm and userlevel stuff.

Added ID stuff to isolate users connected.

Other small fixes and improvements.


2013, Martin Hejnfelt (martin@hejnfelt.com)
