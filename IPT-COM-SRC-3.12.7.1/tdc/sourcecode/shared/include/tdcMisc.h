/*  $Id: tdcMisc.h 11184 2009-05-14 13:28:06Z mritz $              */
/*                                                                      */
/*  DESCRIPTION    Miscellaneous Functions                              */
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

#if !defined (_TDC_MISC_H)
   #define _TDC_MISC_H

#include "tdcOsMisc.h"

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ------------------------------------------------------------------------- */

#if defined (O_HAVE_CONVERSION)
   #define PD_H2N8(x)                      tdcH2N8(x)
   #define PD_H2N16(x)                     tdcH2N16(x)
   #define PD_H2N32(x)                     tdcH2N32(x)
   #define PD_N2H8(x)                      tdcN2H8(x)
   #define PD_N2H16(x)                     tdcN2H16(x)
   #define PD_N2H32(x)                     tdcN2H32(x)

   #define MD_H2N8(x)                      tdcH2N8(x)
   #define MD_H2N16(x)                     tdcH2N16(x)
   #define MD_H2N32(x)                     tdcH2N32(x)
   #define MD_N2H8(x)                      tdcN2H8(x)
   #define MD_N2H16(x)                     tdcN2H16(x)
   #define MD_N2H32(x)                     tdcN2H32(x)

   #define CONVERSION_INFO                 "Compiler Switch \"O_HAVE_CONVERSION\" for IPTDir is set"
#else
   #define PD_H2N8(x)                      (x)
   #define PD_H2N16(x)                     (x)
   #define PD_H2N32(x)                     (x)
   #define PD_N2H8(x)                      (x)
   #define PD_N2H16(x)                     (x)
   #define PD_N2H32(x)                     (x)

   #define MD_H2N8(x)                      (x)
   #define MD_H2N16(x)                     (x)
   #define MD_H2N32(x)                     (x)
   #define MD_N2H8(x)                      (x)
   #define MD_N2H16(x)                     (x)
   #define MD_N2H32(x)                     (x)

   #define CONVERSION_INFO                 "Compiler Switch \"O_HAVE_CONVERSION\" for IPTDir is NOT set"
#endif

/* ------------------------------------------------------------------------- */

typedef void                              T_FILE;
extern /*@null@*/ T_FILE*                 tdcFopen  (          const char*    pFilename,
                                                               const char*    pMode);
extern INT32                              tdcFclose (          T_FILE*        pStream);
extern UINT32                             tdcFread  (/*@out@*/ void*          pBuffer,
                                                               UINT32         size,
                                                               UINT32         count,
                                                               T_FILE*        pStream);
extern UINT32                             tdcFwrite (          const void*    pBuffer,
                                                               UINT32         bufLen,
                                                               UINT32         count,
                                                               T_FILE*        pStream);

extern INT32                              tdcFeof   (          T_FILE*        pStream);
extern /*@null@*/ char*                   tdcFgets  (          char*          pBuffer,
                                                               UINT32         bufSize,
                                                               T_FILE*        pStream);
extern INT32                              tdcFFlush (          T_FILE*        pStream);

extern UINT32                             tdcFSize  (          const char*   pModname, 
                                                               const char*    pFilename);

/* ------------------------------------------------------------------------- */

extern /*@null@*/ const char*             tdcGetEnv  (const char*  pVarName);

/* ------------------------------------------------------------------------- */

extern void       tdcInstallSignalHandler  (void);
extern T_TDC_BOOL tdcRemoveSignalHandler   (void);

extern /*@null@*/ T_SIG_FUNCTION*  Signal  (           int                signr,
                                            /*@null@*/ T_SIG_FUNCTION*    func);

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif

/* ------------------------------------------------------------------------- */

#endif



