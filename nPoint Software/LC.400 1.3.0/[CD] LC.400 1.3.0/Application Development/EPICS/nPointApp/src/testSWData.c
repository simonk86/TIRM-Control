#include <time.h>
#include <stdio.h>
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

static int counter = 0;

static long swData(double offset, double scale, long nItems, int i)
{
   double swval;
   swval = 1000000.0 * (offset + (scale * sin((2.0 * i * M_PI)/nItems) ) );    
   return ( (long)(swval < 0 ? swval - 0.5 : swval + 0.5) );
}

static long createSwTestData( aSubRecord *pasub )
{

/*
 * Inputs:
 * pasub->a = Number of output elements  (LONG)
 * pasub->b = Scaling factor           (DOUBLE)
 * pasub->c = Offset                   (DOUBLE)
 *
 * Output:
 * pasub->vala = Array of values   (LONG ARRAY))
 *
 */
 
   int i;
   long *swarray = (long *)pasub->vala;
   long nItems = *(long *)pasub->a;
   double scale = *(double *)pasub->b;
   double offset = *(double *)pasub->c;
   
   counter++;
   if (nItems < 0) nItems = 0;
   if (nItems > 8192) nItems = 8192;

   for (i = 0; i < nItems; i++)
   {
     swarray[i]  = swData(offset, scale, nItems, i) + counter;
/*     printf("Data %d = %ld\n", i, swarray[i]); */
   }

   return(0);
 }
 
 static long checkSwTestData( aSubRecord *pasub )
{

/*
 * Inputs:
 * pasub->a = Number of input elements  (LONG)
 * pasub->b = Scaling factor           (DOUBLE)
 * pasub->c = Offset                   (DOUBLE)
 * pasub->d = Array of values       (LONG ARRAY)
 *
 */
 
   int i;
   int check = 1;
   long expected;
   long *swarray = (long *)pasub->d;
   long nItems = *(long *)pasub->a;
   double scale = *(double *)pasub->b;
   double offset = *(double *)pasub->c;
   
   if (nItems < 0) nItems = 0;
   if (nItems > 8192) nItems = 8192;

   for (i = 0; i < nItems; i++)
   {
     expected =  swData(offset, scale, nItems, i) + counter;
     if (swarray[i]  != expected) 
     {
       check = 0;
       printf("Data %d = %ld %ld Check failed\n", i, expected, swarray[i]);
     }
   }
   if (check) printf("checkSwData: Checked %ld itemsOK\n", nItems);
   else
     printf("checkSwData: Checked %ld items: ERRORS\n", nItems);
   return(0);
 }

epicsRegisterFunction(createSwTestData);
epicsRegisterFunction(checkSwTestData);

