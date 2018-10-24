FTDI and USB libraries for Windows MINGW
--------------------------------------

The files ftdi1-mingw.lib and libusb-1.0-mingw.lib are takenfrom the Windows 32 bitlibFTDI MinGW development kit,
available as a zip file at
http://code.google.com/p/picusb/downloads/detail?name=libftdi1-1.0_devkit_mingw32_17Feb2013.zip

The files are found in the "lib" folder of that package, under their original names ftdi1.a and libusb-1.0.a. These are
statically linked archive libraries (.a files) and have been renamed to .lib files in accordance with the conventions
required by the EPICS Windows build (it looks for a .lib file when a Library is specified for linking)


PBT 
11/9/13