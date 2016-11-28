/*                                                                     */
/* $Id: tdcResolve.h 11289 2009-08-18 09:04:45Z mritz $             */
/*                                                                     */
/* DESCRIPTION                                                         */
/*                                                                     */
/* AUTHOR         M.Ritz         PPC/EBT                               */
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
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#if !defined (_TDC_RESOLVE_H)
   #define _TDC_RESOLVE_H

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

extern UINT32     tdcGetOwnIpAddrs   (const char*        pModname,
                                      UINT32             maxAddrCnt,   
                            /*@out@*/ UINT32             ipAddrs[]);     /* output */

extern T_TDC_BOOL tdcAssertMCsupport (const char*        pModName,
                                      UINT32             ipAddr);

extern T_TDC_BOOL tdcGetHostByAddr   (const char*         pModName,
                                      UINT32              ipAddr,
                                      UINT32              hostNameLen,
                            /*@out@*/ char*               pHostName);

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif

/* ---------------------------------------------------------------------------- */

#endif
