#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
# define M_PI		3.14159265358979323846	/* pi */
#endif
#include <dbEvent.h>
#include <dbDefs.h>
#include <dbCommon.h>
#include <aSubRecord.h>
#include <registryFunction.h>
#include <epicsExport.h>

#/* Full "20 bit" range for the raw position demands is (2**20)-2 */
#define RAWPOSRANGE 1048574
/* Maximum number of data points in the wavetable buffer */
#define MAX_BUFF_ITEMS 83333

static long sineWaveData(double offset, double scale, double range, long nItems, int i)
{
   double swval;
   swval =  (RAWPOSRANGE/range) * (offset + (scale * sin((2.0 * i * M_PI)/nItems) ) );    
   return ( (long)(swval < 0 ? swval - 0.5 : swval + 0.5) );
}

static long triangleWaveData(double offset, double scale, double range, long nItems, int i)
{
   double trval, prop, frac;
   prop = (double)(i)/(double)nItems;
   frac = prop > 0.5 ? 1.0 - prop : prop;
   trval =  (RAWPOSRANGE/range) * (frac * 2.0);    
   return ( (long)(trval < 0 ? trval - 0.5 : trval + 0.5) );
}

static long createPeriodicData( aSubRecord *pasub )
{

/*
 * Inputs:
 * pasub->a = Number of output elements  (LONG)
 * pasub->b = Type of data to be generated: 0 = Sine, 1 = Triangle (LONG)
 * pasub->c = Scaling factor             (DOUBLE)
 * pasub->d = Value Offset               (DOUBLE)
 * pasub->e = Full motion range (for scaling) (DOUBLE)
 *
 * Output:
 * pasub->vala = Array of values   (LONG ARRAY))
 *
 */
 
   int i;
   long *pdataArray = (long *)pasub->vala;
   long nItems  = *(long *)pasub->a;
   long ntype   = *(long *)pasub->b;
   double scale = *(double *)pasub->c;
   double offset = *(double *)pasub->d;
   double range  = *(double *)pasub->e; 
   
   printf("Called createPeriodicData\n");
   if (nItems < 0) nItems = 0;

   for (i = 0; i < nItems; i++)
   {
   if (ntype == 0)
     pdataArray[i]  = sineWaveData(offset, scale, range,  nItems, i);
   else
     pdataArray[i]  = triangleWaveData(offset, scale, range,  nItems, i);
 
/*     printf(" Type %ld Periodic data value %d = %ld\n", ntype, i, pdataArray[i]); */
   }

   return(0);
 }
 
static long readWtdataFile( aSubRecord *pasub )
{

/*
 * Inputs:
 * pasub->a = filename (CHAR ARRAY)
 * pasub->b = Full motion range (for scaling) (DOUBLE)
 *
 * Output:
 * pasub->vala = Array of values   (LONG ARRAY)
 * pasub->valb = Number of values  (LONG)
 * pasub->valc = Error string (CHAR ARRAY)
 *
 */
 
 #define STRING_MAX 512
   int numline, stat;
   double engpos, drawpos;
   char line[64];
   char filename[STRING_MAX], errstring[STRING_MAX];
   FILE *fp;
   char *fnfield   =  (char *)pasub->a;
   long fnsize     = pasub->noa;
   double range    = *(double *)pasub->b;
   long *dataArray =  (long *)pasub->vala;
   long arrsize    = pasub->nova;
   long *numvals   = (long *)pasub->valb;
   char *errfield  =  (char *)pasub->valc;
   long errsize    = pasub->novc;

/* Sanity checks & coercion for array size fields */
   fnsize = fnsize > STRING_MAX ? STRING_MAX : fnsize;
   errsize = errsize > STRING_MAX ? STRING_MAX : errsize;
   arrsize = arrsize > MAX_BUFF_ITEMS ? MAX_BUFF_ITEMS : arrsize;
   
/* Open the file read-only, name specified in input string A */
   errno = 0;
   strncpy(filename, fnfield, fnsize);
   fp = fopen(filename, "r");
   if (fp == NULL)
 /* Failed to open data file - get error string and output on VALC */
   {
     strncat(errstring, "File ", errsize);
     strncat(errstring, filename, errsize);
     strncat(errstring, ": ", errsize);
     strncat(errstring, strerror(errno), errsize);
     strncpy(errfield, errstring, errsize);
     return(0);
   }

/* Opened file OK, now read data lines, one double position value per line.
   Values provided in engineering units, converted to raw values for output array */
   errno = 0;
   numline = 0;
   stat = 0;
   *numvals = 0;
   while (fgets(line, 64, fp) != NULL && numline < arrsize)
   {
     stat = sscanf(line, "%lf", &engpos);
     if (stat > 0) /* There was at least 1 value read */
     {
       engpos = engpos > range ? range : engpos; /* Coerce position in Max/Min range */
       engpos = engpos < (-1 * range) ? (-1 * range) : engpos;
       /* Convert to raw units and output nearest integer value in array at VALA */
       drawpos = engpos * (RAWPOSRANGE/range);
       dataArray[numline] = (long)(drawpos < 0 ? drawpos - 0.5 : drawpos + 0.5);
       numline++;
     }
   }
   
   if (ferror(fp) != 0)
 /* I/O error while reading file - get error string and output on VALC */
   {
     strncat(errstring, "File ", errsize);
     strncat(errstring, filename, errsize);
     strncat(errstring, ": ", errsize);
     strncat(errstring, strerror(errno), errsize);
     strncpy(errfield, errstring, errsize);
   }
   
   *numvals = numline;
   return(0);
}
   

epicsRegisterFunction(createPeriodicData);
epicsRegisterFunction(readWtdataFile);

