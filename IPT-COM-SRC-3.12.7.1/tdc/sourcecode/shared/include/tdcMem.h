/*  $Id: tdcMem.h 11018 2008-10-15 15:13:56Z tgkamp $                 */
/*                                                                      */
/*  DESCRIPTION    Memory handling                                      */
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

#if !defined (_TDC_MEM_H)
   #define _TDC_MEM_H

#include "tdcOsMisc.h"

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ------------------------------------------------------------------------- */

extern            INT32    tdcMemCmp      (          const void*  pBuf1, const void*   pBuf2,   UINT32  len);
extern            void*    tdcMemCpy      (/*@out@*/ void*        pDest, const void*   pSrc,    UINT32  len);
extern            void*    tdcMemSet      (/*@out@*/ void*        pDest, int           c,       UINT32  cnt);
extern            void     tdcMemClear    (/*@out@*/ void*        pDest, UINT32        cnt);

extern            void     tdcFreeMem     (/*@null@*/ void*    pMem);
extern /*@null@*/ void*    tdcAllocMem    (UINT32              size);
extern /*@null@*/ void*    tdcAllocMemChk (const char*         pModname,
                                           UINT32              size);

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif

/* ------------------------------------------------------------------------- */

#endif



