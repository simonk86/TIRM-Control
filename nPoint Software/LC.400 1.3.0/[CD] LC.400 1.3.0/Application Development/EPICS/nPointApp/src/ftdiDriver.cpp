/********************************************
 *  ftdiDriver.cpp
 * 
 *  Wrapper class for the libftdi1 library
 *  This class provides standard read/write
 *  and flush methods for an FTDI chip USB connection.
 * 
 *  Philip Taylor
 *   8 Jul 2013
 * 
 ********************************************/

#include "ftdiDriver.h"
#include "epicsThread.h"
#ifndef _MINGW
#ifdef _WIN32
int gettimeofday(struct timeval *tv, struct timezone *tz);
void usleep(__int64 usec);
#endif
#endif

/*
 * Uncomment the DEBUG define and recompile for lots of
 * driver debug messages.
 */

/*
#define DEBUG 1
*/

#ifdef DEBUG
#define debugPrint printf
#else
void debugPrint(...){}
#endif


/**
 * Constructor for the FTDI driver.  Accepts a Vendor ID and
 * Product ID. Initializes internal variables.
 *
 * @param vendor - Vendor ID
 * @param product - Product ID
 */
FTDIDriver::FTDIDriver()
{
  static const char *functionName = "FTDIDriver::FTDIDriver";
  debugPrint("%s : Method called\n", functionName);

  // Initialize internal FTDI parameters
  connected_ = 0;
  // Vendor and Product ID set to FTDI FT2232H default values
  vendor_  = 0x0403;
  product_ = 0x6010;
  // Baudrate and latency set to 12 Mb and 2 msecs.
  baudrate_ = 12000000;
  latency_ = 2;
}

/**
 * Setup the Vendor ID and Product ID for the USB device.
 *
 * @param vendor - Vendor ID
 * @param product - Product ID
 * @return - Success or failure.
 */
FTDIDriverStatus FTDIDriver::setVPID(const int vendor, const int product)
{
  static const char *functionName = "FTDIDriver::setVPID";
  debugPrint("%s : Method called\n", functionName);

  // Store the Vendor ID
  vendor_ = vendor;

  // Store the Product ID
  product_ = product;

  return FTDIDriverSuccess;
}

/**
 * Setup the baudrate.
 *
 * @param baudrate - Baud rate
 * @return - Success or failure.
 */
FTDIDriverStatus FTDIDriver::setBaudrate(const int baudrate)
{
  static const char *functionName = "FTDIDriver::setBaudrate";
  debugPrint("%s : Method called\n", functionName);

  // Store the Baudrate
  baudrate_ = baudrate;

  return FTDIDriverSuccess;
}

/**
 * Setup the latency.
 *
 * @param latency - FTDI chip latency time. The latency timer counts from the
 * last time data was sent back to the host. If the latency timer expires,
 * the device will send what data it has available to the host regardless
 * of how many bytes it is waiting on. The latency timer will then reset
 * and begin counting again. The default value for the latency timer is 16ms.
 *
 * @return - Success or failure.
 */
FTDIDriverStatus FTDIDriver::setLatency(const int latency)
{
  static const char *functionName = "FTDIDriver::setLatency";
  debugPrint("%s : Method called\n", functionName);

  // Store the latency value
  latency_ = latency;

  return FTDIDriverSuccess;
}

/**
 * Attempt to create a connection  Once the connection has
 * been established .
 *
 * @return - Success or failure.
 */
FTDIDriverStatus FTDIDriver::connectFTDI()
{
  struct ftdi_version_info version;
  int f;
  unsigned int chipid;
  static const char *functionName = "FTDIDriver::connect";
  debugPrint("%s : Method called\n", functionName);

  version = ftdi_get_library_version();
  printf("Initialized libftdi %s (major: %d, minor: %d, micro: %d, snapshot ver: %s)\n",
    version.version_str, version.major, version.minor, version.micro, version.snapshot_str);

  // Allocate and initialize a new ftdi context
  if ((ftdi_ = ftdi_new()) == NULL)
  {
     debugPrint("ftdi_new failed: %s\n", ftdi_get_error_string(ftdi_));
     return FTDIDriverError;
  }

  // Select interface. In this case just use "ANY"
  if ((f = ftdi_set_interface(ftdi_, INTERFACE_ANY)) != 0)
  {
     debugPrint("ftdi_set_interface failed: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
     return FTDIDriverError;
  }
  
  // Open the FTDI USB device
  if ((f = ftdi_usb_open(ftdi_, vendor_, product_)) != 0)
  {
     debugPrint("Failed to open FTDI device: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
     return FTDIDriverError;
  }

  // Read out FTDI chip ID
  debugPrint("ftdi->type = %d. ", ftdi_->type);
  ftdi_read_chipid(ftdi_, &chipid);
  printf("FTDI chipid: %X\n", chipid);
  
  // Set baudrate
  if ((f = ftdi_set_baudrate(ftdi_, baudrate_)) != 0)
  {
      debugPrint("Failed to set FTDI baudrate: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
      return FTDIDriverError;
  }

  // Set line parameters: 8 bits, 1 stop bit, no parity
  if ((f = ftdi_set_line_property(ftdi_, BITS_8, STOP_BIT_1, NONE)) != 0)
  {
      debugPrint("Failed to set FTDI line parameters: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
      return FTDIDriverError;
  }

  // Set FTDI chip latency in millisecs.
  if ((f = ftdi_set_latency_timer (ftdi_, latency_)) != 0)
  {
     debugPrint("Failed to set FTDI latency: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
     return FTDIDriverError;
  }

  // Reset device after setting the latency, otherwise the initial communication will result in an error
  if ((f = ftdi_usb_reset (ftdi_)) != 0)
  {
     debugPrint("Failed to reset FTDI: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
     return FTDIDriverError;
  }
  
  // Purge the send and receive buffers  
  if ((f = ftdi_usb_purge_tx_buffer (ftdi_)) != 0)
  {
     debugPrint("Failed to purge FTDI tx buffer: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
     return FTDIDriverError;
  }
  
  if ((f = ftdi_usb_purge_rx_buffer (ftdi_)) != 0)
  {
     debugPrint("Failed to purge FTDI rx buffer: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
     return FTDIDriverError;
  }
   
  // Wait 60 ms. for purge to complete
     epicsThreadSleep(0.060);
   
  if ((f = ftdi_read_data_set_chunksize(ftdi_, 8192)) != 0)
  {
     debugPrint("Failed to set FTDI read chunk size: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
     return FTDIDriverError;
  }

  if ((f = ftdi_write_data_set_chunksize(ftdi_, 8192)) != 0)
  {
     debugPrint("Failed to set FTDI write chunk size: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
     return FTDIDriverError;
  }
  
  debugPrint("%s : FTDI Connection ready...\n", functionName);
  connected_ = 1;

  return FTDIDriverSuccess;
}

/**
 * Flush the FTDI connection as best as possible.
 *
 * @return - Success or failure.
 */
FTDIDriverStatus FTDIDriver::flush()
{
  int f;
  static const char *functionName = "FTDIDriver::flush";
  debugPrint("%s : Method called\n", functionName);

  if (connected_ == 0){
    debugPrint("%s : FTDI not connected\n", functionName);
    return FTDIDriverError;
  }

  // Clears read & write buffers on the chip and the internal read buffer
  f = ftdi_usb_purge_buffers(ftdi_);
  if (f < 0){
    debugPrint("Failed to purge FTDI buffers: %d (%s)\n", f, ftdi_get_error_string(ftdi_));
    return FTDIDriverError;
  }
  return FTDIDriverSuccess;
}

/**
 * Write data to the connected channel.  A timeout should be
 * specified in milliseconds.
 *
 * @param buffer - The string buffer to be written.
 * @param bufferSize - The number of bytes to write.
 * @param bytesWritten - The number of bytes that were written.
 * @param timeout - A timeout in ms for the write.
 * @return - Success or failure.
 */
FTDIDriverStatus FTDIDriver::write(const unsigned char *buffer, int bufferSize, size_t *bytesWritten, int timeout)
{
  size_t rc;
  static const char *functionName = "FTDIDriver::write";
  debugPrint("%s : Method called\n", functionName);
  *bytesWritten = 0;
  struct ftdi_transfer_control *tc;

  if (connected_ == 0){
    debugPrint("%s : FTDI not connected\n", functionName);
    return FTDIDriverError;
  }

  debugPrint("%s : Writing\n", functionName);

  timeval stime;
  timeval ctime;
  gettimeofday(&stime, NULL);
  long mtimeout = (stime.tv_usec / 1000) + timeout;
  long tnow = 0;

  // Set timeout value
  ftdi_->usb_write_timeout = timeout;
  tc = ftdi_write_data_submit (ftdi_, (unsigned char *) buffer, (int) bufferSize);
  rc = ftdi_transfer_data_done (tc);
  gettimeofday(&ctime, NULL);
  tnow = ((ctime.tv_sec - stime.tv_sec) * 1000) + (ctime.tv_usec / 1000);
  debugPrint("%s : Time taken for write => %ld ms\n", functionName, (tnow-(mtimeout-timeout)));
  if (rc > 0){
    debugPrint("%s : %d bytes written\n", functionName, rc);
    *bytesWritten = (size_t)rc;
  } else {
    debugPrint("FTDI write error: %d (%s)\n", rc, ftdi_get_error_string(ftdi_));
    *bytesWritten = 0;
    return FTDIDriverError;
  }
  
  return FTDIDriverSuccess;
}

/**
 * Read data from the connected channel.  A timeout should be
 * specified in milliseconds.  The read method will continue to
 * read data from the channel until either data arrives or the
 * timeout is reached.
 *
 * @param buffer - A string buffer to hold the read data.
 * @param bufferSize - The maximum number of bytes to read.
 * @param bytesWritten - The number of bytes that have been read.
 * @param timeout - A timeout in ms for the read.
 * @return - Success or failure.
 */
FTDIDriverStatus FTDIDriver::read(unsigned char *buffer, int bufferSize, size_t *bytesRead, int timeout)
{
  static const char *functionName = "FTDIDriver::read";
  debugPrint("%s : Method called\n", functionName);
  
  if (connected_ == 0){
    debugPrint("%s : Not connected\n", functionName);
    return FTDIDriverError;
  }

  debugPrint("%s : Reading\n", functionName);

  timeval stime;
  timeval ctime;
  gettimeofday(&stime, NULL);
  long mtimeout = (stime.tv_usec / 1000) + timeout;
  long tnow = 0;
  int rc = 0;
  
  *bytesRead = 0;
  while ((*bytesRead == 0) && (tnow < mtimeout)){
    rc = ftdi_read_data (ftdi_, &buffer[*bytesRead], (bufferSize-*bytesRead));
    if (rc > 0){
      *bytesRead+=rc;
    }
    if (*bytesRead == 0){
      usleep(50);
      gettimeofday(&ctime, NULL);
      tnow = ((ctime.tv_sec - stime.tv_sec) * 1000) + (ctime.tv_usec / 1000);
    }
  }

  buffer[*bytesRead] = '\0';
  debugPrint("%s %d Bytes =>\n", functionName, (int)*bytesRead);
  for (int j = 0; j < (int)*bytesRead; j++){
    debugPrint("[%0X] ", buffer[j]);
  }
  debugPrint("\n");

  gettimeofday(&ctime, NULL);
  tnow = ((ctime.tv_sec - stime.tv_sec) * 1000) + (ctime.tv_usec / 1000);
  debugPrint("%s : Time taken for read => %ld ms\n", functionName, (tnow-(mtimeout-timeout)));

  if ((timeout > 0) && (tnow >= mtimeout)){
    return FTDIDriverTimeout;
  }

  return FTDIDriverSuccess;
}


/**
 * Close the connection.
 *
 * @return - Success or failure.
 */
FTDIDriverStatus FTDIDriver::disconnectFTDI()
{
  static const char *functionName = "FTDIDriver::disconnect";
  debugPrint("%s : Method called\n", functionName);

  if (connected_ == 1){
    ftdi_usb_close(ftdi_);
    ftdi_deinit(ftdi_);
    connected_ = 0;
  }
  else {
    debugPrint("%s : Connection was not established\n", functionName);
  }
  return FTDIDriverSuccess;
}

/**
 * Destructor, cleanup.
 */
FTDIDriver::~FTDIDriver()
{
  static const char *functionName = "FTDIDriver::~FTDIDriver";
  debugPrint("%s : Method called\n", functionName);
}

#ifndef _MINGW
#ifdef _WIN32
/* Windows implemenation of gettimeofday and usleep */
#include < time.h >
#include < windows.h >

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag = 0;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres /= 10;  /*convert into microseconds*/
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }

  return 0;
}

void usleep(__int64 usec) 
{ 
    HANDLE timer; 
    LARGE_INTEGER ft; 

    ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL); 
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
    WaitForSingleObject(timer, INFINITE); 
    CloseHandle(timer); 
}
#endif
#endif