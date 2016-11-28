/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2012 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     : IPTrain
 *
 *  MODULE      : pdcom_util.c
 *
 *  ABSTRACT    : Utilities for pdCom, part of IPTCom
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 * $Id: pdcom_util.c 33111 2014-06-18 11:48:30Z gweiss $
 *
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *           Findings from TUEV-Assessment
 *
 *  CR-2869 (Bernd Loehr, 2012-01-16)
 *          Fixed a problem in pdPubGrpTabAdd() and pdSubGrpTabAddfor in case
 *          adding a 20th schedule group would fail
 *
 *  CR-3785 (Gerhard Weiss, 2011-09-22)
 *          Fixed memory leak in pdPubComidCB_get() for the case when a DestUri
 *          cannot be resolved.
 *
 *  CR-1356 (Bernd Loehr, 2010-10-08)
 * 			Additional function iptConfigAddDatasetExt to define dataset
 *			dependent un/marshalling. Parameters for iptMarshallDSF changed.
 *
 *  CR-695 (Gerhard Weiss, 2010-06-02)
 * 			Corrected memory leaks found during release
 * 			Also fixed missing adding to schedule groups
 * 
 *  CR-62 (Bernd Loehr, 2010-08-25)
 * 			Additional function pdSub_renew_all() to renew all MC memberships
 * 			after link down/up event
 *
 *  Internal (Bernd Loehr, 2010-08-13) 
 * 			Old obsolete CVS history removed
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
#include "netdriver.h"

/*******************************************************************************
*  DEFINES
*/

#define  GRPTAB_ADDITEMS   10 /* No of items to add to table if more space is 
                                 needed */

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  GLOBALS
*/

/*******************************************************************************
*  LOCALS
*/

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
NAME:       pdPubGrpTabExpand
ABSTRACT:   Expand publish schedule group table to accomodate more items 
RETURNS:    0 if OK, !=0 if not
*/
static int pdPubGrpTabExpand(void)
{
   PD_PUBSHED_GRP *pNew;
   UINT32 newSize;
   UINT16 i;
   MEM_BLOCK *pBlock;
   PUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.pubSchedGrpTab);
   int ret = IPT_OK;

   if (!pSchedGrpTab->initialized)
   {
      ret = (int)IPT_ERROR;
   }
   else
   {
      newSize = (pSchedGrpTab->maxItems + GRPTAB_ADDITEMS) * sizeof(PD_PUBSHED_GRP);
      pNew = (PD_PUBSHED_GRP *)IPTVosMalloc(newSize);    /*lint !e433 !e826 Size is OK but calculated */
      if (pNew == NULL)
      {
         IPTVosPrint1(IPT_ERR, "pdPubGrpTabExpand: Could not allocate memory size=%d for publishing group table\n",newSize);
         ret = (int)IPT_MEM_ERROR;
      }
      else
      {
         pBlock = (MEM_BLOCK *) ((BYTE *) pNew - sizeof(MEM_BLOCK));/*lint !e826 Moving memory block pointer */ 

         /* Copy all information from old table to new */
         for (i = 0; i < pSchedGrpTab->nItems; i++)
         {
            pNew[i] = pSchedGrpTab->pTable[i];
         }
        
         if (pSchedGrpTab->pTable != NULL)
         {
            IPTVosFree((BYTE *) pSchedGrpTab->pTable);    /* Free old table */
         }

         pSchedGrpTab->pTable = pNew;
         pSchedGrpTab->maxItems = pBlock->size/sizeof(PD_PUBSHED_GRP);
      }
   }

   return (ret);
}

/*******************************************************************************
NAME:       pdSubGrpTabExpand
ABSTRACT:   Expand subscribe schedule group table to accomodate more items 
RETURNS:    0 if OK, !=0 if not
*/
static int pdSubGrpTabExpand(void)
{
   PD_SUBSHED_GRP *pNew;
   UINT32 newSize;
   UINT16 i;
   MEM_BLOCK *pBlock;
   SUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);
   int ret = IPT_OK;

   if (!pSchedGrpTab->initialized)
   {
      ret = (int)IPT_ERROR;
   }
   else
   {
      newSize = (pSchedGrpTab->maxItems + GRPTAB_ADDITEMS) * sizeof(PD_SUBSHED_GRP);
      pNew = (PD_SUBSHED_GRP *)IPTVosMalloc(newSize);    /*lint !e433 !e826 Size is OK but calculated */
      if (pNew == NULL)
      {
         IPTVosPrint1(IPT_ERR, "Could not allocate memory size=%d for subscription group table\n",newSize);
         ret = (int)IPT_MEM_ERROR;
      }
      else
      {
         pBlock = (MEM_BLOCK *) ((BYTE *) pNew - sizeof(MEM_BLOCK)); /*lint !e826 Moving memory block pointer */ 

         /* Copy all information from old table to new */
         for (i = 0; i < pSchedGrpTab->nItems; i++)
         {
            pNew[i] = pSchedGrpTab->pTable[i];
         }
        
         if (pSchedGrpTab->pTable != NULL)
         {
            IPTVosFree((BYTE *) pSchedGrpTab->pTable);    /* Free old table */
         }

         pSchedGrpTab->pTable = pNew;
         pSchedGrpTab->maxItems = pBlock->size/sizeof(PD_SUBSHED_GRP);
      }
   }

   return (ret);
}

/*******************************************************************************
NAME:       pdPubGrpTabAdd
ABSTRACT:   Adds new item to the publish schedular group look up table
RETURNS:    1 = success  0 = failed
*/
static int pdPubGrpTabAdd(
   UINT32 schedGrp,   /* Key, schedule group */
   int    *pIndex)    /* Pointer to index */
{
   int i;
   int found = 0;
   int ret = 1;
   PUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.pubSchedGrpTab);

   if (pSchedGrpTab->pTable == NULL)
   {
      IPTVosPrint0(IPT_ERR, "Publishing group table not initiated\n");
      ret = 0;     /* Not initiated yet */
   }
   else
   {
      if (pSchedGrpTab->nItems >= pSchedGrpTab->maxItems)
      {
         /* Expand table to accomodate more items */
         if (pdPubGrpTabExpand() != (int)IPT_OK)
         {
            ret = 0;
         }
      }
      /* CR-2869: Proceed after successfully expanding the table */
      if (ret == 1)
      {
         /* Find suitable place to insert to */
         found = -1;
         for (i = 0; i < pSchedGrpTab->nItems; i++)
         {
            if (schedGrp <= pSchedGrpTab->pTable[i].schedGrp)
            {
               found = i;
               break;
            }
         }

         /* We have found a place, move all following items to make room */
         if (found < 0)
         {
            found = pSchedGrpTab->nItems;
         }
         else
         {
            for (i = pSchedGrpTab->nItems; i > found; i--)
            {
               pSchedGrpTab->pTable[i] = pSchedGrpTab->pTable[i-1];
            }
         }
   
         /* Store the new item */
         pSchedGrpTab->pTable[found].schedGrp = schedGrp;
         pSchedGrpTab->pTable[found].pFirstCB = NULL;

         pSchedGrpTab->nItems++;
         *pIndex = found;
      }
   }

   return(ret);
}

/*******************************************************************************
NAME:       pdSubGrpTabAdd
ABSTRACT:   Adds new item to the subscribe schedular group look up table
RETURNS:    1 = success  0 = failed
*/
static int pdSubGrpTabAdd(
   UINT32 schedGrp,   /* Key, schedule group */
   int    *pIndex)    /* Pointer to index */
{
   int i;
   int found = 0;
   int ret = 1;
   SUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);

   if (pSchedGrpTab->pTable == NULL)
   {
      IPTVosPrint0(IPT_ERR, "Subscription group table not initiated\n");
      ret = 0;  /* Not initiated yet */
   }
   else
   {
      if (pSchedGrpTab->nItems >= pSchedGrpTab->maxItems)
      {
         /* Expand table to accomodate more items */
         if (pdSubGrpTabExpand() != (int)IPT_OK)
         {
             ret = 0;
         }
      }
      /* CR-2869: Proceed after successfully expanding the table */
      if (ret == 1)
      {
         /* Find suitable place to insert to */
         found = -1;
         for (i = 0; i < pSchedGrpTab->nItems; i++)
         {
            if (schedGrp <= pSchedGrpTab->pTable[i].schedGrp)
            {
               found = i;
               break;
            }
         }

         /* We have found a place, move all following items to make room */
         if (found < 0)
         {
            found = pSchedGrpTab->nItems;
         }
         else
         {
            for (i = pSchedGrpTab->nItems; i > found; i--)
            {
               pSchedGrpTab->pTable[i] = pSchedGrpTab->pTable[i-1];
            }
         }
   
         /* Store the new item */
         pSchedGrpTab->pTable[found].schedGrp = schedGrp;
         pSchedGrpTab->pTable[found].pFirstCB = NULL;

         pSchedGrpTab->nItems++;
         *pIndex = found;
      }
   }
   return(ret);
}

/*******************************************************************************
NAME:       pdPubGrpTabDelete
ABSTRACT:   Delete item in the publish schedular group look up table
RETURNS:    -
*/
static void pdPubGrpTabDelete(
   PUBGRPTAB *pSchedGrpTab,
   UINT32 schedGrp)   /* Key, schedule group */
{
   UINT32 i;

   /* Initiated? */
   if (pSchedGrpTab->pTable != NULL)
   {
      /* Find item */
      for (i = 0; i < pSchedGrpTab->nItems; i++)
      {
         if (schedGrp == pSchedGrpTab->pTable[i].schedGrp)
         {
            break;      /* Item found */
         }
      }

      /* Item found ?*/
      if (i < pSchedGrpTab->nItems)
      {
         pSchedGrpTab->nItems--;

         for (; i < pSchedGrpTab->nItems; i++)
         {
            pSchedGrpTab->pTable[i] = pSchedGrpTab->pTable[i+1];
         }
      }
   }
}

/*******************************************************************************
NAME:       pdGrpTabDelete
ABSTRACT:   Delete item in the subscribe schedular group look up table
RETURNS:    -
*/
static void pdSubGrpTabDelete(
   SUBGRPTAB *pSchedGrpTab,
   UINT32 schedGrp)   /* Key, schedule group */
{
   UINT32 i;

   /* Initiated? */
   if (pSchedGrpTab->pTable != NULL)
   {
      /* Find item */
      for (i = 0; i < pSchedGrpTab->nItems; i++)
      {
         if (schedGrp == pSchedGrpTab->pTable[i].schedGrp)
         {
            break;      /* Item found */
         }
      }

      /* Item found ?*/
      if (i < pSchedGrpTab->nItems)
      {
         pSchedGrpTab->nItems--;

         for (; i < pSchedGrpTab->nItems; i++)
         {
            pSchedGrpTab->pTable[i] = pSchedGrpTab->pTable[i+1];
         }
      }
   }
}

/*******************************************************************************
NAME     : pdSendNetCB_find
ABSTRACT : Find a comid send net control block 
RETURNS  : Pointer to the comid net control block if found, NULL if not found
*/
static PD_SEND_NET_CB *pdSendNetCB_find(
   UINT32 comId,                /* comId */
   UINT32 destIp           /* destination IP */
#ifdef TARGET_SIMU
   ,UINT32 simDeviceIp            /* source IP address (simulation), used if != 0 */
#endif
   )               /* destination IP */
{
   PD_SEND_NET_CB *pFound = NULL;
   PD_SEND_NET_CB *pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB);
   while (pSendNetCB != NULL)
   {
      /* Check if comid and source are equal */
#ifdef TARGET_SIMU
      if (pSendNetCB->comId == comId && (pSendNetCB->simDeviceIp == simDeviceIp || simDeviceIp == 0))
#else      
      if (pSendNetCB->comId == comId)
#endif
      {
         if (destIp == pSendNetCB->destIp)
         {
            pFound = pSendNetCB;
            break;
         }
      }
      
      pSendNetCB = pSendNetCB->pNext;
   }
   
   return pFound;
}
 
/*******************************************************************************
NAME     : pdSendNetCB_create
ABSTRACT : Create new send netbuffer control block 
RETURNS  : Pointer to created control block, NULL if error
*/
static PD_SEND_NET_CB *pdSendNetCB_create(
   UINT32 comId,     /* comId */
   UINT32 destIp,    /* destination IP */
   IPT_CONFIG_EXCHG_PAR_EXT *pExchgPar,  /* Pointer to exchange parameters */
   PD_PUB_CB *pPubCB)   /* Pointer to publish control block */
{
   int res = IPT_OK;
   UINT32 sendBufSize;
   PD_SEND_NET_CB *pSendNetCB;
   PD_RED_ID_ITEM redIdItem;
   PD_RED_ID_ITEM *pRedIdItem;
   PD_HEADER *pHeader;

   /* Create control block */
   pSendNetCB = (PD_SEND_NET_CB *) IPTVosMalloc(sizeof(PD_SEND_NET_CB));
   if (pSendNetCB == NULL)
   {
      /* Could not create memory for the CB */
      IPTVosPrint1(IPT_ERR, "Could not allocate memory size=%d for send control block\n",
                   sizeof(PD_SEND_NET_CB));
   }
   else
   {
      /* Initialize */
      pSendNetCB->pNext = NULL;
      pSendNetCB->pNextToSend = NULL;
      pSendNetCB->comId = comId;
      pSendNetCB->nPublisher = 0;
      pSendNetCB->updatedOnceNs = FALSE;
      pSendNetCB->status = CB_ADDED;
      pSendNetCB->pdSendSocket = 0;
      pSendNetCB->pdOutPackets = 0;
      pSendNetCB->size = 0;

      /* Dataset contains size of complete data in dataset */
      sendBufSize = sizeof(PD_HEADER) + pPubCB->netDatabufferSize;

      /* Create data buffer area */
      pSendNetCB->pSendBuffer = IPTVosMalloc(sendBufSize);
      if (pSendNetCB->pSendBuffer == NULL)
      {
         IPTVosPrint2(IPT_ERR,
                      "Publish comId = %d failed. Could not allocate memory size=%d\n",
                      comId, sendBufSize);
         IPTVosFree((BYTE *) pSendNetCB);
         pSendNetCB =  (PD_SEND_NET_CB *)NULL;;   /* Could not create memory for the data buffer */
      }
      else
      {
         memset(pSendNetCB->pSendBuffer, 0, sendBufSize);

         pSendNetCB->cycleMultiple = pExchgPar->pdSendPar.cycleTime/IPTGLOBAL(task.pdProcCycle);
         if (pSendNetCB->cycleMultiple == 0)
         {
            pSendNetCB->cycleMultiple = 1;
            IPTVosPrint3(IPT_WARN,
                    "The cycle time=%d for comId=%d is less than the PD send thread cycle time=%d\n", 
                         pExchgPar->pdSendPar.cycleTime, comId, IPTGLOBAL(task.pdProcCycle));
         }

         pSendNetCB->destIp = destIp;
         pSendNetCB->redFuncId = pExchgPar->pdSendPar.redundant;
         pHeader = (PD_HEADER *) pSendNetCB->pSendBuffer;       /*lint !e826 Size is OK but described in framelen */

         /* fill the header with fixed values */
         pHeader->protocolVersion = TOWIRE32(PD_PROTOCOL_VERSION);
         pHeader->comId = TOWIRE32(pSendNetCB->comId);
         pHeader->type = TOWIRE16(PD_TYPE); 
         pHeader->reserved = 0;
         pHeader->headerLength = TOWIRE16(sizeof(PD_HEADER) - FCS_SIZE);

         /* Redundance is used for this comid? */
         if (pExchgPar->pdSendPar.redundant != 0)
         {
            /* Search for the redundant iD */
            pRedIdItem = (PD_RED_ID_ITEM *)(void*)iptTabFind(&IPTGLOBAL(pd.redIdTableHdr),
                                                             pSendNetCB->redFuncId);/*lint !e826  Ignore casting warning */
            if (pRedIdItem == NULL)
            {
               redIdItem.redId = pSendNetCB->redFuncId;
               
               /* Use the device redundant mode */   
               redIdItem.leader = IPTGLOBAL(pd.leaderDev);
               pSendNetCB->leader = IPTGLOBAL(pd.leaderDev);

               /* Add the redundancy function reference to the queue listeners table */
               res = iptTabAdd(&IPTGLOBAL(pd.redIdTableHdr), (IPT_TAB_ITEM_HDR *)((void *)&redIdItem));
               if (res != IPT_OK)
               {
                  IPTVosPrint3(IPT_ERR,
                     "Publish comId = %d failed. Failed to add redId=%d to table. Error=%#x\n",
                     comId, redIdItem.redId, res);
                  
                  IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
                  IPTVosFree((BYTE *) pSendNetCB);
                  pSendNetCB =  (PD_SEND_NET_CB *)NULL;
               }
            }
            else
            {
               /* Use the mode set for the redundant Id */
               pSendNetCB->leader = pRedIdItem->leader;   
            }
         }
         else
         {
            /* Redundance is not used for this comid.
               Set the initiate state as leader, i.e. send data */
            pSendNetCB->leader = TRUE;
         }
      }
   }

   return pSendNetCB;
}

/*******************************************************************************
NAME:     joinPDmulticastAddress
ABSTRACT: Join a not already join multicast address. 
          Listener semaphore has to be taken before the call
RETURNS:  - 
*/
static void joinPDmulticastAddress(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr, /* Simualted IP address */
#endif
   UINT32 ipAddress)  /* IP address */
{
   NET_JOINED_MC *pTabItem;
   NET_JOINED_MC tableItem;
   int res;
#ifdef LINUX_MULTIPROC
   NET_CTRL_QUEUE_MSG ctrlMsg;
#endif
  
   /* Search for the address in the joined address table */
#ifdef TARGET_SIMU
   pTabItem = (NET_JOINED_MC *)((void *)iptTab2Find(&IPTGLOBAL(net.pdJoinedMcAddrTable),
                                                    ipAddress, simuIpAddr));
#else
   pTabItem = (NET_JOINED_MC *)((void *)iptTabFind(&IPTGLOBAL(net.pdJoinedMcAddrTable),
                                                    ipAddress));/*lint !e826  Ignore casting warning */
#endif
   /* Addressed already joined? */
   if (pTabItem)
   {
      pTabItem->noOfJoiners++;
   }
   else
   {
#ifdef LINUX_MULTIPROC
      ctrlMsg.ctrl = JOIN_PD_MULTICAST;
      ctrlMsg.multiCastAddr = (UINT32)ipAddress;
#ifdef TARGET_SIMU
      ctrlMsg.simuIpAddr = simuIpAddr;
#endif
      /* send  queue message to net ctrl task */
      res = IPTVosSendMsgQueue(&IPTGLOBAL(net.netCtrlQueueId),
                               (char *)&ctrlMsg, sizeof(NET_CTRL_QUEUE_MSG));
      if (res != (int)IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "ERROR sending queue join multicast address message\n");
      }
#else
      /* join multicast address */
#ifdef TARGET_SIMU
      res = IPTDriveJoinPDMultiCast(ipAddress, simuIpAddr);
#else
      res = IPTDriveJoinPDMultiCast(ipAddress);
#endif
      if (res != (int)IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "Failed to join multicast address\n");
      }
#endif
      else
      {
         tableItem.multiCastAddr = ipAddress;
         tableItem.noOfJoiners = 1;
#ifdef TARGET_SIMU
         tableItem.simuIpAddr = simuIpAddr;
         res = iptTab2Add(&IPTGLOBAL(net.pdJoinedMcAddrTable),
                         (IPT_TAB2_ITEM_HDR *)((void *)&tableItem));
#else
         res = iptTabAdd(&IPTGLOBAL(net.pdJoinedMcAddrTable),
                         (IPT_TAB_ITEM_HDR *)((void *)&tableItem));
#endif
         if (res != (int)IPT_OK)
         {
            IPTVosPrint0(IPT_ERR,
                         "joinPDmulticastAddress failed to add items to table\n");
         }
      }
   }
}

/*******************************************************************************
NAME:     leavePDmulticastAddress
ABSTRACT: Join a not already join multicast address. 
          Listener semaphore has to be taken before the call
RETURNS:  - 
*/
static void leavePDmulticastAddress(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr, /* Simualted IP address */
#endif
   UINT32 ipAddress)  /* IP address */
{
   NET_JOINED_MC *pTabItem;
   int res;
#ifdef LINUX_MULTIPROC
   NET_CTRL_QUEUE_MSG ctrlMsg;
#endif
   
   if (isMulticastIpAddr(ipAddress))
   {
      /* Search for the address in the joined address table */
#ifdef TARGET_SIMU
      pTabItem = (NET_JOINED_MC *)((void *)iptTab2Find(&IPTGLOBAL(net.pdJoinedMcAddrTable),
                                                       ipAddress, simuIpAddr));
#else
      pTabItem = (NET_JOINED_MC *)((void *)iptTabFind(&IPTGLOBAL(net.pdJoinedMcAddrTable),
                                                       ipAddress));/*lint !e826  Ignore casting warning */
#endif
      if (pTabItem)
      {
         pTabItem->noOfJoiners--;
         if (pTabItem->noOfJoiners == 0)
         {
#ifdef LINUX_MULTIPROC
            ctrlMsg.ctrl = LEAVE_PD_MULTICAST;
            ctrlMsg.multiCastAddr = (long int)ipAddress;
            /* send  queue message to net ctrl task */
            res = IPTVosSendMsgQueue(&IPTGLOBAL(net.netCtrlQueueId),
                                     (char *)&ctrlMsg, sizeof(NET_CTRL_QUEUE_MSG));
            if (res != (int)IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "ERROR sending queue leave multicast address message\n");
            }
#else
            /* Leave multicast group */
#ifdef TARGET_SIMU
            res = IPTDriveLeavePDMultiCast(ipAddress, simuIpAddr);
#else
            res = IPTDriveLeavePDMultiCast(ipAddress);
#endif
            if (res != (int)IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "Failed to leave multicast address\n");
            }
#endif
            else
            {
#ifdef TARGET_SIMU
               res = iptTab2Delete(&IPTGLOBAL(net.pdJoinedMcAddrTable), ipAddress, simuIpAddr);
#else
               res = iptTabDelete(&IPTGLOBAL(net.pdJoinedMcAddrTable), ipAddress);
#endif
               if (res != (int)IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR,
                               "leavePDmulticastAddress failed to remove items from table\n");
               } 
            }
         }
      }
   }
}

/*******************************************************************************
NAME     : pdPubCB_add
ABSTRACT : Finish creation of publish control block
RETURNS  : 0 if ok else != 0
*/
/*lint -save -esym(429, pPubCB) pDst is not custotory */
static int pdPubCB_add(
   PD_PUB_CB *pPubCB)   /* Pointer to publish control block */
{
   UINT8 dummy;
   int schedIx;
   int ret = (int)IPT_OK;
   PD_SEND_NET_CB *pSendNetCB;
   T_TDC_RESULT res = TDC_OK;
   UINT32 destIp;
#ifdef TARGET_SIMU
   UINT32 simDeviceIp = 0;
#endif
   const char *pDestUri;
   IPT_CONFIG_EXCHG_PAR_EXT exchgPar;
   IPT_CONFIG_COM_PAR_EXT configComPar;
   IPT_CFG_DATASET_INT *pDataset;
   PUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.pubSchedGrpTab);
   BYTE temp[PD_DATASET_MAXSIZE];
   UINT32 datasetSize;
   UINT32 datasetSizeFCS;

   
   /* Get parameters for this comid from the configuration database */
   ret = iptConfigGetExchgPar(pPubCB->pdCB.comId, &exchgPar);
   if (ret != (int)IPT_OK)
   {
      if (ret != (int)IPT_TDC_NOT_READY)
      {
         IPTVosPrint1(IPT_ERR,
          "Publishing failed for comId=%d. ComId not configured\n", 
                      pPubCB->pdCB.comId);
      }
   }
   else if (exchgPar.pdSendPar.cycleTime == 0)
   {
      IPTVosPrint1(IPT_ERR,
              "Publish comId = %d failed. Zero cycle time configured\n", 
                   pPubCB->pdCB.comId);
      ret = IPT_ERROR;
   }
   else
   {
      /* Data buffer not created? */
      if (pPubCB->pdCB.pDataBuffer == NULL)
      {
         /* Get parameters for this comid from the configuration database */
         if ((pDataset = iptConfigGetDataset(exchgPar.datasetId)) == NULL)
         {
            IPTVosPrint2(IPT_ERR,
               "Publishing failed for comId=%d. Dataset ID=%d not configured\n",
                pPubCB->pdCB.comId, exchgPar.datasetId);
            ret = (int)IPT_NOT_FOUND;   /* Could not find dataset format */
         }
         else if (pDataset->size > PD_DATASET_MAXSIZE)
         {
            IPTVosPrint4(IPT_ERR,
                    "Publishing failed for comId=%d. Dataset size=%d greater than PD max=%d for dataset ID=%d\n", 
                         pPubCB->pdCB.comId, pDataset->size, PD_DATASET_MAXSIZE, exchgPar.datasetId);
            ret = (int)IPT_ILLEGAL_SIZE;
         }
         else if (pDataset->size == 0)
         {
            IPTVosPrint2(IPT_ERR,
                    "Publishing failed for comId=%d. Dynamic dataset size for dataset ID=%d\n",
                     pPubCB->pdCB.comId, exchgPar.datasetId);
            ret = (int)IPT_ILLEGAL_SIZE;
         }
         else
         {
            /* Dataset contains size of complete data in dataset */
            pPubCB->pdCB.size = (UINT16) pDataset->size;
            pPubCB->netDatabufferSize = iptCalcSendBufferSize(pDataset->size);      
            pPubCB->alignment = pDataset->alignment;
            pPubCB->nLines = pDataset->nLines;
            pPubCB->disableMarshalling = pDataset->disableMarshalling;
            pPubCB->pDatasetFormat = pDataset->pFormat;

            /* Create data buffer area */
            pPubCB->pdCB.pDataBuffer = IPTVosMalloc(pPubCB->pdCB.size);
            if (pPubCB->pdCB.pDataBuffer == NULL)
            {
               IPTVosPrint2(IPT_ERR, "Publishing failed for comId=%d. Could not allocate memory size=%d\n",
                            pPubCB->pdCB.comId, pPubCB->pdCB.size);
               return (int)IPT_MEM_ERROR;   /* Could not create memory for the data buffer */
            }
            memset(pPubCB->pdCB.pDataBuffer, 0, pPubCB->pdCB.size);
         }
      }
   
      if (ret == IPT_OK)
      {
         /* Get destination URI. Use parameter if specified, otherwise get from 
            config DB */
         if (pPubCB->destId != 0)
         {
            ret = iptConfigGetDestIdPar(pPubCB->pdCB.comId, pPubCB->destId, &pDestUri);
            if ((ret != (int)IPT_OK) && (ret != (int)IPT_TDC_NOT_READY))
            {
               IPTVosPrint2(IPT_ERR,
                    "Publishing failed for comId=%d. Destination ID=%d not configured\n",
                            pPubCB->pdCB.comId, pPubCB->destId);
            }
         }
         else if (pPubCB->pDestUri != NULL)
         {
            pDestUri = (char *)pPubCB->pDestUri;
         }
         else
         {
            pDestUri =  exchgPar.pdSendPar.pDestinationURI;
            if (pDestUri == NULL)
            {
               IPTVosPrint1(IPT_ERR,
                    "Publishing failed for comId=%d. Destination URI not configured\n",
                            pPubCB->pdCB.comId);
               ret = (int)IPT_NOT_FOUND;
            }
         }

         if (ret == (int)IPT_OK)
         {
#ifdef TARGET_SIMU
            if (pPubCB->pSimUri != NULL)
            {
               /* Simualated/Destination device IP address */
               dummy = 0;
               res = iptGetAddrByName(pPubCB->pSimUri, &simDeviceIp, &dummy);
               if (res == TDC_OK)
               {
                  dummy = 0;
                  res = iptGetAddrByNameSim(pDestUri,
                                            simDeviceIp,
                                            &destIp,
                                            &dummy);
               }
            }
            else
            {
               simDeviceIp = IPTCom_getOwnIpAddr();
               dummy = 0;
               res = iptGetAddrByName(pDestUri,
                                      &destIp,
                                      &dummy);
            }
#else
            dummy = 0;
            res = iptGetAddrByName(pDestUri,
                                   &destIp,
                                   &dummy);
#endif

            if (res != TDC_OK)
            {
               if ((res == TDC_NO_CONFIG_DATA) || (res == TDC_MUST_FINISH_INIT))
               {
                  IPTVosPrint2(IPT_WARN, "Publishing of ComId=%d waiting for TDC configuration. Ret=%#x\n",
                               pPubCB->pdCB.comId, res);
               }
               else
               {
                  IPTVosPrint1(IPT_ERR,
                               "Publishing failed for ComId=%d\n",
                                pPubCB->pdCB.comId);
               }
               ret = res;
            }
            else
            {
               /* Find corresponding schedule group ix */
               if (!pdPubGrpTabFind(pPubCB->schedGrp, &schedIx))
               {
                  if (!pdPubGrpTabAdd(pPubCB->schedGrp, &schedIx))
                  {
                     /* Illegal schedGrp */
                     ret = (int)IPT_ERROR;
                  }
               }

               if (ret == IPT_OK)
               {
                  /* See if we already have a netbuffer CB for this comid+source */
   #ifdef TARGET_SIMU
                  pSendNetCB = pdSendNetCB_find(pPubCB->pdCB.comId, destIp, simDeviceIp);
   #else
                  pSendNetCB = pdSendNetCB_find(pPubCB->pdCB.comId, destIp);
   #endif   
                  if (pSendNetCB == NULL)
                  {
                     /* We did not have any, create a new one */
                     pSendNetCB = pdSendNetCB_create(pPubCB->pdCB.comId, destIp, &exchgPar, pPubCB);
   
                     if (pSendNetCB == NULL)
                     {
                        IPTVosPrint1(IPT_ERR,
                                     "Publishing failed for ComId=%d. Could not create net control block\n",
                                     pPubCB->pdCB.comId);
                        ret = (int)IPT_ERROR;      /* Could not create a new netbuffer CB */
                     }
                     else
                     {
#ifdef TARGET_SIMU
                        pSendNetCB->simDeviceIp = simDeviceIp;              /* Simulated source ip address (!=0 is used) */
#endif
                        ret = iptConfigGetComPar(exchgPar.comParId, &configComPar);
                        if (ret != (int)IPT_OK)
                        {
                           
                           /* Communication parameter not configured */
                           if (ret != (int)IPT_TDC_NOT_READY)
                           {
                              IPTVosPrint2(IPT_ERR,
                                           "Publishing failed for ComId=%d. No communication parameter configured for comParId=%d\n",
                                           pPubCB->pdCB.comId, exchgPar.comParId);
                           }
                           IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
                           IPTVosFree((BYTE *)pSendNetCB);
                        }
                        else
                        {
   #if defined(IF_WAIT_ENABLE)
                           pSendNetCB->comParId = exchgPar.comParId;
   #endif
                           pSendNetCB->pdSendSocket = configComPar.pdSendSocket;

                           /* Prepare the head in case of not deferred start */
                           datasetSize = pPubCB->pdCB.size;
                           ret = iptMarshallDSF(pPubCB->nLines, pPubCB->alignment,
                           							pPubCB->disableMarshalling,
                                                pPubCB->pDatasetFormat,
                                                pPubCB->pdCB.pDataBuffer, temp,
                                                &datasetSize);
                           if (ret != (int)IPT_OK)
                           {
                              IPTVosPrint1(IPT_ERR,
                                           "Publishing failed for ComId=%d. Marshalling of zero data failed\n",
                                           pPubCB->pdCB.comId);
                              IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
                              IPTVosFree((BYTE *)pSendNetCB);
                           }
                           else
                           {
                              /* Load dataset */
                              datasetSizeFCS = pPubCB->netDatabufferSize;
                              ret = iptLoadSendData(temp, datasetSize, pSendNetCB->pSendBuffer + sizeof(PD_HEADER), 
                                                    &datasetSizeFCS);
                              if (ret != (int)IPT_OK)
                              {
                                 IPTVosPrint1(IPT_ERR,
                                              "Publishing failed for ComId=%d. Marshalling of zero data failed\n",
                                              pPubCB->pdCB.comId);
                                 IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
                                 IPTVosFree((BYTE *)pSendNetCB);
                              }
                              else
                              {
                                 ((PD_HEADER*)pSendNetCB->pSendBuffer)->datasetLength = TOWIRE16(datasetSize); /*lint !e826 Type cast OK */
                                 
                                 pSendNetCB->size = sizeof(PD_HEADER) + datasetSizeFCS;
       
                                 /* Add the newly created comid CB first in linked list for netbuffer */
                                 pSendNetCB->pNext = IPTGLOBAL(pd.pFirstSendNetCB);
                                 IPTGLOBAL(pd.pFirstSendNetCB) = pSendNetCB;
                                 IPTGLOBAL(pd.netCBchanged) = 1;
                              }
                           }
                        }
                     }
                  }

                  if ((ret == IPT_OK) && (pSendNetCB != NULL))
                  {
                     pPubCB->destIp = destIp;
                     pPubCB->pSendNetCB = pSendNetCB;              /* Link to netbuffer CB */
                     pPubCB->pNetDatabuffer = pSendNetCB->pSendBuffer + sizeof(PD_HEADER); /* Link to netbuffers data buffer */
                     pPubCB->pPdHeader =  (PD_HEADER *)pSendNetCB->pSendBuffer;  /*lint !e826 Size is OK but described in framelen */
                     pPubCB->pSendNetCB->nPublisher++;
   
                     /* Insert the control block in the publish schedular group table */
                     if (pSchedGrpTab->pTable[schedIx].pFirstCB != NULL)
                     {
                        pPubCB->pNext = pSchedGrpTab->pTable[schedIx].pFirstCB;
                        pPubCB->pNext->pPrev = pPubCB;
                     }
                     pSchedGrpTab->pTable[schedIx].pFirstCB = pPubCB;
                  }
               }
            }
         }
      }
   }
   
   return (ret);
}
/*lint -restore */

/*******************************************************************************
NAME     : pdPubCB_finish
ABSTRACT : Finish craetion of subscribe control block after TDC is ready
RETURNS  : -
*/
static void pdPubCB_finish(void)
{
   PD_NOT_RESOLVED_PUB_CB *pNotResolvedPubCB;
   PD_NOT_RESOLVED_PUB_CB *pNextNotResolvedPubCB;
   int ret;
    
   pNotResolvedPubCB = IPTGLOBAL(pd.pFirstNotResPubCB); 
   while (pNotResolvedPubCB != NULL)
   {
      /* Next to be solved */
      pNextNotResolvedPubCB = pNotResolvedPubCB->pNext;
 
      ret = pdPubCB_add(pNotResolvedPubCB->pPubCB);
      if (ret != (int)IPT_OK)
      {
         /* TDC not ready? Finish later when TDC is ready */
         if ((ret == (int)IPT_TDC_NOT_READY) || (ret == TDC_NO_CONFIG_DATA) || 
             (ret == TDC_MUST_FINISH_INIT))
         {
            IPTGLOBAL(pd.finishSendAddrResolv) = 1;
         }
         else
         {
            pNotResolvedPubCB->pPubCB->waitingTdc = 0;   
         }
      }
      else
      {
         pNotResolvedPubCB->pPubCB->waitingTdc = 0;   
         
         if (!pNotResolvedPubCB->pPubCB->defStart)
         {
            /* Indicate that data has been updated onces, i.e. sending of data will
               be started immediately.
               This is done here of compability reason */
            pNotResolvedPubCB->pPubCB->pdCB.updatedOnce = TRUE;

            if (pNotResolvedPubCB->pPubCB->pSendNetCB)
            {
               /* Set to true to indicate that the buffer has been updated at least
                onces, i.e. sending of data will be started immediately.
               This is done here of compability reason  */
               pNotResolvedPubCB->pPubCB->pSendNetCB->updatedOnceNs = TRUE;
            }
         }
        
         /* Remove control block from list with unresolved */
         if (pNotResolvedPubCB->pNext != NULL)
            pNotResolvedPubCB->pNext->pPrev = pNotResolvedPubCB->pPrev;

         if (pNotResolvedPubCB->pPrev != NULL)
         {
            pNotResolvedPubCB->pPrev->pNext = pNotResolvedPubCB->pNext;
         }
         else
         {
            if (IPTGLOBAL(pd.pFirstNotResPubCB) == pNotResolvedPubCB)
            {
               IPTGLOBAL(pd.pFirstNotResPubCB) = pNotResolvedPubCB->pNext;  
            }
         }
         
         IPTVosFree((BYTE *)pNotResolvedPubCB);
      }
      
      pNotResolvedPubCB = pNextNotResolvedPubCB; 
   }
}

/*******************************************************************************
NAME     : pdRecNetCB_find
ABSTRACT : Find a comid receive net control block 
RETURNS  : Pointer to the comid net control block if found, NULL if not found
*/
static PD_REC_NET_CB *pdRecNetCB_find(
   UINT32 comId,      /* comId */
   UINT32 sourceIp   /* source IP */
#ifdef TARGET_SIMU
   ,UINT32 simDeviceIp    /* Destination device IP (simulation) */
#endif   
)
{
   PD_REC_NET_CB *pRecNetCB = IPTGLOBAL(pd.pFirstRecNetCB); 
   while (pRecNetCB != NULL)
   {
      /* Check if comid and source are equal */
      if ((pRecNetCB->comId == comId) &&
          (sourceIp == pRecNetCB->sourceIp))
      {
#ifdef TARGET_SIMU
         if (simDeviceIp == pRecNetCB->simDeviceIp)
#endif
            return pRecNetCB;
      }
      
      pRecNetCB = pRecNetCB->pNext;
   }
   
   return NULL;
}
 
/*******************************************************************************
NAME     : pdRecNetCB_get
ABSTRACT : Search for an equal net contol block.
           If not found creat a new 
RETURNS  : Pointer to net control block, NULL if error
*/
static PD_REC_NET_CB *pdRecNetCB_get(
   UINT32 comId,      /* comId */
   UINT32 sourceIp    /* source IP */
#ifdef TARGET_SIMU
   ,UINT32 simDeviceIp    /* destination device IP (simulation) */
#endif
)
{
   int ret;
   PD_REC_NET_CB *pRecNetCB;
   IPT_CONFIG_EXCHG_PAR_EXT exchgPar;
   IPT_CFG_DATASET_INT *pDataset;

   /* See if we already have a netbuffer CB for this comid+source */
#ifdef TARGET_SIMU
   pRecNetCB = pdRecNetCB_find(comId, sourceIp, simDeviceIp);
#else
   pRecNetCB = pdRecNetCB_find(comId, sourceIp);
#endif   

   if (pRecNetCB == NULL)
   {
      /* We did not have any, create a new one */
      pRecNetCB = (PD_REC_NET_CB *) IPTVosMalloc(sizeof(PD_REC_NET_CB));
      if (pRecNetCB == NULL)
      {
         /* Could not create memory for the CB */
         IPTVosPrint2(IPT_ERR,
                      "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                      comId, sizeof(PD_REC_NET_CB));
      }
      else
      {
         /* Get parameters for this comid from the configuration database */
         if ((iptConfigGetExchgPar(comId, &exchgPar) != (int)IPT_OK) ||
             ((pDataset = iptConfigGetDataset(exchgPar.datasetId)) == NULL))
         {
            IPTVosFree((BYTE *) pRecNetCB);
            IPTVosPrint1(IPT_ERR,
                    "Subscription failed for ComId=%d. ComId or Dataset not configured\n", 
                         comId);
            pRecNetCB = (PD_REC_NET_CB *)NULL;   /* Could not find dataset format */
         }
         else
         {
            /* Initialize */
            pRecNetCB->pNext = NULL;
            pRecNetCB->pPrev = NULL;
            pRecNetCB->comId = comId;
            pRecNetCB->nSubscriber = 0;
            pRecNetCB->updatedOnceNr = FALSE;
            pRecNetCB->pdInPackets = 0;

            /* Dataset contains size of complete data in dataset */
            pRecNetCB->size = (UINT16) pDataset->size;      
            pRecNetCB->alignment = pDataset->alignment;
            pRecNetCB->nLines = pDataset->nLines;
            pRecNetCB->disableMarshalling = pDataset->disableMarshalling;
            pRecNetCB->pDatasetFormat = pDataset->pFormat;
#ifdef TARGET_SIMU
            pRecNetCB->simDeviceIp = simDeviceIp;
#endif                  
            pRecNetCB->invalid = FALSE;
            pRecNetCB->timeRec = 0;    
            pRecNetCB->sourceIp = sourceIp;

            /* Create data buffer area */
            pRecNetCB->pDataBuffer = IPTVosMalloc(pRecNetCB->size);
            if (pRecNetCB->pDataBuffer == NULL)
            {
               IPTVosPrint2(IPT_ERR,
                            "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                            comId, pRecNetCB->size);
               IPTVosFree((BYTE *) pRecNetCB);
               pRecNetCB = (PD_REC_NET_CB *)NULL;   /* Could not create memory for the data buffer */
            }
            else
            {
               memset(pRecNetCB->pDataBuffer, 0, pRecNetCB->size);
   
               /* Add this comid to the PD receiver table */
      #ifdef TARGET_SIMU
               ret = pdRecTabAdd(comId, simDeviceIp, sourceIp, pRecNetCB);
      #else
               ret = pdRecTabAdd(comId, sourceIp, pRecNetCB);
      #endif
               if (ret == IPT_OK)
               {
                  /* Add the newly created comid CB first in linked list for netbuffer */
                  if (IPTGLOBAL(pd.pFirstRecNetCB) != NULL)
                  {
                     pRecNetCB->pNext = IPTGLOBAL(pd.pFirstRecNetCB);
                     pRecNetCB->pNext->pPrev = pRecNetCB;
                  }
                  IPTGLOBAL(pd.pFirstRecNetCB) = pRecNetCB;
               }
               else
               {
                  IPTVosFree((BYTE *) pRecNetCB->pDataBuffer);
                  IPTVosFree((BYTE *) pRecNetCB);
                  pRecNetCB = (PD_REC_NET_CB *)NULL;
               }         
            }
         }
      }
   }
   return(pRecNetCB);
}

/*******************************************************************************
NAME     : pdSubCB_add
ABSTRACT : Finish creation of subscribe control block
RETURNS  : 0 if ok else != 0
*/
static int pdSubCB_add(
   PD_SUB_CB *pSubCB   /* Pointer to subsribe control block */
   )   /* Pointer to exchange parameters */
{
   char * pNextSourceUri;
   char   srcUri[MAX_TOKLEN+1];
   const char *pSourceUri = NULL;
   const char *pDestUri = NULL;
   char *pFiltUri;
   int i;
   int schedIx = 0;
   int ret = (int)IPT_OK;
   UINT8 dummy;
   UINT8 noOfNetCB = 0;
   UINT32 destIp = 0;
   UINT32 sourceIp;
#ifdef TARGET_SIMU
   UINT32 simDeviceIp = 0;       /* Destination device IP address (simulation) */
#endif
   IPT_CONFIG_EXCHG_PAR_EXT exchgPar;
   PD_REC_NET_CB *pRecNetCB;
   T_TDC_RESULT res = TDC_OK;
   IPT_CFG_DATASET_INT *pDataset;
   SUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);

   memset(&exchgPar, 0, sizeof exchgPar);

   /* Get parameters for this comid from the configuration database */
   ret = iptConfigGetExchgPar(pSubCB->pdCB.comId, &exchgPar);
   if (ret != (int)IPT_OK)
   {               
      if (ret != (int)IPT_TDC_NOT_READY)
      {
         IPTVosPrint1(IPT_ERR, 
               "Subscription failed for ComId=%d. ComId not configured\n",
                      pSubCB->pdCB.comId);
      }
   }
   else
   {
      pSubCB->invalidBehaviour = (UINT8) exchgPar.pdRecPar.validityBehaviour;
      pSubCB->timeout = exchgPar.pdRecPar.timeoutValue;
   
      if (pSubCB->destId != 0)
      {
         ret = iptConfigGetDestIdPar(pSubCB->pdCB.comId, pSubCB->destId, &pDestUri);
         if (ret != (int)IPT_OK)
         {
            if (ret != (int)IPT_TDC_NOT_READY)
            {
               IPTVosPrint2(IPT_ERR,
                            "Subscription failed for ComId=%d. Destination Id=%d not configured\n",
                            pSubCB->pdCB.comId, pSubCB->destId);
            }
         }
      }
      else if (pSubCB->pDestUri)
      {
         pDestUri =  (char *)pSubCB->pDestUri;
      }
      else
      {
         pDestUri =  exchgPar.pdSendPar.pDestinationURI;
      }

      if (ret == (int)IPT_OK)
      {               
         if (pSubCB->filterId)
         {
            ret = iptConfigGetPdSrcFiltPar(pSubCB->pdCB.comId, pSubCB->filterId, &pSourceUri);
            if (ret != (int)IPT_OK)
            {        
               if (ret != (int)IPT_TDC_NOT_READY)
               {
                  IPTVosPrint2(IPT_ERR,
                        "Subscription failed for ComId=%d. FilterId=%d not configured\n",
                               pSubCB->pdCB.comId, pSubCB->filterId);
               }
            }
         }
         else if (pSubCB->pSourceUri)
         {
            pSourceUri = pSubCB->pSourceUri; 
         }
         else
         {
            pSourceUri = exchgPar.pdRecPar.pSourceURI; 
         }
      }
   }

   /* Data buffer not created ? */
   if ((pSubCB->pdCB.pDataBuffer == NULL) && (ret == IPT_OK))
   {
      /* Get parameters for this comid from the configuration database */
      if ((pDataset = iptConfigGetDataset(exchgPar.datasetId)) == NULL)
      {
         IPTVosPrint2(IPT_ERR,
            "Subscription failed for ComId=%d. Dataset ID=%d not configured\n",
             pSubCB->pdCB.comId, exchgPar.datasetId);
         ret = (int)IPT_NOT_FOUND;   /* Could not find dataset format */
      }
      else if (pDataset->size > PD_DATASET_MAXSIZE)
      {
         IPTVosPrint4(IPT_ERR,
                 "Subscription failed for ComId=%d. Dataset size=%d greater than PD max=%d for dataset ID=%d\n", 
                      pSubCB->pdCB.comId, pDataset->size, PD_DATASET_MAXSIZE, exchgPar.datasetId);
         ret = (int)IPT_ILLEGAL_SIZE;
      }
      else if (pDataset->size == 0)
      {
         IPTVosPrint2(IPT_ERR,
                 "Subscription failed for ComId=%d. Dynamic dataset size for dataset ID=%d\n",
                 pSubCB->pdCB.comId, exchgPar.datasetId);
         ret = (int)IPT_ILLEGAL_SIZE;
      }
      else
      {
         /* Dataset contains size of complete data in dataset */
         pSubCB->pdCB.size = (UINT16) pDataset->size;      

         /* Create data buffer area */
         pSubCB->pdCB.pDataBuffer = IPTVosMalloc(pSubCB->pdCB.size);
         if (pSubCB->pdCB.pDataBuffer == NULL)
         {
            IPTVosPrint2(IPT_ERR, "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                         pSubCB->pdCB.comId, pSubCB->pdCB.size);
            ret =  (int)IPT_MEM_ERROR;   /* Could not create memory for the data buffer */
         }
         else
         {
            memset(pSubCB->pdCB.pDataBuffer, 0, pSubCB->pdCB.size);
         }
      }
   }
   
   if (ret == (int)IPT_OK)
   {
      /* Find corresponding schedule group ix */
      if (!pdSubGrpTabFind(pSubCB->schedGrp, &schedIx))
      {
         if (!pdSubGrpTabAdd(pSubCB->schedGrp, &schedIx))
         {
            IPTVosPrint1(IPT_ERR, 
                         "Subscription failed for ComId=%d. Could not create schedular group\n",
                         pSubCB->pdCB.comId);
            /* Illegal schedGrp */
            ret = (int)IPT_ERROR;
         }
      }
   }
   
#ifdef TARGET_SIMU
   if (ret == (int)IPT_OK)
   {
      if (pSubCB->pSimUri != NULL)
      {
         /* Simualated/Destination device IP address */
         dummy = 0;
         res = iptGetAddrByName(pSubCB->pSimUri, &simDeviceIp, &dummy);
         if (res != TDC_OK)
         {
            if ((res == TDC_NO_CONFIG_DATA) || (res == TDC_MUST_FINISH_INIT))
            {
               IPTVosPrint1(IPT_WARN, "TDC waiting for configuration. Ret=%#x\n", 
                            res);
            }
            else
            {
               IPTVosPrint1(IPT_ERR,
                            "Subscription failed for ComId=%d\n", 
                            pSubCB->pdCB.comId);
            }
            ret = res;
         }
      }
      else
      {
         simDeviceIp = IPTCom_getOwnIpAddr();
      }
   }
#endif
   
   if (ret == (int)IPT_OK)
   {
      if (pDestUri != NULL)
      {
         dummy = 0;
#ifdef TARGET_SIMU
         if (pSubCB->pSimUri != NULL)
         {
            res = iptGetAddrByNameSim(pDestUri,
                                      simDeviceIp,
                                      &destIp,
                                      &dummy);
         }
         else
         {
            res = iptGetAddrByName(pDestUri,
                                   &destIp,
                                   &dummy);
         }
#else
         res = iptGetAddrByName(pDestUri,
                                &destIp,
                                &dummy);
#endif
         if (res != TDC_OK)
         {
            if ((res == TDC_NO_CONFIG_DATA) || (res == TDC_MUST_FINISH_INIT))
            {
               IPTVosPrint2(IPT_WARN, "Subscription for ComId=%d. TDC waiting for configuration. Ret=%#x\n", 
                            pSubCB->pdCB.comId, res);
            }
            else
            {
               IPTVosPrint1(IPT_ERR,
                            "Subscription failed for ComId=%d\n", 
                            pSubCB->pdCB.comId);
            }
            ret = res;
         }
      }
      else
      {
         destIp = 0;
      }
   }

   if (ret == (int)IPT_OK)
   {
      /* Any source filter? */
      if (pSourceUri != NULL)
      {
         strncpy(srcUri, pSourceUri, MAX_TOKLEN);
         pFiltUri = srcUri;
         do
         {
            pNextSourceUri = strchr(pFiltUri,',');
            if (pNextSourceUri != NULL)
            {
               /* terminate string */
               pNextSourceUri[0] = 0;
               pNextSourceUri++;  
            }
            dummy = 0;
#ifdef TARGET_SIMU
            if (pSubCB->pSimUri != NULL)
            {
               res = iptGetAddrByNameSim(pFiltUri,
                                         simDeviceIp,
                                         &sourceIp,
                                         &dummy);
            }
            else
            {
               res = iptGetAddrByName(pFiltUri,
                                      &sourceIp,
                                      &dummy);
            }
#else
            res = iptGetAddrByName(pFiltUri,
                                   &sourceIp,
                                   &dummy);
#endif
            if (res == TDC_OK)
            {
                /* If destination IP is a multicastgroup, join the group */
               if (isMulticastIpAddr(destIp))
               {
                  /* Source ip = local device? */
                  if (sourceIp == 0x7f000001)
                  {
                     /* Change the source address to the own IP address as it will 
                        be the source IP address for multicast messages */  
                     sourceIp = IPTCom_getOwnIpAddr();
                     
                     /* Own IP address not found? */
                     if (sourceIp == 0)
                     {
                        ret = (int)IPT_ERR_NO_IPADDR;
                        break;
                     }
                  }
               }
            }
            else
            {
               if ((res == TDC_NO_CONFIG_DATA) || (res == TDC_MUST_FINISH_INIT))
               {
                  IPTVosPrint1(IPT_WARN,
                               "TDC waiting for configuration. Ret=%#x\n", res);
               }
               else
               {
                  IPTVosPrint1(IPT_ERR, 
                              "Subscription failed for ComId=%d\n", 
                               pSubCB->pdCB.comId);
               }
               ret = res;
               break;
            }

#ifdef TARGET_SIMU
            pRecNetCB = pdRecNetCB_get(pSubCB->pdCB.comId, sourceIp, simDeviceIp);
#else
            pRecNetCB = pdRecNetCB_get(pSubCB->pdCB.comId, sourceIp);
#endif         
            if (pRecNetCB == NULL)
            {
               IPTVosPrint2(IPT_ERR,
                            "Subscription failed for ComId=%d wit source filter IP=%#x. Could not create net control block\n",
                            pSubCB->pdCB.comId, sourceIp);
               ret = (int)IPT_ERROR;
               break;
            }
            if (noOfNetCB < MAX_SRC_FILTER)
            {
               pSubCB->pRecNetCB[noOfNetCB] = pRecNetCB;
               /* Link to netbuffers data buffer */
               pSubCB->pNetbuffer[noOfNetCB] = pRecNetCB->pDataBuffer; 
               pRecNetCB->nSubscriber++;

               noOfNetCB++;
            }
            else
            {
               IPTVosPrint1(IPT_ERR,
                            "Subscription failed for ComId=%d. Too many source filters\n",
                            pSubCB->pdCB.comId);
               ret = (int)IPT_ERROR;
               break;
            }

            pFiltUri = pNextSourceUri;
         }
         while(pNextSourceUri != NULL );
      }
      else
      {
         /* No source filter */
#ifdef TARGET_SIMU
         pSubCB->pRecNetCB[0] = pdRecNetCB_get(pSubCB->pdCB.comId, 0, simDeviceIp);
#else      
         pSubCB->pRecNetCB[0] = pdRecNetCB_get(pSubCB->pdCB.comId, 0);
#endif      
         if (pSubCB->pRecNetCB[0] == NULL)
         {
            IPTVosPrint1(IPT_ERR, 
                         "Subscription failed for ComId=%d. Could not create net control block\n",
                         pSubCB->pdCB.comId);
         
            ret = (int)IPT_ERROR;
         }
         else
         {
            noOfNetCB = 1;     
      
            /* Link to netbuffers data buffer */
            pSubCB->pNetbuffer[0] = pSubCB->pRecNetCB[0]->pDataBuffer; 
            pSubCB->pRecNetCB[0]->nSubscriber++;
         
         }
      }
   
      if (ret == (int)IPT_OK)
      {
         pSubCB->noOfNetCB = noOfNetCB;
         pSubCB->destIp = destIp;
   #ifdef TARGET_SIMU
         pSubCB->simDeviceIp = simDeviceIp;
   #endif
          /* If destination IP is a multicastgroup, join the group */
         if (isMulticastIpAddr(destIp))
         {
            joinPDmulticastAddress(
#ifdef TARGET_SIMU
                                   simDeviceIp, /* Simualted IP address */
#endif
                                   destIp);  /* multicast IP address */
         }
        
         if (pSchedGrpTab->pTable[schedIx].pFirstCB != NULL)
         {
            pSubCB->pNext = pSchedGrpTab->pTable[schedIx].pFirstCB;
            pSubCB->pNext->pPrev = pSubCB;
         }
         else
         {
            pSubCB->pNext = NULL;
         }
         pSchedGrpTab->pTable[schedIx].pFirstCB = pSubCB;
      }
      else
      {
         for (i = 0; i < noOfNetCB; i++)
         {
            if (pSubCB->pRecNetCB[i])
            {
               pSubCB->pRecNetCB[i]->nSubscriber--;
               if (pSubCB->pRecNetCB[i]->nSubscriber == 0)
               {
                  pdRecNetCB_destroy(pSubCB->pRecNetCB[i]);
               }
               pSubCB->pRecNetCB[i] = NULL;
            }
         }
         pSubCB->noOfNetCB = 0;
      }
   }

   return ret;
}

/*******************************************************************************
NAME     : pdSubCB_finish
ABSTRACT : Finish craetion of subscribe control block after TDC is ready
RETURNS  : -
*/
static void pdSubCB_finish(void)
{
   PD_NOT_RESOLVED_SUB_CB *pNotResolvedSubCB;
   PD_NOT_RESOLVED_SUB_CB *pNextNotResolvedSubCB;
   int ret;

   pNotResolvedSubCB = IPTGLOBAL(pd.pFirstNotResSubCB); 
   while (pNotResolvedSubCB != NULL)
   {
      /* Next to be solved */
      pNextNotResolvedSubCB = pNotResolvedSubCB->pNext;
 
      ret = pdSubCB_add(pNotResolvedSubCB->pSubCB);
   
      if (ret != (int)IPT_OK)
      {
         /* TDC not ready? Finish later when TDC is ready */
         if ((ret == (int)IPT_TDC_NOT_READY) || (ret == TDC_NO_CONFIG_DATA) || 
             (ret == TDC_MUST_FINISH_INIT) || (ret == (int)IPT_ERR_NO_IPADDR))
         {
            IPTGLOBAL(pd.finishRecAddrResolv) = 1;
         }
         else
         {
            pNotResolvedSubCB->pSubCB->waitingTdc = 0;
         }
      }
      else
      {
         pNotResolvedSubCB->pSubCB->waitingTdc = 0;
         
         /* Remove control block from list with unresolved */
         if (pNotResolvedSubCB->pNext != NULL)
            pNotResolvedSubCB->pNext->pPrev = pNotResolvedSubCB->pPrev;

         if (pNotResolvedSubCB->pPrev != NULL)
         {
            pNotResolvedSubCB->pPrev->pNext = pNotResolvedSubCB->pNext;
         }
         else
         {
            IPTGLOBAL(pd.pFirstNotResSubCB) = pNotResolvedSubCB->pNext;  
         }

         IPTVosFree((BYTE *)pNotResolvedSubCB);
      }
      
      pNotResolvedSubCB = pNextNotResolvedSubCB; 
   }
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       pdGrpTabTerminate
ABSTRACT:   Terminates the schedular groups look up table
RETURNS:    -
*/
void pdGrpTabTerminate(void)
{
   if (IPTGLOBAL(pd.subSchedGrpTab.initialized))
   {
      IPTVosFree((BYTE *) IPTGLOBAL(pd.subSchedGrpTab.pTable));
   }

   if (IPTGLOBAL(pd.pubSchedGrpTab.initialized))
   {
      IPTVosFree((BYTE *) IPTGLOBAL(pd.pubSchedGrpTab.pTable));
   }
}

/*******************************************************************************
NAME:       pdGrpTabInit
ABSTRACT:   Initialises the schedular groups look up table. 
RETURNS:    -
*/
int pdGrpTabInit(void)
{
   int ret;

   IPTGLOBAL(pd.subSchedGrpTab.maxItems) = 0;
   IPTGLOBAL(pd.subSchedGrpTab.nItems) = 0;
   IPTGLOBAL(pd.subSchedGrpTab.pTable) = NULL;
   IPTGLOBAL(pd.subSchedGrpTab.initialized) = TRUE;

   /* Add memory to start with */
   ret = pdSubGrpTabExpand();
   if (ret == IPT_OK)
   {
      IPTGLOBAL(pd.pubSchedGrpTab.maxItems) = 0;
      IPTGLOBAL(pd.pubSchedGrpTab.nItems) = 0;
      IPTGLOBAL(pd.pubSchedGrpTab.pTable) = NULL;
      IPTGLOBAL(pd.pubSchedGrpTab.initialized) = TRUE;

      /* Add memory to start with */
      ret = pdPubGrpTabExpand();
   }
   return(ret);
}

/*******************************************************************************
NAME:       pdPubGrpTabFind
ABSTRACT:   Find item in the publish schedular group look up table.
RETURNS:    1 if found, 0 if not found
*/
int pdPubGrpTabFind(
   UINT32 schedGrp,   /* Key, schedule group */
   int    *pIndex)    /* Pointer to index */
{
   int i;
   int ret = 0;
   PUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.pubSchedGrpTab);

   /* Find item */
   for (i = 0; i < pSchedGrpTab->nItems ; i++)
   {
      if (pSchedGrpTab->pTable[i].schedGrp == schedGrp)
      {
         *pIndex = i;
         ret = 1;    
      }
   }

   return ret;
}

/*******************************************************************************
NAME:       pdSubGrpTabFind
ABSTRACT:   Find item in the subsribe schedular group look up table
RETURNS:    1 if found, 0 if not found
*/
int pdSubGrpTabFind(
   UINT32 schedGrp,   /* Key, schedule group */
   int    *pIndex)    /* Pointer to index */
{
   int i;
   int ret = 0;
   SUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);

   /* Find item */
   for (i = 0; i < pSchedGrpTab->nItems ; i++)
   {
      if (pSchedGrpTab->pTable[i].schedGrp == schedGrp)
      {
         *pIndex = i;
         ret = 1;    
      }
   }

   return ret;
}

/*******************************************************************************
NAME:       PD_finish_subscribe_publish
ABSTRACT:   Finish subscription and publishing called when TDC is ready
RETURNS:    
*/
void PD_finish_subscribe_publish(void)
{
   if (IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      IPTGLOBAL(pd.finishRecAddrResolv) = 0;
   
#if defined(IF_WAIT_ENABLE)
      if (IPTGLOBAL(ifRecReadyPD))
      {
         pdSubCB_finish();
      }
      else
      {
         IPTGLOBAL(pd.finishRecAddrResolv) = 1;
      }
#else
      pdSubCB_finish();
#endif      
      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PD_finish_subscribe_publish: IPTVosPutSem(recSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PD_finish_subscribe_publish: IPTVosGetSem(recSem) ERROR\n");
   }
   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      IPTGLOBAL(pd.finishSendAddrResolv) = 0;
   
      pdPubCB_finish();

      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PD_finish_subscribe_publish: IPTVosPutSem(recSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PD_finish_subscribe_publish: IPTVosGetSem(sendSem) ERROR\n");
   }
}

/*******************************************************************************
NAME     : pdPubComidCB_get
ABSTRACT : Get pointer to a publish control block
RETURNS  : Pointer to the comid CB, NULL if error
*/
PD_PUB_CB *pdPubComidCB_get(
   UINT32 schedGrp,   /* schedule group */
   UINT32 comId,      /* comId */
   UINT32 destId,     /* destination URI Id */
   const char *pDest, /* Pointer to string with destination URI. 
                         Will override information in the configuration database. 
                         Set to NULL if not used. */
   UINT8 defStart    /* Deferred start flag, TRUE if deferred start */
#ifdef TARGET_SIMU
   ,const char *pSimUri /* Source address (for simulation), NULL if not used */
#endif
)
{
   int ret = (int)IPT_OK;
   PD_PUB_CB *pPubCB;
   PD_NOT_RESOLVED_PUB_CB *pNotResolvedPubCB;

   /* Create control block */
   pPubCB = (PD_PUB_CB *) IPTVosMalloc(sizeof(PD_PUB_CB));
   if (pPubCB != NULL)
   {
      /* Initialize */
      pPubCB->pNext = NULL;
      pPubCB->pPrev = NULL;
      pPubCB->pdCB.invalid = FALSE;
      pPubCB->pdCB.updatedOnce = FALSE;
      pPubCB->pubCBchanged = FALSE;
      pPubCB->waitingTdc = 0;
      pPubCB->pdCB.size = 0;      
      pPubCB->pdCB.pDataBuffer = NULL;
      pPubCB->schedGrp = schedGrp;
      pPubCB->pdCB.comId = comId;
      pPubCB->destId = destId;
      if ((destId == 0) && (pDest))
      {
         pPubCB->pDestUri = (char *) IPTVosMalloc(strlen(pDest) +1);
         if (pPubCB->pDestUri != NULL)
         {
            strcpy(pPubCB->pDestUri, pDest);
         }
         else
         {
            ret = (int)IPT_ERROR;
            IPTVosPrint2(IPT_ERR,
                         "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                         comId, strlen(pDest) +1);
         }
      }
      else
      {
         pPubCB->pDestUri = NULL;
      }
      
      if (ret == IPT_OK)
      {
         pPubCB->destIp = 0;
         pPubCB->defStart = defStart;
         pPubCB->pSendNetCB = NULL;
         pPubCB->pNetDatabuffer = NULL;
         pPubCB->pPdHeader = NULL;
   #ifdef TARGET_SIMU   
         if (pSimUri)
         {
            pPubCB->pSimUri = (char *) IPTVosMalloc(strlen(pSimUri) +1);
            if (pPubCB->pSimUri != NULL)
            {
               strcpy(pPubCB->pSimUri, pSimUri);
            }
            else
            {
               ret = (int)IPT_ERROR;
               IPTVosPrint2(IPT_ERR,
                            "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                            comId, strlen(pSimUri) +1);
            }
         }
         else
         {
            pPubCB->pSimUri = NULL;
         }
   #endif   
      }

      if (ret == IPT_OK)
      {
         ret = pdPubCB_add(pPubCB);

         if (ret != (int)IPT_OK)
         {
            /* TDC not ready? Finish later when TDC is ready */
            if ((ret == (int)IPT_TDC_NOT_READY) || (ret == TDC_NO_CONFIG_DATA) || 
                (ret == TDC_MUST_FINISH_INIT))
            {
               /* Create a not resolved CB */
               pNotResolvedPubCB = (PD_NOT_RESOLVED_PUB_CB *)IPTVosMalloc(sizeof(PD_NOT_RESOLVED_PUB_CB));
               if (pNotResolvedPubCB == NULL)
               {
                  IPTVosPrint2(IPT_ERR,
                               "Publishing failed for ComId=%d. Could not allocate memory size=%d\n",
                               comId, sizeof(PD_NOT_RESOLVED_PUB_CB));
                  if (pPubCB->pdCB.pDataBuffer != NULL )
                  {
                     IPTVosFree((BYTE *)pPubCB->pdCB.pDataBuffer);
                  }
                  if (pPubCB->pDestUri != NULL)
                  {
                     IPTVosFree((BYTE *)pPubCB->pDestUri);
                  }                  
                  IPTVosFree((BYTE *)pPubCB);
                  pPubCB =  (PD_PUB_CB *)NULL;
               }
               else
               {
                  pPubCB->waitingTdc = 1;
                 
                  /* Save publish CB */
                  pNotResolvedPubCB->pPubCB = pPubCB;
      
                  pNotResolvedPubCB->pPrev = NULL;
                  /* Insert the control block in the list of not resolved control blocks */
                  if (IPTGLOBAL(pd.pFirstNotResPubCB) != NULL)
                  {
                     pNotResolvedPubCB->pNext = IPTGLOBAL(pd.pFirstNotResPubCB);
                     pNotResolvedPubCB->pNext->pPrev = pNotResolvedPubCB;
                  }
                  else
                  {
                     pNotResolvedPubCB->pNext = NULL;
                  }
                  IPTGLOBAL(pd.pFirstNotResPubCB) = pNotResolvedPubCB;

                  IPTGLOBAL(pd.finishSendAddrResolv) = 1;
               }
            }
            else
            {
               if (pPubCB->pdCB.pDataBuffer != NULL)
               {
                  IPTVosFree((BYTE *)pPubCB->pdCB.pDataBuffer);
               }
               if (pPubCB->pDestUri != NULL)
               {
                  IPTVosFree((BYTE *)pPubCB->pDestUri);
               }
               IPTVosFree((BYTE *)pPubCB);
               pPubCB =  (PD_PUB_CB *)NULL;
            }
         }
      }
   }
   else
   {
      /* Could not create memory for the CB */
      IPTVosPrint2(IPT_ERR, "Failed to publish ComId=%d. Could not allocate memory size=%d\n",
                   comId, sizeof(PD_PUB_CB));
   }
   
   return pPubCB;
}

/*******************************************************************************
NAME     : pdSubComidCB_get
ABSTRACT : Get pointer to a subscribe control block
RETURNS  : Pointer to the comid CB, NULL if error
*/
PD_SUB_CB *pdSubComidCB_get(
   UINT32 schedGrp,       /* Schedule group */
   UINT32 comId,          /* ComId */
   UINT32 filterId,       /* Source URI filter ID */
   const char *pSource,    /* Pointer to string with source URI. 
                              Will override information in the configuration 
                              database. 
                              Set to NULL if not used. */
   UINT32 destId,         /* Destination URI Id */
   const char *pDest      /* Pointer to string with destination URI. 
                             Will override information in the configuration database. 
                             Set NULL if not used. */
#ifdef TARGET_SIMU
   ,const char *pSimUri   /* Destination address (simulation) */
#endif   
)
{
   int i;
   int ret = (int)IPT_OK;
   PD_SUB_CB *pSubCB;
   PD_NOT_RESOLVED_SUB_CB *pNotResolvedSubCB = NULL;

   /* Create control block */
   pSubCB = (PD_SUB_CB *) IPTVosMalloc(sizeof(PD_SUB_CB));

   /* New schedGrpBuffer CB created? */
   if (pSubCB != NULL)
   {
      /* Initialize */
      pSubCB->pNext = NULL;
      pSubCB->pPrev = NULL;
      pSubCB->pdCB.invalid = TRUE;
      pSubCB->pdCB.updatedOnce = FALSE;
      pSubCB->noOfNetCB = 0;
      pSubCB->invalidBehaviour = 0;
      pSubCB->waitingTdc = 0;
      pSubCB->pdCB.size = 0;      
      pSubCB->pdCB.pDataBuffer = NULL;
      pSubCB->schedGrp = schedGrp;
      pSubCB->timeout = 0;
      pSubCB->pdCB.comId = comId;
      pSubCB->destId = destId;

      if ((destId == 0) && (pDest))
      {
         pSubCB->pDestUri = (char *) IPTVosMalloc(strlen(pDest) +1);
         if (pSubCB->pDestUri != NULL)
         {
            strcpy(pSubCB->pDestUri, pDest);
         }
         else
         {
            ret = (int)IPT_ERROR;
            IPTVosPrint2(IPT_ERR,
                         "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                         comId, strlen(pDest) +1);
         }
      }
      else
      {
         pSubCB->pDestUri = NULL;
      }

      pSubCB->destIp = 0;
      pSubCB->filterId = filterId;

      if ((filterId == 0) && (pSource))
      {
         pSubCB->pSourceUri = (char *) IPTVosMalloc(strlen(pSource) +1);
         if (pSubCB->pSourceUri != NULL)
         {
            strcpy(pSubCB->pSourceUri, pSource);
         }
         else
         {
            ret = (int)IPT_ERROR;
            IPTVosPrint2(IPT_ERR,
                         "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                         comId, strlen(pSource) +1);
            
         }
      }
      else
      {
         pSubCB->pSourceUri = NULL;
      }

      for (i = 0; i < MAX_SRC_FILTER; i++)
      {
         pSubCB->pRecNetCB[i] = NULL;
         pSubCB->pNetbuffer[i] = NULL;
      }
#ifdef TARGET_SIMU   
      pSubCB->simDeviceIp = 0;
      if (pSimUri)
      {
         pSubCB->pSimUri = (char *) IPTVosMalloc(strlen(pSimUri) +1);
         if (pSubCB->pSimUri != NULL)
         {
            strcpy(pSubCB->pSimUri, pSimUri);
         }
         else
         {
            ret = (int)IPT_ERROR;
            IPTVosPrint2(IPT_ERR,
                         "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                         comId, strlen(pSimUri) +1);
         }
      }
      else
      {
         pSubCB->pSimUri = NULL;
      }
#endif   

      if (ret == IPT_OK)
      {
   #if defined(IF_WAIT_ENABLE)
         if (!IPTGLOBAL(ifRecReadyPD))
         {
            ret = (int)IPT_ERR_NO_IPADDR;
         }
         else
   #endif
         {
            ret = pdSubCB_add(pSubCB);
         }

         if (ret != (int)IPT_OK)
         {
            /* TDC not ready? Finish later when TDC is ready */
            if ((ret == (int)IPT_TDC_NOT_READY) || (ret == TDC_NO_CONFIG_DATA) || 
                (ret == TDC_MUST_FINISH_INIT) || (ret == (int)IPT_ERR_NO_IPADDR))
            {
               ret = (int)IPT_OK;

               pSubCB->waitingTdc = 1;
               
               /* Create a not resolved CB */
               pNotResolvedSubCB = (PD_NOT_RESOLVED_SUB_CB *)IPTVosMalloc(sizeof(PD_NOT_RESOLVED_SUB_CB));
               if (pNotResolvedSubCB == NULL)
               {
                  ret = (int)IPT_ERROR;
                  IPTVosPrint2(IPT_ERR,
                               "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                               comId, sizeof(PD_NOT_RESOLVED_SUB_CB));
               }
               else
               {
                  if (ret == (int)IPT_OK)
                  {
                     /* Save subscribe CB */
                     pNotResolvedSubCB->pSubCB = pSubCB;
         
                     pNotResolvedSubCB->pPrev = NULL;
                     /* Insert the control block in the list of not resolved control blocks */
                     if (IPTGLOBAL(pd.pFirstNotResSubCB) != NULL)
                     {
                        pNotResolvedSubCB->pNext = IPTGLOBAL(pd.pFirstNotResSubCB);
                        pNotResolvedSubCB->pNext->pPrev = pNotResolvedSubCB;
                     }
                     else
                     {
                        pNotResolvedSubCB->pNext = NULL;
                     }
                     IPTGLOBAL(pd.pFirstNotResSubCB) = pNotResolvedSubCB;

                     IPTGLOBAL(pd.finishRecAddrResolv) = 1;
                  }
               }
            }
         }
      }

      if (ret != IPT_OK)
      {
         if (pSubCB->pSourceUri)
         {
            IPTVosFree((BYTE *)pSubCB->pSourceUri);
         }
         if (pSubCB->pDestUri)
         {
            IPTVosFree((BYTE *)pSubCB->pDestUri);
         }
#ifdef TARGET_SIMU
         if (pSubCB->pSimUri != NULL)
         {
            IPTVosFree((BYTE *)pSubCB->pSimUri);
         }
#endif           
         if (pNotResolvedSubCB)
         {
            IPTVosFree((BYTE *)pNotResolvedSubCB);
         }
         if (pSubCB->pdCB.pDataBuffer)
         {
            IPTVosFree((BYTE *)pSubCB->pdCB.pDataBuffer);
         }
         IPTVosFree((BYTE *)pSubCB);
         pSubCB = (PD_SUB_CB *)NULL;
      }
   }
   else
   {
      IPTVosPrint2(IPT_ERR,
                   "Subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                   comId, sizeof(PD_SUB_CB));
   }
   
   return pSubCB;
}

/*******************************************************************************
NAME     : pdPub_renew
ABSTRACT : Renew publishing if any IP address has been changed
RETURNS  : 0 if ok else != 0
*/
int pdPub_renew(
   PD_PUB_CB *pPubCB)   /* Pointer to subsribe control block */
{
   const char *pDestUri = NULL;
   int ret = (int)IPT_OK;
   int exchgParCollected = 0;
   int schedIx;
   BYTE temp[PD_DATASET_MAXSIZE];
   UINT8 dummy;
   UINT32 destIp;
#ifdef TARGET_SIMU
   UINT32 simDeviceIp = 0;
#endif
   UINT32 datasetSize;
   UINT32 datasetSizeFCS;
   T_TDC_RESULT res = TDC_OK;
   PD_SEND_NET_CB *pSendNetCB;
   IPT_CONFIG_EXCHG_PAR_EXT exchgPar;
   IPT_CFG_DATASET_INT *pDataset;
   IPT_CONFIG_COM_PAR_EXT configComPar;
   PD_RED_ID_ITEM redIdItem;
   PD_RED_ID_ITEM *pRedIdItem;
   PD_HEADER *pHeader;
   PD_PUB_CB *pGrpPubCB;
   PUBGRPTAB *pSchedGrpTab;

   if (!pPubCB->waitingTdc)
   {
      /* Data buffer not created? */
      if (pPubCB->pdCB.pDataBuffer == NULL)
      {
         /* Get parameters for this comid from the configuration database */
         ret = iptConfigGetExchgPar(pPubCB->pdCB.comId, &exchgPar);
         if (ret != (int)IPT_OK)
         {
            IPTVosPrint1(IPT_ERR, 
                  "Renewing publishing failed for ComId=%d. ComId not configured\n",
                         pPubCB->pdCB.comId);
         }
         else if (exchgPar.pdSendPar.cycleTime == 0)
         {
            IPTVosPrint1(IPT_ERR,
                    "Renewing publishing failed for ComId=%d. Zero cycle time configured\n", 
                         pPubCB->pdCB.comId);
            ret = IPT_ERROR;
         }
         else
         {
            exchgParCollected = 1;
            
            /* Get parameters for this comid from the configuration database */
            if ((pDataset = iptConfigGetDataset(exchgPar.datasetId)) == NULL)
            {
               IPTVosPrint2(IPT_ERR,
                  "Renewing publishing failed for comId=%d. Dataset ID=%d not configured\n",
                   pPubCB->pdCB.comId, exchgPar.datasetId);
               ret = (int)IPT_NOT_FOUND;   /* Could not find dataset format */
            }
            else if (pDataset->size > PD_DATASET_MAXSIZE)
            {
               IPTVosPrint4(IPT_ERR,
                       "Renewing publishing failed for comId=%d. Dataset size=%d greater than PD max=%d for dataset ID=%d\n", 
                            pPubCB->pdCB.comId, pDataset->size, PD_DATASET_MAXSIZE, exchgPar.datasetId);
               ret = (int)IPT_ILLEGAL_SIZE;
            }
            else if (pDataset->size == 0)
            {
               IPTVosPrint2(IPT_ERR,
                       "Renewing publishing failed for comId=%d. Dynamic dataset size for dataset ID=%d\n",
                        pPubCB->pdCB.comId, exchgPar.datasetId);
               ret = (int)IPT_ILLEGAL_SIZE;
            }
            else
            {
               /* Dataset contains size of complete data in dataset */
               pPubCB->pdCB.size = (UINT16) pDataset->size;
               pPubCB->netDatabufferSize = iptCalcSendBufferSize(pDataset->size);      
               pPubCB->alignment = pDataset->alignment;
               pPubCB->nLines = pDataset->nLines;
               pPubCB->pDatasetFormat = pDataset->pFormat;

               /* Create data buffer area */
               pPubCB->pdCB.pDataBuffer = IPTVosMalloc(pPubCB->pdCB.size);
               if (pPubCB->pdCB.pDataBuffer == NULL)
               {
                  IPTVosPrint2(IPT_ERR, "Renewing publishing failed for comId=%d. Could not allocate memory size=%d\n",
                               pPubCB->pdCB.comId, pPubCB->pdCB.size);
                  return (int)IPT_MEM_ERROR;   /* Could not create memory for the data buffer */
               }
               memset(pPubCB->pdCB.pDataBuffer, 0, pPubCB->pdCB.size);
            }
         }
      }

      if (ret == IPT_OK)
      {
         /* Get destination URI. Use parameter if specified, otherwise get from 
            config DB */
         if (pPubCB->destId != 0)
         {
            ret = iptConfigGetDestIdPar(pPubCB->pdCB.comId, pPubCB->destId, &pDestUri);
            if (ret != (int)IPT_OK)
            {
               IPTVosPrint2(IPT_ERR,
                            "Renewing publishing failed for ComId=%d. Destination Id=%d not configured\n",
                            pPubCB->pdCB.comId, pPubCB->destId);
            }
         }
         else if (pPubCB->pDestUri != NULL)
         {
            pDestUri = (char *)pPubCB->pDestUri;
         }
         else
         {
            if (!exchgParCollected)
            {
               /* Get parameters for this comid from the configuration database */
               ret = iptConfigGetExchgPar(pPubCB->pdCB.comId, &exchgPar);
               if (ret != (int)IPT_OK)
               {
                  IPTVosPrint1(IPT_ERR, 
                        "Renewing publishing failed for ComId=%d. ComId not configured\n",
                               pPubCB->pdCB.comId);
               }
               else
               {
                  exchgParCollected = 1;
               }
            }

            if (ret == IPT_OK)
            {
               pDestUri =  exchgPar.pdSendPar.pDestinationURI;/*lint !e644 exchgPar is initialized here */
               if (pDestUri == NULL)
               {
                  IPTVosPrint1(IPT_ERR,
                               "Renewing publishing failed. No destination URI in config DB, comid = %d\n", 
                               pPubCB->pdCB.comId);
                  ret = (int)IPT_NOT_FOUND;
               }
            }
         }

         if (ret == IPT_OK)
         {
   #ifdef TARGET_SIMU
            if (pPubCB->pSimUri != NULL)
            {
               /* Simualated/Destination device IP address */
               dummy = 0;
               res = iptGetAddrByName(pPubCB->pSimUri, &simDeviceIp, &dummy);
               if (res == TDC_OK)
               {
                  dummy = 0;
                  res = iptGetAddrByNameSim(pDestUri,
                                            simDeviceIp,
                                            &destIp,
                                            &dummy);
               }
            }
            else
            {
               simDeviceIp = IPTCom_getOwnIpAddr();
               dummy = 0;
               res = iptGetAddrByName(pDestUri,
                                      &destIp,
                                      &dummy);
            }
   #else
            dummy = 0;
            res = iptGetAddrByName(pDestUri,
                                   &destIp,
                                   &dummy);
   #endif

            if (res != TDC_OK)
            {
               ret = res;
               IPTVosPrint1(IPT_ERR,
                            "Renewing publishing failed for ComId=%d\n", 
                            pPubCB->pdCB.comId);

               /* Publish netcontrol block created, i.e. sending to an IP address not valid anymore? */
               if (pPubCB->pSendNetCB)
               {
                  /* Remove from sending list */
                  pPubCB->pSendNetCB->nPublisher--;
                  if (pPubCB->pSendNetCB->nPublisher == 0)
                  {
                     IPTGLOBAL(pd.netCBchanged) = 1;
                  }
                  pPubCB->pSendNetCB = NULL;
               }
            }
            else
            {
               /* Destination IP address changed? */
               if (destIp != pPubCB->destIp)
               {
                  /* See if we already have a netbuffer CB for this comid+destination */
   #ifdef TARGET_SIMU
                  pSendNetCB = pdSendNetCB_find(pPubCB->pdCB.comId, destIp, simDeviceIp);
   #else
                  pSendNetCB = pdSendNetCB_find(pPubCB->pdCB.comId, destIp);
   #endif   
                  if (pSendNetCB == NULL)
                  {
                     /* Create control block */
                     pSendNetCB = (PD_SEND_NET_CB *) IPTVosMalloc(sizeof(PD_SEND_NET_CB));
                     if (pSendNetCB == NULL)
                     {
                        /* Could not create memory for the CB */
                        IPTVosPrint2(IPT_ERR,
                                     "Renewing publishing failed for ComId=%d. Could not allocate memory size=%d\n",
                                     pPubCB->pdCB.comId, sizeof(PD_SEND_NET_CB));
                        ret = (int)IPT_ERROR;
                     }
                     else
                     {
                        /* Create data buffer area */
                        pSendNetCB->pSendBuffer = IPTVosMalloc(sizeof(PD_HEADER) + pPubCB->netDatabufferSize);
                        if (pSendNetCB->pSendBuffer == NULL)
                        {
                           IPTVosPrint2(IPT_ERR,
                                        "Renewing publishing failed for ComId=%d. Could not allocate memory size=%d\n",
                                        pPubCB->pdCB.comId, sizeof(PD_HEADER) + pPubCB->netDatabufferSize);
                           IPTVosFree((BYTE *) pSendNetCB);
                           ret = (int)IPT_ERROR;
                        }
                        else
                        {
                           /* Initialize */
                           pSendNetCB->pNextToSend = NULL;
                           pSendNetCB->comId = pPubCB->pdCB.comId;
                           pSendNetCB->nPublisher = 0;
                           pSendNetCB->status = CB_ADDED;
                           pSendNetCB->pdOutPackets = 0;
                           pSendNetCB->destIp = destIp;
#ifdef TARGET_SIMU
                           pSendNetCB->simDeviceIp = simDeviceIp;              /* Simulated source ip address (!=0 is used) */
#endif

                           /* Previous publish netcontrol block created? */
                           if (pPubCB->pSendNetCB)
                           {
                              pSendNetCB->cycleMultiple = pPubCB->pSendNetCB->cycleMultiple;
#if defined(IF_WAIT_ENABLE)
                              pSendNetCB->comParId = pPubCB->pSendNetCB->comParId;
#endif
                              pSendNetCB->pdSendSocket = pPubCB->pSendNetCB->pdSendSocket;
                              pSendNetCB->redFuncId = pPubCB->pSendNetCB->redFuncId;
                              pSendNetCB->leader = pPubCB->pSendNetCB->leader; 
                              pSendNetCB->updatedOnceNs = pPubCB->pSendNetCB->updatedOnceNs;
                              pSendNetCB->size = pPubCB->pSendNetCB->size;

                              /* Copy data from old buffer */
                              memcpy(pSendNetCB->pSendBuffer, pPubCB->pSendNetCB->pSendBuffer, pPubCB->pSendNetCB->size);
                          
                              /* Decrease number of publisher for the old net control block */
                              pPubCB->pSendNetCB->nPublisher--;
                              if (pPubCB->pSendNetCB->nPublisher == 0)
                              {
                                 IPTGLOBAL(pd.netCBchanged) = 1;
                              }
                           }
                           else
                           {
                              pHeader = (PD_HEADER *) pSendNetCB->pSendBuffer;       /*lint !e826 Size is OK but described in framelen */
                              
                              if (!exchgParCollected)
                              {
                                 /* Get parameters for this comid from the configuration database */
                                 ret = iptConfigGetExchgPar(pPubCB->pdCB.comId, &exchgPar);
                                 if (ret != (int)IPT_OK)
                                 {
                                    IPTVosPrint1(IPT_ERR, 
                                          "Renewing publishing failed for ComId=%d. ComId not configured\n",
                                                 pPubCB->pdCB.comId);
                                 }
                              }

                              if (ret == IPT_OK)
                              {
                                 ret = iptConfigGetComPar(exchgPar.comParId, &configComPar);
                                 if (ret != (int)IPT_OK)
                                 {
                                    /* Could not create a new netbuffer CB */
                                    IPTVosPrint2(IPT_ERR,
                                                 "Renewing publishing failed for ComId=%d. No communication parameter configured for comParId=%d\n",
                                                 pPubCB->pdCB.comId, exchgPar.comParId);
                                    IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
                                    IPTVosFree((BYTE *)pSendNetCB);
                                 }
                                 else
                                 {
                                    pSendNetCB->cycleMultiple = exchgPar.pdSendPar.cycleTime/IPTGLOBAL(task.pdProcCycle);
                                    if (pSendNetCB->cycleMultiple == 0)
                                    {
                                       pSendNetCB->cycleMultiple = 1;
                                       IPTVosPrint3(IPT_WARN,
                                               "The cycle time=%d for comId=%d is less than the PD send thread cycle time=%d\n", 
                                                    exchgPar.pdSendPar.cycleTime, pPubCB->pdCB.comId, IPTGLOBAL(task.pdProcCycle));
                                    }
   #if defined(IF_WAIT_ENABLE)
                                    pSendNetCB->comParId = exchgPar.comParId;
   #endif
                                    pSendNetCB->pdSendSocket = configComPar.pdSendSocket;
                                    pSendNetCB->redFuncId = exchgPar.pdSendPar.redundant;

                                    /* Redundance is used for this comid? */
                                    if (exchgPar.pdSendPar.redundant != 0)
                                    {
                                       /* Search for the redundant iD */
                                       pRedIdItem = (PD_RED_ID_ITEM *)(void*)iptTabFind(&IPTGLOBAL(pd.redIdTableHdr),
                                                                                        pSendNetCB->redFuncId);/*lint !e826  Ignore casting warning */
                                       if (pRedIdItem == NULL)
                                       {
                                          redIdItem.redId = pSendNetCB->redFuncId;
            
                                          /* Use the device redundant mode */   
                                          redIdItem.leader = IPTGLOBAL(pd.leaderDev);
                                          pSendNetCB->leader = IPTGLOBAL(pd.leaderDev);

                                          /* Add the redundancy function reference to the queue listeners table */
                                          res = iptTabAdd(&IPTGLOBAL(pd.redIdTableHdr), (IPT_TAB_ITEM_HDR *)((void *)&redIdItem));
                                          if (res != IPT_OK)
                                          {
                                             IPTVosPrint3(IPT_ERR,
                                                "Renewing publishing failed for ComId=%d. Failed to add redId=%d to table. Error=%#x\n",
                                                pPubCB->pdCB.comId, redIdItem.redId, res);
                                             IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
                                             IPTVosFree((BYTE *) pSendNetCB);
                                             ret = (int)IPT_ERROR;
                                          }
                                       }
                                       else
                                       {
                                          /* Use the mode set for the redundant Id */
                                          pSendNetCB->leader = pRedIdItem->leader;   
                                       }
                                    }
                                    else
                                    {
                                       /* Redundance is not used for this comid.
                                          Set the initiate state as leader, i.e. send data */
                                       pSendNetCB->leader = TRUE;
                                    }

                                    if (pPubCB->defStart)
                                    {
                                       pSendNetCB->size = 0;
                                       pSendNetCB->updatedOnceNs = FALSE;
                                    }
                                    else
                                    {
                                       /* Set to true to indicate that the buffer has been updated at least
                                        onces, i.e. sending of data will be started immediately.
                                       This is done here of compability reason  */
                                       pSendNetCB->updatedOnceNs = TRUE;

                                       /* Prepare the head in case of not deferred start */
                                       datasetSize = pPubCB->pdCB.size;
                                       ret = iptMarshallDSF(pPubCB->nLines, pPubCB->alignment, pPubCB->disableMarshalling,
                                                            pPubCB->pDatasetFormat,
                                                            pPubCB->pdCB.pDataBuffer, temp,
                                                            &datasetSize);
                                       if (ret != (int)IPT_OK)
                                       {
                                          IPTVosPrint1(IPT_ERR,
                                                       "Renewing publishing failed for ComId=%d. Marshalling of zero data failed\n",
                                                       pPubCB->pdCB.comId);
                                          IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
                                          IPTVosFree((BYTE *)pSendNetCB);
                                       }
                                       else
                                       {
                                          /* Load dataset */
                                          datasetSizeFCS = pPubCB->netDatabufferSize;
                                          ret = iptLoadSendData(temp, datasetSize, pSendNetCB->pSendBuffer + sizeof(PD_HEADER), 
                                                                &datasetSizeFCS);
                                          if (ret != (int)IPT_OK)
                                          {
                                             IPTVosPrint1(IPT_ERR,
                                                          "Renewing publishing failed for ComId=%d. Marshalling of zero data failed\n",
                                                          pPubCB->pdCB.comId);
                                             IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
                                             IPTVosFree((BYTE *)pSendNetCB);
                                          }
                                          else
                                          {
                                             pHeader->datasetLength = TOWIRE16(datasetSize);
                                             pSendNetCB->size = sizeof(PD_HEADER) + datasetSizeFCS;
                                          }
                                       }
                                    }
                                
                                    if (ret == IPT_OK)
                                    {
                                       /* fill the header with fixed values */
                                       pHeader->protocolVersion = TOWIRE32(PD_PROTOCOL_VERSION);
                                       pHeader->comId = TOWIRE32(pSendNetCB->comId);
                                       pHeader->type = TOWIRE16(PD_TYPE); 
                                       pHeader->reserved = 0;
                                       pHeader->headerLength = TOWIRE16(sizeof(PD_HEADER) - FCS_SIZE);
                                    }
                                 }
                              }
                           }

                           if (ret == IPT_OK)
                           {
                              /* Add the newly created comid CB first in linked list for netbuffer */
                              pSendNetCB->pNext = IPTGLOBAL(pd.pFirstSendNetCB);
                              IPTGLOBAL(pd.pFirstSendNetCB) = pSendNetCB;
                              IPTGLOBAL(pd.netCBchanged) = 1;
                           }
                        }
                     }
                  }

                  if ((ret == IPT_OK)&& (pSendNetCB))
                  {
                     pPubCB->destIp = destIp;
                     pPubCB->pSendNetCB = pSendNetCB;              /* Link to netbuffer CB */
                     pPubCB->pNetDatabuffer = pSendNetCB->pSendBuffer + sizeof(PD_HEADER); /* Link to netbuffers data buffer */
                     pPubCB->pPdHeader =  (PD_HEADER *)pSendNetCB->pSendBuffer;  /*lint !e826 Size is OK but described in framelen */
                     pPubCB->pSendNetCB->nPublisher++;
                  
                  
                     /* Find corresponding schedule group ix */
                     if (!pdPubGrpTabFind(pPubCB->schedGrp, &schedIx))
                     {
                        if (!pdPubGrpTabAdd(pPubCB->schedGrp, &schedIx))
                        {
                           /* Illegal schedGrp */
                           ret = (int)IPT_ERROR;
                  }
               }

                     if (ret == IPT_OK)
                     {
                        pSchedGrpTab = &IPTGLOBAL(pd.pubSchedGrpTab);
                        pGrpPubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
                        while ((pGrpPubCB != NULL) && (pGrpPubCB != pPubCB))
                        {
                           pGrpPubCB = pGrpPubCB->pNext;
            }
                
                        /* Not already in the group? */
                        if (pGrpPubCB == NULL)
                        {
                           /* Add to the schedular group */
                           if (pSchedGrpTab->pTable[schedIx].pFirstCB != NULL)
                           {
                              pPubCB->pNext = pSchedGrpTab->pTable[schedIx].pFirstCB;
                              pPubCB->pNext->pPrev = pPubCB;
         }
                           else
                           {
                              pPubCB->pNext = NULL;
      }
                           pSchedGrpTab->pTable[schedIx].pFirstCB = pPubCB;
   }
                     }
   
                  }
               }
            }
         }
      }
   }
   
   return(ret);
}

/*******************************************************************************
NAME     : pdSub_renew
ABSTRACT : Renew subscription if any IP address has been changed
RETURNS  : 0 if ok else != 0
*/
int pdSub_renew(
   PD_SUB_CB *pSubCB)   /* Pointer to subsribe control block */
{
   char * pNextSourceUri;
   char   srcUri[MAX_TOKLEN+1];
   const char * pSourceUri = NULL;
   const char * pDestUri = NULL;
   char *pFiltUri;
   int i;
   int ret = (int)IPT_OK;
   int exchgParCollected = 0;
   int multicast = 0;
   int schedIx;
   int ipChanged = 0;
   UINT8 dummy;
   UINT8 noOfNetCB;
   UINT32 destIp;
   UINT32 sourceIp;
#ifdef TARGET_SIMU
   UINT32 simDeviceIp = 0;       /* Destination device IP address (simulation) */
#endif
   IPT_CONFIG_EXCHG_PAR_EXT exchgPar;
   IPT_CFG_DATASET_INT *pDataset;
   PD_REC_NET_CB *pRecNetCB;
   T_TDC_RESULT res = TDC_OK;
   PD_SUB_CB *pGrpSubCB;
   SUBGRPTAB *pSchedGrpTab;

   if (!pSubCB->waitingTdc)
   {

      /* Data buffer not created ? */
      if (pSubCB->pdCB.pDataBuffer == NULL)
      {
         /* Get parameters for this comid from the configuration database */
         ret = iptConfigGetExchgPar(pSubCB->pdCB.comId, &exchgPar);
         if (ret != (int)IPT_OK)
         {               
            IPTVosPrint1(IPT_ERR, 
                  "Renewing subscription failed for ComId=%d. ComId not configured\n",
                         pSubCB->pdCB.comId);
         }
         /* Get parameters for this comid from the configuration database */
         else if ((pDataset = iptConfigGetDataset(exchgPar.datasetId)) == NULL)
         {
            IPTVosPrint2(IPT_ERR,
               "Renewing subscription failed for ComId=%d. Dataset ID=%d not configured\n",
                pSubCB->pdCB.comId, exchgPar.datasetId);
            ret = (int)IPT_NOT_FOUND;   /* Could not find dataset format */
         }
         else if (pDataset->size > PD_DATASET_MAXSIZE)
         {
            IPTVosPrint4(IPT_ERR,
                    "Renewing subscription failed for ComId=%d. Dataset size=%d greater than PD max=%d for dataset ID=%d\n", 
                         pSubCB->pdCB.comId, pDataset->size, PD_DATASET_MAXSIZE, exchgPar.datasetId);
            ret = (int)IPT_ILLEGAL_SIZE;
         }
         else if (pDataset->size == 0)
         {
            IPTVosPrint2(IPT_ERR,
                    "Renewing subscription failed for ComId=%d. Dynamic dataset size for dataset ID=%d\n",
                    pSubCB->pdCB.comId, exchgPar.datasetId);
            ret = (int)IPT_ILLEGAL_SIZE;
         }
         else
         {
            exchgParCollected = 1;
            
            /* Dataset contains size of complete data in dataset */
            pSubCB->pdCB.size = (UINT16) pDataset->size;      

            /* Create data buffer area */
            pSubCB->pdCB.pDataBuffer = IPTVosMalloc(pSubCB->pdCB.size);
            if (pSubCB->pdCB.pDataBuffer == NULL)
            {
               IPTVosPrint2(IPT_ERR, "Renewing subscription failed for ComId=%d. Could not allocate memory size=%d\n",
                            pSubCB->pdCB.comId, pSubCB->pdCB.size);
               ret =  (int)IPT_MEM_ERROR;   /* Could not create memory for the data buffer */
            }
            else
            {
               memset(pSubCB->pdCB.pDataBuffer, 0, pSubCB->pdCB.size);
            }
         }
      }
   
#ifdef TARGET_SIMU
      if (ret == (int)IPT_OK)
      {
         if (pSubCB->pSimUri != NULL)
         {
            /* Simualated/Destination device IP address */
            dummy = 0;
            res = iptGetAddrByName(pSubCB->pSimUri, &simDeviceIp, &dummy);
            if (res != TDC_OK)
            {
               IPTVosPrint1(IPT_ERR,
                            "Renewing subscription failed for ComId=%d\n", 
                            pSubCB->pdCB.comId);
               ret = res;
            }
         }
         else
         {
            simDeviceIp = IPTCom_getOwnIpAddr();
         }
      }
#endif
      if (ret == (int)IPT_OK)
      {
         if (pSubCB->destId != 0)
         {
            ret = iptConfigGetDestIdPar(pSubCB->pdCB.comId, pSubCB->destId, &pDestUri);
            if (ret != (int)IPT_OK)
            {
               IPTVosPrint2(IPT_ERR,
                            "Renewing subscription failed for ComId=%d. Destination Id=%d not configured\n",
                            pSubCB->pdCB.comId, pSubCB->destId);
            }
         }
         else if (pSubCB->pDestUri)
         {
            pDestUri =  (char *)pSubCB->pDestUri;
         }
         else
         {
            if (!exchgParCollected)
            {
               ret = iptConfigGetExchgPar(pSubCB->pdCB.comId, &exchgPar);
               if (ret != (int)IPT_OK)
               {               
                  IPTVosPrint1(IPT_ERR, 
                        "Renewing subscription failed for ComId=%d. ComId not configured\n",
                               pSubCB->pdCB.comId);
               }
               else
               {
                  exchgParCollected = 1;
               }
            }
         
            if (ret == IPT_OK)
            {
               pDestUri =  exchgPar.pdSendPar.pDestinationURI;/*lint !e644 exchgPar is initialized here */
            }
         }

         if (ret == (int)IPT_OK)
         {
            if (pDestUri != NULL)
            {
               dummy = 0;
#ifdef TARGET_SIMU
               if (pSubCB->pSimUri != NULL)
               {
                  res = iptGetAddrByNameSim(pDestUri,
                                            simDeviceIp,
                                            &destIp,
                                            &dummy);
               }
               else
               {
                  res = iptGetAddrByName(pDestUri,
                                         &destIp,
                                         &dummy);
               }
#else
               res = iptGetAddrByName(pDestUri,
                                      &destIp,
                                      &dummy);
#endif
               if (res != TDC_OK)
               {
                  ret = res;
                  IPTVosPrint1(IPT_ERR,
                               "Renewing subscription failed for ComId=%d\n", 
                               pSubCB->pdCB.comId);
               }

               if (ret == (int)IPT_OK)
               {
                  if (isMulticastIpAddr(destIp))
                  {
                     multicast = 1;
                  }
                  if (pSubCB->destIp != destIp)
                  {
                      ipChanged = 1;

                      /* If destination IP is a multicastgroup, join the group */
                     if (isMulticastIpAddr(pSubCB->destIp))
                     {
                        leavePDmulticastAddress(
#ifdef TARGET_SIMU
                                               simDeviceIp, /* Simualted IP address */
#endif
                                               pSubCB->destIp);  /* multicast IP address */
                     }
                      /* If destination IP is a multicastgroup, join the group */
                     if (multicast)
                     {
                        joinPDmulticastAddress(
#ifdef TARGET_SIMU
                                               simDeviceIp, /* Simualted IP address */
#endif
                                               destIp);  /* multicast IP address */
                     }

                     pSubCB->destIp = destIp;
                  }
               }
            }
         }
      }

      if (ret == (int)IPT_OK)
      {               
         if (pSubCB->filterId)
         {
            ret = iptConfigGetPdSrcFiltPar(pSubCB->pdCB.comId, pSubCB->filterId, &pSourceUri);
            if (ret != (int)IPT_OK)
            {        
               IPTVosPrint1(IPT_ERR, 
                     "Renewing subscription failed for ComId=%d. ComId not configured\n",
                            pSubCB->pdCB.comId);
            }
         }
         else if (pSubCB->pSourceUri)
         {
            pSourceUri = pSubCB->pSourceUri; 
         }
         else
         {
            if (!exchgParCollected)
            {
               ret = iptConfigGetExchgPar(pSubCB->pdCB.comId, &exchgPar);
               if (ret != (int)IPT_OK)
               {               
                  IPTVosPrint1(IPT_ERR, 
                        "Renewing subscription failed for ComId=%d. ComId not configured\n",
                               pSubCB->pdCB.comId);
               }
            }

            if (ret == IPT_OK)
            {
               pSourceUri = exchgPar.pdRecPar.pSourceURI; 
            }
         }
      }
     
      if (ret == (int)IPT_OK)
      {
         /* Any source filter? */
         if (pSourceUri != NULL)
         {
            noOfNetCB = 0;
            strncpy(srcUri, pSourceUri, MAX_TOKLEN);
            pFiltUri = srcUri;
            do
            {
               ret = IPT_OK;
               pNextSourceUri = strchr(pFiltUri,',');
               if (pNextSourceUri != NULL)
               {
                  /* terminate string */
                  pNextSourceUri[0] = 0;
                  pNextSourceUri++;  
               }
               dummy = 0;
#ifdef TARGET_SIMU
               if (pSubCB->pSimUri != NULL)
               {
                  res = iptGetAddrByNameSim(pFiltUri,
                                            simDeviceIp,
                                            &sourceIp,
                                            &dummy);
               }
               else
               {
                  res = iptGetAddrByName(pFiltUri,
                                         &sourceIp,
                                         &dummy);
               }
#else
               res = iptGetAddrByName(pFiltUri,
                                      &sourceIp,
                                      &dummy);
#endif
               if (res != TDC_OK)
               {
                  IPTVosPrint1(IPT_ERR,
                               "Renewing subscription failed for ComId=%d\n", 
                               pSubCB->pdCB.comId);
                  ret = res;
                  break;
               }
                /* If destination IP is a multicastgroup, join the group */
               if (multicast)
               {
                  /* Source ip = local device? */
                  if (sourceIp == 0x7f000001)
                  {
                     /* Change the source address to the own IP address as it will 
                        be the source IP address for multicast messages */  
                     sourceIp = IPTCom_getOwnIpAddr();
                  
                     /* Own IP address not found? */
                     if (sourceIp == 0)
                     {
                        ret = (int)IPT_ERR_NO_IPADDR;
                        IPTVosPrint1(IPT_ERR,
                                     "Renewing subscription failed for ComId=%d. No own IP address\n",
                                     pSubCB->pdCB.comId);
                        break;
                     }
                  }
               }

               if (   (noOfNetCB < MAX_SRC_FILTER)
                   && (   (pSubCB->pRecNetCB[noOfNetCB] == NULL)
                       || (sourceIp != pSubCB->pRecNetCB[noOfNetCB]->sourceIp)))
               {
                  ipChanged = 1;
#ifdef TARGET_SIMU
                  pRecNetCB = pdRecNetCB_get(pSubCB->pdCB.comId, sourceIp, simDeviceIp);
#else
                  pRecNetCB = pdRecNetCB_get(pSubCB->pdCB.comId, sourceIp);
#endif         
                  if (pRecNetCB == NULL)
                  {
                     IPTVosPrint1(IPT_ERR,
                                  "Renewing subscription failed for ComId=%d. Could not create net control block\n",
                                  pSubCB->pdCB.comId);
                     ret = (int)IPT_ERROR;
                     break;
                  }
                  else
                  {
                     if (pSubCB->pRecNetCB[noOfNetCB] != NULL)
                     {
                        pSubCB->pRecNetCB[noOfNetCB]->nSubscriber--;
                        if (pSubCB->pRecNetCB[noOfNetCB]->nSubscriber == 0)
                        {
                           pdRecNetCB_destroy(pSubCB->pRecNetCB[noOfNetCB]);
                        }
                     }
               
                     pSubCB->pRecNetCB[noOfNetCB] = pRecNetCB;
                     /* Link to netbuffers data buffer */
                     pSubCB->pNetbuffer[noOfNetCB] = pRecNetCB->pDataBuffer; 
                     pRecNetCB->nSubscriber++;
                  
                  }
               }
   
               noOfNetCB++;
               pFiltUri = pNextSourceUri;
            }
            while(pNextSourceUri != NULL );
            
            
            if (ret == IPT_OK)
            {
               pSubCB->noOfNetCB = noOfNetCB;

               if (ipChanged)
               {
                  /* Find corresponding schedule group ix */
                  if (!pdSubGrpTabFind(pSubCB->schedGrp, &schedIx))
                  {
                     if (!pdSubGrpTabAdd(pSubCB->schedGrp, &schedIx))
                     {
                        IPTVosPrint1(IPT_ERR, 
                                     "Subscription failed for ComId=%d. Could not create schedular group\n",
                                     pSubCB->pdCB.comId);
                        /* Illegal schedGrp */
                        ret = (int)IPT_ERROR;
            }
                  }

                  if (ret == IPT_OK)
                  {
                     pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);
                     pGrpSubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
                     while ((pGrpSubCB != NULL) && (pGrpSubCB != pSubCB))
                     {
                        pGrpSubCB = pGrpSubCB->pNext;
                     }
             
                     /* Not already in the group? */
                     if (pGrpSubCB == NULL)
                     {
                        /* Add to the schedular group */
                        if (pSchedGrpTab->pTable[schedIx].pFirstCB != NULL)
                        {
                           pSubCB->pNext = pSchedGrpTab->pTable[schedIx].pFirstCB;
                           pSubCB->pNext->pPrev = pSubCB;
                        }
            else
            {
                           pSubCB->pNext = NULL;
                        }
                        pSchedGrpTab->pTable[schedIx].pFirstCB = pSubCB;
                     }
                  }
               }
            }
            else
            {
               for (i = 0; i < MAX_SRC_FILTER; i++)
               {
                  if (pSubCB->pRecNetCB[i])
                  {
                     pSubCB->pRecNetCB[i]->nSubscriber--;
                     if (pSubCB->pRecNetCB[i]->nSubscriber == 0)
                     {
                        pdRecNetCB_destroy(pSubCB->pRecNetCB[i]);
                     }
                     pSubCB->pRecNetCB[i] = NULL;
                  }
               }
               
               pSubCB->noOfNetCB = 0;
            }
         }
      }
   }

   return ret;
}

/*******************************************************************************
NAME     : pdRecNetCB_destroy
ABSTRACT : Destroy receive control block
           Remove from receive table
           Leave multicast address if it is not used anymore
RETURNS  : -
*/
void pdRecNetCB_destroy(
   PD_REC_NET_CB *pRecNetCB)  /* Pointer to control block to destroy */
{
  
   /* Remove this comid + sourceIp from the receive table */
#ifdef TARGET_SIMU
   pdRecTabDelete(pRecNetCB->comId, pRecNetCB->simDeviceIp, pRecNetCB->sourceIp);
#else
   pdRecTabDelete(pRecNetCB->comId, pRecNetCB->sourceIp);
#endif 
   if (pRecNetCB->pNext != NULL)
      pRecNetCB->pNext->pPrev = pRecNetCB->pPrev;

   if (pRecNetCB->pPrev != NULL)
   {
      pRecNetCB->pPrev->pNext = pRecNetCB->pNext;
   }
   else
   {
      IPTGLOBAL(pd.pFirstRecNetCB) = pRecNetCB->pNext;  
   }

  
   IPTVosFree((BYTE *) pRecNetCB->pDataBuffer);
   IPTVosFree((BYTE *) pRecNetCB);

}

/*******************************************************************************
NAME     : pdComidPubCB_cleanup
ABSTRACT : Remove a publish control block if there is no more use for it
RETURNS  : -
*/
void pdComidPubCB_cleanup(
   PD_PUB_CB *pPubCB)  /* Pointer to the control block */
{
   
   int schedIx;
   PUBGRPTAB *pSchedGrpTab;
   PD_NOT_RESOLVED_PUB_CB *pNotResolvedPubCB;
   PD_NOT_RESOLVED_PUB_CB *pNextNotResolvedPubCB;

   /* Net control block connected? */
   if (pPubCB->pSendNetCB)
   {
      pPubCB->pSendNetCB->nPublisher--;
      if (pPubCB->pSendNetCB->nPublisher == 0)
      {
         IPTGLOBAL(pd.netCBchanged) = 1;
      }
   }

   if (pPubCB->pNext != NULL)
      pPubCB->pNext->pPrev = pPubCB->pPrev;


   if (pPubCB->pPrev != NULL)
   {
      pPubCB->pPrev->pNext = pPubCB->pNext;
   }
   else
   {
      pSchedGrpTab = &IPTGLOBAL(pd.pubSchedGrpTab);
      if ((pdPubGrpTabFind(pPubCB->schedGrp, &schedIx)) &&
          (pSchedGrpTab->pTable[schedIx].pFirstCB == pPubCB))
      {
         pSchedGrpTab->pTable[schedIx].pFirstCB = pPubCB->pNext;  
   
         if (pSchedGrpTab->pTable[schedIx].pFirstCB == NULL)
         {
            pdPubGrpTabDelete(pSchedGrpTab, pPubCB->schedGrp);
         }
      }
      else
      {
         pNotResolvedPubCB = IPTGLOBAL(pd.pFirstNotResPubCB); 
         while (pNotResolvedPubCB != NULL)
         {
            pNextNotResolvedPubCB = pNotResolvedPubCB->pNext;
            if (pNotResolvedPubCB->pPubCB == pPubCB)
            {
               if (pNotResolvedPubCB->pNext != NULL)
                  pNotResolvedPubCB->pNext->pPrev = pNotResolvedPubCB->pPrev;

               if (pNotResolvedPubCB->pPrev != NULL)
               {
                  pNotResolvedPubCB->pPrev->pNext = pNotResolvedPubCB->pNext;
               }
               else
               {
                  IPTGLOBAL(pd.pFirstNotResPubCB) = pNotResolvedPubCB->pNext;  
               }

               IPTVosFree((BYTE *) pNotResolvedPubCB);            
            }
            pNotResolvedPubCB = pNextNotResolvedPubCB; 
         }
      }
   }

   /* buffer  for overide destination URI? */
   if (pPubCB->pDestUri != NULL )
   {
      IPTVosFree((BYTE *) pPubCB->pDestUri);
   } 
#ifdef TARGET_SIMU               
   if (pPubCB->pSimUri != NULL )
   {
      IPTVosFree((BYTE *) pPubCB->pSimUri);
   } 
#endif               
   
   if (pPubCB->pdCB.pDataBuffer != NULL)
   {
      IPTVosFree((BYTE *) pPubCB->pdCB.pDataBuffer);
   }
 
   IPTVosFree((BYTE *) pPubCB);
}

/*******************************************************************************
NAME     : pdComidSubCB_cleanup
ABSTRACT : Remove a subscribe control block if there is no more use for it
RETURNS  : -
*/
void pdComidSubCB_cleanup(
   PD_SUB_CB *pSubCB)  /* Pointer to the control block */
{
   int i;
   int schedIx;
   SUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);
   PD_NOT_RESOLVED_SUB_CB *pNotResolvedSubCB, *pNotResolvedSubCBnext;

   for (i = 0; i < pSubCB->noOfNetCB; i++)
   {
      if (pSubCB->pRecNetCB[i])
      {
         pSubCB->pRecNetCB[i]->nSubscriber--;
      }
   }
   
   if (pSubCB->pNext != NULL)
      pSubCB->pNext->pPrev = pSubCB->pPrev;

   if (pSubCB->pPrev != NULL)
   {
      pSubCB->pPrev->pNext = pSubCB->pNext;
   }
   else
   {
      if ((pdSubGrpTabFind(pSubCB->schedGrp, &schedIx)) &&
          (pSchedGrpTab->pTable[schedIx].pFirstCB == pSubCB))
      {
         pSchedGrpTab->pTable[schedIx].pFirstCB = pSubCB->pNext;  
   
         if (pSchedGrpTab->pTable[schedIx].pFirstCB == NULL)
         {
            pdSubGrpTabDelete(pSchedGrpTab, pSubCB->schedGrp);
         }
      }
      else
      {
         pNotResolvedSubCB = IPTGLOBAL(pd.pFirstNotResSubCB); 
         while (pNotResolvedSubCB != NULL)
         {
            pNotResolvedSubCBnext = pNotResolvedSubCB->pNext;
            if (pNotResolvedSubCB->pSubCB == pSubCB)
            {
               if (pNotResolvedSubCB->pNext != NULL)
                  pNotResolvedSubCB->pNext->pPrev = pNotResolvedSubCB->pPrev;


               if (pNotResolvedSubCB->pPrev != NULL)
               {
                  pNotResolvedSubCB->pPrev->pNext = pNotResolvedSubCB->pNext;
               }
               else
               {
                  IPTGLOBAL(pd.pFirstNotResSubCB) = pNotResolvedSubCB->pNext;  
               }
            
               IPTVosFree((BYTE *) pNotResolvedSubCB);
            
            }
      
            pNotResolvedSubCB = pNotResolvedSubCBnext; 
         }
      }
   }

   /* Check if we can remove it from the netbuffer too */
   for (i = 0; i < pSubCB->noOfNetCB; i++)
   {
      if ((pSubCB->pRecNetCB[i]) && (pSubCB->pRecNetCB[i]->nSubscriber == 0))
      {
         pdRecNetCB_destroy(pSubCB->pRecNetCB[i]);
      }
   }
   
    /* If destination IP is a multicastgroup, join the group */
   if (isMulticastIpAddr(pSubCB->destIp))
   {
      leavePDmulticastAddress(
#ifdef TARGET_SIMU
                              pSubCB->simDeviceIp, /* Simualted IP address */
#endif
                              pSubCB->destIp);  /* IP address */
   }
   
   if (pSubCB->pDestUri != NULL )
   {
      IPTVosFree((BYTE *) pSubCB->pDestUri);
   } 
   if (pSubCB->pSourceUri != NULL )
   {
      IPTVosFree((BYTE *) pSubCB->pSourceUri);
   } 
#ifdef TARGET_SIMU
   if (pSubCB->pSimUri != NULL )
   {
      IPTVosFree((BYTE *) pSubCB->pSimUri);
   } 
#endif               
   
   if (pSubCB->pdCB.pDataBuffer != NULL)
   {
      IPTVosFree((BYTE *) pSubCB->pdCB.pDataBuffer);
   }
   IPTVosFree((BYTE *) pSubCB);
}

/*******************************************************************************
NAME     : PD_sub_renew_all
ABSTRACT : Renew all subscriptions in all schedule groups 
RETURNS  : 0 if no error, != 0 if any of the renews failed
*/
int PD_sub_renew_all(void)
{
   	UINT16 			i;
   	SUBGRPTAB 		*pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);
	PD_SUB_CB 		*pSubCB;
	int 			ret = 0;

    ret = IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER);
    if (ret == IPT_OK)
    {   
        for (i = 0; i < pSchedGrpTab->nItems ; i++)
        {
            pSubCB = pSchedGrpTab->pTable[i].pFirstCB;

            while (pSubCB != NULL)
            {
            	/* rejoin MC group if necessary */
            	if (isMulticastIpAddr(pSubCB->destIp))
                {
                	ret += pdSub_renew(pSubCB); 
                }
                pSubCB = pSubCB->pNext;
            }
        }

        if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
        {
            IPTVosPrint0(IPT_ERR, "PD_sub_renew_all: IPTVosPutSem(recSem) ERROR\n");
        }
    }
    else
    {
      	IPTVosPrint0(IPT_ERR, "PD_sub_renew_all: IPTVosGetSem(recSem) ERROR\n");
    }

	return ret;
}
