/*                                                                            */
/*  $Id: tdcOsMisc.h 11018 2008-10-15 15:13:56Z tgkamp $                    */
/*                                                                            */
/*  DESCRIPTION    Front-End for miscelleanous functions like hamster,        */
/*                 alloc... as an OS independant interface                    */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBTS                                    */
/*                                                                            */
/*  REMARKS                                                                   */
/*                                                                            */
/*  DEPENDENCIES   Either the switch VXWORKS, LINUX or WIN32 has to be set    */
/*                                                                            */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002.                  */
/*                                                                            */

/*  ----------------------------------------------------------------------------  */

#if !defined (_TDC_OS_MISC_H)
   #define _TDC_OS_MISC_H

/*  ----------------------------------------------------------------------------  */

#ifdef __cplusplus                                 /* to be compatible with C++ */
   extern "C" {
#endif

/*  ----------------------------------------------------------------------------  */

#define AS_TYPE_AP_C                2              /* Application is written in the
                                                      programming language 'C'. */
#define TASK_TYPE                   AS_TYPE_AP_C

/*  ----------------------------------------------------------------------------  */

extern int msec2OsTicks             (UINT32     msecs);

/*  ----------------------------------------------------------------------------  */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif


#endif



