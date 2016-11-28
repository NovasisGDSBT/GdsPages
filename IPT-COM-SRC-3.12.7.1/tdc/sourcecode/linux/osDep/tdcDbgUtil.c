/*
 *  $Id: tdcDbgUtil.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    Front-End for some Debug Utilities
 *
 *  AUTHOR         Manfred Ritz
 *
 *  REMARKS        The switch LINUX has to be set
 *
 *  DEPENDENCIES
 *
 *  MODIFICATIONS (log starts 2010-08-11)
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *          Adding corrections for TÜV-Assessment
 *
 *  All rights reserved. Reproduction, modification, use or disclosure
 *  to third parties without express authority is forbidden.
 *  Copyright Bombardier Transportation GmbH, Germany, 2002-2012.
 *
 */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#if !defined (LINUX)
   #error "This is the Linux Version of TdcDbgUtil, i.e. LINUX has to be specified"
#endif

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "tdc.h"

// ----------------------------------------------------------------------------

#define tdcFPrintf                  (void) fprintf

// ----------------------------------------------------------------------------

static UINT16                       debugLevel       = DBG_LEVEL_ERROR;
static T_TDC_BOOL                   bEnableLogging   = FALSE;
static char                         logFileName[256] = TDC_LOG_FILE_NAME;
static char                         logfileMode[20]  = "w";       // overwrite old logfile

// ----------------------------------------------------------------------------

static T_TDC_BOOL               	   disasterFlag     = FALSE;

// ----------------------------------------------------------------------------

static void dbgPrintf (const char*  pFormat, ...);

// ----------------------------------------------------------------------------

T_TDC_BOOL  isDisaster  (void)
{
   return (disasterFlag);
}

// ----------------------------------------------------------------------------

static void logDebugMessage (const char*  pText)
{
   static FILE      *pLogfile = NULL;

   dbgPrintf ("%s", pText);

   if (bEnableLogging)
   {
      if (pLogfile == NULL)
      {
         if ((pLogfile = fopen (logFileName, logfileMode)) == NULL)
         {
            //tdcPrintf ("!!! --- Failed to Open Logfile '%s'\n", logFileName);
            dbgPrintf ("------------- Could Not open Logfile---------------\n");
            return;
         }
      }
                            
      tdcFPrintf (pLogfile, "%s", pText); 
      tdcFFlush  (pLogfile);
   }
}

// ----------------------------------------------------------------------------

void tdcSetEnableLog  (const char*  pPar0,
                       const char*  pPar1,
                       const char*  pPar2)
{
   TDC_UNUSED (pPar1)
   TDC_UNUSED (pPar2)

   if (pPar0 == NULL)
   {
      dbgPrintf ("tdcSetEnableLog needs at least one more parameter");
   }
   else
   {
      if (strcmp (pPar0, "TRUE")  == 0)     {bEnableLogging = TRUE;}
      if (strcmp (pPar0, "FALSE") == 0)     {bEnableLogging = FALSE;}

      dbgPrintf ("tdcSetEnableLog called (%s - %d)\n", pPar0, (int) bEnableLogging);
   }
}

// -----------------------------------------------------------------------------

void tdcSetLogfileName (const char*    pPar0,
                        const char*    pPar1,
                        const char*    pPar2)
{
   TDC_UNUSED (pPar1)
   TDC_UNUSED (pPar2)

   if (pPar0 == NULL)
   {
      dbgPrintf ("tdcSetLogfileName needs at least one more parameter");
   }
   else
   {
      (void) tdcStrNCpy (logFileName, pPar0, (UINT32) sizeof (logFileName));
   }
}

// -----------------------------------------------------------------------------

void tdcSetLogfileMode (const char*    pPar0,
                        const char*    pPar1,
                        const char*    pPar2)
{
   TDC_UNUSED (pPar1)
   TDC_UNUSED (pPar2)

   if (pPar0 == NULL)
   {
      dbgPrintf ("tdcSetLogfileMode needs at least one more parameter");
   }
   else
   {
      if (strcmp (pPar0, "OVERWRITE") == 0)     {tdcStrNCpy (logfileMode, "w", (UINT32) sizeof (logfileMode));}
      if (strcmp (pPar0, "APPEND")    == 0)     {tdcStrNCpy (logfileMode, "a", (UINT32) sizeof (logfileMode));}

      dbgPrintf ("tdcSetLogfileMode called (%s)\n", pPar0);
   }
}

// ----------------------------------------------------------------------------

void tdcSetLogfileOpenMode (const char* pPar0,
                            const char* pPar1,
                            const char* pPar2)
{
   TDC_UNUSED (pPar1)
   TDC_UNUSED (pPar2)

   if (pPar0 == NULL)
   {
      dbgPrintf ("tdcSetLogfileOpenMode needs at least one more parameter");
   }
   else
   {
      if (strcmp (pPar0, "OVERWRITE") == 0)   {(void) tdcStrCpy (logfileMode, "w");}
      if (strcmp (pPar0, "APPEND")    == 0)   {(void) tdcStrCpy (logfileMode, "a");}

      dbgPrintf ("tdcSetLogfileOpenMode called (%s - %s)\n", pPar0, logfileMode);
   }
}

// ----------------------------------------------------------------------------

void tdcSetDebugLevel (const char* pPar0,
                       const char* pPar1,
                       const char* pPar2)
{
   TDC_UNUSED (pPar1)
   TDC_UNUSED (pPar2)

   if (pPar0 == NULL)
   {
      dbgPrintf ("tdcSetDebugLevel needs at least one more parameter");
   }
   else
   {
      const char*    pNewDebugLevel = "ERROR";
      const char*    pOldDebugLevel = (debugLevel == DBG_LEVEL_INFO) ? "INFO"
                                                                     : (debugLevel == DBG_LEVEL_WARN) ? "WARNING"
                                                                                                      : "ERROR";
      if (strcmp (pPar0, "INFO")    == 0)   {debugLevel = DBG_LEVEL_INFO; pNewDebugLevel = "INFO";}
      if (strcmp (pPar0, "WARNING") == 0)   {debugLevel = DBG_LEVEL_WARN; pNewDebugLevel = "WARNING";}
      if (strcmp (pPar0, "ERROR")   == 0)   {debugLevel = DBG_LEVEL_ERROR;}

      dbgPrintf ("TDC:  DebugLevel changed from \"%s\" to \"%s\"\n", pOldDebugLevel, pNewDebugLevel);
   }
}

// ----------------------------------------------------------------------------

static void dbgPrintf (const char*  pFormat, ...)
{

   if (debugLevel >= DBG_LEVEL_INFO)
   {
      va_list        argList;
      char           text [241];

             va_start  (argList, pFormat);
      (void) vsnprintf (text, sizeof (text), pFormat, argList);
             va_end    (argList);
                            
      printf ("%s", text);  
   }
}

// ----------------------------------------------------------------------------

void tdcDebug (const char*         pFilename,
                     UINT16        line,
                     INT16         strategy,
               const char*         pModname,
               const char*         pFormat, ... )
{
   va_list        argList;
   char           text [321];
   char           textOut[401];

   if (    (    (strategy   == DEBUG_PRINT)
             && (debugLevel <  DBG_LEVEL_INFO)
           )
        || (    (strategy   == DEBUG_CONTINUE)
             && (debugLevel <  DBG_LEVEL_WARN)
           )
      )
   {
      return;
   }

          va_start  (argList, pFormat);
   (void) vsnprintf (text, sizeof (text), pFormat, argList);
          va_end    (argList);

   switch (strategy)
   {
      case DEBUG_PRINT:
      {
         (void) tdcSNPrintf (textOut, (unsigned int) sizeof (textOut), "%-9s%s\n", pModname, text);
         textOut[sizeof (textOut) - 1] = '\0';
         logDebugMessage (textOut);
         break;
      }
      case DEBUG_CONTINUE:
      {
         (void) tdcSNPrintf (textOut, (unsigned int) sizeof (textOut), "%-9s%s!\n" LEFT_MARGIN "File: %s, Line: %d\n",
                                   pModname, text, pFilename, (int) line);
         textOut[sizeof (textOut) - 1] = '\0';
         logDebugMessage (textOut);
         break;
      }
      case DEBUG_HALT:
      default:
      {                   
         (void) tdcSNPrintf (textOut, (unsigned int) sizeof (textOut), "%-9s%s!\n" LEFT_MARGIN "File: %s, Line: %d\n",
                                   pModname, text, pFilename, (int) line);
         textOut[sizeof (textOut) - 1] = '\0';
         logDebugMessage (textOut);

         exit (EXIT_FAILURE);          /*@ -unreachable */
         break;                        /*@ =unreachable */
      }
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

int tdcPrintf (const char* pFormat, ...)
{
   va_list     argList;
   char        text[1000];
   int         numWri1;
   int         numWri2;

   va_start (argList, pFormat);
   numWri1 = vsnprintf (text, sizeof (text), pFormat, argList);
   va_end   (argList);

                                    
   numWri2 = printf ("%s", text);   

   if (numWri1 != numWri2)
   {
      // WARNING
   }

   return (numWri2);
}

// ----------------------------------------------------------------------------

void tdcSNPrintf (char* pBuf, unsigned int bufSize, const char* pFormat, ...)
{
   va_list     argList;

          va_start  (argList, pFormat);
   (void) vsnprintf (pBuf, (size_t) bufSize, pFormat, argList);
          va_end    (argList);
}
 
// ----------------------------------------------------------------------------
 
 
