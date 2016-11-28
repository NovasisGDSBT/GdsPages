/*                                                                     */
/* $Id: tdcOsMisc.c 11184 2009-05-14 13:28:06Z mritz $             */
/*                                                                     */
/* DESCRIPTION    Front-End for miscelleanous functions like hamster,  */
/*                alloc... as an OS independant interface              */
/*                                                                     */
/* AUTHOR         Manfred Ritz                                         */
/*                                                                     */
/* REMARKS        The switch VXWORKS has to be set                     */
/*                                                                     */
/* DEPENDENCIES                                                        */
/*                                                                     */
/*                                                                     */
/* All rights reserved. Reproduction, modification, use or disclosure  */
/* to third parties without express authority is forbidden.            */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.            */
/*                                                                     */
/* ---------------------------------------------------------------------------- */

#if !defined (VXWORKS)
   #error "This is the CSS2 Version of TDC_OS_MISC, i.e. VXWORKS has to be specified"
#endif

/* ---------------------------------------------------------------------------- */

#include <vxWorks.h>
#include <stdio.h>
#include <stdlib.h>
#include <taskLib.h>
#include <wdLib.h>

extern int     sysClkRateGet (void);   /* Now returns int, not UINT32. gah, 2/2/94 */

#define TDC_BASETYPES_H    /* Use original VxWorks Header File definitions */
#include "tdc.h"

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

int tdcSelect (int    nfds,
               void*  pRdSet,
               void*  pWrSet,
               void*  pExSet,
               INT32  timeoutMSec)
{
   /* O_XXXXX) */

   TDC_UNUSED (nfds)
   TDC_UNUSED (pRdSet)
   TDC_UNUSED (pWrSet)
   TDC_UNUSED (pExSet)
   TDC_UNUSED (timeoutMSec)

   return (0);
}

/* ---------------------------------------------------------------------------- */

T_SIG_FUNCTION* Signal (int                 signr,
                        T_SIG_FUNCTION*     func)
{
   /* O_XXXXX) */

   TDC_UNUSED (signr)
   TDC_UNUSED (func)

   return (NULL);
}

/* ---------------------------------------------------------------------------- */

T_FILE* tdcFopen  (const char*     pFilename,
                   const char*     pMode)
{                                                     /*@ -abstract -dependenttrans */
   return ((T_FILE *) fopen (pFilename, pMode));      /*@ =abstract =dependenttrans */
}

/* ---------------------------------------------------------------------------- */

INT32 tdcFclose (T_FILE*      pStream)
{
   if (pStream != NULL)
   {                                         /*@ -abstract */
      return (fclose ((FILE *) pStream));    /*@ =abstract */
   }           

   return (0);
}

/* ---------------------------------------------------------------------------- */

INT32 tdcFeof (T_FILE*     pStream)
{  
   /*lint -e(611) , suppress suspicious cast once*/   /*@ -abstract */
   return ((INT32) feof ((FILE *) pStream));          /*@ =abstract */
}
/* ---------------------------------------------------------------------------- */

UINT32  tdcFread  (void*           pBuffer,
                   UINT32          bufLen,
                   UINT32          count,
                   T_FILE*         pStream)
{                                                                                         /*@ -abstract */
   return ((UINT32) fread (pBuffer, (size_t) bufLen, (size_t) count, (FILE *) pStream));  /*@ =abstract */
}

/* ---------------------------------------------------------------------------- */

UINT32 tdcFwrite (const void*    pBuffer,
                  UINT32         bufLen,
                  UINT32         count,
                  T_FILE*        pStream)
{                                                                                         /*@ -abstract */
   return ((UINT32) fwrite (pBuffer, (size_t) bufLen, (size_t) count, (FILE *) pStream)); /*@ =abstract */
}

/* ---------------------------------------------------------------------------- */

char* tdcFgets  (char*           pBuffer,
                 UINT32          bufSize,
                 T_FILE*         pStream)
{                                                                    /*@ -abstract -temptrans */
   return (fgets (pBuffer, (int) bufSize, (FILE *) pStream));        /*@ =abstract =temptrans */
}

/* ---------------------------------------------------------------------------- */

INT32 tdcFFlush (T_FILE*         pStream)
{
   return (fflush ((FILE *) pStream));
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

void tdcInstallSignalHandler (void)
{
   /* Nothing to do in CSS*/
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcRemoveSignalHandler (void)
{
   /* Nothing to do in CSS*/
   return (TRUE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

UINT32 tdcFSize (const char*   pModname, 
                 const char*   pFilename)
{
   UINT8       buffer[512];
   UINT32      fileSize = (UINT32) 0;
   T_FILE*     pFile    = tdcFopen (pFilename, "rb");

   TDC_UNUSED (pModname)

   if (pFile != NULL)
   {
      UINT32   bytesRead;

      for (bytesRead = tdcFread (buffer, (UINT32) 1, (UINT32) sizeof (buffer),  pFile);
           bytesRead > 0;
           bytesRead = tdcFread (buffer, (UINT32) 1, (UINT32) sizeof (buffer),  pFile))
      {
         fileSize += bytesRead;
      }

      (void) tdcFclose (pFile);
   }

   return (fileSize);
}

