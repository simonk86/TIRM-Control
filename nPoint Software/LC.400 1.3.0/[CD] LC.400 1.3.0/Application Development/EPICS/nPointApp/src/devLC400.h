#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <alarm.h>
#include <recGbl.h>
#include <dbAccess.h>
#include <dbDefs.h>
#include <link.h>
#include <epicsPrint.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <cantProceed.h>
#include <dbCommon.h>
#include <dbScan.h>
#include <callback.h>
#include <stringinRecord.h>
#include <stringoutRecord.h>
#include <waveformRecord.h>
#include <menuFtype.h>
#include <recSup.h>
#include <devSup.h>

#include <epicsExport.h>
#include "asynDriver.h"
#include "asynDrvUser.h"
#include "asynOctet.h"
#include "asynInt32.h"
#include "asynFloat64.h"
#include "asynEpicsUtils.h"

typedef struct devPvt
{
  dbCommon                 *precord;
  asynUser                 *pasynUser;
  char                     *portName;
  unsigned int             addr;
  asynOctet                *poctet;
  asynInt32                *pint32;
  asynFloat64              *pfloat64;
  void                     *interfacePvt;
  int                      canBlock;
  char                     *userParam;
  char                     *buffer;
  size_t                   bufSize;
  size_t                   bufLen;
  DBADDR                   dbAddr;
  CALLBACK                 callback;
  IOSCANPVT                ioScanPvt;
  void                     *registrarPvt;
  int                      gotValue;
  interruptCallbackOctet   octetCallback;
  interruptCallbackInt32   int32Callback;
  interruptCallbackFloat64 float64Callback;
} devPvt;

typedef struct commonDset
{
  long number;
  DEVSUPFUN dev_report;
  DEVSUPFUN init;
  DEVSUPFUN init_record;
  DEVSUPFUN get_ioint_info;
  DEVSUPFUN processCommon;
  DEVSUPFUN special_linconv;
} commonDset;

long initCommon(dbCommon *precord,
		DBLINK *plink,
		userCallback callback,
		const char *interfaceType);
void initDrvUser(devPvt *pdevPvt);
void initCmdBuffer(devPvt *pdevPvt);
void initDbAddr(devPvt *pdevPvt);
long processCommon(dbCommon *precord);
asynStatus parseLink(asynUser *pasynUser,
		     DBLINK *plink,
		     char **port,
		     unsigned int *addr,
		     char **userParam);
