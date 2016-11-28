/*                                                                     */
/* $Id: tdcOsThread.c 11184 2009-05-14 13:28:06Z mritz $           */
/*                                                                     */
/* DESCRIPTION    Thread handling for VXWORKS                          */
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

#include <vxWorks.h>
#include <stdio.h>
#include <taskLib.h>

#define TDC_BASETYPES_H    /* Use original VxWorks Header File definitions */
#include "tdc.h"
#include "tdcThread.h"

/* ---------------------------------------------------------------------------- */

T_THREAD_ID startupTdcSingleThread (const T_THREAD_FRAME*    pTdcThreadId)
{
   T_THREAD_ID     pThreadId = NULL;

   if (*pTdcThreadId->pBool)
   {
      if ((pThreadId = tdcAllocMem ((UINT32) sizeof (int))) != NULL)
      {
         char           text[81];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                             "Try to create new task (%s)", pTdcThreadId->pTaskName);
         text[sizeof (text) - 1] = '\0';
         DEBUG_INFO (MOD_MAIN, text);

         * ((int *) pThreadId) = taskSpawn ((char *) pTdcThreadId->pTaskName,
                                            (int) pTdcThreadId->priority,
                                            0,                                   /* options */
                                            pTdcThreadId->stackSize,
                                            (FUNCPTR) pTdcThreadId->pThreadFunc,
                                            (int) pTdcThreadId->pArgV,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0);

         if ((* ((int *) pThreadId)) == ERROR)
         {
            (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                                "Could not create new task (%s)\n", pTdcThreadId->pTaskName);
            text[sizeof (text) - 1] = '\0';
            DEBUG_ERROR (MOD_MAIN, text);
            tdcFreeMem  (pThreadId);
            pThreadId = NULL;
         }
      }
      else
      {
         DEBUG_ERROR (MOD_MAIN, "Could not spawn new task - out of dynamic memory!");
      }
   }

   return (pThreadId);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
/* ---------------------------------------------------------------------------- */




