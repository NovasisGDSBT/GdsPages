//
// $Id: tdcOsThread.c 32751 2014-05-22 08:54:22Z bremenyi $
//
// DESCRIPTION    Daemon for IP-Train Diretory Client (TDC)
//
// AUTHOR         M.Ritz      PPC/EBT
//
// REMARKS        The switch LINUX has to be set
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


#if !defined (LINUX)
   #error "This is the Linux Version of tdcDaemonLin, i.e. LINUX has to be specified"
#endif

// ----------------------------------------------------------------------------

#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include "tdc.h"
#include "tdcThread.h"

// ----------------------------------------------------------------------------

typedef void* (*PTHREAD_FUNC) (void* pArgV);

T_THREAD_ID startupTdcSingleThread (const T_THREAD_FRAME*   pTdcThreadId)
{
   T_THREAD_ID     pThreadId = NULL;

   if (*pTdcThreadId->pBool)
   {
      if ((pThreadId = tdcAllocMem ((UINT32) sizeof (pthread_t))) != NULL)
      {
         char           text[81];
         pthread_t      threadId;

         (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Try to create new thread (%s)", pTdcThreadId->pTaskName);
         text[sizeof (text) - 1] = '\0';
         DEBUG_INFO (MOD_MAIN, text);

         /*
            NOTE(-SSS-):
               Check for system error number!
         */
         errno = 0;
         
         if (pthread_create (&threadId,
                             NULL,
                             (PTHREAD_FUNC) pTdcThreadId->pThreadFunc,
                             pTdcThreadId->pArgV)                         != 0)
         {
            (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Could not create new thread (%s), [errno=%d]", pTdcThreadId->pTaskName, errno);
            text[sizeof (text) - 1] = '\0';

            DEBUG_ERROR (MOD_MAIN, text);
            tdcFreeMem  (pThreadId);
            pThreadId = NULL;
         }
         else
         {                                           /*@ -nullderef */
            * ((pthread_t *) pThreadId) = threadId;  /*@ =nullderef */

            /*
               NOTE(-SSS-):
               Indicate that the thread TH is never to be joined with PTHREAD_JOIN.
               The resources of TH will therefore be freed immediately when it
               terminates, instead of waiting for another thread to perform PTHREAD_JOIN
               on it.
            */
            DEBUG_INFO2 (MOD_IPC, "MainThread: call pthread_detach (%ld) for thread (%s)", threadId, pTdcThreadId->pTaskName);
            errno = pthread_detach (threadId);
            if ( errno != 0)
            {
               (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Could not detach from new thread (%s), [errno=%d]", pTdcThreadId->pTaskName, errno);
               text[sizeof (text) - 1] = '\0';
               DEBUG_ERROR (MOD_MAIN, text);
            }
         }
      }
      else
      {
         DEBUG_ERROR (MOD_MAIN, "Could not create new thread - out of dynamic memory!");
      }
   }

   return (pThreadId);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------




