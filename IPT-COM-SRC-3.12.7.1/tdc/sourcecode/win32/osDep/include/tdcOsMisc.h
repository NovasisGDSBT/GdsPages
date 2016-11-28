//
// $Id: tdcOsMisc.h 11018 2008-10-15 15:13:56Z tgkamp $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interface
//
// AUTHOR         M.Ritz         PPC/EBT
//
// REMARKS
//
// DEPENDENCIES   Either the switch VXWORKS, LINUX or WIN32 has to be set
//
//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Bombardier Transportation GmbH, Germany, 2002.
//


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


#if !defined (_TDC_OS_MISC_H)
   #define _TDC_OS_MISC_H


// ----------------------------------------------------------------------------


#ifdef __cplusplus                            // to be compatible with C++
   extern "C" {
#endif

// ----------------------------------------------------------------------------

#define TASK_TYPE                   0

// ----------------------------------------------------------------------------

/*@ -exportlocal */
extern int        tdcRaiseSIGABRT (void);
/*@ =exportlocal */

/*@ -exportlocal */
extern int        tdcSelect       (int             nfds,
                                   void*           pReadSet,
                                   void*           pWriteSet,
                                   void*           pExceptSet,
                                   INT32           timeout);
/*@ =exportlocal */

// ----------------------------------------------------------------------------


extern T_TDC_BOOL tdcStreamReadN  (const char*     pModname,
                                   T_TDC_STREAM    fdRd,
                                   void*           pReadBuf,
                                   UINT32*         pBytesRead,
                                   UINT32          bytesExpected,
                                   int             flags);

extern T_TDC_BOOL tdcStreamWriteN (const char*     pModname,
                                   T_TDC_STREAM    fdWr,
                                   void*           pWriBuf,
                                   UINT32*         pBytesWritten,
                                   UINT32          bytesExpected,
                                   int             flags);


// ----------------------------------------------------------------------------


#ifdef __cplusplus                            // to be compatible with C++
   }
#endif


#endif
