/*
 *  $Id: tdcDbgUtil.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    Front-End for some Debug Utilities
 *
 *  AUTHOR         Manfred Ritz
 *
 *  REMARKS        The switch VXWORKS has to be set
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

/* ---------------------------------------------------------------------------- */

#include <vxWorks.h>
#include <stdio.h>
#include <string.h>

#define TDC_BASETYPES_H    /* Use original VxWorks Header File definitions */
#include "tdc.h"

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#define LOG_CONTINUE          0
#define LOG_RESET             1

#define DU_CONTINUE           LOG_CONTINUE
#define DU_RESET              LOG_RESET

#define tdcFPrintf            (void) fprintf


typedef unsigned char   CHAR;
extern void du_hamster_text (const CHAR* p_file_id,
                             UINT16 line_nr,
                             INT16 strategy,
                             const CHAR* fmt,
                             ... );

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

static UINT16                 debugLevel       = DBG_LEVEL_ERROR;
static T_TDC_BOOL             bEnableLogging   = FALSE;
static char                   logFileName[256] = TDC_LOG_FILE_NAME;
static char                   logFileMode[20]  = "w";          /* overwrite old logfile */

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL             disasterFlag = (T_TDC_BOOL) FALSE;

/* ---------------------------------------------------------------------------- */

static void logDebugMessage (const char*  pText)
{
   static T_FILE*          pLogfile = NULL;

   (void) tdcPrintf ("%s", pText);

   if (bEnableLogging)
   {
      if (pLogfile == NULL)
      {
         if ((pLogfile = tdcFopen (logFileName, logFileMode)) == NULL)
         {
            (void) tdcPrintf ("!!! --- Failed to Open Logfile '%s'\n", logFileName);
            return;
         }
      }
                                
      tdcFPrintf (pLogfile, "%s", pText);/*lint !e64 pLogfile file type casing ok */

      (void) tdcFFlush  (pLogfile);
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
      (void) tdcPrintf ("tdcSetEnableLog needs at least one more parameter");
   }
   else
   {
      if (strcmp (pPar0, "TRUE")  == 0)     {bEnableLogging = TRUE;}
      if (strcmp (pPar0, "FALSE") == 0)     {bEnableLogging = FALSE;}

      (void) tdcPrintf ("tdcSetEnableLog called (%s - %d)\n", pPar0, (int) bEnableLogging);
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
      (void) tdcPrintf ("tdcSetLogfileName needs at least one more parameter");
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
      (void) tdcPrintf ("tdcSetLogfileMode needs at least one more parameter");
   }
   else
   {
      if (strcmp (pPar0, "OVERWRITE") == 0)     {(void) tdcStrNCpy (logFileMode, "w", (UINT32) sizeof (logFileMode));}
      if (strcmp (pPar0, "APPEND")    == 0)     {(void) tdcStrNCpy (logFileMode, "a", (UINT32) sizeof (logFileMode));}

      (void) tdcPrintf ("tdcSetLogfileMode called (%s)\n", pPar0);
   }
}

// -----------------------------------------------------------------------------

T_TDC_BOOL isDisaster (void)
{
   return (disasterFlag);
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
      (void) tdcPrintf ("tdcSetDebugLevel needs at least one more parameter!\n");
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

      (void) tdcPrintf ("TDC:  DebugLevel changed from \"%s\" to \"%s\"\n", pOldDebugLevel, pNewDebugLevel);
   }
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL doDebug (INT16   strategy)
{
   T_TDC_BOOL    letItBe = (    (    (strategy   == DEBUG_PRINT)
                                  && (debugLevel <  DBG_LEVEL_INFO)
                                 )
                              || (   (strategy   == DEBUG_CONTINUE)
                                  && (debugLevel <  DBG_LEVEL_WARN)
                                 )
                            );
   return (!letItBe);
}

/* ---------------------------------------------------------------------------- */

static void tdcDebug (const char*         pFileName,
                            UINT16        line,
                            INT16         strategy,
                            const char*   pHead,
                            const char*   pText)
{
   char     textOut[300] = "";

   switch (strategy)
   {
      case DEBUG_PRINT:
      {
         (void) tdcSNPrintf (textOut, (UINT32) sizeof (textOut), "%-7s%s\n", pHead, pText); 
         break;
      }
      case DEBUG_CONTINUE:
      {
         (void) tdcSNPrintf (textOut, (UINT32) sizeof (textOut), 
                             "%-7s%s!\n" LEFT_MARGIN "File: %s, Line: %d\n",
                             pHead, pText, pFileName, (int) line);
         /*du_hamster_text (pFileName, line, DU_CONTINUE, "%s %s", pHead, pText);*/
         break;
      }
      case DEBUG_HALT:
      default:
      {
         (void) tdcSNPrintf (textOut, (UINT32) sizeof (textOut), 
                             "%-7s%s!!!\n" LEFT_MARGIN "File: %s, Line: %d\n",
                             pHead, pText, pFileName, (int) line);
         /*du_hamster_text (pFileName, line, DU_RESET, "%s %s", pHead, pText);*/
         disasterFlag = TRUE;
         break;
      }
   }

   logDebugMessage (textOut);
 }

/* ---------------------------------------------------------------------------- */

void tdcDebug0 (const char*         pFileName,
                      UINT16        line,
                      INT16         strategy,
                const char*         pModname,
                const char*         pFormat)
{
   if (doDebug (strategy))
   {
      tdcDebug (pFileName, line, strategy, pModname, pFormat);
   }
}
/* ---------------------------------------------------------------------------- */

void tdcDebug1 (const char*         pFileName,
                      UINT16        line,
                      INT16         strategy,
                const char*         pModname,
                const char*         pFormat, int x1)
{
   if (doDebug (strategy))
   {
      char     text[161] = "";
                                                                        /*@ -formatconst   */
      (void) tdcSNPrintf (text, (UINT32) sizeof (text), pFormat, x1);          /*@ =formatconst   */

      tdcDebug   (pFileName, line, strategy, pModname, text);
   }
}

/* ---------------------------------------------------------------------------- */

void tdcDebug2 (const char*         pFileName,
                      UINT16        line,
                      INT16         strategy,
                const char*         pModname,
                const char*         pFormat, int x1, int x2)
{
   if (doDebug (strategy))
   {
      char     text[161] = "";
                                                                           /*@ -formatconst   */
      (void) tdcSNPrintf (text, (UINT32) sizeof (text), pFormat, x1, x2);         /*@ =formatconst   */

      tdcDebug  (pFileName, line, strategy, pModname, text);
   }
}

/* ---------------------------------------------------------------------------- */

void tdcDebug3 (const char*         pFileName,
                      UINT16        line,
                      INT16         strategy,
                const char*         pModname,
                const char*         pFormat, int x1, int x2, int x3)
{
   if (doDebug (strategy))
   {
      char     text[161] = "";
                                                                           /*@ -formatconst */
      (void) tdcSNPrintf (text, (UINT32) sizeof (text), pFormat, x1, x2, x3);     /*@ =formatconst */

      tdcDebug   (pFileName, line, strategy, pModname, text);
   }
}

/* ---------------------------------------------------------------------------- */

void tdcDebug4 (const char*         pFileName,
                      UINT16        line,
                      INT16         strategy,
                const char*         pModname,
                const char*         pFormat, int x1, int x2, int x3, int x4)
{
   if (doDebug (strategy))
   {
      char     text[161] = "";
                                                                           /*@ -formatconst */
      (void) tdcSNPrintf (text, (UINT32) sizeof (text), pFormat, x1, x2, x3, x4); /*@ =formatconst */

      tdcDebug   (pFileName, line, strategy, pModname, text);
   }
}
/* ---------------------------------------------------------------------------- */
