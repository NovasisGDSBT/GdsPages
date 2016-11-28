/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2014 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     : IPTrain
 *
 *  MODULE      : pdcom_receive.c
 *
 *  ABSTRACT    : Receive frames from wire for pdCom, part of IPTCom
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 * $Id: pdcom_receive.c 31141 2014-01-22 16:03:48Z bloehr $
 *
 *  CR-7240 (Bernd Loehr, 2014-01-21)
 * 	        Simulation problem: Same comID on different simulated devices
 *          Fixed in pdRecTabAdd()
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *           Findings from TUEV-Assessment
 *
 *  CR-1356 (Bernd Loehr, 2010-10-08)
 * 			Additional function iptConfigAddDatasetExt to define dataset
 *          dependent un/marshalling. Parameters for iptUnmarshallDSF changed.
 *
 *  CR-432 (Gerhard Weiss, 2010-11-24)
 *          Removed unused variables
 *
 *  Internal (Bernd Loehr, 2010-08-16) 
 * 			Old obsolete CVS history removed
 *
 *
 ******************************************************************************/

/*******************************************************************************
*  INCLUDES */
#if defined(VXWORKS)
#include <vxWorks.h>
#endif

#include <string.h>
#include "iptcom.h"        /* Common type definitions for IPT */
#include "vos.h"           /* OS independent system calls */
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"

/*******************************************************************************
*  DEFINES
*/
#define  RECTAB_ADDITEMS   10 /* No of items to add to table if more space is 
                                 needed */

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  GLOBALS
*/

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
NAME:       pdRecTabExpand
ABSTRACT:   Expand table to accomodate more items 
RETURNS:    0 if OK, !=0 if not
*/
static int pdRecTabExpand(void)
{
   BYTE *pNew;
   UINT32 newSize;
   UINT32 bufSize;

   newSize = (IPTGLOBAL(pd.recTab.maxItems) + RECTAB_ADDITEMS) * sizeof(RECTAB_ITEM);
   pNew = IPTVosMallocBuf(newSize, &bufSize);
   if (pNew == NULL)
   {
      IPTVosPrint1(IPT_ERR,
               "Could not allocate memory size=%d\n", newSize);
      return (int)IPT_ERROR;
   }

   /* Copy all information from old table to new */
   if (IPTGLOBAL(pd.recTab.pTable) != NULL)
   {
      if (IPTGLOBAL(pd.recTab.tableSize) <= bufSize)
      {
         memcpy(pNew, IPTGLOBAL(pd.recTab.pTable), IPTGLOBAL(pd.recTab.tableSize));
         IPTVosFree((BYTE *) IPTGLOBAL(pd.recTab.pTable));    /* Free old table */
      }
      else
      {
         IPTVosPrint2(IPT_ERR,
                  "Newsize=%d < Oldsize=%d\n", newSize, IPTGLOBAL(pd.recTab.tableSize));
         (void)IPTVosFree((BYTE *) pNew); 
         return (int)IPT_ERROR;
      }
   }

   IPTGLOBAL(pd.recTab.pTable) = (RECTAB_ITEM *) pNew;  /*lint !e826 Size is OK. Calculated above when we did Malloc */
   IPTGLOBAL(pd.recTab.maxItems) = bufSize/sizeof(RECTAB_ITEM);
   IPTGLOBAL(pd.recTab.tableSize) = bufSize;

   return (int)IPT_OK;
}

#ifndef TARGET_SIMU
/*******************************************************************************
NAME:       pdRecTabFind
ABSTRACT:   Find ComId in the receive look up table
RETURNS:    Value from the receive look up table, 0 if not found
*/
static RECTAB_ITEM * pdRecTabFind(
   UINT32 comId)   /* First key, comid */
{
   int high, low, probe;
   UINT32 storedComId;
   
   /* Find item with binary search */
   high = IPTGLOBAL(pd.recTab.nItems);
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      storedComId = IPTGLOBAL(pd.recTab.pTable)[probe].comId; 

      if (storedComId > comId)
         high = probe;
      else if (storedComId == comId)
         return(&IPTGLOBAL(pd.recTab.pTable)[probe]);
      else
         low = probe;
   }

   return (RECTAB_ITEM *)NULL;
}

#else
/*******************************************************************************
NAME:       pdRecTabFind
ABSTRACT:   Find item in the receive look up table
            Table key 1 should be correct, table key 2 should be correct.
            Verify that the pCB retrieved from table is subscriber.
RETURNS:    Value from the receive look up table, 0 if not found
*/
static RECTAB_ITEM * pdRecTabFind(
   UINT32 comId,   /* First key, comid */
   UINT32 simIpAddr)   /* Second key, simIpAddr IP */
{
 
   int high, low, probe;
   UINT32 storedComId;
   
   /* Find item with binary search */
   high = IPTGLOBAL(pd.recTab.nItems);
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;
      storedComId = IPTGLOBAL(pd.recTab.pTable)[probe].comId; 

      if (storedComId > comId)
      {
         high = probe;
      }
      else if (storedComId == comId)
      {
         if (IPTGLOBAL(pd.recTab.pTable)[probe].simIpAddr > simIpAddr)
         {
            high = probe;
         }
         else if (IPTGLOBAL(pd.recTab.pTable)[probe].simIpAddr == simIpAddr)
         {
            return(&IPTGLOBAL(pd.recTab.pTable)[probe]);
         }
         else
         {
            low = probe;
         }
      }
      else
      {
         low = probe;
      }
   }

   return (RECTAB_ITEM *)NULL;
}

#endif

/*******************************************************************************
NAME:       pdFiltRecTabExpand
ABSTRACT:   Expand table to accomodate more items 
RETURNS:    0 if OK, !=0 if not
*/
static int pdFiltRecTabExpand(RECTAB_ITEM *pRecItem)
{
   BYTE *pNew;
   UINT32 newSize;
   UINT32 bufSize;

   newSize = (pRecItem->maxItems + 1) * sizeof(FILTRECTAB_ITEM);
   pNew = IPTVosMallocBuf(newSize, &bufSize);
   if (pNew == NULL)
   {
      IPTVosPrint1(IPT_ERR,
               "Could not allocate memory size=%d\n", newSize);
      return (int)IPT_ERROR;
   }

   /* Copy all information from old table to new */
   if (pRecItem->pFiltTable != NULL)
   {
      if (pRecItem->tableSize <= bufSize)
      {
         memcpy(pNew, pRecItem->pFiltTable, pRecItem->tableSize);
         (void)IPTVosFree((BYTE *) pRecItem->pFiltTable);    /* Free old table */
      }
      else
      {
         IPTVosPrint2(IPT_ERR,
                  "Newsize=%d < Oldsize=%d\n", newSize, pRecItem->tableSize);
         (void)IPTVosFree((BYTE *) pNew); 
         return (int)IPT_ERROR;
      }
   }

   pRecItem->pFiltTable = (FILTRECTAB_ITEM *) pNew;  /*lint !e826 Size is OK. Calculated above when we did Malloc */
   pRecItem->maxItems = bufSize/sizeof(FILTRECTAB_ITEM);
   pRecItem->tableSize = bufSize;

   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       pdFiltRecTabFind
ABSTRACT:   Find net control block matching source filter Ip in the receive look up table
RETURNS:    Value from the receive look up table, 0 if not found
*/
static FILTRECTAB_ITEM* pdFiltRecTabFind(
   UINT32 filtIpAddr,     /* Source filter IP address */
   RECTAB_ITEM *pRecItem) /* Pointer to table header */

{
   int high, low, probe;
   FILTRECTAB_ITEM *pFiltTable = pRecItem->pFiltTable;

   /* Find item with binary search */
   high = pRecItem->nItems;
   low = -1;

   while (high - low > 1)
   {  
      probe = (high + low) / 2;

      if (pFiltTable[probe].filtIpAddr > filtIpAddr)
         high = probe;
      else if (pFiltTable[probe].filtIpAddr == filtIpAddr)
         return(&pFiltTable[probe]);
      else
         low = probe;
   }

   return (FILTRECTAB_ITEM *)NULL;
}

/*******************************************************************************
NAME:       pdRecTabAdd
ABSTRACT:   Adds new item to the receive look up table
RETURNS:    -
*/
static int pdFiltRecTabAdd(
   RECTAB_ITEM *pRecItem,     /* Pointer to table header */
   FILTRECTAB_ITEM *pNewItem) /* Pointer to table with source fliter IP addresses */
{
   
   int ret;
   int i, found;
   UINT32 filtIpAddr;
   FILTRECTAB_ITEM *pItem;
   char *p1, *p2, *p3;
   
   filtIpAddr = pNewItem->filtIpAddr;
   
   /* Check if item already exists */
   pItem = pdFiltRecTabFind(filtIpAddr,pRecItem);
   if (pItem != NULL)
   {
      IPTVosPrint2(IPT_ERR,
               "Could not add subscrbier with filterIP=%#x for ComId=%d\n", filtIpAddr, pRecItem->comId);
      return (int)IPT_TAB_ERR_EXISTS;
   }

   if (pRecItem->nItems >= pRecItem->maxItems)
   {
      /* Expand table to accomodate more items */
      if ((ret = pdFiltRecTabExpand(pRecItem)) != (int)IPT_OK)
         return ret;
   }
   
   /* Add new item sorted */
   /* Find suitable place to insert to */
   found = -1;
   for (i = 0; i < pRecItem->nItems; i++)
   {  
      if (filtIpAddr <= pRecItem->pFiltTable[i].filtIpAddr)
      {
         found = i;
         break;
      }
   }

   /* We have found a place, move all following items to make room */
   if (found < 0)
      found = pRecItem->nItems;

   p1 = (char *) &pRecItem->pFiltTable[found];
   p2 = p1 + sizeof(FILTRECTAB_ITEM);
   p3 = (char *) pRecItem->pFiltTable + pRecItem->nItems * sizeof(FILTRECTAB_ITEM);
   memmove(p2, p1, p3 - p1);
   
   /* Store the new item */
   memcpy(p1, pNewItem, sizeof(FILTRECTAB_ITEM));
   pRecItem->nItems++;
   
   return (int)IPT_OK;
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       pdRecTabInit
ABSTRACT:   Initialises the receive look up table. 
RETURNS:    -
*/
int pdRecTabInit(void)
{
   int ret;

   IPTGLOBAL(pd.recTab.maxItems) = 0;
   IPTGLOBAL(pd.recTab.nItems) = 0;
   IPTGLOBAL(pd.recTab.pTable) = NULL;
   IPTGLOBAL(pd.recTab.tableSize) = 0;

   /* Add memory to start with */
   ret = pdRecTabExpand();
   return(ret);
}

/*******************************************************************************
NAME:       pdRecTabTerminate
ABSTRACT:   Terminates the receive look up table
RETURNS:    -
*/
void pdRecTabTerminate(void)
{
   UINT32 i;

   if (!IPTGLOBAL(pd.recTab.pTable))
      return;

   for (i=0; i<IPTGLOBAL(pd.recTab.nItems); i++)
   {
      if (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable)
      {
         IPTVosFree((BYTE *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable);
      }  
   }

   IPTVosFree((BYTE *) IPTGLOBAL(pd.recTab.pTable));
}

/*******************************************************************************
NAME:       pdRecTabAdd
ABSTRACT:   Adds new item to the receive look up table
RETURNS:    -
*/
/*lint -save -esym(429, pRecNetCB) pDst is not custotory */
int pdRecTabAdd(
   UINT32 comId,              /* First key, comid */
#ifdef TARGET_SIMU
   UINT32 simIpAddr,         /*  simIpAddr IP */
#endif
   UINT32 filtIpAddr,        /* Second key, source filter IP */
   PD_REC_NET_CB *pRecNetCB) /* Value to store for this item, <> 0 ,
                                pointer to net buffer*/
{
   UINT32 i;
   char *p1, *p2, *p3;
   int found;
   int ret = IPT_OK;
   RECTAB_ITEM *pRecItem;
   FILTRECTAB_ITEM filtItem;


   if (IPTGLOBAL(pd.recTab.pTable) == NULL)
   {
      IPTVosPrint0(IPT_ERR,
               "ERROR rec table not initiated\n");
      return (int)IPT_ERROR;     /* Not initiated yet */
   }

#ifdef TARGET_SIMU
   pRecItem = pdRecTabFind(comId, simIpAddr);
#else
   pRecItem = pdRecTabFind(comId);
#endif

   /* ComId not subrcribed? */
   if (!pRecItem)
   {
      if (IPTGLOBAL(pd.recTab.nItems) >= IPTGLOBAL(pd.recTab.maxItems))
      {
         /* Expand table to accomodate more items */
         if (pdRecTabExpand() != (int)IPT_OK)
            return (int)IPT_ERROR;
      }

      /* Add new item sorted */
      /* Find suitable place to insert to */
      for (i = 0; i < IPTGLOBAL(pd.recTab.nItems); i++)
      {

#ifdef TARGET_SIMU
         /* CR_7240: Do not leave loop before checking IP address on simulated devices */
         if (comId < IPTGLOBAL(pd.recTab.pTable)[i].comId)
         {
            break;
         }
         /* Too far for key 2 ? */
         if ((comId == IPTGLOBAL(pd.recTab.pTable)[i].comId) && 
            (simIpAddr < IPTGLOBAL(pd.recTab.pTable)[i].simIpAddr))
            break;
#else
         if (comId <= IPTGLOBAL(pd.recTab.pTable)[i].comId)
         {
            break;
         }
#endif
      }

      found = i;

      p1 = (char *) &IPTGLOBAL(pd.recTab.pTable)[found];
      p2 = p1 + sizeof(RECTAB_ITEM);
      p3 = (char *) IPTGLOBAL(pd.recTab.pTable) + 
           IPTGLOBAL(pd.recTab.nItems) * sizeof(RECTAB_ITEM);
      memmove(p2, p1, p3 - p1);

      /* Store the new item */
      IPTGLOBAL(pd.recTab.pTable)[found].comId = comId;
#ifdef TARGET_SIMU
      IPTGLOBAL(pd.recTab.pTable)[found].simIpAddr = simIpAddr;
#endif
      IPTGLOBAL(pd.recTab.pTable)[found].nItems = 0;
      IPTGLOBAL(pd.recTab.pTable)[found].maxItems = 0;
      IPTGLOBAL(pd.recTab.pTable)[found].pFiltTable = (FILTRECTAB_ITEM *)NULL;
      IPTGLOBAL(pd.recTab.pTable)[found].tableSize = 0;
      IPTGLOBAL(pd.recTab.nItems)++;
      if (filtIpAddr == 0)
      {
         IPTGLOBAL(pd.recTab.pTable)[found].pRecNetCB = pRecNetCB;
      }
      else
      {
         IPTGLOBAL(pd.recTab.pTable)[found].pRecNetCB = NULL;

         ret = pdFiltRecTabExpand(&IPTGLOBAL(pd.recTab.pTable)[found]);
         if (ret == IPT_OK)
         {
            filtItem.filtIpAddr = filtIpAddr;
            filtItem.pFiltRecNetCB = pRecNetCB;
            ret = pdFiltRecTabAdd(&IPTGLOBAL(pd.recTab.pTable)[found], &filtItem);
         }
         if (ret != IPT_OK)
         {
            pdRecTabDelete(comId,
#ifdef TARGET_SIMU
                           simIpAddr,   /* Second key, source IP */
#endif
                           filtIpAddr);   /* Second key, source IP */
             
         }
      }
   }
   /* No source filter? */
   else if (filtIpAddr == 0)
   {
      if (pRecItem->pRecNetCB == NULL)
      {
         pRecItem->pRecNetCB = pRecNetCB;
         ret = IPT_OK;  
      }
      else
      {
         IPTVosPrint1(IPT_ERR,
                  "Could not add subscrbier with for ComId=%d with no source filter\n", pRecItem->comId);
         ret = IPT_ERROR;
      }
   }
   else
   {
      if (!pRecItem->pFiltTable)
      {
         pRecItem->maxItems = 0;
         pRecItem->nItems = 0;
         pRecItem->tableSize = 0;

         ret = pdFiltRecTabExpand(pRecItem);
         if (ret != IPT_OK)
         {
            return (int)IPT_ERROR;
         }

      }
      filtItem.filtIpAddr = filtIpAddr;
      filtItem.pFiltRecNetCB = pRecNetCB;
      ret = pdFiltRecTabAdd(pRecItem, &filtItem);
   }
   return ret;  
}
/*lint -restore*/

/*******************************************************************************
NAME:       pdRecTabDelete
ABSTRACT:   Delete item in the receive look up table
RETURNS:    -
*/
void pdRecTabDelete(
   UINT32 comId,        /* First key, comid */
#ifdef TARGET_SIMU
   UINT32 simIpAddr,   /* Second key, source IP */
#endif
   UINT32 filtIpAddr)   /* Second key, source IP */
{
   RECTAB_ITEM *pRecItem;
   RECTAB_ITEM *pRecNext, *pRecEnd;
   FILTRECTAB_ITEM *pFiltRecItem;
   FILTRECTAB_ITEM *pFiltRecNext, *pFiltRecEnd;

   if (IPTGLOBAL(pd.recTab.pTable) == NULL)
   {
      return;     /* Not initiated yet */
   }
   
#ifdef TARGET_SIMU
   pRecItem = pdRecTabFind(comId, simIpAddr);
#else
   pRecItem = pdRecTabFind(comId);
#endif
   if (!pRecItem)
   {
      return;
   }
   
   if (filtIpAddr == 0)
   {
      /* Any subsribers with filters? */
      if (pRecItem->nItems > 0)
      {
         pRecItem->pRecNetCB = NULL;   
      }
      else
      {
         /* Remove item by compressing the table */
         pRecNext = &pRecItem[1];
         pRecEnd = &IPTGLOBAL(pd.recTab.pTable)[IPTGLOBAL(pd.recTab.nItems)];
         memmove((char *)pRecItem, (char *)pRecNext, (UINT32) ((char*)pRecEnd - (char*)pRecNext));
   
         IPTGLOBAL(pd.recTab.nItems)--;
      }
   }
   else
   {
      pFiltRecItem = pdFiltRecTabFind(filtIpAddr, pRecItem);
      if (!pFiltRecItem)
      {
         return;
      }
      
      /* Remove item by compressing the table */
      pFiltRecNext = &pFiltRecItem[1];
      pFiltRecEnd = &pRecItem->pFiltTable[pRecItem->nItems];
      memmove((char *)pFiltRecItem, (char *)pFiltRecNext, (UINT32) ((char*)pFiltRecEnd - (char*)pFiltRecNext));

      pRecItem->nItems--;
      if (pRecItem->nItems == 0)
      {
         IPTVosFree((BYTE *) pRecItem->pFiltTable);
         pRecItem->pFiltTable = NULL;
      
         /* No unfiltered CB left? */
         if (pRecItem->pRecNetCB == NULL)
         {
            /* Remove item by compressing the table */
            pRecNext = &pRecItem[1];
            pRecEnd = &IPTGLOBAL(pd.recTab.pTable)[IPTGLOBAL(pd.recTab.nItems)];
            memmove((char *)pRecItem, (char *)pRecNext, (UINT32) ((char*)pRecEnd - (char*)pRecNext));
   
            IPTGLOBAL(pd.recTab.nItems)--;
         }
      }
   }
}

/*******************************************************************************
*  NAME     : PDCom_receive
*  ABSTRACT : Handle PD frame received on the wire:
*             - check CRC's
*             - network unmarshal
*             - store in Netbuffer
*  RETURNS  : -
*/
void PDCom_receive(
   UINT32 sourceIPAddr, /* The IP address of the source */
#ifdef TARGET_SIMU
   UINT32 simDevIPAddr,   /* The IP address of the destination (used with simulation) */
#endif   
   BYTE *pPayLoad,      /* Pointer to received message */
   int frameLen)        /* Size of received message */
{
   int ret;
   int dataLen;
   BYTE *pData;
   BYTE temp[PD_DATASET_MAXSIZE];
   UINT32 datasetSize;
   UINT32 protocolVersion, topoCount, comId;
   UINT16 pdType, hdrLen;
   PD_HEADER *pHeader;
   PD_REC_NET_CB *pRecNetCB;    /* Pointer to netbuffer receive control block */
   PD_REC_NET_CB *pFiltRecNetCB;/* Pointer to filter netbuffer receive control
                                   block */
   RECTAB_ITEM *pdRec;
   FILTRECTAB_ITEM *pdFiltRec;
   
   IPTGLOBAL(pd.pdCnt.pdInPackets)++;
   pHeader = (PD_HEADER *) pPayLoad;         /*lint !e826 Size is OK but described in framelen */
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      return;
   }
 
   comId = FROMWIRE32(pHeader->comId);
   sourceIPAddr = FROMWIRE32(sourceIPAddr);
   
   /* Search for net CB for subscribers with and/or without source filter */
#ifndef TARGET_SIMU
   pdRec = pdRecTabFind(comId);
#else
   pdRec = pdRecTabFind(comId, FROMWIRE32(simDevIPAddr));
#endif
   if (!pdRec)
   {
      IPTGLOBAL(pd.pdCnt.pdInNoSubscriber)++;
      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDCom_receive: IPTVosPutSem(recSem) ERROR\n");
      }
      return;
   }
   pRecNetCB = pdRec->pRecNetCB;

   if (pdRec->pFiltTable )
   {
      pdFiltRec = pdFiltRecTabFind(sourceIPAddr, pdRec);
      if (pdFiltRec)
      {
         pFiltRecNetCB = pdFiltRec->pFiltRecNetCB;
      }
      else
      {
         pFiltRecNetCB = NULL;
      }
   }
   else
   {
      pFiltRecNetCB = NULL;
   }
   
   if ((pRecNetCB != NULL) || (pFiltRecNetCB != NULL))
   {
      hdrLen = FROMWIRE16(pHeader->headerLength);
   
      /* Check that protocol version is equal in most significant byte */
      protocolVersion = FROMWIRE32(pHeader->protocolVersion);
      if ((protocolVersion & 0xff000000) == (PD_PROTOCOL_VERSION & 0xff000000))
      {
         /* Check rest of header */
         topoCount = FROMWIRE32(pHeader->topoCount);
         if (topoCount != 0)
         {
#ifdef TARGET_SIMU
            if (iptCheckRecTopoCnt(topoCount, simDevIPAddr) != (int)IPT_OK)
#else
            if (iptCheckRecTopoCnt(topoCount) != (int)IPT_OK)
#endif
            {
               IPTGLOBAL(pd.pdCnt.pdInTopoErrors)++;
            
               IPTVosPrint1(IPT_WARN,
                            "Message received with wrong topo counter=%d\n",
                            topoCount);
               if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "PDCom_receive: IPTVosPutSem(recSem) ERROR\n");
               }
               return;
            }
         }
     
         pdType = FROMWIRE16(pHeader->type); 
      
         if ((hdrLen >= (sizeof(PD_HEADER) - FCS_SIZE)) &&
            (pdType == PD_TYPE))
         {
            /* Check FCS of header */
            ret = iptCheckFCS(pPayLoad, hdrLen + FCS_SIZE);
            if (ret == (int)IPT_OK)
            {

               pdType = FROMWIRE16(pHeader->type); 
         
               if ((hdrLen >= (sizeof(PD_HEADER) - FCS_SIZE)) &&
                  (pdType == PD_TYPE))
               {
                  pData = pPayLoad + hdrLen + FCS_SIZE;
                  dataLen = frameLen - hdrLen - FCS_SIZE;

                  /* Unload dataset, check and remove included FCS */
                  datasetSize = sizeof(temp);
                  ret = iptLoadReceiveDataFCS(pData, (UINT32)dataLen, temp, &datasetSize);
                  if (ret == (int)IPT_OK)
                  {
                     if (pRecNetCB != NULL)
                     {
                        datasetSize = pRecNetCB->size;
                        /* Unmarshall dataset into netbuffer */
                        ret = iptUnmarshallDSF(pRecNetCB->nLines, pRecNetCB->alignment, pRecNetCB->disableMarshalling, 
                                               pRecNetCB->pDatasetFormat,
                                               temp, pRecNetCB->pDataBuffer,
                                               &datasetSize);
                        if (ret == (int)IPT_OK)
                        {
                           /* Increase number of received PD packets */
                           pRecNetCB->pdInPackets++;
            
                           /* Set time of arrival */
                           pRecNetCB->timeRec = IPTVosGetMilliSecTimer();
                           pRecNetCB->invalid = FALSE;
         
                           /* Indicate data received at least once*/
                           pRecNetCB->updatedOnceNr = TRUE;

                           if ((pFiltRecNetCB != NULL) && 
                               (pFiltRecNetCB->size == pRecNetCB->size))
                           {
                              /* Increase number of received PD packets */
                              pFiltRecNetCB->pdInPackets++;
               
                              memcpy(pFiltRecNetCB->pDataBuffer, pRecNetCB->pDataBuffer,
                                     pFiltRecNetCB->size);
                              /* Set time of arrival */
                              pFiltRecNetCB->timeRec = pRecNetCB->timeRec;
                              pFiltRecNetCB->invalid = FALSE;
         
                              /* Indicate data received at least once*/
                              pFiltRecNetCB->updatedOnceNr = TRUE;
                           }
                        }
                     }
                     else if (pFiltRecNetCB != NULL)
                     {
                        datasetSize = pFiltRecNetCB->size;
                        /* Unmarshall dataset into netbuffer */
                        ret = iptUnmarshallDSF(pFiltRecNetCB->nLines, pFiltRecNetCB->alignment, pFiltRecNetCB->disableMarshalling,
                                               pFiltRecNetCB->pDatasetFormat,
                                               temp, pFiltRecNetCB->pDataBuffer,
                                               &datasetSize);
                        if (ret  == (int)IPT_OK)
                        {
                           /* Increase number of received PD packets */
                           pFiltRecNetCB->pdInPackets++;
               
                           /* Set time of arrival */
                           pFiltRecNetCB->timeRec = IPTVosGetMilliSecTimer();
                           pFiltRecNetCB->invalid = FALSE;
         
                           /* Indicate data received at least once*/
                           pFiltRecNetCB->updatedOnceNr = TRUE;
                        }
                     }
                  }
                  else if (ret == (int)IPT_FCS_ERROR)
                  {
                     IPTVosPrint0(IPT_WARN,
                           "Message received with wrong data frame checksum\n");
                     IPTGLOBAL(pd.pdCnt.pdInFCSErrors)++;
                  }
               
               }
               else
               {
                  IPTVosPrint0(IPT_WARN, "Message received with wrong header\n");
                  IPTGLOBAL(pd.pdCnt.pdInFCSErrors)++;
               }
            }
            else
            {
               IPTVosPrint0(IPT_WARN, "Message received with wrong header checksum\n");
               IPTGLOBAL(pd.pdCnt.pdInFCSErrors)++;
            }
         }
         else
         {
            IPTVosPrint0(IPT_WARN, "Message received with wrong header\n");
            IPTGLOBAL(pd.pdCnt.pdInFCSErrors)++;
         }
      }
      else
      {
         IPTVosPrint0(IPT_WARN, "Message received with wrong protocol version\n");
         IPTGLOBAL(pd.pdCnt.pdInProtocolErrors)++;
      }
   }
   else
   {
      IPTGLOBAL(pd.pdCnt.pdInNoSubscriber)++;
   }
 
   if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "PDCom_receive: IPTVosPutSem(recSem) ERROR\n");
   }
}

/*******************************************************************************
NAME:       PD_showRecTable
ABSTRACT:    
RETURNS:    -
*/
void PD_showRecTable(void)
{
   UINT32 i;
   UINT32 j;
   UINT32 k;
   PD_REC_NET_CB * pRecNetCB;
   SUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);
   PD_SUB_CB *pSubCB;

   MON_PRINTF("PD receive table\n");
   for (i = 0; i < IPTGLOBAL(pd.recTab.nItems); i++)
   {
      if (IPTGLOBAL(pd.recTab.pTable)[i].pRecNetCB)
      {
         MON_PRINTF(" Comid=%d No source filter Received packets=%d\n",
                IPTGLOBAL(pd.recTab.pTable)[i].comId,
                ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pRecNetCB)->pdInPackets);
      }

      for (j=0; j<IPTGLOBAL(pd.recTab.pTable)[i].nItems; j++)
      {
         MON_PRINTF(" Comid=%d Source Filter IP=%d.%d.%d.%d Received packets=%d\n",
                 IPTGLOBAL(pd.recTab.pTable)[i].comId,
                 (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr & 0xff000000) >> 24,
                 (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr & 0xff0000) >> 16,
                 (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr & 0xff00) >> 8,
                 (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr & 0xff),
                 ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].pFiltRecNetCB)->pdInPackets);
         
      }
   }
  
   MON_PRINTF("PD subscribe network table\n");
   pRecNetCB = IPTGLOBAL(pd.pFirstRecNetCB); 
   while (pRecNetCB != NULL)
   {
      if (pRecNetCB->sourceIp)
      {
         MON_PRINTF(" ComId=%d Filter IP=%d.%d.%d.%d Invalid=%d updatedOnce=%d nSubscriber=%d\n",
                    pRecNetCB->comId,
                    (pRecNetCB->sourceIp & 0xff000000) >> 24,
                    (pRecNetCB->sourceIp & 0xff0000) >> 16,
                    (pRecNetCB->sourceIp & 0xff00) >> 8,
                    (pRecNetCB->sourceIp & 0xff),
                    pRecNetCB->invalid,
                    pRecNetCB->updatedOnceNr,
                    pRecNetCB->nSubscriber);
      }
      else
      {
         MON_PRINTF(" ComId=%d No source filter Invalid=%d updatedOnce=%d nSubscriber=%d\n",
                    pRecNetCB->comId,
                    pRecNetCB->invalid,
                    pRecNetCB->updatedOnceNr,
                    pRecNetCB->nSubscriber);
      }
      
      pRecNetCB = pRecNetCB->pNext;
   }

   for (i = 0; i < pSchedGrpTab->nItems ; i++)
   {
      MON_PRINTF("Schedular group %d\n", pSchedGrpTab->pTable[i].schedGrp);
      pSubCB = pSchedGrpTab->pTable[i].pFirstCB;
      k = 1;
      while (pSubCB != NULL)
      {
         MON_PRINTF(" Subscriber %d in Schedular group %d\n", k, pSchedGrpTab->pTable[i].schedGrp);
         for (j = 0; j < pSubCB->noOfNetCB ; j++)
         {
            pRecNetCB = pSubCB->pRecNetCB[j];
            if (pRecNetCB)
            {
               if (pRecNetCB->sourceIp)
               {
                  MON_PRINTF("  ComId=%d Filter IP=%d.%d.%d.%d Invalid=%d updatedOnce=%d nSubscriber=%d\n",
                             pRecNetCB->comId,
                             (pRecNetCB->sourceIp & 0xff000000) >> 24,
                             (pRecNetCB->sourceIp & 0xff0000) >> 16,
                             (pRecNetCB->sourceIp & 0xff00) >> 8,
                             (pRecNetCB->sourceIp & 0xff),
                             pRecNetCB->invalid,
                             pRecNetCB->updatedOnceNr,
                             pRecNetCB->nSubscriber);
               }
               else
               {
                  MON_PRINTF("  ComId=%d No source filter Invalid=%d updatedOnce=%d nSubscriber=%d\n",
                             pRecNetCB->comId,
                             pRecNetCB->invalid,
                             pRecNetCB->updatedOnceNr,
                             pRecNetCB->nSubscriber);
               }
            }
         }
         k++;
         pSubCB = pSubCB->pNext;
      }
   }
   MON_PRINTF("\n");
}
