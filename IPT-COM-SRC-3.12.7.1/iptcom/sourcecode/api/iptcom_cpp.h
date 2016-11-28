/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2011 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     : IPTrain
 *
 *  MODULE      : iptcom_cpp.h
 *
 *  ABSTRACT    : Public header file for IPTCom class
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 * $Id: iptcom_cpp.h 33666 2014-07-17 14:43:01Z gweiss $
 *
 *  CR-2970  (Gerhard Weiss, 2011-06-15) Syntax modification for allowing
 *           compilation for NRTOS2
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

#ifndef IPTCOM_CPP_H
#define IPTCOM_CPP_H

/*******************************************************************************
*  INCLUDES
*/
#ifdef IPTCOM_CM_COMPONENT
#include "synchronousaco.h"   /* CM interface definitions */
#endif
#include "iptcom.h"           /* IPTCom interfaces */
#include "mdcom.h"            /* MD interface */
#include "pdcom.h"            /* PD interface */
#include "iptDef.h"        /* IPT definitions */
#include "tdcSyl.h"           /* TDC System level definitions */
#include "tdcApi.h"           /* TDC interface routines */

/*******************************************************************************
*  DEFINES
*/

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

#ifdef __cplusplus

/*******************************************************************************
*  CLASSES (C++)
*/
/* IPTCom class */ 

#ifdef IPTCOM_CM_COMPONENT
class DLL_DECL IPTCom : public SynchronousACO
#else
class DLL_DECL IPTCom
#endif
{
public:
   IPTCom(const char *Name);
#ifdef IPTCOM_CM_COMPONENT
   void prepareInit(E_STARTUP_MODE startupMode);
   void prepareInit(E_STARTUP_MODE startupMode, char *pXMLPath);
   void terminate(CM_INT32 remainingTime);
#else
   void prepareInit(int startupMode);
   void prepareInit(int startupMode, char *pXMLPath);
   void terminate(INT32 remainingTime);
#endif
   void addConfig(const char *pXMLPath);
   int getStatus(void);
   virtual ~IPTCom(void) { IPTCom_destroy(); }
   void getData();
   void putData();
   void process(void);
   void cycleEntry();
   void cycleExit();
   void configFile(const char *pXMLPath);
   void disableMarshalling(int disable);
   void enableFrameSizeCheck(int enable);

#ifdef IPTCOM_CM_COMPONENT
   virtual const SoftwareVersion *getVersion() const;
   virtual const char * getErrorString(CM_INT32 errorNumber) const;
#else
/* TODO
   virtual const SoftwareVersion *getVersion() const;
*/
   virtual const char * getErrorString(INT32 errorNumber) const;
#endif
   virtual UINT32 getCompInst() const;
   virtual UINT32 getCompId() const;
   virtual int 	  handleEvent (UINT32 selector, void* pArg);

#ifdef LINUX_MULTIPROC
   int MPattach(void);
   int MPdetach(void);
#endif
   
   PDCom PD;
   MDCom MD;
   IPTDirClient TDC;

private:
#ifdef IPTCOM_CM_COMPONENT
	SoftwareVersion aSoftwareVersion;
#endif
};

/* IPTComException class */ 
#ifdef IPTCOM_CM_COMPONENT
class DLL_DECL IPTComException : public TcmsCoException
#else
class DLL_DECL IPTComException
#endif
{
#ifdef IPTCOM_CM_COMPONENT
public:
	IPTComException(CM_INT32 errorCode, const char* fileName, UINT32 lineNumber) : TcmsCoException(errorCode, fileName, lineNumber) { }

private:
#else
   public:
      IPTComException ( INT32 errorCode, const char* fileName, UINT32 lineNumber ) : a_errorCode(errorCode),a_fileName(fileName),a_lineNumber(lineNumber) { }
      INT32 getErrorCode() const { return a_errorCode; }
      const char* getFileName() const { return a_fileName; }
      UINT32 getLineNumber() const { return a_lineNumber; }

   private:
      INT32 a_errorCode;
      const char *a_fileName;
      UINT32 a_lineNumber;
#endif
};


#endif

#endif

