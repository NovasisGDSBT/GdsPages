/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2012 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     :  IPTrain
 *
 *  MODULE      :  mdlistener.c
 *
 *  ABSTRACT    :  Message data listener
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 *  $Id: mdlistener.c 11859 2012-04-18 16:01:04Z gweiss $
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *       findings from TUEV-Assessment
 *
 *  CR-62 (Bernd Loehr, 2010-08-25)
 * 			Additional function MD_renew_mc_listeners() to renew all MC
 * 			memberships after link down/up event
 *
 *  Internal (Bernd Loehr, 2010-08-13) 
 * 			Old obsolete CVS history removed
 *
 ******************************************************************************/


/*******************************************************************************
* INCLUDES */

#if defined(VXWORKS)
#include <vxWorks.h>
#endif

#include <string.h>

#include "iptcom.h"     /* Common type definitions for IPT */
#include "vos.h"
#include "mdcom_priv.h"
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"
#include "mdses.h"

/*******************************************************************************
* DEFINES */


/*******************************************************************************
* TYPEDEFS */


/*******************************************************************************
* GLOBALS */

/*******************************************************************************
* LOCALS */

/*******************************************************************************
* LOCAL FUNCTIONS */

/*******************************************************************************
NAME:       mdQueueTabFind
ABSTRACT:   Find item in a table based on a keys
RETURNS:    Pointer to item, NULL if not found
*/
static LISTENER_QUEUE *mdQueueTabFind(
   MD_QUEUE        listenerQueueId,  /* Listeners queue Id */
   const void      *pCallerRef)      /* Listeners caller reference */
{
   LISTENER_QUEUE *pItem;
 
   pItem = IPTGLOBAL(md.pFirstListenerQueue);
   
   while (pItem)
   {
      if ((pItem->listenerQueueId == listenerQueueId) &&
          (pItem->pCallerRef ==  pCallerRef))
      {
         return(pItem);
      }
      pItem = pItem->pNext;
   }
   return((LISTENER_QUEUE *)NULL);
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:       mdFuncTabFind
ABSTRACT:   Find item in a table based on a keys
RETURNS:    Pointer to item, NULL if not found
*/
static LISTENER_FUNC *mdFuncTabFind(
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef)      /* Listeners caller reference */
{
   LISTENER_FUNC *pItem;
 
   pItem = IPTGLOBAL(md.pFirstListenerFunc);
   
   while (pItem)
   {
      if ((pItem->func == func) &&
          (pItem->pCallerRef ==  pCallerRef))
      {
         return(pItem);
      }
      pItem = pItem->pNext;
   }
   return((LISTENER_FUNC *)NULL);
}
#endif

/*******************************************************************************
NAME:       mdQueueTabAdd
ABSTRACT:   Adds new item to the queue look up table
RETURNS:    -
*/
static LISTENER_QUEUE * mdQueueTabAdd(
   MD_QUEUE        listenerQueueId,  /* Listeners queue Id */
   const void      *pCallerRef)      /* Listeners caller reference */
{
   LISTENER_QUEUE *pNewItem;
   LISTENER_QUEUE *pStoredItem;

   pStoredItem = mdQueueTabFind(listenerQueueId, pCallerRef);

   /* Not stored */
   if (!pStoredItem)
   {
      pNewItem = (LISTENER_QUEUE *)(IPTVosMalloc(sizeof(LISTENER_QUEUE)));
      if (!pNewItem)
      {
         IPTVosPrint1(IPT_ERR,
                  "Could not allocate memory size=%d\n", sizeof(LISTENER_QUEUE));
         return (LISTENER_QUEUE *)NULL;
      }

      pNewItem->pNext = IPTGLOBAL(md.pFirstListenerQueue);
      pNewItem->pPrev = NULL;
      pNewItem->listenerQueueId = listenerQueueId;
      pNewItem->pCallerRef = pCallerRef;
      pNewItem->noOfListener = 1;
      pNewItem->lastRecMsgNo = 0;
      if (IPTGLOBAL(md.pFirstListenerQueue))
      {
         IPTGLOBAL(md.pFirstListenerQueue)->pPrev = pNewItem;  
      }
      IPTGLOBAL(md.pFirstListenerQueue) = pNewItem;
      return(pNewItem);
   }
   else
   {
      pStoredItem->noOfListener++;
      return(pStoredItem);
   }
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:       mdFuncTabAdd
ABSTRACT:   Adds new item to the queue look up table
RETURNS:    -
*/
static LISTENER_FUNC * mdFuncTabAdd(
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef)      /* Listeners caller reference */
{
   LISTENER_FUNC *pNewItem;
   LISTENER_FUNC *pStoredItem;

   pStoredItem = mdFuncTabFind(func, pCallerRef);

   /* Not stored */
   if (!pStoredItem)
   {
      pNewItem = (LISTENER_FUNC *)(IPTVosMalloc(sizeof(LISTENER_FUNC)));
      if (!pNewItem)
      {
         IPTVosPrint1(IPT_ERR,
                  "Could not allocate memory size=%d\n", sizeof(LISTENER_FUNC));
         return (LISTENER_FUNC *)NULL;
      }

      pNewItem->pNext = IPTGLOBAL(md.pFirstListenerFunc);
      pNewItem->pPrev = NULL;
      pNewItem->func = func;
      pNewItem->pCallerRef = pCallerRef;
      pNewItem->noOfListener = 1;
      pNewItem->lastRecMsgNo = 0;
      if (IPTGLOBAL(md.pFirstListenerFunc))
      {
         IPTGLOBAL(md.pFirstListenerFunc)->pPrev = pNewItem;  
      }
      IPTGLOBAL(md.pFirstListenerFunc) = pNewItem;
      return(pNewItem);
   }
   else
   {
      pStoredItem->noOfListener++;
      return(pStoredItem);
   }
}
#endif

/*******************************************************************************
NAME:     anyListenerOnQueue 
ABSTRACT: Check if there is any queue listeners for received ComId or user URI
RETURNS:  0 = not found, 1 = found.
RETURNS:  0 if OK, !=0 if not.
*/
static int anyListenerOnQueue(
   MD_QUEUE queue,   /* Queue  */
   LISTERNER_TABLES *pListTables)
{
   UINT32 i;
   COMID_ITEM       *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   QUEUE_LIST       *pQlistener;
   QUEUE_FRG_LIST   *pQFrglistener;

   pComIdItem = (COMID_ITEM*)((void*)pListTables->comidListTableHdr.pTable);
   if (pComIdItem)
   {
      for (i=0; i<pListTables->comidListTableHdr.nItems; i++)
      {
         pQlistener = pComIdItem[i].lists.pQueueList;
         while(pQlistener)
         {
            if (pQlistener->pQueue->listenerQueueId == queue)
            {
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return(1);
            }
            pQlistener = pQlistener->pNext; 
         }

         pQFrglistener = pComIdItem[i].lists.pQueueFrgList;
         while(pQFrglistener)
         {
            if (pQFrglistener->pQueue->listenerQueueId == queue)
            {
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return(1);
            }
            pQFrglistener = pQFrglistener->pNext; 
         }
      }
   }
   
   pQlistener = pListTables->aInstAfunc.pQueueList;
   while(pQlistener)
   {
      if (pQlistener->pQueue->listenerQueueId == queue)
      {
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
         return(1);
      }
      pQlistener = pQlistener->pNext; 
   }

   pQFrglistener = pListTables->aInstAfunc.pQueueFrgList;
   while(pQFrglistener)
   {
      if (pQFrglistener->pQueue->listenerQueueId == queue)
      {
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
         return(1);
      }
      pQFrglistener = pQFrglistener->pNext; 
   }
  
   pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
   if (pInstXFuncN)
   {
      for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
      {
         pQlistener = pInstXFuncN[i].lists.pQueueList;
         while(pQlistener)
         {
            if (pQlistener->pQueue->listenerQueueId == queue)
            {
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return(1);
            }
            pQlistener = pQlistener->pNext; 
         }

         pQFrglistener = pInstXFuncN[i].lists.pQueueFrgList;
         while(pQFrglistener)
         {
            if (pQFrglistener->pQueue->listenerQueueId == queue)
            {
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return(1);
            }
            pQFrglistener = pQFrglistener->pNext; 
         }
      }
   }

   pAinstFuncN = (AINST_FUNCN_ITEM *)pListTables->aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
   if (pAinstFuncN)
   {
      for (i = 0; i < pListTables->aInstFuncNListTableHdr.nItems ; i++)
      {
         pQlistener = pAinstFuncN[i].lists.pQueueList;
         while(pQlistener)
         {
            if (pQlistener->pQueue->listenerQueueId == queue)
            {
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return(1);
            }
            pQlistener = pQlistener->pNext; 
         }

         pQFrglistener = pAinstFuncN[i].lists.pQueueFrgList;
         while(pQFrglistener)
         {
            if (pQFrglistener->pQueue->listenerQueueId == queue)
            {
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return(1);
            }
            pQFrglistener = pQFrglistener->pNext; 
         }
      }
   }

   pInstXaFunc = (INSTX_AFUNC_ITEM *)pListTables->instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
   if (pInstXaFunc)
   {
      for (i = 0; i < pListTables->instXaFuncListTableHdr.nItems ; i++)
      {
         pQlistener = pInstXaFunc[i].lists.pQueueList;
         while(pQlistener)
         {
            if (pQlistener->pQueue->listenerQueueId == queue)
            {
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return(1);
            }
            pQlistener = pQlistener->pNext; 
         }

         pQFrglistener = pInstXaFunc[i].lists.pQueueFrgList;
         while(pQFrglistener)
         {
            if (pQFrglistener->pQueue->listenerQueueId == queue)
            {
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return(1);
            }
            pQFrglistener = pQFrglistener->pNext; 
         }
      }
   }
   
   return(0);
}

/*******************************************************************************
NAME:     searchQueueListener
ABSTRACT: Check if a queue listener already exists
RETURNS:  1 = alredy exists 0 = not exist
*/
static int searchQueueListener(
   QUEUE_LIST      *pQlistener,
   MD_QUEUE        listenerQueueId, /* Queue ID */
   const void      *pCallerRef,     /* Caller reference */
   UINT32          destIpAddr,
   UINT32          comId,           /* Communication ID */
   UINT32          destId,          /* Destination URI ID */
   const char      *pDestUri)       /* Pointer to destination URI */
{
   int alreadyAdded = 0;
  
   while (pQlistener)
   {
      if ((pQlistener->pQueue->listenerQueueId == listenerQueueId) &&
          (pQlistener->pQueue->pCallerRef == pCallerRef))
      {
         if (pQlistener->destIpAddr == destIpAddr)
         {
            if (destIpAddr == IP_ADDRESS_NOT_RESOLVED)
            {
               if (pDestUri)
               {
                  if (pQlistener->pDestUri)
                  {
                     if(iptStrcmp(pDestUri, pQlistener->pDestUri) == 0)
                     {
                        alreadyAdded = 1;
                        break;
                     }
                  }
               }
               else if ((pQlistener->destId == destId) &&
                   (pQlistener->comId == comId))
               {
                  alreadyAdded = 1;
                  break;
               }
            }
            else
            {
               alreadyAdded = 1;
               break;
            }
         }
      }

      pQlistener = pQlistener->pNext;
   }   
   return(alreadyAdded);
}

/*******************************************************************************
NAME:     searchFrgQueueListener
ABSTRACT: Check if a FRG queue listener already exists
RETURNS:  1 = alredy exists 0 = not exist
*/
static int searchFrgQueueListener(
   QUEUE_FRG_LIST *pQFrglistener,
   MD_QUEUE        listenerQueueId, /* Queue ID */
   const void      *pCallerRef,     /* Caller reference */
   const void      *pRedFuncRef,    /* Redundancy function reference */
   UINT32          destIpAddr,
   UINT32          comId,           /* Communication ID */
   UINT32          destId,          /* Destination URI ID */
   const char      *pDestUri)       /* Pointer to destination URI */
{
   int alreadyAdded = 0;
  
   while (pQFrglistener)
   {
      if ((pQFrglistener->pQueue->listenerQueueId == listenerQueueId) &&
          (pQFrglistener->pQueue->pCallerRef == pCallerRef) &&
          (pQFrglistener->pRedFuncRef == pRedFuncRef))
      {
         if (pQFrglistener->destIpAddr == destIpAddr)
         {
            if (destIpAddr == IP_ADDRESS_NOT_RESOLVED)
            {
               if (pDestUri)
               {
                  if (pQFrglistener->pDestUri)
                  {
                     if(iptStrcmp(pDestUri, pQFrglistener->pDestUri) == 0)
                     {
                        alreadyAdded = 1;
                        break;
                     }
                  }
               }
               else if ((pQFrglistener->destId == destId) &&
                   (pQFrglistener->comId == comId))
               {
                  alreadyAdded = 1;
                  break;
               }
            }
            else
            {
               alreadyAdded = 1;
               break;
            }
         }
      }

      pQFrglistener = pQFrglistener->pNext;
   }   
   return(alreadyAdded);
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     searchFuncListener
ABSTRACT: Check if a call-back function listener already exists
RETURNS:  1 = alredy exists 0 = not exist
*/
static int searchFuncListener(
   FUNC_LIST      *pFlistener,
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef,     /* Caller reference */
   UINT32          destIpAddr,
   UINT32          comId,           /* Communication ID */
   UINT32          destId,          /* Destination URI ID */
   const char      *pDestUri)       /* Pointer to destination URI */
{
   int alreadyAdded = 0;
  
   while (pFlistener)
   {
      if ((pFlistener->pFunc->func == func) &&
          (pFlistener->pFunc->pCallerRef == pCallerRef))
      {
         if (pFlistener->destIpAddr == destIpAddr)
         {
            if (destIpAddr == IP_ADDRESS_NOT_RESOLVED)
            {
               if (pDestUri)
               {
                  if (pFlistener->pDestUri)
                  {
                     if(iptStrcmp(pDestUri, pFlistener->pDestUri) == 0)
                     {
                        alreadyAdded = 1;
                        break;
                     }
                  }
               }
               else if ((pFlistener->destId == destId) &&
                   (pFlistener->comId == comId))
               {
                  alreadyAdded = 1;
                  break;
               }
            }
            else
            {
               alreadyAdded = 1;
               break;
            }
         }
      }

      pFlistener = pFlistener->pNext;
   }   
   return(alreadyAdded);
}

/*******************************************************************************
NAME:     searchFrgFuncListener
ABSTRACT: Check if a FRG call-back function listener listener already exists
RETURNS:  1 = alredy exists 0 = not exist
*/
static int searchFrgFuncListener(
   FUNC_FRG_LIST      *pFFrglistener,
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef,     /* Caller reference */
   const void      *pRedFuncRef,    /* Redundancy function reference */
   UINT32          destIpAddr,
   UINT32          comId,           /* Communication ID */
   UINT32          destId,          /* Destination URI ID */
   const char      *pDestUri)       /* Pointer to destination URI */
{
   int alreadyAdded = 0;
  
   while (pFFrglistener)
   {
      if ((pFFrglistener->pFunc->func == func) &&
          (pFFrglistener->pFunc->pCallerRef == pCallerRef) &&
          (pFFrglistener->pRedFuncRef == pRedFuncRef))
      {
         if (pFFrglistener->destIpAddr == destIpAddr)
         {
            if (destIpAddr == IP_ADDRESS_NOT_RESOLVED)
            {
               if (pDestUri)
               {
                  if (pFFrglistener->pDestUri)
                  {
                     if(iptStrcmp(pDestUri, pFFrglistener->pDestUri) == 0)
                     {
                        alreadyAdded = 1;
                        break;
                     }
                  }
               }
               else if ((pFFrglistener->destId == destId) &&
                   (pFFrglistener->comId == comId))
               {
                  alreadyAdded = 1;
                  break;
               }
            }
            else
            {
               alreadyAdded = 1;
               break;
            }
         }
      }

      pFFrglistener = pFFrglistener->pNext;
   }   
   return(alreadyAdded);
}
#endif

/*******************************************************************************
NAME:     getFrgFrgItem
ABSTRACT: Search or create a function redundancy group item. 
          Listener semaphore has to be taken before the call
RETURNS:  Pointer to the state signal used for the actual redundancy function
          reference 
*/
static FRG_ITEM *getFrgFrgItem(
   const void   *pRedFuncRef)    /* Redundancy function reference */
{
   int res;
   FRG_ITEM *pFrgItem;
   FRG_ITEM frgItem;
   
   pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                              (UINT32)pRedFuncRef));/*lint !e826  Ignore casting warning */
   if (pFrgItem == 0)
   {
      frgItem.pRedFuncRef = pRedFuncRef;
      frgItem.frgState = FRG_FOLLOWER;
      frgItem.noOfListeners = 0;
     
      res = iptTabAdd(&IPTGLOBAL(md.frgTableHdr), (IPT_TAB_ITEM_HDR *)((void *)&frgItem));
      if (res != IPT_OK)
      {
            IPTVosPrint1(IPT_ERR,
               "getFrgFrgItem: Failed to add FRG to table. Error=%#x\n",
               res);
      }
      else
      {
         pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                       (UINT32)pRedFuncRef));
      }
   }

   return(pFrgItem);
}


/*******************************************************************************
NAME:     joinMDmulticastAddress
ABSTRACT: Join a not already join multicast address. 
          Listener semaphore has to be taken before the call
RETURNS:  - 
*/
static void joinMDmulticastAddress(
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
   pTabItem = (NET_JOINED_MC *)((void *)iptTab2Find(&IPTGLOBAL(net.mdJoinedMcAddrTable),
                                                    ipAddress, simuIpAddr));
#else
   pTabItem = (NET_JOINED_MC *)((void *)iptTabFind(&IPTGLOBAL(net.mdJoinedMcAddrTable),
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
      ctrlMsg.ctrl = JOIN_MD_MULTICAST;
      ctrlMsg.multiCastAddr = (long int)ipAddress;
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
      res = IPTDriveJoinMDMultiCast(ipAddress, simuIpAddr);
#else
      res = IPTDriveJoinMDMultiCast(ipAddress);
#endif
      if (res != (int)IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "ERROR join multicast address\n");
      }
#endif
      else
      {
         tableItem.multiCastAddr = ipAddress;
         tableItem.noOfJoiners = 1;
#ifdef TARGET_SIMU
         tableItem.simuIpAddr = simuIpAddr;
         res = iptTab2Add(&IPTGLOBAL(net.mdJoinedMcAddrTable),
                         (IPT_TAB2_ITEM_HDR *)((void *)&tableItem));
#else
         res = iptTabAdd(&IPTGLOBAL(net.mdJoinedMcAddrTable),
                         (IPT_TAB_ITEM_HDR *)((void *)&tableItem));
#endif
         if (res != (int)IPT_OK)
         {
            IPTVosPrint0(IPT_ERR,
                         "joinMDmulticastAddress failed to add items to table\n");
         }
      }
   }
}

/*******************************************************************************
NAME:     leaveMDmulticastAddress
ABSTRACT: Join a not already join multicast address. 
          Listener semaphore has to be taken before the call
RETURNS:  - 
*/
static void leaveMDmulticastAddress(
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
      pTabItem = (NET_JOINED_MC *)((void *)iptTab2Find(&IPTGLOBAL(net.mdJoinedMcAddrTable),
                                                       ipAddress, simuIpAddr));
#else
      pTabItem = (NET_JOINED_MC *)((void *)iptTabFind(&IPTGLOBAL(net.mdJoinedMcAddrTable),
                                                       ipAddress));/*lint !e826  Ignore casting warning */
#endif
      if (pTabItem)
      {
         pTabItem->noOfJoiners--;
         if (pTabItem->noOfJoiners == 0)
         {
#ifdef LINUX_MULTIPROC
            ctrlMsg.ctrl = LEAVE_MD_MULTICAST;
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
            res = IPTDriveLeaveMDMultiCast(ipAddress, simuIpAddr);
#else
            res = IPTDriveLeaveMDMultiCast(ipAddress);
#endif
            if (res != (int)IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "ERROR leave multicast address\n");
            }
#endif
            else
            {
#ifdef TARGET_SIMU
               res = iptTab2Delete(&IPTGLOBAL(net.mdJoinedMcAddrTable), ipAddress, simuIpAddr);
#else
               res = iptTabDelete(&IPTGLOBAL(net.mdJoinedMcAddrTable), ipAddress);
#endif
               if (res != (int)IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR,
                               "leaveMDmulticastAddress failed to remove items from table\n");
               } 
            }
         }
      }
   }
}

/*******************************************************************************
NAME:     getComIdDestIPAddr
ABSTRACT: Get destination IP address for a given comID
          Listener semaphore has to be taken before the call
RETURNS:  Destination IP address
          O = not found
*/
static int getComIdDestIPAddr(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
#endif
    UINT32 comId,       /* comID */
    UINT32 destId,      /* destination ID */
    const char   *pDestUri,   /* pointer to destination URI */
    UINT32 *pDestIp)    /* Pointer to destination IP address */
{
   UINT8 dummy;
   UINT32 destIpAddr;
   int res = IPT_OK;
   IPT_CONFIG_EXCHG_PAR_EXT exchgPar;

   if (destId)
   {
      res = iptConfigGetDestIdPar(comId, destId, &pDestUri);
   }
   else if (pDestUri == NULL)
   {
      res = iptConfigGetExchgPar(comId, &exchgPar);
      if (res == IPT_OK)
      {
         pDestUri = exchgPar.mdSendPar.pDestinationURI;   
      }
   }

   if (res == (int)IPT_OK)
   {
      if (pDestUri != NULL)
      {
         /* Get IP address from IPTDir based on the destination URI */
         dummy = 0;
#ifdef TARGET_SIMU
         if (simuIpAddr != IPTCom_getOwnIpAddr())
         {
            res = iptGetAddrByNameSim(pDestUri, simuIpAddr,
                                      &destIpAddr, &dummy);
         }
         else
         {
            res = iptGetAddrByName(pDestUri, &destIpAddr,
                                   &dummy);
         }
#else
         res = iptGetAddrByName(pDestUri, &destIpAddr,
                                &dummy);
#endif
         if (res == TDC_OK)
         {
            *pDestIp = destIpAddr;
            if (isMulticastIpAddr(destIpAddr))
            {
#if defined(IF_WAIT_ENABLE)
               if (IPTGLOBAL(ifRecReadyMD))
               {
                  /* join multicast address */
                  joinMDmulticastAddress(
      #ifdef TARGET_SIMU
                                         simuIpAddr,
      #endif
                                         destIpAddr);
               }
               else
               {
                  /* Indicate that joining has to be done when ethernet interface is ready */
                  IPTGLOBAL(md.finish_addr_resolv) = 1;
                  *pDestIp = IP_ADDRESS_NOT_RESOLVED;
               }
#else
               /* join multicast address */
               joinMDmulticastAddress(
   #ifdef TARGET_SIMU
                                      simuIpAddr,
   #endif
                                      destIpAddr);
#endif
            }
         }
         else
         {
            if ((res == TDC_NO_CONFIG_DATA) || (res == TDC_MUST_FINISH_INIT))
            {
               res = IPT_OK;

               /* Indicate that address resolving has to be done later when TDC 
                  has got data from IPTDir */
               IPTGLOBAL(md.finish_addr_resolv) = 1;

               /* Exit wait for TDC to be ready*/
               *pDestIp = IP_ADDRESS_NOT_RESOLVED;
            }
            else
            {
               IPTVosPrint3(IPT_ERR,
               "Could not convert ComId=%d destination URI=%s to IP address. TDC result=%#x\n",
                            comId, pDestUri, res);
               *pDestIp = 0;
            }
         }
      }
      else
      {
         res = (int)IPT_INVALID_PAR;
         *pDestIp = 0;
      }
   }
   else if (res == (int)IPT_TDC_NOT_READY)
   {
      res = IPT_OK;

      /* Indicate that address resolving has to be done later when TDC has
         got data from IPTDir */
      IPTGLOBAL(md.finish_addr_resolv) = 1;

      /* Exit wait for TDC to be ready*/
      *pDestIp = IP_ADDRESS_NOT_RESOLVED;
   }
   else
   {
      *pDestIp = 0;
   }

   return(res);
}


/*******************************************************************************
NAME:     removeQueueNormListener
ABSTRACT: Remove a queue listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeQueueNormListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
#endif
   MD_QUEUE     queue,       /* Queue identification of the queue to be removed 
                                from listener list*/
   int          all,
   const void   *pCallerRef, /* Caller reference */
   LISTENER_LISTS *pLists)
{
   int res;
   QUEUE_LIST     *pQlistener;
   QUEUE_LIST     *pPrevQlistener;

   pQlistener = pLists->pQueueList;
   pPrevQlistener = NULL;
   while (pQlistener)
   {
      if (  (pQlistener->pQueue->listenerQueueId == queue)
          &&(  (all) 
             ||(pQlistener->pQueue->pCallerRef == pCallerRef)))
      {
         leaveMDmulticastAddress(
   #ifdef TARGET_SIMU
                                 simuIpAddr,
   #endif
                                 pQlistener->destIpAddr);

         pQlistener->pQueue->noOfListener--;
         if (pQlistener->pQueue->noOfListener == 0)
         {
            if (pQlistener->pQueue->pPrev)
            {
               pQlistener->pQueue->pPrev->pNext = pQlistener->pQueue->pNext; 
            }
            else
            {
               IPTGLOBAL(md.pFirstListenerQueue) = pQlistener->pQueue->pNext;
            }
            if (pQlistener->pQueue->pNext)
            {
               pQlistener->pQueue->pNext->pPrev = pQlistener->pQueue->pPrev;
            }
            res = IPTVosFree((unsigned char *)pQlistener->pQueue);
            if(res != 0)
            {
              IPTVosPrint1(IPT_ERR,
                           "removeQueueNormListener failed to free data memory, res=%d\n",res);
            }
         }
 
         if (pPrevQlistener)
         {
            pPrevQlistener->pNext = pQlistener->pNext;
         }
         else
         {
            pLists->pQueueList = pQlistener->pNext;
         }
         res = IPTVosFree((unsigned char *)pQlistener);
         if(res != 0)
         {
           IPTVosPrint1(IPT_ERR,
                        "removeQueueNormListener failed to free data memory, res=%d\n",res);
         }

         if (pPrevQlistener)
         {
            pQlistener = pPrevQlistener->pNext;
         }
         else
         {
            pQlistener = pLists->pQueueList;
         }
          
      }
      else
      {
         pPrevQlistener = pQlistener;
         pQlistener = pQlistener->pNext;   
      }
   }
}

/*******************************************************************************
NAME:     removeQueueFrgListener
ABSTRACT: Remove a queue FRG listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeQueueFrgListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
#endif
   MD_QUEUE     queue,       /* Queue identification of the queue to be removed 
                                from listener list*/
   int          all,
   const void   *pCallerRef, /* Caller reference */
   LISTENER_LISTS *pLists)
{
   int res;
   QUEUE_FRG_LIST *pQFrglistener;
   QUEUE_FRG_LIST *pPrevQFrglistener;
   FRG_ITEM *pFrgItem;

   pQFrglistener = pLists->pQueueFrgList;
   pPrevQFrglistener = NULL;
   while (pQFrglistener)
   {
      if (  (pQFrglistener->pQueue->listenerQueueId == queue)
          &&(  (all) 
             ||(pQFrglistener->pQueue->pCallerRef == pCallerRef)))
      {
         leaveMDmulticastAddress(
   #ifdef TARGET_SIMU
                                 simuIpAddr,
   #endif
                                 pQFrglistener->destIpAddr);

         pQFrglistener->pQueue->noOfListener--;
         if (pQFrglistener->pQueue->noOfListener == 0)
         {
            if (pQFrglistener->pQueue->pPrev)
            {
               pQFrglistener->pQueue->pPrev->pNext = pQFrglistener->pQueue->pNext; 
            }
            else
            {
               IPTGLOBAL(md.pFirstListenerQueue) = pQFrglistener->pQueue->pNext;
            }
            if (pQFrglistener->pQueue->pNext)
            {
               pQFrglistener->pQueue->pNext->pPrev = pQFrglistener->pQueue->pPrev;
            }
            res = IPTVosFree((unsigned char *)pQFrglistener->pQueue);
            if(res != 0)
            {
              IPTVosPrint1(IPT_ERR,
                           "removeQueueFrgListener failed to free data memory, res=%d\n",res);
            }
         }
 
         pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                    (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */

         if (pFrgItem)
         {
            pFrgItem->noOfListeners--;
            if (pFrgItem->noOfListeners == 0)
            {
               res = iptTabDelete(&IPTGLOBAL(md.frgTableHdr),
                                  (UINT32)(pFrgItem->pRedFuncRef));
               if (res != IPT_OK)
               {
                  IPTVosPrint1(IPT_ERR,
                     "removeQueueFrgListener: Failed to delete FRG from table. Error=%#x\n",
                     res);
               }
            }   
         }
         
         if (pPrevQFrglistener)
         {
            pPrevQFrglistener->pNext = pQFrglistener->pNext;
         }
         else
         {
            pLists->pQueueFrgList = pQFrglistener->pNext;
         }
         res = IPTVosFree((unsigned char *)pQFrglistener);
         if(res != 0)
         {
           IPTVosPrint1(IPT_ERR,
                        "removeQueueFrgListener failed to free data memory, res=%d\n",res);
         }

         if (pPrevQFrglistener)
         {
            pQFrglistener = pPrevQFrglistener->pNext;
         }
         else
         {
            pQFrglistener = pLists->pQueueFrgList;
         }
          
      }
      else
      {
         pPrevQFrglistener = pQFrglistener;
         pQFrglistener = pQFrglistener->pNext;   
      }
   }
}

/*******************************************************************************
NAME:     removeQComIdListener
ABSTRACT: Remove a ComId queue listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeQComIdListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
   LISTERNER_TABLES *pListTables,
#endif
   MD_QUEUE     queue,       /* Queue identification of the queue to be removed 
                                from listener list*/
   int          all,
   const void   *pCallerRef, /* Caller reference */
   COMID_ITEM    *pComIdItem)
{
   int res;

#ifdef TARGET_SIMU
   removeQueueNormListener(simuIpAddr, queue, all, pCallerRef, &pComIdItem->lists);
   removeQueueFrgListener(simuIpAddr, queue, all, pCallerRef, &pComIdItem->lists);
#else
   removeQueueNormListener(queue, all, pCallerRef, &pComIdItem->lists);
   removeQueueFrgListener(queue, all, pCallerRef, &pComIdItem->lists);
#endif

  if ((pComIdItem->lists.pQueueList == NULL) &&
       (pComIdItem->lists.pQueueFrgList == NULL) &&
       (pComIdItem->lists.pFuncList == NULL) &&
       (pComIdItem->lists.pFuncFrgList == NULL))
   {
#ifdef TARGET_SIMU
      res = iptTabDelete(&pListTables->comidListTableHdr, pComIdItem->keyComId);
#else
      res = iptTabDelete(&IPTGLOBAL(md.listTables.comidListTableHdr), pComIdItem->keyComId);
#endif
      if (res != IPT_OK)
      {
         IPTVosPrint2(IPT_ERR,
                     "removeQComIdListener failed to delete listener for comId=%d, res=%d\n",
                     pComIdItem->keyComId, res);
      } 
   }
}

/*******************************************************************************
NAME:     removeQIxFnUriListener
ABSTRACT: Remove a ComId queue listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeQIxFnUriListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
   LISTERNER_TABLES *pListTables,
#endif
   MD_QUEUE     queue,       /* Queue identification of the queue to be removed 
                                from listener list*/
   int          all,
   const void   *pCallerRef, /* Caller reference */
   INSTX_FUNCN_ITEM *pInstXFuncN)
{
   int res;

#ifdef TARGET_SIMU
   removeQueueNormListener(simuIpAddr, queue, all, pCallerRef, &pInstXFuncN->lists);
   removeQueueFrgListener(simuIpAddr, queue, all, pCallerRef, &pInstXFuncN->lists);
#else
   removeQueueNormListener(queue, all, pCallerRef, &pInstXFuncN->lists);
   removeQueueFrgListener(queue, all, pCallerRef, &pInstXFuncN->lists);
#endif
  if ((pInstXFuncN->lists.pQueueList == NULL) &&
       (pInstXFuncN->lists.pQueueFrgList == NULL) &&
       (pInstXFuncN->lists.pFuncList == NULL) &&
       (pInstXFuncN->lists.pFuncFrgList == NULL))
   {
#ifdef TARGET_SIMU
      res = iptUriLabelTab2Delete(&pListTables->instXFuncNListTableHdr, pInstXFuncN->instName, pInstXFuncN->funcName);
#else
      res = iptUriLabelTab2Delete(&IPTGLOBAL(md.listTables.instXFuncNListTableHdr), pInstXFuncN->instName, pInstXFuncN->funcName);
#endif
      if (res != IPT_OK)
      {
         IPTVosPrint3(IPT_ERR,
                     "removeQIxFnUriListener failed to delete listener for URI=%s.%s, res=%d\n",
                     pInstXFuncN->instName, pInstXFuncN->funcName, res);
      } 
   }
}

/*******************************************************************************
NAME:     removeQaIFnUriListener
ABSTRACT: Remove a ComId queue listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeQaIFnUriListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
   LISTERNER_TABLES *pListTables,
#endif
   MD_QUEUE     queue,       /* Queue identification of the queue to be removed 
                                from listener list*/
   int          all,
   const void   *pCallerRef, /* Caller reference */
   AINST_FUNCN_ITEM *pAinstFuncN)
{
   int res;

#ifdef TARGET_SIMU
   removeQueueNormListener(simuIpAddr, queue, all, pCallerRef, &pAinstFuncN->lists);
   removeQueueFrgListener(simuIpAddr, queue, all, pCallerRef, &pAinstFuncN->lists);
#else
   removeQueueNormListener(queue, all, pCallerRef, &pAinstFuncN->lists);
   removeQueueFrgListener(queue, all, pCallerRef, &pAinstFuncN->lists);
#endif
  if ((pAinstFuncN->lists.pQueueList == NULL) &&
       (pAinstFuncN->lists.pQueueFrgList == NULL) &&
       (pAinstFuncN->lists.pFuncList == NULL) &&
       (pAinstFuncN->lists.pFuncFrgList == NULL))
   {
#ifdef TARGET_SIMU
      res = iptUriLabelTabDelete(&pListTables->aInstFuncNListTableHdr,  pAinstFuncN->funcName);
#else
      res = iptUriLabelTabDelete(&IPTGLOBAL(md.listTables.aInstFuncNListTableHdr),  pAinstFuncN->funcName);
#endif
      if (res != IPT_OK)
      {
         IPTVosPrint2(IPT_ERR,
                     "removeQIxFnUriListener failed to delete listener for URI=aInst.%s, res=%d\n",
                     pAinstFuncN->funcName, res);
      } 
   }
}

/*******************************************************************************
NAME:     removeQIxaFUriListener
ABSTRACT: Remove a ComId queue listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeQIxaFUriListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
   LISTERNER_TABLES *pListTables,
#endif
   MD_QUEUE     queue,       /* Queue identification of the queue to be removed 
                                from listener list*/
   int          all,
   const void   *pCallerRef, /* Caller reference */
   INSTX_AFUNC_ITEM *pInstXaFunc)
{
   int res;

#ifdef TARGET_SIMU
   removeQueueNormListener(simuIpAddr, queue, all, pCallerRef, &pInstXaFunc->lists);
   removeQueueFrgListener(simuIpAddr, queue, all, pCallerRef, &pInstXaFunc->lists);
#else
   removeQueueNormListener(queue, all, pCallerRef, &pInstXaFunc->lists);
   removeQueueFrgListener(queue, all, pCallerRef, &pInstXaFunc->lists);
#endif
  if ((pInstXaFunc->lists.pQueueList == NULL) &&
       (pInstXaFunc->lists.pQueueFrgList == NULL) &&
       (pInstXaFunc->lists.pFuncList == NULL) &&
       (pInstXaFunc->lists.pFuncFrgList == NULL))
   {
#ifdef TARGET_SIMU
      res = iptUriLabelTabDelete(&pListTables->instXaFuncListTableHdr, pInstXaFunc->instName);
#else
      res = iptUriLabelTabDelete(&IPTGLOBAL(md.listTables.instXaFuncListTableHdr), pInstXaFunc->instName);
#endif
      if (res != IPT_OK)
      {
         IPTVosPrint2(IPT_ERR,
                     "removeQIxFnUriListener failed to delete listener for URI=%s.aFunc, res=%d\n",
                     pInstXaFunc->instName, res);
      } 
   }
}

/*******************************************************************************
NAME:     removeFuncNormListener
ABSTRACT: Remove a func listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeFuncNormListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
#endif
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   int          all,
   const void   *pCallerRef, /* Caller reference */
   LISTENER_LISTS *pLists)
{
   int res;
   FUNC_LIST     *pFlistener;
   FUNC_LIST     *pPrevFlistener;

   pFlistener = pLists->pFuncList;
   pPrevFlistener = NULL;
   while (pFlistener)
   {
      if (  (pFlistener->pFunc->func == func)
          &&(  (all) 
             ||(pFlistener->pFunc->pCallerRef == pCallerRef)))
      {
         leaveMDmulticastAddress(
   #ifdef TARGET_SIMU
                                 simuIpAddr,
   #endif
                                 pFlistener->destIpAddr);

         pFlistener->pFunc->noOfListener--;
         if (pFlistener->pFunc->noOfListener == 0)
         {
            if (pFlistener->pFunc->pPrev)
            {
               pFlistener->pFunc->pPrev->pNext = pFlistener->pFunc->pNext; 
            }
            else
            {
               IPTGLOBAL(md.pFirstListenerFunc) = pFlistener->pFunc->pNext;
            }
            if (pFlistener->pFunc->pNext)
            {
               pFlistener->pFunc->pNext->pPrev = pFlistener->pFunc->pPrev;
            }
            res = IPTVosFree((unsigned char *)pFlistener->pFunc);
            if(res != 0)
            {
              IPTVosPrint1(IPT_ERR,
                           "removeFuncNormListener failed to free data memory, res=%d\n",res);
            }
         }
 
         if (pPrevFlistener)
         {
            pPrevFlistener->pNext = pFlistener->pNext;
         }
         else
         {
            pLists->pFuncList = pFlistener->pNext;
         }
         res = IPTVosFree((unsigned char *)pFlistener);
         if(res != 0)
         {
           IPTVosPrint1(IPT_ERR,
                        "removeFuncNormListener failed to free data memory, res=%d\n",res);
         }

         if (pPrevFlistener)
         {
            pFlistener = pPrevFlistener->pNext;
         }
         else
         {
            pFlistener = pLists->pFuncList;
         }
          
      }
      else
      {
         pPrevFlistener = pFlistener;
         pFlistener = pFlistener->pNext;   
      }
   }
}

/*******************************************************************************
NAME:     removeFuncFrgListener
ABSTRACT: Remove a func FRG listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeFuncFrgListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
#endif
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   int          all,
   const void   *pCallerRef, /* Caller reference */
   LISTENER_LISTS *pLists)
{
   int res;
   FUNC_FRG_LIST *pQFrglistener;
   FUNC_FRG_LIST *pPrevQFrglistener;
   FRG_ITEM *pFrgItem;

   pQFrglistener = pLists->pFuncFrgList;
   pPrevQFrglistener = NULL;
   while (pQFrglistener)
   {
      if (  (pQFrglistener->pFunc->func == func)
          &&(  (all) 
             ||(pQFrglistener->pFunc->pCallerRef == pCallerRef)))
      {
         leaveMDmulticastAddress(
   #ifdef TARGET_SIMU
                                 simuIpAddr,
   #endif
                                 pQFrglistener->destIpAddr);

         pQFrglistener->pFunc->noOfListener--;
         if (pQFrglistener->pFunc->noOfListener == 0)
         {
            if (pQFrglistener->pFunc->pPrev)
            {
               pQFrglistener->pFunc->pPrev->pNext = pQFrglistener->pFunc->pNext; 
            }
            else
            {
               IPTGLOBAL(md.pFirstListenerFunc) = pQFrglistener->pFunc->pNext;
            }
            if (pQFrglistener->pFunc->pNext)
            {
               pQFrglistener->pFunc->pNext->pPrev = pQFrglistener->pFunc->pPrev;
            }
            res = IPTVosFree((unsigned char *)pQFrglistener->pFunc);
            if(res != 0)
            {
              IPTVosPrint1(IPT_ERR,
                           "removeFuncFrgListener failed to free data memory, res=%d\n",res);
            }
         }
 
         pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                    (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
         if (pFrgItem)
         {
            pFrgItem->noOfListeners--;
            if (pFrgItem->noOfListeners == 0)
            {
               res = iptTabDelete(&IPTGLOBAL(md.frgTableHdr),
                                  (UINT32)(pFrgItem->pRedFuncRef));
               if (res != IPT_OK)
               {
                  IPTVosPrint1(IPT_ERR,
                     "deleteFrgRefTable: Failed to delete FRG from table. Error=%#x\n",
                     res);
               }
            }   
         }
         
         if (pPrevQFrglistener)
         {
            pPrevQFrglistener->pNext = pQFrglistener->pNext;
         }
         else
         {
            pLists->pFuncFrgList = pQFrglistener->pNext;
         }
         res = IPTVosFree((unsigned char *)pQFrglistener);
         if(res != 0)
         {
           IPTVosPrint1(IPT_ERR,
                        "removeFuncFrgListener failed to free data memory, res=%d\n",res);
         }

         if (pPrevQFrglistener)
         {
            pQFrglistener = pPrevQFrglistener->pNext;
         }
         else
         {
            pQFrglistener = pLists->pFuncFrgList;
         }
          
      }
      else
      {
         pPrevQFrglistener = pQFrglistener;
         pQFrglistener = pQFrglistener->pNext;   
      }
   }
}

/*******************************************************************************
NAME:     removeFComIdListener
ABSTRACT: Remove a func listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeFComIdListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
   LISTERNER_TABLES *pListTables,
#endif
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   int          all,
   const void   *pCallerRef, /* Caller reference */
   COMID_ITEM    *pComIdItem)
{
   int res;

#ifdef TARGET_SIMU
   removeFuncNormListener(simuIpAddr, func, all, pCallerRef, &pComIdItem->lists);
   removeFuncFrgListener(simuIpAddr, func, all, pCallerRef, &pComIdItem->lists);
#else
   removeFuncNormListener(func, all, pCallerRef, &pComIdItem->lists);
   removeFuncFrgListener(func, all, pCallerRef, &pComIdItem->lists);
#endif
  if ((pComIdItem->lists.pQueueList == NULL) &&
       (pComIdItem->lists.pQueueFrgList == NULL) &&
       (pComIdItem->lists.pFuncList == NULL) &&
       (pComIdItem->lists.pFuncFrgList == NULL))
   {
#ifdef TARGET_SIMU
      res = iptTabDelete(&pListTables->comidListTableHdr, pComIdItem->keyComId);
#else
      res = iptTabDelete(&IPTGLOBAL(md.listTables.comidListTableHdr), pComIdItem->keyComId);
#endif
      if (res != IPT_OK)
      {
         IPTVosPrint2(IPT_ERR,
                     "removeFComIdListener failed to delete listener for comId=%d, res=%d\n",
                     pComIdItem->keyComId, res);
      } 
   }
}

/*******************************************************************************
NAME:     removeFIxFnUriListener
ABSTRACT: Remove a ComId func listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeFIxFnUriListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
   LISTERNER_TABLES *pListTables,
#endif
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   int          all,
   const void   *pCallerRef, /* Caller reference */
   INSTX_FUNCN_ITEM *pInstXFuncN)
{
   int res;

#ifdef TARGET_SIMU
   removeFuncNormListener(simuIpAddr, func, all, pCallerRef, &pInstXFuncN->lists);
   removeFuncFrgListener(simuIpAddr, func, all, pCallerRef, &pInstXFuncN->lists);
#else
   removeFuncNormListener(func, all, pCallerRef, &pInstXFuncN->lists);
   removeFuncFrgListener(func, all, pCallerRef, &pInstXFuncN->lists);
#endif
  if ((pInstXFuncN->lists.pQueueList == NULL) &&
       (pInstXFuncN->lists.pQueueFrgList == NULL) &&
       (pInstXFuncN->lists.pFuncList == NULL) &&
       (pInstXFuncN->lists.pFuncFrgList == NULL))
   {
#ifdef TARGET_SIMU
      res = iptUriLabelTab2Delete(&pListTables->instXFuncNListTableHdr, pInstXFuncN->instName, pInstXFuncN->funcName);
#else
      res = iptUriLabelTab2Delete(&IPTGLOBAL(md.listTables.instXFuncNListTableHdr), pInstXFuncN->instName, pInstXFuncN->funcName);
#endif
      if (res != IPT_OK)
      {
         IPTVosPrint3(IPT_ERR,
                     "removeFIxFnUriListener failed to delete listener for URI=%s.%s, res=%d\n",
                     pInstXFuncN->instName, pInstXFuncN->funcName, res);
      } 
   }
}

/*******************************************************************************
NAME:     removeFaIFnUriListener
ABSTRACT: Remove a ComId func listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeFaIFnUriListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
   LISTERNER_TABLES *pListTables,
#endif
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   int          all,
   const void   *pCallerRef, /* Caller reference */
   AINST_FUNCN_ITEM *pAinstFuncN)
{
   int res;

#ifdef TARGET_SIMU
   removeFuncNormListener(simuIpAddr, func, all, pCallerRef, &pAinstFuncN->lists);
   removeFuncFrgListener(simuIpAddr, func, all, pCallerRef, &pAinstFuncN->lists);
#else
   removeFuncNormListener(func, all, pCallerRef, &pAinstFuncN->lists);
   removeFuncFrgListener(func, all, pCallerRef, &pAinstFuncN->lists);
#endif
  if ((pAinstFuncN->lists.pQueueList == NULL) &&
       (pAinstFuncN->lists.pQueueFrgList == NULL) &&
       (pAinstFuncN->lists.pFuncList == NULL) &&
       (pAinstFuncN->lists.pFuncFrgList == NULL))
   {
#ifdef TARGET_SIMU
      res = iptUriLabelTabDelete(&pListTables->aInstFuncNListTableHdr,  pAinstFuncN->funcName);
#else
      res = iptUriLabelTabDelete(&IPTGLOBAL(md.listTables.aInstFuncNListTableHdr),  pAinstFuncN->funcName);
#endif
      if (res != IPT_OK)
      {
         IPTVosPrint2(IPT_ERR,
                     "removeFIxFnUriListener failed to delete listener for URI=aInst.%s, res=%d\n",
                     pAinstFuncN->funcName, res);
      } 
   }
}

/*******************************************************************************
NAME:     removeFIxaFUriListener
ABSTRACT: Remove a ComId func listener. 
RETURNS:  0 if OK, !=0 if not.
*/
static void  removeFIxaFUriListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
   LISTERNER_TABLES *pListTables,
#endif
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   int          all,
   const void   *pCallerRef, /* Caller reference */
   INSTX_AFUNC_ITEM *pInstXaFunc)
{
   int res;

#ifdef TARGET_SIMU
   removeFuncNormListener(simuIpAddr, func, all, pCallerRef, &pInstXaFunc->lists);
   removeFuncFrgListener(simuIpAddr, func, all, pCallerRef, &pInstXaFunc->lists);
#else
   removeFuncNormListener(func, all, pCallerRef, &pInstXaFunc->lists);
   removeFuncFrgListener(func, all, pCallerRef, &pInstXaFunc->lists);
#endif
  if ((pInstXaFunc->lists.pQueueList == NULL) &&
       (pInstXaFunc->lists.pQueueFrgList == NULL) &&
       (pInstXaFunc->lists.pFuncList == NULL) &&
       (pInstXaFunc->lists.pFuncFrgList == NULL))
   {
#ifdef TARGET_SIMU
      res = iptUriLabelTabDelete(&pListTables->instXaFuncListTableHdr, pInstXaFunc->instName);
#else
      res = iptUriLabelTabDelete(&IPTGLOBAL(md.listTables.instXaFuncListTableHdr), pInstXaFunc->instName);
#endif
      if (res != IPT_OK)
      {
         IPTVosPrint2(IPT_ERR,
                     "removeFIxFnUriListener failed to delete listener for URI=%s.aFunc, res=%d\n",
                     pInstXaFunc->instName, res);
      } 
   }
}

/*******************************************************************************
NAME:     extractUriAndIP
ABSTRACT: Extract instance, function and host name from a URI string.
RETURNS:  0 if OK, !=0 if not.
*/
static int extractUriInstFuncHost(
   const char *pDestURI,       /* Pointer to destination URI string */
   char       *pUriInstName,   /* Pointer to instance URI string array with size IPT_MAX_LABEL_LEN */
   char       *pUriFuncName,   /* Pointer to function URI string array with size IPT_MAX_LABEL_LEN  */
   char       *pUriHostName)   /* Pointer to host URI string array with size IPT_MAX_HOST_URI_LEN */
{
   const char *pUriHost;
   const char *pUriFunc;
   const char *pTemp;
   int len;

   len = strlen(pDestURI);
   if (len == 0)
   {
      return((int)IPT_INVALID_PAR);
   }

   /* Ignore any ipt:// */
   pTemp = strrchr(pDestURI,'/');
   if (pTemp)
   {
      if (len > pTemp + 1 - pDestURI)
      {
         pDestURI = pTemp + 1;
         len = strlen(pDestURI); 
      }
      else
      {
         return((int)IPT_INVALID_PAR);
      }
   }

   /* Search for '@' */
   pUriHost = strchr(pDestURI,'@');
   if (pUriHost)
   {
      if (len > pUriHost + 1 - pDestURI)
      {
         if (strlen(pUriHost+1) < IPT_MAX_HOST_URI_LEN -1)
         {
            /* Copy URI host part */
            strcpy(pUriHostName,pUriHost+1);
         }
         else
         {
            return((int)IPT_INVALID_PAR);
         }
      }
      else
      {
         /* no host part after '@' */
         pUriHostName[0] = 0;
      }
      
      /* Search for dot delimiter */
      pUriFunc = strchr(pDestURI,'.');

      /* Dot found in URI host part or not found at all?
         I.e. no URI instance given */
      if ((pUriFunc == 0) || (pUriFunc > pUriHost))
      {
         if (((pUriHost - pDestURI) < IPT_MAX_LABEL_LEN) && ((pUriHost - pDestURI) > 0))
         {
            /* Copy URI function part */
            memcpy(pUriFuncName,pDestURI,pUriHost - pDestURI);

            /* Terminate the string */
            pUriFuncName[pUriHost - pDestURI] = 0;
            
            strcpy(pUriInstName,"aInst");
         }
         else
         {
            return((int)IPT_INVALID_PAR);
         }
      }
      else
      {
         /* Check that no extra dot is found before '@' */
         pTemp = strchr(pUriFunc +1,'.');
         if ((pTemp) && (pTemp < pUriHost))
         {
            return((int)IPT_INVALID_PAR);
         }
         
         if (((pUriFunc - pDestURI) < IPT_MAX_LABEL_LEN) &&
             ((pUriHost - pUriFunc - 1) < IPT_MAX_LABEL_LEN) &&
             ((pUriHost - pUriFunc - 1) > 0))
         {
            if ((pUriFunc - pDestURI) > 0)
            {
               /* Copy URI instance part */
               memcpy(pUriInstName,pDestURI,pUriFunc - pDestURI);

               /* Terminate the string */
               pUriInstName[pUriFunc - pDestURI] = 0;
            }
            else
            {
               strcpy(pUriInstName,"aInst");
            }
            
            /* Copy URI function part */
            memcpy(pUriFuncName,pUriFunc+1,pUriHost - pUriFunc - 1);

            /* Terminate the string */
            pUriFuncName[pUriHost - pUriFunc - 1] = 0;
         }
         else
         {
            return((int)IPT_INVALID_PAR);
         }
      }
   }
   /* No URI host part */
   else
   {
      pUriHostName[0] = 0;
   
      /* Search for dot delimiter */
      pUriFunc = strchr(pDestURI,'.');

         
      /* Dot found? I.e. URI instance part given */
      if (pUriFunc)
      {
         /* Any function name? */
         if ((pUriFunc + 1 - pDestURI) >= len)
         {
            return((int)IPT_INVALID_PAR);
         }

         /* Check that no extra dot is found  */
         pTemp = strchr(pUriFunc +1,'.');
         if (pTemp)
         {
            return((int)IPT_INVALID_PAR);
         }
         
         if (((pUriFunc - pDestURI) < IPT_MAX_LABEL_LEN) &&
             (strlen(pUriFunc+1) < IPT_MAX_LABEL_LEN))
         {
            /* Any instance name? */
            if ((pUriFunc - pDestURI) > 0)
            {
               /* Copy URI instance part */
               memcpy(pUriInstName,pDestURI,pUriFunc - pDestURI);

               /* Terminate the string */
               pUriInstName[pUriFunc - pDestURI] = 0;
            }
            else
            {
               strcpy(pUriInstName,"aInst");
            }
            
            /* Copy URI function part */
            strcpy(pUriFuncName,pUriFunc+1);
         }
         else
         {
            return((int)IPT_INVALID_PAR);
         }
      }
      /* Only URI function part */
      else
      {
         pUriFunc = (char *)pDestURI;
         if (len < IPT_MAX_LABEL_LEN)
         {
            /* Copy URI function part */
            strcpy(pUriFuncName,pUriFunc);

            strcpy(pUriInstName,"aInst");
         }
         else
         {
            return((int)IPT_INVALID_PAR);
         }
      }
   }
   return((int)IPT_OK);
}
      
/*******************************************************************************
NAME:     addListener
ABSTRACT: Add a listener for message data with the given comid's. Join 
          multicast groups for comid's with multicast destinations. 
          IPTCom will put received messages on the queue and or call-back function
RETURNS:  0 if OK, !=0 if not.
*/
static int addListener(
   MD_QUEUE        listenerQueueId, /* Queue ID */
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef,     /* Caller reference */
   const void      *pRedFuncRef,    /* Redundancy function reference */
   UINT32          destIpAddr,
   UINT32          comId,           /* Communication ID */
   UINT32          destId,          /* Destination URI ID */
   const char      *pDestUri,       /* Pointer to destination URI */
   LISTENER_LISTS  *pLists)
{
   int res = IPT_OK;
   int            alreadyAdded;
   LISTENER_QUEUE *pQueueItem;
   QUEUE_LIST     *pQlistener;
   QUEUE_FRG_LIST *pQFrglistener;
#ifndef LINUX_MULTIPROC
   LISTENER_FUNC  *pFuncItem;
   FUNC_LIST     *pFlistener;
   FUNC_FRG_LIST *pFFrglistener;
#endif
   FRG_ITEM *pFrgItem;

   if ((!listenerQueueId) &&(!func))
   {
      IPTVosPrint0(IPT_ERR,
                   "addListener: No queue or call-back function\n");
      return((int)IPT_INVALID_PAR);
   }

   if (listenerQueueId)
   {
      if (pRedFuncRef)
      {
         alreadyAdded = searchFrgQueueListener(pLists->pQueueFrgList,
                                               listenerQueueId,
                                               pCallerRef,
                                               pRedFuncRef,
                                               destIpAddr,
                                               comId,
                                               destId,
                                               pDestUri);

         if (!alreadyAdded)
         {
            pQFrglistener = (QUEUE_FRG_LIST *) IPTVosMalloc(sizeof(QUEUE_FRG_LIST));
            if (pQFrglistener == NULL)
            {
               IPTVosPrint1(IPT_ERR,
                            "Could not allocate memory size=%d\n",
                            sizeof(QUEUE_FRG_LIST));
               return IPT_MEM_ERROR;
            }
            else
            {
               pQFrglistener->pRedFuncRef = pRedFuncRef;
               pQFrglistener->comId = comId;
               pQFrglistener->destId = destId;
               pQFrglistener->destIpAddr = destIpAddr;
               pQFrglistener->pDestUri = NULL;
               pQFrglistener->pNext = pLists->pQueueFrgList;
               pLists->pQueueFrgList = pQFrglistener;
               if ((pDestUri) && (destId == 0))
               {
                  if (destIpAddr == IP_ADDRESS_NOT_RESOLVED)
                  {
                     /* Save overide destination URI parameter */
                     pQFrglistener->pDestUri = (char *) IPTVosMalloc(strlen(pDestUri) + 1);
                     if (pQFrglistener->pDestUri == NULL)
                     {
                        res = IPTVosFree((unsigned char *)pQFrglistener);
                        if(res != 0)
                        {
                          IPTVosPrint1(IPT_ERR,
                                       "Failed to free data memory, res=%d\n",res);
                        }
                        IPTVosPrint1(IPT_ERR,
                                     "Could not allocate memory size=%d\n",
                                     strlen(pDestUri) + 1);
                        return IPT_MEM_ERROR;
                     }
                     else
                     {
                        strcpy(pQFrglistener->pDestUri, pDestUri);
                     }
                  }
               }
            }
      
            pFrgItem = getFrgFrgItem(pRedFuncRef);
            if (pFrgItem == 0)
            {
               if (pQFrglistener->pDestUri)
               {
                  res = IPTVosFree((unsigned char *)pQFrglistener->pDestUri);
                  if(res != 0)
                  {
                    IPTVosPrint1(IPT_ERR,
                                 "Failed to free data memory, res=%d\n",res);
                  }
               }
               res = IPTVosFree((unsigned char *)pQFrglistener);
               if(res != 0)
               {
                 IPTVosPrint1(IPT_ERR,
                              "Failed to free data memory, res=%d\n",res);
               }
               return (int)IPT_MEM_ERROR;
            }
            else
            {
               pFrgItem->noOfListeners++;   
            }
         
            pQueueItem = mdQueueTabAdd(listenerQueueId, pCallerRef);
            if (pQueueItem == NULL)
            {
               if (pQFrglistener->pDestUri)
               {
                  res = IPTVosFree((unsigned char *)pQFrglistener->pDestUri);
                  if(res != 0)
                  {
                    IPTVosPrint1(IPT_ERR,
                                 "Failed to free data memory, res=%d\n",res);
                  }
               }
               res = IPTVosFree((unsigned char *)pQFrglistener);
               if(res != 0)
               {
                 IPTVosPrint1(IPT_ERR,
                              "Failed to free data memory, res=%d\n",res);
               }
               pFrgItem->noOfListeners--;   
               if (pFrgItem->noOfListeners == 0)
               {
                  res = iptTabDelete(&IPTGLOBAL(md.frgTableHdr),
                                     (UINT32)(pFrgItem->pRedFuncRef));
                  if (res != IPT_OK)
                  {
                     IPTVosPrint1(IPT_ERR,
                        "deleteFrgRefTable: Failed to delete FRG from table. Error=%#x\n",
                        res);
                  }
               }   
            
               return IPT_MEM_ERROR;
            }
            else
            {
               pQFrglistener->pQueue = pQueueItem; 
            }
         }
      }
      else
      {
         alreadyAdded = searchQueueListener(pLists->pQueueList,
                                            listenerQueueId,
                                            pCallerRef,
                                            destIpAddr,
                                            comId,
                                            destId,
                                            pDestUri);

         if (!alreadyAdded)
         {
            pQlistener = (QUEUE_LIST *) IPTVosMalloc(sizeof(QUEUE_LIST));
            if (pQlistener == NULL)
            {
               IPTVosPrint1(IPT_ERR,
                            "Could not allocate memory size=%d\n",
                            sizeof(QUEUE_LIST));
               return IPT_MEM_ERROR;
            }
            else
            {
               pQlistener->comId = comId;
               pQlistener->destId = destId;
               pQlistener->destIpAddr = destIpAddr;
               pQlistener->pDestUri = NULL;
               pQlistener->pNext = pLists->pQueueList;
               pLists->pQueueList = pQlistener;
               if ((pDestUri) && (destId == 0))
               {
                  if (destIpAddr == IP_ADDRESS_NOT_RESOLVED)
                  {
                     /* Save overide destination URI parameter */
                     pQlistener->pDestUri = (char *) IPTVosMalloc(strlen(pDestUri) + 1);
                     if (pQlistener->pDestUri == NULL)
                     {
                        res = IPTVosFree((unsigned char *)pQlistener);
                        if(res != 0)
                        {
                          IPTVosPrint1(IPT_ERR,
                                       "Failed to free data memory, res=%d\n",res);
                        }
                        IPTVosPrint1(IPT_ERR,
                                     "Could not allocate memory size=%d\n",
                                     strlen(pDestUri) + 1);
                        res = IPT_MEM_ERROR;
                     }
                     else
                     {
                        strcpy(pQlistener->pDestUri, pDestUri);
                     }
                  }
               }
            }
      
            pQueueItem = mdQueueTabAdd(listenerQueueId, pCallerRef);
            if (pQueueItem == NULL)
            {
               if (pQlistener->pDestUri)
               {
                  res = IPTVosFree((unsigned char *)pQlistener->pDestUri);
                  if(res != 0)
                  {
                    IPTVosPrint1(IPT_ERR,
                                 "Failed to free data memory, res=%d\n",res);
                  }
               }
               res = IPTVosFree((unsigned char *)pQlistener);
               if(res != 0)
               {
                 IPTVosPrint1(IPT_ERR,
                              "Failed to free data memory, res=%d\n",res);
               }
               return IPT_MEM_ERROR;
            }
            else
            {
               pQlistener->pQueue = pQueueItem; 
            }
         }
      }
   }

#ifndef LINUX_MULTIPROC
   if ((res == IPT_OK) && (func != 0))
   {
      if (pRedFuncRef)
      {
         alreadyAdded = searchFrgFuncListener(pLists->pFuncFrgList,
                                               func,
                                               pCallerRef,
                                               pRedFuncRef,
                                               destIpAddr,
                                               comId,
                                               destId,
                                               pDestUri);

         if (!alreadyAdded)
         {
            pFFrglistener = (FUNC_FRG_LIST *) IPTVosMalloc(sizeof(FUNC_FRG_LIST));
            if (pFFrglistener == NULL)
            {
               IPTVosPrint1(IPT_ERR,
                            "Could not allocate memory size=%d\n",
                            sizeof(FUNC_FRG_LIST));
               return IPT_MEM_ERROR;
            }
            else
            {
               pFFrglistener->pRedFuncRef = pRedFuncRef;
               pFFrglistener->comId = comId;
               pFFrglistener->destId = destId;
               pFFrglistener->destIpAddr = destIpAddr;
               pFFrglistener->pDestUri = NULL;
               pFFrglistener->pNext = pLists->pFuncFrgList;
               pLists->pFuncFrgList = pFFrglistener;
               if ((pDestUri) && (destId == 0))
               {
                  if (destIpAddr == IP_ADDRESS_NOT_RESOLVED)
                  {
                     /* Save overide destination URI parameter */
                     pFFrglistener->pDestUri = (char *) IPTVosMalloc(strlen(pDestUri) + 1);
                     if (pFFrglistener->pDestUri == NULL)
                     {
                        res = IPTVosFree((unsigned char *)pFFrglistener);
                        if(res != 0)
                        {
                          IPTVosPrint1(IPT_ERR,
                                       "Failed to free data memory, res=%d\n",res);
                        }
                        IPTVosPrint1(IPT_ERR,
                                     "Could not allocate memory size=%d\n",
                                     strlen(pDestUri) +1);
                        return IPT_MEM_ERROR;
                     }
                     else
                     {
                        strcpy(pFFrglistener->pDestUri,pDestUri);
                     }
                  }
               }
            }
      
            pFrgItem = getFrgFrgItem(pRedFuncRef);
            if (pFrgItem == 0)
            {
               if (pFFrglistener->pDestUri)
               {
                  res = IPTVosFree((unsigned char *)pFFrglistener->pDestUri);
                  if(res != 0)
                  {
                    IPTVosPrint1(IPT_ERR,
                                 "Failed to free data memory, res=%d\n",res);
                  }
               }
               res = IPTVosFree((unsigned char *)pFFrglistener);
               if(res != 0)
               {
                 IPTVosPrint1(IPT_ERR,
                              "Failed to free data memory, res=%d\n",res);
               }
               return (int)IPT_MEM_ERROR;
            }
            else
            {
               pFrgItem->noOfListeners++;   
            }
         
            pFuncItem = mdFuncTabAdd(func, pCallerRef);
            if (pFuncItem == NULL)
            {
               if (pFFrglistener->pDestUri)
               {
                  res = IPTVosFree((unsigned char *)pFFrglistener->pDestUri);
                  if(res != 0)
                  {
                    IPTVosPrint1(IPT_ERR,
                                 "Failed to free data memory, res=%d\n",res);
                  }
               }
               res = IPTVosFree((unsigned char *)pFFrglistener);
               if(res != 0)
               {
                 IPTVosPrint1(IPT_ERR,
                              "Failed to free data memory, res=%d\n",res);
               }
               pFrgItem->noOfListeners--;   
               if (pFrgItem->noOfListeners == 0)
               {
                  res = iptTabDelete(&IPTGLOBAL(md.frgTableHdr),
                                     (UINT32)(pFrgItem->pRedFuncRef));
                  if (res != IPT_OK)
                  {
                     IPTVosPrint1(IPT_ERR,
                        "deleteFrgRefTable: Failed to delete FRG from table. Error=%#x\n",
                        res);
                  }
               }   
            
               return IPT_MEM_ERROR;
            }
            else
            {
               pFFrglistener->pFunc = pFuncItem; 
            }
         }
      }
      else
      {
         alreadyAdded = searchFuncListener(pLists->pFuncList,
                                           func,
                                           pCallerRef,
                                           destIpAddr,
                                           comId,
                                           destId,
                                           pDestUri);

         if (!alreadyAdded)
         {
            pFlistener = (FUNC_LIST *) IPTVosMalloc(sizeof(FUNC_LIST));
            if (pFlistener == NULL)
            {
               IPTVosPrint1(IPT_ERR,
                            "Could not allocate memory size=%d\n",
                            sizeof(FUNC_LIST));
               return IPT_MEM_ERROR;
            }
            else
            {
               pFlistener->destId = comId;
               pFlistener->destId = destId;
               pFlistener->destIpAddr = destIpAddr;
               pFlistener->pDestUri = NULL;
               pFlistener->pNext = pLists->pFuncList;
               pLists->pFuncList = pFlistener;
               if ((pDestUri) && (destId == 0))
               {
                  if (destIpAddr == IP_ADDRESS_NOT_RESOLVED)
                  {
                     /* Save overide destination URI parameter */
                     pFlistener->pDestUri = (char *) IPTVosMalloc(strlen(pDestUri) + 1);
                     if (pFlistener->pDestUri == NULL)
                     {
                        res = IPTVosFree((unsigned char *)pFlistener);
                        if(res != 0)
                        {
                          IPTVosPrint1(IPT_ERR,
                                       "Failed to free data memory, res=%d\n",res);
                        }
                        IPTVosPrint1(IPT_ERR,
                                     "Could not allocate memory size=%d\n",
                                     strlen(pDestUri) + 1);
                        return IPT_MEM_ERROR;
                     }
                     else
                     {
                        strcpy(pFlistener->pDestUri, pDestUri);
                     }
                  }
               }
            }
      
            pFuncItem = mdFuncTabAdd(func, pCallerRef);
            if (pFuncItem == NULL)
            {
               if (pFlistener->pDestUri)
               {
                  res = IPTVosFree((unsigned char *)pFlistener->pDestUri);
                  if(res != 0)
                  {
                    IPTVosPrint1(IPT_ERR,
                                 "Failed to free data memory, res=%d\n",res);
                  }
               }
               res = IPTVosFree((unsigned char *)pFlistener);
               if(res != 0)
               {
                 IPTVosPrint1(IPT_ERR,
                              "Failed to free data memory, res=%d\n",res);
               }
               return IPT_MEM_ERROR;
            }
            else
            {
               pFlistener->pFunc = pFuncItem; 
            }
         }
      }
   }
#endif

   return(res);
}

/*******************************************************************************
NAME:     putMsgOnQueue 
ABSTRACT: Put a received message on listerners queues.
          If there is more listeners left a buffer for the message data is
          created and data is copied.
RETURNS:  0 if OK, !=0 if not.
*/
static void putMsgOnQueue(
   LISTENER_QUEUE *pQueue,
   UINT32 dataLength,             /* Length of received data */
   char   **ppMsgData,            /* Pointer to pointer to received data */
   QUEUE_MSG *pMsg,               /* Buffer for received message */
   int moreListeners)
{
   char *pTemp;
   int ret = IPT_OK;

   pMsg->pMsgData = *ppMsgData;

   if (moreListeners)
   {
      /* Allocate a new buffer for the received data */
      pTemp = (char *)IPTVosMalloc(dataLength);
      if (pTemp)
      {
         /* Copy the data */
         memcpy(pTemp,*ppMsgData,dataLength); 
         *ppMsgData = pTemp;
      
      }
      else
      {
         ret = IPT_ERROR;
         IPTVosPrint1(IPT_ERR,
                      "Put msg on listener queue: Out of memory. Requested size=%d\n",
                      dataLength);
      }
   }
 
   if (ret == IPT_OK)
   {
      pMsg->msgInfo.pCallerRef = (void *)pQueue->pCallerRef;
     
      ret = IPTVosSendMsgQueue((IPT_QUEUE *)((void *)&pQueue->listenerQueueId),
                               (char *)pMsg, sizeof(QUEUE_MSG));
      if (ret != (int)IPT_OK)
      {
         pTemp = getQueueItemName(pQueue->listenerQueueId);
         IPTVosPrint5(IPT_ERR,
               "ERROR sending message on listener queue ID=%#x Name=%s Callref=%#x. Msg type=%d ComId=%d\n",
                      pQueue->listenerQueueId, 
                      (pTemp != NULL)?pTemp:"None",
                      pMsg->msgInfo.pCallerRef,
                      pMsg->msgInfo.msgType, 
                      pMsg->msgInfo.comId );

         ret = IPTVosFree((unsigned char *)pMsg->pMsgData);
         if(ret != 0)
         {
           IPTVosPrint1(IPT_ERR,
                        "mdPutMsgOnListenerQueue failed to free data memory, code=%d\n",ret);
         }
      }
   }
}

/*******************************************************************************
NAME:     putMsgOnNormListQueue 
ABSTRACT: Search for  normal listerners queues.
          Put a received message on previous found queue and on found queues
          except for last found.
RETURNS:  0 if OK, !=0 if not.
*/
static void putMsgOnNormListQueue(
   LISTENER_LISTS *pLists,
   UINT32 lastRecMsgNo,
   LISTENER_QUEUE **ppPrevQueue,
   UINT32 dataLength,             /* Length of received data */
   char   **ppMsgData,            /* Pointer to pointer to received data */
   QUEUE_MSG *pMsg)               /* Buffer for received message */
{

   QUEUE_LIST     *pQlistener = pLists->pQueueList;
   while (pQlistener)
   {
      if (pQlistener->pQueue->lastRecMsgNo != lastRecMsgNo)
      {
         if (!pLists->counted)
         {
            pLists->mdInPackets++;
            pLists->counted = 1;
         }

         if (*ppPrevQueue)
         {
            putMsgOnQueue(*ppPrevQueue, dataLength, ppMsgData, pMsg, 1 ); 
         }

         *ppPrevQueue = pQlistener->pQueue;
         pQlistener->pQueue->lastRecMsgNo = lastRecMsgNo;   
      }

      pQlistener = pQlistener->pNext;
   }    
}

/*******************************************************************************
NAME:     putMsgOnFrgListQueue 
ABSTRACT: Search for  active Frg listerners queues.
          Put a received message on previous found queue and on found queues
          except for last found.
RETURNS:  0 if OK, !=0 if not.
*/
static void putMsgOnFrgListQueue(
   LISTENER_LISTS *pLists,
   UINT32 lastRecMsgNo,
   LISTENER_QUEUE **ppPrevQueue,
   UINT32 dataLength,             /* Length of received data */
   char   **ppMsgData,            /* Pointer to pointer to received data */
   QUEUE_MSG *pMsg)               /* Buffer for received message */
{
   FRG_ITEM *pFrgItem;
   QUEUE_FRG_LIST *pQFrglistener = pLists->pQueueFrgList; 

   while (pQFrglistener)
   {
      pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                 (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
      if ((pFrgItem) &&
          (pFrgItem->frgState == FRG_LEADER) &&
          (pQFrglistener->pQueue->lastRecMsgNo != lastRecMsgNo))
      {
         if (!pLists->frgCounted)
         {
            pLists->mdFrgInPackets++;;
            pLists->frgCounted = 1;
         }

         if (*ppPrevQueue)
         {
            putMsgOnQueue(*ppPrevQueue, dataLength, ppMsgData, pMsg, 1 ); 
         }

         *ppPrevQueue = pQFrglistener->pQueue;
         pQFrglistener->pQueue->lastRecMsgNo = lastRecMsgNo;   
      }
   
      pQFrglistener = pQFrglistener->pNext;
   }    
}

/*******************************************************************************
NAME:     putMsgOnNormListFunc 
ABSTRACT: Search for  normal listerners queues.
          Put a received message on previous found queue and on found queues
          except for last found.
RETURNS:  0 if OK, !=0 if not.
*/
static void putMsgOnNormListFunc(
   LISTENER_LISTS *pLists,
   UINT32 lastRecMsgNo,
   UINT32 dataLength,             /* Length of received data */
   char   *pMsgData,            /* Pointer to pointer to received data */
   MSG_INFO *pMsgInfo)    /* Message info */
{
   FUNC_LIST     *pFlistener = pLists->pFuncList;
   
   while (pFlistener)
   {
      if (pFlistener->pFunc->lastRecMsgNo != lastRecMsgNo)
      {
         if (!pLists->counted)
         {
            pLists->mdInPackets++;
            pLists->counted = 1;
         }

         pMsgInfo->pCallerRef = (void *)pFlistener->pFunc->pCallerRef;
         /* call function */
         pFlistener->pFunc->func(pMsgInfo, pMsgData, dataLength);
         pFlistener->pFunc->lastRecMsgNo = lastRecMsgNo;
      }

      pFlistener = pFlistener->pNext;
   }    
}

/*******************************************************************************
NAME:     putMsgOnFrgListFunc 
ABSTRACT: Search for  active Frg listerners queues.
          Put a received message on previous found queue and on found queues
          except for last found.
RETURNS:  0 if OK, !=0 if not.
*/
static void putMsgOnFrgListFunc(
   LISTENER_LISTS *pLists,
   UINT32 lastRecMsgNo,
   UINT32 dataLength,             /* Length of received data */
   char   *pMsgData,            /* Pointer to pointer to received data */
   MSG_INFO *pMsgInfo)    /* Message info */
{
   FRG_ITEM *pFrgItem;
   FUNC_FRG_LIST *pFFrglistener = pLists->pFuncFrgList;

   while (pFFrglistener)
   {
      pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                 (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
      if ((pFrgItem) &&
          (pFrgItem->frgState == FRG_LEADER) &&
          (pFFrglistener->pFunc->lastRecMsgNo != lastRecMsgNo))
      {
         if (!pLists->counted)
         {
            pLists->mdFrgInPackets++;;
            pLists->counted = 1;
         }
         
         pMsgInfo->pCallerRef = (void *)pFFrglistener->pFunc->pCallerRef;
         /* call function */
         pFFrglistener->pFunc->func(pMsgInfo, pMsgData, dataLength);
         pFFrglistener->pFunc->lastRecMsgNo = lastRecMsgNo;
      }
   
      pFFrglistener = pFFrglistener->pNext;
   }    
}

/*******************************************************************************
NAME:     finish_addListener
ABSTRACT: Resolve IP addresses for added listeners that was added before TDC
          was ready. Join multicast addresses.
RETURNS:  - 
*/
static int finish_addListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   /* Simualted IP address */
#endif
   LISTENER_LISTS *pLists) /* Pointer to lists with listeners */
{
 
   QUEUE_LIST     *pQlistener;
   QUEUE_FRG_LIST *pQFrglistener;
   FUNC_LIST      *pFlistener;
   FUNC_FRG_LIST  *pFFrglistener;

   pQlistener = pLists->pQueueList;
   while (pQlistener)
   {
      if (pQlistener->destIpAddr == IP_ADDRESS_NOT_RESOLVED)
      {
         (void)getComIdDestIPAddr(
#ifdef TARGET_SIMU
                                  simuIpAddr,
#endif
                                  pQlistener->comId, 
                                  pQlistener->destId, 
                                  pQlistener->pDestUri, 
                                  &(pQlistener->destIpAddr));
         if (pQlistener->destIpAddr == IP_ADDRESS_NOT_RESOLVED)
         {
            /* Exit wait for TDC to be ready*/
            return (int)IPT_ERROR;
         }
         else if (pQlistener->pDestUri)
         {
            IPTVosFree((BYTE *)pQlistener->pDestUri);
            pQlistener->pDestUri = NULL;
         }
      }

      pQlistener = pQlistener->pNext;
   }

   pQFrglistener = pLists->pQueueFrgList;
   while (pQFrglistener)
   {
      if (pQFrglistener->destIpAddr == IP_ADDRESS_NOT_RESOLVED)
      {
         (void)getComIdDestIPAddr(
#ifdef TARGET_SIMU
                                  simuIpAddr,
#endif
                                  pQFrglistener->comId, 
                                  pQFrglistener->destId, 
                                  pQFrglistener->pDestUri, 
                                  &(pQFrglistener->destIpAddr));
         if (pQFrglistener->destIpAddr == IP_ADDRESS_NOT_RESOLVED)
         {
            /* Exit wait for TDC to be ready*/
            return (int)IPT_ERROR;
         }
         else if (pQFrglistener->pDestUri)
         {
            IPTVosFree((BYTE *)pQFrglistener->pDestUri);
            pQFrglistener->pDestUri = NULL;
         }
      }

      pQFrglistener = pQFrglistener->pNext;
   }

   pFlistener = pLists->pFuncList;
   while (pFlistener)
   {
      if (pFlistener->destIpAddr == IP_ADDRESS_NOT_RESOLVED)
      {
         (void)getComIdDestIPAddr(
#ifdef TARGET_SIMU
                                  simuIpAddr,
#endif
                                  pFlistener->comId, 
                                  pFlistener->destId, 
                                  pFlistener->pDestUri, 
                                  &(pFlistener->destIpAddr));
         if (pFlistener->destIpAddr == IP_ADDRESS_NOT_RESOLVED)
         {
            /* Exit wait for TDC to be ready*/
            return (int)IPT_ERROR;
         }
         else if (pFlistener->pDestUri)
         {
            IPTVosFree((BYTE *)pFlistener->pDestUri);
            pFlistener->pDestUri = NULL;
         }
      }

      pFlistener = pFlistener->pNext;
   }

   pFFrglistener = pLists->pFuncFrgList;
   while (pFFrglistener)
   {
      if (pFFrglistener->destIpAddr == IP_ADDRESS_NOT_RESOLVED)
      {
         (void)getComIdDestIPAddr(
#ifdef TARGET_SIMU
                                  simuIpAddr,
#endif
                                  pFFrglistener->comId, 
                                  pFFrglistener->destId, 
                                  pFFrglistener->pDestUri, 
                                  &(pFFrglistener->destIpAddr));
         if (pFFrglistener->destIpAddr == IP_ADDRESS_NOT_RESOLVED)
         {
            /* Exit wait for TDC to be ready*/
            return (int)IPT_ERROR;
         }
         else if (pFFrglistener->pDestUri)
         {
            IPTVosFree((BYTE *)pFFrglistener->pDestUri);
            pFFrglistener->pDestUri = NULL;
         }
      }

      pFFrglistener = pFFrglistener->pNext;
   }

    return (int)IPT_OK;
}

#ifdef TARGET_SIMU            
/*******************************************************************************
NAME:     getSimDev
ABSTRACT: Get simulated device item.
RETURNS:  Pointer ti item 
*/
static SIMU_DEV_ITEM * getSimDev(
   const char      *pSimURI)        /* Host URI of simulated device */
{
   int res = IPT_OK;
   int i;
   
   int found;
   UINT8 		  dummy;
   UINT32         simuIpAddr;
   SIMU_DEV_ITEM  simDevItem;
   SIMU_DEV_ITEM  *pSimDevItem;

   if (pSimURI == NULL)
   {
      IPTVosPrint0(IPT_ERR,
         "getSimDev: NULL pointer to simulated URI\n");
      return((SIMU_DEV_ITEM *)NULL);
   }

   /* search for simulated device */
   found = 0;
   pSimDevItem = (SIMU_DEV_ITEM *)((void *)IPTGLOBAL(md.simuDevListTableHdr.pTable));
   for (i=0; i<IPTGLOBAL(md.simuDevListTableHdr.nItems); i++)
   {
      if (iptStrncmp(pSimDevItem[i].simUri ,pSimURI, sizeof(pSimDevItem[i].simUri)) == 0)
      {
         found = 1;
         pSimDevItem = &pSimDevItem[i];
         break;  
      } 
   }

   if (!found)
   {
      if (pSimURI[0] != 0)
      {
         strncpy(simDevItem.simUri, pSimURI, sizeof(simDevItem.simUri) -1);
         simDevItem.simUri[sizeof(simDevItem.simUri) - 1] = 0;

         dummy = 0;
         res = iptGetAddrByName(pSimURI, &simuIpAddr , &dummy);
         if (res == TDC_OK)
         {
            simDevItem.simuIpAddr = simuIpAddr;
         }
         else
         {
            if ((res == TDC_NO_CONFIG_DATA) || (res == TDC_MUST_FINISH_INIT))
            {
               /* Indicate that address resolving has to be done later when TDC 
                  has got data from IPTDir */
               IPTGLOBAL(md.finish_addr_resolv) = 1;

               /* Exit wait for TDC to be ready*/
               simDevItem.simuIpAddr = IP_ADDRESS_NOT_RESOLVED;

               res = IPT_OK;
            }
            else
            {
               IPTVosPrint2(IPT_ERR,
               "Could not convert destination URI=%s to IP address. TDC result=%#x\n",
                            pSimURI, res);
               simDevItem.simuIpAddr = 0;
            }
         }
      }
      else
      {
         simDevItem.simuIpAddr = IPTCom_getOwnIpAddr();
         simDevItem.simUri[0] = 0;
      }

      if (res == IPT_OK)
      {
         if ((res = iptTabInit(&simDevItem.listTables.comidListTableHdr, sizeof(COMID_ITEM))) != (int)IPT_OK)
         {
            IPTVosPrint1(IPT_ERR,
               "getSimDev: Failed to initiate table. Error=%#x\n",
               res);
         }
         else if ((res = iptUriLabelTab2Init(&simDevItem.listTables.instXFuncNListTableHdr, sizeof(INSTX_FUNCN_ITEM))) != (int)IPT_OK)
         {
            iptTabTerminate(&simDevItem.listTables.comidListTableHdr);
            IPTVosPrint1(IPT_ERR,
               "getSimDev: Failed to initiate table. Error=%#x\n",
               res);
         }
         else if ((res = iptUriLabelTabInit(&simDevItem.listTables.aInstFuncNListTableHdr, sizeof(AINST_FUNCN_ITEM))) != (int)IPT_OK)
         {
            iptTabTerminate(&simDevItem.listTables.comidListTableHdr);
            iptUriLabelTab2Terminate(&simDevItem.listTables.instXFuncNListTableHdr);
            IPTVosPrint1(IPT_ERR,
               "getSimDev: Failed to initiate table. Error=%#x\n",
               res);
         }
         else if ((res = iptUriLabelTabInit(&simDevItem.listTables.instXaFuncListTableHdr, sizeof(INSTX_AFUNC_ITEM))) != (int)IPT_OK)
         {
            iptTabTerminate(&simDevItem.listTables.comidListTableHdr);
            iptUriLabelTab2Terminate(&simDevItem.listTables.instXFuncNListTableHdr);
            iptUriLabelTabTerminate(&simDevItem.listTables.aInstFuncNListTableHdr);
            IPTVosPrint1(IPT_ERR,
               "getSimDev: Failed to initiate table. Error=%#x\n",
               res);
         }
         else
         {
            simDevItem.listTables.aInstAfunc.pQueueList = NULL;
            simDevItem.listTables.aInstAfunc.pQueueFrgList = NULL;
            simDevItem.listTables.aInstAfunc.pFuncList = NULL;
            simDevItem.listTables.aInstAfunc.pFuncFrgList = NULL;
           
            res = iptTabUriAdd(&IPTGLOBAL(md.simuDevListTableHdr), (IPT_TAB_URI_ITEM *)((void *)&simDevItem));
            if (res != IPT_OK)
            {
               IPTVosPrint1(IPT_ERR,
                  "getSimDev: Failed to add simulated device to table. Error=%#x\n",
                  res);
            }
            else
            {
               pSimDevItem = (SIMU_DEV_ITEM *)((void *)IPTGLOBAL(md.simuDevListTableHdr.pTable));
               for (i=0; i<IPTGLOBAL(md.simuDevListTableHdr.nItems); i++)
               {
                  if (iptStrncmp(pSimDevItem[i].simUri ,pSimURI, sizeof(pSimDevItem[i].simUri)) == 0)
                  {
                     found = 1;
                     pSimDevItem = &pSimDevItem[i];
                     break;  
                  } 
         
               }
            }
         }
      }
   }
   
   if (!found)
   {
      pSimDevItem = NULL;
   }
   return(pSimDevItem);
}
#endif

/*******************************************************************************
* GLOBAL FUNCTIONS */

/*******************************************************************************
NAME:     extractUri
ABSTRACT: Extract instance and function from a URI string  
RETURNS:  Type of user URI
*/
int extractUri(
   const char *pDestURI,       /* Pointer to destination URI string */
   char       *pUriInstName,   /* Pointer to instance URI string array with size IPT_MAX_LABEL_LEN */
   char       *pUriFuncName,   /* Pointer to function URI string array with size IPT_MAX_LABEL_LEN */
   int        *pUriType)       /* Pointer to URI type */
{
   const char *pUriHost;
   const char *pUriFunc;
   const char *pTemp;
   int ret = IPT_OK;
   int len;

   *pUriInstName = 0;
   *pUriFuncName = 0;
   *pUriType = NO_USER_URI;

   len = strlen(pDestURI);
   if (len == 0)
   {
      return((int)IPT_INVALID_PAR);
   }
   
   /* Ignore any ipt:// */
   pTemp = strrchr(pDestURI,'/');
   if (pTemp)
   {
      if (len > pTemp + 1 - pDestURI)
      {
         pDestURI = pTemp + 1;
         len = strlen(pDestURI); 
      }
      else
      {
         return((int)IPT_INVALID_PAR);
      }
   }

   /* Search for '@' */
   pUriHost = strchr(pDestURI,'@');
   if (pUriHost)
   {
      /* Search for dot delimiter */
      pUriFunc = strchr(pDestURI,'.');

      /* Dot found in URI host part or not found at all?
         I.e. no URI instance given */
      if ((pUriFunc == 0) || (pUriFunc > pUriHost))
      {
         if (((pUriHost - pDestURI) < IPT_MAX_LABEL_LEN) && ((pUriHost - pDestURI) > 0))
         {
            /* Copy URI function part */
            memcpy(pUriFuncName,pDestURI,pUriHost - pDestURI);

            /* Terminate the strings */
            pUriFuncName[pUriHost - pDestURI] = 0;
            pUriInstName[0] = 0;
            if (iptStrcmp("aFunc",pUriFuncName) == 0)
            {
               *pUriType = AINST_AFUNC_URI;   
            }
            else
            {
               *pUriType = AINST_FUNCN_URI;   
            }
         }
         else
         {
            ret = IPT_INVALID_PAR;
         }
      }
      else
      {
         /* Check that no extra dot is found before '@' */
         pTemp = strchr(pUriFunc + 1,'.');
         if ((pTemp) && (pTemp < pUriHost))
         {
            ret = IPT_INVALID_PAR;
         }
         else if (((pUriFunc - pDestURI) < IPT_MAX_LABEL_LEN) &&
             ((pUriHost - pUriFunc - 1) < IPT_MAX_LABEL_LEN) &&
             ((pUriHost - pUriFunc - 1) > 0))
         {
            /* Copy URI function part */
            memcpy(pUriFuncName,pUriFunc+1,pUriHost - pUriFunc - 1);

            /* Terminate the string */
            pUriFuncName[pUriHost - pUriFunc - 1] = 0;
            
            if ((pUriFunc - pDestURI) > 0)
            {
               /* Copy URI instance part */
               memcpy(pUriInstName,pDestURI,pUriFunc - pDestURI);

               /* Terminate the string */
               pUriInstName[pUriFunc - pDestURI] = 0;

               if (iptStrcmp("aFunc",pUriFuncName) == 0)
               {
                  if (iptStrcmp("aInst",pUriInstName) == 0)
                  {
                     *pUriType = AINST_AFUNC_URI;   
                  }
                  else
                  {
                     *pUriType = INSTX_AFUNC_URI;   
                  }
               }
               else
               {
                  if (iptStrcmp("aInst",pUriInstName) == 0)
                  {
                     *pUriType = AINST_FUNCN_URI;   
                  }
                  else
                  {
                     *pUriType = INSTX_FUNCN_URI;   
                  }
               }
            }
            else
            {
               /* Terminate the string */
               pUriInstName[pUriFunc - pDestURI] = 0;
               if (iptStrcmp("aFunc",pUriFuncName) == 0)
               {
                  *pUriType = AINST_AFUNC_URI;   
               }
               else
               {
                  *pUriType = AINST_FUNCN_URI;   
               }
            }
         }
         else
         {
            ret = IPT_INVALID_PAR;
         }
      }
   }
   /* No URI host part */
   else
   {
      /* Search for dot delimiter */
      pUriFunc = strchr(pDestURI,'.');

      /* Dot found? I.e. URI instance part given */
      if (pUriFunc)
      {
         /* Not any function name? */
         if ((pUriFunc + 1 - pDestURI) >= len)
         {
            ret = IPT_INVALID_PAR;
         }
         else
         {
            /* Check that no extra dot is found  */
            pTemp = strchr(pUriFunc +1,'.');
            if (pTemp)
            {
               ret = IPT_INVALID_PAR;
            }
            else if (((pUriFunc - pDestURI) < IPT_MAX_LABEL_LEN) &&
                (strlen(pUriFunc+1) < IPT_MAX_LABEL_LEN))
            {
               /* Copy URI function part */
               strcpy(pUriFuncName,pUriFunc+1);
            
               if ((pUriFunc - pDestURI) > 0)
               {
                  /* Copy URI instance part */
                  memcpy(pUriInstName,pDestURI,pUriFunc - pDestURI);

                  /* Terminate the string */
                  pUriInstName[pUriFunc - pDestURI] = 0;
                  if (iptStrcmp("aFunc",pUriFuncName) == 0)
                  {
                     if (iptStrcmp("aInst",pUriInstName) == 0)
                     {
                        *pUriType = AINST_AFUNC_URI;   
                     }
                     else
                     {
                        *pUriType = INSTX_AFUNC_URI;   
                     }
                  }
                  else
                  {
                     if (iptStrcmp("aInst",pUriInstName) == 0)
                     {
                        *pUriType = AINST_FUNCN_URI;   
                     }
                     else
                     {
                        *pUriType = INSTX_FUNCN_URI;   
                     }
                  }
               }
               else
               {
                  /* Terminate the string */
                  pUriInstName[pUriFunc - pDestURI] = 0;
                  if (iptStrcmp("aFunc",pUriFuncName) == 0)
                  {
                     *pUriType = AINST_AFUNC_URI;   
                  }
                  else
                  {
                     *pUriType = AINST_FUNCN_URI;   
                  }
               }
            }
            else
            {
               ret = IPT_INVALID_PAR;
            }
         }
      }
      /* Only URI function part */
      else
      {
         pUriFunc = (char *)pDestURI;
         if (len < IPT_MAX_LABEL_LEN)
         {
            /* Copy URI function part */
            strcpy(pUriFuncName,pUriFunc);

            /* Terminate the strings */
            pUriInstName[0] = 0;
            if (iptStrcmp("aFunc",pUriFuncName) == 0)
            {
               *pUriType = AINST_AFUNC_URI;   
            }
            else
            {
               *pUriType = AINST_FUNCN_URI;   
            }
         }
         else
         {
            ret = IPT_INVALID_PAR;
         }
      }
   }

   return(ret);
}

/*******************************************************************************
NAME:     MD_finish_addListener
ABSTRACT: Resolve IP addresses for added listeners that was added before TDC
          was ready. Join multicast addresses.
RETURNS:  - 
*/
void MD_finish_addListener(void)
{
   int ret;
   int i;
#ifdef TARGET_SIMU            
   int j;
   UINT8 dummy;
   UINT32 simuIpAddr;
   SIMU_DEV_ITEM  *pSimDevItem;
#endif
 
   COMID_ITEM    *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;

   ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      IPTGLOBAL(md.finish_addr_resolv) = 0;
     
#ifdef TARGET_SIMU
      pSimDevItem = (SIMU_DEV_ITEM *)(IPTGLOBAL(md.simuDevListTableHdr.pTable));/*lint !e826 Type cast OK */
      for (j = 0; j <IPTGLOBAL(md.simuDevListTableHdr.nItems); j++)
      {
         if (pSimDevItem[j].simuIpAddr == IP_ADDRESS_NOT_RESOLVED)
         {
            dummy = 0;
            ret = iptGetAddrByName(pSimDevItem[j].simUri, &simuIpAddr,
                                   &dummy);
            if (ret == TDC_OK)
            {
               pSimDevItem[j].simuIpAddr = simuIpAddr;
            }
            else
            {
               if ((ret == TDC_NO_CONFIG_DATA) || (ret == TDC_MUST_FINISH_INIT))
               {
                  /* Indicate that address resolving has to be done later when TDC 
                     has got data from IPTDir */
                  IPTGLOBAL(md.finish_addr_resolv) = 1;

                  /* Exit wait for TDC to be ready*/
                  pSimDevItem[j].simuIpAddr = IP_ADDRESS_NOT_RESOLVED;
                  
                  /* Exit wait for TDC to be ready*/
                  if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                  {
                     IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                  }
                  return;
               }
               else
               {
                  IPTVosPrint2(IPT_ERR,
                  "Could not convert destination URI=%s to IP address. TDC result=%#x\n",
                               pSimDevItem[j].simUri, ret);
                  pSimDevItem[j].simuIpAddr = 0;
               }
            }
         }
     
         pComIdItem = (COMID_ITEM *)(pSimDevItem[j].listTables.comidListTableHdr.pTable);/*lint !e826 Type cast OK */
         for (i = 0; i <pSimDevItem[j].listTables.comidListTableHdr.nItems; i++)
         {
            ret = finish_addListener(pSimDevItem[j].simuIpAddr, &(pComIdItem[i].lists));
            if (ret != IPT_OK)
            {
               /* Exit wait for TDC to be ready*/
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return;
            }
         }
        
         ret = finish_addListener(pSimDevItem[j].simuIpAddr, &pSimDevItem[j].listTables.aInstAfunc);
         if (ret != IPT_OK)
         {
            /* Exit wait for TDC to be ready*/
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return;
         }

         pInstXFuncN = (INSTX_FUNCN_ITEM *)pSimDevItem[j].listTables.instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
         for (i = 0; i < pSimDevItem[j].listTables.instXFuncNListTableHdr.nItems ; i++)
         {
            ret = finish_addListener(pSimDevItem[j].simuIpAddr, &(pInstXFuncN[i].lists));
            if (ret != IPT_OK)
            {
               /* Exit wait for TDC to be ready*/
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return;
            }
         }

         pAinstFuncN = (AINST_FUNCN_ITEM *)pSimDevItem[j].listTables.aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
         for (i = 0; i < pSimDevItem[j].listTables.aInstFuncNListTableHdr.nItems ; i++)
         {
            ret = finish_addListener(pSimDevItem[j].simuIpAddr, &(pAinstFuncN[i].lists));
            if (ret != IPT_OK)
            {
               /* Exit wait for TDC to be ready*/
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return;
            }
         }

         pInstXaFunc = (INSTX_AFUNC_ITEM *)pSimDevItem[j].listTables.instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
         for (i = 0; i < pSimDevItem[j].listTables.instXaFuncListTableHdr.nItems ; i++)
         {
            ret = finish_addListener(pSimDevItem[j].simuIpAddr, &(pInstXaFunc[i].lists));
            if (ret != IPT_OK)
            {
               /* Exit wait for TDC to be ready*/
               if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
               return;
            }
         }
      }
#else
      pComIdItem = (COMID_ITEM *)(IPTGLOBAL(md.listTables.comidListTableHdr.pTable));/*lint !e826 Type cast OK */
      for (i = 0; i <IPTGLOBAL(md.listTables.comidListTableHdr.nItems); i++)
      {
         ret = finish_addListener(&(pComIdItem[i].lists));
         if (ret != IPT_OK)
         {
            /* Exit wait for TDC to be ready*/
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return;
         }
      }
     
      ret = finish_addListener(&IPTGLOBAL(md.listTables.aInstAfunc));
      if (ret != IPT_OK)
      {
         /* Exit wait for TDC to be ready*/
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
         return;
      }

      pInstXFuncN = (INSTX_FUNCN_ITEM *)IPTGLOBAL(md.listTables.instXFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
      for (i = 0; i < IPTGLOBAL(md.listTables.instXFuncNListTableHdr.nItems) ; i++)
      {
         ret = finish_addListener(&(pInstXFuncN[i].lists));
         if (ret != IPT_OK)
         {
            /* Exit wait for TDC to be ready*/
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return;
         }
      }

      pAinstFuncN = (AINST_FUNCN_ITEM *)IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
      for (i = 0; i < IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.nItems) ; i++)
      {
         ret = finish_addListener(&(pAinstFuncN[i].lists));
         if (ret != IPT_OK)
         {
            /* Exit wait for TDC to be ready*/
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return;
         }
      }

      pInstXaFunc = (INSTX_AFUNC_ITEM *)IPTGLOBAL(md.listTables.instXaFuncListTableHdr.pTable);/*lint !e826 Type cast OK */
      for (i = 0; i < IPTGLOBAL(md.listTables.instXaFuncListTableHdr.nItems) ; i++)
      {
         ret = finish_addListener(&(pInstXaFunc[i].lists));
         if (ret != IPT_OK)
         {
            /* Exit wait for TDC to be ready*/
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return;
         }
      }
#endif

      if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "MD_finish_addListener: IPTVosGetSem ERROR\n");
   }
}

/*******************************************************************************
NAME:     MD_renew_mc_listeners 
ABSTRACT: Re-join all listeners which belong to MC groups
RETURNS:  nothing.
*/
void  MD_renew_mc_listeners(void)
{

	/*	No simulated link-up events in TARGET_SIMU!	*/
#ifndef TARGET_SIMU
    int 				ret;
    COMID_ITEM    	*pComIdItem;
    QUEUE_LIST		*pItem1;
	QUEUE_FRG_LIST	*pItem2;
    FUNC_LIST		*pItem3;
	FUNC_FRG_LIST	*pItem4;
    INSTX_FUNCN_ITEM *pInstXFuncN;
    AINST_FUNCN_ITEM *pAinstFuncN;
    INSTX_AFUNC_ITEM *pInstXaFunc;

   	ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   	if (ret == IPT_OK)
   	{
      	pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr), 0));/*lint !e826  Ignore casting warning */

      	while(pComIdItem)
        {
         	pItem1 = pComIdItem->lists.pQueueList;
         	while (pItem1)
         	{
				if (isMulticastIpAddr(pItem1->destIpAddr))
            	{
               	 	joinMDmulticastAddress(pItem1->destIpAddr);
            	}
            	pItem1 = pItem1->pNext;
         	}
         
            pItem2 = pComIdItem->lists.pQueueFrgList;
            while (pItem2)
            {
            	if (isMulticastIpAddr(pItem2->destIpAddr))
                {
                	joinMDmulticastAddress(pItem2->destIpAddr);
                }
                pItem2 = pItem2->pNext;
            }

            pItem3 = pComIdItem->lists.pFuncList;
            while (pItem3)
            {
            	if (isMulticastIpAddr(pItem3->destIpAddr))
                {
                	joinMDmulticastAddress(pItem3->destIpAddr);
                }
                pItem3 = pItem3->pNext;
            }

            pItem4 = pComIdItem->lists.pFuncFrgList;
            while (pItem4)
            {
                if (isMulticastIpAddr(pItem4->destIpAddr))
                {
                	joinMDmulticastAddress(pItem4->destIpAddr);
                }
                pItem4 = pItem4->pNext;
            }

			pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr),
                                                             pComIdItem->keyComId));/*lint !e826  Ignore casting warning */
        }

      	/* Re-join listeners for all instances of all functions */
     	pItem1 = IPTGLOBAL(md.listTables.aInstAfunc).pQueueList;
     	while (pItem1)
        {
        	if (isMulticastIpAddr(pItem1->destIpAddr))
        	{
            	joinMDmulticastAddress(pItem1->destIpAddr);
        	}
        	pItem1 = pItem1->pNext;
        }
      
     	pItem2 = IPTGLOBAL(md.listTables.aInstAfunc).pQueueFrgList;
     	while (pItem2)
        {
        	if (isMulticastIpAddr(pItem2->destIpAddr))
        	{
            	joinMDmulticastAddress(pItem2->destIpAddr);
        	}
        	pItem2 = pItem2->pNext;
        }
      
     	pItem3 = IPTGLOBAL(md.listTables.aInstAfunc).pFuncList;
     	while (pItem3)
        {
        	if (isMulticastIpAddr(pItem3->destIpAddr))
        	{
            	joinMDmulticastAddress(pItem3->destIpAddr);
        	}
        	pItem3 = pItem3->pNext;
        }
      
      	pItem4 = IPTGLOBAL(md.listTables.aInstAfunc).pFuncFrgList;
     	while (pItem4)
        {
        	if (isMulticastIpAddr(pItem4->destIpAddr))
        	{
            	joinMDmulticastAddress(pItem4->destIpAddr);
        	}
        	pItem4 = pItem4->pNext;
        }
      
      
      	pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&IPTGLOBAL(md.listTables.instXFuncNListTableHdr),
                                                                  "", ""));
      	while(pInstXFuncN)
      	{
   			pItem1 = pInstXFuncN->lists.pQueueList;

         	while (pItem1)
         	{
         		if (isMulticastIpAddr(pItem1->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem1->destIpAddr);
        		}
        		pItem1 = pItem1->pNext;
        	}
         
   			pItem2 = pInstXFuncN->lists.pQueueFrgList;
         	while (pItem2)
         	{
         		if (isMulticastIpAddr(pItem2->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem2->destIpAddr);
        		}
        		pItem2 = pItem2->pNext;
        	}
         
   			pItem3 = pInstXFuncN->lists.pFuncList;
         	while (pItem3)
         	{
         		if (isMulticastIpAddr(pItem3->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem3->destIpAddr);
        		}
        		pItem3 = pItem3->pNext;
        	}
         
   			pItem4 = pInstXFuncN->lists.pFuncFrgList;
         
         	while (pItem4)
         	{
         		if (isMulticastIpAddr(pItem4->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem4->destIpAddr);
        		}
        		pItem4 = pItem4->pNext;
        	}
      
         	pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&IPTGLOBAL(md.listTables.instXFuncNListTableHdr),
                                                                      pInstXFuncN->instName, pInstXFuncN->funcName));
		}

      	pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.aInstFuncNListTableHdr),
                                                                  ""));
      	while(pAinstFuncN)
      	{
   			pItem1 = pAinstFuncN->lists.pQueueList;
         	while (pItem1)
         	{
         		if (isMulticastIpAddr(pItem1->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem1->destIpAddr);
        		}
        		pItem1 = pItem1->pNext;
        	}
         
   			pItem2 = pAinstFuncN->lists.pQueueFrgList;
         	while (pItem2)
         	{
         		if (isMulticastIpAddr(pItem2->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem2->destIpAddr);
        		}
        		pItem2 = pItem2->pNext;
        	}
         
   			pItem3 = pAinstFuncN->lists.pFuncList;
         	while (pItem3)
         	{
         		if (isMulticastIpAddr(pItem3->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem3->destIpAddr);
        		}
        		pItem3 = pItem3->pNext;
        	}
         
   			pItem4 = pAinstFuncN->lists.pFuncFrgList;
         
         	while (pItem4)
         	{
         		if (isMulticastIpAddr(pItem4->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem4->destIpAddr);
        		}
        		pItem4 = pItem4->pNext;
        	}
      
         	pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.aInstFuncNListTableHdr),
                                                                     pAinstFuncN->funcName));
		}
      

      pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.instXaFuncListTableHdr),
                                                                  ""));
      	while(pInstXaFunc)
      	{
   			pItem1 = pInstXaFunc->lists.pQueueList;
         	while (pItem1)
         	{
         		if (isMulticastIpAddr(pItem1->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem1->destIpAddr);
        		}
        		pItem1 = pItem1->pNext;
        	}
         
   			pItem2 = pInstXaFunc->lists.pQueueFrgList;
         	while (pItem2)
         	{
         		if (isMulticastIpAddr(pItem2->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem2->destIpAddr);
        		}
        		pItem2 = pItem2->pNext;
        	}
         
   			pItem3 = pInstXaFunc->lists.pFuncList;
         	while (pItem3)
         	{
         		if (isMulticastIpAddr(pItem3->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem3->destIpAddr);
        		}
        		pItem3 = pItem3->pNext;
        	}
         
   			pItem4 = pInstXaFunc->lists.pFuncFrgList;
         
         	while (pItem4)
         	{
         		if (isMulticastIpAddr(pItem4->destIpAddr))
        		{
            		joinMDmulticastAddress(pItem4->destIpAddr);
        		}
        		pItem4 = pItem4->pNext;
        	}
      
         	pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.instXaFuncListTableHdr),
                                                                  pInstXaFunc->instName));
		}

      	if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      	{
         	IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      	}
	}
	else
   	{
      	IPTVosPrint0(IPT_ERR, "mdPutMsgOnListenerQueue: IPTVosGetSem ERROR\n");
   	}
#endif
}

/*******************************************************************************
NAME:     mdAnyListenerOnQueue 
ABSTRACT: Check if there is any queue listeners for received ComId or user URI
RETURNS:  0 = not found, 1 = found.
*/
int mdAnyListenerOnQueue(
   MD_QUEUE queue)   /* Queue  */
{
   int ret;
   LISTERNER_TABLES *pListTables;
#ifdef TARGET_SIMU            
   int j;
   SIMU_DEV_ITEM  *pSimDevItem;
#endif

   ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      pListTables = &IPTGLOBAL(md.listTables);
      ret = anyListenerOnQueue(queue, pListTables);

#ifdef TARGET_SIMU            
      if (ret == 0)
      {
         pSimDevItem = (SIMU_DEV_ITEM *)(IPTGLOBAL(md.simuDevListTableHdr.pTable));/*lint !e826 Type cast OK */
         for (j = 0; (j < IPTGLOBAL(md.simuDevListTableHdr.nItems)) && (ret == 0); j++)
         {
            pListTables = &pSimDevItem[j].listTables;
            ret = anyListenerOnQueue(queue, pListTables);
         }
      }
#endif
      
      if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "mdAnyQueueListener: IPTVosGetSem ERROR\n");
      ret = 0;
   }
   return(ret);
}

/*******************************************************************************
NAME:     mdAnyListener 
ABSTRACT: Check if there is any queue listeners for received ComId or user URI
RETURNS:  0 = not found, 1 = found.
RETURNS:  0 if OK, !=0 if not.
*/
int mdAnyListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr, /* Simualted IP address */
#endif
   UINT32 comid,
   int    uriType,                    /* Type of received user part URI */
   char   *pUriInstName,              /* Pointer to URI instance name */
   char   *pUriFuncName)              /* Pointer to URI function name */
{
   int ret;
   UINT32 i;
   COMID_ITEM    *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   QUEUE_FRG_LIST *pQFrglistener;
   FUNC_FRG_LIST  *pFFrglistener;
   LISTERNER_TABLES *pListTables;
   FRG_ITEM *pFrgItem;
#ifdef TARGET_SIMU            
   SIMU_DEV_ITEM  *pSimDevItem;
#endif

   ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
#ifdef TARGET_SIMU
      if (simuIpAddr == IPTCom_getOwnIpAddr())
      {
         pListTables = &IPTGLOBAL(md.listTables);
      }
      else
      {
         pSimDevItem = (SIMU_DEV_ITEM *) ((void *)iptTabFind((const IPT_TAB_HDR *)&IPTGLOBAL(md.simuDevListTableHdr), simuIpAddr));
         if (pSimDevItem)
         {
            pListTables = &pSimDevItem->listTables;
         }
         else
         {
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return(0);
         }
      }
#else
      pListTables = &IPTGLOBAL(md.listTables);
#endif
      pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&pListTables->comidListTableHdr,
                                                      comid));/*lint !e826  Ignore casting warning */
      if (pComIdItem)
      {
         if ((pComIdItem->lists.pQueueList) || (pComIdItem->lists.pFuncList))
         {
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return(1);
         }
        
         if (pComIdItem->lists.pQueueFrgList)
         {
            pQFrglistener = pComIdItem->lists.pQueueFrgList;
            while (pQFrglistener)
            {
               pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                          (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
               if (pFrgItem)
               {
                  if (pFrgItem->frgState == FRG_LEADER)
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
               }
               pQFrglistener = pQFrglistener->pNext;
            }
         }
         
         if (pComIdItem->lists.pFuncFrgList)
         {
            pFFrglistener = pComIdItem->lists.pFuncFrgList;
            while (pFFrglistener)
            {
               pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                          (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
               if (pFrgItem)
               {
                  if (pFrgItem->frgState == FRG_LEADER)
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
               }
               pFFrglistener = pFFrglistener->pNext;
            }
         }
      }

      if (uriType != NO_USER_URI)
      {
         if ((pListTables->aInstAfunc.pQueueList) || (pListTables->aInstAfunc.pFuncList))
         {
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return(1);
         }
         
         if (pListTables->aInstAfunc.pQueueFrgList)
         {
            pQFrglistener = pListTables->aInstAfunc.pQueueFrgList;
            while (pQFrglistener)
            {
               pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                          (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
               if (pFrgItem)
               {
                  if (pFrgItem->frgState == FRG_LEADER)
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
               }
               pQFrglistener = pQFrglistener->pNext;
            }
         }
         
         if (pListTables->aInstAfunc.pFuncFrgList)
         {
            pFFrglistener = pListTables->aInstAfunc.pFuncFrgList;
            while (pFFrglistener)
            {
               pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                          (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
               if (pFrgItem)
               {
                  if (pFrgItem->frgState == FRG_LEADER)
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
               }
               pFFrglistener = pFFrglistener->pNext;
            }
         }
         switch (uriType)
         {
            /* Received user URI addressed to given instances of a given function */
            case INSTX_FUNCN_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2Find(&pListTables->instXFuncNListTableHdr,
                                                                      pUriInstName, pUriFuncName));
               if (pInstXFuncN)
               {
                  if ((pInstXFuncN->lists.pQueueList) || (pInstXFuncN->lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }

                  if (pInstXFuncN->lists.pQueueFrgList)
                  {
                     pQFrglistener = pInstXFuncN->lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pInstXFuncN->lists.pFuncFrgList)
                  {
                     pFFrglistener = pInstXFuncN->lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }

               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *) iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                      pUriFuncName);
               if (pAinstFuncN)
               {
                  if ((pAinstFuncN->lists.pQueueList) || (pAinstFuncN->lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
                  
                  if (pAinstFuncN->lists.pQueueFrgList)
                  {
                     pQFrglistener = pAinstFuncN->lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pAinstFuncN->lists.pFuncFrgList)
                  {
                     pFFrglistener = pAinstFuncN->lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }
               
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *) iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                      pUriInstName);
               if (pInstXaFunc)
               {
                  if ((pInstXaFunc->lists.pQueueList) || (pInstXaFunc->lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
                  
                  if (pInstXaFunc->lists.pQueueFrgList)
                  {
                     pQFrglistener = pInstXaFunc->lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pInstXaFunc->lists.pFuncFrgList)
                  {
                     pFFrglistener = pInstXaFunc->lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }
               break;

            /* Received user URI addressed to all instances of a given function? */
            case AINST_FUNCN_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
               {
                  if ((iptStrcmp(pInstXFuncN[i].funcName,pUriFuncName)) == 0)
                  {
                     if ((pInstXFuncN[i].lists.pQueueList) || (pInstXFuncN[i].lists.pFuncList))
                     {
                        if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                        {
                           IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                        }
                        return(1);
                     }
                     
                     if (pInstXFuncN[i].lists.pQueueFrgList)
                     {
                        pQFrglistener = pInstXFuncN[i].lists.pQueueFrgList;
                        while (pQFrglistener)
                        {
                           pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                      (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                           if (pFrgItem)
                           {
                              if (pFrgItem->frgState == FRG_LEADER)
                              {
                                 if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                                 {
                                    IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                                 }
                                 return(1);
                              }
                           }
                           pQFrglistener = pQFrglistener->pNext;
                        }
                     }
                     
                     if (pInstXFuncN[i].lists.pFuncFrgList)
                     {
                        pFFrglistener = pInstXFuncN[i].lists.pFuncFrgList;
                        while (pFFrglistener)
                        {
                           pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                      (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                           if (pFrgItem)
                           {
                              if (pFrgItem->frgState == FRG_LEADER)
                              {
                                 if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                                 {
                                    IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                                 }
                                 return(1);
                              }
                           }
                           pFFrglistener = pFFrglistener->pNext;
                        }
                     }
                  }
               }
               
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *) iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                      pUriFuncName);
               if (pAinstFuncN)
               {
                  if ((pAinstFuncN->lists.pQueueList) || (pAinstFuncN->lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
                  
                  if (pAinstFuncN->lists.pQueueFrgList)
                  {
                     pQFrglistener = pAinstFuncN->lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pAinstFuncN->lists.pFuncFrgList)
                  {
                     pFFrglistener = pAinstFuncN->lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }
               
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *)pListTables->instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXaFuncListTableHdr.nItems ; i++)
               {
                  if ((pInstXaFunc[i].lists.pQueueList) || (pInstXaFunc[i].lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
                  
                  if (pInstXaFunc[i].lists.pQueueFrgList)
                  {
                     pQFrglistener = pInstXaFunc[i].lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pInstXaFunc[i].lists.pFuncFrgList)
                  {
                     pFFrglistener = pInstXaFunc[i].lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }
               break;
      
            /* Received user URI addressed to a given instance of all functions? */
            case INSTX_AFUNC_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
               {
                  if ((iptStrcmp(pInstXFuncN[i].instName,pUriInstName)) == 0)
                  {
                     if ((pInstXFuncN[i].lists.pQueueList) || (pInstXFuncN[i].lists.pFuncList))
                     {
                        if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                        {
                           IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                        }
                        return(1);
                     }
                     
                     if (pInstXFuncN[i].lists.pQueueFrgList)
                     {
                        pQFrglistener = pInstXFuncN[i].lists.pQueueFrgList;
                        while (pQFrglistener)
                        {
                           pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                      (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                           if (pFrgItem)
                           {
                              if (pFrgItem->frgState == FRG_LEADER)
                              {
                                 if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                                 {
                                    IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                                 }
                                 return(1);
                              }
                           }
                           pQFrglistener = pQFrglistener->pNext;
                        }
                     }
                     
                     if (pInstXFuncN[i].lists.pFuncFrgList)
                     {
                        pFFrglistener = pInstXFuncN[i].lists.pFuncFrgList;
                        while (pFFrglistener)
                        {
                           pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                      (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                           if (pFrgItem)
                           {
                              if (pFrgItem->frgState == FRG_LEADER)
                              {
                                 if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                                 {
                                    IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                                 }
                                 return(1);
                              }
                           }
                           pFFrglistener = pFFrglistener->pNext;
                        }
                     }
                  }
               }
              
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *)pListTables->aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->aInstFuncNListTableHdr.nItems ; i++)
               {
                  if ((pAinstFuncN[i].lists.pQueueList) || (pAinstFuncN[i].lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
                  
                  if (pAinstFuncN[i].lists.pQueueFrgList)
                  {
                     pQFrglistener = pAinstFuncN[i].lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pAinstFuncN[i].lists.pFuncFrgList)
                  {
                     pFFrglistener = pAinstFuncN[i].lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }
              
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *) iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                      pUriInstName);
               if (pInstXaFunc)
               {
                  if ((pInstXaFunc->lists.pQueueList) || (pInstXaFunc->lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
                  
                  if (pInstXaFunc->lists.pQueueFrgList)
                  {
                     pQFrglistener = pInstXaFunc->lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pInstXaFunc->lists.pFuncFrgList)
                  {
                     pFFrglistener = pInstXaFunc->lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }
               break;

            case AINST_AFUNC_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
               {
                  if ((pInstXFuncN[i].lists.pQueueList) || (pInstXFuncN[i].lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
                  
                  if (pInstXFuncN[i].lists.pQueueFrgList)
                  {
                     pQFrglistener = pInstXFuncN[i].lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pInstXFuncN[i].lists.pFuncFrgList)
                  {
                     pFFrglistener = pInstXFuncN[i].lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }
               
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *)pListTables->aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->aInstFuncNListTableHdr.nItems ; i++)
               {
                  if ((pAinstFuncN[i].lists.pQueueList) || (pAinstFuncN[i].lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
                  
                  if (pAinstFuncN[i].lists.pQueueFrgList)
                  {
                     pQFrglistener = pAinstFuncN[i].lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */

                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pAinstFuncN[i].lists.pFuncFrgList)
                  {
                     pFFrglistener = pAinstFuncN[i].lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */

                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }
               
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *)pListTables->instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXaFuncListTableHdr.nItems ; i++)
               {
                  if ((pInstXaFunc[i].lists.pQueueList) || (pInstXaFunc[i].lists.pFuncList))
                  {
                     if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                     return(1);
                  }
                  
                  if (pInstXaFunc[i].lists.pQueueFrgList)
                  {
                     pQFrglistener = pInstXaFunc[i].lists.pQueueFrgList;
                     while (pQFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pQFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */

                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pQFrglistener = pQFrglistener->pNext;
                     }
                  }
                  
                  if (pInstXaFunc[i].lists.pFuncFrgList)
                  {
                     pFFrglistener = pInstXaFunc[i].lists.pFuncFrgList;
                     while (pFFrglistener)
                     {
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pFFrglistener->pRedFuncRef));/*lint !e826  Ignore casting warning */

                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
                              {
                                 IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                              }
                              return(1);
                           }
                        }
                        pFFrglistener = pFFrglistener->pNext;
                     }
                  }
               }
               break;

            default:
               break;
         }
      }
      if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "mdAnyQueueListener: IPTVosGetSem ERROR\n");
   }
   return(0);
}

/*******************************************************************************
NAME:     mdPutMsgOnListenerQueue 
ABSTRACT: Put a received message on listerners queues.
          If there is more listeners left a buffer for the message data is
          created and data is copied.
RETURNS:  0 if OK, !=0 if not.
*/
int mdPutMsgOnListenerQueue(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,             /* Simualted IP address */
#endif
   UINT32 lastRecMsgNo,
   UINT32 dataLength,             /* Length of received data */
   char   **ppMsgData,            /* Pointer to pointer to received data */
   QUEUE_MSG *pMsg,               /* Buffer for received message */
   int    uriType,                /* Type of received user part URI */
   char   *pUriInstName,          /* Pointer to URI instance name */
   char   *pUriFuncName,          /* Pointer to URI function name */
   int    *pCallBackListener)      /* Flag set if a call-back function listener found */
{
   char *pTemp;
   int ret;
   int res;
   int callBackListenerFound = 0;  /* Flag set if there is any call-back
                                  function listerner for the receive message */
   UINT32 i;
   COMID_ITEM    *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   LISTENER_QUEUE *pPrevQueue = NULL;
   LISTERNER_TABLES *pListTables;
#ifdef TARGET_SIMU
   SIMU_DEV_ITEM *pSimDevItem;
#endif

   ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
#ifdef TARGET_SIMU
      if (simuIpAddr == IPTCom_getOwnIpAddr())
      {
         pListTables = &IPTGLOBAL(md.listTables);
      }
      else
      {
         pSimDevItem = (SIMU_DEV_ITEM *) ((void *)iptTabFind((const IPT_TAB_HDR *)&IPTGLOBAL(md.simuDevListTableHdr), simuIpAddr));
         if (pSimDevItem)
         {
            pListTables = &pSimDevItem->listTables;
         }
         else
         {
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            *pCallBackListener = 0;
            return((int)IPT_ERROR);
         }
      }
#else
      pListTables = &IPTGLOBAL(md.listTables);
#endif

      pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&pListTables->comidListTableHdr,
                                                      pMsg->msgInfo.comId));/*lint !e826  Ignore casting warning */
      if (pComIdItem)
      {
         if ((pComIdItem->lists.pFuncList) || (pComIdItem->lists.pFuncFrgList))
         {
            callBackListenerFound = 1;  
         }

         pComIdItem->lists.counted = 0;
         putMsgOnNormListQueue(&pComIdItem->lists, lastRecMsgNo, &pPrevQueue,
                                     dataLength, ppMsgData, pMsg); 

         pComIdItem->lists.frgCounted = 0;
         putMsgOnFrgListQueue(&pComIdItem->lists, lastRecMsgNo, &pPrevQueue,
                                     dataLength, ppMsgData, pMsg); 

      }
      
      if (uriType != NO_USER_URI)
      {
         if ((pListTables->aInstAfunc.pFuncList) || (pListTables->aInstAfunc.pFuncFrgList))
         {
            callBackListenerFound = 1;
         }
         
         pListTables->aInstAfunc.counted = 0;
         putMsgOnNormListQueue(&pListTables->aInstAfunc, lastRecMsgNo, &pPrevQueue,
                                     dataLength, ppMsgData, pMsg); 

         pListTables->aInstAfunc.frgCounted = 0;
         putMsgOnFrgListQueue(&pListTables->aInstAfunc, lastRecMsgNo, &pPrevQueue,
                                     dataLength, ppMsgData, pMsg); 

         switch (uriType)
         {
            /* Received user URI addressed to given instances of a given function */
            case INSTX_FUNCN_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2Find(&pListTables->instXFuncNListTableHdr,
                                                                      pUriInstName, pUriFuncName));
               if (pInstXFuncN)
               {
                  if ((pInstXFuncN->lists.pFuncList) || (pInstXFuncN->lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pInstXFuncN->lists.counted = 0;
                  putMsgOnNormListQueue(&pInstXFuncN->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pInstXFuncN->lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pInstXFuncN->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }
               
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *) iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                      pUriFuncName);
               if (pAinstFuncN)
               {
                  if ((pAinstFuncN->lists.pFuncList) || (pAinstFuncN->lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pAinstFuncN->lists.counted = 0;
                  putMsgOnNormListQueue(&pAinstFuncN->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pAinstFuncN->lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pAinstFuncN->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }
              
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *) iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                      pUriInstName);
               if (pInstXaFunc)
               {
                  if ((pInstXaFunc->lists.pFuncList) || (pInstXaFunc->lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pInstXaFunc->lists.counted = 0;
                  putMsgOnNormListQueue(&pInstXaFunc->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pInstXaFunc->lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pInstXaFunc->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }
               break;

            /* Received user URI addressed to all instances of a given function? */
            case AINST_FUNCN_URI:

               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
               {
                  if ((iptStrcmp(pInstXFuncN[i].funcName,pUriFuncName)) == 0)
                  {
                     if ((pInstXFuncN[i].lists.pFuncList) || (pInstXFuncN[i].lists.pFuncFrgList))
                     {
                        callBackListenerFound = 1;
                     }
                    
                     pInstXFuncN[i].lists.counted = 0;
                     putMsgOnNormListQueue(&pInstXFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                                 dataLength, ppMsgData, pMsg); 

                     pInstXFuncN[i].lists.frgCounted = 0;
                     putMsgOnFrgListQueue(&pInstXFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                                 dataLength, ppMsgData, pMsg); 
                  }
               }
              
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *) iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                      pUriFuncName);
               if (pAinstFuncN)
               {
                  if ((pAinstFuncN->lists.pFuncList) || (pAinstFuncN->lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pAinstFuncN->lists.counted = 0;
                  putMsgOnNormListQueue(&pAinstFuncN->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pAinstFuncN->lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pAinstFuncN->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }
              
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *)pListTables->instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXaFuncListTableHdr.nItems ; i++)
               {
                  if ((pInstXaFunc[i].lists.pFuncList) || (pInstXaFunc[i].lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pInstXaFunc[i].lists.counted = 0;
                  putMsgOnNormListQueue(&pInstXaFunc[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pInstXaFunc[i].lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pInstXaFunc[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }
               
               break;
      
            /* Received user URI addressed to a given instance of all functions? */
            case INSTX_AFUNC_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
               {
                  if ((iptStrcmp(pInstXFuncN[i].instName,pUriInstName)) == 0)
                  {
                     if ((pInstXFuncN[i].lists.pFuncList) || (pInstXFuncN[i].lists.pFuncFrgList))
                     {
                        callBackListenerFound = 1;
                     }
                    
                     pInstXFuncN[i].lists.counted = 0;
                     putMsgOnNormListQueue(&pInstXFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                                 dataLength, ppMsgData, pMsg); 

                     pInstXFuncN[i].lists.frgCounted = 0;
                     putMsgOnFrgListQueue(&pInstXFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                                 dataLength, ppMsgData, pMsg); 
                  }
               }
              
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *)pListTables->aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->aInstFuncNListTableHdr.nItems ; i++)
               {
                  if ((pAinstFuncN[i].lists.pFuncList) || (pAinstFuncN[i].lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pAinstFuncN[i].lists.counted = 0;
                  putMsgOnNormListQueue(&pAinstFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pAinstFuncN[i].lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pAinstFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }
              
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *) iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                      pUriInstName);
               if (pInstXaFunc)
               {
                  if ((pInstXaFunc->lists.pFuncList) || (pInstXaFunc->lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pInstXaFunc->lists.counted = 0;
                  putMsgOnNormListQueue(&pInstXaFunc->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pInstXaFunc->lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pInstXaFunc->lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }

               break;

            case AINST_AFUNC_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
               {
                  if ((pInstXFuncN[i].lists.pFuncList) || (pInstXFuncN[i].lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pInstXFuncN[i].lists.counted = 0;
                  putMsgOnNormListQueue(&pInstXFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pInstXFuncN[i].lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pInstXFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }
               
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *)pListTables->aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->aInstFuncNListTableHdr.nItems ; i++)
               {
                  if ((pAinstFuncN[i].lists.pFuncList) || (pAinstFuncN[i].lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pAinstFuncN[i].lists.counted = 0;
                  putMsgOnNormListQueue(&pAinstFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pAinstFuncN[i].lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pAinstFuncN[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }
              
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *)pListTables->instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXaFuncListTableHdr.nItems ; i++)
               {
                  if ((pInstXaFunc[i].lists.pFuncList) || (pInstXaFunc[i].lists.pFuncFrgList))
                  {
                     callBackListenerFound = 1;
                  }
                 
                  pInstXaFunc[i].lists.counted = 0;
                  putMsgOnNormListQueue(&pInstXaFunc[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 

                  pInstXaFunc[i].lists.frgCounted = 0;
                  putMsgOnFrgListQueue(&pInstXaFunc[i].lists, lastRecMsgNo, &pPrevQueue,
                                              dataLength, ppMsgData, pMsg); 
               }
               
               break;

            default:
               break;
         }
      }
   
      /* Put the message on the last found queue */
      if (pPrevQueue)
      {
         pMsg->pMsgData = *ppMsgData;
   
         /* Call-back  listener found? */
         if (callBackListenerFound)
         {
            /* Allocate a new buffer for the received data */
            pTemp = (char *)IPTVosMalloc(dataLength);
            if (pTemp)
            {
               /* Copy the data */
               memcpy(pTemp,*ppMsgData,dataLength); 
            }
            else
            {
               ret = (int)IPT_ERROR;
               IPTVosPrint1(IPT_ERR,
                            "putMsgOnListenerQueue: Out of memory. Requested size=%d\n",
                            dataLength);
            }
            *ppMsgData = pTemp;
         }
         else
         {
            *ppMsgData = 0;   
         }

         pMsg->msgInfo.pCallerRef = (void *)pPrevQueue->pCallerRef;
        
         res = IPTVosSendMsgQueue((IPT_QUEUE *)((void *)&pPrevQueue->listenerQueueId),
                                  (char *)pMsg, sizeof(QUEUE_MSG));
         if (res != (int)IPT_OK)
         {
            ret = (int)IPT_ERROR;

            pTemp = getQueueItemName(pPrevQueue->listenerQueueId);
            if (uriType == NO_USER_URI)
            {
               IPTVosPrint4(IPT_ERR,
                     "ERROR sending message on ComId listener queue ID=%#x Name=%s. Msg type=%d comid=%d\n",
                            pPrevQueue->listenerQueueId, 
                            (pTemp != NULL)?pTemp:"None",
                            pMsg->msgInfo.msgType, 
                            pMsg->msgInfo.comId );
            }
            else
            {
               IPTVosPrint6(IPT_ERR,
                     "ERROR sending message on listener queue ID=%#x Name=%s. Msg type=%d comid=%d User URI=%s.%s\n",
                            pPrevQueue->listenerQueueId,
                            (pTemp != NULL)?pTemp:"None",
                            pMsg->msgInfo.msgType, 
                            pMsg->msgInfo.comId, pUriInstName, pUriFuncName );
            }
      
            res = IPTVosFree((unsigned char *)pMsg->pMsgData);
            if(res != 0)
            {
              IPTVosPrint1(IPT_ERR,
                           "trReceive failed to free data memory, code=%d\n",res);
            }
         }
      }
      if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
     
      *pCallBackListener = callBackListenerFound;
   }
   else
   {
      *pCallBackListener = 1;

      IPTVosPrint0(IPT_ERR, "mdPutMsgOnListenerQueue: IPTVosGetSem ERROR\n");
   }
   return(ret);
}

/*******************************************************************************
NAME:     mdPutMsgOnListenerFunc 
ABSTRACT: Put a received message on listerners queues.
          If there is more listeners left a buffer for the message data is
          created and data is copied.
RETURNS:  -
*/
void mdPutMsgOnListenerFunc(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,             /* Simualted IP address */
#endif
   UINT32 lastRecMsgNo,
   UINT32 dataLength,             /* Length of received data */
   char   *pMsgData,            /* Pointer to pointer to received data */
   MSG_INFO *pMsgInfo,    /* Message info */
   int    uriType,                /* Type of received user part URI */
   char   *pUriInstName,          /* Pointer to URI instance name */
   char   *pUriFuncName)          /* Pointer to URI function name */
{
   int ret;
   UINT32 i;
   COMID_ITEM    *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   LISTERNER_TABLES *pListTables;
#ifdef TARGET_SIMU
   SIMU_DEV_ITEM *pSimDevItem;
#endif

   ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
#ifdef TARGET_SIMU
      if (simuIpAddr == IPTCom_getOwnIpAddr())
      {
         pListTables = &IPTGLOBAL(md.listTables);
      }
      else
      {
         pSimDevItem = (SIMU_DEV_ITEM *) ((void *)iptTabFind((const IPT_TAB_HDR *)&IPTGLOBAL(md.simuDevListTableHdr), simuIpAddr));
         if (pSimDevItem)
         {
            pListTables = &pSimDevItem->listTables;
         }
         else
         {
            if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return;
         }
      }
#else
      pListTables = &IPTGLOBAL(md.listTables);
#endif
      pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&pListTables->comidListTableHdr,
                                                      pMsgInfo->comId));/*lint !e826  Ignore casting warning */
      if (pComIdItem)
      {
         putMsgOnNormListFunc(&pComIdItem->lists, lastRecMsgNo,
                                     dataLength, pMsgData, pMsgInfo); 

         putMsgOnFrgListFunc(&pComIdItem->lists, lastRecMsgNo,
                                     dataLength, pMsgData, pMsgInfo); 
      }
      
      if (uriType != NO_USER_URI)
      {
         putMsgOnNormListFunc(&pListTables->aInstAfunc, lastRecMsgNo,
                                     dataLength, pMsgData, pMsgInfo); 

         putMsgOnFrgListFunc(&pListTables->aInstAfunc, lastRecMsgNo,
                                     dataLength, pMsgData, pMsgInfo); 

         switch (uriType)
         {
            /* Received user URI addressed to given instances of a given function */
            case INSTX_FUNCN_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2Find(&pListTables->instXFuncNListTableHdr,
                                                                      pUriInstName, pUriFuncName));
               if (pInstXFuncN)
               {
                  putMsgOnNormListFunc(&pInstXFuncN->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pInstXFuncN->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }
               
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *) iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                      pUriFuncName);
               if (pAinstFuncN)
               {
                  putMsgOnNormListFunc(&pAinstFuncN->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pAinstFuncN->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }
              
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *) iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                      pUriInstName);
               if (pInstXaFunc)
               {
                  putMsgOnNormListFunc(&pInstXaFunc->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pInstXaFunc->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }
               break;

            /* Received user URI addressed to all instances of a given function? */
            case AINST_FUNCN_URI:

               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
               {
                  if ((iptStrcmp(pInstXFuncN[i].funcName,pUriFuncName)) == 0)
                  {
                     putMsgOnNormListFunc(&pInstXFuncN[i].lists, lastRecMsgNo,
                                                 dataLength, pMsgData, pMsgInfo); 

                     putMsgOnFrgListFunc(&pInstXFuncN[i].lists, lastRecMsgNo,
                                                 dataLength, pMsgData, pMsgInfo); 
                  }
               }
              
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *) iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                      pUriFuncName);
               if (pAinstFuncN)
               {
                  putMsgOnNormListFunc(&pAinstFuncN->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pAinstFuncN->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }
              
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *)pListTables->instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXaFuncListTableHdr.nItems ; i++)
               {
                  putMsgOnNormListFunc(&pInstXaFunc[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pInstXaFunc[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }
               
               break;
      
            /* Received user URI addressed to a given instance of all functions? */
            case INSTX_AFUNC_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
               {
                  if ((iptStrcmp(pInstXFuncN[i].instName,pUriInstName)) == 0)
                  {
                     putMsgOnNormListFunc(&pInstXFuncN[i].lists, lastRecMsgNo,
                                                 dataLength, pMsgData, pMsgInfo); 

                     putMsgOnFrgListFunc(&pInstXFuncN[i].lists, lastRecMsgNo,
                                                 dataLength, pMsgData, pMsgInfo); 
                  }
               }
              
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *)pListTables->aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->aInstFuncNListTableHdr.nItems ; i++)
               {
                  putMsgOnNormListFunc(&pAinstFuncN[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pAinstFuncN[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }
              
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *) iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                      pUriInstName);
               if (pInstXaFunc)
               {
                  putMsgOnNormListFunc(&pInstXaFunc->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pInstXaFunc->lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }

               break;

            case AINST_AFUNC_URI:
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
               {
                  putMsgOnNormListFunc(&pInstXFuncN[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pInstXFuncN[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }
               
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *)pListTables->aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->aInstFuncNListTableHdr.nItems ; i++)
               {
                  putMsgOnNormListFunc(&pAinstFuncN[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pAinstFuncN[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }
              
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *)pListTables->instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
               for (i = 0; i < pListTables->instXaFuncListTableHdr.nItems ; i++)
               {
                  putMsgOnNormListFunc(&pInstXaFunc[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 

                  putMsgOnFrgListFunc(&pInstXaFunc[i].lists, lastRecMsgNo,
                                              dataLength, pMsgData, pMsgInfo); 
               }
               
               break;
            
            default:
               break;
         }
      }
      if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "mdPutMsgOnListenerQueue: IPTVosGetSem ERROR\n");
   }
   return;
}

/*******************************************************************************
NAME:     MDComAPI_comIdListener
ABSTRACT: Add a listener for message data with the given comid's. Join 
          multicast groups for comid's with multicast destinations. 
          IPTCom will put received messages on the queue 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_comIdListener(
   MD_QUEUE        listenerQueueId, /* Queue ID */
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef,     /* Caller reference */
   const UINT32    comid[],         /* Array with comid's, ended by 0 */
   const void      *pRedFuncRef,    /* Redundancy function reference */
   UINT32          destId,          /* Destination URI ID */
   const char      *pDestUri)       /* Pointer to destination URI */
#ifdef TARGET_SIMU            
{ 
   char empty[] = {0};
   return(MDComAPI_comIdListenerSim(listenerQueueId, func, pCallerRef, comid, pRedFuncRef, destId, pDestUri, empty));   
}

int MDComAPI_comIdListenerSim(
   MD_QUEUE        listenerQueueId, /* Queue ID */
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef,     /* Caller reference */
   const UINT32    comid[],         /* Array with comid's, ended by 0 */
   const void      *pRedFuncRef,    /* Redundancy function reference */
   UINT32          destId,          /* Destination URI ID */
   const char      *pDestUri,       /* Pointer to destination URI */
   const char      *pSimURI)        /* Host URI of simulated device */
#endif   
{
   int res = IPT_OK;
   int i;
   int destIdConfigured = 0;
   UINT32         destIpAddr;
   COMID_ITEM    *pComIdItem;
   COMID_ITEM    comIdItem;
   LISTERNER_TABLES *pListTables;
   
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;            
   SIMU_DEV_ITEM  *pSimDevItem;
#endif

#ifdef LINUX_MULTIPROC
   if (func != 0)
   {
      IPTVosPrint0(IPT_ERR,
         "Failed to add Comid listener. Call-back function not allowed for Linux multi process\n");
      return((int)IPT_INVALID_PAR);
   }
#endif
  
   if ((!comid) || (comid[0] == 0))
   {
      IPTVosPrint0(IPT_ERR,
         "Failed to add Comid listener. No ComId\n");
      return((int)IPT_INVALID_PAR);
   }

#ifdef TARGET_SIMU
   if (pSimURI == NULL)
   {
      IPTVosPrint0(IPT_ERR,
         "Failed to add Comid listener. NULL pointer to simulated URI\n");
      return((int)IPT_INVALID_PAR);
   }
#endif

   if (IPTGLOBAL(md.mdComInitiated))
   {
      res = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (res == IPT_OK)
      {
#ifdef TARGET_SIMU
         if (pSimURI[0] == 0)
         {
            pListTables = &IPTGLOBAL(md.listTables);
            simuIpAddr = IPTCom_getOwnIpAddr();
         }
         else
         {
            /* search for simulated device */
            pSimDevItem = getSimDev(pSimURI);
            if (pSimDevItem)
            {
               simuIpAddr = pSimDevItem->simuIpAddr;
               pListTables = &pSimDevItem->listTables;
            }
            else
            {
               res = (int)IPT_ERROR;
            }
         }
         if (res == IPT_OK)
         {
#else
         pListTables = &IPTGLOBAL(md.listTables);
#endif
         
         for (i=0; comid[i] != 0; i++)
         {
            pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&pListTables->comidListTableHdr,
                                                            comid[i]));/*lint !e826  Ignore casting warning */

            /* Not found? */
            if (!pComIdItem)
            {
               comIdItem.lists.pQueueList = NULL;
               comIdItem.lists.pQueueFrgList = NULL;
               comIdItem.lists.pFuncList = NULL;
               comIdItem.lists.pFuncFrgList = NULL;
               comIdItem.lists.mdInPackets = 0;
               comIdItem.lists.mdFrgInPackets = 0;
               comIdItem.keyComId = comid[i];

               /* Add the ComId to the queue listeners ComId table */
               res = iptTabAdd(&pListTables->comidListTableHdr, (IPT_TAB_ITEM_HDR *)((void *)&comIdItem));
               if (res != IPT_OK)
               {
                  IPTVosPrint2(IPT_ERR,
                     "Failed to add Comid=%d listener. Failed to add to table. Error=%#x\n",
                     comid[i], res);
                  break; /* Break for loop */
               }

               pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&pListTables->comidListTableHdr,
                                                               comid[i]));/*lint !e826  Ignore casting warning */
               if (!pComIdItem)
               {
                  IPTVosPrint1(IPT_ERR,
                     "Failed to add Comid=%d listener. Failed to add to table.\n",
                     comid[i]);
                  res = IPT_ERROR;
                  break;
               }
            }

            /* Find destination address for comId */
            res = getComIdDestIPAddr(
#ifdef TARGET_SIMU
                                     simuIpAddr,
#endif
                                     comid[i], destId, pDestUri, &destIpAddr);
            if (destId != 0)
            {
               if ((res == IPT_OK))
               {
                  destIdConfigured = 1;
               }
            }
            
            res = addListener(listenerQueueId, func, pCallerRef, pRedFuncRef,
                              destIpAddr, comid[i], destId, pDestUri,
                              &pComIdItem->lists);
            if (res != IPT_OK)
            {
               IPTVosPrint1(IPT_ERR,
                  "Failed to add Comid=%d listener. Failed to add to table.\n",
                  comid[i]);
               break;
            }                   
         }

         if ((destId != 0) && (destIdConfigured == 0))
         {
            res = (int)IPT_INVALID_PAR;

            IPTVosPrint1(IPT_ERR,
               "Failed to add Comid listener. None of the ComId's has destination ID=%d configured\n",
               destId);
         }
 
         if (res != IPT_OK)
         {
            if (pRedFuncRef)
            {
               for (i--; i>=0; i--)
               {
                  pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&pListTables->comidListTableHdr,
                                                                  comid[i]));/*lint !e826  Ignore casting warning */
                  if (pComIdItem)
                  {
#ifdef TARGET_SIMU
                     removeQueueFrgListener(simuIpAddr, listenerQueueId, 0, pCallerRef, &pComIdItem->lists); 
                     removeFuncFrgListener(simuIpAddr, func, 0, pCallerRef, &pComIdItem->lists); 
#else
                     removeQueueFrgListener(listenerQueueId, 0, pCallerRef, &pComIdItem->lists); 
                     removeFuncFrgListener(func, 0, pCallerRef, &pComIdItem->lists); 
#endif
                     if ((pComIdItem->lists.pQueueList == NULL) &&
                         (pComIdItem->lists.pQueueFrgList == NULL) &&
                         (pComIdItem->lists.pFuncList == NULL) &&
                         (pComIdItem->lists.pFuncFrgList == NULL))
                     {
                        res = iptTabDelete(&pListTables->comidListTableHdr, pComIdItem->keyComId);
                        if (res != IPT_OK)
                        {
                           IPTVosPrint2(IPT_ERR,
                                       "Failed to add Comid listener. Failed to delete listener for comId=%d, res=%d\n",
                                       pComIdItem->keyComId, res);
                        } 
                     } 
                  }
               }
            }
            else
            {
               for (i--; i>=0; i--)
               {
                  pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&pListTables->comidListTableHdr,
                                                                  comid[i]));/*lint !e826  Ignore casting warning */
                  if (pComIdItem)
                  {
#ifdef TARGET_SIMU
                     removeQueueNormListener(simuIpAddr, listenerQueueId, 0, pCallerRef, &pComIdItem->lists); 
                     removeFuncNormListener(simuIpAddr, func, 0, pCallerRef, &pComIdItem->lists); 
#else
                     removeQueueNormListener(listenerQueueId, 0, pCallerRef, &pComIdItem->lists); 
                     removeFuncNormListener(func, 0, pCallerRef, &pComIdItem->lists); 
#endif
                     if ((pComIdItem->lists.pQueueList == NULL) &&
                         (pComIdItem->lists.pQueueFrgList == NULL) &&
                         (pComIdItem->lists.pFuncList == NULL) &&
                         (pComIdItem->lists.pFuncFrgList == NULL))
                     {
                        res = iptTabDelete(&pListTables->comidListTableHdr, pComIdItem->keyComId);
                        if (res != IPT_OK)
                        {
                           IPTVosPrint2(IPT_ERR,
                                       "Failed to add Comid listener. Failed to delete listener for comId=%d, res=%d\n",
                                       pComIdItem->keyComId, res);
                        } 
                     } 
                  }
               }
            }
         }
#ifdef TARGET_SIMU
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "Failed to add Comid listener. Couldn't get simulated device\n");
         }
#endif
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "Failed to add Comid listener. IPTVosGetSem ERROR\n");
      }
   }
   else
   {
      res = (int)IPT_MD_NOT_INIT;
   }
   return(res);
}

/*******************************************************************************
NAME:     MDComAPI_addComIdListenerQ
ABSTRACT: Add a listener for message data with the given comid's. Join 
          multicast groups for comid's with multicast destinations. 
          IPTCom will put received messages on the queue 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_addComIdListenerQ(
   MD_QUEUE     listenerQueueId, /* Queue ID */
   const void   *pCallerRef,     /* Caller reference */
   const UINT32 comid[])         /* Array with comid's, ended by 0 */
{
   return(MDComAPI_comIdListener(listenerQueueId, 0, pCallerRef, comid, NULL,
                                 0, NULL));
}

#ifdef TARGET_SIMU            
int MDComAPI_addComIdListenerQSim(
   MD_QUEUE     listenerQueueId, /* Queue ID */
   const void   *pCallerRef,     /* Caller reference */
   const UINT32 comid[],         /* Array with comid's, ended by 0 */
   const char *pSimURI)          /* Host URI of simulated device */
{
   return(MDComAPI_comIdListenerSim(listenerQueueId, 0, pCallerRef, comid, NULL,
                                 0, NULL, pSimURI));
}
#endif


#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_addComIdListenerF
ABSTRACT: Add a listener for message data with the given comid's. Join 
          multicast groups for comid's with multicast destinations. 
          IPTCom will call the call-back function when messages are received 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_addComIdListenerF(
   IPT_REC_FUNCPTR func,        /* Pointer to callback function */
   const void      *pCallerRef, /* Caller reference */
   const UINT32    comid[])     /* Array with comid's, ended by 0 */
{
   return(MDComAPI_comIdListener(0, func, pCallerRef, comid, NULL,
                                 0, NULL));
}

#ifdef TARGET_SIMU            
int MDComAPI_addComIdListenerFSim(
   IPT_REC_FUNCPTR func,        /* Pointer to callback function */
   const void      *pCallerRef, /* Caller reference */
   const UINT32    comid[],     /* Array with comid's, ended by 0 */
   const char *pSimURI)          /* Host URI of simulated device */
{
   return(MDComAPI_comIdListenerSim(0, func, pCallerRef, comid, NULL,
                                 0, NULL, pSimURI));
}
#endif
#endif

/*******************************************************************************
NAME:     MDComAPI_addFrgComIdListenerQ
ABSTRACT: Add a listener for message data with the given comid's. 
          Join multicast groups for comid's with multicast destinations. 
          IPTCom will put received messages on the queue. 
          The reception of messages with the given comid's can be 
          blocked/unblocked by call of the MDComAPI_blockFrgListener or 
          MDComAPI_unblockFrgListener.
          If the application has not called the MDComAPI_unblockFrgListener 
          with the same pRedFuncRef value the listener will be in blocked state.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_addFrgComIdListenerQ(
   MD_QUEUE     listenerQueueId, /* Queue ID */
   const void   *pCallerRef,     /* Caller reference */
   const UINT32 comid[],         /* Array with comid's, ended by 0 */
   const void   *pRedFuncRef)    /* Redundancy function reference */
{
   return(MDComAPI_comIdListener(listenerQueueId, 0, pCallerRef, comid, pRedFuncRef,
                                 0, NULL));
}
#ifdef TARGET_SIMU            
int MDComAPI_addFrgComIdListenerQSim(
   MD_QUEUE     listenerQueueId, /* Queue ID */
   const void   *pCallerRef,     /* Caller reference */
   const UINT32 comid[],         /* Array with comid's, ended by 0 */
   const void   *pRedFuncRef,    /* Redundancy function reference */
   const char *pSimURI)          /* Host URI of simulated device */
{
   return(MDComAPI_comIdListenerSim(listenerQueueId, 0, pCallerRef, comid, pRedFuncRef,
                                 0, NULL, pSimURI));
}
#endif

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_addFrgComIdListenerF
ABSTRACT: Add a listener for message data with the given comid's. 
          Join multicast groups for comid's with multicast destinations. 
          IPTCom will call the call-back function when messages are received. 
          The reception of messages with the given comid's can be 
          blocked/unblocked by call of the MDComAPI_blockFrgListener or 
          MDComAPI_unblockFrgListener.
          If the application has not called the MDComAPI_unblockFrgListener with
          the same pRedFuncRef value the listener will be in blocked state.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_addFrgComIdListenerF(
   IPT_REC_FUNCPTR func,         /* Pointer to callback function */
   const void      *pCallerRef,  /* Caller reference */
   const UINT32    comid[],      /* Array with comid's, ended by 0 */
   const void      *pRedFuncRef) /* Redundancy function reference */
{
   return(MDComAPI_comIdListener(0, func, pCallerRef, comid, pRedFuncRef,
                                 0, NULL));
}

#ifdef TARGET_SIMU
int MDComAPI_addFrgComIdListenerFSim(
   IPT_REC_FUNCPTR func,         /* Pointer to callback function */
   const void      *pCallerRef,  /* Caller reference */
   const UINT32    comid[],      /* Array with comid's, ended by 0 */
   const void      *pRedFuncRef, /* Redundancy function reference */
   const char      *pSimURI)      /* Host URI of simulated device */
{
   return(MDComAPI_comIdListenerSim(0, func, pCallerRef, comid, pRedFuncRef,
                                 0, NULL, pSimURI));
}
#endif
#endif

/*******************************************************************************
NAME:     MDComAPI_addUriListenerQ
ABSTRACT: Add a listener for message data with the given destination URI. Join 
          multicast groups if a destination URI with a multicast destinations 
          host part is given. IPTCom will put received messages on the queue  
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_uriListener(
   MD_QUEUE        listenerQueueId, /* Queue ID */
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef,     /* Caller reference */
   UINT32          comId,           /* ComId */
   UINT32          destId,          /* Destination URI ID */
   const char      *pDestUri,       /* Pointer to destination URI string */
   const void      *pRedFuncRef)    /* Redundancy function reference */
#ifdef TARGET_SIMU
{ 
   char empty[] = {0};
   return(MDComAPI_uriListenerSim(listenerQueueId, func, pCallerRef, comId, destId, pDestUri, pRedFuncRef, empty));   
}

int MDComAPI_uriListenerSim(
   MD_QUEUE        listenerQueueId, /* Queue ID */
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef,     /* Caller reference */
   UINT32          comId,           /* ComId */
   UINT32          destId,          /* Destination URI ID */
   const char      *pDestUri,       /* Pointer to destination URI string */
   const void      *pRedFuncRef,    /* Redundancy function reference */
   const char      *pSimURI)           /* Host URI of simulated device */
#endif
{
   char uriInstName[IPT_MAX_LABEL_LEN];
   char uriFuncName[IPT_MAX_LABEL_LEN];
   char uriHostName[IPT_MAX_HOST_URI_LEN];
   char *pHostUri = NULL;
   int res;   
   UINT8 dummy;
   UINT32 destIpAddr = 0;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   INSTX_FUNCN_ITEM instXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   AINST_FUNCN_ITEM aInstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   INSTX_AFUNC_ITEM instXaFunc;
   LISTERNER_TABLES *pListTables;

#ifdef TARGET_SIMU
   UINT32 simuIpAddr;            
   SIMU_DEV_ITEM  *pSimDevItem;
#endif
   
   if (IPTGLOBAL(md.mdComInitiated))
   {
      if (pDestUri == 0)
      {
         IPTVosPrint0(IPT_ERR,
            "Failed to add URI listener. No dest URI\n");
         return((int)IPT_INVALID_PAR);
      }

#ifdef TARGET_SIMU
      if (pSimURI == NULL)
      {
         IPTVosPrint0(IPT_ERR,
            "Failed to add URI listener. NULL pointer to simulated URI\n");
         return((int)IPT_INVALID_PAR);
      }
#endif
      if ((res = extractUriInstFuncHost(pDestUri, uriInstName, uriFuncName, 
                                 uriHostName)) != (int)IPT_OK)
      {
         IPTVosPrint1(IPT_ERR,
            "Failed to add URI listener. wrong dest URI=%s\n",pDestUri);
         return(res);
      }

      res = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (res == IPT_OK)
      {
#ifdef TARGET_SIMU
         /* search for simulated device */
         if (pSimURI[0] == 0)
         {
            pListTables = &IPTGLOBAL(md.listTables);
            simuIpAddr = IPTCom_getOwnIpAddr();
         }
         else
         {
            /* search for simulated device */
            pSimDevItem = getSimDev(pSimURI);
            if (pSimDevItem)
            {
               simuIpAddr = pSimDevItem->simuIpAddr;
               pListTables = &pSimDevItem->listTables;
            }
            else
            {
               res = (int)IPT_ERROR;
            }
         }
         if (res == IPT_OK)
         {
#else
         pListTables = &IPTGLOBAL(md.listTables);
#endif
       
         if (comId != 0)
         {
               /* Find destination address for comId */
               res = getComIdDestIPAddr(
#ifdef TARGET_SIMU
                                        simuIpAddr,
#endif
                                        comId, destId, NULL, &destIpAddr);
               if (res != IPT_OK)
               {
                  if (destId != 0)
                  {
                     IPTVosPrint3(IPT_ERR,
                        "Failed to add URI listener=%s No configured destination Uri for dest Id=%d for ComId=%d\n",
                        pDestUri, destId, comId);
                  }
                  else
                  {
                     IPTVosPrint2(IPT_ERR,
                        "Failed to add URI listener=%s No configured destination Uri for ComId=%d\n",
                        pDestUri, comId);
                  }
               }
         }
         else
         {
            destId = 0;

            if (uriHostName[0] != 0)
            {
               pHostUri = uriHostName;

               /* Get IP address from IPTDir based on the host URI */
               dummy = 0;
#ifdef TARGET_SIMU
               if (pSimURI[0] == 0)
               {
                  res = iptGetAddrByName(uriHostName, &destIpAddr, &dummy);
               }
               else
               {
                  res = iptGetAddrByNameSim(uriHostName, simuIpAddr,  &destIpAddr, &dummy);
               }
#else
               res = iptGetAddrByName(uriHostName, &destIpAddr, &dummy);
#endif
               if (res == TDC_OK)
               {
                  if (isMulticastIpAddr(destIpAddr))
                  {
#if defined(IF_WAIT_ENABLE)
                     if (IPTGLOBAL(ifRecReadyMD))
                     {
                        /* join multicast address */
                        joinMDmulticastAddress(
      #ifdef TARGET_SIMU
                                               simuIpAddr,
      #endif
                                               destIpAddr);
                     }
                     else
                     {
                        /* Indicate that joining has to be done when ethernet interface is ready */
                        IPTGLOBAL(md.finish_addr_resolv) = 1;
                        destIpAddr = IP_ADDRESS_NOT_RESOLVED;
                     }
#else
                     /* join multicast address */
                     joinMDmulticastAddress(
   #ifdef TARGET_SIMU
                                            simuIpAddr,
   #endif
                                            destIpAddr);
#endif
                  }
               }
               else
               {
                  if ((res == TDC_NO_CONFIG_DATA) || (res == TDC_MUST_FINISH_INIT))
                  {
                     res = (int)IPT_OK;

                     /* Indicate that address resolving has to be done later when TDC 
                        has got data from IPTDir */
                     IPTGLOBAL(md.finish_addr_resolv) = 1;

                     /* Exit wait for TDC to be ready*/
                     destIpAddr = IP_ADDRESS_NOT_RESOLVED;
                  }
                  else
                  {
                     IPTVosPrint3(IPT_ERR,
                     "Failed to add URI listener=%s. Could not convert host URI=%s to IP address. TDC result=%#x\n",
                                  pDestUri, uriHostName, res);
                     destIpAddr = 0;
                  }
               }
            }
         }

         if (res == IPT_OK)
         {
            if ((iptStrcmp(uriInstName , "aInst")) == 0)
            {
               if ((iptStrcmp(uriFuncName , "aFunc")) == 0)
               {
                  res = addListener(listenerQueueId, func, pCallerRef, pRedFuncRef,
                                    destIpAddr, comId, destId, pHostUri,
                                    &pListTables->aInstAfunc); 
               }
               else /* aInst.funcN */
               {
                  pAinstFuncN = (AINST_FUNCN_ITEM *) iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                         uriFuncName);
                  if (!pAinstFuncN)
                  {
                     strcpy(aInstFuncN.funcName, uriFuncName);
                     aInstFuncN.lists.pQueueList = NULL;
                     aInstFuncN.lists.pQueueFrgList = NULL;
                     aInstFuncN.lists.pFuncList = NULL;
                     aInstFuncN.lists.pFuncFrgList = NULL;
                     aInstFuncN.lists.mdInPackets = 0;
                     aInstFuncN.lists.mdFrgInPackets = 0;
                     res = iptUriLabelTabAdd(&pListTables->aInstFuncNListTableHdr,
                                             (URI_LABEL_TAB_ITEM *)((void*)&aInstFuncN)); /*lint !e826 Type cast OK */
                     if (res != IPT_OK)
                     {
                        IPTVosPrint2(IPT_ERR,
                           "Failed to add URI listener=%s. Failed to add URI to table. Error=%#x\n",
                           pDestUri, res);
                     }
                     else
                     {
                        pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                               uriFuncName));
                        if (!pAinstFuncN)
                        {
                           IPTVosPrint1(IPT_ERR,
                              "Failed to add URI listener=%s. Failed to add URI to table.\n",
                              pDestUri);
                           res = IPT_ERROR;
                        }
                     }
                  }
                 
                  if (pAinstFuncN)
                  {
                     res = addListener(listenerQueueId, func, pCallerRef, pRedFuncRef,
                                       destIpAddr, comId,destId, pHostUri,
                                       &pAinstFuncN->lists); 
                  }
               }
            }
            else if ((iptStrcmp(uriFuncName , "aFunc")) == 0)
            {
               /* instx.aFunc */
               pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                      uriInstName));
               if (!pInstXaFunc)
               {
                  strcpy(instXaFunc.instName, uriInstName);
                  instXaFunc.lists.pQueueList = NULL;
                  instXaFunc.lists.pQueueFrgList = NULL;
                  instXaFunc.lists.pFuncList = NULL;
                  instXaFunc.lists.pFuncFrgList = NULL;
                  instXaFunc.lists.mdInPackets = 0;
                  instXaFunc.lists.mdFrgInPackets = 0;
                  res = iptUriLabelTabAdd(&pListTables->instXaFuncListTableHdr, (URI_LABEL_TAB_ITEM *)((void *)&instXaFunc)); /*lint !e826 Type cast OK */
                  if (res != IPT_OK)
                  {
                     IPTVosPrint2(IPT_ERR,
                        "Failed to add URI listener=%s Failed to add URI to table. Error=%#x\n",
                        pDestUri, res);
                  }
                  else
                  {
                     pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                            uriInstName));
                     if (!pInstXaFunc)
                     {
                        IPTVosPrint1(IPT_ERR,
                           "Failed to add URI listener=%s Failed to add URI to table.\n",
                           pDestUri);
                        res = IPT_ERROR;
                     }
                  }
               }

               if (pInstXaFunc)
               {
                  res = addListener(listenerQueueId, func, pCallerRef, pRedFuncRef,
                                    destIpAddr, comId, destId, pHostUri,
                                    &pInstXaFunc->lists); 
               }
            }
            else
            {
               /* instx.funcN */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2Find(&pListTables->instXFuncNListTableHdr,
                                                                      uriInstName, uriFuncName));
               if (!pInstXFuncN)
               {
                  strcpy(instXFuncN.instName, uriInstName);
                  strcpy(instXFuncN.funcName, uriFuncName);
                  instXFuncN.lists.pQueueList = NULL;
                  instXFuncN.lists.pQueueFrgList = NULL;
                  instXFuncN.lists.pFuncList = NULL;
                  instXFuncN.lists.pFuncFrgList = NULL;
                  instXFuncN.lists.mdInPackets = 0;
                  instXFuncN.lists.mdFrgInPackets = 0;
                  res = iptUriLabelTab2Add(&pListTables->instXFuncNListTableHdr, (URI_LABEL_TAB2_ITEM *)((void *)&instXFuncN));/*lint !e826 Type cast OK */
                  if (res != IPT_OK)
                  {
                     IPTVosPrint2(IPT_ERR,
                        "Failed to add URI listener=%s Failed to add URI to table. Error=%#x\n",
                        pDestUri, res);
                  }
                  else
                  {
                     pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2Find(&pListTables->instXFuncNListTableHdr,
                                                                            uriInstName, uriFuncName));
                     if (!pInstXFuncN)
                     {
                        IPTVosPrint1(IPT_ERR,
                           "Failed to add URI listener=%s Failed to add URI to table.\n",
                           pDestUri);
                        res = IPT_ERROR;
                     }
                  }
               }

               if (pInstXFuncN)
               {
                  res = addListener(listenerQueueId, func, pCallerRef, pRedFuncRef,
                                    destIpAddr, comId, destId, pHostUri,
                                    &pInstXFuncN->lists);
                  if (res != IPT_OK)
                  {
                        IPTVosPrint2(IPT_ERR,
                           "Failed to add URI listener=%s Error=%#x\n",
                           pDestUri, res);
                  }                   
               }
            }
         }
#ifdef TARGET_SIMU
         }
         else
         {
            IPTVosPrint1(IPT_ERR,
               "Failed to add URI listener=%s Error=%#x\n",
               pDestUri);
            res = (int)IPT_ERROR;
         }
#endif         
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint1(IPT_ERR, "Failed to add URI listener=%s IPTVosGetSem ERROR\n",
         pDestUri);
      }
   }
   else
   {
      res = (int)IPT_MD_NOT_INIT;
   }

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI_addUriListenerQ
ABSTRACT: Add a listener for message data with the given destination URI. Join 
          multicast groups if a destination URI with a multicast destinations 
          host part is given. IPTCom will put received messages on the queue  
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_addUriListenerQ(
   MD_QUEUE    listenerQueueId, /* Queue ID */
   const void  *pCallerRef,     /* Caller reference */
   const char  *pDestUri)       /* Pointer to destination URI string */
{
   return(MDComAPI_uriListener(listenerQueueId, NULL, pCallerRef, 0, 0, pDestUri, 0));
}
#ifdef TARGET_SIMU

int MDComAPI_addUriListenerQSim(
   MD_QUEUE    listenerQueueId, /* Queue ID */
   const void  *pCallerRef,     /* Caller reference */
   const char  *pDestUri,       /* Pointer to destination URI string */
   const char  *pSimURI)        /* Host URI of simulated device */
{
   return(MDComAPI_uriListenerSim(listenerQueueId, NULL, pCallerRef, 0, 0, pDestUri, 0, pSimURI));
}
#endif

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_addUriListenerF
ABSTRACT: Add a listener for message data with the given destination URI. 
          Join multicast groups if a destination URI with a multicast 
          destinations host part is given. 
          IPTCom will call the call-back function when messages are received 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_addUriListenerF(
   IPT_REC_FUNCPTR func,        /* Pointer to callback function */
   const void      *pCallerRef, /* Caller reference */
   const char      *pDestUri)   /* Pointer to destination URI string */
{
   return(MDComAPI_uriListener(0, func, pCallerRef, 0, 0, pDestUri, 0));
}
#ifdef TARGET_SIMU
int MDComAPI_addUriListenerFSim(
   IPT_REC_FUNCPTR func,        /* Pointer to callback function */
   const void      *pCallerRef, /* Caller reference */
   const char      *pDestUri,   /* Pointer to destination URI string */
   const char      *pSimURI)           /* Host URI of simulated device */
{
   return(MDComAPI_uriListenerSim(0, func, pCallerRef, 0, 0, pDestUri, 0, pSimURI));
}
#endif
#endif
/*******************************************************************************
NAME:     MDComAPI_addFrgUriListenerQ
ABSTRACT: Add a listener for message data with the given destination URI. 
          Join multicast groups if a destination URI with a multicast 
          destinations host part is given. 
          IPTCom will put received messages on the queue. 
          The reception of messages with the given destination URI can be 
          blocked/unblocked by call of the MDComAPI_blockFrgListener or 
          MDComAPI_unblockFrgListener.
          If the application has not called the MDComAPI_unblockFrgListener with
          the same pRedFuncRef value the listener will be in blocked state.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_addFrgUriListenerQ(
   MD_QUEUE    listenerQueueId, /* Queue ID */
   const void  *pCallerRef,     /* Caller reference */
   const char  *pDestUri,       /* Pointer to destination URI string */
   const void   *pRedFuncRef)   /* Redundancy function reference */
{
   return(MDComAPI_uriListener(listenerQueueId, NULL, pCallerRef, 0, 0, pDestUri, pRedFuncRef));
}
#ifdef TARGET_SIMU

int MDComAPI_addFrgUriListenerQSim(
   MD_QUEUE    listenerQueueId, /* Queue ID */
   const void  *pCallerRef,     /* Caller reference */
   const char  *pDestUri,       /* Pointer to destination URI string */
   const void  *pRedFuncRef,    /* Redundancy function reference */
   const char  *pSimURI)        /* Host URI of simulated device */
{
   return(MDComAPI_uriListenerSim(listenerQueueId, NULL, pCallerRef, 0, 0, pDestUri, pRedFuncRef, pSimURI));
}
#endif
#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_addFrgUriListenerF
ABSTRACT: Add a listener for message data with the given destination URI. 
          Join multicast groups if a destination URI with a multicast 
          destinations host part is given. 
          IPTCom will call the call-back function when messages are received. 
          The reception of messages with the given destination URI can be 
          blocked/unblocked by call of the MDComAPI_blockFrgListener or 
          MDComAPI_unblockFrgListener.
          If the application has not called the MDComAPI_unblockFrgListener with
          the same pRedFuncRef value the listener will be in blocked state.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_addFrgUriListenerF(
   IPT_REC_FUNCPTR func,         /* Pointer to callback function */
   const void      *pCallerRef,  /* Caller reference */
   const char      *pDestUri,    /* Pointer to destination URI string */
   const void      *pRedFuncRef) /* Redundancy function reference */
{
   return(MDComAPI_uriListener(0, func, pCallerRef, 0, 0, pDestUri, pRedFuncRef));
}
#ifdef TARGET_SIMU
int MDComAPI_addFrgUriListenerFSim(
   IPT_REC_FUNCPTR func,         /* Pointer to callback function */
   const void      *pCallerRef,  /* Caller reference */
   const char      *pDestUri,    /* Pointer to destination URI string */
   const void      *pRedFuncRef, /* Redundancy function reference */
   const char      *pSimURI)     /* Host URI of simulated device */
{
   return(MDComAPI_uriListenerSim(0, func, pCallerRef, 0, 0, pDestUri, pRedFuncRef, pSimURI));
}
#endif
#endif

/*******************************************************************************
NAME:     MDComAPI_removeListenerQ
ABSTRACT: Remove a queue from the listener list  
RETURNS:  -
*/
void MDComAPI_removeListenerQ(                                                               
   MD_QUEUE queue)   /* Queue identification of the queue to be removed from
                        listener list*/
#ifdef TARGET_SIMU
{
   char empty[] = {0};
   MDComAPI_removeListenerQSim(queue, empty);   
}

void MDComAPI_removeListenerQSim(
   MD_QUEUE    queue,   /* Queue identification of the queue to be removed from
                           listener list*/
   const char *pSimURI) /* Host URI of simulated device */
#endif
{
   int ret;
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;
   SIMU_DEV_ITEM  *pSimDevItem = NULL;
#endif
   UINT32 comId;
   COMID_ITEM    *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   char instName[IPT_MAX_LABEL_LEN+1]; /* URI instance name */
   char funcName[IPT_MAX_LABEL_LEN+1]; /* URI funtion name */
   LISTERNER_TABLES *pListTables;

#ifdef TARGET_SIMU
   if (pSimURI == NULL)
   {
      IPTVosPrint0(IPT_ERR,
         "MDComAPI_removeListenerQSim: NULL pointer to simulated URI\n");
   }
#endif
   if (IPTGLOBAL(md.mdComInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
#ifdef TARGET_SIMU
         if (pSimURI[0] == 0)
         {
            pListTables = &IPTGLOBAL(md.listTables);
            simuIpAddr = IPTCom_getOwnIpAddr();
         }
         else
         {
            /* search for simulated device */
            pSimDevItem = getSimDev(pSimURI);
            if (pSimDevItem)
            {
               pListTables = &pSimDevItem->listTables;
               simuIpAddr = pSimDevItem->simuIpAddr;
            }
            else
            {
               ret = (int)IPT_ERROR;
            }
         }
         if (ret == IPT_OK)
         {
#else
         pListTables = &IPTGLOBAL(md.listTables);
#endif
         /* Remove listeners for ComId's */
         comId = 0;
         pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&pListTables->comidListTableHdr,
                                                             comId));/*lint !e826  Ignore casting warning */
         while(pComIdItem)
         {
            comId = pComIdItem->keyComId;
#ifdef TARGET_SIMU
            removeQComIdListener(simuIpAddr, pListTables, queue, 1, 0, pComIdItem);
#else
            removeQComIdListener(queue, 1, 0, pComIdItem);
#endif
            pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&pListTables->comidListTableHdr,
                                                                comId));/*lint !e826  Ignore casting warning */
         }

         /* Remove listeners for all instances of all functions */
#ifdef TARGET_SIMU
         removeQueueNormListener(simuIpAddr, queue, 1, 0, &pListTables->aInstAfunc);
         removeQueueFrgListener(simuIpAddr, queue,  1, 0, &pListTables->aInstAfunc);
#else
         removeQueueNormListener(queue, 1, 0, &pListTables->aInstAfunc);
         removeQueueFrgListener(queue,  1, 0, &pListTables->aInstAfunc);
#endif

         /* Remove listeners for a specified instance of a specified function? */
         pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&pListTables->instXFuncNListTableHdr,
                                                                     "", ""));
         while(pInstXFuncN)
         {
            strncpy(instName, pInstXFuncN->instName, sizeof(instName));
            strncpy(funcName, pInstXFuncN->funcName, sizeof(funcName));
#ifdef TARGET_SIMU
            removeQIxFnUriListener(simuIpAddr, pListTables, queue, 1, 0, pInstXFuncN);
#else
            removeQIxFnUriListener(queue, 1, 0, pInstXFuncN);
#endif      
            pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&pListTables->instXFuncNListTableHdr,
                                                                         instName, funcName));
         }

         /* Remove listeners for all instance of a specified function? */
         pAinstFuncN = (AINST_FUNCN_ITEM *)iptUriLabelTabFindNext(&pListTables->aInstFuncNListTableHdr,
                                                                     "");
         while(pAinstFuncN)
         {
            strncpy(funcName, pAinstFuncN->funcName, sizeof(funcName));
#ifdef TARGET_SIMU
            removeQaIFnUriListener(simuIpAddr, pListTables, queue, 1, 0, pAinstFuncN);
#else
            removeQaIFnUriListener(queue, 1, 0, pAinstFuncN);
#endif      
            pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFindNext(&pListTables->aInstFuncNListTableHdr,
                                                                      funcName));
         }

         /* Remove listeners for a specified instance of all function? */
         pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&pListTables->instXaFuncListTableHdr,
                                                                     ""));
         while(pInstXaFunc)
         {
            strncpy(instName, pInstXaFunc->instName, sizeof(instName));
#ifdef TARGET_SIMU
            removeQIxaFUriListener(simuIpAddr, pListTables, queue, 1, 0, pInstXaFunc);
#else
            removeQIxaFUriListener(queue, 1, 0, pInstXaFunc);
#endif      
            pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&pListTables->instXaFuncListTableHdr,
                                                                      instName));
         }
#ifdef TARGET_SIMU
         }
#endif
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "MDComAPI_removeListenerQ: IPTVosGetSem ERROR\n");
      }
   }
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_removeListenerF
ABSTRACT: Remove a function from the listener list 
RETURNS:  -
*/
void MDComAPI_removeListenerF(
   IPT_REC_FUNCPTR func)  /* Function to be removed from listener list*/
#ifdef TARGET_SIMU
{
   char empty[] = {0};
   MDComAPI_removeListenerFSim(func , empty);
}

void MDComAPI_removeListenerFSim(
   IPT_REC_FUNCPTR func, /* Function to be removed from listener list*/
   const char *pSimURI)  /* Host URI of simulated device */
#endif
{
   int ret;
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;
   SIMU_DEV_ITEM  *pSimDevItem;
#endif
   UINT32 comId;
   COMID_ITEM    *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   char instName[IPT_MAX_LABEL_LEN+1]; /* URI instance name */
   char funcName[IPT_MAX_LABEL_LEN+1]; /* URI funtion name */
   LISTERNER_TABLES *pListTables;

#ifdef TARGET_SIMU
   if (pSimURI == NULL)
   {
      IPTVosPrint0(IPT_ERR,
         "MDComAPI_removeListenerFSim: NULL pointer to simulated URI\n");
   }
#endif

   if (IPTGLOBAL(md.mdComInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
#ifdef TARGET_SIMU
         if (pSimURI[0] == 0)
         {
            pListTables = &IPTGLOBAL(md.listTables);
            simuIpAddr = IPTCom_getOwnIpAddr();
         }
         else
         {
            /* search for simulated device */
            pSimDevItem = getSimDev(pSimURI);
            if (pSimDevItem)
            {
               pListTables = &pSimDevItem->listTables;
               simuIpAddr = pSimDevItem->simuIpAddr;
            }
            else
            {
               ret = (int)IPT_ERROR;
            }
         }
         if (ret == IPT_OK)
         {
#else
         pListTables = &IPTGLOBAL(md.listTables);
#endif
         comId = 0;
         pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&pListTables->comidListTableHdr,
                                                             comId));/*lint !e826  Ignore casting warning */
         while(pComIdItem)
         {
            comId = pComIdItem->keyComId;
#ifdef TARGET_SIMU
            removeFComIdListener(simuIpAddr, pListTables, func, 1, 0, pComIdItem);
#else
            removeFComIdListener(func, 1, 0, pComIdItem);
#endif
            pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&pListTables->comidListTableHdr,
                                                                comId));/*lint !e826  Ignore casting warning */
         }
 
         /* Remove listeners for all instances of all functions */
#ifdef TARGET_SIMU
         removeFuncNormListener(simuIpAddr, func, 1, 0, &pListTables->aInstAfunc);
         removeFuncFrgListener(simuIpAddr, func,  1, 0, &pListTables->aInstAfunc);
#else
         removeFuncNormListener(func, 1, 0, &pListTables->aInstAfunc);
         removeFuncFrgListener(func,  1, 0, &pListTables->aInstAfunc);
#endif
         /* Remove listeners for a specified instance of a specified function? */
         pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&pListTables->instXFuncNListTableHdr,
                                                                     "", ""));
         while(pInstXFuncN)
         {
            strncpy(instName, pInstXFuncN->instName, sizeof(instName));
            strncpy(funcName, pInstXFuncN->funcName, sizeof(funcName));
#ifdef TARGET_SIMU
            removeFIxFnUriListener(simuIpAddr, pListTables, func, 1, 0, pInstXFuncN);
#else
            removeFIxFnUriListener(func, 1, 0, pInstXFuncN);
#endif      
            pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&pListTables->instXFuncNListTableHdr,
                                                                         instName, funcName));
         }

         /* Remove listeners for all instance of a specified function? */
         pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFindNext(&pListTables->aInstFuncNListTableHdr,
                                                                     ""));
         while(pAinstFuncN)
         {
            strncpy(funcName, pAinstFuncN->funcName, sizeof(funcName));
#ifdef TARGET_SIMU
            removeFaIFnUriListener(simuIpAddr, pListTables, func, 1, 0, pAinstFuncN);
#else
            removeFaIFnUriListener(func, 1, 0, pAinstFuncN);
#endif      
            pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFindNext(&pListTables->aInstFuncNListTableHdr,
                                                                      funcName));
         }

         /* Remove listeners for a specified instance of all function? */
         pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&pListTables->instXaFuncListTableHdr,
                                                                     ""));
         while(pInstXaFunc)
         {
            strncpy(instName, pInstXaFunc->instName, sizeof(instName));
#ifdef TARGET_SIMU
            removeFIxaFUriListener(simuIpAddr, pListTables, func, 1, 0, pInstXaFunc);
#else
            removeFIxaFUriListener(func, 1, 0, pInstXaFunc);
#endif      
            pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&pListTables->instXaFuncListTableHdr,
                                                                      instName));
         }
#ifdef TARGET_SIMU
         }
#endif
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "MDComAPI_removeListenerQ: IPTVosGetSem ERROR\n");
      }
   }
}
#endif

/*******************************************************************************
NAME:     MDComAPI_removeComIdListenerQ
ABSTRACT: Remove queue comid listener from the listener list  
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_removeComIdListenerQ(
   MD_QUEUE    queue,       /* Queue identification of the queue to be removed 
                               from listener list*/
   const void  *pCallerRef, /* Caller reference */
   const UINT32 comid[])    /* Array with comid's, ended by 0 */
#ifdef TARGET_SIMU
{
   char empty[] = {0};
   return(MDComAPI_removeComIdListenerQSim(queue, pCallerRef, comid, empty));   
}

int MDComAPI_removeComIdListenerQSim(
   MD_QUEUE     queue,       /* Queue identification of the queue to be removed 
                                from listener list*/
   const void   *pCallerRef, /* Caller reference */
   const UINT32 comid[],     /* Array with comid's, ended by 0 */
   const char   *pSimURI)    /* Host URI of simulated device */
#endif
{
   int ret = IPT_OK;
   UINT32 i;
   COMID_ITEM    *pComIdItem;
   LISTERNER_TABLES *pListTables;
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;
   SIMU_DEV_ITEM  *pSimDevItem;
#endif

   if (IPTGLOBAL(md.mdComInitiated))
   {
#ifdef TARGET_SIMU
      if (pSimURI == NULL)
      {
         IPTVosPrint0(IPT_ERR,
            "MDComAPI_removeComIdListenerQ: NULL pointer to simulated URI\n");
         return((int)IPT_INVALID_PAR);
      }
#endif

      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
#ifdef TARGET_SIMU
         if (pSimURI[0] == 0)
         {
            pListTables = &IPTGLOBAL(md.listTables);
            simuIpAddr = IPTCom_getOwnIpAddr();
         }
         else
         {
            /* search for simulated device */
            pSimDevItem = getSimDev(pSimURI);
            if (pSimDevItem)
            {
               pListTables = &pSimDevItem->listTables;
               simuIpAddr = pSimDevItem->simuIpAddr;
            }
            else
            {
               ret = (int)IPT_ERROR;
            }
         }
         if (ret == IPT_OK)
         {
#else
         pListTables = &IPTGLOBAL(md.listTables);
#endif
         for (i=0; comid[i] != 0; i++)
         {
            pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&pListTables->comidListTableHdr,
                                                            comid[i]));/*lint !e826  Ignore casting warning */
            if (pComIdItem)
            {
               if (pCallerRef)
               {
#ifdef TARGET_SIMU
                  removeQComIdListener(simuIpAddr, pListTables, queue, 0, pCallerRef, pComIdItem);
#else
                  removeQComIdListener(queue, 0, pCallerRef, pComIdItem);
#endif
               }
               else
               {
#ifdef TARGET_SIMU
                  removeQComIdListener(simuIpAddr, pListTables, queue, 1, 0, pComIdItem);
#else
                  removeQComIdListener(queue, 1, 0, pComIdItem);
#endif
               }
            }
         }
#ifdef TARGET_SIMU
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "MDComAPI_removeComIdListenerQ: Couldn't get simulated device\n");
            ret = (int)IPT_ERROR;
         }
#endif
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "MDComAPI_removeListenerQ: IPTVosGetSem ERROR\n");
      }
   }
   return(ret);
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_removeComIdListenerF
ABSTRACT: Remove queue comid listener from the listener list  
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_removeComIdListenerF(
   IPT_REC_FUNCPTR func,    /* Function to be removed from listener list*/
   const void  *pCallerRef, /* Caller reference */
   const UINT32 comid[])    /* Array with comid's, ended by 0 */
#ifdef TARGET_SIMU
{
   char empty[] = {0};
   return(MDComAPI_removeComIdListenerFSim(func, pCallerRef, comid, empty));   
}

int MDComAPI_removeComIdListenerFSim(
   IPT_REC_FUNCPTR func,    /* Function to be removed from listener list*/
   const void  *pCallerRef, /* Caller reference */
   const UINT32 comid[],    /* Array with comid's, ended by 0 */
   const char   *pSimURI)    /* Host URI of simulated device */
#endif

{
   int ret = IPT_OK;
   UINT32 i;
   COMID_ITEM    *pComIdItem;
   LISTERNER_TABLES *pListTables;
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;
   SIMU_DEV_ITEM  *pSimDevItem;
#endif

   if (IPTGLOBAL(md.mdComInitiated))
   {
#ifdef TARGET_SIMU
      if (pSimURI == NULL)
      {
         IPTVosPrint0(IPT_ERR,
            "MDComAPI_removeComIdListenerFSim: NULL pointer to simulated URI\n");
         return((int)IPT_INVALID_PAR);
      }
#endif

      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
#ifdef TARGET_SIMU
         if (pSimURI[0] == 0)
         {
            pListTables = &IPTGLOBAL(md.listTables);
            simuIpAddr = IPTCom_getOwnIpAddr();
         }
         else
         {
            /* search for simulated device */
            pSimDevItem = getSimDev(pSimURI);
            if (pSimDevItem)
            {
               pListTables = &pSimDevItem->listTables;
               simuIpAddr = pSimDevItem->simuIpAddr;
            }
            else
            {
               ret = (int)IPT_ERROR;
            }
         }
         if (ret == IPT_OK)
         {
#else
         pListTables = &IPTGLOBAL(md.listTables);
#endif
         for (i=0; comid[i] != 0; i++)
         {
            pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&pListTables->comidListTableHdr,
                                                            comid[i]));/*lint !e826  Ignore casting warning */
            if (pComIdItem)
            {
               if (pCallerRef)
               {
#ifdef TARGET_SIMU
                  removeFComIdListener(simuIpAddr, pListTables, func, 0, pCallerRef, pComIdItem);
#else
                  removeFComIdListener(func, 0, pCallerRef, pComIdItem);
#endif
               }
               else
               {
#ifdef TARGET_SIMU
                  removeFComIdListener(simuIpAddr, pListTables, func, 1, 0, pComIdItem);
#else
                  removeFComIdListener(func, 1, 0, pComIdItem);
#endif
               }
            }
         }
#ifdef TARGET_SIMU
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "MDComAPI_removeComIdListenerFSim: Couldn't get simulated device\n");
            ret = (int)IPT_ERROR;
         }
#endif
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "MDComAPI_removeComIdListenerF: IPTVosGetSem ERROR\n");
      }
   }
   return(ret);
} 
#endif

/*******************************************************************************
NAME:     MDComAPI_removeUriListenerQ
ABSTRACT: Remove queue URI listener from the listener list  
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_removeUriListenerQ(
   MD_QUEUE queue,          /* Queue identification of the queue to be removed 
                               from listener list */
   const void  *pCallerRef, /* Caller reference */
   const char  *pDestUri)   /* Pointer to destination URI string */
#ifdef TARGET_SIMU
{
   char empty[] = {0};
   return(MDComAPI_removeUriListenerQSim(queue, pCallerRef, pDestUri, empty));   
}

int MDComAPI_removeUriListenerQSim(
   MD_QUEUE queue,          /* Queue identification of the queue to be removed 
                               from listener list */
   const void  *pCallerRef, /* Caller reference */
   const char  *pDestUri,   /* Pointer to destination URI string */
   const char  *pSimURI)    /* Host URI of simulated device */
#endif
{
   char uriInstName[IPT_MAX_LABEL_LEN] = {0};
   char uriFuncName[IPT_MAX_LABEL_LEN] = {0};
   int  uriType = NO_USER_URI;
   int  ret = IPT_OK;
   int  all;
   UINT32 i;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   LISTERNER_TABLES *pListTables;
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;
   SIMU_DEV_ITEM  *pSimDevItem;
#endif

   if (IPTGLOBAL(md.mdComInitiated))
   {
#ifdef TARGET_SIMU
      if (pSimURI == NULL)
      {
         IPTVosPrint0(IPT_ERR,
            "MDComAPI_removeUriListenerQSim: NULL pointer to simulated URI\n");
         return((int)IPT_INVALID_PAR);
      }
#endif

      if (pDestUri != 0)
      {
         if ((ret = extractUri(pDestUri, uriInstName, uriFuncName, &uriType)) != (int)IPT_OK)
         {
            IPTVosPrint1(IPT_ERR,
               "MDComAPI_removeUriListenerQ: wrong dest URI=%s\n",pDestUri);
            return(ret);
         }
      }
     
      if (pCallerRef)
      {
         all = 0;   
      }
      else
      {
         all = 1;   
      }

      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
#ifdef TARGET_SIMU
         if (pSimURI[0] == 0)
         {
            pListTables = &IPTGLOBAL(md.listTables);
            simuIpAddr = IPTCom_getOwnIpAddr();
         }
         else
         {
            /* search for simulated device */
            pSimDevItem = getSimDev(pSimURI);
            if (pSimDevItem)
            {
               pListTables = &pSimDevItem->listTables;
               simuIpAddr = pSimDevItem->simuIpAddr;
            }
            else
            {
               ret = (int)IPT_ERROR;
            }
         }
         if (ret == IPT_OK)
         {
#else
         pListTables = &IPTGLOBAL(md.listTables);
#endif
         if (pDestUri != 0)
         {
            if (uriType == AINST_AFUNC_URI)
            {
               /* Remove listeners for all instances of all functions */
#ifdef TARGET_SIMU
               removeQueueNormListener(simuIpAddr, queue, all, pCallerRef, &pListTables->aInstAfunc);
               removeQueueFrgListener(simuIpAddr, queue, all, pCallerRef, &pListTables->aInstAfunc);

#else
               removeQueueNormListener(queue, all, pCallerRef, &pListTables->aInstAfunc);
               removeQueueFrgListener(queue, all, pCallerRef, &pListTables->aInstAfunc);
#endif
            }
            else if (uriType == INSTX_FUNCN_URI)
            {
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2Find(&pListTables->instXFuncNListTableHdr,
                                                                      uriInstName, uriFuncName));
               if (pInstXFuncN)
               {
#ifdef TARGET_SIMU
                  removeQIxFnUriListener(simuIpAddr, pListTables, queue, all, pCallerRef, pInstXFuncN);
#else
                  removeQIxFnUriListener(queue, all, pCallerRef, pInstXFuncN);
#endif
               }
            }
            else if (uriType == AINST_FUNCN_URI)
            {
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                      uriFuncName));
               if (pAinstFuncN)
               {
#ifdef TARGET_SIMU
                  removeQaIFnUriListener(simuIpAddr, pListTables, queue, all, pCallerRef, pAinstFuncN);
#else
                  removeQaIFnUriListener(queue, all, pCallerRef, pAinstFuncN);
#endif
               }
            }
            else if (uriType == INSTX_AFUNC_URI)
            {
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                      uriInstName));
               if (pInstXaFunc)
               {
#ifdef TARGET_SIMU
                  removeQIxaFUriListener(simuIpAddr, pListTables, queue, 0, pCallerRef, pInstXaFunc);
#else
                  removeQIxaFUriListener(queue, 0, pCallerRef, pInstXaFunc);
#endif
               }
            }
         }
         else
         {
            /* Remove listeners for all instances of all functions */
#ifdef TARGET_SIMU
            removeQueueNormListener(simuIpAddr, queue, all, pCallerRef, &pListTables->aInstAfunc);
            removeQueueFrgListener(simuIpAddr, queue, all, pCallerRef, &pListTables->aInstAfunc);
#else
            removeQueueNormListener(queue, all, pCallerRef, &pListTables->aInstAfunc);
            removeQueueFrgListener(queue, all, pCallerRef, &pListTables->aInstAfunc);
#endif
            /* Remove listeners for a specified instance of a specified function? */
            pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
            for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
            {
#ifdef TARGET_SIMU
               removeQIxFnUriListener(simuIpAddr, pListTables, queue, all, pCallerRef, &pInstXFuncN[i]);
#else
               removeQIxFnUriListener(queue, all, pCallerRef, &pInstXFuncN[i]);
#endif
            }
            
            /* Remove listeners for all instance of a specified function? */
            pAinstFuncN = (AINST_FUNCN_ITEM *)pListTables->aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
            for (i = 0; i < pListTables->aInstFuncNListTableHdr.nItems ; i++)
            {
#ifdef TARGET_SIMU
               removeQaIFnUriListener(simuIpAddr, pListTables, queue, all, pCallerRef, &pAinstFuncN[i]);
#else
               removeQaIFnUriListener(queue, all, pCallerRef, &pAinstFuncN[i]);
#endif
            }
           
            /* Remove listeners for a specified instance of all function? */
            pInstXaFunc = (INSTX_AFUNC_ITEM *)pListTables->instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
            for (i = 0; i < pListTables->instXaFuncListTableHdr.nItems ; i++)
            {
#ifdef TARGET_SIMU
               removeQIxaFUriListener(simuIpAddr, pListTables, queue, 0, pCallerRef, &pInstXaFunc[i]);
#else
               removeQIxaFUriListener(queue, 0, pCallerRef, &pInstXaFunc[i]);
#endif
            }
         }

#ifdef TARGET_SIMU
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "MDComAPI_removeUriListenerQSim: Couldn't get simulated device\n");
            ret = (int)IPT_ERROR;
         }
#endif
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "MDComAPI_removeUriListenerQ: IPTVosGetSem ERROR\n");
      }
   }
   return(ret);
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_removeUriListenerF
ABSTRACT: Remove queue URI listener from the listener list  
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_removeUriListenerF(
   IPT_REC_FUNCPTR func,    /* Function to be removed from listener list*/
   const void  *pCallerRef, /* Caller reference */
   const char  *pDestUri)   /* Pointer to destination URI string */
#ifdef TARGET_SIMU
{
   char empty[] = {0};
   return(MDComAPI_removeUriListenerFSim(func, pCallerRef, pDestUri, empty));   
}

int MDComAPI_removeUriListenerFSim(
   IPT_REC_FUNCPTR func,    /* Function to be removed from listener list*/
   const void  *pCallerRef, /* Caller reference */
   const char  *pDestUri,   /* Pointer to destination URI string */
   const char  *pSimURI)    /* Host URI of simulated device */
#endif
{
   char uriInstName[IPT_MAX_LABEL_LEN] = {0};
   char uriFuncName[IPT_MAX_LABEL_LEN] = {0};
   int  uriType = NO_USER_URI;
   int  ret = IPT_OK;
   int  all;
   UINT32 i;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   LISTERNER_TABLES *pListTables;
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;
   SIMU_DEV_ITEM  *pSimDevItem;
#endif

   if (IPTGLOBAL(md.mdComInitiated))
   {
#ifdef TARGET_SIMU
      if (pSimURI == NULL)
      {
         IPTVosPrint0(IPT_ERR,
            "MDComAPI_removeUriListenerFSim: NULL pointer to simulated URI\n");
         return((int)IPT_INVALID_PAR);
      }
#endif

      if (pDestUri != 0)
      {
         if ((ret = extractUri(pDestUri, uriInstName, uriFuncName, &uriType)) != (int)IPT_OK)
         {
            IPTVosPrint1(IPT_ERR,
               "MDComAPI_removeUriListenerF: wrong dest URI=%s\n",pDestUri);
            return(ret);
         }
      }
     
      if (pCallerRef)
      {
         all = 0;   
      }
      else
      {
         all = 1;   
      }

      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
#ifdef TARGET_SIMU
         if (pSimURI[0] == 0)
         {
            pListTables = &IPTGLOBAL(md.listTables);
            simuIpAddr = IPTCom_getOwnIpAddr();
         }
         else
         {
            /* search for simulated device */
            pSimDevItem = getSimDev(pSimURI);
            if (pSimDevItem)
            {
               pListTables = &pSimDevItem->listTables;
               simuIpAddr = pSimDevItem->simuIpAddr;
            }
            else
            {
               ret = (int)IPT_ERROR;
            }
         }
         if (ret == IPT_OK)
         {
#else
         pListTables = &IPTGLOBAL(md.listTables);
#endif
         if (pDestUri != 0)
         {
            if (uriType == AINST_AFUNC_URI)
            {
               /* Remove listeners for all instances of all functions */
#ifdef TARGET_SIMU
               removeFuncNormListener(simuIpAddr, func, all, pCallerRef, &pListTables->aInstAfunc);
               removeFuncFrgListener(simuIpAddr, func, all, pCallerRef, &pListTables->aInstAfunc);
#else
               removeFuncNormListener(func, all, pCallerRef, &pListTables->aInstAfunc);
               removeFuncFrgListener(func, all, pCallerRef, &pListTables->aInstAfunc);
#endif
            }
            else if (uriType == INSTX_FUNCN_URI)
            {
               /* Any listener for a specified instance of a specified function? */
               pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2Find(&pListTables->instXFuncNListTableHdr,
                                                                      uriInstName, uriFuncName));
               if (pInstXFuncN)
               {
#ifdef TARGET_SIMU
                  removeFIxFnUriListener(simuIpAddr, pListTables, func, all, pCallerRef, pInstXFuncN);
#else
                  removeFIxFnUriListener(func, all, pCallerRef, pInstXFuncN);
#endif
               }
            }
            else if (uriType == AINST_FUNCN_URI)
            {
               /* Any listener for all instance of a specified function? */
               pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFind(&pListTables->aInstFuncNListTableHdr,
                                                                      uriFuncName));
               if (pAinstFuncN)
               {
#ifdef TARGET_SIMU
                  removeFaIFnUriListener(simuIpAddr, pListTables, func, all, pCallerRef, pAinstFuncN);
#else
                  removeFaIFnUriListener(func, all, pCallerRef, pAinstFuncN);
#endif
               }
            }
            else if (uriType == INSTX_AFUNC_URI)
            {
               /* Any listener for a specified instance of all function? */
               pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFind(&pListTables->instXaFuncListTableHdr,
                                                                      uriInstName));
               if (pInstXaFunc)
               {
#ifdef TARGET_SIMU
                  removeFIxaFUriListener(simuIpAddr, pListTables, func, 0, pCallerRef, pInstXaFunc);
#else
                  removeFIxaFUriListener(func, 0, pCallerRef, pInstXaFunc);
#endif
               }
            }
         }
         else
         {
            /* Remove listeners for all instances of all functions */
#ifdef TARGET_SIMU
            removeFuncNormListener(simuIpAddr, func, all, pCallerRef, &pListTables->aInstAfunc);
            removeFuncFrgListener(simuIpAddr, func, all, pCallerRef, &pListTables->aInstAfunc);
#else
            removeFuncNormListener(func, all, pCallerRef, &pListTables->aInstAfunc);
            removeFuncFrgListener(func, all, pCallerRef, &pListTables->aInstAfunc);
#endif
            /* Remove listeners for a specified instance of a specified function? */
            pInstXFuncN = (INSTX_FUNCN_ITEM *)pListTables->instXFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
            for (i = 0; i < pListTables->instXFuncNListTableHdr.nItems ; i++)
            {
#ifdef TARGET_SIMU
               removeFIxFnUriListener(simuIpAddr, pListTables, func, all, pCallerRef, &pInstXFuncN[i]);
#else
               removeFIxFnUriListener(func, all, pCallerRef, &pInstXFuncN[i]);
#endif
            }
            
            /* Remove listeners for all instance of a specified function? */
            pAinstFuncN = (AINST_FUNCN_ITEM *)pListTables->aInstFuncNListTableHdr.pTable;/*lint !e826 Type cast OK */
            for (i = 0; i < pListTables->aInstFuncNListTableHdr.nItems ; i++)
            {
#ifdef TARGET_SIMU
               removeFaIFnUriListener(simuIpAddr, pListTables, func, all, pCallerRef, &pAinstFuncN[i]);
#else
               removeFaIFnUriListener(func, all, pCallerRef, &pAinstFuncN[i]);
#endif
            }
           
            /* Remove listeners for a specified instance of all function? */
            pInstXaFunc = (INSTX_AFUNC_ITEM *)pListTables->instXaFuncListTableHdr.pTable;/*lint !e826 Type cast OK */
            for (i = 0; i < pListTables->instXaFuncListTableHdr.nItems ; i++)
            {
#ifdef TARGET_SIMU
               removeFIxaFUriListener(simuIpAddr, pListTables, func, 0, pCallerRef, &pInstXaFunc[i]);
#else
               removeFIxaFUriListener(func, 0, pCallerRef, &pInstXaFunc[i]);
#endif
            }
         }

#ifdef TARGET_SIMU
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "MDComAPI_removeUriListenerFSim: Couldn't get simulated device\n");
            ret = (int)IPT_ERROR;
         }
#endif
         if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "MDComAPI_removeUriListenerF: IPTVosGetSem ERROR\n");
      }
   }
   return(ret);
}
#endif

/*******************************************************************************
NAME:     MDComAPI_unblockFrgListener
ABSTRACT: Set a FRG listener in leader state, i.e. received FRG messages will
          be forwarded to the listener application and the messages will be
          acknowledged. 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_unblockFrgListener(
   const void *pRedFuncRef)  /* Redendancy function reference value */
{
   int res;  
   FRG_ITEM *pFrgItem;

   res = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (res == IPT_OK)
   {
      pFrgItem = getFrgFrgItem(pRedFuncRef);
      if (pFrgItem)
      {
         pFrgItem->frgState = FRG_LEADER;
      }
      else
      {
         res = IPT_MEM_ERROR;
      }
      if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "MDComAPI_unblockFrgListener: IPTVosGetSem ERROR\n");
   }

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI_blockFrgListener
ABSTRACT: Set a FRG listener in follower state, i.e. received FRG messages will
          be forwarded to the listener application and the messages will be
          acknowledged. 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_blockFrgListener(
   const void *pRedFuncRef)  /* Redendancy function reference value */
{
   int res;  
   FRG_ITEM *pFrgItem;

   res = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (res == IPT_OK)
   {
      pFrgItem = getFrgFrgItem(pRedFuncRef);
      if (pFrgItem)
      {
         pFrgItem->frgState = FRG_FOLLOWER;
      }
      else
      {
         res = IPT_MEM_ERROR;
      }
      if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "MDComAPI_blockFrgListener: IPTVosGetSem ERROR\n");
   }

   return(res);
}

/*******************************************************************************
NAME:     mdTerminateListener 
ABSTRACT: Remove all listeners
RETURNS:  -
*/
void mdTerminateListener(void)
{
   int ret;
   COMID_ITEM    *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
#ifdef TARGET_SIMU
   UINT32 i;
   UINT32 simuIpAddr;
   SIMU_DEV_ITEM  *pSimDevItem;
#endif

   ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
#ifdef TARGET_SIMU
      simuIpAddr = IPTCom_getOwnIpAddr();

      pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr), 0));
      while(pComIdItem)
      {
         while (pComIdItem->lists.pQueueList)
         {
            removeQComIdListener(simuIpAddr, &IPTGLOBAL(md.listTables), pComIdItem->lists.pQueueList->pQueue->listenerQueueId,
                                 1, 0, pComIdItem);
         }
         
         while (pComIdItem->lists.pQueueFrgList)
         {
            removeQComIdListener(simuIpAddr, &IPTGLOBAL(md.listTables), pComIdItem->lists.pQueueFrgList->pQueue->listenerQueueId,
                                 1, 0, pComIdItem);
         }
         
         while (pComIdItem->lists.pFuncList)
         {
            removeFComIdListener(simuIpAddr, &IPTGLOBAL(md.listTables), pComIdItem->lists.pFuncList->pFunc->func,
                                 1, 0, pComIdItem);
         }
         
         while (pComIdItem->lists.pFuncFrgList)
         {
            removeFComIdListener(simuIpAddr, &IPTGLOBAL(md.listTables), pComIdItem->lists.pFuncFrgList->pFunc->func,
                                 1, 0, pComIdItem);
         }
      
         pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr),
                                                             0));/*lint !e826  Ignore casting warning */
      }

      /* Remove listeners for all instances of all functions */
      while (IPTGLOBAL(md.listTables.aInstAfunc).pQueueList)
      {
         removeQueueNormListener(simuIpAddr, IPTGLOBAL(md.listTables.aInstAfunc).pQueueList->pQueue->listenerQueueId,
                                 1, 0, &IPTGLOBAL(md.listTables.aInstAfunc));
      }
      
      while (IPTGLOBAL(md.listTables.aInstAfunc).pQueueFrgList)
      {
         removeQueueFrgListener(simuIpAddr, IPTGLOBAL(md.listTables.aInstAfunc).pQueueFrgList->pQueue->listenerQueueId,
                                 1, 0, &IPTGLOBAL(md.listTables.aInstAfunc));
      }
      
      while (IPTGLOBAL(md.listTables.aInstAfunc).pFuncList)
      {
         removeFuncNormListener(simuIpAddr, IPTGLOBAL(md.listTables.aInstAfunc).pFuncList->pFunc->func,
                                 1, 0, &IPTGLOBAL(md.listTables.aInstAfunc));
      }
      
      while (IPTGLOBAL(md.listTables.aInstAfunc).pFuncFrgList)
      {
         removeFuncFrgListener(simuIpAddr, IPTGLOBAL(md.listTables.aInstAfunc).pFuncFrgList->pFunc->func,
                                 1, 0, &IPTGLOBAL(md.listTables.aInstAfunc));
      }
      
      pInstXFuncN = (INSTX_FUNCN_ITEM *) iptUriLabelTab2FindNext(&IPTGLOBAL(md.listTables.instXFuncNListTableHdr),
                                                                  "", "");
      while(pInstXFuncN)
      {
         while (pInstXFuncN->lists.pQueueList)
         {
            removeQIxFnUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pInstXFuncN->lists.pQueueList->pQueue->listenerQueueId,
                                   1, 0, pInstXFuncN);
         }
         
         while (pInstXFuncN->lists.pQueueFrgList)
         {
            removeQIxFnUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pInstXFuncN->lists.pQueueFrgList->pQueue->listenerQueueId,
                                   1, 0, pInstXFuncN);
         }
         
         while (pInstXFuncN->lists.pFuncList)
         {
            removeFIxFnUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pInstXFuncN->lists.pFuncList->pFunc->func,
                                   1, 0, pInstXFuncN);
         }
         
         while (pInstXFuncN->lists.pFuncFrgList)
         {
            removeFIxFnUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pInstXFuncN->lists.pFuncFrgList->pFunc->func,
                                   1, 0, pInstXFuncN);
         }
      
         pInstXFuncN = (INSTX_FUNCN_ITEM *) iptUriLabelTab2FindNext(&IPTGLOBAL(md.listTables.instXFuncNListTableHdr),
                                                                      "", "");
      }

      pAinstFuncN = (AINST_FUNCN_ITEM *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.aInstFuncNListTableHdr),
                                                                  "");
      while(pAinstFuncN)
      {
         while (pAinstFuncN->lists.pQueueList)
         {
            removeQaIFnUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pAinstFuncN->lists.pQueueList->pQueue->listenerQueueId,
                                   1, 0, pAinstFuncN);
         }
         
         while (pAinstFuncN->lists.pQueueFrgList)
         {
            removeQaIFnUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pAinstFuncN->lists.pQueueFrgList->pQueue->listenerQueueId,
                                   1, 0, pAinstFuncN);
         }
         
         while (pAinstFuncN->lists.pFuncList)
         {
            removeFaIFnUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pAinstFuncN->lists.pFuncList->pFunc->func,
                                   1, 0, pAinstFuncN);
         }
         
         while (pAinstFuncN->lists.pFuncFrgList)
         {
            removeFaIFnUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pAinstFuncN->lists.pFuncFrgList->pFunc->func,
                                   1, 0, pAinstFuncN);
         }
      
         pAinstFuncN = (AINST_FUNCN_ITEM *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.aInstFuncNListTableHdr),
                                                                     "");
      }

      pInstXaFunc = (INSTX_AFUNC_ITEM *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.instXaFuncListTableHdr),
                                                                  "");
      while(pInstXaFunc)
      {
         while (pInstXaFunc->lists.pQueueList)
         {
            removeQIxaFUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pInstXaFunc->lists.pQueueList->pQueue->listenerQueueId,
                                   1, 0, pInstXaFunc);
         }
         
         while (pInstXaFunc->lists.pQueueFrgList)
         {
            removeQIxaFUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pInstXaFunc->lists.pQueueFrgList->pQueue->listenerQueueId,
                                   1, 0, pInstXaFunc);
         }
         
         while (pInstXaFunc->lists.pFuncList)
         {
            removeFIxaFUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pInstXaFunc->lists.pFuncList->pFunc->func,
                                   1, 0, pInstXaFunc);
         }
         
         while (pInstXaFunc->lists.pFuncFrgList)
         {
            removeFIxaFUriListener(simuIpAddr, &IPTGLOBAL(md.listTables), pInstXaFunc->lists.pFuncFrgList->pFunc->func,
                                   1, 0, pInstXaFunc);
         }
      
         pInstXaFunc = (INSTX_AFUNC_ITEM *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.instXaFuncListTableHdr),
                                                                     "");
      }

      pSimDevItem = (SIMU_DEV_ITEM *)((void *)IPTGLOBAL(md.simuDevListTableHdr.pTable));
      for (i=0; i<IPTGLOBAL(md.simuDevListTableHdr.nItems); i++)
      {
         pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&pSimDevItem[i].listTables.comidListTableHdr,
                                                             0));/*lint !e826  Ignore casting warning */
         while(pComIdItem)
         {
            while (pComIdItem->lists.pQueueList)
            {
               removeQComIdListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables, pComIdItem->lists.pQueueList->pQueue->listenerQueueId,
                                    1, 0, pComIdItem);
            }
         
            while (pComIdItem->lists.pQueueFrgList)
            {
               removeQComIdListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables, pComIdItem->lists.pQueueFrgList->pQueue->listenerQueueId,
                                    1, 0, pComIdItem);
            }
         
            while (pComIdItem->lists.pFuncList)
            {
               removeFComIdListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables, pComIdItem->lists.pFuncList->pFunc->func,
                                    1, 0, pComIdItem);
            }
         
            while (pComIdItem->lists.pFuncFrgList)
            {
               removeFComIdListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables, pComIdItem->lists.pFuncFrgList->pFunc->func,
                                    1, 0, pComIdItem);
            }
      
            pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&pSimDevItem[i].listTables.comidListTableHdr,
                                                                0));/*lint !e826  Ignore casting warning */
         }

         /* Remove listeners for all instances of all functions */
         while (pSimDevItem[i].listTables.aInstAfunc.pQueueList)
         {
            removeQueueNormListener(pSimDevItem[i].simuIpAddr, pSimDevItem[i].listTables.aInstAfunc.pQueueList->pQueue->listenerQueueId,
                                    1, 0, &pSimDevItem[i].listTables.aInstAfunc);
         }
      
         while (pSimDevItem[i].listTables.aInstAfunc.pQueueFrgList)
         {
            removeQueueFrgListener(pSimDevItem[i].simuIpAddr, pSimDevItem[i].listTables.aInstAfunc.pQueueFrgList->pQueue->listenerQueueId,
                                    1, 0, &pSimDevItem[i].listTables.aInstAfunc);
         }
      
         while (pSimDevItem[i].listTables.aInstAfunc.pFuncList)
         {
            removeFuncNormListener(pSimDevItem[i].simuIpAddr, pSimDevItem[i].listTables.aInstAfunc.pFuncList->pFunc->func,
                                    1, 0, &pSimDevItem[i].listTables.aInstAfunc);
         }
      
         while (pSimDevItem[i].listTables.aInstAfunc.pFuncFrgList)
         {
            removeFuncFrgListener(pSimDevItem[i].simuIpAddr, pSimDevItem[i].listTables.aInstAfunc.pFuncFrgList->pFunc->func,
                                    1, 0, &pSimDevItem[i].listTables.aInstAfunc);
         }
      
         pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&pSimDevItem[i].listTables.instXFuncNListTableHdr,
                                                                     "", ""));
         while(pInstXFuncN)
         {
            while (pInstXFuncN->lists.pQueueList)
            {
               removeQIxFnUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pInstXFuncN->lists.pQueueList->pQueue->listenerQueueId,
                                      1, 0, pInstXFuncN);
            }
         
            while (pInstXFuncN->lists.pQueueFrgList)
            {
               removeQIxFnUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pInstXFuncN->lists.pQueueFrgList->pQueue->listenerQueueId,
                                      1, 0, pInstXFuncN);
            }
         
            while (pInstXFuncN->lists.pFuncList)
            {
               removeFIxFnUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pInstXFuncN->lists.pFuncList->pFunc->func,
                                      1, 0, pInstXFuncN);
            }
         
            while (pInstXFuncN->lists.pFuncFrgList)
            {
               removeFIxFnUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pInstXFuncN->lists.pFuncFrgList->pFunc->func,
                                      1, 0, pInstXFuncN);
            }
      
            pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&pSimDevItem[i].listTables.instXFuncNListTableHdr,
                                                                         "", ""));
         }

         pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFindNext(&pSimDevItem[i].listTables.aInstFuncNListTableHdr,
                                                                     ""));
         while(pAinstFuncN)
         {
            while (pAinstFuncN->lists.pQueueList)
            {
               removeQaIFnUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pAinstFuncN->lists.pQueueList->pQueue->listenerQueueId,
                                      1, 0, pAinstFuncN);
            }
         
            while (pAinstFuncN->lists.pQueueFrgList)
            {
               removeQaIFnUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pAinstFuncN->lists.pQueueFrgList->pQueue->listenerQueueId,
                                      1, 0, pAinstFuncN);
            }
         
            while (pAinstFuncN->lists.pFuncList)
            {
               removeFaIFnUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pAinstFuncN->lists.pFuncList->pFunc->func,
                                      1, 0, pAinstFuncN);
            }
         
            while (pAinstFuncN->lists.pFuncFrgList)
            {
               removeFaIFnUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pAinstFuncN->lists.pFuncFrgList->pFunc->func,
                                      1, 0, pAinstFuncN);
            }
      
            pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFindNext(&pSimDevItem[i].listTables.aInstFuncNListTableHdr,
                                                                        ""));
         }

         pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&pSimDevItem[i].listTables.instXaFuncListTableHdr,
                                                                     ""));
         while(pInstXaFunc)
         {
            while (pInstXaFunc->lists.pQueueList)
            {
               removeQIxaFUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pInstXaFunc->lists.pQueueList->pQueue->listenerQueueId,
                                      1, 0, pInstXaFunc);
            }
         
            while (pInstXaFunc->lists.pQueueFrgList)
            {
               removeQIxaFUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pInstXaFunc->lists.pQueueFrgList->pQueue->listenerQueueId,
                                      1, 0, pInstXaFunc);
            }
         
            while (pInstXaFunc->lists.pFuncList)
            {
               removeFIxaFUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pInstXaFunc->lists.pFuncList->pFunc->func,
                                      1, 0, pInstXaFunc);
            }
         
            while (pInstXaFunc->lists.pFuncFrgList)
            {
               removeFIxaFUriListener(pSimDevItem[i].simuIpAddr, &pSimDevItem[i].listTables,pInstXaFunc->lists.pFuncFrgList->pFunc->func,
                                      1, 0, pInstXaFunc);
            }
      
            pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&pSimDevItem[i].listTables.instXaFuncListTableHdr,
                                                                        ""));
         }
      }
#else
      pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr),
                                                          0));/*lint !e826  Ignore casting warning */
      while(pComIdItem)
      {
         while (pComIdItem->lists.pQueueList)
         {
            removeQComIdListener(pComIdItem->lists.pQueueList->pQueue->listenerQueueId,
                                 1, 0, pComIdItem);
         }
         
         while (pComIdItem->lists.pQueueFrgList)
         {
            removeQComIdListener(pComIdItem->lists.pQueueFrgList->pQueue->listenerQueueId,
                                 1, 0, pComIdItem);
         }
         
         while (pComIdItem->lists.pFuncList)
         {
            removeFComIdListener(pComIdItem->lists.pFuncList->pFunc->func,
                                 1, 0, pComIdItem);
         }
         
         while (pComIdItem->lists.pFuncFrgList)
         {
            removeFComIdListener(pComIdItem->lists.pFuncFrgList->pFunc->func,
                                 1, 0, pComIdItem);
         }
      
         pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr),
                                                             0));/*lint !e826  Ignore casting warning */
      }

      /* Remove listeners for all instances of all functions */
      while (IPTGLOBAL(md.listTables.aInstAfunc).pQueueList)
      {
         removeQueueNormListener(IPTGLOBAL(md.listTables.aInstAfunc).pQueueList->pQueue->listenerQueueId,
                                 1, 0, &IPTGLOBAL(md.listTables.aInstAfunc));
      }
      
      while (IPTGLOBAL(md.listTables.aInstAfunc).pQueueFrgList)
      {
         removeQueueFrgListener(IPTGLOBAL(md.listTables.aInstAfunc).pQueueFrgList->pQueue->listenerQueueId,
                                 1, 0, &IPTGLOBAL(md.listTables.aInstAfunc));
      }
      
      while (IPTGLOBAL(md.listTables.aInstAfunc).pFuncList)
      {
         removeFuncNormListener(IPTGLOBAL(md.listTables.aInstAfunc).pFuncList->pFunc->func,
                                 1, 0, &IPTGLOBAL(md.listTables.aInstAfunc));
      }
      
      while (IPTGLOBAL(md.listTables.aInstAfunc).pFuncFrgList)
      {
         removeFuncFrgListener(IPTGLOBAL(md.listTables.aInstAfunc).pFuncFrgList->pFunc->func,
                                 1, 0, &IPTGLOBAL(md.listTables.aInstAfunc));
      }
      
      pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&IPTGLOBAL(md.listTables.instXFuncNListTableHdr),
                                                                  "", ""));
      while(pInstXFuncN)
      {
         while (pInstXFuncN->lists.pQueueList)
         {
            removeQIxFnUriListener(pInstXFuncN->lists.pQueueList->pQueue->listenerQueueId,
                                   1, 0, pInstXFuncN);
         }
         
         while (pInstXFuncN->lists.pQueueFrgList)
         {
            removeQIxFnUriListener(pInstXFuncN->lists.pQueueFrgList->pQueue->listenerQueueId,
                                   1, 0, pInstXFuncN);
         }
         
         while (pInstXFuncN->lists.pFuncList)
         {
            removeFIxFnUriListener(pInstXFuncN->lists.pFuncList->pFunc->func,
                                   1, 0, pInstXFuncN);
         }
         
         while (pInstXFuncN->lists.pFuncFrgList)
         {
            removeFIxFnUriListener(pInstXFuncN->lists.pFuncFrgList->pFunc->func,
                                   1, 0, pInstXFuncN);
         }
      
         pInstXFuncN = (INSTX_FUNCN_ITEM *)((void *)iptUriLabelTab2FindNext(&IPTGLOBAL(md.listTables.instXFuncNListTableHdr),
                                                                      "", ""));
      }

      pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.aInstFuncNListTableHdr),
                                                                  ""));
      while(pAinstFuncN)
      {
         while (pAinstFuncN->lists.pQueueList)
         {
            removeQaIFnUriListener(pAinstFuncN->lists.pQueueList->pQueue->listenerQueueId,
                                   1, 0, pAinstFuncN);
         }
         
         while (pAinstFuncN->lists.pQueueFrgList)
         {
            removeQaIFnUriListener(pAinstFuncN->lists.pQueueFrgList->pQueue->listenerQueueId,
                                   1, 0, pAinstFuncN);
         }
         
         while (pAinstFuncN->lists.pFuncList)
         {
            removeFaIFnUriListener(pAinstFuncN->lists.pFuncList->pFunc->func,
                                   1, 0, pAinstFuncN);
         }
         
         while (pAinstFuncN->lists.pFuncFrgList)
         {
            removeFaIFnUriListener(pAinstFuncN->lists.pFuncFrgList->pFunc->func,
                                   1, 0, pAinstFuncN);
         }
      
         pAinstFuncN = (AINST_FUNCN_ITEM *)((void *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.aInstFuncNListTableHdr),
                                                                     ""));
      }

      pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.instXaFuncListTableHdr),
                                                                  ""));
      while(pInstXaFunc)
      {
         while (pInstXaFunc->lists.pQueueList)
         {
            removeQIxaFUriListener(pInstXaFunc->lists.pQueueList->pQueue->listenerQueueId,
                                   1, 0, pInstXaFunc);
         }
         
         while (pInstXaFunc->lists.pQueueFrgList)
         {
            removeQIxaFUriListener(pInstXaFunc->lists.pQueueFrgList->pQueue->listenerQueueId,
                                   1, 0, pInstXaFunc);
         }
         
         while (pInstXaFunc->lists.pFuncList)
         {
            removeFIxaFUriListener(pInstXaFunc->lists.pFuncList->pFunc->func,
                                   1, 0, pInstXaFunc);
         }
         
         while (pInstXaFunc->lists.pFuncFrgList)
         {
            removeFIxaFUriListener(pInstXaFunc->lists.pFuncFrgList->pFunc->func,
                                   1, 0, pInstXaFunc);
         }
      
         pInstXaFunc = (INSTX_AFUNC_ITEM *)((void *)iptUriLabelTabFindNext(&IPTGLOBAL(md.listTables.instXaFuncListTableHdr),
                                                                     ""));
      }

#endif
      if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "mdPutMsgOnListenerQueue: IPTVosGetSem ERROR\n");
   }
}

