/*  $Id: tdcString.h 11696 2010-09-17 10:03:34Z gweiss $              */
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

#if !defined (_TDC_STRING_H)
   #define _TDC_STRING_H

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ------------------------------------------------------------------------- */


extern            UINT32   tdcStrLen      (          const char*     pString);
/* pointer to hayStack must not be 'const' qualified, in order to be 
   processable by result of tdcStrStr!
*/
extern /*@null@*/ char*    tdcStrStr      (                char*     hayStack,
                                                     const char*     needle);
extern /*@null@*/ char*    tdcStrCpy      (/*@out@*/ char*           pDest, 
                                                     const char*     pSrc);
extern /*@null@*/ char*    tdcStrNCpy     (/*@out@*/ char*           pDest, 
                                                     const char*     pSrc,    
                                                     UINT32          len);
extern /*@null@*/ char*    tdcStrNCat     (/*@out@*/ char*           pDest, 
                                                     const char*     pSrc, 
                                                     UINT32          destLen);
extern            INT32    tdcStrCmp      (          const char*     pStr1, 
                                                     const char*     pStr2);
extern            INT32    tdcStrNCmp     (          const char*     pStr1, 
                                                     const char*     pStr2,   
                                                     UINT32          len);
extern            INT32    tdcStrICmp     (          const char*     pStr1, 
                                                     const char*     pStr2);
extern            INT32    tdcStrNICmp    (          const char*     pStr1, 
                                                     const char*     pStr2,   
                                                     UINT32          len);

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif

/* ------------------------------------------------------------------------- */

#endif



