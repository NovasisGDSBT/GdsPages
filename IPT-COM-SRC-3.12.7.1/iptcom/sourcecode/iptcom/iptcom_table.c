/*******************************************************************************
*  COPYRIGHT   : (C) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT     : IPTrain
*
*  MODULE      : iptcom_table.c
*
*  ABSTRACT    : Class for storage of data in tables
*
********************************************************************************
*  HISTORY     :
*	
* $Id: iptcom_table.c 11620 2010-08-16 13:06:37Z bloehr $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
* 
*******************************************************************************/

/*******************************************************************************
*  INCLUDES */
#if defined(VXWORKS)
#include <vxWorks.h>
#endif

#include <string.h>
#include "iptcom.h"	/* Common type definitions for IPT  */
#include "vos.h"		/* OS independent system calls */
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"
#include "vos_priv.h"

/*******************************************************************************
*  DEFINES
*/

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  GLOBALS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       iptTabInit
ABSTRACT:   Initialises an IPT table. 
RETURNS:    0 if OK, !=0 if not
*/
int iptTabInit(
   IPT_TAB_HDR *pTab,   /* Pointer to table header */
   UINT16 itemSize)     /* Size of each table item */
{
   pTab->itemSize = itemSize;
   pTab->maxItems = 0;
   pTab->nItems = 0;
   pTab->pTable = (IPT_TAB_ITEM_HDR *)NULL;
   pTab->tableSize = 0;
   pTab->initialized = TRUE;

   /* Add memory to start with */
   return iptTabExpand(pTab);
}

/*******************************************************************************
NAME:       iptTab2Init
ABSTRACT:   Initialises an IPT table. 
RETURNS:    0 if OK, !=0 if not
*/
int iptTab2Init(
   IPT_TAB2_HDR *pTab,   /* Pointer to table header */
   UINT16 itemSize)     /* Size of each table item */
{
   return iptTabInit((IPT_TAB_HDR *)pTab, itemSize);
}

/*******************************************************************************
NAME:       iptUriLabelTabInit
ABSTRACT:   Initialises an IPT table. 
RETURNS:    0 if OK, !=0 if not
*/
int iptUriLabelTabInit(
   URI_LABEL_TAB_HDR *pTab,   /* Pointer to table header */
   UINT16 itemSize)     /* Size of each table item */
{
   return iptTabInit((IPT_TAB_HDR *)pTab, itemSize);
}

/*******************************************************************************
NAME:       iptUriLabelTab2Init
ABSTRACT:   Initialises an IPT table. 
RETURNS:    0 if OK, !=0 if not
*/
int iptUriLabelTab2Init(
   URI_LABEL_TAB2_HDR *pTab,   /* Pointer to table header */
   UINT16 itemSize)     /* Size of each table item */
{
   return iptTabInit((IPT_TAB_HDR *)pTab, itemSize);
}

/*******************************************************************************
NAME:       iptTabUriInit
ABSTRACT:   Initialises an IPT table. 
RETURNS:    0 if OK, !=0 if not
*/
int iptTabUriInit(
   IPT_TAB_URI_HDR *pTab,   /* Pointer to table header */
   UINT16 itemSize)     /* Size of each table item */
{
   return iptTabInit((IPT_TAB_HDR *)pTab, itemSize);
}

/*******************************************************************************
NAME:       iptTab2UriInit
ABSTRACT:   Initialises an IPT table. 
RETURNS:    0 if OK, !=0 if not
*/
int iptTab2UriInit(
   IPT_TAB2_URI_HDR *pTab,   /* Pointer to table header */
   UINT16 itemSize)     /* Size of each table item */
{
   return iptTabInit((IPT_TAB_HDR *)pTab, itemSize);
}

/*******************************************************************************
NAME:       iptTabTerminate
ABSTRACT:   Terminates the IPT table
RETURNS:    -
*/
void iptTabTerminate(
   IPT_TAB_HDR *pTab)   /* Pointer to table header */
{
   if (!pTab->initialized)
      return;
  
   pTab->initialized = FALSE;

   (void) IPTVosFree((unsigned char *) pTab->pTable);
   pTab->pTable = (IPT_TAB_ITEM_HDR *)NULL;
}

/*******************************************************************************
NAME:       iptTab2Terminate
ABSTRACT:   Terminates the IPT table
RETURNS:    -
*/
void iptTab2Terminate(
   IPT_TAB2_HDR *pTab)   /* Pointer to table header */
{
   iptTabTerminate((IPT_TAB_HDR *)pTab);
}

/*******************************************************************************
NAME:       iptUriLabelTabTerminate
ABSTRACT:   Terminates the IPT table
RETURNS:    -
*/
void iptUriLabelTabTerminate(
   URI_LABEL_TAB_HDR *pTab)   /* Pointer to table header */
{
   iptTabTerminate((IPT_TAB_HDR *)pTab);
}

/*******************************************************************************
NAME:       iptUriLabelTab2Terminate
ABSTRACT:   Terminates the IPT table
RETURNS:    -
*/
void iptUriLabelTab2Terminate(
   URI_LABEL_TAB2_HDR *pTab)   /* Pointer to table header */
{
   iptTabTerminate((IPT_TAB_HDR *)pTab);
}

/*******************************************************************************
NAME:       iptTab2UriTerminate
ABSTRACT:   Terminates the IPT table
RETURNS:    -
*/
void iptTabUriTerminate(
   IPT_TAB_URI_HDR *pTab)   /* Pointer to table header */
{
   iptTabTerminate((IPT_TAB_HDR *)pTab);
}

/*******************************************************************************
NAME:       iptTab2UriTerminate
ABSTRACT:   Terminates the IPT table
RETURNS:    -
*/
void iptTab2UriTerminate(
   IPT_TAB2_URI_HDR *pTab)   /* Pointer to table header */
{
   iptTabTerminate((IPT_TAB_HDR *)pTab);
}

/*******************************************************************************
NAME:       iptTabExpand
ABSTRACT:   Expand table to accomodate more items 
RETURNS:    0 if OK, !=0 if not
*/
int iptTabExpand(
   IPT_TAB_HDR *pTab)   /* Pointer to table header */
{
   BYTE *pNew;
   UINT32 newSize;
   UINT32 bufSize;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   newSize = (pTab->maxItems + IPT_TAB_ADDITEMS) * pTab->itemSize;
   pNew = IPTVosMallocBuf(newSize, &bufSize);
   if (pNew == NULL)
      return (int)IPT_MEM_ERROR;
   
   /* Copy all information from old table to new */
   if (pTab->pTable != NULL)
   {
      if (bufSize >= pTab->tableSize)
      {
         memcpy(pNew, pTab->pTable, pTab->tableSize);
         (void) IPTVosFree((BYTE *) pTab->pTable);    /* Free old table */
      }
      else
      {
         (void) IPTVosFree((BYTE *) pNew);
         return (int)IPT_MEM_ERROR;
      }
   }
   
   pTab->pTable = (IPT_TAB_ITEM_HDR *) pNew; /*lint !e826 Size is OK. Calculated above when we did Malloc */
   pTab->maxItems = bufSize/pTab->itemSize;
   pTab->tableSize = bufSize;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptTabAdd
ABSTRACT:   Adds new item sorted into the IPT table
RETURNS:    0 if OK, =!0 if not
*/
int iptTabAdd(
   IPT_TAB_HDR *pTab,            /* Pointer to table header */
   const IPT_TAB_ITEM_HDR *pNewItem)   /* Pointer to item to add, with key first */
{
   UINT32 key, i;
   int ret;
   IPT_TAB_ITEM_HDR *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Check if item already exists */
   key = pNewItem->key;
   pItem = iptTabFind(pTab, key);
   if (pItem != NULL)
      return (int)IPT_TAB_ERR_EXISTS;

   if (pTab->nItems >= pTab->maxItems)
   {
      /* Expand table to accomodate more items */
      if ((ret = iptTabExpand(pTab)) != (int)IPT_OK)
         return ret;
   }
   
   /* Add new item sorted */
   
   /* Find suitable place to insert to */
   pItem = pTab->pTable;
   for (i = 0; i < pTab->nItems; i++)
   {  
      if (key <= pItem->key)
         break;
      else  
      {  /* Moving pointer below is protected with for statement above */
         pItem = (IPT_TAB_ITEM_HDR *) ((char *) pItem + pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */
      }
   }

   
   /* We have found a place, move all following items to make room */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;
   memmove(p2, p1, (UINT32) (p3 - p1));
   /*lint +epn */
   
   /* Store the new item */
   memcpy(p1, pNewItem, pTab->itemSize);
   pTab->nItems++;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptTab2Add
ABSTRACT:   Adds new item sorted into the IPT table
RETURNS:    0 if OK, =!0 if not
*/
int iptTab2Add(
   IPT_TAB2_HDR *pTab,            /* Pointer to table header */
   const IPT_TAB2_ITEM_HDR *pNewItem)   /* Pointer to item to add, with key first */
{
   UINT32 key1, key2, i;
   int ret;
   IPT_TAB2_ITEM_HDR *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Check if item already exists */
   key1 = pNewItem->key1;
   key2 = pNewItem->key2;
   pItem = iptTab2Find(pTab, key1, key2);
   if (pItem != NULL)
      return (int)IPT_TAB_ERR_EXISTS;

   if (pTab->nItems >= pTab->maxItems)
   {
      /* Expand table to accomodate more items */
      if ((ret = iptTabExpand((IPT_TAB_HDR *)pTab)) != (int)IPT_OK)
         return ret;
   }
   
   /* Add new item sorted */
   
   /* Find suitable place to insert to */
   pItem = pTab->pTable;
   for (i = 0; i < pTab->nItems; i++)
   {  
      if (key1 < pItem->key1)
         break;

      if ((key1 == pItem->key1) && 
         (key2 < pItem->key2))
         break;

        /* Moving pointer below is protected with for statement above */
         pItem = (IPT_TAB2_ITEM_HDR *) ((char *) pItem + pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */
   }

   
   /* We have found a place, move all following items to make room */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;
   memmove(p2, p1, (UINT32) (p3 - p1));
   /*lint +epn */
   
   /* Store the new item */
   memcpy(p1, pNewItem, pTab->itemSize);
   pTab->nItems++;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptUriLabelTabAdd
ABSTRACT:   Adds new item sorted into the IPT table
RETURNS:    0 if OK, =!0 if not
*/
int iptUriLabelTabAdd(
   URI_LABEL_TAB_HDR *pTab,            /* Pointer to table header */
   const URI_LABEL_TAB_ITEM *pNewItem)   /* Pointer to item to add, with key first */
{
   UINT32 i;
   const char *pKey;
   int ret;
   URI_LABEL_TAB_ITEM *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Check if item already exists */
   pKey = pNewItem->labelName;
   pItem = iptUriLabelTabFind(pTab, pKey);
   if (pItem != NULL)
      return (int)IPT_TAB_ERR_EXISTS;

   if (pTab->nItems >= pTab->maxItems)
   {
      /* Expand table to accomodate more items */
      if ((ret = iptTabExpand((IPT_TAB_HDR *)pTab)) != (int)IPT_OK)
         return ret;
   }
   
   /* Add new item sorted */
   
   /* Find suitable place to insert to */
   pItem = pTab->pTable;
   for (i = 0; i < pTab->nItems; i++)
   {  
      if (iptStrcmp(pKey ,pItem->labelName) <= 0)
         break;
      else  
      {  /* Moving pointer below is protected with for statement above */
         pItem = (URI_LABEL_TAB_ITEM *) ((char *) pItem + pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */
      }
   }

   
   /* We have found a place, move all following items to make room */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;
   memmove(p2, p1, (UINT32) (p3 - p1));
   /*lint +epn */
   
   /* Store the new item */
   memcpy(p1, pNewItem, pTab->itemSize);
   pTab->nItems++;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptUriLabelTab2Add
ABSTRACT:   Adds new item sorted into the IPT table
RETURNS:    0 if OK, =!0 if not
*/
int iptUriLabelTab2Add(
   URI_LABEL_TAB2_HDR *pTab,            /* Pointer to table header */
   const URI_LABEL_TAB2_ITEM *pNewItem)   /* Pointer to item to add, with key first */
{
   UINT32 i;
   const char  *pKey1, *pKey2;
   int ret;
   URI_LABEL_TAB2_ITEM *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Check if item already exists */
   pKey1 = pNewItem->labelName1;
   pKey2 = pNewItem->labelName2;
   pItem = iptUriLabelTab2Find(pTab, pKey1, pKey2);
   if (pItem != NULL)
      return (int)IPT_TAB_ERR_EXISTS;

   if (pTab->nItems >= pTab->maxItems)
   {
      /* Expand table to accomodate more items */
      if ((ret = iptTabExpand((IPT_TAB_HDR *)pTab)) != (int)IPT_OK)
         return ret;
   }
   
   /* Add new item sorted */
   
   /* Find suitable place to insert to */
   pItem = pTab->pTable;
   for (i = 0; i < pTab->nItems; i++)
   {  
      if (iptStrcmp(pKey1 ,pItem->labelName1) < 0)
         break;

      if ((iptStrcmp(pKey1 ,pItem->labelName1) == 0) && 
          (iptStrcmp(pKey2 ,pItem->labelName2) < 0))
         break;

        /* Moving pointer below is protected with for statement above */
         pItem = (URI_LABEL_TAB2_ITEM *) ((char *) pItem + pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */
   }

   
   /* We have found a place, move all following items to make room */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;
   memmove(p2, p1, (UINT32) (p3 - p1));
   /*lint +epn */
   
   /* Store the new item */
   memcpy(p1, pNewItem, pTab->itemSize);
   pTab->nItems++;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptTabUriAdd
ABSTRACT:   Adds new item sorted into the IPT table
RETURNS:    0 if OK, =!0 if not
*/
int iptTabUriAdd(
   IPT_TAB_URI_HDR *pTab,            /* Pointer to table header */
   const IPT_TAB_URI_ITEM *pNewItem)   /* Pointer to item to add, with key first */
{
   UINT32 key1, i;
   const char *pKey2;
   int ret;
   IPT_TAB_URI_ITEM *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Check if item already exists */
   key1 = pNewItem->key1;
   pKey2 = pNewItem->labelName;
   pItem = iptTabUriFind(pTab, key1, pKey2);
   if (pItem != NULL)
      return (int)IPT_TAB_ERR_EXISTS;

   if (pTab->nItems >= pTab->maxItems)
   {
      /* Expand table to accomodate more items */
      if ((ret = iptTabExpand((IPT_TAB_HDR *)pTab)) != (int)IPT_OK)
         return ret;
   }
   
   /* Add new item sorted */
   
   /* Find suitable place to insert to */
   pItem = pTab->pTable;
   for (i = 0; i < pTab->nItems; i++)
   {  
      if (key1 < pItem->key1)
         break;


      if ((key1 == pItem->key1) && 
          (iptStrcmp(pKey2 ,pItem->labelName) < 0))
         break;

        /* Moving pointer below is protected with for statement above */
         pItem = (IPT_TAB_URI_ITEM *) ((char *) pItem + pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */
   }

   /* We have found a place, move all following items to make room */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;
   memmove(p2, p1, (UINT32) (p3 - p1));
   /*lint +epn */
   
   /* Store the new item */
   memcpy(p1, pNewItem, pTab->itemSize);
   pTab->nItems++;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptTab2UriAdd
ABSTRACT:   Adds new item sorted into the IPT table
RETURNS:    0 if OK, =!0 if not
*/
int iptTab2UriAdd(
   IPT_TAB2_URI_HDR *pTab,            /* Pointer to table header */
   const IPT_TAB2_URI_ITEM *pNewItem)   /* Pointer to item to add, with key first */
{
   UINT32 key1, key2, i;
   const char *pKey3;
   int ret;
   IPT_TAB2_URI_ITEM *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Check if item already exists */
   key1 = pNewItem->key1;
   key2 = pNewItem->key2;
   pKey3 = pNewItem->labelName;
   pItem = iptTab2UriFind(pTab, key1, key2, pKey3);
   if (pItem != NULL)
      return (int)IPT_TAB_ERR_EXISTS;

   if (pTab->nItems >= pTab->maxItems)
   {
      /* Expand table to accomodate more items */
      if ((ret = iptTabExpand((IPT_TAB_HDR *)pTab)) != (int)IPT_OK)
         return ret;
   }
   
   /* Add new item sorted */
   
   /* Find suitable place to insert to */
   pItem = pTab->pTable;
   for (i = 0; i < pTab->nItems; i++)
   {  
      if (key1 < pItem->key1)
         break;

      if ((key1 == pItem->key1) && 
          (key2 < pItem->key2))
         break;

      if ((key1 == pItem->key1) && 
          (key2 == pItem->key2) &&
          (iptStrcmp(pKey3 ,pItem->labelName) < 0))
         break;

        /* Moving pointer below is protected with for statement above */
         pItem = (IPT_TAB2_URI_ITEM *) ((char *) pItem + pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */
   }

   /* We have found a place, move all following items to make room */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;
   memmove(p2, p1, (UINT32) (p3 - p1));
   /*lint +epn */
   
   /* Store the new item */
   memcpy(p1, pNewItem, pTab->itemSize);
   pTab->nItems++;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptTabDelete
ABSTRACT:   Delete item in the look up table
RETURNS:    0 if OK, !=0 if not
*/
int iptTabDelete(
   IPT_TAB_HDR *pTab,  /* Pointer to table header */
   UINT32 key)         /* Key */
{
   IPT_TAB_ITEM_HDR *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Find item */
   pItem = iptTabFind(pTab, key);
   
   if (pItem == NULL)
      return (int)IPT_TAB_ERR_NOT_FOUND;  /* No item found */
   
   /* Remove item by compressing the table */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;/*lint !e64 Assignment is OK */
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;/*lint !e64 Assignment is OK */
   memmove(p1, p2, (UINT32) (p3 - p2));
   /*lint +epn */
   
   pTab->nItems--;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptTab2Delete
ABSTRACT:   Delete item in the look up table
RETURNS:    0 if OK, !=0 if not
*/
int iptTab2Delete(
   IPT_TAB2_HDR *pTab,  /* Pointer to table header */
   UINT32 key1,         /* Key1 */
   UINT32 key2)         /* Key2 */
{
   IPT_TAB2_ITEM_HDR *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Find item */
   pItem = iptTab2Find(pTab, key1, key2);
   
   if (pItem == NULL)
      return (int)IPT_TAB_ERR_NOT_FOUND;  /* No item found */
   
   /* Remove item by compressing the table */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;/*lint !e64 Assignment is OK */
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;/*lint !e64 Assignment is OK */
   memmove(p1, p2, (UINT32) (p3 - p2));
   /*lint +epn */
   
   pTab->nItems--;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptUriLabelTabDelete
ABSTRACT:   Delete item in the look up table
RETURNS:    0 if OK, !=0 if not
*/
int iptUriLabelTabDelete(
   URI_LABEL_TAB_HDR *pTab,  /* Pointer to table header */
   char *pKey)         /* Key */
{
   URI_LABEL_TAB_ITEM *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Find item */
   pItem = iptUriLabelTabFind(pTab, pKey);
   
   if (pItem == NULL)
      return (int)IPT_TAB_ERR_NOT_FOUND;  /* No item found */
   
   /* Remove item by compressing the table */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;/*lint !e64 Assignment is OK */
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;/*lint !e64 Assignment is OK */
   memmove(p1, p2, (UINT32) (p3 - p2));
   /*lint +epn */
   
   pTab->nItems--;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptUriLabelTab2Delete
ABSTRACT:   Delete item in the look up table
RETURNS:    0 if OK, !=0 if not
*/
int iptUriLabelTab2Delete(
   URI_LABEL_TAB2_HDR *pTab,  /* Pointer to table header */
   char  *pKey1,         /* Key1 */
   char  *pKey2)         /* Key2 */
{
   URI_LABEL_TAB2_ITEM *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Find item */
   pItem = iptUriLabelTab2Find(pTab, pKey1, pKey2);
   
   if (pItem == NULL)
      return (int)IPT_TAB_ERR_NOT_FOUND;  /* No item found */
   
   /* Remove item by compressing the table */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;/*lint !e64 Assignment is OK */
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;/*lint !e64 Assignment is OK */
   memmove(p1, p2, (UINT32) (p3 - p2));
   /*lint +epn */
   
   pTab->nItems--;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptTabUriDelete
ABSTRACT:   Delete item in the look up table
RETURNS:    0 if OK, !=0 if not
*/
int iptTabUriDelete(
   IPT_TAB_URI_HDR *pTab,  /* Pointer to table header */
   UINT32 key1,         /* Key1 */
   const char  *pKey2) /* Key2 */
{
   IPT_TAB_URI_ITEM *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Find item */
   pItem = iptTabUriFind(pTab, key1, pKey2);
   
   if (pItem == NULL)
      return (int)IPT_TAB_ERR_NOT_FOUND;  /* No item found */
   
   /* Remove item by compressing the table */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;/*lint !e64 Assignment is OK */
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;/*lint !e64 Assignment is OK */
   memmove(p1, p2, (UINT32) (p3 - p2));
   /*lint +epn */
   
   pTab->nItems--;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptTab2UriDelete
ABSTRACT:   Delete item in the look up table
RETURNS:    0 if OK, !=0 if not
*/
int iptTab2UriDelete(
   IPT_TAB2_URI_HDR *pTab,  /* Pointer to table header */
   UINT32 key1,         /* Key1 */
   UINT32 key2,         /* Key2 */
   const char  *pKey3) /* Key3 */
{
   IPT_TAB2_URI_ITEM *pItem;
   char *p1, *p2, *p3;
   
   if (!pTab->initialized)
      return (int)IPT_TAB_ERR_INIT;

   /* Find item */
   pItem = iptTab2UriFind(pTab, key1, key2, pKey3);
   
   if (pItem == NULL)
      return (int)IPT_TAB_ERR_NOT_FOUND;  /* No item found */
   
   /* Remove item by compressing the table */
   /* lint -epn Suppress pointer mismatch warnings */
   p1 = (char *) pItem;/*lint !e64 Assignment is OK */
   p2 = p1 + pTab->itemSize;
   p3 = (char *) pTab->pTable + pTab->nItems * pTab->itemSize;/*lint !e64 Assignment is OK */
   memmove(p1, p2, (UINT32) (p3 - p2));
   /*lint +epn */
   
   pTab->nItems--;
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptTabFind
ABSTRACT:   Find item in a table based on a key
RETURNS:    Pointer to item, NULL if not found
*/
IPT_TAB_ITEM_HDR *iptTabFind(
   const IPT_TAB_HDR *pTab, /* Pointer to table header */
   UINT32 key)              /* Key */
{
   IPT_TAB_ITEM_HDR *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (IPT_TAB_ITEM_HDR *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (IPT_TAB_ITEM_HDR *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (pItem->key > key)
         high = probe;
      else
         low = probe;
   }

   pItem = (IPT_TAB_ITEM_HDR *) ((char *) pTab->pTable + low * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

   if (low == -1 || pItem->key != key)
      return (IPT_TAB_ITEM_HDR *)NULL;

   return pItem;
}

/*******************************************************************************
NAME:       iptTab2Find
ABSTRACT:   Find item in a table based on a key
RETURNS:    Pointer to item, NULL if not found
*/
IPT_TAB2_ITEM_HDR *iptTab2Find(
   const IPT_TAB2_HDR *pTab, /* Pointer to table header */
   UINT32 key1,              /* Key1 */
   UINT32 key2)              /* Key2 */
{
   IPT_TAB2_ITEM_HDR *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (IPT_TAB2_ITEM_HDR *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (IPT_TAB2_ITEM_HDR *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (pItem->key1 > key1)
         high = probe;
      else if ((pItem->key1 == key1) && (pItem->key2 > key2))
         high = probe;
      else
         low = probe;
   }

   pItem = (IPT_TAB2_ITEM_HDR *) ((char *) pTab->pTable + low * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

   if (low == -1 || pItem->key1 != key1 || pItem->key2 != key2)
      return (IPT_TAB2_ITEM_HDR *)NULL;

   return pItem;
}

/*******************************************************************************
NAME:       iptUriLabelTabFind
ABSTRACT:   Find item in a table based on a key
RETURNS:    Pointer to item, NULL if not found
*/
URI_LABEL_TAB_ITEM *iptUriLabelTabFind(
   const URI_LABEL_TAB_HDR *pTab,  /* Pointer to table header */
   const char *pKey)         /* Key */
{
   URI_LABEL_TAB_ITEM *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (URI_LABEL_TAB_ITEM *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (URI_LABEL_TAB_ITEM *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (iptStrcmp(pItem->labelName, pKey) > 0)
         high = probe;
      else
         low = probe;
   }

   pItem = (URI_LABEL_TAB_ITEM *) ((char *) pTab->pTable + low * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

   if (low == -1 || (iptStrcmp(pKey ,pItem->labelName) != 0))
      return (URI_LABEL_TAB_ITEM *)NULL;

   return pItem;
}

/*******************************************************************************
NAME:       iptUriLabelTab2Find
ABSTRACT:   Find item in a table based on a key
RETURNS:    Pointer to item, NULL if not found
*/
URI_LABEL_TAB2_ITEM *iptUriLabelTab2Find(
   const URI_LABEL_TAB2_HDR *pTab,  /* Pointer to table header */
   const char  *pKey1,         /* Key1 */
   const char  *pKey2)         /* Key2 */
{
   URI_LABEL_TAB2_ITEM *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (URI_LABEL_TAB2_ITEM *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (URI_LABEL_TAB2_ITEM *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (iptStrcmp(pItem->labelName1, pKey1) > 0)
         high = probe;
      else if ((iptStrcmp(pItem->labelName1, pKey1) == 0) && (iptStrcmp(pItem->labelName2, pKey2) > 0))
         high = probe;
      else
         low = probe;
   }

   pItem = (URI_LABEL_TAB2_ITEM *) ((char *) pTab->pTable + low * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

   if (low == -1 || (iptStrcmp(pItem->labelName1, pKey1) != 0) || (iptStrcmp(pItem->labelName2, pKey2) != 0))
      return (URI_LABEL_TAB2_ITEM *)NULL;

   return pItem;
}

/*******************************************************************************
NAME:       iptTabUriFind
ABSTRACT:   Find item in a table based on two numeric keys and one string key
RETURNS:    Pointer to item, NULL if not found
*/
IPT_TAB_URI_ITEM *iptTabUriFind(
   const IPT_TAB_URI_HDR *pTab, /* Pointer to table header */
   UINT32 key1,              /* Key1 */
   const char  *pKey2)       /* Key2 */
{
   IPT_TAB_URI_ITEM *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (IPT_TAB_URI_ITEM *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (IPT_TAB_URI_ITEM *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (pItem->key1 > key1)
         high = probe;
      else if ((pItem->key1 == key1) && 
               (iptStrcmp(pItem->labelName, pKey2) > 0))
         high = probe;
      else
         low = probe;
  
   }

   pItem = (IPT_TAB_URI_ITEM *) ((char *) pTab->pTable + low * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

   if (low == -1 || pItem->key1 != key1 ||(iptStrcmp(pItem->labelName, pKey2) != 0))
      return (IPT_TAB_URI_ITEM *)NULL;

   return pItem;
}

/*******************************************************************************
NAME:       iptTab2UriFind
ABSTRACT:   Find item in a table based on two numeric keys and one string key
RETURNS:    Pointer to item, NULL if not found
*/
IPT_TAB2_URI_ITEM *iptTab2UriFind(
   const IPT_TAB2_URI_HDR *pTab, /* Pointer to table header */
   UINT32 key1,              /* Key1 */
   UINT32 key2,              /* Key2 */
   const char  *pKey3)       /* Key3 */
{
   IPT_TAB2_URI_ITEM *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (IPT_TAB2_URI_ITEM *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (IPT_TAB2_URI_ITEM *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (pItem->key1 > key1)
         high = probe;
      else if ((pItem->key1 == key1) && 
               (pItem->key2 > key2))
         high = probe;
      else if ((pItem->key1 == key1) && 
               (pItem->key2 == key2) &&
               (iptStrcmp(pItem->labelName, pKey3) > 0))
         high = probe;
      else
         low = probe;
  
   }

   pItem = (IPT_TAB2_URI_ITEM *) ((char *) pTab->pTable + low * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

   if (low == -1 || pItem->key1 != key1 || pItem->key2 != key2)
      return (IPT_TAB2_URI_ITEM *)NULL;

   return pItem;
}

/*******************************************************************************
NAME:       iptTabFindNext
ABSTRACT:   Find next item in a table greater than the key
RETURNS:    Pointer to next item, NULL if not found
*/
IPT_TAB_ITEM_HDR *iptTabFindNext(
   const IPT_TAB_HDR *pTab, /* Pointer to table header */
   UINT32 key)              /* Key */
{
   IPT_TAB_ITEM_HDR *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (IPT_TAB_ITEM_HDR *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (IPT_TAB_ITEM_HDR *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (pItem->key > key)
         high = probe;
      else
         low = probe;
   }


   if (pTab->nItems > 0)
   {
      if (low == -1 )
      {
         /* Return first item */
         pItem = (IPT_TAB_ITEM_HDR *) ((char *) pTab->pTable); /*lint !e826 Cast to calculate dynamic item size is OK */ 
      
      }
      else if (low < pTab->nItems - 1)
      {
         /* Return next item */
         pItem = (IPT_TAB_ITEM_HDR *) ((char *) pTab->pTable + (low+1) * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 
      }
      else
      {
         /* No more items in the table */
         return (IPT_TAB_ITEM_HDR *)NULL;
      }
   }
   else
   {
      /* No items in the table */
      return (IPT_TAB_ITEM_HDR *)NULL;
   }

   return pItem;
}

/*******************************************************************************
NAME:       iptTab2FindNext
ABSTRACT:   Find next item in a table greater than the key
RETURNS:    Pointer to next item, NULL if not found
*/
IPT_TAB2_ITEM_HDR *iptTab2FindNext(
   const IPT_TAB2_HDR *pTab, /* Pointer to table header */
   UINT32 key1,              /* Key1 */
   UINT32 key2)              /* Key2 */
{
   IPT_TAB2_ITEM_HDR *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (IPT_TAB2_ITEM_HDR *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (IPT_TAB2_ITEM_HDR *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (pItem->key1 > key1)
         high = probe;
      else if ((pItem->key1 == key1) && (pItem->key2 > key2))
         high = probe;
      else
         low = probe;
   }

   if (pTab->nItems > 0)
   {
      if (low == -1 )
      {
         /* Return first item */
         pItem = (IPT_TAB2_ITEM_HDR *) ((char *) pTab->pTable); /*lint !e826 Cast to calculate dynamic item size is OK */ 
      
      }
      else if (low < pTab->nItems - 1)
      {
         /* Return next item */
         pItem = (IPT_TAB2_ITEM_HDR *) ((char *) pTab->pTable + (low+1) * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 
      }
      else
      {
         /* No more items in the table */
         return (IPT_TAB2_ITEM_HDR *)NULL;
      }
   }
   else
   {
      /* No items in the table */
      return (IPT_TAB2_ITEM_HDR *)NULL;
   }

   return pItem;
}

/*******************************************************************************
NAME:       iptUriLabelTabFindNext
ABSTRACT:   Find next item in a table greater than the key
RETURNS:    Pointer to next item, NULL if not found
*/
URI_LABEL_TAB_ITEM *iptUriLabelTabFindNext(
   const URI_LABEL_TAB_HDR *pTab, /* Pointer to table header */
   const char *pKey)              /* Key */
{
   URI_LABEL_TAB_ITEM *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (URI_LABEL_TAB_ITEM *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (URI_LABEL_TAB_ITEM *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (iptStrcmp(pItem->labelName, pKey) > 0)
         high = probe;
      else
         low = probe;
   }


   if (pTab->nItems > 0)
   {
      if (low == -1 )
      {
         /* Return first item */
         pItem = (URI_LABEL_TAB_ITEM *) pTab->pTable;
      
      }
      else if (low < pTab->nItems - 1)
      {
         /* Return next item */
         pItem = (URI_LABEL_TAB_ITEM *) ((char *) pTab->pTable + (low+1) * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 
      }
      else
      {
         /* No more items in the table */
         return (URI_LABEL_TAB_ITEM *)NULL;
      }
   }
   else
   {
      /* No items in the table */
      return (URI_LABEL_TAB_ITEM *)NULL;
   }

   return pItem;
}

/*******************************************************************************
NAME:       iptUriLabelTab2FindNext
ABSTRACT:   Find next item in a table greater than the key
RETURNS:    Pointer to next item, NULL if not found
*/
URI_LABEL_TAB2_ITEM *iptUriLabelTab2FindNext(
   const URI_LABEL_TAB2_HDR *pTab, /* Pointer to table header */
   const char *pKey1,              /* Key1 */
   const char *pKey2)              /* Key2 */
{
   URI_LABEL_TAB2_ITEM *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (URI_LABEL_TAB2_ITEM *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (URI_LABEL_TAB2_ITEM *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (iptStrcmp(pItem->labelName1, pKey1) > 0)
         high = probe;
      else if ((iptStrcmp(pItem->labelName1, pKey1) == 0) && (iptStrcmp(pItem->labelName2, pKey2) > 0))
      {
         high = probe;
      }
      else
         low = probe;
   }


   if (pTab->nItems > 0)
   {
      if (low == -1 )
      {
         /* Return first item */
         pItem = (URI_LABEL_TAB2_ITEM *) pTab->pTable;
      
      }
      else if (low < pTab->nItems - 1)
      {
         /* Return next item */
         pItem = (URI_LABEL_TAB2_ITEM *) ((char *) pTab->pTable + (low+1) * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 
      }
      else
      {
         /* No more items in the table */
         return (URI_LABEL_TAB2_ITEM *)NULL;
      }
   }
   else
   {
      /* No items in the table */
      return (URI_LABEL_TAB2_ITEM *)NULL;
   }

   return pItem;
}

/*******************************************************************************
NAME:       iptTabUriFindNext
ABSTRACT:   Find next item in a table greater than the key
RETURNS:    Pointer to next item, NULL if not found
*/
IPT_TAB_URI_ITEM *iptTabUriFindNext(
   const IPT_TAB_URI_HDR *pTab, /* Pointer to table header */
   UINT32 key1,              /* Key1 */
   const char  *pKey2)       /* Key3 */
{
   IPT_TAB_URI_ITEM *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (IPT_TAB_URI_ITEM *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (IPT_TAB_URI_ITEM *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (pItem->key1 > key1)
         high = probe;
      else if ((pItem->key1 == key1) && 
               (iptStrcmp(pItem->labelName, pKey2) > 0))
         high = probe;
      else
         low = probe;
   }

   if (pTab->nItems > 0)
   {
      if (low == -1 )
      {
         /* Return first item */
         pItem = (IPT_TAB_URI_ITEM *) pTab->pTable;  
      
      }
      else if (low < pTab->nItems - 1)
      {
         /* Return next item */
         pItem = (IPT_TAB_URI_ITEM *) ((char *) pTab->pTable + (low+1) * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 
      }
      else
      {
         /* No more items in the table */
         return (IPT_TAB_URI_ITEM *)NULL;
      }
   }
   else
   {
      /* No items in the table */
      return (IPT_TAB_URI_ITEM *)NULL;
   }

   return pItem;
}
/*******************************************************************************
NAME:       iptTab2UriFindNext
ABSTRACT:   Find next item in a table greater than the key
RETURNS:    Pointer to next item, NULL if not found
*/
IPT_TAB2_URI_ITEM *iptTab2UriFindNext(
   const IPT_TAB2_URI_HDR *pTab, /* Pointer to table header */
   UINT32 key1,              /* Key1 */
   UINT32 key2,              /* Key2 */
   const char  *pKey3)       /* Key3 */
{
   IPT_TAB2_URI_ITEM *pItem;
   int high, low, probe;
   
   if (!pTab->initialized)
      return (IPT_TAB2_URI_ITEM *)NULL;

   /* Find item with binary search */
   high = pTab->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      /* Moving pointers below are protected with while statement above */
      pItem = (IPT_TAB2_URI_ITEM *) ((char *) pTab->pTable + probe * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 

      if (pItem->key1 > key1)
         high = probe;
      else if ((pItem->key1 == key1) && 
               (pItem->key2 > key2))
         high = probe;
      else if ((pItem->key1 == key1) && 
               (pItem->key2 == key2) &&
               (iptStrcmp(pItem->labelName, pKey3) > 0))
         high = probe;
      else
         low = probe;
   }

   if (pTab->nItems > 0)
   {
      if (low == -1 )
      {
         /* Return first item */
         pItem = (IPT_TAB2_URI_ITEM *) pTab->pTable;  
      
      }
      else if (low < pTab->nItems - 1)
      {
         /* Return next item */
         pItem = (IPT_TAB2_URI_ITEM *) ((char *) pTab->pTable + (low+1) * pTab->itemSize); /*lint !e826 Cast to calculate dynamic item size is OK */ 
      }
      else
      {
         /* No more items in the table */
         return (IPT_TAB2_URI_ITEM *)NULL;
      }
   }
   else
   {
      /* No items in the table */
      return (IPT_TAB2_URI_ITEM *)NULL;
   }

   return pItem;
}


