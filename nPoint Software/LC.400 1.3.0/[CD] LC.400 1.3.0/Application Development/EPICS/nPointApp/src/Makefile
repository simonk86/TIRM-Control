TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# build an ioc application
PROD_IOC = asynNpointTest

DBD += asynNpointTest.dbd
# powetPMACTest.dbd will be made up from these files:
asynNpointTest_DBD += base.dbd
asynNpointTest_DBD += asyn.dbd
asynNpointTest_DBD += drvAsynFTDIPort.dbd
asynNpointTest_DBD += stream.dbd
asynNpointTest_DBD += devLC400.dbd
asynNpointTest_DBD += asubFunction.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
asynNpointTest_SRCS += asynNpointTest_registerRecordDeviceDriver.cpp
asynNpointTest_SRCS += ftdiDriver.cpp
asynNpointTest_SRCS += devLC400.c
asynNpointTest_SRCS += drvAsynFTDIPort.cpp
asynNpointTest_SRCS += testSWData.c wavetableData.c
asynNpointTest_SRCS_DEFAULT += asynNpointTestMain.cpp
asynNpointTest_SRCS_vxWorks += -nil-

USR_CXXFLAGS += -DHAVE_UNISTD_H -DHAVE_SYS_TIME_H

# Include the following libraries in the build
# Note different names and DIR locations for Linux and WIN32 libraries
asynNpointTest_LIBS += asyn
asynNpointTest_LIBS_WIN32 += ftdi1-mingw
asynNpointTest_LIBS_Linux += ftdi1
asynNpointTest_LIBS_Linux += usb-1.0
asynNpointTest_LIBS_WIN32 += usb-1.0-mingw
asynNpointTest_LIBS += stream
asynNpointTest_LIBS += $(EPICS_BASE_IOC_LIBS)
usb-1.0_DIR = /usr/lib
ftdi1_DIR = /usr/lib
ftdi1-mingw_DIR = ../mingw-lib
usb-1.0-mingw_DIR = ../mingw-lib

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


