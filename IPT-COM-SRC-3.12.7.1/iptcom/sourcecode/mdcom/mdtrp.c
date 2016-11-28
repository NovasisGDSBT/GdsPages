/*******************************************************************************
 *  COPYRIGHT      : (c) 2006-2014 Bombardier Transportation
 *******************************************************************************
 *  PROJECT        : IPTrain
 *
 *  MODULE         : mdtrp.c
 *
 *  ABSTRACT       : Message data communication transport layer
 *
 *******************************************************************************
 * HISTORY         :
 *	
 * $Id: mdtrp.c 36146 2015-03-24 08:48:45Z gweiss $
 *
 *  CR-7779 (Gerhard Weiss 2014-07-01)
 *          added check for receiving MD frame len (configurable)
 *
 *  CR-8966 (GERHARD WEISS, 2014-02-14)
 *          Undo CR-6084 partitially: remove the "trick" part"
 *
 *  CR-6084 Sequence counter 0 handling impoved and "trick" remote
 *          side to get a seq 0 packet twice
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 * 			TÜV Assessment findings, add NULL pointer check
 *
 *  Internal (Bernd Loehr, 2010-08-13) 
 * 			Old obsolete CVS history removed
 *
 *
 ******************************************************************************/

/*******************************************************************************
* INCLUDES */
#if defined(VXWORKS)
#include <vxWorks.h>
#endif

#include <stdio.h>      
#include <string.h>     

#include "iptcom.h"        /* Common type definitions for IPT */
#include "vos.h"
#include "netdriver.h"
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"
#include "mdcom_priv.h"
#include "mdses.h"
#include "mdtrp.h"

/*******************************************************************************
* DEFINES */

/* Acknowledge message data */
#define ACK_MSG_SIZE     4
#define FRG_ACK_MSG_SIZE 8
#define ACK_CODE_OFF     0
#define ACK_SEQ_NO_OFF   2
#define DEST_IP_OFF      4

/* Transport instance states */
#define TR_INIT     1
#define TR_INIT2    2
#define TR_SEND     3
#define TR_WAIT     4
#define TR_READY    5

/*******************************************************************************
* TYPEDEFS */
 
/*******************************************************************************
* GLOBALS */

/*******************************************************************************
* LOCALS */

/*******************************************************************************
* LOCAL FUNCTIONS */

/*******************************************************************************
NAME:     clearRecSequenceCounter
ABSTRACT: Clear the receive sequence counter list from not valid IP addresses
          if the topo counter has been changed.
RETURNS:  -
*/
static void clearRecSequenceCounter(void)
{
   REC_SEQ_CNT **ppRecSeqCntStr;
   REC_SEQ_CNT *pRecSeqCntStr;
   SEQ_NO_LIST_ITEM *pSeqNoList;
   REC_FRG_SEQ_CNT **ppRecFrgSeqCntStr;
   REC_FRG_SEQ_CNT *pRecFrgSeqCntStr;
   int res;
   UINT8 srcAddrTopoCnt;
   UINT8 destAddrTopoCnt;
   UINT8 currentTopoCnt;
   UINT8 inaugState;
   
   if (!IPTGLOBAL(tdcsim.enableTDCSimulation))
   {
      /* Get current top counter value */
      currentTopoCnt = 0;
      if ((res = tdcGetIptState (&inaugState, &currentTopoCnt)) == TDC_OK)
      {
         if (inaugState == TDC_IPT_INAUGSTATE_OK)
         {
            /* Topo counter changed? */
            if ((currentTopoCnt != 0) && 
                (currentTopoCnt != IPTGLOBAL(md.lastValidRecTopoCnt)))
            {
               IPTGLOBAL(md.lastValidRecTopoCnt) = currentTopoCnt;
               
               /* Check the list for IP addresses with invalid topo counter bits */
               ppRecSeqCntStr = &IPTGLOBAL(md.pFirstRecSeqCnt);
               while ((*ppRecSeqCntStr) != 0)
               {
                  /* Get topocounter bits from IP address */
                  srcAddrTopoCnt = ipAddrGetTopoCnt((*ppRecSeqCntStr)->srcIpAddr);
      
                  /* Is there topo counter value set in the IP address and
                     is the value not equal to the current topo counter value?*/ 
                  if ((srcAddrTopoCnt != 0) && (srcAddrTopoCnt != currentTopoCnt))
                  {
                     /* Save address to be free */
                     pRecSeqCntStr = *ppRecSeqCntStr;
                     pSeqNoList = (*ppRecSeqCntStr)->pSeqNoList;
                     
                     /* Remove from the list */
                     *ppRecSeqCntStr = (*ppRecSeqCntStr)->pNext;

                     /* deallocate send sequence structure */
                     res = IPTVosFree((BYTE *)pRecSeqCntStr);
                     if(res != 0)
                     {
                        IPTVosPrint1(IPT_ERR, 
                           "Failed to free memory, code=%#x\n",
                           res);
                     }

                     if (pSeqNoList)
                     {
                        /* deallocate send sequence list */
                        res = IPTVosFree((BYTE *)pSeqNoList);
                        if(res != 0)
                        {
                           IPTVosPrint1(IPT_ERR, 
                              "Failed to free memory, code=%#x\n",
                              res);
                        }
                     }
                  }
                  else
                  {
                     ppRecSeqCntStr = &((*ppRecSeqCntStr)->pNext);
                  }
               }

               /* Check the list for IP addresses with invalid topo counter bits */
               ppRecFrgSeqCntStr = &IPTGLOBAL(md.pFirstFrgRecSeqCnt);
               while ((*ppRecFrgSeqCntStr) != 0)
               {
                  
                  /* Get topocounter bits from IP address */
                  srcAddrTopoCnt = ipAddrGetTopoCnt((*ppRecFrgSeqCntStr)->srcIpAddr);
                  destAddrTopoCnt = ipAddrGetTopoCnt((*ppRecFrgSeqCntStr)->destIpAddr);
                  
                  /* Is there topo counter value set in the IP address and
                     is the value not equal to the current topo counter value?*/ 
                  if (((srcAddrTopoCnt != 0) && (srcAddrTopoCnt != currentTopoCnt)) ||
                      ((destAddrTopoCnt != 0) && (destAddrTopoCnt != currentTopoCnt)))
                  {
                     /* Save address to be free */
                     pRecFrgSeqCntStr = *ppRecFrgSeqCntStr;
                     pSeqNoList = (*ppRecFrgSeqCntStr)->pSeqNoList;
                     
                     /* Remove from the list */
                     *ppRecFrgSeqCntStr = (*ppRecFrgSeqCntStr)->pNext;

                     /* deallocate send sequence structure */
                     res = IPTVosFree((BYTE *)pRecFrgSeqCntStr);
                     if(res != 0)
                     {
                        IPTVosPrint1(IPT_ERR, 
                           "Failed to free memory, code=%#x\n",
                           res);
                     }

                     if (pSeqNoList)
                     {
                        /* deallocate send sequence list */
                        res = IPTVosFree((BYTE *)pSeqNoList);
                        if(res != 0)
                        {
                           IPTVosPrint1(IPT_ERR, 
                              "Failed to free memory, code=%#x\n",
                              res);
                        }
                     }
                  }
                  else
                  {
                     ppRecFrgSeqCntStr = &((*ppRecFrgSeqCntStr)->pNext);
                  }
               }
            }
         }
      }
   }
}

/*******************************************************************************
NAME:     checkRecSequenceCounter
ABSTRACT: Check if a message already has been received or not
RETURNS:  1 = Already received
          0 = Not received before
          IPT_ERROR = error
*/
static int checkRecSequenceCounter(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,
#endif
   UINT32 srcIpAddr,              /* Source IP address */
   UINT16 sequenceNumber,         /* Sequence number */
   SEQ_NO_LIST_ITEM **ppListItem) /* OUT Pointer to listItem */
{
   REC_SEQ_CNT **ppRecSeqCntStr;
   int i;
   int ret;
   UINT16 max = IPTGLOBAL(md.maxStoredSeqNo);
   
   ppRecSeqCntStr = &IPTGLOBAL(md.pFirstRecSeqCnt);
   while ((*ppRecSeqCntStr) != 0)
   {
      /* Search for the received IP address */
#ifdef TARGET_SIMU
      if ((((*ppRecSeqCntStr)->srcIpAddr) == srcIpAddr) &&
          (((*ppRecSeqCntStr)->simuIpAddr) == simuIpAddr))
#else
      if (((*ppRecSeqCntStr)->srcIpAddr) == srcIpAddr)
#endif
      {
         if ((sequenceNumber == 0) /* && ((*ppRecSeqCntStr)->lastSeqNo != 0) */)
         {
            /* Clear stored sequence number list */
            for (i=0; i<max; i++)
            {
               (*ppRecSeqCntStr)->pSeqNoList[i].seqNo = 0;
            }
            (*ppRecSeqCntStr)->lastSeqNo = 0;
            *ppListItem = &((*ppRecSeqCntStr)->pSeqNoList[0]);
            return(0);
         }
         (*ppRecSeqCntStr)->lastSeqNo = sequenceNumber;
         i = (*ppRecSeqCntStr)->lastIndex;
         /* Check if the sequence number already has been received */
         do
         {
            if (sequenceNumber == (*ppRecSeqCntStr)->pSeqNoList[i].seqNo)
            {
               /* This sequence number has already been received */
               *ppListItem = &((*ppRecSeqCntStr)->pSeqNoList[i]);
               return(1);
            }
            i--;
            if (i < 0)
            {
               i = max - 1;
            }
         }
         while(i != (*ppRecSeqCntStr)->lastIndex );

         /* This sequence number has not been received */

         /* Save the sequence number */
         (*ppRecSeqCntStr)->lastIndex++;
         if ((*ppRecSeqCntStr)->lastIndex >= max)
         {
            (*ppRecSeqCntStr)->lastIndex = 0;
         }
         (*ppRecSeqCntStr)->pSeqNoList[(*ppRecSeqCntStr)->lastIndex].seqNo = 
          sequenceNumber;
         *ppListItem = &((*ppRecSeqCntStr)->pSeqNoList[(*ppRecSeqCntStr)->lastIndex]);
         return(0);
      }
      ppRecSeqCntStr = &((*ppRecSeqCntStr)->pNext);
   }

   /* This is the first time for this source IP address */
   ret = 0;

   /* Allocate memory for a new sequence counter structure */
   *ppRecSeqCntStr = (REC_SEQ_CNT *)IPTVosMalloc(sizeof(REC_SEQ_CNT));
   if ((*ppRecSeqCntStr) != 0)
   {
      (*ppRecSeqCntStr)->pSeqNoList = (SEQ_NO_LIST_ITEM *)IPTVosMalloc(max * sizeof(SEQ_NO_LIST_ITEM)); /*lint !e433 !e826 Size is OK but calculated */

      if ((*ppRecSeqCntStr)->pSeqNoList != 0)
      {
         (*ppRecSeqCntStr)->pNext = 0;
         (*ppRecSeqCntStr)->srcIpAddr = srcIpAddr;
#ifdef TARGET_SIMU
         (*ppRecSeqCntStr)->simuIpAddr = simuIpAddr;
#endif
         (*ppRecSeqCntStr)->lastIndex = 0;
         (*ppRecSeqCntStr)->lastSeqNo = sequenceNumber;
         (*ppRecSeqCntStr)->pSeqNoList[0].seqNo = sequenceNumber;
          *ppListItem = &((*ppRecSeqCntStr)->pSeqNoList[0]);
     
      
         for (i=1; i<max; i++)
         {
            (*ppRecSeqCntStr)->pSeqNoList[i].seqNo = 0;
         }
      }
      else
      {
         ret = IPT_ERROR;

         (void)IPTVosFree((BYTE *)*ppRecSeqCntStr);
         IPTVosPrint5(IPT_ERR,
            "checkRecSequenceCounter: Out of memory. Requested size=%d srcIpAddr=%d.%d.%d.%d\n",
            max * sizeof(SEQ_NO_LIST_ITEM),
            (srcIpAddr & 0xff000000) >> 24,
            (srcIpAddr & 0xff0000) >> 16,
            (srcIpAddr & 0xff00) >> 8,
            srcIpAddr & 0xff);
      }
   }
   else
   {
      ret = IPT_ERROR;
      
      IPTVosPrint5(IPT_ERR,
         "checkRecSequenceCounter: Out of memory. Requested size=%d srcIpAddr=%d.%d.%d.%d\n",
         sizeof(REC_SEQ_CNT),
         (srcIpAddr & 0xff000000) >> 24,
         (srcIpAddr & 0xff0000) >> 16,
         (srcIpAddr & 0xff00) >> 8,
         srcIpAddr & 0xff);
   }

   /* Check if there is any not valid IP address that can be removed 
      from the list of sequence counter structures */
   clearRecSequenceCounter();

   return(ret);
}

/*******************************************************************************
NAME:     checkRecFrgSequenceCounter
ABSTRACT: Check if a message already has been received or not
RETURNS:  1 = Already received
          0 = Not received before
          IPT_ERROR = error
*/
static int checkRecFrgSequenceCounter(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,
#endif
   UINT32 srcIpAddr,            /* Source IP address */
   UINT32 destIpAddr,           /* Destination IP address */
   UINT16 sequenceNumber,       /* Sequence number */
   SEQ_NO_LIST_ITEM **ppListItem) /* Pointer to listItem */
{
   REC_FRG_SEQ_CNT **ppRecFrgSeqCntStr;
   int i;
   int ret;
   UINT16 max = IPTGLOBAL(md.maxStoredSeqNo);
   
   ppRecFrgSeqCntStr = &IPTGLOBAL(md.pFirstFrgRecSeqCnt);
   while ((*ppRecFrgSeqCntStr) != 0)
   {
      /* Search for the received IP address */
#ifdef TARGET_SIMU
      if ((((*ppRecFrgSeqCntStr)->srcIpAddr) == srcIpAddr) &&
          (((*ppRecFrgSeqCntStr)->destIpAddr) == destIpAddr) &&
          (((*ppRecFrgSeqCntStr)->simuIpAddr) == simuIpAddr))
#else
      if ((((*ppRecFrgSeqCntStr)->srcIpAddr) == srcIpAddr) &&
          (((*ppRecFrgSeqCntStr)->destIpAddr) == destIpAddr))
#endif
      {
         if ((sequenceNumber == 0) && ((*ppRecFrgSeqCntStr)->lastSeqNo != 0))
         {
            /* Clear stored sequence number list */
            for (i=0; i<max; i++)
            {
               (*ppRecFrgSeqCntStr)->pSeqNoList[i].seqNo = 0;
            }
            (*ppRecFrgSeqCntStr)->lastSeqNo = 0;
            *ppListItem = &((*ppRecFrgSeqCntStr)->pSeqNoList[0]);
            return(0);
         }
         (*ppRecFrgSeqCntStr)->lastSeqNo = sequenceNumber;
         i = (*ppRecFrgSeqCntStr)->lastIndex;
         /* Check if the sequence number already has been received */
         do
         {
            if (sequenceNumber == (*ppRecFrgSeqCntStr)->pSeqNoList[i].seqNo)
            {
               /* This sequence number has already been received */
               *ppListItem = &((*ppRecFrgSeqCntStr)->pSeqNoList[i]);
               return(1);
            }
            i--;
            if (i < 0)
            {
               i = max - 1;
            }
         }
         while(i != (*ppRecFrgSeqCntStr)->lastIndex );

         /* This sequence number has not been received */

         /* Save the sequence number */
         (*ppRecFrgSeqCntStr)->lastIndex++;
         if ((*ppRecFrgSeqCntStr)->lastIndex >= max)
         {
            (*ppRecFrgSeqCntStr)->lastIndex = 0;
         }
         (*ppRecFrgSeqCntStr)->pSeqNoList[(*ppRecFrgSeqCntStr)->lastIndex].seqNo = 
          sequenceNumber;
         *ppListItem = &((*ppRecFrgSeqCntStr)->pSeqNoList[(*ppRecFrgSeqCntStr)->lastIndex]);
         return(0);
      }
      ppRecFrgSeqCntStr = &((*ppRecFrgSeqCntStr)->pNext);
   }

   /* This is the first time for this source IP address */
   ret = 0;

   /* Allocate memory for a new sequence counter structure */
   *ppRecFrgSeqCntStr = (REC_FRG_SEQ_CNT *)IPTVosMalloc(sizeof(REC_FRG_SEQ_CNT));
   if ((*ppRecFrgSeqCntStr) != 0)
   {
      (*ppRecFrgSeqCntStr)->pSeqNoList = (SEQ_NO_LIST_ITEM *)IPTVosMalloc(max * sizeof(SEQ_NO_LIST_ITEM)); /*lint !e433 !e826 Size is OK but calculated */
      if ((*ppRecFrgSeqCntStr)->pSeqNoList != 0)
      {
         (*ppRecFrgSeqCntStr)->pNext = 0;
         (*ppRecFrgSeqCntStr)->srcIpAddr = srcIpAddr;
         (*ppRecFrgSeqCntStr)->destIpAddr = destIpAddr;
#ifdef TARGET_SIMU
         (*ppRecFrgSeqCntStr)->simuIpAddr = simuIpAddr;
#endif
         (*ppRecFrgSeqCntStr)->lastIndex = 0;
         (*ppRecFrgSeqCntStr)->lastSeqNo = sequenceNumber;
         (*ppRecFrgSeqCntStr)->pSeqNoList[0].seqNo = sequenceNumber;
          *ppListItem = &((*ppRecFrgSeqCntStr)->pSeqNoList[0]);
     
         for (i=1; i<max; i++)
         {
            (*ppRecFrgSeqCntStr)->pSeqNoList[i].seqNo = 0;
         }
      }
      else
      {
         ret = IPT_ERROR;

         (void)IPTVosFree((BYTE *)*ppRecFrgSeqCntStr);
         IPTVosPrint5(IPT_ERR,
            "checkRecSequenceCounter: Out of memory. Requested size=%d srcIpAddr=%d.%d.%d.%d\n",
            max * sizeof(SEQ_NO_LIST_ITEM),
            (srcIpAddr & 0xff000000) >> 24,
            (srcIpAddr & 0xff0000) >> 16,
            (srcIpAddr & 0xff00) >> 8,
            srcIpAddr & 0xff);
      }
   }
   else
   {
      ret = IPT_ERROR;

      IPTVosPrint5(IPT_ERR,
         "checkRecFrgSequenceCounter: Out of memory. Requested size=%d srcIpAddr=%d.%d.%d.%d\n",
         sizeof(REC_FRG_SEQ_CNT),
         (srcIpAddr & 0xff000000) >> 24,
         (srcIpAddr & 0xff0000) >> 16,
         (srcIpAddr & 0xff00) >> 8,
         srcIpAddr & 0xff);
   }

   /* Check if there is any not valid IP address that can be removed 
      from the list of sequence counter structures */
   clearRecSequenceCounter();

   return(ret);
}

/*******************************************************************************
NAME:     createSendSeqNoList
ABSTRACT: Creates a list for used and not yet acknowledged sequence numbers
          for a certain IP address. 
RETURNS:  -
*/
static void createSendSeqNoList(
   TRANSPORT_INSTANCE *pTrInstance)  /* Pointer to transport instance */
{
   SEND_SEQ_CNT *pSendSeqCntStr;
   int i;
   UINT16 maxActiveSeqNo;

   pSendSeqCntStr = pTrInstance->pSendSeqCntStr;
   maxActiveSeqNo = pSendSeqCntStr->maxActiveSeqNo;
   if (maxActiveSeqNo == 0)
   {
      maxActiveSeqNo = 1;
      pSendSeqCntStr->maxActiveSeqNo = 1;
   }
   /* Allocate memory for a new sequence counter structure */
   pSendSeqCntStr->pActiveSeqNoList = (UINT16 *)IPTVosMalloc(sizeof(UINT16) * maxActiveSeqNo);
   if ((pSendSeqCntStr->pActiveSeqNoList) != 0)
   {
      for (i=0; i < maxActiveSeqNo; i++)
      {
         pSendSeqCntStr->pActiveSeqNoList[i] = 0;
      }
      
      pSendSeqCntStr->seqNoSync = SYNC;
   }
   else
   {
      IPTVosPrint1(IPT_ERR,
         "createSendSeqNoList: Out of memory. Requested size=%d\n",
         sizeof(UINT16) * maxActiveSeqNo);
      pSendSeqCntStr->seqNoSync = NOT_SYNC;
   }
}

/*******************************************************************************
NAME:     getSendSequenceCounter
ABSTRACT: Search for a sequence counter structure for a destination IP address.
          If not found a new is created.
          The sequence counter structure is used to handle the sequence counter
          and an active transport instances counter for a certain IP address. 
RETURNS:  0 if OK, !=0 if not.
*/
static int getSendSequenceCounter(
   TRANSPORT_INSTANCE *pTrInstance)  /* Pointer to transport instance */
{
   int res;
   SEND_SEQ_CNT **ppSendSeqCntStr;

   res = IPTVosGetSem(&IPTGLOBAL(md.SendSeqSemId), IPT_WAIT_FOREVER);
   if (res != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "getSendSequenceCounter: IPTVosGetSem ERROR\n");
      return(res);
   }
   
   ppSendSeqCntStr = &IPTGLOBAL(md.pFirstSendSeqCnt);
   while ((*ppSendSeqCntStr) != 0)
   {
      /* Search for the destination IP address */
#ifdef TARGET_SIMU
      if ((((*ppSendSeqCntStr)->destIpAddr) == pTrInstance->comPar.destIpAddr) &&
          (((*ppSendSeqCntStr)->simuIpAddr) == pTrInstance->comPar.simDevIpAddr))
#else
      if (((*ppSendSeqCntStr)->destIpAddr) == pTrInstance->comPar.destIpAddr)
#endif
      {
         pTrInstance->pSendSeqCntStr = *ppSendSeqCntStr;
      
         if(IPTVosPutSemR(&IPTGLOBAL(md.SendSeqSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
         return((int)IPT_OK);
      }
      ppSendSeqCntStr = &((*ppSendSeqCntStr)->pNext);
   }
   
   /* This is the first time for this destination IP address */
   /* Allocate memory for a new sequence counter structure */
   *ppSendSeqCntStr = (SEND_SEQ_CNT *)IPTVosMalloc(sizeof(SEND_SEQ_CNT));
   if ((*ppSendSeqCntStr) != 0)
   {
      (*ppSendSeqCntStr)->pNext = 0;
      (*ppSendSeqCntStr)->destIpAddr = pTrInstance->comPar.destIpAddr;
#ifdef TARGET_SIMU
      (*ppSendSeqCntStr)->simuIpAddr = pTrInstance->comPar.simDevIpAddr;
#endif
      (*ppSendSeqCntStr)->sendSequenceNo = 0;
      pTrInstance->pSendSeqCntStr = *ppSendSeqCntStr;
      (*ppSendSeqCntStr)->pActiveSeqNoList = 0;
      (*ppSendSeqCntStr)->seqNoSync = NOT_SYNC;
     
      if(IPTVosPutSemR(&IPTGLOBAL(md.SendSeqSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
      return((int)IPT_OK);
   }
   else
   {
      IPTVosPrint5(IPT_ERR,
         "getSendSequenceCounter: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d\n",
         sizeof(SEND_SEQ_CNT),
         (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
         (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
         (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
         pTrInstance->comPar.destIpAddr & 0xff);
      pTrInstance->pSendSeqCntStr = 0;
     
      if(IPTVosPutSemR(&IPTGLOBAL(md.SendSeqSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
      return((int)IPT_MEM_ERROR);
   }
}

/*******************************************************************************
NAME:     checkSendSequenceCounter
ABSTRACT: Check that the difference between the current and the oldest not 
          acknowled sequence number is not greater than maximum 
          (IPTGLOBAL(md.maxStoredSeqNo)). This is necessary to allow the receiver
          to keep track of already received messages. The maximum is the number
          of sequence number stored in the receiver end.
RETURNS:  0 = Sending is not allowed
          1 = Sending allowed
*/
static int checkSendSequenceCounter(
   TRANSPORT_INSTANCE *pTrInstance)  /* Pointer to transport instance */
{
   int i = 0;
   int freeIndex = 0;
   int freeFound = 0;

   UINT16 lastSeqNo;    /* Last used sequence number */
   UINT16 *pActiveSeqNoList;/* Pointer to array of not acknowledged sequence numbers */
   UINT16 maxNoActiveSeqNo;

   if (pTrInstance->pSendSeqCntStr->seqNoSync == SYNC )
   {
      lastSeqNo = pTrInstance->pSendSeqCntStr->sendSequenceNo;
      pActiveSeqNoList = pTrInstance->pSendSeqCntStr->pActiveSeqNoList;
      maxNoActiveSeqNo = pTrInstance->pSendSeqCntStr->maxActiveSeqNo;

      for (i = 0; i < maxNoActiveSeqNo; i++)
      {
         /* Not acknowledged sequence number in the array? */
         if (pActiveSeqNoList[i] != 0 )
         {
            /* Check that the difference between the next sequence number to be  
               used and the not acknowledged sequence is less than maximum */
            if (lastSeqNo >= pActiveSeqNoList[i])
            {
               if ((lastSeqNo - pActiveSeqNoList[i]) >= maxNoActiveSeqNo - 1)
               {
                  /* To big difference */
                  return(0);
               }
            }
            /* The sequnce counter has turn around the maximum value for UINT16 */
            else
            {
               if ((lastSeqNo + (0xffff - pActiveSeqNoList[i]))
                    >=  maxNoActiveSeqNo - 1)
               {
                  /* To big difference */
                  return(0);
               }
            }
         }
         else if (!freeFound)
         {
            freeFound = 1;
            freeIndex = i;
         }
      }

      if (freeFound)
      {
         /* Sending can be done */

         /* Increase sequence number for the the destination IP address */
         lastSeqNo = ++pTrInstance->pSendSeqCntStr->sendSequenceNo;
      
         /* Avoid zero sequence counter */
         if (lastSeqNo == 0)
         {
            lastSeqNo = ++pTrInstance->pSendSeqCntStr->sendSequenceNo;
         }
         pTrInstance->pSendSeqCntStr->pActiveSeqNoList[freeIndex] = lastSeqNo;
         pTrInstance->sequenceNumber = lastSeqNo;
         pTrInstance->seqCntInd = freeIndex;
         return(1);
      }
      else
      {
         /* To many active sendings is going on */
         return(0);
      }
   }
   else if (pTrInstance->pSendSeqCntStr->seqNoSync == NOT_SYNC)
   {
      pTrInstance->pSendSeqCntStr->seqNoSync = WAIT_FOR_SYNC;
      pTrInstance->sequenceNumber = 0;
      return(1);
   }
   else
   {
      return(0);
   }
}

/*******************************************************************************
NAME:     clearSendSeqCntLists
ABSTRACT: Search in the stored send sequence number lists for a destination IP
          address with old topocounter bits.
          If not found the list removed if no active tr instance is using the list.
RETURNS:  0 if OK, i.e. all can be cleared, !=0 if not.
*/
static int clearSendSeqCntLists(
   UINT8 topoCounter)  /* Current topo counter value */
{
   int ret = IPT_OK;
   int res;
   int found;
   UINT8 addrTopoCnt;
   SEND_SEQ_CNT **ppSendSeqCntStr;
   SEND_SEQ_CNT *pToBeFree;
   UINT16 *pActiveSeqNoList; /* Pointer to array of sequence numbers */
   TRANSPORT_INSTANCE *pTrInstance;

   res = IPTVosGetSem(&IPTGLOBAL(md.SendSeqSemId), IPT_WAIT_FOREVER);
   if (res != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "clearSendSeqCntLists: IPTVosGetSem ERROR\n");
      return(res);
   }

   ppSendSeqCntStr = &IPTGLOBAL(md.pFirstSendSeqCnt);
   while ((*ppSendSeqCntStr) != 0)
   {
      /* Get topocounter bits from IP address */
      addrTopoCnt = ipAddrGetTopoCnt((*ppSendSeqCntStr)->destIpAddr);
      
      /* Is there topo counter value set in the IP address and
         is the value not equal to the current topo counter value?*/ 
      if ((addrTopoCnt != 0) && (addrTopoCnt != topoCounter))
      {
         found = 0;
         
         /* Check that no TR instance is using the sequence counter structure.
            No semaphore protection is needed as this is call within
            MD process, which is the only part that is changing the list */ 
         pTrInstance = IPTGLOBAL(md.pFirstTrInstance);
         while (pTrInstance)
         {
            if (pTrInstance->pSendSeqCntStr ==  *ppSendSeqCntStr)
            {
               /* A TR instance is still using the list for the IP address */
               ret = IPT_ERROR;
               found = 1;
              
               /* break while loop */
               break;  
            }

            /* next */
            pTrInstance = pTrInstance->pNext;
         }  
         
         /* No TR instance is using the list? */
         if (!found)
         {
            /* Save addresses to buffers to be freed */
            pToBeFree = *ppSendSeqCntStr;
            pActiveSeqNoList = (*ppSendSeqCntStr)->pActiveSeqNoList;

            /* Remove from list */
            *ppSendSeqCntStr = (*ppSendSeqCntStr)->pNext;

            /* deallocate send sequence structure */
            res = IPTVosFree((BYTE *)pToBeFree);
            if(res != 0)
            {
               IPTVosPrint1(IPT_ERR, 
                  "Failed to free memory, code=%#x\n",
                  res);
            }

            if (pActiveSeqNoList)
            {
               /* deallocate send sequence list */
               res = IPTVosFree((BYTE *)pActiveSeqNoList);
               if(res != 0)
               {
                  IPTVosPrint1(IPT_ERR, 
                     "Failed to free memory, code=%#x\n",
                     res);
               }
            }
         }
         else
         {
            ppSendSeqCntStr = &(*ppSendSeqCntStr)->pNext;
         }
      }
      else
      {  
         ppSendSeqCntStr = &(*ppSendSeqCntStr)->pNext;
      }
   }

   if(IPTVosPutSemR(&IPTGLOBAL(md.SendSeqSemId)) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
   }

   return(ret);
}

/*******************************************************************************
NAME:     deleteTrInstance
ABSTRACT: Delete a transport instance. 
          The used memory is freed. 
          The pointer to next instance of the deleted instance is copied to the
          previous instance or to the list start pointer (pFirstTrInstance).
RETURNS:  -
*/
static void deleteTrInstance(
   TRANSPORT_INSTANCE *pTrInstance)  /* Pointer to transport instance */
{
   int res;
   TRANSPORT_INSTANCE **ppTrInstance;
   TRANSPORT_INSTANCE *pPrevTrInstance;
   
   ppTrInstance = &IPTGLOBAL(md.pFirstTrInstance);
   pPrevTrInstance = 0;
   
   while (*ppTrInstance)
   {
      if (*ppTrInstance == pTrInstance)
      {
         /* Protect the list while changing */
         res = IPTVosGetSem(&IPTGLOBAL(md.trListSemId), IPT_WAIT_FOREVER);
         if (res == IPT_OK)
         {
            *ppTrInstance = pTrInstance->pNext;
         
            /* Last instance to be deleted? */
            if (IPTGLOBAL(md.pLastTrInstance) == pTrInstance)
            {
               IPTGLOBAL(md.pLastTrInstance) = pPrevTrInstance;
            }
            if(IPTVosPutSemR(&IPTGLOBAL(md.trListSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
         
            /* deallocate send message buffer */
            res = IPTVosFree((BYTE*)pTrInstance->pSendMsg);
            if(res != 0)
            {
               IPTVosPrint1(IPT_ERR,
                            "Failed to free transport message buffer, code=%#x\n",
                            res);
            }
         
            /* deallocate transport instance */
            res = IPTVosFree((BYTE *)pTrInstance);
            if(res != 0)
            {
               IPTVosPrint1(IPT_ERR,
                            "Failed to free transport instance memory, code=%#x\n",
                            res);
            }
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "deleteTrInstance: IPTVosGetSem ERROR\n");
         }
         
         /* finished */
         break;
      }
      else
      {
         /* next */
         pPrevTrInstance = *ppTrInstance;
         ppTrInstance = &(*ppTrInstance)->pNext;
      }
   }  
}

/*******************************************************************************
NAME:     unicastInstance
ABSTRACT: State machine for unicast transport instance.
RETURNS:  -
*/
static void unicastInstance(
   TRANSPORT_INSTANCE *pTrInstance) /* Pointer to transport instance */
{
   char *pTemp;
   int res;
   int trResCode = 0;
   QUEUE_MSG resultMsg;
   UINT32 deltaTime;
   
   do
   {
      switch (pTrInstance->state)
      {
      case TR_INIT:
         
         pTrInstance->state = TR_INIT2;
         
         /* No break continue with init 2 */
         
      case TR_INIT2:
         
         /* Check if sending can be done */
         if (checkSendSequenceCounter(pTrInstance))
         {
            pTrInstance->state = TR_SEND;
            
            /* Prepare message sequence counter */
            *((UINT16*)(pTrInstance->pSendMsg + SEQ_NO_OFF)) = TOWIRE16(pTrInstance->sequenceNumber); /*lint !e826 Type cast OK */
            
            /* Header frame check sequence */
            iptAddDataFCS((BYTE *)pTrInstance->pSendMsg,
               (UINT16)FROMWIRE16(*((UINT16*)(pTrInstance->pSendMsg + HEAD_LENGTH_OFF)))); /*lint !e826 Type cast OK */
            
            /* No break continue with send */
         }
         else
         {
            /* Wait until there less active transport instances */
            break;
         }
   
         /* fall through */
         case TR_SEND:
         pTrInstance->ackReceived = 0;
         pTrInstance->timeOutTime = IPTVosGetMilliSecTimer() +
                                    pTrInstance->comPar.ackTimeOutVal;
         /* Send message */
#ifdef TARGET_SIMU      
         res = IPTDriveMDSocketSend(TOWIRE32(pTrInstance->comPar.destIpAddr),
                                    TOWIRE32(pTrInstance->comPar.simDevIpAddr),
                                    (BYTE *)pTrInstance->pSendMsg,
                                    pTrInstance->sendMsgLength,
                                    pTrInstance->comPar.mdSendSocket );
#else
         res = IPTDriveMDSocketSend(TOWIRE32(pTrInstance->comPar.destIpAddr),
                                    (BYTE *)pTrInstance->pSendMsg,
                                    pTrInstance->sendMsgLength,
                                    pTrInstance->comPar.mdSendSocket );
#endif
         
         if (res == (int)IPT_OK)
         {
            pTrInstance->sendCnt++;
            pTrInstance->state = TR_WAIT;
         }
         else
         {
            trResCode = MD_SEND_FAILED;
            pTrInstance->state = TR_READY;
            IPTVosPrint7(IPT_ERR,
               "Send error=%#x ComId=%d pCallerRef=%#x IPaddr=%d.%d.%d.%d\n",
               res,
               pTrInstance->comId,
               pTrInstance->pCallerRef,
               (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
               (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
               (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
               pTrInstance->comPar.destIpAddr & 0xff);
         }
         break;
         
      case TR_WAIT:
         
         if (pTrInstance->ackReceived )
         {
            if (pTrInstance->ackCode == ACK_WRONG_FCS)
            {
               if (pTrInstance->sendCnt <= pTrInstance->comPar.maxResend )
               {
                  IPTVosPrint7(IPT_WARN,
                     "Ack message received with Ack code = wrong frame checksum. ComId=%d pCallerRef=%#x SeqNo=%d IPaddr=%d.%d.%d.%d\n",
                     pTrInstance->comId,
                     pTrInstance->pCallerRef,
                     pTrInstance->sequenceNumber,
                     (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
                     (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
                     (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
                     pTrInstance->comPar.destIpAddr & 0xff);
              
                  IPTGLOBAL(md.mdCnt.mdOutRetransmissions)++;

                  pTrInstance->state = TR_SEND;
               }
               else
               {
                  IPTVosPrint7(IPT_WARN,
                     "Maximum resend exceeded. Ack code = wrong frame checksum. ComId=%d pCallerRef=%#x SeqNo=%d IPaddr=%d.%d.%d.%d\n",
                     pTrInstance->comId,
                     pTrInstance->pCallerRef,
                     pTrInstance->sequenceNumber,
                     (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
                     (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
                     (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
                     pTrInstance->comPar.destIpAddr & 0xff);
                  trResCode = MD_NO_ACK_RECEIVED;
                  pTrInstance->state = TR_READY;
               }
            }
            else
            {
               if (pTrInstance->ackCode == ACK_OK)
               {
                  trResCode = MD_SEND_OK;
               }
               else
               {
                  if (pTrInstance->ackCode == ACK_DEST_UKNOWN)
                  {
                     trResCode = MD_NO_LISTENER;

                     IPTVosPrint6(IPT_WARN,
                        "Ack received with code = No listener. ComId=%d pCallerRef=%#x IPaddr=%d.%d.%d.%d\n",
                        pTrInstance->comId,
                        pTrInstance->pCallerRef,
                        (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
                        (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
                        (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
                        pTrInstance->comPar.destIpAddr & 0xff);
                  }
                  else if (pTrInstance->ackCode == ACK_BUFFER_NOT_AVAILABLE)
                  {
                     trResCode = MD_NO_BUF_AVAILABLE;

                     IPTVosPrint6(IPT_WARN,
                        "Ack received with code = Queue/memory full. ComId=%d pCallerRef=%#x IPaddr=%d.%d.%d.%d\n",
                        pTrInstance->comId,
                        pTrInstance->pCallerRef,
                        (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
                        (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
                        (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
                        pTrInstance->comPar.destIpAddr & 0xff);
                  }
                  else if (pTrInstance->ackCode == ACK_WRONG_DATA)
                  {
                     trResCode = MD_WRONG_DATA;

                     IPTVosPrint6(IPT_WARN,
                        "Ack received with code = Wrong data. ComId=%d pCallerRef=%#x IPaddr=%d.%d.%d.%d\n",
                        pTrInstance->comId,
                        pTrInstance->pCallerRef,
                        (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
                        (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
                        (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
                        pTrInstance->comPar.destIpAddr & 0xff);
                  }
                  else
                  {
                     /* Unknown ack code received */
                     trResCode = MD_WRONG_DATA;
                     IPTVosPrint7(IPT_ERR,
                        "Ack message received with Unknown Ack code=%#x. ComId=%d pCallerRef=%#x IPaddr=%d.%d.%d.%d\n",
                        pTrInstance->ackCode,
                        pTrInstance->comId,
                        pTrInstance->pCallerRef,
                        (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
                        (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
                        (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
                        pTrInstance->comPar.destIpAddr & 0xff);
                  }
               }
               pTrInstance->state = TR_READY;
            }
         }
         else
         {
            deltaTime = pTrInstance->timeOutTime - IPTVosGetMilliSecTimer() -1;
            if (deltaTime >= pTrInstance->comPar.ackTimeOutVal)
            {
               if (pTrInstance->sendCnt <= pTrInstance->comPar.maxResend )
               {
                  IPTGLOBAL(md.mdCnt.mdOutRetransmissions)++;
                  pTrInstance->state = TR_SEND;
               }
               else
               {
                  IPTVosPrint7(IPT_WARN,
                     "Ack message not received within time-out. ComId=%d pCallerRef=%#x SeqNo=%d IPaddr=%d.%d.%d.%d\n",
                     pTrInstance->comId,
                     pTrInstance->pCallerRef,
                     pTrInstance->sequenceNumber,
                     (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
                     (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
                     (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
                     pTrInstance->comPar.destIpAddr & 0xff);
                  trResCode = MD_NO_ACK_RECEIVED;
                  pTrInstance->state = TR_READY;
               }
            }
         }
            
         break;
            
      default:
         
         break;
         
         }    
         
   }

   while((pTrInstance->state !=  TR_WAIT) && 
      (pTrInstance->state !=  TR_READY) &&
      (pTrInstance->state !=  TR_INIT2) );
   
   if (pTrInstance->state == TR_READY)
   {
      if (pTrInstance->trType == UNICAST_OR_FRG_REPORT_TRANSPORT_TYPE)
      {
         if (pTrInstance->callerQueueId != 0)
         {
            resultMsg.msgInfo.comId = pTrInstance->comId;
            resultMsg.msgInfo.msgType = MD_MSGTYPE_RESULT;
            resultMsg.msgInfo.sessionId = 0;
            resultMsg.msgInfo.srcIpAddr = pTrInstance->replierIpAddr;
            resultMsg.msgInfo.resultCode = trResCode;
            resultMsg.msgInfo.noOfResponses = 0;
            resultMsg.msgInfo.pCallerRef = (void *)pTrInstance->pCallerRef;
            resultMsg.msgInfo.userStatus = 0;
            resultMsg.msgInfo.topoCnt = 0;
            resultMsg.msgInfo.responseTimeout = 0;
            resultMsg.msgInfo.destURI[0] = 0;
            resultMsg.msgInfo.srcURI[0] = 0;
            resultMsg.msgLength = 0;
            resultMsg.pMsgData = NULL;
            
            
            /* Send result on caller queue */
            if (IPTVosSendMsgQueue((IPT_QUEUE *)((void *)&(pTrInstance->callerQueueId)),
               (char *)&resultMsg,
               sizeof(QUEUE_MSG)) != (int)IPT_OK)
            {
               pTemp = getQueueItemName(pTrInstance->callerQueueId);
               IPTVosPrint5(IPT_ERR,
                "ERROR sending Result message on queue ID=%#x Name=%s ComId=%d resultCode=%d pCallerRef=%#x\n",
                 pTrInstance->callerQueueId, 
                 (pTemp != NULL)?pTemp:"None",
                 resultMsg.msgInfo.comId,
                 resultMsg.msgInfo.resultCode, resultMsg.msgInfo.pCallerRef);
            }

            /* Clear queue id to allow destroy the queue */
            pTrInstance->callerQueueId = 0;
         }
         
         if (pTrInstance->callerFunc != 0)
         {
            resultMsg.msgInfo.comId = pTrInstance->comId;
            resultMsg.msgInfo.msgType = MD_MSGTYPE_RESULT;
            resultMsg.msgInfo.sessionId = 0;
            resultMsg.msgInfo.srcIpAddr = pTrInstance->replierIpAddr;
            resultMsg.msgInfo.resultCode = trResCode;
            resultMsg.msgInfo.noOfResponses = 0;
            resultMsg.msgInfo.pCallerRef = (void *)pTrInstance->pCallerRef;
            resultMsg.msgInfo.userStatus = 0;
            resultMsg.msgInfo.topoCnt = 0;
            resultMsg.msgInfo.responseTimeout = 0;
            resultMsg.msgInfo.destURI[0] = 0;
            resultMsg.msgInfo.srcURI[0] = 0;
            resultMsg.msgLength = 0;
            resultMsg.pMsgData = NULL;
            
            
            /* Call the caller callback function with send result */
            pTrInstance->callerFunc(&resultMsg.msgInfo, NULL, 0);
         }
      }
      else if (pTrInstance->trType == UNICAST_OR_FRG_REQUEST_TRANSPORT_TYPE)
      {
         /* report to session layer */
         pTrInstance->pSeInstance->trResCode = trResCode;
         pTrInstance->pSeInstance->ackReceived = 1;
      }
      
      /* Mark the sequence number as ready */
      if ((pTrInstance->pSendSeqCntStr->pActiveSeqNoList) &&
         (pTrInstance->sequenceNumber != 0 ))
      {
         pTrInstance->pSendSeqCntStr->pActiveSeqNoList[pTrInstance->seqCntInd] = 0;
      }
      
      if (pTrInstance->pSendSeqCntStr->seqNoSync == WAIT_FOR_SYNC)
      {
         if ((trResCode == MD_NO_ACK_RECEIVED) || (trResCode == MD_SEND_FAILED))
         {
            pTrInstance->pSendSeqCntStr->seqNoSync = NOT_SYNC; 
         }
         else
         {
            createSendSeqNoList(pTrInstance); 
         }
      }
      
      /* remove state from list */
      deleteTrInstance(pTrInstance);
   }
}

/*******************************************************************************
NAME:     multicastInstance
ABSTRACT: Sending a multicast message.
RETURNS:  -
*/
static void multicastInstance(
   TRANSPORT_INSTANCE *pTrInstance)  /* Pointer to transport instance */
{
   int res;
   
   
   switch (pTrInstance->state)
   {
      case TR_INIT:
        /* No sequence number needed for multicast as there is no resending */
        pTrInstance->sequenceNumber = 0;

         /* Prepare message sequence counter */
         *((UINT16*)(pTrInstance->pSendMsg + SEQ_NO_OFF)) = TOWIRE16(pTrInstance->sequenceNumber);/*lint !e826 Type cast OK */
   
         pTrInstance->state = TR_SEND;
         
         /* No break continue with send */
         
      case TR_SEND:
         /* Header frame check sequence */
         iptAddDataFCS((BYTE *)pTrInstance->pSendMsg,
             FROMWIRE16(*((UINT16*)(pTrInstance->pSendMsg + HEAD_LENGTH_OFF))));/*lint !e826 Type cast OK */
   
         /* Send message */
#ifdef TARGET_SIMU
         res = IPTDriveMDSocketSend(TOWIRE32(pTrInstance->comPar.destIpAddr),
                                    TOWIRE32(pTrInstance->comPar.simDevIpAddr),
                                    (BYTE *)pTrInstance->pSendMsg,
                                    pTrInstance->sendMsgLength,
                                    pTrInstance->comPar.mdSendSocket );
#else
         res = IPTDriveMDSocketSend(TOWIRE32(pTrInstance->comPar.destIpAddr),
                                    (BYTE *)pTrInstance->pSendMsg,
                                    pTrInstance->sendMsgLength,
                                    pTrInstance->comPar.mdSendSocket );
#endif
   
         if (res != (int)IPT_OK)
         {
            IPTVosPrint7(IPT_ERR,
               "Send error=%#x ComId=%d pCallerRef=%#x IPaddr=%d.%d.%d.%d\n",
               res,
               pTrInstance->comId,
               pTrInstance->pCallerRef,
               (pTrInstance->comPar.destIpAddr & 0xff000000) >> 24,
               (pTrInstance->comPar.destIpAddr & 0xff0000) >> 16,
               (pTrInstance->comPar.destIpAddr & 0xff00) >> 8,
               pTrInstance->comPar.destIpAddr & 0xff);
         }
   
         /* remove state from list */
         deleteTrInstance(pTrInstance);
         break;
         
      default:
         
         break;
   }
}

/*******************************************************************************
* GLOBAL FUNCTIONS */

/*******************************************************************************
NAME:     trInit
ABSTRACT: Initiation of the transport layer.
          Creating semaphores.
RETURNS:  0 if OK, !=0 if not.
*/
int trInit(void)
{
   int res;
   
   /* Create a semaphore for the transport layer list resource
   initial state = free */
   res =  IPTVosCreateSem(&IPTGLOBAL(md.trListSemId), IPT_SEM_FULL);
   if (res == (int)IPT_OK)
   {
      /* Create a semaphore for the send sequence list resource
      initial state = free */
      res =  IPTVosCreateSem(&IPTGLOBAL(md.SendSeqSemId), IPT_SEM_FULL);
      if (res == (int)IPT_OK)
      {
         IPTGLOBAL(md.trInitiated) = 1;
      }
      else
      {
         IPTVosPrint0(IPT_ERR,
                      "ERROR creating send sequence list resource semaphore\n");
      }    
   }
   else
   {
      IPTVosPrint0(IPT_ERR,
                   "ERROR creating tranport layer list resource semaphore\n");
   }    
   
   return(res);
}

/*******************************************************************************
NAME:     trTerminate
ABSTRACT: Termination of the transport layer.
RETURNS:  -
*/
void trTerminate(void)
{
   int res;
   SEND_SEQ_CNT **ppSendSeqCntStr;
   SEND_SEQ_CNT *pSendSeqCntStr;
   UINT16 *pActiveSeqNoList; /* Pointer to array of sequence numbers */

   /* Wait until all transport instances has been finished */
   while (IPTGLOBAL(md.pFirstTrInstance) != 0)
   {
      IPTVosTaskDelay(10);
   }
   
   res = IPTVosGetSem(&IPTGLOBAL(md.SendSeqSemId), IPT_WAIT_FOREVER);
   if (res == IPT_OK)
   {
      ppSendSeqCntStr = &IPTGLOBAL(md.pFirstSendSeqCnt);
      while ((*ppSendSeqCntStr) != 0)
      {
         /* Save address to sequence structure */
         pSendSeqCntStr = *ppSendSeqCntStr;
         pActiveSeqNoList = pSendSeqCntStr->pActiveSeqNoList;

         ppSendSeqCntStr = &((*ppSendSeqCntStr)->pNext);

         /* deallocate send sequence structure */
         res = IPTVosFree((BYTE *)pSendSeqCntStr);
         if(res != 0)
         {
            IPTVosPrint1(IPT_ERR, 
               "Failed to free memory, code=%#x\n",
               res);
         }
 
         if (pActiveSeqNoList)
         {
            /* deallocate send sequence list */
            res = IPTVosFree((BYTE *)pActiveSeqNoList);
            if(res != 0)
            {
               IPTVosPrint1(IPT_ERR, 
                  "Failed to free memory, code=%#x\n",
                  res);
            }
         }
      }

      IPTVosDestroySem(&IPTGLOBAL(md.SendSeqSemId));
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "trTerminate: IPTVosGetSem ERROR\n");
   }

   IPTGLOBAL(md.trInitiated) = 0;

   /* Destroy a semaphore for the transport layer list resource */
   IPTVosDestroySem(&IPTGLOBAL(md.trListSemId));
}

/*******************************************************************************
NAME:     createTrInstance
ABSTRACT: Create a new transport instance. Used for sending without session
          layer involved
RETURNS:  0 if OK, !=0 if not.
*/
int createTrInstance(
   int trType,                         /* Type of communication */
   const void *pCallerRef,                   /* Caller reference */
   MD_QUEUE callerQueueId,             /* Caller queue ID */
   IPT_REC_FUNCPTR callerFunc,         /* Pointer to callback function */
   UINT32 comId,                       /* ComID */
   UINT32 sendMsgLength,               /* Message length */
   char *pSendMsg,                     /* Pointer to message */
   MD_COM_PAR comPar,                     /* Communication parameters */
   TRANSPORT_INSTANCE **ppNewInstance) /* Pointer to pointer to instance */
{
   int res = (int)IPT_OK;

   /* check queue or function */
   if (trType == UNICAST_OR_FRG_REPORT_TRANSPORT_TYPE)
   {
      if ((callerQueueId == 0) && (callerFunc == 0))
      {
         IPTVosPrint0(IPT_WARN,
                      "ERROR no caller queue or callback function given\n");
         res = (int)IPT_INVALID_PAR;
      }
   }
   
   if (res == (int)IPT_OK)
   {
      *ppNewInstance = (TRANSPORT_INSTANCE *)IPTVosMalloc(sizeof(TRANSPORT_INSTANCE));
      
      if (*ppNewInstance != 0)
      {
         (*ppNewInstance)->pNext = 0;
         (*ppNewInstance)->trType = trType;
         (*ppNewInstance)->state = TR_INIT;
         (*ppNewInstance)->replierIpAddr = comPar.destIpAddr;
         (*ppNewInstance)->comId = comId;
         (*ppNewInstance)->callerQueueId = callerQueueId;
         (*ppNewInstance)->callerFunc = callerFunc;
         (*ppNewInstance)->comPar =  comPar;
         (*ppNewInstance)->sendMsgLength = sendMsgLength;
         (*ppNewInstance)->pSendMsg = pSendMsg;
         (*ppNewInstance)->sendCnt = 0;
         (*ppNewInstance)->pCallerRef = pCallerRef;
         
         if (trType == MULTICAST_TRANSPORT_TYPE)
         {
            /* No sequence counter will be used */
            (*ppNewInstance)->pSendSeqCntStr = 0;
         }
         else
         {
            res = getSendSequenceCounter(*ppNewInstance);
            if (res != (int)IPT_OK)
            {
               /* deallocate transport instance */
               res = IPTVosFree((BYTE *)*ppNewInstance);
               if(res != 0)
               {
                  IPTVosPrint1(IPT_ERR, 
                     "Failed to free session instance memory, code=%#x\n",
                     res);
               }
               *ppNewInstance = 0;
            }
         }
      }
      else
      {
         IPTVosPrint6(IPT_ERR,
            "createTrInstance: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d pCallerRef=%d\n",
            sizeof(TRANSPORT_INSTANCE),
            (comPar.destIpAddr & 0xff000000) >> 24,
            (comPar.destIpAddr & 0xff0000) >> 16,
            (comPar.destIpAddr & 0xff00) >> 8,
            comPar.destIpAddr & 0xff,
            pCallerRef);
         res = (int)IPT_MEM_ERROR;
      }
   }
   
   return(res);    
}

/*******************************************************************************
NAME:     createSeTrInstance
ABSTRACT: Create a new transport instance. Used for sending when a session is
          layer involved
RETURNS:  0 if OK, !=0 if not.
*/
int createSeTrInstance(
   int trType,                          /* Type of communication */
   const void *pCallerRef,                    /* Caller reference */
   SESSION_INSTANCE *pSeInstance,       /* Pointer to session instance */
   UINT32 comId,                        /* ComID */
   UINT32 sendMsgLength,                /* Message length */
   char *pSendMsg,                      /* Pointer to message */
   MD_COM_PAR comPar,                      /* Communication parameters */
   TRANSPORT_INSTANCE **ppNewInstance)  /* pointer to pointer to instance */
{                                  
   int res = (int)IPT_OK;
   
   if (pSeInstance == 0)
   {
      IPTVosPrint0(IPT_ERR, "No Session layer given\n");
      res = (int)IPT_ERROR;
   }
   else
   {
      *ppNewInstance = (TRANSPORT_INSTANCE *)IPTVosMalloc(sizeof(TRANSPORT_INSTANCE));
      
      if (*ppNewInstance != 0)
      {
         (*ppNewInstance)->pNext = 0;
         (*ppNewInstance)->trType = trType;
         (*ppNewInstance)->state = TR_INIT;
         (*ppNewInstance)->replierIpAddr = comPar.destIpAddr;
         (*ppNewInstance)->comId = comId;
         (*ppNewInstance)->pSeInstance = pSeInstance;
         (*ppNewInstance)->comPar =  comPar;
         (*ppNewInstance)->sendMsgLength = sendMsgLength;
         (*ppNewInstance)->pSendMsg = pSendMsg;
         (*ppNewInstance)->sendCnt = 0;
         (*ppNewInstance)->pCallerRef = pCallerRef;

     
         res = getSendSequenceCounter(*ppNewInstance);
         if (res != (int)IPT_OK)
         {
            /* deallocate transport instance */
            res = IPTVosFree((BYTE *)*ppNewInstance);
            if(res != 0)
            {
               IPTVosPrint1(IPT_ERR, 
                  "Failed to free session instance memory, code=%#x\n",
                  res);
            }
            *ppNewInstance = 0;
         }
      }
      else
      {
         res = (int)IPT_MEM_ERROR;

         IPTVosPrint6(IPT_ERR,
            "createSeTrInstance: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d pCallerRef=%d\n",
            sizeof(TRANSPORT_INSTANCE),
            (comPar.destIpAddr & 0xff000000) >> 24,
            (comPar.destIpAddr & 0xff0000) >> 16,
            (comPar.destIpAddr & 0xff00) >> 8,
            comPar.destIpAddr & 0xff,
            pCallerRef);
      }
   }
   
   return(res);    
}

/*******************************************************************************
NAME:     searchTrQueue
ABSTRACT: Check if a queue is used by a transport instance.
RETURNS:  1 = found
          0 not found
*/
int searchTrQueue(
   MD_QUEUE queue)   /* Queue id */
{
   int ret;
   TRANSPORT_INSTANCE *pTrInstance; /* Pointer to transport instance */
   int found = 0;
   if (IPTGLOBAL(md.trInitiated))
   {
      /* search for sending instance */
      ret = IPTVosGetSem(&IPTGLOBAL(md.trListSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pTrInstance = IPTGLOBAL(md.pFirstTrInstance);
   
         while ((pTrInstance) && (found == 0))
         {
            if (pTrInstance->callerQueueId == queue)
            {
               found = 1;
            }
            else
            {
               pTrInstance = pTrInstance->pNext;
            }
         }
         if(IPTVosPutSemR(&IPTGLOBAL(md.trListSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "searchTrQueue: IPTVosGetSem ERROR\n");
      }
   }
   return(found);
}

/*******************************************************************************
NAME:     removeTrQueue
ABSTRACT: Remove queueu used by a transport instance.
RETURNS:  -
*/
void removeTrQueue(
   MD_QUEUE queue)   /* Queue id */
{
   int ret;
   TRANSPORT_INSTANCE *pTrInstance; /* Pointer to transport instance */
   if (IPTGLOBAL(md.trInitiated))
   {
      /* search for sending instance */
      ret = IPTVosGetSem(&IPTGLOBAL(md.trListSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pTrInstance = IPTGLOBAL(md.pFirstTrInstance);
   
         while (pTrInstance)
         {
            if (pTrInstance->callerQueueId == queue)
            {
               pTrInstance->callerQueueId = 0;
            }
            else
            {
               pTrInstance = pTrInstance->pNext;
            }
         }
         if(IPTVosPutSemR(&IPTGLOBAL(md.trListSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "removeTrQueue: IPTVosGetSem ERROR\n");
      }
   }
}

/*******************************************************************************
NAME:     insertTrInstance
ABSTRACT: Insert a new transport instance into the linked list of active
          transport layer instances.
RETURNS:  -
*/
void insertTrInstance(
   TRANSPORT_INSTANCE *pTrInstance) /* Pointer to transport instance */
{
   int ret;
   /* Insert instance in the list */
   ret = IPTVosGetSem(&IPTGLOBAL(md.trListSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      if (IPTGLOBAL(md.pLastTrInstance) != 0)
      {
         IPTGLOBAL(md.pLastTrInstance)->pNext = pTrInstance;
      }
      IPTGLOBAL(md.pLastTrInstance) = pTrInstance;
   
      if (IPTGLOBAL(md.pFirstTrInstance) == 0)
      {
         IPTGLOBAL(md.pFirstTrInstance) = pTrInstance;
      }
      if(IPTVosPutSemR(&IPTGLOBAL(md.trListSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "insertTrInstance: IPTVosGetSem ERROR\n");
   }
}


/*******************************************************************************
NAME:     trSendTask
ABSTRACT: Handling active transport layer instances.
          This function shall be call by the cyclic sending task.
RETURNS:  -
*/
void trSendTask( void )
{
   UINT8 inaugState;
   UINT8 currentTopoCnt;
   int ret;    
   TRANSPORT_INSTANCE *pTrInstance = IPTGLOBAL(md.pFirstTrInstance);
   TRANSPORT_INSTANCE *pNextTrInstance;
   
   while (pTrInstance)
   {
      pNextTrInstance = pTrInstance->pNext;
      switch (pTrInstance->trType)
      {
      case UNICAST_OR_FRG_NO_REPORT_TRANSPORT_TYPE:
      case UNICAST_OR_FRG_REPORT_TRANSPORT_TYPE:
      case UNICAST_OR_FRG_REQUEST_TRANSPORT_TYPE:
         unicastInstance(pTrInstance);
         break;
         
      case MULTICAST_TRANSPORT_TYPE:
         multicastInstance(pTrInstance);
         break;
         
      default:
         break;
      }
      pTrInstance = pNextTrInstance;
   }  
 
   if (!IPTGLOBAL(tdcsim.enableTDCSimulation))
   {
      currentTopoCnt = 0;
      if (tdcGetIptState (&inaugState, &currentTopoCnt) == TDC_OK)
      {
         if (inaugState == TDC_IPT_INAUGSTATE_OK)
         {
            /* Topo counter changed? */
            if ((currentTopoCnt != 0) && 
                (currentTopoCnt != IPTGLOBAL(md.lastSendTopoCnt)))
            {
               /* Clean up send sequence counter lists */
               ret = clearSendSeqCntLists(currentTopoCnt);
               if (ret == IPT_OK)
               {
                  IPTGLOBAL(md.lastSendTopoCnt) = currentTopoCnt;
               }
            }
         }
      }
   }

}



/*******************************************************************************
NAME:     sendAck
ABSTRACT: Prepare and send an acknowledge message.
RETURNS:  0 if OK, !=0 if not.
*/
int sendAck(
   UINT32 destIpAddr,   /* Destination IP address */
#ifdef TARGET_SIMU      
   UINT32 simDevIPAddr,    /* Source IP address (simulated)*/
#endif
   UINT16 ackCode,      /* Acknowledge code to be send */
   char *pInBuf)        /* Received message buffer */
{
   int i;
   int res;
   int res2;
   UINT16 headerLength;
   UINT16 dataLength;
   UINT8 srcURILength;
   UINT8 destURILength;
   UINT32 *pIn;
   UINT32 *pOut;
   UINT32 topoCnt;
   char *pSendMsg;
   char *pAllocMsg;
   char ackMsg[MIN_MD_HEADER_SIZE + 2*(((MD_URILEN +1 +3)/4) * 4) +ACK_MSG_SIZE + 2*FCS_SIZE];
   
   /* Switch destination and source URI length from received message */
   destURILength = *((UINT8*)(pInBuf + SRC_URI_LENGTH_OFF));
   srcURILength = *((UINT8*)(pInBuf + DEST_URI_LENGTH_OFF));
   
   /* Use header length from the received message */
   headerLength = FROMWIRE16(*((UINT16*)(pInBuf + HEAD_LENGTH_OFF)));/*lint !e826  Ignore casting warning */
   
   /* Set data length in 8 bits word for the ACK message */
   dataLength = ACK_MSG_SIZE;
   
   if ((headerLength + dataLength + 2*FCS_SIZE) > sizeof(ackMsg))
   {
      pAllocMsg = (char *)IPTVosMalloc(headerLength + dataLength + 2*FCS_SIZE);
      pSendMsg = pAllocMsg;
      if (pSendMsg == 0)
      {
         IPTVosPrint5(IPT_ERR,
            "sendAck: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d \n",
            headerLength + dataLength + 2*FCS_SIZE,
            (destIpAddr & 0xff000000) >> 24,
            (destIpAddr & 0xff0000) >> 16,
            (destIpAddr & 0xff00) >> 8,
            destIpAddr & 0xff);
         return((int)IPT_ERROR);
      }
   }
   else
   {
      pAllocMsg = 0;
      pSendMsg = ackMsg;
   }
   
   /* time stamp */
   *((UINT32*)(pSendMsg + TIMESTAMP_OFF)) = TOWIRE32(IPTVosGetMicroSecTimer());
   
   /* protocol version */
   *((UINT32*)(pSendMsg + PROT_VER_OFF)) = TOWIRE32(MD_PROTOCOL_VERSION);
   
   /* Topo counter */
#ifdef TARGET_SIMU
   res = iptGetTopoCnt(destIpAddr, simDevIPAddr, &topoCnt);
#else
   res = iptGetTopoCnt(destIpAddr, &topoCnt);
#endif
   if (res == (int)IPT_OK)
   {
      *((UINT32*)(pSendMsg + TOPO_COUNT_OFF)) = TOWIRE32(topoCnt);
   }
   else
   {
      if (pAllocMsg)
      {
         res2 = IPTVosFree((BYTE *)pAllocMsg);
         if(res2 != 0)
         {
            IPTVosPrint1(IPT_ERR, "sendAck failed to free message buffer, code=%#x\n",res2);
         }
      }

      return((int)IPT_ERROR);
   }
  
   /* Use comId from received message (No marshalling needed, same byte order
   as in received message) */
   *((UINT32*)(pSendMsg + COMID_OFF)) = *((UINT32*)(pInBuf + COMID_OFF));/*lint !e826  Ignore casting warning */
   
   /* Message type */
   *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(ACK_MSG);
   
   /* Dataset Length in 8 bit words */
   *((UINT16*)(pSendMsg + DATA_LENGTH_OFF)) = TOWIRE16(dataLength);
   
   /* User status bytes */
   *((UINT16*)(pSendMsg + USER_STATUS_OFF)) = 0;
   
   /* Header length in 8 bit words */
   *((UINT16*)(pSendMsg + HEAD_LENGTH_OFF)) = TOWIRE16(headerLength);
   
   /* Source URI length */
   *((UINT8*)(pSendMsg + SRC_URI_LENGTH_OFF)) = srcURILength;
   
   /* Destination URI length */
   *((UINT8*)(pSendMsg + DEST_URI_LENGTH_OFF)) = destURILength;
   
   /* Index */
   *((INT16*)(pSendMsg + INDEX_OFF)) = TOWIRE16(-1);
   
   /* Sequence number set to maximum stored sequence number for the receiving
      end device */
   *((UINT16*)(pSendMsg + SEQ_NO_OFF)) = TOWIRE16(IPTGLOBAL(md.maxStoredSeqNo));
   
   /* Total length of a unsplit message (Dataset) in blocks of 1024 bytes */
   *((UINT16*)(pSendMsg + MSG_LENGTH_OFF)) = TOWIRE16(1);
   
   /* Session ID */
   *((UINT32*)(pSendMsg + SESSION_ID_OFF)) = 0;
   
   /* Use received destination URI as source URI string */
   pOut = (UINT32*)(pSendMsg + SRC_URI_OFF);
   pIn =  (UINT32*)(pInBuf + SRC_URI_OFF + 4 * destURILength);/*lint !e826  Ignore casting warning */
   for (i=0; i<srcURILength; i++)
   {
      *pOut++ = *pIn++;
   }
   
   /* Use received source URI as destination URI string */
   pIn =  (UINT32*)(pInBuf + SRC_URI_OFF);/*lint !e826  Ignore casting warning */
   for (i=0; i<destURILength; i++)
   {
      *pOut++ = *pIn++;
   }
   
   /* Response timeout */
   *((UINT32 *)(pOut++))  = 0;

   /* Destination IP address */
   *((UINT32 *)(pOut))  = TOWIRE32(destIpAddr);

   /* Header checksum  */
   iptAddDataFCS((BYTE *)pSendMsg, headerLength);
   
   /* Acknowledge code */
   *((UINT16 *)(&pSendMsg[headerLength + FCS_SIZE])) = TOWIRE16(ackCode);
   
   /* Use received message sequence number as acknowledge sequence (No 
   marshalling needed, same byte order as in received message) */
   *((UINT16 *)(&pSendMsg[headerLength + FCS_SIZE + 2])) = *((UINT16*)(pInBuf + SEQ_NO_OFF));/*lint !e826  Ignore casting warning */
   
   /* Data checksum */
   iptAddDataFCS((BYTE *)&pSendMsg[headerLength + FCS_SIZE], 4);
   
   /* Send acknowledge message */
#ifdef TARGET_SIMU      
   res = IPTDriveMDSend(TOWIRE32(destIpAddr), TOWIRE32(simDevIPAddr), pSendMsg, headerLength + dataLength + 2*FCS_SIZE); 
#else
   res = IPTDriveMDSend(TOWIRE32(destIpAddr), (BYTE *)pSendMsg, headerLength + dataLength + 2*FCS_SIZE); 
#endif
   
   if (pAllocMsg)
   {
      res2 = IPTVosFree((BYTE *)pAllocMsg);
      if(res2 != 0)
      {
         IPTVosPrint1(IPT_ERR, "sendAck failed to free message buffer, code=%#x\n",res2);
      }
   }
   
   return(res);
}

/*******************************************************************************
NAME:     sendFrgAck
ABSTRACT: Prepare and send an FRG acknowledge message.
RETURNS:  0 if OK, !=0 if not.
*/
int sendFrgAck(
   UINT32 destIpAddr,    /* Destination IP address */
#ifdef TARGET_SIMU      
   UINT32 simDevIPAddr,   /* Source IP address (simulated)*/
#endif
   UINT32 frgDestIpAddr, /* Destination IP address used for the received 
                            FRG message */
   UINT16 ackCode,       /* Acknowledge code to be send */
   char *pInBuf)         /* Received message buffer */
{
   int i;
   int res;
   int res2;
   UINT16 headerLength;
   UINT16 dataLength;
   UINT8 srcURILength;
   UINT8 destURILength;
   UINT32 *pIn;
   UINT32 *pOut;
   UINT32 topoCnt;
   char *pSendMsg;
   char *pAllocMsg;
   char ackMsg[MIN_MD_HEADER_SIZE + 2*(((MD_URILEN +1 +3)/4) * 4) +FRG_ACK_MSG_SIZE + 2*FCS_SIZE];
   
   /* Switch destination and source URI length from received message */
   destURILength = *((UINT8*)(pInBuf + SRC_URI_LENGTH_OFF));
   srcURILength = *((UINT8*)(pInBuf + DEST_URI_LENGTH_OFF));
   
   /* Use header length from the received message */
   headerLength = FROMWIRE16(*((UINT16*)(pInBuf + HEAD_LENGTH_OFF)));/*lint !e826  Ignore casting warning */
   
   /* Set data length in 8 bits word for the ACK message */
   dataLength = FRG_ACK_MSG_SIZE;
   
   if ((headerLength + dataLength + 2*FCS_SIZE) > sizeof(ackMsg))
   {
      pAllocMsg = (char *)IPTVosMalloc(headerLength + dataLength + 2*FCS_SIZE);
      pSendMsg = pAllocMsg;
      if (pSendMsg == 0)
      {
         IPTVosPrint5(IPT_ERR,
            "sendAck: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d \n",
            headerLength + dataLength + 2*FCS_SIZE,
            (destIpAddr & 0xff000000) >> 24,
            (destIpAddr & 0xff0000) >> 16,
            (destIpAddr & 0xff00) >> 8,
            destIpAddr & 0xff);
         return((int)IPT_ERROR);
      }
   }
   else
   {
      pAllocMsg = 0;
      pSendMsg = ackMsg;
   }
   
   /* time stamp */
   *((UINT32*)(pSendMsg + TIMESTAMP_OFF)) = TOWIRE32(IPTVosGetMicroSecTimer());
   
   /* protocol version */
   *((UINT32*)(pSendMsg + PROT_VER_OFF)) = TOWIRE32(MD_PROTOCOL_VERSION);
   
   /* Topo counter */
#ifdef TARGET_SIMU
   res = iptGetTopoCnt(destIpAddr, simDevIPAddr, &topoCnt);
#else
   res = iptGetTopoCnt(destIpAddr, &topoCnt);
#endif
   if (res == (int)IPT_OK)
   {
      *((UINT32*)(pSendMsg + TOPO_COUNT_OFF)) = TOWIRE32(topoCnt);
   }
   else
   {
      if (pAllocMsg)
      {
         res2 = IPTVosFree((BYTE *)pAllocMsg);
         if(res2 != 0)
         {
            IPTVosPrint1(IPT_ERR, "sendAck failed to free message buffer, code=%#x\n",res2);
         }
      }

      return((int)IPT_ERROR);
   }

   /* Use comId from received message (No marshalling needed, same byte order
   as in received message) */
   *((UINT32*)(pSendMsg + COMID_OFF)) = *((UINT32*)(pInBuf + COMID_OFF));/*lint !e826  Ignore casting warning */
   
   /* Message type */
   *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(FRG_ACK_MSG);
   
   /* Dataset Length in 8 bit words */
   *((UINT16*)(pSendMsg + DATA_LENGTH_OFF)) = TOWIRE16(dataLength);
   
   /* User status bytes */
   *((UINT16*)(pSendMsg + USER_STATUS_OFF)) = 0;
   
   /* Header length in 8 bit words */
   *((UINT16*)(pSendMsg + HEAD_LENGTH_OFF)) = TOWIRE16(headerLength);
   
   /* Source URI length */
   *((UINT8*)(pSendMsg + SRC_URI_LENGTH_OFF)) = srcURILength;
   
   /* Destination URI length */
   *((UINT8*)(pSendMsg + DEST_URI_LENGTH_OFF)) = destURILength;
   
   /* Index */
   *((INT16*)(pSendMsg + INDEX_OFF)) = TOWIRE16(-1);
   
   /* Sequence number set to maximum stored sequence number for the receiving
      end device */
   *((UINT16*)(pSendMsg + SEQ_NO_OFF)) = TOWIRE16(IPTGLOBAL(md.maxStoredSeqNo));
   
   /* Total length of a unsplit message (Dataset) in blocks of 1024 bytes */
   *((UINT16*)(pSendMsg + MSG_LENGTH_OFF)) = TOWIRE16(1);
   
   /* Session ID */
   *((UINT32*)(pSendMsg + SESSION_ID_OFF)) = 0;
   
   /* Use received destination URI as source URI string */
   pOut = (UINT32*)(pSendMsg + SRC_URI_OFF);
   pIn =  (UINT32*)(pInBuf + SRC_URI_OFF + 4 * destURILength);/*lint !e826  Ignore casting warning */
   for (i=0; i<srcURILength; i++)
   {
      *pOut++ = *pIn++;
   }
   
   /* Use received source URI as destination URI string */
   pIn =  (UINT32*)(pInBuf + SRC_URI_OFF);/*lint !e826  Ignore casting warning */
   for (i=0; i<destURILength; i++)
   {
      *pOut++ = *pIn++;
   }
   
   /* Response timeout */
   *((UINT32 *)(pOut++))  = 0;

   /* Destination IP address */
   *((UINT32 *)(pOut))  = TOWIRE32(destIpAddr);

   /* Header checksum  */
   iptAddDataFCS((BYTE *)pSendMsg, headerLength);
   
   /* Acknowledge code */
   *((UINT16 *)(&pSendMsg[headerLength + FCS_SIZE])) = TOWIRE16(ackCode);
   
   /* Use received message sequence number as acknowledge sequence (No 
   marshalling needed, same byte order as in received message) */
   *((UINT16 *)(&pSendMsg[headerLength + FCS_SIZE + 2])) = *((UINT16*)(pInBuf + SEQ_NO_OFF));/*lint !e826  Ignore casting warning */
   
   /* Destination IP address, i.e. FRG multicast address*/
   *((UINT32 *)(&pSendMsg[headerLength + FCS_SIZE+ 4])) = TOWIRE32(frgDestIpAddr);

   /* Data checksum */
   iptAddDataFCS((BYTE *)&pSendMsg[headerLength + FCS_SIZE], FRG_ACK_MSG_SIZE);
   
   /* Send acknowledge message */
#ifdef TARGET_SIMU      
   res = IPTDriveMDSend(TOWIRE32(destIpAddr), TOWIRE32(simDevIPAddr), pSendMsg, headerLength + dataLength + 2*FCS_SIZE); 
#else
   res = IPTDriveMDSend(TOWIRE32(destIpAddr), (BYTE *)pSendMsg, headerLength + dataLength + 2*FCS_SIZE); 
#endif
   
   if (pAllocMsg)
   {
      res2 = IPTVosFree((BYTE *)pAllocMsg);
      if(res2 != 0)
      {
         IPTVosPrint1(IPT_ERR,
                      "sendAck failed to free message buffer, code=%#x\n",res2);
      }
   }
   
   return(res);
}

/*******************************************************************************
NAME:     trReceive
ABSTRACT: Take care of received messages.
RETURNS:  -
*/
void trReceive(
   UINT32 srcIpAddr,    /* Source IP address */
#ifdef TARGET_SIMU
   UINT32 simDevIpAddr, /* Destination IP address */
#endif
   char *pInBuf,        /* Receive buffer */
   UINT32 inBufBufLen)  /* No of received bytes */
{
   char *pData;
   char *pPureData = 0;
   char *pName;
   char uriInstName[IPT_MAX_LABEL_LEN];
   char uriFuncName[IPT_MAX_LABEL_LEN];
#ifdef TARGET_SIMU
   char simDevURI[IPT_MAX_LABEL_LEN];
#endif
   int res;
   int found;
   int alreadyReceived;
   int listenerQueuFull = 0;
   int listenerFound;
   int callBackListerFound = 0;
   int uriType;
   BYTE *pTemp;
   UINT8 srcURILength;
   UINT8 destURILength;
   UINT16 ackSequenceNumber;
   UINT16 ackCode;
   UINT16 ackCode2;
   UINT16 headerLength;
   UINT16 dataLength;
   UINT16 sequenceNumber = 0;
   UINT16 msgType;
   UINT32 destIpAddr = 0;
   UINT32 ackDestIpAddr;
   UINT32 dataPartLen;
   UINT32 pureDataLength = 0;
   UINT32 datasetId;
   UINT32 tempBufSize;
   UINT32 comId;
   UINT32 topoCount;
   UINT32 protocolVersion;
   static UINT32 lastRecMsgNo=0;
   TRANSPORT_INSTANCE *pTrInstance;
   SESSION_INSTANCE *pSeInstance;
   SEQ_NO_LIST_ITEM *pSeqNoListItem = NULL; 
   QUEUE_MSG applMsg;
   MSG_INFO msgInfo;

   memset(&applMsg, 0, sizeof applMsg);
   
   IPTGLOBAL(md.mdCnt.mdInPackets)++;
   
   /* Change source IP address to bigendian */
   srcIpAddr = FROMWIRE32(srcIpAddr);

#ifdef TARGET_SIMU
   simDevIpAddr = FROMWIRE32(simDevIpAddr);
#endif
   /* Get header length */
   headerLength = FROMWIRE16(*(UINT16*)(pInBuf+HEAD_LENGTH_OFF)); /*lint !e826  Ignore casting warning */

   /* check header CRC */
   res = iptCheckFCS((BYTE *)pInBuf, headerLength + FCS_SIZE);
   if (res != (int)IPT_OK)
   {
      IPTGLOBAL(md.mdCnt.mdInFCSErrors)++;
      
      IPTVosPrint0(IPT_WARN, "Message received with wrong header checksum\n");
      return;
   }
   
   /* Check protocol version */
   protocolVersion = FROMWIRE32(*(UINT32*)(pInBuf+PROT_VER_OFF));/*lint !e826  Ignore casting warning */
   if ((protocolVersion & 0xff000000) != (MD_PROTOCOL_VERSION & 0xff000000))
   {
      IPTGLOBAL(md.mdCnt.mdInProtocolErrors)++;
      
      IPTVosPrint1(IPT_WARN,
                   "Message received with wrong protocol version=%d\n",
                   FROMWIRE32(*(UINT32*)(pInBuf+PROT_VER_OFF)));/*lint !e826  Ignore casting warning */
      return;
   }
 
   topoCount = FROMWIRE32(*(UINT32*)(pInBuf+TOPO_COUNT_OFF));/*lint !e826  Ignore casting warning */
   if (topoCount != 0)
   {
#ifdef TARGET_SIMU
      if (iptCheckRecTopoCnt(topoCount, simDevIpAddr) != (int)IPT_OK)
#else
      if (iptCheckRecTopoCnt(topoCount) != (int)IPT_OK)
#endif
      {
         IPTGLOBAL(md.mdCnt.mdInTopoErrors)++;
        
         IPTVosPrint1(IPT_WARN,
                      "Message received with wrong topo counter=%d\n",
                      topoCount);
         return;
      }
   }
   
   /* Calculate length and start of the data part */
   pData = pInBuf + headerLength + FCS_SIZE; 
   dataPartLen = inBufBufLen - headerLength - FCS_SIZE;
   
   /* Get data length */
   dataLength = FROMWIRE16(*(UINT16*)(pInBuf+DATA_LENGTH_OFF));/*lint !e826  Ignore casting warning */
   
   /* Get message type */
   msgType = FROMWIRE16(*(UINT16*)(pInBuf+TYPE_OFF));/*lint !e826  Ignore casting warning */
  
   /* Acknowledge message? */
   if (msgType == ACK_MSG)
   {
      /* check data frame checksum */
      res = iptCheckFCS((BYTE *)pData, dataPartLen);
      if (res != (int)IPT_OK)
      {
         IPTGLOBAL(md.mdCnt.mdInFCSErrors)++;
         
         IPTVosPrint0(IPT_WARN,
                      "ACK Message received with wrong data frame checksum\n");
         return;
      }
      
      if (dataLength != ACK_MSG_SIZE)
      {
         IPTVosPrint0(IPT_WARN, "ACK Message received with wrong data length\n");
         return;
      }

      /* get acknowledged sequence number */
      ackSequenceNumber = FROMWIRE16(*(UINT16*)(pData + ACK_SEQ_NO_OFF));/*lint !e826  Ignore casting warning */
      
      found = 0;
      if (IPTGLOBAL(md.trInitiated))
      {
         /* search for sending instance */
         res = IPTVosGetSem(&IPTGLOBAL(md.trListSemId), IPT_WAIT_FOREVER);
         if (res == IPT_OK)
         {
            pTrInstance = IPTGLOBAL(md.pFirstTrInstance);
      
            while ((pTrInstance) && (found == 0))
            {
               if ((pTrInstance->sequenceNumber == ackSequenceNumber) &&
                   (pTrInstance->pSendSeqCntStr) &&
                   (pTrInstance->pSendSeqCntStr->destIpAddr == srcIpAddr))
               {
                  pTrInstance->ackCode = FROMWIRE16(*(UINT16*)(pData + ACK_CODE_OFF));/*lint !e826  Ignore casting warning */
                  if (pTrInstance->pSendSeqCntStr->seqNoSync == WAIT_FOR_SYNC)
                  {
                     pTrInstance->pSendSeqCntStr->maxActiveSeqNo = 
                                  FROMWIRE16(*(UINT16*)(pInBuf + SEQ_NO_OFF));/*lint !e826  Ignore casting warning */ 
                  }
                  pTrInstance->ackReceived = 1;
            
                  found = 1;
               }
               else
               {
                  pTrInstance = pTrInstance->pNext;
               }
            }
            if(IPTVosPutSemR(&IPTGLOBAL(md.trListSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "trReceive: IPTVosGetSem ERROR\n");
         }
 
         if (found == 0)
         {
            IPTVosPrint4(IPT_WARN,
            "Ack Message received from IP=%d.%d.%d.%d without any corresponding transport instance\n",
                         srcIpAddr/(UINT32)(0x1000000),
                         (srcIpAddr%(UINT32)0x1000000)/(UINT32)0x10000,
                         (srcIpAddr%(UINT32)0x10000)/(UINT32)0x100,
                         srcIpAddr%(UINT32)0x100);
         }
      }
   }
   /* Acknowledge message for a FRG message? */
   else if (msgType == FRG_ACK_MSG)
   {
      /* check data frame checksum */
      res = iptCheckFCS((BYTE *)pData, dataPartLen);
      if (res != (int)IPT_OK)
      {
         IPTGLOBAL(md.mdCnt.mdInFCSErrors)++;

         IPTVosPrint0(IPT_WARN,
                   "FRG ACK Message received with wrong data frame checksum\n");
         return;
      }
      
      if (dataLength != FRG_ACK_MSG_SIZE)
      {
         IPTVosPrint0(IPT_WARN,
                      "FRG ACK Message received with wrong data length\n");
         return;
      }

      /* get acknowledged sequence number */
      ackSequenceNumber = FROMWIRE16(*(UINT16*)(pData + ACK_SEQ_NO_OFF));/*lint !e826  Ignore casting warning */
      
      /* get acknowledged destination IP address */
      ackDestIpAddr = FROMWIRE32(*(UINT32*)(pData + DEST_IP_OFF));/*lint !e826  Ignore casting warning */
      
      found = 0;
      if (IPTGLOBAL(md.trInitiated))
      {
         /* search for sending instance */
         res = IPTVosGetSem(&IPTGLOBAL(md.trListSemId), IPT_WAIT_FOREVER);
         if (res == IPT_OK)
         {
            pTrInstance = IPTGLOBAL(md.pFirstTrInstance);

            while ((pTrInstance) && (found == 0))
            {
               if ((pTrInstance->sequenceNumber == ackSequenceNumber) &&
                   (pTrInstance->pSendSeqCntStr) &&
                   (pTrInstance->pSendSeqCntStr->destIpAddr == ackDestIpAddr))
               {
                  pTrInstance->ackCode = FROMWIRE16(*(UINT16*)(pData + ACK_CODE_OFF));/*lint !e826  Ignore casting warning */
                  if (pTrInstance->pSendSeqCntStr->seqNoSync == WAIT_FOR_SYNC)
                  {
                     pTrInstance->pSendSeqCntStr->maxActiveSeqNo = 
                                  FROMWIRE16(*(UINT16*)(pInBuf + SEQ_NO_OFF)); /*lint !e826  Ignore casting warning */
                  }
                  pTrInstance->ackReceived = 1;
            
                  found = 1;
               }
               else
               {
                  pTrInstance = pTrInstance->pNext;
               }
            }
            if(IPTVosPutSemR(&IPTGLOBAL(md.trListSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "trReceive: IPTVosGetSem ERROR\n");
         }
      
 
         if (found == 0)
         {
            IPTVosPrint4(IPT_WARN,
            "FRG Ack Message received from IP=%d.%d.%d.%d without any corresponding transport instance\n",
                         srcIpAddr/(UINT32)(0x1000000),
                         (srcIpAddr%(UINT32)0x1000000)/(UINT32)0x10000,
                         (srcIpAddr%(UINT32)0x10000)/(UINT32)0x100,
                         srcIpAddr%(UINT32)0x100);
         }
      }
   }
   /* Message data message? */
   else if ((msgType == MD_ACK_MSG)     || (msgType == MD_NOACK_MSG) ||
            (msgType == MD_REQ_ACK_MSG) || (msgType == MD_REQ_NOACK_MSG) ||
            (msgType == MD_FRG_MSG)     || (msgType == MD_REQ_FRG_MSG))
   {
      lastRecMsgNo++;
      ackCode = ACK_OK;   /* temporary setting */
      comId = FROMWIRE32(*(UINT32*)(pInBuf+COMID_OFF));/*lint !e826  Ignore casting warning */
      srcURILength = *((UINT8*)(pInBuf + SRC_URI_LENGTH_OFF));
      destURILength = *((UINT8*)(pInBuf + DEST_URI_LENGTH_OFF));

      if ((msgType == MD_ACK_MSG) || (msgType == MD_REQ_ACK_MSG))
      {
         /* Check sequence number, i.e. that the message not already has been received */
         sequenceNumber = FROMWIRE16(*(UINT16*)(pInBuf+SEQ_NO_OFF));/*lint !e826  Ignore casting warning */
#ifdef TARGET_SIMU
         alreadyReceived = checkRecSequenceCounter(simDevIpAddr, srcIpAddr, sequenceNumber, &pSeqNoListItem);
#else
         alreadyReceived = checkRecSequenceCounter(srcIpAddr, sequenceNumber, &pSeqNoListItem);
#endif
      }
      else if ((msgType == MD_FRG_MSG) || (msgType == MD_REQ_FRG_MSG))
      {
         /* Check sequence number, i.e. that the message not already has been received */
         sequenceNumber = FROMWIRE16(*(UINT16*)(pInBuf+SEQ_NO_OFF));/*lint !e826  Ignore casting warning */
         destIpAddr = FROMWIRE32(*(UINT32*)(pInBuf + SRC_URI_OFF + 4 * srcURILength + 4 * destURILength + 4));/*lint !e826  Ignore casting warning */
#ifdef TARGET_SIMU
         alreadyReceived = checkRecFrgSequenceCounter(simDevIpAddr, srcIpAddr, destIpAddr, sequenceNumber, &pSeqNoListItem);
#else
         alreadyReceived = checkRecFrgSequenceCounter(srcIpAddr, destIpAddr, sequenceNumber, &pSeqNoListItem);
#endif
      }
      else
      {
        /* No resending for message that shall not be acknowledged 
           e.g. multicast */
        alreadyReceived = 0; 
      }

      if (alreadyReceived == 0)
      {
         (void)extractUri((char *)(pInBuf + SRC_URI_OFF + 4 * srcURILength),
                              uriInstName,
                              uriFuncName,
                              &uriType);
        
         /* search for listener */
         listenerFound = mdAnyListener(
#ifdef TARGET_SIMU
                                             simDevIpAddr,
#endif
                                             comId,  
                                             uriType,
                                             uriInstName,
                                             uriFuncName);

         if ((!listenerFound) && (comId != ECHO_COMID))
         {
            IPTGLOBAL(md.mdCnt.mdInNoListeners)++;

            IPTVosPrint6(IPT_WARN,
               "No listener for received message from IP=%d.%d.%d.%d ComId=%d Dest URI=%s\n",
               srcIpAddr/(UINT32)(0x1000000),
               (srcIpAddr%(UINT32)0x1000000)/(UINT32)0x10000,
               (srcIpAddr%(UINT32)0x10000)/(UINT32)0x100,
               srcIpAddr%(UINT32)0x100,
               FROMWIRE32(*(UINT32*)(pInBuf+COMID_OFF)),
               (char *)(pInBuf + SRC_URI_OFF + 4 * srcURILength));/*lint !e826  Ignore casting warning */
            ackCode = ACK_DEST_UKNOWN;
         }
     
         if (ackCode == ACK_OK)
         {
            /* Get dataset */
            res = iptConfigGetDatasetId(comId, &datasetId);
            
            if ((res == (int)IPT_OK) && (datasetId != 0))
            {
               
               tempBufSize = ((dataLength+3) / 4 ) * 4;

               pTemp = IPTVosMalloc(tempBufSize);
               if (pTemp != 0)
               {
                  /* Check data frame checksum */
                  res = iptLoadReceiveDataFCS((BYTE *)pData,
                     dataPartLen,
                     pTemp,
                     &tempBufSize);  
                  if (res == 0)
                  {
                     /* Calculation of unpacked data size */
                     {                        
                        if (IPTGLOBAL(enableFrameSizeCheck) != FALSE)
                           pureDataLength = tempBufSize;
                     }
                     
                     /* CR 7779, Check for ill-formated frames */
                     res = iptCalcDatasetSize(datasetId,
                                              pTemp,
                                              &pureDataLength);
                     if (res == IPT_OK)
                     {
                        pPureData = (char *)IPTVosMalloc(pureDataLength);

                        if (pPureData != 0)
                        {
                           /* Unmarshall */
                           /* CR 7779, Check for proper size of received frame */
                           
                           res = iptUnmarshallDS(datasetId, pTemp, (BYTE *)pPureData,
                                               &pureDataLength);
                           if (res != (int)IPT_OK)
                           {
                              IPTVosPrint1(IPT_WARN,
                                    "Unmarshall of datasetId=%d failed\n",datasetId);
                              ackCode = ACK_WRONG_DATA;
                           }
                        }
                        else
                        {
                           IPTVosPrint6(IPT_ERR,
                           "trReceive: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d sequenceNumber=%d\n",
                                        pureDataLength,
                                        (srcIpAddr & 0xff000000) >> 24,
                                        (srcIpAddr & 0xff0000) >> 16,
                                        (srcIpAddr & 0xff00) >> 8,
                                        srcIpAddr & 0xff,
                                        sequenceNumber);
                           ackCode = ACK_BUFFER_NOT_AVAILABLE;
                        }
                     }
                     else
                     {
                        IPTVosPrint3(IPT_ERR,
                              "Size of datasetId=%d of received ComId=%d could not be calculated res=%#x\n",
                                     datasetId, comId, res);
                        ackCode = ACK_WRONG_DATA;
                     }
                  }
                  else
                  {
                     IPTGLOBAL(md.mdCnt.mdInFCSErrors)++;
                     
                     IPTVosPrint0(IPT_WARN,
                           "Message received with wrong data frame checksum\n");
                     ackCode = ACK_WRONG_FCS;

                  }

                  res = IPTVosFree(pTemp);
                  if(res != 0)
                  {
                     IPTVosPrint1(IPT_ERR,
                                  "trReceive failed to free memory, code=%#x\n",
                                  res);
                  }
               }
               else
               {
                  IPTVosPrint6(IPT_ERR,
                              "trReceive: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d sequenceNumber=%d\n",
                               tempBufSize,
                               (srcIpAddr & 0xff000000) >> 24,
                               (srcIpAddr & 0xff0000) >> 16,
                               (srcIpAddr & 0xff00) >> 8,
                               srcIpAddr & 0xff,
                               sequenceNumber);
                 
                  ackCode = ACK_BUFFER_NOT_AVAILABLE;
               }
            }
            else if (res != (int)IPT_TDC_NOT_READY) /* Unspecified dataset */
            {
               pureDataLength = ((dataLength+3) / 4 ) * 4;
               pPureData = (char *)IPTVosMalloc(pureDataLength);
               if (pPureData != 0)
               {
                  /* Check data frame checksum */
                  res = iptLoadReceiveDataFCS((BYTE *)pData,
                     dataPartLen,
                     (BYTE *)pPureData,
                     &pureDataLength);  
                  if (res == 0) 
                  {
                     /* Remove padding bytes from the data */
                     pureDataLength = dataLength;   
                  }
                  else
                  {
                     IPTGLOBAL(md.mdCnt.mdInFCSErrors)++;
                     IPTVosPrint0(IPT_WARN,
                           "Message received with wrong data frame checksum\n");
                     ackCode = ACK_WRONG_FCS;
                  }
               }
               else
               {
                  IPTVosPrint6(IPT_ERR,
                              "trReceive: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d sequenceNumber=%d\n",
                               pureDataLength,
                               (srcIpAddr & 0xff000000) >> 24,
                               (srcIpAddr & 0xff0000) >> 16,
                               (srcIpAddr & 0xff00) >> 8,
                               srcIpAddr & 0xff,
                               sequenceNumber);
                  ackCode = ACK_BUFFER_NOT_AVAILABLE;
               }
            }
            else /* IPTCom DB waiting for TDC to be ready */
            {
               IPTVosPrint1(IPT_WARN,
               "trReceive: IPTCom config waiting for TDC to be ready Comid=%d\n",
                            comId);
               ackCode = ACK_BUFFER_NOT_AVAILABLE;
            }

            if (ackCode == ACK_OK)
            {
               /* Prepare message to application */
         
               msgInfo.comId = comId;
               msgInfo.sessionId = FROMWIRE32(*(UINT32*)(pInBuf+SESSION_ID_OFF));/*lint !e826  Ignore casting warning */
               if ((msgType == MD_REQ_ACK_MSG) ||
                   (msgType == MD_REQ_NOACK_MSG) ||
                   (msgType == MD_REQ_FRG_MSG))
               {
                  msgInfo.msgType = MD_MSGTYPE_REQUEST;
               }
               else
               {
                  msgInfo.msgType = MD_MSGTYPE_DATA;
               }
               msgInfo.srcIpAddr = srcIpAddr;
               msgInfo.resultCode = MD_RECEIVE_OK;
               msgInfo.noOfResponses = 0;
               msgInfo.pCallerRef = 0;
               msgInfo.userStatus = 0;
               msgInfo.topoCnt = topoCount;

               strncpy(msgInfo.srcURI, (char *)(pInBuf + SRC_URI_OFF),
                       MD_URILEN);
               strncpy(msgInfo.destURI,
                       (char *)(pInBuf + SRC_URI_OFF + 4*srcURILength),
                       MD_URILEN);

               msgInfo.responseTimeout = FROMWIRE32(*(UINT32*)(pInBuf + 
                                                               SRC_URI_OFF + 
                                                               4*srcURILength + 
                                                               4*destURILength));/*lint !e826  Ignore casting warning */
               applMsg.msgInfo = msgInfo;
               applMsg.msgLength = pureDataLength;
               
               
               if (comId == ECHO_COMID)
               {
   #ifdef TARGET_SIMU
                  /* Create a simulation URI */
                  sprintf(simDevURI,"%d.%d.%d.%d", 
                     (simDevIpAddr >> 24) & 0xFF, 
                     (simDevIpAddr >> 16) & 0xFF, 
                     (simDevIpAddr >> 8) & 0xFF, 
                     simDevIpAddr & 0xFF);

                  (void)MDComAPI_putResponseMsgSim(comId,
                                          0,
                                          pPureData,
                                          pureDataLength,
                                          FROMWIRE32(*(UINT32*)(pInBuf +
                                                                SESSION_ID_OFF)),                          
                                          0, srcIpAddr, 0, 0, simDevURI);
   #else
                  (void)MDComAPI_putResponseMsg(comId,
                                          0,
                                          pPureData,
                                          pureDataLength,
                                          FROMWIRE32(*(UINT32*)(pInBuf +
                                                                SESSION_ID_OFF)), 
                                          0, srcIpAddr, 0, 0);/*lint !e826  Ignore casting warning */                         
   #endif
               }

               res = mdPutMsgOnListenerQueue(
#ifdef TARGET_SIMU
                                             simDevIpAddr,
#endif
                                             lastRecMsgNo,
                                             pureDataLength,
                                             &pPureData,
                                             &applMsg,
                                             uriType,
                                             uriInstName,
                                             uriFuncName,
                                             &callBackListerFound);
               if (res)
               {
                  listenerQueuFull = 1;  
               }
            }
         }

         /* Send acknowledge message? */
         if ((msgType == MD_ACK_MSG) || (msgType == MD_REQ_ACK_MSG) ||
             (msgType == MD_FRG_MSG) || (msgType == MD_REQ_FRG_MSG))
         {
            if (listenerQueuFull)
            {
               ackCode2 = ACK_BUFFER_NOT_AVAILABLE;
            }
            else
            {
               ackCode2 = ackCode;
            }
 
            if (ackCode2 == ACK_WRONG_FCS)
            {
               /* Enable resending by clearing the stored sequence number */
               /* CR-3477, GW, 2012-04-17, add NULL check */
               if (NULL != pSeqNoListItem)
                  pSeqNoListItem->seqNo = 0;
            }
            else
            {
               /* Store ack code together with the sequence number */
               /* CR-3477, GW, 2012-04-17, add NULL check */
               if (NULL != pSeqNoListItem)
                  pSeqNoListItem->lastAckCode = ackCode2; 
            }

            if ((msgType == MD_ACK_MSG) || (msgType == MD_REQ_ACK_MSG))
            {
               /* Send ACK message */
#ifdef TARGET_SIMU
               res = sendAck(srcIpAddr, simDevIpAddr, ackCode2, pInBuf);
#else
               res = sendAck(srcIpAddr, ackCode2, pInBuf);
#endif
               if (res != (int)IPT_OK)
               {
                  IPTVosPrint5(IPT_ERR,
                     "Send ack error=%#x IPaddr=%d.%d.%d.%d\n",
                     res,
                     (srcIpAddr & 0xff000000) >> 24,
                     (srcIpAddr & 0xff0000) >> 16,
                     (srcIpAddr & 0xff00) >> 8,
                     srcIpAddr & 0xff);
               }
            }
            else if (ackCode2 != ACK_DEST_UKNOWN)
            {
               /* Send ACK message */
#ifdef TARGET_SIMU
               res = sendFrgAck(srcIpAddr, simDevIpAddr, destIpAddr, ackCode2, pInBuf);
#else
               res = sendFrgAck(srcIpAddr, destIpAddr, ackCode2, pInBuf);
#endif
               if (res != (int)IPT_OK)
               {
                  IPTVosPrint5(IPT_ERR,
                     "Send FRG ack error=%#x IPaddr=%d.%d.%d.%d\n",
                     res,
                     (srcIpAddr & 0xff000000) >> 24,
                     (srcIpAddr & 0xff0000) >> 16,
                     (srcIpAddr & 0xff00) >> 8,
                     srcIpAddr & 0xff);
               }
            }
         }
      
         /* Received OK? */
         if ((ackCode == ACK_OK) && (callBackListerFound))
         {
            mdPutMsgOnListenerFunc(
#ifdef TARGET_SIMU
                                   simDevIpAddr,
#endif
                                   lastRecMsgNo,
                                   pureDataLength,
                                   pPureData,
                                   &msgInfo,
                                   uriType,
                                   uriInstName,
                                   uriFuncName);
         }

         if (pPureData != 0)
         {
            res = IPTVosFree((BYTE *)pPureData);
            if(res != 0)
            {
               IPTVosPrint1(IPT_ERR,
                            "trReceive failed to free memory, code=%#x\n",res);
            }
         }
      }
      else /* already received */
      {
         if (alreadyReceived == (int)IPT_ERROR)
         {
            /* There was not memory free to create a sequence counter list */
            ackCode =  ACK_BUFFER_NOT_AVAILABLE;
         }
         else
         {
            ackCode =  pSeqNoListItem->lastAckCode;/*lint !e613 pSeqNoListItem cannot be NUL */
         }
 
         
         if ((msgType == MD_ACK_MSG) || (msgType == MD_REQ_ACK_MSG))
         {
            /* Send ACK message */
#ifdef TARGET_SIMU
            res = sendAck(srcIpAddr, simDevIpAddr, ackCode, pInBuf);
#else
            res = sendAck(srcIpAddr, ackCode, pInBuf);
#endif            
            if (res != (int)IPT_OK)
            {
               IPTVosPrint5(IPT_ERR,
                  "Send ack error=%#x IPaddr=%d.%d.%d.%d\n",
                  res,
                  (srcIpAddr & 0xff000000) >> 24,
                  (srcIpAddr & 0xff0000) >> 16,
                  (srcIpAddr & 0xff00) >> 8,
                  srcIpAddr & 0xff);
            }
         }
         else if (ackCode != ACK_DEST_UKNOWN)
         {
            /* Send ACK message */
#ifdef TARGET_SIMU
            res = sendFrgAck(srcIpAddr, simDevIpAddr, destIpAddr, ackCode, pInBuf);
#else
            res = sendFrgAck(srcIpAddr, destIpAddr, ackCode, pInBuf);
#endif
            if (res != (int)IPT_OK)
            {
               IPTVosPrint5(IPT_ERR,
                  "Send FRG ack error=%#x IPaddr=%d.%d.%d.%d\n",
                  res,
                  (srcIpAddr & 0xff000000) >> 24,
                  (srcIpAddr & 0xff0000) >> 16,
                  (srcIpAddr & 0xff00) >> 8,
                  srcIpAddr & 0xff);
            }
         }
      }
   }
   else if (msgType == MD_REPLY_MSG)
   {
      ackCode = ACK_OK;   /* temporary setting */
      comId = FROMWIRE32(*(UINT32*)(pInBuf+COMID_OFF));/*lint !e826  Ignore casting warning */
      
      /* check sequence number */
      /* Not previously received */
      sequenceNumber = FROMWIRE16(*(UINT16*)(pInBuf+SEQ_NO_OFF));/*lint !e826  Ignore casting warning */
#ifdef TARGET_SIMU
      alreadyReceived = checkRecSequenceCounter(simDevIpAddr, srcIpAddr, sequenceNumber,
                                                &pSeqNoListItem);
#else
      alreadyReceived = checkRecSequenceCounter(srcIpAddr, sequenceNumber,
                                                &pSeqNoListItem);
#endif
      if (alreadyReceived == 0)
      {
         /* search for session instance and get session instance semaphore*/
         pSeInstance =  searchSession(FROMWIRE32(*(UINT32*)(pInBuf + 
                                                            SESSION_ID_OFF)));/*lint !e826  Ignore casting warning */
      
         if (pSeInstance == 0)
         {
            IPTVosPrint6(IPT_WARN,
               "No session instance for received reply message from srcIpAddr=%d.%d.%d.%d with sessionId=%d trSeqNo=%d\n",
               srcIpAddr/(UINT32)(0x1000000),
               (srcIpAddr%(UINT32)0x1000000)/(UINT32)0x10000,
               (srcIpAddr%(UINT32)0x10000)/(UINT32)0x100,
               srcIpAddr%(UINT32)0x100,
               FROMWIRE32(*(UINT32*)(pInBuf+SESSION_ID_OFF)),
               FROMWIRE16(*(UINT16*)(pInBuf + SEQ_NO_OFF)) );/*lint !e826  Ignore casting warning */
            ackCode = ACK_DEST_UKNOWN;
         }
         else
         {
            /* Get dataset */
            res = iptConfigGetDatasetId(comId, &datasetId);
            if ((res == (int)IPT_OK) && (datasetId != 0))
            {
               tempBufSize = ((dataLength+3) / 4 ) * 4;
               pTemp = IPTVosMalloc(tempBufSize);
               if (pTemp != 0)
               {
                  /* Check data frame checksum */
                  res = iptLoadReceiveDataFCS((BYTE *)pData,
                     dataPartLen,
                     pTemp,
                     &tempBufSize);  
                  if (res == 0)
                  {
                     /* Calculation of unpacked data size */
                     {
                        if (IPTGLOBAL(enableFrameSizeCheck) != FALSE)
                           pureDataLength = tempBufSize;
                     }
                     
                     /* Calculation of unpacked data size */
                     res = iptCalcDatasetSize(datasetId,
                                              pTemp,
                                              &pureDataLength);
                     if (res == IPT_OK)
                     {
                        pPureData = (char *)IPTVosMalloc(pureDataLength);

                        if (pPureData != 0)
                        {
                           /* Unmarshall */
                           res = iptUnmarshallDS(datasetId, pTemp, (BYTE *)pPureData,
                                               &pureDataLength);
                           if (res != (int)IPT_OK)
                           {
                              res = IPTVosFree((BYTE *)pPureData);
                              if(res != 0)
                              {
                                 IPTVosPrint1(IPT_ERR,
                                     "trReceive failed to free memory, code=%#x\n",
                                              res);
                              }
                              IPTVosPrint1(IPT_WARN,
                                    "Unmarshall of datasetId=%d failed\n",datasetId);
                              ackCode = ACK_WRONG_DATA;
                           }
                        }
                        else
                        {
                           IPTVosPrint6(IPT_ERR,
                                       "trReceive: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d sequenceNumber=%d\n",
                                        pureDataLength,
                                       (srcIpAddr & 0xff000000) >> 24,
                                       (srcIpAddr & 0xff0000) >> 16,
                                       (srcIpAddr & 0xff00) >> 8,
                                       srcIpAddr & 0xff,
                                        sequenceNumber);
                           ackCode = ACK_BUFFER_NOT_AVAILABLE;
                        }
                     }
                     else
                     {
                        IPTVosPrint3(IPT_ERR,
                              "Size of datasetId=%d of received ComId=%d could not be calculated res=%#x\n",
                                     datasetId, comId, res);
                        ackCode = ACK_WRONG_DATA;
                     }
                  }
                  else
                  {
                     IPTGLOBAL(md.mdCnt.mdInFCSErrors)++;
                     IPTVosPrint0(IPT_WARN,
                           "Message received with wrong data frame checksum\n");
                     ackCode = ACK_WRONG_FCS;
                  }

                  res = IPTVosFree(pTemp);
                  if(res != 0)
                  {
                     IPTVosPrint1(IPT_ERR,
                                  "trReceive failed to free memory, code=%#x\n",
                                  res);
                  }
               }
               else
               {
                  IPTVosPrint6(IPT_ERR,
                              "trReceive: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d sequenceNumber=%d\n",
                               tempBufSize,
                               (srcIpAddr & 0xff000000) >> 24,
                               (srcIpAddr & 0xff0000) >> 16,
                               (srcIpAddr & 0xff00) >> 8,
                               srcIpAddr & 0xff,
                               sequenceNumber);
                  ackCode = ACK_BUFFER_NOT_AVAILABLE;
               }
            }
            else if (res != (int)IPT_TDC_NOT_READY) /* Unspecified dataset */
            {
               pureDataLength = ((dataLength+3) / 4 ) * 4;
               pPureData = (char *)IPTVosMalloc(pureDataLength);
               if (pPureData != 0)
               {
                  /* Check data frame checksum */
                  res = iptLoadReceiveDataFCS((BYTE *)pData,
                     dataPartLen,
                     (BYTE *)pPureData,
                     &pureDataLength);  
                  if (res == 0)
                  {
                     /* Remove padding bytes from the data */
                     pureDataLength = dataLength;   
                  }
                  else
                  {
                     IPTGLOBAL(md.mdCnt.mdInFCSErrors)++;
                     
                     IPTVosPrint0(IPT_WARN,
                           "Message received with wrong data frame checksum\n");

                     ackCode = ACK_WRONG_FCS;
                     res = IPTVosFree((BYTE *)pPureData);
                     if(res != 0)
                     {
                        IPTVosPrint1(IPT_ERR,
                                  "trReceive failed to free memory, code=%#x\n",
                                     res);
                     }
                  }
               }
               else
               {
                  IPTVosPrint6(IPT_ERR,
                              "trReceive: Out of memory. Requested size=%d IPaddr=%d.%d.%d.%d sequenceNumber=%d\n",
                               pureDataLength,
                               (srcIpAddr & 0xff000000) >> 24,
                               (srcIpAddr & 0xff0000) >> 16,
                               (srcIpAddr & 0xff00) >> 8,
                               srcIpAddr & 0xff,
                               sequenceNumber);
                  ackCode = ACK_BUFFER_NOT_AVAILABLE;
               }
            }
            else /* IPTCom DB waiting for TDC to be ready */
            {
               IPTVosPrint1(IPT_WARN,
               "trReceive: IPTCom config waiting for TDC to be ready Comid=%d\n",
                            comId);
               ackCode = ACK_BUFFER_NOT_AVAILABLE;
            }
            
            if (ackCode == ACK_OK)
            {
               /* Prepare message to application */
               applMsg.msgInfo.comId = comId;
               applMsg.msgInfo.msgType = MD_MSGTYPE_RESPONSE;
               applMsg.msgLength = pureDataLength;
               applMsg.msgInfo.sessionId = FROMWIRE32(*(UINT32*)(pInBuf + 
                                                               SESSION_ID_OFF));/*lint !e826  Ignore casting warning */
               applMsg.msgInfo.srcIpAddr = srcIpAddr;
               applMsg.msgInfo.resultCode = MD_RECEIVE_OK;
               applMsg.msgInfo.noOfResponses = pSeInstance->replyCount + 1;                    
               applMsg.msgInfo.pCallerRef = (void *)pSeInstance->pCallerRef;                    
               applMsg.msgInfo.userStatus = FROMWIRE16(*(UINT16*)(pInBuf + 
                                                              USER_STATUS_OFF)); /*lint !e826  Ignore casting warning */                   
               applMsg.msgInfo.topoCnt = topoCount;                    

               srcURILength = *((UINT8*)(pInBuf + SRC_URI_LENGTH_OFF));
               destURILength = *((UINT8*)(pInBuf + DEST_URI_LENGTH_OFF));
               strncpy(applMsg.msgInfo.srcURI, (char *)(pInBuf + SRC_URI_OFF),
                       MD_URILEN);
               strncpy(applMsg.msgInfo.destURI,
                       (char *)(pInBuf + SRC_URI_OFF + 4 * srcURILength),
                       MD_URILEN);

               applMsg.msgInfo.responseTimeout = FROMWIRE32(*(UINT32*)(pInBuf + 
                                                                  SRC_URI_OFF + 
                                                                  4*srcURILength + 
                                                                  4*destURILength));/*lint !e826  Ignore casting warning */
            
               applMsg.pMsgData = pPureData;
         
               if (pSeInstance->callerQueueId != 0)
               {
                  /* Send result on caller queue. The pPureData memory is 
                     released when the queuue is read */
                  res = IPTVosSendMsgQueue((IPT_QUEUE *)((void *)&(pSeInstance->callerQueueId)),
                                           (char *)&applMsg, sizeof(QUEUE_MSG));
                  if (res != (int)IPT_OK)
                  {
                     listenerQueuFull = 1;
                    
                     pName = getQueueItemName(pSeInstance->callerQueueId);
                     IPTVosPrint4(IPT_ERR,
                                 "ERROR sending Response message on queue ID=%d Name=%s SessionID=%d pCallerRef=%#x\n",
                                 pSeInstance->callerQueueId,
                                 (pName != NULL)?pName:"None",
                                 pSeInstance->sessionId,
                                 pSeInstance->pCallerRef);
                     
                     
                     res = IPTVosFree((BYTE *)pPureData);
                     if(res != 0)
                     {
                        IPTVosPrint1(IPT_ERR,
                                  "trReceive failed to free memory, code=%#x\n",
                                     res);
                     }
                  }
               }

               pSeInstance->replyCount++;
            }
         }
         
         if (listenerQueuFull)
         {
            ackCode2 = ACK_BUFFER_NOT_AVAILABLE;
         }
         else
         {
            ackCode2 = ackCode;
         }
 
         if (ackCode2 == ACK_WRONG_FCS)
         {
            /* Enable resending by clearing the stored sequence number */
            pSeqNoListItem->seqNo = 0;
         }
         else
         {
            /* Store ack code together with the sequence number */
            pSeqNoListItem->lastAckCode = ackCode2; 
         }

         /* Send ACK message */
#ifdef TARGET_SIMU
            res = sendAck(srcIpAddr, simDevIpAddr, ackCode2, pInBuf);
#else
            res = sendAck(srcIpAddr, ackCode2, pInBuf);
#endif            
         if (res != (int)IPT_OK)
         {
            IPTVosPrint5(IPT_ERR,
               "Send ack error=%#x IPaddr=%d.%d.%d.%d\n",
               res,
               (srcIpAddr & 0xff000000) >> 24,
               (srcIpAddr & 0xff0000) >> 16,
               (srcIpAddr & 0xff00) >> 8,
               srcIpAddr & 0xff);
         }
      

         if (ackCode == ACK_OK)
         {
            if (pSeInstance != 0)
            {
               if (pSeInstance->callerFunc != 0)
               {
                  /* Call the caller callback function with send result */
                  pSeInstance->callerFunc(&applMsg.msgInfo, pPureData,
                                          pureDataLength);
 
                  res = IPTVosFree((BYTE *)pPureData);
                  if(res != 0)
                  {
                     IPTVosPrint1(IPT_ERR,
                             "trReceive failed to free data memory, code=%#x\n",
                                  res);
                  }
               }

               res = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);

               /* Release the session instance */
               pSeInstance->recActive = 0;
               
               if (res == IPT_OK)
               {
                  if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
                  {
                     IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                  }
               }
               else
               {
                  IPTVosPrint0(IPT_ERR, "trReceive: IPTVosGetSem ERROR\n");
               }

            }
         }
         else if (pSeInstance != 0)
         {
            res = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);

            /* Release the session instance */
            pSeInstance->recActive = 0;
            
            if (res == IPT_OK)
            {
               if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
               {
                  IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
               }
            }
            else
            {
               IPTVosPrint0(IPT_ERR, "trReceive: IPTVosGetSem ERROR\n");
            }
         }
      }
      else
      {
         if (alreadyReceived == (int)IPT_ERROR)
         {
            /* There was not memory free to create a sequence counter list */
            ackCode =  ACK_BUFFER_NOT_AVAILABLE;
         }
         else
         {
            ackCode =  pSeqNoListItem->lastAckCode;
         }
 
         /* Send ACK message */
#ifdef TARGET_SIMU
         res = sendAck(srcIpAddr, simDevIpAddr, ackCode, pInBuf);
#else
         res = sendAck(srcIpAddr, ackCode, pInBuf);
#endif            
         if (res != (int)IPT_OK)
         {
            IPTVosPrint5(IPT_ERR,
               "Send ack error=%#x IPaddr=%d.%d.%d.%d\n",
               res,
               (srcIpAddr & 0xff000000) >> 24,
               (srcIpAddr & 0xff0000) >> 16,
               (srcIpAddr & 0xff00) >> 8,
               srcIpAddr & 0xff);
         }
      }
      
   }
}

/*******************************************************************************
NAME:     showSequenceCounterLists
ABSTRACT: 
RETURNS:  -
*/
void showSequenceCounterLists(void)
{
   int res;
   SEND_SEQ_CNT    *pSendSeqCntStr;
   REC_SEQ_CNT     *pRecSeqCntStr;
   REC_FRG_SEQ_CNT *pRecFrgSeqCntStr;
   
   UINT16 max = IPTGLOBAL(md.maxStoredSeqNo);

   MON_PRINTF("Send sequence counter list\n");

   res = IPTVosGetSem(&IPTGLOBAL(md.SendSeqSemId), IPT_WAIT_FOREVER);
   if (res != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "showSequenceCounterLists: IPTVosGetSem ERROR\n");
   }
   else
   {
      pSendSeqCntStr = IPTGLOBAL(md.pFirstSendSeqCnt);
      while (pSendSeqCntStr != 0)
      {
         MON_PRINTF(" IPaddress=%d.%d.%d.%d max=%d\n", 
                (pSendSeqCntStr->destIpAddr >> 24) & 0xFF,
                (pSendSeqCntStr->destIpAddr >> 16) & 0xFF,
                (pSendSeqCntStr->destIpAddr >> 8) & 0xFF,
                pSendSeqCntStr->destIpAddr & 0xFF,
                pSendSeqCntStr->maxActiveSeqNo);
        
         pSendSeqCntStr = pSendSeqCntStr->pNext;
      }
      if(IPTVosPutSemR(&IPTGLOBAL(md.SendSeqSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }

   MON_PRINTF("\nReceive max stored sequence no = %d\n",max);
   MON_PRINTF("Receive sequence counter list\n");
   
   pRecSeqCntStr = IPTGLOBAL(md.pFirstRecSeqCnt);
   while (pRecSeqCntStr != 0)
   {
      MON_PRINTF(" IPaddress=%d.%d.%d.%d\n", 
             (pRecSeqCntStr->srcIpAddr >> 24) & 0xFF,
             (pRecSeqCntStr->srcIpAddr >> 16) & 0xFF,
             (pRecSeqCntStr->srcIpAddr >> 8) & 0xFF,
             pRecSeqCntStr->srcIpAddr & 0xFF);
   
     pRecSeqCntStr = pRecSeqCntStr->pNext;
   }
   
   MON_PRINTF("FRG receive sequence counter list\n");
  
   pRecFrgSeqCntStr = IPTGLOBAL(md.pFirstFrgRecSeqCnt);
   while (pRecFrgSeqCntStr != 0)
   {
      MON_PRINTF(" IPaddress src=%d.%d.%d.%d dest=%d.%d.%d.%d\n", 
             (pRecFrgSeqCntStr->srcIpAddr >> 24) & 0xFF,
             (pRecFrgSeqCntStr->srcIpAddr >> 16) & 0xFF,
             (pRecFrgSeqCntStr->srcIpAddr >> 8) & 0xFF,
             pRecFrgSeqCntStr->srcIpAddr & 0xFF,
             (pRecFrgSeqCntStr->destIpAddr >> 24) & 0xFF,
             (pRecFrgSeqCntStr->destIpAddr >> 16) & 0xFF,
             (pRecFrgSeqCntStr->destIpAddr >> 8) & 0xFF,
             pRecFrgSeqCntStr->destIpAddr & 0xFF);
   
     pRecFrgSeqCntStr = pRecFrgSeqCntStr->pNext;
   }

}

