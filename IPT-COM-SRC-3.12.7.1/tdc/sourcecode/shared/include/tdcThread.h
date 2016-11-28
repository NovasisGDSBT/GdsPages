/*                                                                     */
/* $Id: tdcThread.h 11512 2010-04-08 09:35:16Z gweiss $              */
/*                                                                     */
/* DESCRIPTION    Thread Definitions for TDC                           */
/*                                                                     */
/* AUTHOR         M.Ritz         PPC/EBTS                              */
/*                                                                     */
/* REMARKS                                                             */
/*                                                                     */
/* DEPENDENCIES                                                        */
/*                                                                     */
/*                                                                     */
/* All rights reserved. Reproduction, modification, use or disclosure  */
/* to third parties without express authority is forbidden.            */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.            */
/*                                                                     */

/* ---------------------------------------------------------------------------- */

#if !defined (TDC_THREAD_H)
   #define TDC_THREAD_H

#if defined(__INTEGRITY)
#include <INTEGRITY.h>
#endif
/* ---------------------------------------------------------------------------- */

#if defined (__cplusplus)
extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

typedef void* (*T_THREAD_FUNC)   (/*@in@*/ void*   pArgV);

typedef struct
{
   const T_TDC_BOOL*       pBool;
   const char*             pApplName;        
   const char*             pTaskName;        /* or threadName */
   INT32                   applType;         /* AS_TYPE_AP_C ... - CSS only */
   UINT8                   priority;         /* 60 .. 249 - CSS only */
   INT32                   stackSize;        /* CSS only */
   /*@null@*/ void*        pArgV;            /* only one parameter, posix-compatible */
   T_THREAD_FUNC           pThreadFunc;      /* == entryPoint in CSS */
   INT32                   threadIdx;        /* entry into thread table */
} T_THREAD_FRAME;

/* ---------------------------------------------------------------------------- */

#if defined(__INTEGRITY)
typedef /*@null@*/ Task*      T_THREAD_ID;
#else
typedef /*@null@*/ void*      T_THREAD_ID;
#endif

extern /*@null@*/ T_THREAD_ID startupTdcSingleThread  (const T_THREAD_FRAME*   pTdcThreadId);

/* ---------------------------------------------------------------------------- */

extern T_THREAD_ID                        tdcThreadIdTab[];

/* ---------------------------------------------------------------------------- */


#if defined (__cplusplus)
}
#endif

/* ---------------------------------------------------------------------------- */

#endif
