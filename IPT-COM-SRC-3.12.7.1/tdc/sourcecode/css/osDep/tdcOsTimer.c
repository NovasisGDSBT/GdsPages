/*                                                                     */
/* $Id: tdcOsTimer.c 11727 2010-11-25 11:09:17Z gweiss $             */
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

typedef struct
{
               T_TDC_BOOL           bActive;
   /*@null@*/  WDOG_ID              timerId;
               int                  osTicks;
               UINT32               count;
   /*@null@*/  T_SIG_FUNCTION*      sigHandler;
} T_TIMER_PARAM;

static T_TIMER_PARAM    timerParam = {(T_TDC_BOOL) 0, NULL, 0, (UINT32) 0, NULL};

/* ---------------------------------------------------------------------------- */

static void timerFunction (int   arg)
{
   T_TIMER_PARAM*        pTimerParam = (T_TIMER_PARAM *) arg;

   if (    (pTimerParam->count    > 0)
        && (pTimerParam->timerId != NULL)
      )
   {                                /*@ -type */
      pTimerParam->sigHandler (0);  /*@ =type */

      pTimerParam->count--;

      if (wdStart (pTimerParam->timerId,
                   pTimerParam->osTicks,
                   (FUNCPTR) timerFunction,
                   arg) != OK)
      {
         DEBUG_WARN (MOD_MAIN, "Could not restart watchdog timer");
      }
   }
}

/* ---------------------------------------------------------------------------- */

int msec2OsTicks (UINT32     msecs)
{
   INT32       ticksPerSecond = sysClkRateGet ();
   INT32       osTicks        = (ticksPerSecond * msecs) / 1000;

   if (((ticksPerSecond * msecs) % 1000) != 0)
   {
     osTicks += 1; /* round up */
   }

   return (osTicks);
}

/* ---------------------------------------------------------------------------- */

int tdcInitITimer (T_SIG_FUNCTION      sigHandler,
                   UINT32              cycleTime)
{
   timerParam.timerId    = wdCreate ();
   timerParam.osTicks    = msec2OsTicks (cycleTime);
   timerParam.count      = MAX_UINT32;
   timerParam.sigHandler = sigHandler;

   if (timerParam.timerId == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "Could not create watchdog timer");
      return (FALSE);
   }
   else 
   {
      DEBUG_INFO1 (MOD_MAIN, "timeout (%d msec) successfully created", cycleTime);
   }

   if (wdStart (timerParam.timerId, 
                timerParam.osTicks, 
                (FUNCPTR) timerFunction, 
                (int) &timerParam) != OK)
   {
      DEBUG_WARN (MOD_MAIN, "Could not start watchdog timer");
      return (FALSE);
   }

   timerParam.bActive = TRUE;

   DEBUG_INFO (MOD_MAIN, "watchdogtimer successfully started");

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcRemoveITimer (UINT32           cycleTime)
{
   T_TDC_BOOL     bOk = TRUE;
   TDC_UNUSED(cycleTime);

   DEBUG_WARN (MOD_ITIMER, "Termination requested");

   if (    (timerParam.bActive)
        && (timerParam.timerId != NULL)
      )
   {
      bOk = FALSE;

      if (wdDelete (timerParam.timerId) == OK)
      {
         timerParam.bActive = FALSE;
         bOk                = TRUE;
      }
   }

   if (bOk)
   {
      DEBUG_WARN (MOD_ITIMER, "Termination finished");
   }
   else
   {
      DEBUG_WARN (MOD_ITIMER, "Termination failed");
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

void tdcSleep (UINT32    delayTimeMSec)
{
   INT32       osTicks  = msec2OsTicks (delayTimeMSec);

   if (taskDelay (osTicks) != OK)
   {
      DEBUG_WARN1 (MOD_MAIN, "Error calling taskDelay (%d)", osTicks);
   }
}

/* ---------------------------------------------------------------------------- */



