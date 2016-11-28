//
// $Id: tdcOsThread.c 11619 2010-07-22 10:04:00Z bloehr $
//
// DESCRIPTION    Daemon for IP-Train Diretory Client (TDC)
//
// AUTHOR         M.Ritz      PPC/EBT
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
   #error "This is the INTEGRITY Version of tdcOsThread, "
          "i.e. INTEGRITY has to be specified"
#endif

// ----------------------------------------------------------------------------

#include <INTEGRITY.h>
#include <string.h>
#include <pthread.h>
#include <atapi_commands.h>	 

#include "tdc.h"
#include "tdcThread.h"

// ----------------------------------------------------------------------------

typedef void* (*PTHREAD_FUNC) (void* pArgV);


void TaskWrapper(void)
{
    const T_THREAD_FRAME*   pTdcThreadId;
    Error retVal;

    if ((retVal = GetTaskIdentification(CurrentTask(), (Address *)&pTdcThreadId)) == Success)
    {
        if (pTdcThreadId != NULL)
        {
            pTdcThreadId->pThreadFunc(pTdcThreadId->pArgV);
            printf("Task tTdcMain exiting\n");
            Exit(0);
        }
    }
    else
    {
        char taskName[120] = "";
        Address lengd;

        GetTaskName(CurrentTask(), taskName, 120, &lengd);
        
        DEBUG_ERROR2(MOD_MAIN, "Could not get task identification for task '%s' errno %s\n", 
                     taskName, ErrorString(retVal));
    }

    Exit(255);
}

T_THREAD_ID startupTdcSingleThread (const T_THREAD_FRAME*   pTdcThreadId)
{
   T_THREAD_ID     pThreadId = NULL;

   if (pTdcThreadId == NULL)
   {    
      DEBUG_ERROR (MOD_MAIN, "startupTdcSingleThread: Wrong parameter (NULL) for thread start up");
      return pThreadId;
   }
    
   if (*pTdcThreadId->pBool)
   {
      if ((pThreadId = tdcAllocMem ((UINT32) sizeof (Task))) != NULL)
      {
         char           text[81];

         (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Try to create new task (%s)", pTdcThreadId->pTaskName);
         text[sizeof (text) - 1] = '\0';
         DEBUG_INFO (MOD_MAIN, text);

         if (CommonCreateTask((int) pTdcThreadId->priority,
                              (Address)TaskWrapper,
                              pTdcThreadId->stackSize,
                              (char *) pTdcThreadId->pTaskName,
                              pThreadId)                         != Success)

         {
            (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Could not create new task (%s)", pTdcThreadId->pTaskName);
            text[sizeof (text) - 1] = '\0';

            DEBUG_ERROR (MOD_MAIN, text);
            tdcFreeMem  (pThreadId);
            pThreadId = NULL;
            return pThreadId;
         }

         if (SetTaskIdentification(*pThreadId, (Address)pTdcThreadId) != Success)
         {  
             (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Could not set task arguments (%s)", pTdcThreadId->pTaskName);
             text[sizeof (text) - 1] = '\0';

             DEBUG_ERROR (MOD_MAIN, text);
             CommonCloseTask(*pThreadId);
             tdcFreeMem  (pThreadId);
             pThreadId = NULL;
             return pThreadId;
         }

         if (RunTask(*pThreadId) != Success)
         {
             (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Could not start task (%s)", pTdcThreadId->pTaskName);
             text[sizeof (text) - 1] = '\0';

             DEBUG_ERROR (MOD_MAIN, text);
             CommonCloseTask(*pThreadId);
             tdcFreeMem  (pThreadId);
             pThreadId = NULL;
             return pThreadId;
         }
         }
         else
      {
         DEBUG_ERROR (MOD_MAIN, "Could not create new task - out of dynamic memory!");
      }
   }

   return (pThreadId);
}


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------




