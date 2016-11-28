//
// $Id: tdcOsMisc.c 12809 2012-12-17 15:08:43Z gweiss $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interface
//
// AUTHOR         M.Ritz         PPC/EBT
//
// REMARKS        The switch INTEGRITY has to be set
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


#if !defined (__INTEGRITY)
   #error "This is the INTEGRITY Version of tdcOsMisc, "
          "i.e. INTEGRITY has to be specified"
#endif

// ----------------------------------------------------------------------------

#include <INTEGRITY.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "tdc.h"

/* ---------------------------------------------------------------------------- */

const char* tdcGetEnv (const char*  pVarName)
{                                /*@ -observertrans -dependenttrans */
   if(pVarName == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetEnv: Wrong parameter (NULL)");
      return NULL;
   }

   return (getenv (pVarName));   /*@ =observertrans =dependenttrans */
}

// ----------------------------------------------------------------------------
// signal wrapper function according R. Stevens "UNIX Network Programming Vol. 1"

T_SIG_FUNCTION *Signal (int sigNo, T_SIG_FUNCTION* func)
{
   struct sigaction   action;
   struct sigaction   oldAction;

   if (func == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "Signal: Wrong parameter (NULL)");
      return SIG_ERR;
   }

   action.sa_handler = func;
   (void) sigemptyset (&action.sa_mask);
   action.sa_flags = 0;

   if (sigNo == SIGALRM)
   {
#if defined SA_INTERRUPT
      action.sa_flags |= SA_INTERRUPT;      // SunOS 4.x
#endif
   }
   else
   {
#if defined SA_RESTART
      action.sa_flags |= SA_RESTART;      // SVR4, 4.4BSD
#endif
   }
                                                   /*@ -nullstate */
   if (sigaction (sigNo, &action, &oldAction) < 0)
   {                                   /*@ -unqualifiedtrans -compdestroy */
      return (SIG_ERR);                /*@ =unqualifiedtrans =compdestroy =nullstate */
   }
   else
   {                                   /*@ -unqualifiedtrans -compdestroy */
      return (oldAction.sa_handler);   /*@ =unqualifiedtrans =compdestroy */
   }
}

/* ------------------------------------------------------------------------- */
                                                                                                /*@ -exportlocal */
int tdcSelect (int        nfds,
               void*      pRdSet,
               void*      pWrSet,
               void*      pExSet,
               INT32      timeoutMSec)
{
   fd_set*           pReadSet;
   fd_set*           pWriteSet;
   fd_set*           pExceptSet;
   struct timeval    timeout;
   struct timeval*   pTimeout;

   if ((pRdSet == NULL) || (pWrSet == NULL) || (pExSet == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcSelect: Wrong parameter (NULL)");
      return -1;
   }
   
   pReadSet   = (fd_set *) pRdSet;
   pWriteSet  = (fd_set *) pWrSet;
   pExceptSet = (fd_set *) pExSet;
                                                      /*@ -usedef */
   timeout.tv_sec  = timeoutMSec / 1000;
   timeout.tv_usec = (timeoutMSec % 1000) * 1000;     /*@ =usedef */

   pTimeout = (timeoutMSec == TDC_SEMA_WAIT_FOREVER) ? (NULL) : (&timeout);
                                                                        /*@ -nullpass */
   return (select (nfds, pReadSet, pWriteSet, pExceptSet, pTimeout));   /*@ =nullpass */
}                                                                                               /*@ =exportlocal */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_BOOL tdcStreamReadN (const char*       pModname,
                           T_TDC_STREAM      fdRd,
                           void*             pReadBuf,
                           UINT32*           pBytesRead,
                           UINT32            bytesExpected)
{
   if ((pReadBuf == NULL) || (pBytesRead == NULL))
   {
      DEBUG_WARN (pModname, "tdcStreamReadN: Wrong parameter (NULL)");
      return FALSE;
   }

   *pBytesRead = 0;

   for (; (*pBytesRead) < bytesExpected;)
   {
      size_t      requSize = (size_t) (bytesExpected - (*pBytesRead));
      ssize_t     readSize = read (fdRd, pReadBuf, requSize);

      if (readSize < 0)
      {
         if (errno != EINTR)
         {
            DEBUG_WARN3 (pModname, "Could Not read enough Bytes from stream (%d - %d), Errno (%d)",
                                 bytesExpected, (*pBytesRead), errno);
            *pBytesRead = (UINT32)-1;
            return (FALSE);
         }
      }
      else
      {
         if (readSize == 0)
         {
            break;      // EOF
         }
         else
         {
            if (readSize > (ssize_t) requSize)
            {
               DEBUG_ERROR (pModname, "ERROR Reading from stream - Size too large");
               *pBytesRead = (UINT32)-1;
               return (FALSE);
            }
            else
            {
               (*pBytesRead) += readSize;
               pReadBuf       = & (((UINT8 *) pReadBuf)[readSize]);
            }
         }
      }
   }

   return ((*pBytesRead) == bytesExpected);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL tdcStreamWriteN (const char*      pModname,
                            T_TDC_STREAM     fdWr,
                            void*            pWriBuf,
                            UINT32*          pBytesWritten,
                            UINT32           bytesExpected)
{
   if ((pWriBuf == NULL) || (pBytesWritten == NULL))
   {
      DEBUG_WARN (pModname, "tdcStreamWriteN: Wrong parameter (NULL)");
      return FALSE;
   }

   *pBytesWritten = 0;

   for (; (*pBytesWritten) < bytesExpected;)
   {
      size_t      requSize = (size_t) (bytesExpected - (*pBytesWritten));
      ssize_t     wriSize  = write (fdWr, pWriBuf, requSize);

      if (wriSize < 0)
      {
         if (errno != EINTR)
         {
            DEBUG_WARN (pModname, "Could not write to stream");
            *pBytesWritten = (UINT32)-1;
            return (FALSE);
         }
      }
      else
      {
         if (wriSize > (ssize_t) requSize)
         {
            DEBUG_ERROR (pModname, "ERROR writing to stream - Size too large");
            *pBytesWritten = (UINT32)-1;
            return (FALSE);
         }
         else
         {
            *pBytesWritten += wriSize;
            pWriBuf         = & (((UINT8 *) pWriBuf)[wriSize]);
         }
      }
   }

   return ((*pBytesWritten) == bytesExpected);
}

/* ---------------------------------------------------------------------------- */

T_FILE* tdcFopen  (const char*     pFilename,
                   const char*     pMode)
{                                                  /*@ -abstract -dependenttrans */
   if ((pFilename == NULL) || (pMode == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcFopen: Wrong parameter (NULL)");
      return NULL;
   }

   return ((T_FILE *) fopen (pFilename, pMode));   /*@ =abstract =dependenttrans */
}

/* ---------------------------------------------------------------------------- */

INT32 tdcFclose (T_FILE*         pStream)
{
   if (pStream != NULL)
   {                                      
      return (fclose ((FILE *) pStream)); 
   }

   DEBUG_WARN (MOD_MAIN, "tdcFclose: Wrong parameter (NULL)");
   return (0);
}

/* ---------------------------------------------------------------------------- */

INT32 tdcFeof (T_FILE*     pStream)
{                                    
   if (pStream != NULL)
   {                                 
      return (feof ((FILE *) pStream)); 
   }

   DEBUG_WARN (MOD_MAIN, "tdcFeof: Wrong parameter (NULL)");
   return (0);
}

/* ---------------------------------------------------------------------------- */

UINT32  tdcFread  (void*           pBuffer,
                   UINT32          bufLen,
                   UINT32          count,
                   T_FILE*         pStream)                                      
{                                                                                         /*@ -abstract */
   if ((pBuffer == NULL) || (pStream == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcFread: Wrong parameter (NULL)");
      return 0;
   }

   return ((UINT32) fread (pBuffer, (size_t) bufLen, (size_t) count, (FILE *) pStream));  /*@ =abstract */
}

/* ---------------------------------------------------------------------------- */

UINT32 tdcFwrite (const void*    pBuffer,
                  UINT32         bufLen,
                  UINT32         count,
                  T_FILE*        pStream)
{                                                                                         /*@ -abstract */
   if ((pBuffer == NULL) || (pStream == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcFwrite: Wrong parameter (NULL)");
      return 0;
   }

   return ((UINT32) fwrite (pBuffer, (size_t) bufLen, (size_t) count, (FILE *) pStream)); /*@ =abstract */
}

/* ---------------------------------------------------------------------------- */

char* tdcFgets  (char*           pBuffer,
                 UINT32          bufSize,
                 T_FILE*         pStream)
{                                                              /*@ -abstract -temptrans */
   if ((pBuffer == NULL) || (pStream == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcFgets: Wrong parameter (NULL)");
      return NULL;
   }

   return (fgets (pBuffer, (int) bufSize, (FILE *) pStream));  /*@ =abstract =temptrans */
}

/* ---------------------------------------------------------------------------- */

INT32 tdcFFlush (T_FILE*         pStream)
{
   if (pStream == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcFFlush: Wrong parameter (NULL)");
      return EOF;
   }
   
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
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

typedef struct
{
               T_TDC_BOOL        bIgnore;
               int               sigNo;
               const char*       pName;
               void             (*pNewHandler) (int);
   /*@null@*/  void             (*pOldHandler) (int);
} T_TDC_SIGNAL_TAB;


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void    myDebugSignalFct (int    sigNo);

// ----------------------------------------------------------------------------

static T_TDC_SIGNAL_TAB      tdcSignalTab[] =
{
//  ignore     sig-nr        sig-name       sig-handler
   {TRUE,      SIGHUP,       "SIGHUP",      myDebugSignalFct,     NULL},
   {TRUE,      SIGINT,       "SIGINT",      myDebugSignalFct,     NULL},
   {TRUE,      SIGQUIT,      "SIGQUIT",     myDebugSignalFct,     NULL},
   {TRUE,      SIGILL,       "SIGILL",      myDebugSignalFct,     NULL},
   {TRUE,      SIGTRAP,      "SIGTRAP",     myDebugSignalFct,     NULL},
   {TRUE,      SIGABRT,      "SIGABRT",     myDebugSignalFct,     NULL},
   {TRUE,      SIGFPE,       "SIGFPE",      myDebugSignalFct,     NULL},
   {TRUE,      SIGKILL,      "SIGKILL",     myDebugSignalFct,     NULL},
   {TRUE,      SIGBUS,       "SIGBUS",      myDebugSignalFct,     NULL},
   {TRUE,      SIGSEGV,      "SIGSEGV",     myDebugSignalFct,     NULL},
   {TRUE,      SIGSYS,       "SIGSYS",      myDebugSignalFct,     NULL},
   {TRUE,      SIGPIPE,      "SIGPIPE",     myDebugSignalFct,     NULL},
   {TRUE,      SIGALRM,      "SIGALRM",     myDebugSignalFct,     NULL},
   {TRUE,      SIGTERM,      "SIGTERM",     myDebugSignalFct,     NULL},
   {TRUE,      SIGURG,       "SIGURG",      myDebugSignalFct,     NULL},
   {TRUE,      SIGSTOP,      "SIGSTOP",     myDebugSignalFct,     NULL},
   {TRUE,      SIGTSTP,      "SIGTSTP",     myDebugSignalFct,     NULL},
   {TRUE,      SIGCONT,      "SIGCONT",     myDebugSignalFct,     NULL},
   {TRUE,      SIGCHLD,      "SIGCHLD",     myDebugSignalFct,     NULL},
   {TRUE,      SIGTTIN,      "SIGTTIN",     myDebugSignalFct,     NULL},
   {TRUE,      SIGTTOU,      "SIGTTOU",     myDebugSignalFct,     NULL},
   {TRUE,      SIGPOLL,      "SIGPOLL",     myDebugSignalFct,     NULL},
   {TRUE,      SIGXCPU,      "SIGXCPU",     myDebugSignalFct,     NULL},
   {TRUE,      SIGXFSZ,      "SIGXFSZ",     myDebugSignalFct,     NULL},
   {TRUE,      SIGVTALRM,    "SIGVTALRM",   myDebugSignalFct,     NULL},
   {TRUE,      SIGPROF,      "SIGPROF",     myDebugSignalFct,     NULL},
   {TRUE,      SIGWINCH,     "SIGWINCH",    myDebugSignalFct,     NULL},
   {TRUE,      SIGUSR1,      "SIGUSR1",     myDebugSignalFct,     NULL},
   {TRUE,      SIGUSR2,      "SIGUSR2",     myDebugSignalFct,     NULL},
   {TRUE,      SIGRTMIN,     "SIGRTMIN",    myDebugSignalFct,     NULL},
   {TRUE,      SIGRTMAX,     "SIGRTMAX",    myDebugSignalFct,     NULL},
};


// ----- myDebugSignalFct() -----------------------------------------------
// Abstract    : myDebugSignalFct is for debug purposes only.
//             : It catches a signal - sent by the system - and logs its number
// Parameter(s): signr - The signal number sent by the system
// Return value: ---
// Remarks     :
// History     : 01-07-25     Ritz           TTC/EDE           modified
// ----------------------------------------------------------------------------
static void myDebugSignalFct (int  sigNo)
{
   DEBUG_ERROR1 (MOD_MAIN, "Signal (%d) occured", sigNo);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

void tdcInstallSignalHandler (void)
{
   UINT32          i;

   for (i = 0; i < TAB_SIZE (tdcSignalTab); i++)
   {
      if (!tdcSignalTab[i].bIgnore)
      {
         tdcSignalTab[i].pOldHandler = Signal (tdcSignalTab[i].sigNo, tdcSignalTab[i].pNewHandler);

         if (tdcSignalTab[i].pOldHandler == SIG_ERR)
         {
            char     text[100];

            (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Could not install %s-Handler", tdcSignalTab[i].pName);
            text[sizeof (text) - 1] = '\0';

            DEBUG_ERROR (MOD_MAIN, text);
         }
      }
   }
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcRemoveSignalHandler (void)
{
   UINT32            i;
   T_TDC_BOOL        bOk = TRUE;

   DEBUG_WARN (MOD_MAIN, "Remove Signal Handlers requested");

   for (i = 0; i < TAB_SIZE (tdcSignalTab); i++)
   {
      if (!tdcSignalTab[i].bIgnore)
      {
         if (Signal (tdcSignalTab[i].sigNo, tdcSignalTab[i].pOldHandler) == SIG_ERR)
         {
            char     text[100];

            (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Could not remove %s-Handler", tdcSignalTab[i].pName);
            text[sizeof (text) - 1] = '\0';

            DEBUG_ERROR (MOD_MAIN, text);
            bOk = FALSE;
         }
      }
   }

   DEBUG_WARN (MOD_MAIN, "Remove Signal Handlers finished");

   return (bOk);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

UINT32 tdcFSize (const char*   pModname, 
                 const char*   pFilename)
{
   UINT32      fileSize = (UINT32) 0;
   int         fd;

   if (pFilename == NULL)
   {
      DEBUG_WARN (pModname, "tdcFSize: Wrong parameter (NULL)");
      return 0;
   }

   fd = open (pFilename, O_RDONLY);

   if (fd != -1)
   {
      struct stat         fileStat;

      if (fstat (fd, &fileStat) == 0)
      {
         fileSize = (UINT32) fileStat.st_size;
      }
      else
      {
         char     text[200];

         (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Failed to call fstat (%s)", pFilename);

         DEBUG_WARN (pModname, text); 
      }

      (void) close (fd);
   }

   return (fileSize);
}
