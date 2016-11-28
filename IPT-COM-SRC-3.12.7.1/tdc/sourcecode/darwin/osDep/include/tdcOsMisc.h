//
// $Id: tdcOsMisc.h 11853 2012-02-10 17:14:13Z bloehr $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interface
//
// AUTHOR         M.Ritz         PPC/EBT
//
// REMARKS
//
// DEPENDENCIES   Either the switch VXWORKS, DARWIN or WIN32 has to be set
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
extern int        tdcSelect       (             int             nfds,
                                   /*@null@*/   void*           pReadSet,
                                   /*@null@*/   void*           pWriteSet,
                                   /*@null@*/   void*           pExceptSet,
                                                INT32           timeout); 
/*@ =exportlocal */

// ----------------------------------------------------------------------------


extern T_TDC_BOOL tdcStreamReadN  (          const char*     pModname,
                                             T_TDC_STREAM    fdRd,
                                   /*@out@*/ void*           pReadBuf,
                                   /*@out@*/ UINT32*         pBytesRead,
                                             UINT32          bytesExpected);

extern T_TDC_BOOL tdcStreamWriteN (          const char*     pModname,
                                             T_TDC_STREAM    fdWr,
                                             void*           pWriBuf,
                                   /*@out@*/ UINT32*         pBytesWritten,
                                             UINT32          bytesExpected);


// ----------------------------------------------------------------------------


#ifdef __cplusplus                            // to be compatible with C++
   }
#endif


#endif
