//
// $Id: tdcOsTimer.c 11727 2010-11-25 11:09:17Z gweiss $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interfaceTimer handling
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
   #error "This is the INTEGRITY Version of tdcOsTimer, "
          "i.e. INTEGRITY has to be specified"
#endif

// ----------------------------------------------------------------------------

#include <INTEGRITY.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>


#include "tdc.h"

/* ---------------------------------------------------------------------------- */

typedef struct
{
               T_TDC_BOOL           bActive;
   /*@null@*/  T_SIG_FUNCTION*      SigHandler;
} T_TIMER_PARAM;

static T_TIMER_PARAM    timerParam = {FALSE, NULL,};
typedef struct InterruptHandlerStruct
{
    Value dummy;
    void *pHandler;
} InterruptHandlerStruct;

Activity                InterruptActivity;
Clock                   InterruptClock;
Value                   AlarmCounter;
InterruptHandlerStruct  TheInterruptHandlerStruct;

Object TimerHandler(InterruptHandlerStruct *pArg)
{
    timerParam.SigHandler(SIGALRM);
    return (Object)InterruptClock;
}

// ----------------------------------------------------------------------------

int tdcInitITimer (T_SIG_FUNCTION   sigHandler,
                   UINT32           cycleTime)
{
   T_TDC_BOOL        success = FALSE;
   Time Interval;
   long long fraction;
   Error retVal;

   if(sigHandler == NULL)
   {
      DEBUG_ERROR (MOD_ITIMER, "tdcInitITimer: Wrong parameter (NULL)");
      return success;
   }

   TheInterruptHandlerStruct.pHandler = TimerHandler;

   timerParam.SigHandler = sigHandler;
   
   retVal = CreateVirtualClock(HighestResStandardClock, CLOCK_READTIME | CLOCK_ALARM, &InterruptClock);

   if(retVal != Success)
   {
      DEBUG_ERROR (MOD_ITIMER, "Timer creation failed");
   }
   else
   {
      Interval.Seconds = cycleTime / 1000;
      /* 0x10000000 / 1000 = 4294967.296 (0x418937) */
      fraction = ((long long)(cycleTime % 1000)) << 32;
      Interval.Fraction = (UINT4)(fraction / 1000);

      retVal = SetClockAlarm(InterruptClock, true, NULLTime, &Interval);

      if(retVal != Success)
      {
         DEBUG_ERROR (MOD_ITIMER, "Timer create failed");
      }
      else
      {
         retVal = CreateActivity(CurrentTask(), 2, true, (Value)&TheInterruptHandlerStruct, &InterruptActivity);

         if(retVal != Success)
         {
            DEBUG_ERROR (MOD_ITIMER, "Timer activity create failed");
         }
         else
         {
            retVal = AsynchronousReceive(InterruptActivity, (Object)InterruptClock, NULL);
            
            if(retVal != Success)
            {
               DEBUG_ERROR (MOD_ITIMER, "Timer asynchronous receive failed");
            }
            else
            {
               timerParam.bActive = TRUE;

               success = TRUE;
            }
         }
      }
   }
   return (success);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcRemoveITimer (UINT32           cycleTime)
{
   T_TDC_BOOL     bOk = TRUE;
   TDC_UNUSED(cycleTime);

   DEBUG_WARN (MOD_ITIMER, "Termination requested");

   if (timerParam.bActive)
   {
      CloseClock(InterruptClock);
      CloseActivity(InterruptActivity);
            timerParam.bActive = FALSE;
            bOk                = TRUE;
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

// ----------------------------------------------------------------------------

void tdcSleep (UINT32    msecs)
{
    (void) usleep ((unsigned long) (msecs * 1000));
}

// ----------------------------------------------------------------------------



