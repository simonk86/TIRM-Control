/*
 * File:   devLC400.c
 *
 * Date:   19th July 2013
 * Author: Philip Taylor (Observatory Sciences Ltd)
 * Email:  pbt@observatorysciences.co.uk
 *
 * Description:
 *
 * This file contains the device support code for writing a data array
 * to the nPoint LC.400 Controller.  This device support
 * requires the asyn module to establish communications.
 *
 */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>

#include <alarm.h>
#include <recGbl.h>
#include <dbAccess.h>
#include <dbDefs.h>
#include <link.h>
#include <epicsPrint.h>
#include <epicsMutex.h>
#include <epicsString.h>
#include <epicsThread.h>
#include <cantProceed.h>
#include <dbCommon.h>
#include <dbScan.h>
#include <callback.h>
#include <stringinRecord.h>
#include <stringoutRecord.h>
#include <longinRecord.h>
#include <longoutRecord.h>
#include <aiRecord.h>
#include <aoRecord.h>
#include <biRecord.h>
#include <boRecord.h>
#include <mbbiRecord.h>
#include <mbboRecord.h>
#include <waveformRecord.h>
#include <aSubRecord.h>
#include <menuFtype.h>
#include <recSup.h>
#include <devSup.h>

#include <registryFunction.h>
#include <epicsExport.h>
#include "asynDriver.h"
#include "asynDrvUser.h"
#include "asynOctet.h"
#include "asynEpicsUtils.h"

#include "devLC400.h"
/* #include "LC400.h" */

#define IS_LITTLE_ENDIAN  1
#define IS_BIG_ENDIAN     0
#define MAX_ARRAY_IO  100000

/* General purpose function declarations */
static asynStatus writeIt(asynUser *pasynUser, const char *message, size_t nbytes);
static asynStatus readIt(asynUser *pasynUser, char *message, size_t maxBytes, size_t *nBytesRead);
static asynStatus flushIt(asynUser *pasynUser);
static void finish(dbCommon *precord);
static char *skipWhite(char *pstart, int commaOk);
static int endian();
static void npointFormatMess(unsigned char messtype, unsigned char *buffer, uint32_t address, int32_t value);
static void npointFormatWriteNext(unsigned char *buffer, int32_t value);

/* Define the functions for initialising and callback of the waveform records */
static long initWfWrite(waveformRecord *pwf);
static void callbackWfWrite(asynUser *pasynUser);
static long initWfRead(waveformRecord *pwf);
static void callbackWfRead(asynUser *pasynUser);

commonDset asynWfLC400Write        = {5, 0, 0, initWfWrite,      0, processCommon};
commonDset asynWfLC400Read         = {5, 0, 0, initWfRead,       0, processCommon};

epicsExportAddress(dset, asynWfLC400Write);
epicsExportAddress(dset, asynWfLC400Read);

/*
 * Function: writeIt
 *
 * Parameters: pasynUser - Pointer to the asynUser structure
 *             message   - Pointer to the string to write
 *             nbytes    - Number of characters (1 byte each) to write
 *
 * Returns: asynStatus success value
 * 
 * Description:
 *
 * Used to write a string to the driver via asynDriver support code.
 * A status value is returned accordingly.
 */
static asynStatus writeIt(asynUser *pasynUser,const char *message,size_t nbytes)
{
  devPvt     *pdevPvt = (devPvt *)pasynUser->userPvt;
  dbCommon   *precord = pdevPvt->precord;
  asynOctet  *poctet = pdevPvt->poctet;
  void       *octetPvt = pdevPvt->interfacePvt;
  asynStatus status;
  size_t     nbytesTransferred;
  
  /* Make the call into asyn for writing the message */
  status = poctet->write(octetPvt,pasynUser,message,nbytes,&nbytesTransferred);
  if(status!=asynSuccess) {
    /* An error has occurred, use the asynDriver print method to notify */
    asynPrint(pasynUser,ASYN_TRACE_ERROR,
	      "%s devAsynOctet: writeIt failed %s\n",
	      precord->name,pasynUser->errorMessage);
    recGblSetSevr(precord, WRITE_ALARM, INVALID_ALARM);
    return status;
  }
  if(nbytes != nbytesTransferred) {
    /* An incorrect number of bytes has been written, again raise an error */
    asynPrint(pasynUser,ASYN_TRACE_ERROR,
	      "%s devAsynOctet: writeIt requested %d but sent %d bytes\n",
	      precord->name,nbytes,nbytesTransferred);
    recGblSetSevr(precord, WRITE_ALARM, MINOR_ALARM);
    return asynError;
  }
  asynPrintIO(pasynUser,ASYN_TRACEIO_DEVICE,message,nbytes,
	      "%s devAsynOctet: writeIt %d\n",precord->name, nbytes);
  return status;
}

/*
 * Function: readIt
 *
 * Parameters: pasynUser - Pointer to the asynUser structure
 *             message   - Pointer to where the string will be stored
 *             maxBytes  - Maximum number of bytes the above string can hold
 *             nbytes    - Number of characters (1 byte each) that have been read
 *
 * Returns: asynStatus success value
 * 
 * Description:
 *
 * Reads a string back from the driver via asynDriver support code.
 * A status value is returned accordingly.
 */
static asynStatus readIt(asynUser *pasynUser, char *message,
        size_t maxBytes, size_t *nBytesRead)
{
  devPvt     *pdevPvt = (devPvt *)pasynUser->userPvt;
  dbCommon   *precord = pdevPvt->precord;
  asynOctet  *poctet = pdevPvt->poctet;
  void       *octetPvt = pdevPvt->interfacePvt;
  asynStatus status;
  int        eomReason;
  
  /* Make the call into asyn for reading the message */
  status = poctet->read(octetPvt,pasynUser,message,maxBytes,nBytesRead,&eomReason);
  if(status!=asynSuccess) {
    /* An error has occurred, use the asynDriver print method to notify */
    asynPrint(pasynUser,ASYN_TRACE_ERROR,
	      "%s devAsynOctet: readIt failed %s\n",
	      precord->name,pasynUser->errorMessage);
    recGblSetSevr(precord, READ_ALARM, INVALID_ALARM);
    return status;
  }
  asynPrintIO(pasynUser,ASYN_TRACEIO_DEVICE,message, *nBytesRead,
	      "%s devAsynOctet: readIt %d\n",precord->name, *nBytesRead);
  return status;
}

/*
 * Function: flushIt
 *
 * Parameters: pasynUser - Pointer to the asynUser structure
 *
 * Returns: asynStatus success value
 * 
 * Description:
 *
 * Attempts to flush the connection made through the asynDriver.
 * A status value is returned accordingly.
 */
static asynStatus flushIt(asynUser *pasynUser)
{
  devPvt     *pdevPvt = (devPvt *)pasynUser->userPvt;
  asynOctet  *poctet = pdevPvt->poctet;
  void       *octetPvt = pdevPvt->interfacePvt;
  asynStatus status;

  /* Make the call into asyn for flushing the connection */
	status = poctet->flush(octetPvt, pasynUser);
	return status;
}

/*
 * Function: finish
 *
 * Parameters: pr - Pointer to a record structure
 *
 * Returns: void
 * 
 * Description:
 *
 * Completes processing of a record.
 */
static void finish(dbCommon *pr)
{
  devPvt     *pPvt = (devPvt *)pr->dpvt;
  if(pr->pact) callbackRequestProcessCallback(&pPvt->callback,pr->prio,pr);
}

/*
 * Function: initWfRead
 *
 * Parameters: pwf - Pointer to a waveform record structure
 *
 * Returns: 0
 * 
 * Description:
 *
 * Initialises waveform record, registers the process callback function.
 * Initialises the database address and the drvUser structure.
 */
static long initWfRead(waveformRecord *pwf)
{
  asynStatus status;
  devPvt     *pdevPvt;
  
  status = initCommon((dbCommon *)pwf,&pwf->inp,callbackWfRead,asynOctetType);
  if(status!=asynSuccess) return 0;
  pdevPvt = (devPvt *)pwf->dpvt;
  initDbAddr(pdevPvt);
  initDrvUser(pdevPvt);
  return 0;
}

/*
 * Function: callbackWfRead
 *
 * Parameters: pasynUser - Pointer to the asynUser structure
 *
 * Returns: void
 * 
 * Description:
 *
 * Called from the asynDriver.  Record has processed so call
 * writeIt to issue a read array request and then readIt to 
 * read the data array.
 */
 
static void callbackWfRead(asynUser *pasynUser)
{
  devPvt         *pdevPvt = (devPvt *)pasynUser->userPvt;
  waveformRecord *pwf = (waveformRecord *)pdevPvt->precord;
  asynStatus     status;
  char *readDataBuff;
  unsigned char  readArrayComm[10];
  size_t         nelm, totBytesToRead, arrBytesToRead, bytesRead;
  int            i, itemsPerElement;

      /* Supported data types are LONG, ULONG and DOUBLE (only) */
      if (pwf->ftvl != DBF_LONG && pwf->ftvl != DBF_ULONG && 
          pwf->ftvl != DBF_DOUBLE)
      {  asynPrint(pasynUser,ASYN_TRACE_ERROR,
	      "%s Unsupported FTVL: %d\n",pwf->name, pwf->ftvl);
         recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
         finish((dbCommon *)pwf);
         return;
      }
      
      /* Check the INP field is not empty */
     if (pdevPvt->addr <= 0 ){
      /* Invalid INP field, invalid memory location */
      asynPrint(pasynUser,ASYN_TRACE_ERROR,
	      "%s error, invalid inp\n",pwf->name);
      asynPrint(pasynUser,ASYN_TRACE_ERROR,
	      "addr = %x\n",pdevPvt->addr);
      recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
      finish((dbCommon *)pwf);
      return;
  } else {
        /* Flush the connection to remove any stale data */
	status = flushIt(pasynUser);
 
        /* Size of data array to read is NELM items of type FTVL*/
        nelm = pwf->nelm;
        arrBytesToRead = nelm * dbValueSize(pwf->ftvl);
        itemsPerElement = (pwf->ftvl == DBF_DOUBLE) ? 2 : 1;
/* Data returned is N * 32-bit item array + 5 bytes preamble + 1 byte terminator */
        totBytesToRead = arrBytesToRead + 6;
        readDataBuff = (char *)calloc(totBytesToRead, 1);
	/* Request read array by sending an A4 message Read Array command. Element
	   count in the message is in units of 32-bit items */
	npointFormatMess(0xA4, readArrayComm, pdevPvt->addr, nelm * itemsPerElement);
	status = writeIt(pasynUser, (char *)readArrayComm, sizeof(readArrayComm));
        if (status == asynSuccess){
             /* Now read in the data array */
             status = readIt(pasynUser, readDataBuff, totBytesToRead, &bytesRead);
             if (status == asynSuccess && bytesRead == totBytesToRead)
             {
             /* Check that the first 5 bytes preamble is that sent with the Read Array command */
             for (i = 0; i < 5; i++)
             {
               if ((unsigned char)readDataBuff[i] != readArrayComm[i])
               {
                 asynPrint(pasynUser,ASYN_TRACE_ERROR,
                 "asynWfLC400Read error: Unexpected character in data = %0x\n", 
                  (unsigned char) readDataBuff[i]);
                  recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
                  free(readDataBuff);
                  finish((dbCommon *)pwf);
	          return;
               }
             }
             /* Check that the data was terminated with a '0x55' character */
             if (readDataBuff[bytesRead-1] != 0x55)
             {
               asynPrint(pasynUser,ASYN_TRACE_ERROR,
	                 "asynWfLC400Read error: Unexpected terminator %0x\n",
	                  readDataBuff[bytesRead-1]);
               recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
               free(readDataBuff);
               finish((dbCommon *)pwf);
	       return;
	     }            
             /* Data OK, so copy the array data out to the waveform record buffer. 
                Skip the first 5 bytes preamble & terminator byte  */
              memcpy(pwf->bptr, readDataBuff+5, arrBytesToRead);
              pwf->udf = 0;
              pwf->rarm = 0;
              pwf->nord = nelm;
             } else {
               asynPrint(pasynUser,ASYN_TRACE_ERROR,
	                 "asynWfLC400Read error: Status %d reading array data. Read %d bytes, expected %d bytes\n",
	                  status, bytesRead, totBytesToRead);
               recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
               free(readDataBuff);
               finish((dbCommon *)pwf);
               return;
             }
       }
       else {
             asynPrint(pasynUser,ASYN_TRACE_ERROR,
	               "asynWfLC400Read error: Status %d writing Read Array command\n",
	                status);
             recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
             free(readDataBuff);
             finish((dbCommon *)pwf);
             return;
            }
       
    }
  /* Finish processing the record. */
  free(readDataBuff);
  finish((dbCommon *)pwf);
  return;
}

/*
 * Function: initWfWrite
 *
 * Parameters: pwf - Pointer to a waveform record structure
 *
 * Returns: 0
 * 
 * Description:
 *
 * Initialises waveform record, registers the process callback function.
 * Initialises the database address and the drvUser structure.
 */
static long initWfWrite(waveformRecord *pwf)
{
  asynStatus status;
  devPvt     *pdevPvt;
  
  status = initCommon((dbCommon *)pwf,&pwf->inp,callbackWfWrite,asynOctetType);
  if(status!=asynSuccess) return 0;
  pdevPvt = (devPvt *)pwf->dpvt;
  initDbAddr(pdevPvt);
  initDrvUser(pdevPvt);
  return 0;
}

/*
 * Function: callbackWfWrite
 *
 * Parameters: pasynUser - Pointer to the asynUser structure
 *
 * Returns: void
 * 
 * Description:
 *
 * Called from the asynDriver.  Record has processed so call
 * writeIt to issue a GET command and then readIt to get the response.
 */
static void callbackWfWrite(asynUser *pasynUser)
{
  devPvt         *pdevPvt = (devPvt *)pasynUser->userPvt;
  waveformRecord *pwf = (waveformRecord *)pdevPvt->precord;
  asynStatus     status;
  int32_t        *wfDataBuff;
  unsigned char  *commBuff, *ptr;
  unsigned char  writeSingleBuff[10];
  int            nbuff = 0;
  int            itemsPerElement, wfDataBytes, commBuffBytes;

      /* Supported data types are LONG, ULONG and DOUBLE (only) */
      if (pwf->ftvl != DBF_LONG && pwf->ftvl != DBF_ULONG && 
          pwf->ftvl != DBF_DOUBLE)
      {  asynPrint(pasynUser,ASYN_TRACE_ERROR,
	      "%s Unsupported FTVL: %d\n",pwf->name, pwf->ftvl);
         recGblSetSevr(pwf,WRITE_ALARM,INVALID_ALARM);
         finish((dbCommon *)pwf);
         return;
      }

	/* Check the INP field is not empty */
      if (pdevPvt->addr <= 0 ){
      /* Invalid INP field, invalid memory location */
         asynPrint(pasynUser,ASYN_TRACE_ERROR,
	      "%s error, invalid inp\n",pwf->name);
         asynPrint(pasynUser,ASYN_TRACE_ERROR,
	      "addr = %x\n",pdevPvt->addr);
         recGblSetSevr(pwf,WRITE_ALARM,INVALID_ALARM);
         finish((dbCommon *)pwf);
         return;
  } else {
	  /* Flush the connection to remove any stale data */
	  status = flushIt(pasynUser);
          itemsPerElement = (pwf->ftvl == DBF_DOUBLE) ? 2 : 1;
          wfDataBytes = pwf->nelm * dbValueSize(pwf->ftvl);
          /* Allocate memory & get the data from the record buffer */
          wfDataBuff = (int32_t *)calloc(wfDataBytes, 1);
          memcpy(wfDataBuff, pwf->bptr, wfDataBytes);
	  /* Send initial data. Start with an A2 message Write Single Location command */
	  npointFormatMess(0xA2, writeSingleBuff, pdevPvt->addr, wfDataBuff[0]);
	  status = writeIt(pasynUser, (char *)writeSingleBuff, sizeof(writeSingleBuff));
	  /* Allocate memory for command buffer. 6 byte Write Next command for each item written */
	  commBuffBytes = pwf->nelm * itemsPerElement * 6;
	  commBuff = (unsigned char *)calloc(commBuffBytes, 1);
	  ptr = commBuff;
	  /* Set up buffered A3 messages to write the rest of the words with Write Next commands */
	  for (nbuff=1; nbuff < (pwf->nelm * itemsPerElement); nbuff++)
	  {
	    npointFormatWriteNext(ptr, wfDataBuff[nbuff]);
	    ptr += 6;
	  }
/*        asynPrint(pasynUser,ASYN_TRACE_ERROR,"%d/%d  %d\n", nbuff+1, pwf->nelm * itemsPerElement, wfDataBuff[nbuff]); */
          /* Write out all the Write Next commands in the buffer */
	  status = writeIt(pasynUser, (char *)commBuff, commBuffBytes);

        if (status==asynSuccess){
            pwf->udf = 0;
            pwf->rarm = 0;
            pwf->nord = nbuff;
        } else {
          recGblSetSevr(pwf,WRITE_ALARM,INVALID_ALARM);
          free(wfDataBuff);
          free(commBuff);
          finish((dbCommon *)pwf);
          return;
        }
  }

  /* Finish processing the record. */
  free(wfDataBuff);
  free(commBuff);
  finish((dbCommon *)pwf);
}


/*
 * Function: initCommon
 *
 * Parameters: precord       - Pointer to the record structure
 *             plink         - Pointer to the DBLINK structure
 *             callback      - Function called when record processed
 *             interfaceType - Pointer to string val of interface type
 *
 * Returns: asynStatus success value
 * 
 * Description:
 *
 * Common initialisation for all records.  Create the asynUser structure.
 * Parse the input for the address number.  Attempt to connect and find 
 * the interface.
 */
long initCommon(dbCommon *precord,
		DBLINK *plink,
		userCallback callback,
		const char *interfaceType)
{
  devPvt        *pdevPvt;
  asynStatus    status;
  asynUser      *pasynUser;
  asynInterface *pasynInterface;
  commonDset    *pdset = (commonDset *)precord->dset;

  pdevPvt = callocMustSucceed(1, sizeof(*pdevPvt), "initCommon");
  precord->dpvt = pdevPvt;
  pdevPvt->precord = precord;

  /* Create the asynUser */
  pasynUser = pasynManager->createAsynUser(callback, 0);
	pasynUser->timeout = 0.1;
  pasynUser->userPvt = pdevPvt;
  pdevPvt->pasynUser = pasynUser;

  /*
   * Parse the link for the Port number.  Should be IO_INST and start
   * with @<port> S<n> <user info>.
   */
  status = parseLink(pasynUser,
		     plink,
		     &pdevPvt->portName,
		     &pdevPvt->addr,
		     &pdevPvt->userParam);
                
  if (status != asynSuccess){
    asynPrint(pasynUser,ASYN_TRACE_ERROR,
	  	"%s Error in link %s\n", precord->name, pasynUser->errorMessage);
    goto bad;
  }

  /* Connect to the device */
  status = pasynManager->connectDevice(pasynUser,
				       pdevPvt->portName,
				       pdevPvt->addr);

  if (status != asynSuccess){
    asynPrint(pasynUser,ASYN_TRACE_ERROR,
			"%s initCommon connectDevice failed %s\n",
			precord->name,
			pasynUser->errorMessage);
		goto bad;
  }

  /* Find and set any interfaces */
  pasynInterface = pasynManager->findInterface(pasynUser, interfaceType, 1);
  if (!pasynInterface){
    asynPrint(pasynUser,ASYN_TRACE_ERROR,
			"%s initCommon interface %s not found\n",
			precord->name,
			interfaceType);
    goto bad;
  }

  if (strcmp(interfaceType, asynOctetType) == 0){
      pdevPvt->poctet = pasynInterface->pinterface;
  } else if (strcmp(interfaceType, asynInt32Type) == 0){
      pdevPvt->pint32 = pasynInterface->pinterface;
  } else if (strcmp(interfaceType, asynFloat64Type) == 0){
      pdevPvt->pfloat64 = pasynInterface->pinterface;
  }

  pdevPvt->interfacePvt = pasynInterface->drvPvt;

  /* Determine if device can block */
  pasynManager->canBlock(pasynUser, &pdevPvt->canBlock);
  if (pdset->get_ioint_info){
    scanIoInit(&pdevPvt->ioScanPvt);
  }

  return 0;

	bad:
  	precord->pact = 1;
  	return -1;
}

/*
 * Function: initDrvUser
 *
 * Parameters: pdevPvt - Pointer to the device structure
 *
 * Returns: void
 * 
 * Description:
 *
 * Initialises the drvUser structure.
 */
void initDrvUser(devPvt *pdevPvt)
{
  asynUser *pasynUser = pdevPvt->pasynUser;
  asynStatus status;
  asynInterface *pasynInterface;
  dbCommon *precord = pdevPvt->precord;

  pasynInterface = pasynManager->findInterface(pasynUser, asynDrvUserType, 1);
  if (pasynInterface && pdevPvt->userParam){
    asynDrvUser *pasynDrvUser;
    void *drvPvt;

    pasynDrvUser = (asynDrvUser *)pasynInterface->pinterface;
    drvPvt = pasynInterface->drvPvt;
    status = pasynDrvUser->create(drvPvt, pasynUser, pdevPvt->userParam, 0, 0);
    if (status != asynSuccess){
      asynPrint(pasynUser,ASYN_TRACE_ERROR,
				"%s devAnc350 drvUserCreate failed %s\n",
				precord->name,
				pasynUser->errorMessage);
    }
  }
}

/*
 * Function: initDrvUser
 *
 * Parameters: pdevPvt - Pointer to the device structure
 *
 * Returns: void
 * 
 * Description:
 *
 * Initialises the drvUser structure.
 */
void initDbAddr(devPvt *pdevPvt)
{
  char userParam[64];
  dbCommon *precord = pdevPvt->precord;

  strcpy(userParam, precord->name);
  strcat(userParam, ".VAL");

  if (dbNameToAddr(userParam, &pdevPvt->dbAddr)){
    asynPrint(pdevPvt->pasynUser,ASYN_TRACE_ERROR,
			"%s initDbAddr record %s not present\n",
			precord->name,
			userParam);
    precord->pact = 1;
  }
}

/*
 * Function: processCommon
 *
 * Parameters: precord - Pointer to a record structure
 *
 * Returns: long status.
 * 
 * Description:
 *
 * This function is called whenever one of the records is processed.
 * The callback request is queued so that the whole system doesn't 
 * block.
 */
long processCommon(dbCommon *precord)
{
  devPvt *pdevPvt = (devPvt *)precord->dpvt;
  asynStatus status;

  if (!pdevPvt->gotValue && precord->pact == 0){
    if (pdevPvt->canBlock) precord->pact = 1;
    /* Request the callback be put on the queue */
    status = pasynManager->queueRequest(pdevPvt->pasynUser,
					asynQueuePriorityMedium,
					0.0);
    if ((status == asynSuccess) && pdevPvt->canBlock) return 0;
    if (pdevPvt->canBlock) precord->pact = 0;
    if (status != asynSuccess){
      /* Bad status, the queueing failed, raise an error */
      asynPrint(pdevPvt->pasynUser, ASYN_TRACE_ERROR,
				"%s processCommon error queuing request %s\n",
				precord->name,
				pdevPvt->pasynUser->errorMessage);
      recGblSetSevr(precord, READ_ALARM, INVALID_ALARM);
    }
  }  
  if (!strcmp("ai", precord->rdes->name)){
    return 2;
  }
  return 0;
}

/*
 * Function: parseLink
 *
 * Parameters: pasynUser - pointer to the asynUser structure
 *             plink     - pointer to a DBLINK structure
 *             port      - pointer to pointer of a string
 *             addr      - unused
 *             userParam - pointer to pointer of user param string
 *
 * Returns: asynStatus
 * 
 * Description:
 *
 * Breaks down the user input string into its various components.
 */
asynStatus parseLink(asynUser *pasynUser,
		     DBLINK *plink,
		     char **port,
		     unsigned int *addr,
		     char **userParam)
{
  struct instio *pinstio;
  int len, nscan;
  char *p;
  char *pnext;

  assert(addr && port && userParam);
  *addr = 0;
  *port = NULL;
  *userParam = NULL;

  /* Determine type of link, we are only interested in INST_IO */
  switch (plink->type)
    {
    case INST_IO:
      pinstio = (struct instio*)&(plink->value);
			p = pinstio->string;
			pnext = p;
      /* The next part should contain the port */
      for (len = 0; *p && !isspace(*p) && (*p != ','); len++, p++){}
      if (*p == 0) goto error;
      /* Allocate enough memory to hold the full port name */
      *port = mallocMustSucceed(len+1, "parseLink");
      (*port)[len] = 0;
      strncpy(*port, pnext, len);

      /* The next part should contain the address and number of items to be written */
      for (len = 0; *p && !isspace(*p) && (*p != ','); len++, p++){}
      if (*p == 0) goto error;
      pnext = p;
      if ( (nscan=sscanf(pnext, "%x", addr)) != 1) goto error;

      /* Rest of string can be considered user params */
      if (userParam) *userParam = 0;
      p++;
      if (*p){
				p = skipWhite(p, 0);
				if (userParam && *p){
	  			len = strlen(p);
	  			*userParam = mallocMustSucceed(len+1, "devAsynCommonPmac");
					strncpy(*userParam, p, len);
					(*userParam)[len] = 0;
				}
			}
      break;

    error:
      /* We've hit an error so print out an explanation of why */
      epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
		    "invalid INST_IO Must be @PORT ADDR");
      return asynError;
      break;

    default:
      /* This is no good we want an INST_IO */
      epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
		    "Link must be INST_IO");
      return asynError;
      break;
    }

  return asynSuccess;
}

/*
 * Function: skipWhite
 *
 * Parameters: pstart  - pointer to start of string we a searching through
 *             commaOk - flag to say if we count commas or not
 *
 * Returns: pointer to a character
 * 
 * Description:
 *
 * Simple helper function to skip through any whitespace and optionally
 * commas.
 */
static char *skipWhite(char *pstart, int commaOk)
{
  char *p = pstart;
  while (*p && (isspace(*p) || (commaOk && (*p==',')))) p++;
  return p;
}

static void npointFormatMess(unsigned char messtype, unsigned char *buffer, uint32_t address, int32_t value)
{ 
  /* Set up single location write A2 message with specified address OR
     read array A4 message with number of items specified as value
  */ 
     buffer[0] =  messtype;
     if (endian() == IS_LITTLE_ENDIAN)
     {
       buffer[1] =  address & 0x000000ff;
       buffer[2] =  (address & 0x0000ff00) >> 8;
       buffer[3] =  (address & 0x00ff0000) >> 16;
       buffer[4] =  (address & 0xff000000) >> 24;
       buffer[5] =  value & 0x000000ff;
       buffer[6] =  (value & 0x0000ff00) >> 8;
       buffer[7] =  (value & 0x00ff0000) >> 16;
       buffer[8] =  (value & 0xff000000) >> 24;
     }
     else
     {
       memcpy(buffer+1, &address, sizeof(uint32_t));
       memcpy(buffer+5, &value,   sizeof(int32_t));
     }
     buffer[9] = 0x55;  
}

static void npointFormatWriteNext(unsigned char *buffer, int32_t value)
{ 
  /* Set up Write Next command A3 message  */ 
     buffer[0] = 0xA3;
     if (endian() == IS_LITTLE_ENDIAN)
     {
       buffer[1] =  value & 0x000000ff;
       buffer[2] =  (value & 0x0000ff00) >> 8;
       buffer[3] =  (value & 0x00ff0000) >> 16;
       buffer[4] =  (value & 0xff000000) >> 24;
     }
     else
     {
       memcpy(buffer+1, &value, sizeof(int32_t));
     }
     buffer[5] = 0x55;  
}

static int endian()
{
    int x = 1;
    if(*(char *)&x == 1)
      return IS_LITTLE_ENDIAN;
    else	
      return IS_BIG_ENDIAN;
}

