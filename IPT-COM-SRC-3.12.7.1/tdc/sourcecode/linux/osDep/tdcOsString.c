//
// $Id: tdcOsString.c 11696 2010-09-17 10:03:34Z gweiss $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interface
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

#include "tdc.h"

/* ---------------------------------------------------------------------------- */

char* tdcStrStr (      char*         hayStack,
                 const char*         needle)
{                                     
   return (strstr (hayStack, needle));
}

// ----------------------------------------------------------------------------

UINT32 tdcStrLen (const char*    pString)
{
   return ((UINT32) strlen (pString));
}

// ----------------------------------------------------------------------------

char* tdcStrCpy (char*  pDest, const char* pSrc)
{                                   /*@ -mayaliasunique -temptrans */
   return (strcpy (pDest, pSrc));   /*@ =mayaliasunique =temptrans */
}

// ----------------------------------------------------------------------------

char* tdcStrNCpy (char*  pDest, const char* pSrc, UINT32 len)     // ensure terminated string
{
   if (len > 0)
   {                                            /*@ -mayaliasunique */
      strncpy (pDest, pSrc, (size_t) len);      /*@ =mayaliasunique */
      pDest [len - 1] = '\0';
   }
   else
   {
      pDest[0] = '\0';
   }
                     /*@ -temptrans */
   return (pDest);   /*@ =temptrans */
}

// ----------------------------------------------------------------------------

char* tdcStrNCat (char*  pDest, const char*   pSrc, UINT32 destLen)
{
   UINT32   curDestLen = (UINT32) strlen (pDest);

   if (curDestLen < destLen)
   {
      UINT32      remainingLen = destLen - curDestLen;

      (void) tdcStrNCpy (pDest + curDestLen, pSrc, remainingLen);
   }
                                       /*@ -temptrans */
   return (pDest);                     /*@ =temptrans */
}

// ----------------------------------------------------------------------------

INT32 tdcStrCmp (const char*  pStr1, const char*  pStr2)
{
   return (strcmp (pStr1, pStr2));
}

// ----------------------------------------------------------------------------

INT32 tdcStrNCmp (const char*  pStr1, const char*  pStr2, UINT32  len)
{
   return (strncmp (pStr1, pStr2, (size_t) len));
}

// ----------------------------------------------------------------------------

INT32 tdcStrICmp (const char*   pStr1, const char*   pStr2)
{
   return (strcasecmp (pStr1, pStr2));
}

// ----------------------------------------------------------------------------

INT32 tdcStrNICmp (const char*   pStr1, const char*   pStr2,   UINT32   len)
{
   return (strncasecmp (pStr1, pStr2, (size_t) len));
}

// ----------------------------------------------------------------------------


