/*******************************************************************************
*  COPYRIGHT   : (C) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT     : IPTrain
*
*  MODULE      : pdcom_send.c
*
*  ABSTRACT    : 
*
********************************************************************************
*  HISTORY     :
*	
* $Id: pdcom_send.c 33375 2014-06-30 07:29:35Z gweiss $
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
#include "iptcom.h"	      /* Common type definitions for IPT */
#include "vos.h"		      /* OS independent system calls */
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"

/*******************************************************************************
*  DEFINES */

/*******************************************************************************
*  TYPEDEFS */

/*******************************************************************************
*  GLOBALS */

/*******************************************************************************
*  LOCALS */

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS */

/*******************************************************************************
*  LOCAL FUNCTIONS */

/*******************************************************************************
*  GLOBAL FUNCTIONS */

/*******************************************************************************
NAME:       PDCom_send
ABSTRACT:   The mainroutine for the PD send task
            Cyclic called to send PD frames of the current time slot.
            Remove net control blocks of unsubscribed PD telegrams from
            the send list.
            Add net control blocks of new subscribed PD telegrams to the send list.
            Order the send list in time slots to shape the send load. 
RETURNS:    -
*/
void PDCom_send(void)
{
   int ret;
   int done;
   int send;
   int sendSize;
   BYTE sendBuf[sizeof(PD_HEADER) + PD_DATASET_MAXSIZE + 4 * ((PD_DATASET_MAXSIZE + (IPT_BLOCK_LENGTH - 4)) / IPT_BLOCK_LENGTH)];
   UINT32 i, j;
   UINT32 max;
   UINT32 min;
   UINT32 minIndex;
   UINT32 oldTableSize;
   UINT32 bufSize;
   UINT32 topoCnt;
   UINT32 destIpAddr;
   UINT32 timeStamp;
   PD_HEADER *pHeader;
   PD_CYCLE_ITEM cycleItem;
   PD_CYCLE_ITEM *pCycleItem;
   PD_CYCLE_SLOT_ITEM *pSendTable;   /* Pointer to array of send items */
   PD_SEND_NET_CB *pSendNetCB;       /* Pointer to netbuffer control block to send */
   PD_SEND_NET_CB *pSendNetCBNow;    /* Pointer to netbuffer control block to send */
   PD_SEND_NET_CB *pPrevSendNetCB;   /* Pointer to netbuffer control block to send */
   PD_SEND_NET_CB *pSendItems;       /* Pointer to netbuffer control block to send */
   PD_SEND_NET_CB *pPrevSendItems;   /* Pointer to netbuffer control block to send */
#if defined(IF_WAIT_ENABLE)
   IPT_CONFIG_COM_PAR_EXT configComPar;
#endif
   
   /* Any changed publishing? */
   if (IPTGLOBAL(pd.netCBchanged))
   {
      if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_NO_WAIT) == IPT_OK)
      {
         IPTGLOBAL(pd.netCBchanged) = 0;

         pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB); 
         pPrevSendNetCB = NULL;
         while (pSendNetCB != NULL)
         {
            /* Removed publishing ? */
            if (pSendNetCB->nPublisher == 0)
            {
               /* Find the cycle time item for the cycle time of the net control block to be removed */
               pCycleItem = (PD_CYCLE_ITEM *)iptTabFind(&IPTGLOBAL(pd.sendTableHdr),
                                                        pSendNetCB->cycleMultiple);  /*lint !e826 Type cast OK */
               if (pCycleItem != NULL)
               {
                  done = 0;
                  for (i=0; i<pCycleItem->tableSize && !done; i++)
                  {
                     pSendItems = pCycleItem->pSendTable[i].pSendItems;
                     pPrevSendItems = NULL;
                     while (pSendItems)
                     {
                        if (pSendItems == pSendNetCB)
                        {
                           if (pPrevSendItems != NULL)
                           {
                              pPrevSendItems->pNextToSend = pSendNetCB->pNextToSend;
                           }
                           else
                           {
                              pCycleItem->pSendTable[i].pSendItems = pSendNetCB->pNextToSend;  
                           }

                           /* Removed from a slot with the current max number of CB in a
                              slot for this cycle time ?*/ 
                           if (pCycleItem->pSendTable[i].noOfNcb >= pCycleItem->curMaxNoOfNcb)
                           {
                              pCycleItem->pSendTable[i].noOfNcb--;
                              done = 1;
                           }
                           else
                           {
                              /* To keep the shaping move a CB, to the place of the removed CB,
                                 from a slot with with the current max number of CB.  */
                              send = 0;
                              for (j=i+1; j<pCycleItem->tableSize && !done ; j++)
                              {
                                 if (pCycleItem->pSendTable[j].noOfNcb >= pCycleItem->curMaxNoOfNcb)
                                 {
                                    pSendItems = pCycleItem->pSendTable[j].pSendItems;
                                    pCycleItem->pSendTable[j].pSendItems = pSendItems->pNextToSend;
                                    pCycleItem->pSendTable[j].noOfNcb--;
                                     
                                    pSendItems->pNextToSend = pCycleItem->pSendTable[i].pSendItems;
                                    pCycleItem->pSendTable[i].pSendItems = pSendItems;
                                  
                                    done = 1;
                                 }

                                 /* Passing send index? Set send flag to decrease and not increase the 
                                    time for next sending of the moved CB */ 
                                 if (j == pCycleItem->sendIndex)      
                                 {
                                    send = 1;
                                 }
                              }

                              /* Not moved yet. Start searching from the beginning of the table */
                              for (j=0; j<i && !done ; j++)
                              {
                                 if (pCycleItem->pSendTable[j].noOfNcb >= pCycleItem->curMaxNoOfNcb)
                                 {
                                    pSendItems = pCycleItem->pSendTable[j].pSendItems;
                                    pCycleItem->pSendTable[j].pSendItems = pSendItems->pNextToSend;
                                    pCycleItem->pSendTable[j].noOfNcb--;
                                     
                                    pSendItems->pNextToSend = pCycleItem->pSendTable[i].pSendItems;
                                    pCycleItem->pSendTable[i].pSendItems = pSendItems;
                                  
                                    done = 1;
                                 }

                                 /* Passing send index? Set send flag to decrease and not increase the 
                                    time for next sending of the moved CB */ 
                                 if (j == pCycleItem->sendIndex)      
                                 {
                                    send = 1;
                                 }
                              }

                              if (send)
                              {
                                 /* If redundant, only send if we are leader. If not function or 
                                    device redundancy is used for the comid, the leader state 
                                    will always be true */
                                 if (pCycleItem->pSendTable[i].pSendItems->leader)
                                 {
                                    pSendNetCBNow = pCycleItem->pSendTable[i].pSendItems;
                                    destIpAddr = pSendNetCBNow->destIp; 

                                 #ifdef TARGET_SIMU
                                    ret = iptGetTopoCnt(destIpAddr, pSendNetCBNow->simDeviceIp, &topoCnt);
                                 #else
                                    ret = iptGetTopoCnt(destIpAddr, &topoCnt);
                                 #endif
                                    if (ret == (int)IPT_OK)
                                    {
                                       /* Load PD header with data */
                                       pHeader = (PD_HEADER *) pSendNetCBNow->pSendBuffer;       /*lint !e826 Size is OK but described in framelen */
                                       timeStamp = IPTVosGetMicroSecTimer();
                                       pHeader->timeStamp = TOWIRE32(timeStamp);
                                       pHeader->topoCount = TOWIRE32(topoCnt);
                                     
                                       /* Add header FCS last */
                                       iptAddDataFCS(pSendNetCBNow->pSendBuffer, sizeof(PD_HEADER) - 4);     

                                       /* Send the telegram */
                              #ifdef TARGET_SIMU
                                       IPTDrivePDSocketSend(destIpAddr, pSendNetCBNow->simDeviceIp, pSendNetCBNow->pSendBuffer, pSendNetCBNow->size, pSendNetCBNow->pdSendSocket);
                              #else
                                       IPTDrivePDSocketSend(destIpAddr, pSendNetCBNow->pSendBuffer, pSendNetCBNow->size, pSendNetCBNow->pdSendSocket);
                              #endif
                                       pSendNetCBNow->pdOutPackets++;
                                    }
                                 }
                              }
                           }

                           /* Calculate the current maximum no of CB has changed */
                           max = 0;
                           for (j=0; j<pCycleItem->tableSize; j++)
                           {
                              if (pCycleItem->pSendTable[j].noOfNcb > max)
                              {
                                 max = pCycleItem->pSendTable[j].noOfNcb;
                              }
                           }
                           pCycleItem->curMaxNoOfNcb = max;

                           break; /* break while loop and also the for loop */
                        }

                        pPrevSendItems = pSendItems;
                        pSendItems = pSendItems->pNextToSend;
                     }  
                  }   
               }

               if (pPrevSendNetCB != NULL)
               {
                  pPrevSendNetCB->pNext = pSendNetCB->pNext;
               }
               else
               {
                  IPTGLOBAL(pd.pFirstSendNetCB) = pSendNetCB->pNext;  
               }

               IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
               IPTVosFree((BYTE *) pSendNetCB);

               if (pPrevSendNetCB != NULL)
               {
                  pSendNetCB = pPrevSendNetCB->pNext;
               }
               else
               {
                  pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB);  
               }
            }
            /* Added publishing ? */
            else if (pSendNetCB->status == CB_ADDED)
            {
#if defined(IF_WAIT_ENABLE)
               if (pSendNetCB->pdSendSocket == 0 )
               {
                  ret = iptConfigGetComPar(pSendNetCB->comParId, &configComPar);
                  if (ret == (int)IPT_OK)
                  {
                     pSendNetCB->pdSendSocket = configComPar.pdSendSocket;
                  }
               }
               
               if (pSendNetCB->pdSendSocket != 0)
#endif
               {
                  pSendNetCB->status = CB_READY;

                  pCycleItem = (PD_CYCLE_ITEM *)iptTabFind(&IPTGLOBAL(pd.sendTableHdr),
                                                           pSendNetCB->cycleMultiple);  /*lint !e826 Type cast OK */
                  if (pCycleItem == NULL)
                  {
                     cycleItem.cycleMultiple = pSendNetCB->cycleMultiple;
                     cycleItem.sendIndex = 0;
                     cycleItem.curMaxNoOfNcb = 1;
                     cycleItem.pSendTable = (PD_CYCLE_SLOT_ITEM *)IPTVosMallocBuf(sizeof(PD_CYCLE_SLOT_ITEM), &bufSize); /*lint  !e826 Size is OK but calculated */
                     if (!cycleItem.pSendTable)
                     {
                        IPTVosPrint1(IPT_ERR, "Publish of comid=%d failed. Not enough memory\n", pSendNetCB->comId);
                     }
                     else
                     {
                        cycleItem.tableSize = (bufSize/sizeof(PD_CYCLE_SLOT_ITEM) < cycleItem.cycleMultiple) ?
                                               bufSize/sizeof(PD_CYCLE_SLOT_ITEM) : cycleItem.cycleMultiple;
                        cycleItem.pSendTable[0].noOfNcb = 1;
                        cycleItem.pSendTable[0].pSendItems = pSendNetCB;
                        for (i=1; i<cycleItem.tableSize ; i++)
                        {
                           cycleItem.pSendTable[i].noOfNcb = 0;
                           cycleItem.pSendTable[i].pSendItems = NULL;
                        }
            
                        ret = iptTabAdd(&IPTGLOBAL(pd.sendTableHdr), (IPT_TAB_ITEM_HDR *)((void *)&cycleItem));
                        if (ret != (int)IPT_OK)
                        {
                           IPTVosPrint1(IPT_ERR, "Publish of comid=%d failed\n", pSendNetCB->comId);
                           IPTVosFree((BYTE *)cycleItem.pSendTable);
                        }
                     }
                  }
                  else
                  {
                     /* Find the slot with minimum no of net CB */
                     min = 0xFFFFFFFF;
                     minIndex = 0;
                     for (i=0; i<pCycleItem->tableSize ; i++)
                     {
                        if (pCycleItem->pSendTable[i].noOfNcb < min)
                        {
                           minIndex = i;
                           min = pCycleItem->pSendTable[i].noOfNcb;
                           /* A free slot found ?*/
                           if (min == 0)
                           {
                              pCycleItem->pSendTable[i].noOfNcb = 1;
                              pCycleItem->pSendTable[i].pSendItems = pSendNetCB;
                              if (pCycleItem->curMaxNoOfNcb == 0)
                              {
                                 pCycleItem->curMaxNoOfNcb = 1; 
                              }
                              break;
                           }
                        }
                     }
            
                     /* Not added yet? */
                     if (min != 0)
                     {
                        /* Table not maximum size? */
                        if (pCycleItem->tableSize < pCycleItem->cycleMultiple)
                        {
                           oldTableSize = pCycleItem->tableSize;

                           /* Increase the table size */
                           pSendTable = (PD_CYCLE_SLOT_ITEM *)IPTVosMallocBuf((oldTableSize + 1 )* sizeof(PD_CYCLE_SLOT_ITEM), &bufSize); /*lint  !e826 Size is OK but calculated */
               
                           if (!pSendTable)
                           {
                              IPTVosPrint1(IPT_ERR, "Publish of comid=%d failed. Not enough memory\n", pSendNetCB->comId);
                           }
                           else
                           {
                              /* Copy data from the old table */
                              for (i=0; i<oldTableSize ; i++)
                              {
                                 pSendTable[i] = pCycleItem->pSendTable[i];
                              }

                              /* Add the new net CB to the first free slot */
                              pSendTable[oldTableSize].noOfNcb = 1;
                              pSendTable[oldTableSize].pSendItems = pSendNetCB;

                              /* Free the old table */
                              IPTVosFree((BYTE *)pCycleItem->pSendTable);
                     
                              pCycleItem->pSendTable = pSendTable;
                              pCycleItem->tableSize = (bufSize/sizeof(PD_CYCLE_SLOT_ITEM) < pCycleItem->cycleMultiple) ?
                                                      bufSize/sizeof(PD_CYCLE_SLOT_ITEM) : pCycleItem->cycleMultiple;
               
                              /* Clear the rest of the table */
                              for (i=oldTableSize + 1; i<pCycleItem->tableSize ; i++)
                              {
                                 pCycleItem->pSendTable[i].noOfNcb = 0;
                                 pCycleItem->pSendTable[i].pSendItems = NULL;
                              }
                           }
                        }
                        /* The table is of maximum size */
                        else
                        {
                           /* Add the new net CB to the first minimum slot */
                           pCycleItem->pSendTable[minIndex].noOfNcb++;
                           if (pCycleItem->curMaxNoOfNcb < pCycleItem->pSendTable[minIndex].noOfNcb)
                           {
                              pCycleItem->curMaxNoOfNcb = pCycleItem->pSendTable[minIndex].noOfNcb ;
                           }
                  
                           pSendNetCB->pNextToSend = pCycleItem->pSendTable[minIndex].pSendItems;
                           pCycleItem->pSendTable[minIndex].pSendItems = pSendNetCB;
                        }    
                     }   
                  }
               }
#if defined(IF_WAIT_ENABLE)
               else
               {
                  IPTGLOBAL(pd.netCBchanged) = 1;
               }
#endif
               pPrevSendNetCB = pSendNetCB;
               pSendNetCB = pSendNetCB->pNext;
            }
            else
            {
               pPrevSendNetCB = pSendNetCB;
               pSendNetCB = pSendNetCB->pNext;
            }
         }

         if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "PDCom_send: IPTVosPutSem(sendSem) ERROR\n");
         }
      }
   }

   /* Send telegrams */
   pCycleItem = (PD_CYCLE_ITEM *)(IPTGLOBAL(pd.sendTableHdr.pTable)); /*lint  !e826 Size is OK but calculated */
   for (i = 0; i < IPTGLOBAL(pd.sendTableHdr.nItems); i++)
   {
      if (pCycleItem[i].sendIndex < pCycleItem[i].tableSize)
      {
         pSendNetCB = pCycleItem[i].pSendTable[pCycleItem[i].sendIndex].pSendItems;

         while (pSendNetCB)
         {
            /* Ever been updated or not defered start and socket created? */
            if (pSendNetCB->updatedOnceNs)
            {
               /* If redundant, only send if we are leader. If not function or 
                  device redundancy is used for the comid, the leader state 
                  will always be true */
               if (pSendNetCB->leader)
               {
                  destIpAddr = pSendNetCB->destIp; 

               #ifdef TARGET_SIMU
                  ret = iptGetTopoCnt(destIpAddr, pSendNetCB->simDeviceIp, &topoCnt);
               #else
                  ret = iptGetTopoCnt(destIpAddr, &topoCnt);
               #endif
                  if (ret == (int)IPT_OK)
                  {
                     if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
                     {
                        sendSize = pSendNetCB->size;
                        memcpy(sendBuf, pSendNetCB->pSendBuffer, sendSize);

                        if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
                        {
                           IPTVosPrint0(IPT_ERR, "PDCom_send: IPTVosPutSem(sendSem) ERROR\n");
                        }
 
                        /* Load PD header with data */
                        pHeader = (PD_HEADER *) sendBuf;       /*lint !e826 Size is OK but described in framelen */
                        timeStamp = IPTVosGetMicroSecTimer();
                        pHeader->timeStamp = TOWIRE32(timeStamp);
                        pHeader->topoCount = TOWIRE32(topoCnt);
                      
                        /* Add header FCS last */
                        iptAddDataFCS(sendBuf, sizeof(PD_HEADER) - 4);     

                        /* Send the telegram */
               #ifdef TARGET_SIMU
                        IPTDrivePDSocketSend(destIpAddr, pSendNetCB->simDeviceIp, sendBuf, sendSize, pSendNetCB->pdSendSocket);
               #else
                        IPTDrivePDSocketSend(destIpAddr, sendBuf, sendSize, pSendNetCB->pdSendSocket);
               #endif
                        pSendNetCB->pdOutPackets++;
                     }
                     else
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
                     }
                  }
               }
            }

            /* next telegram */
            pSendNetCB = pSendNetCB->pNextToSend;
         }
      }
   
      pCycleItem[i].sendIndex++;
      if (pCycleItem[i].sendIndex >= pCycleItem[i].cycleMultiple)
      {
         pCycleItem[i].sendIndex = 0;
      }
   }
}

/*******************************************************************************
NAME:       PD_showSendTable
ABSTRACT:    
RETURNS:    -
*/
void PD_showSendTable(void)
{
   UINT32 i, j;
   PD_CYCLE_ITEM *pCycleItem;
   PD_SEND_NET_CB *pSendNetCB;    /* Pointer to netbuffer control block to send */
   PUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.pubSchedGrpTab);
   PD_PUB_CB *pPubCB;
   
   MON_PRINTF("PD publish cycle time table\n");
   /* Send telegrams */
   pCycleItem = (PD_CYCLE_ITEM *)(IPTGLOBAL(pd.sendTableHdr.pTable)); /*lint  !e826 Size is OK but calculated */
   for (i = 0; i < IPTGLOBAL(pd.sendTableHdr.nItems); i++)
   {
      MON_PRINTF("CycleTime=%d\n", pCycleItem[i].cycleMultiple*IPTGLOBAL(task.pdProcCycle));
      for (j= 0; j < pCycleItem[i].tableSize; j++)
      {
         

         pSendNetCB = pCycleItem[i].pSendTable[j].pSendItems;
         if (pSendNetCB)
         {
            MON_PRINTF(" Timeslot=%d\n", j);
            
            while (pSendNetCB)
            {
               MON_PRINTF("  ComId=%d destination IP=%d.%d.%d.%d Leader=%d pdOutPackets=%d\n",
                           pSendNetCB->comId,
                           (pSendNetCB->destIp & 0xff000000) >> 24,
                           (pSendNetCB->destIp & 0xff0000) >> 16,
                           (pSendNetCB->destIp & 0xff00) >> 8,
                           (pSendNetCB->destIp & 0xff),
                           pSendNetCB->leader, 
                           pSendNetCB->pdOutPackets);
               /* next telegram */
               pSendNetCB = pSendNetCB->pNextToSend;
            }
         }
      }
   }
   
   MON_PRINTF("PD publish table\n");
   pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB); 
   while (pSendNetCB != NULL)
   {
      MON_PRINTF(" ComId=%d Leader=%d updatedOnce=%d nPublisher=%d\n",
                  pSendNetCB->comId,
                  pSendNetCB->leader,
                  pSendNetCB->updatedOnceNs,
                  pSendNetCB->nPublisher);

      pSendNetCB = pSendNetCB->pNext;
   }

   for (i = 0; i < pSchedGrpTab->nItems ; i++)
   {
      MON_PRINTF("Publisher in Schedular group %d\n", pSchedGrpTab->pTable[i].schedGrp);
      pPubCB = pSchedGrpTab->pTable[i].pFirstCB;
      while (pPubCB != NULL)
      {
         MON_PRINTF(" ComId=%d destination IP=%d.%d.%d.%d\n",
                    pPubCB->pdCB.comId,
                   (pPubCB->destIp & 0xff000000) >> 24,
                   (pPubCB->destIp & 0xff0000) >> 16,
                   (pPubCB->destIp & 0xff00) >> 8,
                   (pPubCB->destIp & 0xff));

         pPubCB = pPubCB->pNext;
      }
   }
   MON_PRINTF("\n");
}
