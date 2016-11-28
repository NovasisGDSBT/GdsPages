/*
 *
 *  $Id: tdcOsTimer.c 11754 2010-11-30 13:14:53Z gweiss $
 *
 * DESCRIPTION    Front-End for miscelleanous functions like hamster,
 *            alloc... as an OS independant interface
 *
 * AUTHOR         M.Ritz         PPC/EBT
 *
 * REMARKS        The switch WIN32 has to be set
 *
 * DEPENDENCIES
 *
 * MODIFICATIONS:
 *
 * CR-685 (Gerhard Weiss, 2010-11-25)
 *       Corrected release of timer
 *
 * All rights reserved. Reproduction, modification, use or disclosure
 * to third parties without express authority is forbidden.
 * Copyright Bombardier Transportation GmbH, Germany, 2002-2010.
 */


/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */


#if !defined (WIN32)
   #error "This is the Windows Version of TDC_OS_MISC, i.e. WIN32 has to be specified"
#endif

#include <windows.h>
#include <mmsystem.h>         // requires winmm.lib !!

/* -------------------------------------------------------------------------- */

#include "tdc.h"

/* -------------------------------------------------------------------------- */

typedef struct
{
   T_TDC_BOOL           bActive;
   MMRESULT             nTimerID;
} T_TIMER_PARAM;

static T_TIMER_PARAM    timerParam = {(T_TDC_BOOL) 0, (MMRESULT) 0};

// ----------------------------------------------------------------------------

void tdcSleep (UINT32    msecs)
{
	Sleep (msecs);
}

/* ------------------------------------------------------------------------- */
                                                                                                /*@-exportlocal@*/
/*lint -e(950) , allow reserved ANSI keyword __stdcall once for the callback function*/
static void CALLBACK timerCallback (UINT     nTimerID,
                                    UINT     wMsg,
                                    DWORD    dwUser,
                                    DWORD    dw1,
                                    DWORD    dw2)
{
   T_SIG_FUNCTION*   mySigHandler = (T_SIG_FUNCTION *) dwUser;
   /*@ -type */

   TDC_UNUSED (nTimerID)
   TDC_UNUSED (wMsg)
   TDC_UNUSED (dw1)
   TDC_UNUSED (dw2)

   mySigHandler (0);    /*@ =type */
}                                                                                               /*@=exportlocal@*/

// ----------------------------------------------------------------------------

int tdcInitITimer (T_SIG_FUNCTION   sigHandler,
                   UINT32           cycleTime)
{
   UINT        uDelay    = cycleTime;
   UINT        uTimerRes = cycleTime;
   UINT        uPeriod   = cycleTime;
   T_TDC_BOOL  bOk = TRUE;

   (void) timeBeginPeriod (uPeriod);
                                                                                                   /*@ -type */
   timerParam.nTimerID = timeSetEvent (uDelay, uTimerRes, timerCallback, (DWORD) sigHandler, TIME_PERIODIC|TIME_KILL_SYNCHRONOUS);  /*@ =type */

   if (timerParam.nTimerID == (MMRESULT) NULL)
   {
      bOk = FALSE;
      DEBUG_ERROR (MOD_MAIN, "Could not create the Interval-Timer");
   }

   timerParam.bActive = TRUE;

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcRemoveITimer (UINT32           cycleTime)
{
   T_TDC_BOOL     bOk = TRUE;
   
   (void) timeEndPeriod (cycleTime);

   DEBUG_WARN (MOD_ITIMER, "Termination requested");

   if (timerParam.bActive)
   {
      bOk = FALSE;

      timerParam.bActive = FALSE;
      
      if (timeKillEvent (timerParam.nTimerID) == TIMERR_NOERROR)
      {
         bOk = TRUE;
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


