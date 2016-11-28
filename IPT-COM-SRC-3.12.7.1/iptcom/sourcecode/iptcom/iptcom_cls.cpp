/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2011 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     :  IPTrain
 *
 *  MODULE      :  iptcom_cls.cpp
 *
 *  ABSTRACT    :  Public C++ methods for IPT communication classes:
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 * $Id: iptcom_cls.cpp 33666 2014-07-17 14:43:01Z gweiss $
 *
 *  CR-2970  (Gerhard Weiss, 2011-06-15) Syntax modification for allowing
 *           compilation for NRTOS2
 *
 *  CR-432 (Gerhard Weiss, 2010-11-03)
 *           Corrected position of UNUSED Parameter Macros
 *
 *  CR-62 (Bernd Loehr, 2010-08-25)
 * 			Additional function handle_event() to renew all MC memberships
 * 			after link down/up event
 *
 *  Internal (Bernd Loehr, 2010-08-16) 
 * 			Old obsolete CVS history removed
 *
 * 
 *
 ******************************************************************************/

/*******************************************************************************
*  INCLUDES
*/
#include <string.h>
#include "iptcom.h"        /* Common type definitions for IPT */
#include "iptcom_cpp.h"

/*******************************************************************************
*  DEFINES
*/

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  GLOBAL DATA
*/

/*******************************************************************************
*  LOCAL DATA
*/

/* Search path for the xml config file in C++ implementation */
#ifndef LINUX_CLIENT
char theXMLFilePath[IPT_SIZE_OF_FILEPATH+1];
#endif

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       IPTCom
ABSTRACT:   Constructor
RETURNS:    -
*/
#ifdef IPTCOM_CM_COMPONENT
IPTCom::IPTCom(const char *name) : SynchronousACO(name)
#else
IPTCom::IPTCom(const char * /*name*/)
#endif
{
#ifdef IPTCOM_CM_COMPONENT
   aSoftwareVersion.set(IPT_VERSION, IPT_RELEASE, IPT_UPDATE, IPT_EVOLUTION);
#else
/* TODO
   aSoftwareVersion.set(IPT_VERSION, IPT_RELEASE, IPT_UPDATE, IPT_EVOLUTION);
*/
#endif
#ifndef LINUX_CLIENT
    memset(theXMLFilePath, 0, IPT_SIZE_OF_FILEPATH);
    strncpy(theXMLFilePath, IPT_XML_CONFIG_FILENAME, sizeof(IPT_XML_CONFIG_FILENAME));
#endif
}

/*******************************************************************************
NAME:       prepareInit
ABSTRACT:   Initiate IPTCom
RETURNS:    
*/
#ifdef IPTCOM_CM_COMPONENT
void IPTCom::prepareInit(E_STARTUP_MODE startupMode)
#else
void IPTCom::prepareInit(int startupMode)
#endif
{
   int res;

#ifndef LINUX_CLIENT
   if (theXMLFilePath[0] != 0)
   {
      res = IPTCom_prepareInit(startupMode, theXMLFilePath);
   }
   else
   {
      res = IPTCom_prepareInit(startupMode, (char *)NULL);
   }
#else
   IPT_UNUSED (startupMode)
   
   res = IPT_ERROR;
#endif
   if isException(res)
   {
      throw IPTComException(res, __FILE__, __LINE__ );
   }
}


/*******************************************************************************
NAME:       prepareInit
ABSTRACT:   Initiate IPTCom with path and name of IPTCom configuration file
RETURNS:    
*/
#ifdef IPTCOM_CM_COMPONENT
void IPTCom::prepareInit(E_STARTUP_MODE startupMode, char *pXMLPath)
#else
void IPTCom::prepareInit(int startupMode, char *pXMLPath)
#endif
{
   int res;

#ifndef LINUX_CLIENT
      res = IPTCom_prepareInit(startupMode, pXMLPath);
#else
   IPT_UNUSED (startupMode)
   IPT_UNUSED (pXMLPath)
   
   res = IPT_ERROR;
#endif

   if isException(res)
   {
      throw IPTComException(res, __FILE__, __LINE__ );
   }
}


/*******************************************************************************
NAME:       terminate
ABSTRACT:   Terminate IPTCom
RETURNS:      
*/
#ifdef IPTCOM_CM_COMPONENT
void IPTCom::terminate(CM_INT32 remainingTime)
#else
void IPTCom::terminate(INT32 remainingTime)
#endif
{
#ifndef LINUX_CLIENT
   IPTCom_terminate(remainingTime);
#else
   IPT_UNUSED (remainingTime)
#endif
}

/*******************************************************************************
NAME:       process
ABSTRACT:   IPTCom specific processing.
RETURNS:      -
*/
void IPTCom::process(void)
{
#ifndef LINUX_CLIENT
   IPTCom_process();
#endif
}

/*******************************************************************************
NAME:       addConfig
ABSTRACT:   Parse IPTCom XML configuration file and add IPTCom configuration
            data.
RETURNS:      -
*/
void IPTCom::addConfig(const char *pXMLPath)
{
   int res;
   
   res = IPTCom_addConfig(pXMLPath);

   if isException(res)
   {
      throw IPTComException(res, __FILE__, __LINE__ );
   }
}

/*******************************************************************************
NAME:       IPTCom_getStatus
ABSTRACT:   Reads the IPTCom and TDC status.
RETURNS:    Returns the IPTCom status
*/
int IPTCom::getStatus()
{
   return(IPTCom_getStatus());
}

/*******************************************************************************
NAME:       getData
ABSTRACT:   Not used
RETURNS:      -
*/
void IPTCom::getData()
{
}

/*******************************************************************************
NAME:       putData
ABSTRACT:   Not used
RETURNS:      -
*/
void IPTCom::putData()
{
}

/*******************************************************************************
NAME:       cycleEntry
ABSTRACT:   Not used
RETURNS:      -
*/
void IPTCom::cycleEntry()
{
}

/*******************************************************************************
NAME:       cycleExit
ABSTRACT:   Not used
RETURNS:      -
*/
void IPTCom::cycleExit()
{
}


/*******************************************************************************
NAME:       configFile
ABSTRACT:   Specifies the searchpath for the XML config file
RETURNS:      -
*/
void IPTCom::configFile(const char *pXMLPath)
{
#ifndef LINUX_CLIENT
   if (pXMLPath)
   {
      strncpy(theXMLFilePath, pXMLPath, IPT_SIZE_OF_FILEPATH);
      theXMLFilePath[IPT_SIZE_OF_FILEPATH] = 0;
   }
   else
   {
      theXMLFilePath[0] = 0;
   }
#else
   IPT_UNUSED (pXMLPath)
#endif
}

/*******************************************************************************
 NAME:       disableMarshalling
 ABSTRACT:   Disable marshalling in PDCom and MDCom.
 Marshalling is the adjustment to data so it complies with the standard
 on the wire, e.g. big/little-endianess.
 RETURNS:      -
 */
void IPTCom::disableMarshalling(int disable)
{
   IPTCom_disableMarshalling(disable);
}

/*******************************************************************************
 NAME:       enableFrameSizeCheck
 ABSTRACT:   Enable frame size check in MDCom.
 Ensures, that too small frames are negated
 RETURNS:      -
 */
void IPTCom::enableFrameSizeCheck(int enable)
{
   IPTCom_enableFrameSizeCheck(enable);
}

/*******************************************************************************
NAME:       getVersion
ABSTRACT:   Returns the SW version of the IPTCom.
RETURNS:    SoftwareVersion
*/
#ifdef IPTCOM_CM_COMPONENT
const SoftwareVersion *IPTCom::getVersion() const
{
   return &aSoftwareVersion;
}
#else
/* TODO */
#endif

/*******************************************************************************
NAME:       getErrorString
ABSTRACT:   Returns the corresponding error string to the provided sub code.
RETURNS:    Pointer to error string
*/
#ifdef IPTCOM_CM_COMPONENT
const char * IPTCom::getErrorString(CM_INT32 errorNumber) const
#else
const char * IPTCom::getErrorString(INT32 errorNumber) const
#endif
{
   if ((errorNumber & ERR_TDCCOM) == ERR_TDCCOM)
/*TODO  tdcGetErrorString is not included in the TDC IPC library
        Remove this when it is */
#ifndef LINUX_CLIENT
      return tdcGetErrorString(errorNumber);
#else
      return("NOT yet included IN TDC IPC Library");
      
#endif
   else
      return IPTCom_getErrorString(errorNumber);
}

/*******************************************************************************
NAME:       getCompInst
ABSTRACT:   Returns the component instance number of IPTCom
RETURNS:    Instance
*/
UINT32 IPTCom::getCompInst() const
{
   return IPTCOM_COINST;
}

/*******************************************************************************
NAME:       getCompId
ABSTRACT:   Returns the component id of IPTCom
RETURNS:    Component id.
*/
UINT32 IPTCom::getCompId() const
{
   return IPTCOM_COID;
}

/*******************************************************************************
NAME:       getCompId
ABSTRACT:   Returns the component id of IPTCom
RETURNS:    Component id.
*/
int IPTCom::handleEvent (UINT32 selector, void* pArg)
{
   return IPTCom_handleEvent(selector, pArg);
}

#ifdef LINUX_MULTIPROC
/*******************************************************************************
NAME:       IPTCom_MPattach
ABSTRACT:   Attaches to an IPTCom already started with IPTCom_prepareInit
            by another process in a Linux multi-processing system.
RETURNS:    Result code or Exception
*/
int IPTCom::MPattach(void)
{
   int res;

   res = IPTCom_MPattach();

   if isException(res)
   {
      throw IPTComException(res, __FILE__, __LINE__ );
   }

   return(res);
}

/*******************************************************************************
NAME:       IPTCom_MPdetach
ABSTRACT:   Detaches a process that has been attached to an IPTCom 
            in a Linux multi-processing system.
RETURNS:    -
*/
int IPTCom::MPdetach(void)
{
   int res;

   res = IPTCom_MPdetach();

   if isException(res)
   {
      throw IPTComException(res, __FILE__, __LINE__ );
   }

   return(res);
}
#endif
