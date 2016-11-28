/*                                                                     */
/* $Id: tdcOsMem.c 11184 2009-05-14 13:28:06Z mritz $              */
/*                                                                     */
/* DESCRIPTION    Front-End for miscelleanous functions like hamster,  */
/*                alloc... as an OS independant interface              */
/*                                                                     */
/* AUTHOR         Manfred Ritz                                         */
/*                                                                     */
/* REMARKS        The switch VXWORKS has to be set                     */
/*                                                                     */
/* DEPENDENCIES                                                        */
/*                                                                     */
/*                                                                     */
/* All rights reserved. Reproduction, modification, use or disclosure  */
/* to third parties without express authority is forbidden.            */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.            */
/*                                                                     */
/* ---------------------------------------------------------------------------- */

#if !defined (VXWORKS)
   #error "This is the CSS2 Version of TDC_OS_MISC, i.e. VXWORKS has to be specified"
#endif

/* ---------------------------------------------------------------------------- */

#include <vxWorks.h>
#include <stdlib.h>
#include <string.h>

#define TDC_BASETYPES_H    /* Use original VxWorks Header File definitions */
#include "tdc.h"

/* ---------------------------------------------------------------------------- */

void* tdcAllocMem (UINT32  size)
{
   return (calloc ((size_t) 1, (size_t) size));
}

/* ---------------------------------------------------------------------------- */

void* tdcAllocMemChk (const char*  pModname, UINT32   size)
{
   void*     pBuf = calloc ((size_t) 1, (size_t) size);

   if (pBuf == NULL)
   {
      DEBUG_ERROR1 (pModname, "Out of dynamic memory (alloc-size = %d bytes)", size);
   }

   return (pBuf);
}

/* ---------------------------------------------------------------------------- */

void tdcFreeMem (void*  pMem)
{
   if (pMem != NULL)
   {                       /*@ -temptrans */
      free (pMem);         /*@ =temptrans */
   }
}

/* ---------------------------------------------------------------------------- */

void* tdcMemSet (void*   pDest, int c,  UINT32  cnt)
{                                               /*@ -temptrans */
   return (memset (pDest, c, (size_t) cnt));    /*@ =temptrans */
}

/* ---------------------------------------------------------------------------- */

void tdcMemClear (void*   pDest, UINT32 cnt)
{
   (void) memset (pDest, 0, (size_t) cnt);
}

/* ---------------------------------------------------------------------------- */

void* tdcMemCpy (void*   pDest, const void* pSrc, UINT32  len)
{                                                     /*@ -mayaliasunique -temptrans */
   return (memcpy (pDest, pSrc, (size_t) len));       /*@ =mayaliasunique =temptrans */
}

/* ---------------------------------------------------------------------------- */

INT32 tdcMemCmp (const void* pBuf1, const void* pBuf2, UINT32  len)
{
   return (memcmp (pBuf1, pBuf2, (size_t) len));
}

/* ---------------------------------------------------------------------------- */


