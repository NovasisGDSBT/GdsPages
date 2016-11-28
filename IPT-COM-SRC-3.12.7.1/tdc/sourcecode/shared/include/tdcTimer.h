/*  $Id: tdcTimer.h 11727 2010-11-25 11:09:17Z gweiss $                */
/*                                                                      */
/*  DESCRIPTION    Timer Functions                                      */
/*                                                                      */
/*  AUTHOR         M.Ritz         PPC/EBT                               */
/*                                                                      */
/*  REMARKS                                                             */
/*                                                                      */
/*  DEPENDENCIES                                                        */
/*                                                                      */
/*  All rights reserved. Reproduction, modification, use or disclosure  */
/*  to third parties without express authority is forbidden.            */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002.            */

/* ------------------------------------------------------------------------- */

#if !defined (_TDC_TIMER_H)
   #define _TDC_TIMER_H

#include "tdcOsMisc.h"

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ------------------------------------------------------------------------- */

extern            void     tdcSleep       (UINT32              msecs);

/* ------------------------------------------------------------------------- */

typedef void T_SIG_FUNCTION (int);

extern int                 tdcInitITimer   (T_SIG_FUNCTION     sig_handler,
                                            UINT32             cycleTime);
extern T_TDC_BOOL          tdcRemoveITimer (UINT32           cycleTime);

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif

/* ------------------------------------------------------------------------- */

#endif



