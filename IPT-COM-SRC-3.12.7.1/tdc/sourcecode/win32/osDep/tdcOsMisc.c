//
// $Id: tdcOsMisc.c 11184 2009-05-14 13:28:06Z mritz $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interface
//
// AUTHOR         M.Ritz         PPC/EBT
//
// REMARKS        The switch WIN32 has to be set
//
// DEPENDENCIES
//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Bombardier Transportation GmbH, Germany, 2002.
//


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


#if !defined (WIN32)
   #error "This is the Windows Version of TDC_OS_MISC, i.e. WIN32 has to be specified"
#endif

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

// ----------------------------------------------------------------------------

#include "pthread.h"

#include "tdc.h"

/* ---------------------------------------------------------------------------- */

const char* tdcGetEnv (const char*  pVarName)
{                               /*@ -observertrans -dependenttrans */
   return (getenv (pVarName));  /*@ =observertrans =dependenttrans*/
}

/* ------------------------------------------------------------------------- */
                                                                                                /*@ -exportlocal@*/
int tdcSelect (int        nfds,
               void*      pRdSet,
               void*      pWrSet,
               void*      pExSet,
               INT32      timeoutMSec)
{
   /*@null@*/ fd_set*           pReadSet   = (fd_set *) pRdSet;
   /*@null@*/ fd_set*           pWriteSet  = (fd_set *) pWrSet;
   /*@null@*/ fd_set*           pExceptSet = (fd_set *) pExSet;

              struct timeval    timeout;
              struct timeval*   pTimeout;
                                                      /*@ -usedef */
   timeout.tv_sec  = timeoutMSec / 1000;
   timeout.tv_usec = (timeoutMSec % 1000) * 1000;     /*@ =usedef */

   pTimeout = (timeoutMSec == TDC_SEMA_WAIT_FOREVER) ? (NULL) : (&timeout);
                                                                        /*@ -nullpass */
   return (select (nfds, pReadSet, pWriteSet, pExceptSet, pTimeout));   /*@ =nullpass */
}                                                                                               /*@ =exportlocal@*/

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL tdcStreamReadN (const char*       pModname,
                           T_TDC_STREAM      fdRd,
                           void*             pReadBuf,
                           UINT32*           pBytesRead,
                           UINT32            bytesExpected,
                           int               flags)
{
   *pBytesRead = 0;

   for (; (*pBytesRead) < bytesExpected;)
   {
      int  	   requSize = (int) (bytesExpected - (*pBytesRead));
      int     	readSize = recv (fdRd, pReadBuf, requSize, flags);

      if (readSize < 0)
      {
         DEBUG_WARN (pModname, "tdcStreamReadN(): Client shut down\n");
         return (FALSE);
      }
      else if (readSize == 0)
      {
         DEBUG_WARN (pModname, "tdcStreamReadN(): Client shut down gracefully\n");
         return (FALSE);
      }
      else if (readSize > (int) requSize)
      {
         DEBUG_ERROR (pModname, "ERROR Reading from stream - Size too large");
         *pBytesRead = -1;
         return (FALSE);
      }
      else
      {
         (*pBytesRead) += readSize;
         pReadBuf       = & (((UINT8 *) pReadBuf)[readSize]);
      }
   }

   return ((*pBytesRead) == bytesExpected);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL tdcStreamWriteN (const char*      pModname,
                            T_TDC_STREAM     fdWr,
                            void*            pWriBuf,
                            UINT32*          pBytesWritten,
                            UINT32           bytesExpected,
                            int              flags)
{
   *pBytesWritten = 0;

   for (; (*pBytesWritten) < bytesExpected;)
   {
      int      requSize = bytesExpected - (*pBytesWritten);
      int      wriSize  = send (fdWr, pWriBuf, requSize, flags);

      if (wriSize < 0)
      {
         DEBUG_WARN (pModname, "tdcStreamWriteN(): Client shut down\n");
      }
      else if (wriSize == 0)
      {
         DEBUG_WARN (pModname, "tdcStreamWriteN(): Client gracefully shut down\n");
      }
      else if (wriSize > (int) requSize)
      {
         DEBUG_ERROR (pModname, "ERROR writing to stream - Size too large");
         *pBytesWritten = -1;
         return (FALSE);
      }
      else
      {
         *pBytesWritten += wriSize;
         pWriBuf         = & (((UINT8 *) pWriBuf)[wriSize]);
      }
   }

   return ((*pBytesWritten) == bytesExpected);
}

/* ---------------------------------------------------------------------------- */

T_FILE* tdcFopen  (const char*     pFilename,
                   const char*     pMode)
{                                                 /*@ -abstract -dependenttrans */
   return ((T_FILE *) fopen (pFilename, pMode));  /*@ =abstract =dependenttrans */
}

/* ---------------------------------------------------------------------------- */

INT32 tdcFclose (T_FILE*         pStream)
{
   if (pStream != NULL)
   {                                         /*@ -abstract */
      return (fclose ((FILE *) pStream));    /*@ =abstract */
   }

   return (0);
}

/* ---------------------------------------------------------------------------- */

INT32 tdcFeof (T_FILE*     pStream)
{                                     /*@ -abstract */
   return (feof ((FILE *) pStream));  /*@ =abstract */
}

/* ---------------------------------------------------------------------------- */

UINT32 tdcFread  (void*          pBuffer,
                  UINT32         bufLen,
                  UINT32         count,
                  T_FILE*        pStream)
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
   {                                                              /*@ -abstract -temptrans */
   return (fgets (pBuffer, (int) bufSize, (FILE *) pStream));     /*@ =abstract =temptrans */
}

/* ---------------------------------------------------------------------------- */

INT32 tdcFFlush (T_FILE*         pStream)
{
   return (fflush ((FILE *) pStream));
}

/* ---------------------------------------------------------------------------- */

int tdcRaiseSIGABRT (void)
{
   return (raise (SIGABRT));
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

void tdcInstallSignalHandler (void)
{
   // Nothing to do in Windows
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcRemoveSignalHandler (void)
{
   /* Nothing to do in Windows */
   return (TRUE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

UINT32 tdcFSize (const char*   pModname, 
                 const char*   pFilename)
{
   UINT32      fileSize = (UINT32) 0;                     /*@-unrecog*/
   int         fd       = _open (pFilename, _O_RDONLY);   /*@=unrecog*/

   if (fd != -1)
   {
      struct _stat         fileStat;
                                                   /*@-unrecog*/
      if (_fstat (fd, &fileStat) != -1)            /*@=unrecog*/
      {                                            /*@-usedef*/
         fileSize = (UINT32) fileStat.st_size;     /*@=usedef*/
      }
      else
      {
         char     text[200];

         (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Failed to call fstat (%s)", pFilename);

         DEBUG_WARN (pModname, text); 
      }
                                 /*@-unrecog*/
      (void) _close (fd);        /*@=unrecog*/
   }

   return (fileSize);
}
