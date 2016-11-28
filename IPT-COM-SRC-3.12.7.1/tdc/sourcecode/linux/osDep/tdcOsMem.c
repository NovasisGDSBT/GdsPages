//
// $Id: tdcOsMem.c 11184 2009-05-14 13:28:06Z mritz $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interfaceMemory Handling
//
// AUTHOR         M.Ritz         PPC/EBT
//
// REMARKS        The switch LINUX has to be set
//
// DEPENDENCIES
//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Bombardier Transportation GmbH, Germany, 2002.
//


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


#if !defined (LINUX)
   #error "This is the Linux Version of TDC_OS_MISC, i.e. LINUX has to be specified"
#endif

// ----------------------------------------------------------------------------

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>
#include <sys/time.h>


#include "tdc.h"

/* ---------------------------------------------------------------------------- */

/*@null@*/
void* tdcAllocMem (UINT32  size)
{
   return (calloc ((size_t) size, (size_t) 1));
}

// ----------------------------------------------------------------------------

/*@null@*/ void* tdcAllocMemChk (const char* pModname, UINT32    size)
{
   void*     pBuf = calloc ((size_t) size, (size_t) 1);

   if (pBuf == NULL)
   {
      DEBUG_ERROR1 (pModname, "Out of dynamic memory (alloc-size = %d bytes)", size);
   }

   return (pBuf);
}

// ----------------------------------------------------------------------------

void tdcFreeMem (/*@null@*/ void*  pMem)
{
   if (pMem != NULL)
   {                       /*@ -temptrans@*/
      free (pMem);         /*@ =temptrans@*/
   }
}

// ----------------------------------------------------------------------------

void* tdcMemSet (void*   pDest, int c, UINT32 cnt)
{                                                  /*@ -mayaliasunique -temptrans */
   return (memset (pDest, c, (size_t) cnt));       /*@ =mayaliasunique =temptrans */
}

// ----------------------------------------------------------------------------

void tdcMemClear (void*   pDest, UINT32 cnt)
{
   (void) memset (pDest, 0, (size_t) cnt);
}

// ----------------------------------------------------------------------------

void* tdcMemCpy (void*   pDest, const void* pSrc, UINT32  len)
{                                                  /*@ -mayaliasunique -temptrans */
   return (memcpy (pDest, pSrc, (size_t) len));    /*@ =mayaliasunique =temptrans */
}

// ----------------------------------------------------------------------------

INT32 tdcMemCmp (const void*  pBuf1, const void* pBuf2, UINT32  len)
{
   return (memcmp (pBuf1, pBuf2, (size_t) len));
}

// ----------------------------------------------------------------------------


