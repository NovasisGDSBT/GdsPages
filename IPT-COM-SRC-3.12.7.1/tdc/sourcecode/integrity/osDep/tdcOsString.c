//
// $Id: tdcOsString.c 11696 2010-09-17 10:03:34Z gweiss $
//
// DESCRIPTION    Front-End for miscelleanous functions like hamster,
//                alloc... as an OS independant interface
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
   #error "This is the INTEGRITY Version of tdcOsString, "
          "i.e. INTEGRITY has to be specified"
#endif

// ----------------------------------------------------------------------------

#include <INTEGRITY.h>
#include <string.h>

#include "tdc.h"

/* ---------------------------------------------------------------------------- */

char* tdcStrStr (      char*         hayStack,
                 const char*         needle)
{                                     
   if((hayStack == NULL) || (needle == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrStr: Wrong parameter (NULL)");
      return NULL;
   }

   return (strstr (hayStack, needle));
}

// ----------------------------------------------------------------------------

UINT32 tdcStrLen (const char*    pString)
{
   if(pString == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrLen: Wrong parameter (NULL)");
      return 0;
   }

   return ((UINT32) strlen (pString));
}

// ----------------------------------------------------------------------------

char* tdcStrCpy (char*  pDest, const char* pSrc)
{                                   /*@ -mayaliasunique -temptrans */
   if((pDest == NULL) || (pSrc == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrCpy: Wrong parameter (NULL)");
      return NULL;
   }

   return (strcpy (pDest, pSrc));   /*@ =mayaliasunique =temptrans */
}

// ----------------------------------------------------------------------------

char* tdcStrNCpy (char*  pDest, const char* pSrc, UINT32 len)     // ensure terminated string
{
   if((pDest == NULL) || (pSrc == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrNCpy: Wrong parameter (NULL)");
      return NULL;
   }

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
   UINT32   curDestLen;

   if((pDest == NULL) || (pSrc == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrNCat: Wrong parameter (NULL)");
      return NULL;
   }
   
   curDestLen = (UINT32) strlen (pDest);
   
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
   if((pStr1 == NULL) || (pStr2 == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrCmp: Wrong parameter (NULL)");
      return 0;
   }
   else if (pStr1 == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrCmp: Wrong first parameter (NULL)");
      return -1;
   }
   else if (pStr2 == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrCmp: Wrong second parameter (NULL)");
      return 1;
   }

   return (strcmp (pStr1, pStr2));
}

// ----------------------------------------------------------------------------

INT32 tdcStrNCmp (const char*  pStr1, const char*  pStr2, UINT32  len)
{
   if((pStr1 == NULL) || (pStr2 == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrNCmp: Wrong parameter (NULL)");
      return 0;
   }
   else if (pStr1 == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrNCmp: Wrong first parameter (NULL)");
      return -1;
   }
   else if (pStr2 == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrNCmp: Wrong second parameter (NULL)");
      return 1;
   }

   return (strncmp (pStr1, pStr2, (size_t) len));
}

// ----------------------------------------------------------------------------

INT32 tdcStrICmp (const char*   pStr1, const char*   pStr2)
{
   if((pStr1 == NULL) || (pStr2 == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrICmp: Wrong parameter (NULL)");
      return 0;
   }
   else if (pStr1 == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrICmp: Wrong first parameter (NULL)");
      return -1;
   }
   else if (pStr2 == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrICmp: Wrong second parameter (NULL)");
      return 1;
   }

   return (strcasecmp (pStr1, pStr2));
}

// ----------------------------------------------------------------------------

INT32 tdcStrNICmp (const char*   pStr1, const char*   pStr2,   UINT32   len)
{
   if((pStr1 == NULL) || (pStr2 == NULL))
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrNICmp: Wrong parameter (NULL)");
      return 0;
   }
   else if (pStr1 == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrNICmp: Wrong first parameter (NULL)");
      return -1;
   }
   else if (pStr2 == NULL)
   {
      DEBUG_WARN (MOD_MAIN, "tdcStrNICmp: Wrong second parameter (NULL)");
      return 1;
   }
   
   return (strncasecmp (pStr1, pStr2, (size_t) len));
}

// ----------------------------------------------------------------------------


