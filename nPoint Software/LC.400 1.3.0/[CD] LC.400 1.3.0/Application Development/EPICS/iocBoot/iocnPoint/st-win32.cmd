#!../../bin/win32-x86-mingw/asynNpointTest.exe

< envPaths
epicsEnvSet ("STREAM_PROTOCOL_PATH", "../../proto")
epicsEnvSet ("EPICS_CA_MAX_ARRAY_BYTES","500000")

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/asynNpointTest.dbd")
asynNpointTest_registerRecordDeviceDriver pdbbase

drvAsynFTDIPortConfigure("LC400", "0x403", "0x6010","3000000", "2", "0", "0")

# When the controller is first powered on, after the control loop begins
# running the controller still needs up to 2 seconds to initialize values in memory
epicsThreadSleep 2.0

## Load record instances
dbLoadTemplate("db/nPointTest.substitutions")

# asynSetTraceMask("LC400", -1, 127)
# asynSetTraceIOMask("LC400", -1, 127)

cd ${TOP}/iocBoot/${IOC}
iocInit

