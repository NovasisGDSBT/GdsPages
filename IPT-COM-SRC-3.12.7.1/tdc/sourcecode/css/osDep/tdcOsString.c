/*                                                                     */
/* $Id: tdcOsString.c 11696 2010-09-17 10:03:34Z gweiss $           */
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TDC_BASETYPES_H    /* Use original VxWorks Header File definitions */
#include "tdc.h"

/* ---------------------------------------------------------------------------- */

char* tdcStrStr (      char*     pHayStack,
                 const char*     pNeedle)
{                                       
   return (strstr (pHayStack, pNeedle));
}

/* ---------------------------------------------------------------------------- */

UINT32 tdcStrLen (const char*    pString)
{
   return ((UINT32) strlen (pString));
}

/* ---------------------------------------------------------------------------- */

char* tdcStrCpy (char*  pDest, const char* pSrc)
{                                        /*@ -mayaliasunique -temptrans */
   return (strcpy (pDest, pSrc));        /*@ =mayaliasunique =temptrans*/

}
/* ---------------------------------------------------------------------------- */

char* tdcStrNCpy (char*  pDest, const char* pSrc, UINT32 len)
{
   if (len > 0)
   {  /* strncpy doesn't ensure terminating zero in VxWorks */    /*@ -mayaliasunique */
      (void) strncpy (pDest, pSrc, (size_t) len);                 /*@ =mayaliasunique */

      pDest[len - 1] = '\0';
   }
   else
   {
      pDest[0] = '\0';
   }
                     /*@ -temptrans */
   return (pDest);   /*@ =temptrans */
}

/* ---------------------------------------------------------------------------- */

char* tdcStrNCat (char*  pDest, const char*   pSrc, UINT32 destLen)
{
   UINT32   curDestLen = (UINT32) strlen (pDest);

   if (curDestLen < destLen)
   {
      UINT32      remainingLen = destLen - curDestLen;

      (void) tdcStrNCpy (pDest + curDestLen, pSrc, remainingLen);
   }
                                    /*@ -temptrans */
   return (pDest);                  /*@ =temptrans */
}

/* ---------------------------------------------------------------------------- */

INT32 tdcStrCmp (const char*  pStr1, const char*  pStr2)
{
   return (strcmp (pStr1, pStr2));
}

/* ---------------------------------------------------------------------------- */

INT32 tdcStrNCmp (const char*  pStr1, const char*  pStr2, UINT32  len)
{
   return (strncmp (pStr1, pStr2, (size_t) len));
}

/* ---------------------------------------------------------------------------- */

INT32 tdcStrICmp (const char*  pStr1, const char*  pStr2)
{
   if (pStr1 == NULL)
   {
      return (-1);
   }
   else
   {
      if (pStr2 == NULL)
      {
         return (-2);
      }
      else
      {
         for (; tolower (*pStr1) == tolower (*pStr2); pStr1++, pStr2++)
         {
            if ((*pStr1)  ==  '\0')
            {
               return (0);  /* strings are identical */
            }
         }
      }
   }

   return (1);
}

/* ---------------------------------------------------------------------------- */

INT32 tdcStrNICmp (const char*  pStr1, const char*  pStr2, UINT32  len)
{
   if (pStr1 == NULL)
   {
      return (-1);
   }
   else
   {
      if (pStr2 == NULL)
      {
         return (-2);
      }
      else
      {
         UINT32      i;

         for (i = 0; i < len; i++, pStr1++, pStr2++)
         {
            if (tolower (*pStr1) != tolower (*pStr2))
            {
               return (1);       /* strings are not identical */
            }

            if ((*pStr1)  ==  '\0')
            {
               break;       /* strings are identical */
            }
         }

         /* partial strings are identical */
      }
   }

   return (0);
}

/* ---------------------------------------------------------------------------- */

