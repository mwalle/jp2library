# JP2 remote control library

## Warning
Please note that this program is in a very early stage. You should make a
complete backup of your remote by using the following command:
> jp2dump /dev/ttyUSB0 remote_backup.bin 0x800 0x1f800


jp2library provides a set of tools together with a library to flash and
read out your JP1.4/JP2 remote control.

jp2library is a rewrite of the [jp12serial](http://www.hifi-remote.com/forums)
library. While the jp12serial supports all JP1/JP2 interfaces, the jp2library
only supports the newer JP1.4 and JP2 protocol.

Additionally, a JNI library is provided to be used with RMIR and IR. It aims
to be a drop-in replacement to the jp12serial.dll.

## Building
> cmake .
> make

If the java includes are not found try to specify the JAVA_HOME variable:
> JAVA_HOME=/path/to/jdk cmake .
> make

## Tools

### jp2dump
Dumps the content of your remote control.

### jp2cli
A frontend to all functionalities of the jp2library.

## Technical stuff
 * The JP1.4/JP2 protocol uses an UART interface to communicate with the remote.
 * There are different areas within your remote: the bootloader, the actual
   program of the remote control, IR protocol definitions and an update area.

