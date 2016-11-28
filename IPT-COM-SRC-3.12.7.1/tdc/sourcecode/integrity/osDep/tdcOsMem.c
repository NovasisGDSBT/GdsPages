//
// $Id: tdcOsMem.c 11619 2010-07-22 10:04:00Z bloehr $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interfaceMemory Handling
//
// AUTHOR         M.Ritz         PPC/EBT
//
// REMARKS        The switch INTEGRITY has to be set
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


#if !defined (__INTEGRITY)
   #error "This is the INTEGRITY Version of tdcOsMem, "
          "i.e. INTEGRITY has to be specified"
#endif

// ----------------------------------------------------------------------------

#include <INTEGRITY.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>


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
   if(pDest != NULL)
   {
      return (memset (pDest, c, (size_t) cnt));       /*@ =mayaliasunique =temptrans */
   }
 
   return NULL;
}

// ----------------------------------------------------------------------------

void tdcMemClear (void*   pDest, UINT32 cnt)
{
   if(pDest != NULL)
   {
      (void) memset (pDest, 0, (size_t) cnt);
   }
}

// ----------------------------------------------------------------------------

void* tdcMemCpy (void*   pDest, const void* pSrc, UINT32  len)
{                                                  /*@ -mayaliasunique -temptrans */
   if((pDest != NULL) && (pSrc != NULL))
   {
      return (memcpy (pDest, pSrc, (size_t) len));    /*@ =mayaliasunique =temptrans */
   }

   return NULL;
}

// ----------------------------------------------------------------------------

INT32 tdcMemCmp (const void*  pBuf1, const void* pBuf2, UINT32  len)
{
   if((pBuf1 != NULL) && (pBuf2 != NULL))
   {
      return (memcmp (pBuf1, pBuf2, (size_t) len));
   }

   return 0;
}

// ----------------------------------------------------------------------------

