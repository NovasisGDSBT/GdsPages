//
// $Id: tdcOsThread.c 35423 2015-02-02 15:32:02Z gweiss $
//
// DESCRIPTION    Thread hansdling for WIN32
//
// AUTHOR         M.Ritz      PPC/EBT
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
   #error "This is the Windows Version of tdcDaemonWin, i.e. WIN32 has to be specified"
#endif


// ----------------------------------------------------------------------------

#include <string.h>
#include <signal.h>
#include <winsock2.h>
#include <pthread.h>
#include <errno.h>

#include "tdc.h"
#include "tdcThread.h"

// ----------------------------------------------------------------------------

typedef void* (*PTHREAD_FUNC) (void* pArgV);

T_THREAD_ID startupTdcSingleThread (const T_THREAD_FRAME*   pTdcThreadId)
{
   T_THREAD_ID     pTaskId = NULL;

   if (*pTdcThreadId->pBool)
   {
      if ((pTaskId = tdcAllocMem ((UINT32) sizeof (pthread_t))) != NULL)
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
            (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Could not create new thread (%s)", pTdcThreadId->pTaskName);
            text[sizeof (text) - 1] = '\0';
            DEBUG_ERROR (MOD_MAIN, text);
            tdcFreeMem  (pTaskId);
            pTaskId = NULL;
         }
         else
         {                                         /*@ -nullderef @*/
            * ((pthread_t *) pTaskId) = threadId;  /*@ =nullderef @*/
            
            /*
             NOTE(-SSS-):
             Indicate that the thread TH is never to be joined with PTHREAD_JOIN.
             The resources of TH will therefore be freed immediately when it
             terminates, instead of waiting for another thread to perform PTHREAD_JOIN
             on it.
             */
            DEBUG_INFO2 (MOD_IPC, "MainThread: call pthread_detach (0x%08x) for thread (%s)", threadId.p, pTdcThreadId->pTaskName);
           
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

   return (pTaskId);
}


