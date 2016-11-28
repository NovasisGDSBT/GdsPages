/*                                                                           */
/* $Id:: tdcTCyclic.c 11558 2010-05-11 15:32:39Z gweiss                 $    */
/*                                                                           */
/* DESCRIPTION                                                               */
/*                                                                           */
/* AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                           */
/* REMARKS                                                                   */
/*                                                                           */
/* DEPENDENCIES   Either the switch VXWORKS, LINUX or WIN32 has to be set    */
/*                                                                           */
/* All rights reserved. Reproduction, modification, use or disclosure        */
/* to third parties without express authority is forbidden.                  */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.                  */
/*                                                                           */

/* ---------------------------------------------------------------------------- */

#include "tdc.h"
#include "tdcInit.h"
#include "tdcProcData.h"
#include "tdcIptCom.h"
#include "tdcThread.h"

/* ---------------------------------------------------------------------------- */

void tdcTCyclicInit (void*       pArgV)
{
   TDC_UNUSED (pArgV)
}

/* ---------------------------------------------------------------------------- */


static void tCyclicBody (void)
{
   tdcProcDataCycle ();
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcTerminateTCyclic (void)
{
   int      i;

   DEBUG_WARN (MOD_CYC, "Termination requested");

   for (i = 0; i < 10; i++, tdcSleep (100))
   {
      if (tdcMutexLock (MOD_CYC, taskIdMutexId) == TDC_MUTEX_OK)
      {
         if (tdcThreadIdTab[T_CYCLIC_INDEX] == NULL)
         {
            (void) tdcMutexUnlock (MOD_CYC, taskIdMutexId);
            DEBUG_WARN     (MOD_CYC, "Termination finished");
            return (TRUE);
         }

         (void) tdcMutexUnlock (MOD_CYC, taskIdMutexId);
      }
   }

   DEBUG_WARN (MOD_CYC, "Termination failed");

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

void tdcTCyclic (void*  pArgV)
{
   TDC_UNUSED(pArgV)

   if (timerSemaId != NULL)
   {
      for (;;)
      {
         if (bTerminate)
         {
            DEBUG_INFO (MOD_CYC, "Aborting tdcTCyclic() due to Terminate-Request");
            break;
         }

         if (tdcWaitSema (MOD_CYC, timerSemaId, TDC_SEMA_WAIT_FOREVER) == TDC_SEMA_OK)
         {
           tCyclicBody ();
         }
      }
   }
   else
   {
      DEBUG_ERROR (MOD_CYC, "timerSemaId not initialized");
   }

   tdcTerminatePD (MOD_CYC);
}







