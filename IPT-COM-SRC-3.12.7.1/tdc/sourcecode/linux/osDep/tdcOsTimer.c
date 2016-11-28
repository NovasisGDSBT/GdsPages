//
// $Id: tdcOsTimer.c 11727 2010-11-25 11:09:17Z gweiss $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interfaceTimer handling
//
// AUTHOR         M.Ritz         PPC/EBT
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
   #error "This is the Linux Version of TDC_OS_MISC, i.e. LINUX has to be specified"
#endif

// ----------------------------------------------------------------------------

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>


#include "tdc.h"

/* ---------------------------------------------------------------------------- */

typedef struct
{
               T_TDC_BOOL           bActive;
   /*@null@*/  T_SIG_FUNCTION*      oldSigHandler;
               struct itimerval     oldItInterval;
} T_TIMER_PARAM;

static T_TIMER_PARAM    timerParam = {FALSE, NULL,
             { {(long) 0, (long) 0}, 
               {(long) 0, (long) 0}}
             };

// ----------------------------------------------------------------------------

int tdcInitITimer (T_SIG_FUNCTION   sigHandler,
                   UINT32           cycleTime)
{
   T_TDC_BOOL        success = FALSE;

   timerParam.oldSigHandler = Signal (SIGALRM, sigHandler);

   if (timerParam.oldSigHandler == SIG_ERR)
   {
      DEBUG_WARN (MOD_ITIMER, "Could Not Install Signal-Handler");
   }
   else
   {
      // 2008-02-20, MRi - POSIX.1-2001 requires tv_usec to be in the range 0 ... 999999 !!!

      struct timeval       cycleTimeVal = {(cycleTime * 1000) / 1000000,  (cycleTime * 1000) % 1000000};
      struct itimerval     itInterval;

      itInterval.it_interval = cycleTimeVal;
      itInterval.it_value    = cycleTimeVal;

      //DEBUG_INFO (MODITIMER, "signal (SIGALRM, ...) successfully called");

      if (setitimer (ITIMER_REAL, &itInterval, &timerParam.oldItInterval) != 0)
      {
         DEBUG_WARN2 (MOD_ITIMER, "Failed to install Interval-Timer (%d, %d)", cycleTimeVal.tv_sec, cycleTimeVal.tv_usec);
      }
      else
      {
         DEBUG_INFO2 (MOD_ITIMER, "setitimer successfully called(%d, %d)", cycleTimeVal.tv_sec, cycleTimeVal.tv_usec);
         timerParam.bActive = TRUE;
         success = TRUE;
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
      bOk = FALSE;

      if (Signal (SIGALRM, timerParam.oldSigHandler) != SIG_ERR)
      {
         if (setitimer (ITIMER_REAL, &timerParam.oldItInterval, NULL) != 0)
         {
            timerParam.bActive = FALSE;
            bOk                = TRUE;
         }
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

// ----------------------------------------------------------------------------

void tdcSleep (UINT32    msecs)
{
    (void) usleep ((unsigned long) (msecs * 1000));
}

// ----------------------------------------------------------------------------



