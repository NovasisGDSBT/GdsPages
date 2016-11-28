/*******************************************************************************
*  COPYRIGHT      : (c) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT        : IPTrain
*
*  MODULE         : mdses.c
*
*  ABSTRACT       : Message data communication session layer
*
********************************************************************************
* HISTORY         :
*	
* $Id: mdses.c 11620 2010-08-16 13:06:37Z bloehr $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*
*******************************************************************************/

/*******************************************************************************
* INCLUDES */

#if defined(VXWORKS)
#include <vxWorks.h>
#endif

#include "iptcom.h"		/* Common type definitions for IPT */
#include "vos.h"
#include "mdcom_priv.h"
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"
#include "mdses.h"


#define SE_INIT       1
#define SE_WAIT_ACK   2
#define SE_WAIT_REPLY 3
#define SE_READY      4

/*******************************************************************************
* TYPEDEFS */


/*******************************************************************************
* GLOBALS */

/*******************************************************************************
* LOCALS */

/*******************************************************************************
* LOCAL FUNCTIONS */

/*******************************************************************************
NAME:     deleteSeInstance
ABSTRACT: Delete a session instance. 
          The used memory is freed. 
          The pointer to next instance of the deleted instance is copied to the
          previous instance or to the list start pointer (pFirstSeInstance).
RETURNS:  -
*/
static void deleteSeInstance(
   SESSION_INSTANCE *pSeInstance) /* Pointer to session instance to be deleted */
{
   int res;
   SESSION_INSTANCE **ppSeInstance;
   SESSION_INSTANCE *pPrevSeInstance;
   
   ppSeInstance = &IPTGLOBAL(md.pFirstSeInstance);
   pPrevSeInstance = 0;
   
   while (*ppSeInstance)
   {
      if (*ppSeInstance == pSeInstance)
      {
         *ppSeInstance = pSeInstance->pNext;
      
         /* Last instance to be deleted? */
         if (IPTGLOBAL(md.pLastSeInstance) == pSeInstance)
         {
            IPTGLOBAL(md.pLastSeInstance) = pPrevSeInstance;
         }
      
         /* deallocate session instance */
         res = IPTVosFree((BYTE *)pSeInstance);
         if(res != 0)
         {
            IPTVosPrint1(IPT_ERR, 
               "Failed to free session instance memory, code=%#x\n",
               res);
         }
      
      
         
         /* finished */
         break;
      }
      else
      {
         /* next */
         pPrevSeInstance = *ppSeInstance;
         ppSeInstance = &((*ppSeInstance)->pNext);
      }
   }  
}

/*******************************************************************************
NAME:     unicastInstance
ABSTRACT: State machine for unicast session.
RETURNS:  -
*/
static void unicastInstance(
   SESSION_INSTANCE *pSeInstance)  /* Pointer to session instance */
{
   char *pTemp;
   int res;
   QUEUE_MSG resultMsg;
   UINT32 deltaTime;
   IPT_REC_FUNCPTR callerFunc;       /* Pointer to callback function */
   
   resultMsg.msgInfo.resultCode = MD_RECEIVE_OK;
  
   switch (pSeInstance->state)
   {
      case SE_INIT:
         pSeInstance->state = SE_WAIT_ACK;
      
      /* fall through */
      case SE_WAIT_ACK:
         if (pSeInstance->ackReceived )
         {
            if (pSeInstance->trResCode == MD_SEND_OK)
            {
               pSeInstance->timeOutTime = IPTVosGetMilliSecTimer() +
                                          pSeInstance->replyTimeOutVal;
               pSeInstance->state = SE_WAIT_REPLY;
            }
            else
            {
               res = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
               if (res == IPT_OK)
               {
                  pSeInstance->state = SE_READY;
                  resultMsg.msgInfo.resultCode = pSeInstance->trResCode;
                  resultMsg.msgInfo.noOfResponses  = 0;
                  resultMsg.msgInfo.userStatus  = 0;

                  IPTVosPrint3(IPT_WARN,
                             "ERROR result code=%d sessionId=%d pCallerRef=%#x\n",
                               pSeInstance->trResCode,
                               pSeInstance->sessionId,
                               pSeInstance->pCallerRef );
               }
               else
               {
                  IPTVosPrint0(IPT_ERR, "unicastInstance: IPTVosGetSem ERROR\n");
               }
            }
         }
         break;
      
      case SE_WAIT_REPLY:
         /* Known number of expected replies? */
         if (pSeInstance->expectedNoOfReplies != 0)
         {
            if (pSeInstance->replyCount >= pSeInstance->expectedNoOfReplies )
            {
               res = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
               if (res == IPT_OK)
               {
                  /* Check that receiving is not going on */
                  if (pSeInstance->recActive == 0)
                  {
                     /* Replies sent to caller by transport layer */
            
                     resultMsg.msgInfo.resultCode = MD_RECEIVE_OK;
                     pSeInstance->state = SE_READY;
                  }
                  else
                  {
                     /* receiving is going on wait until next call */
                     if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                  }
               }
               else
               {
                  IPTVosPrint0(IPT_ERR, "unicastInstance: IPTVosGetSem ERROR\n");
               }
            }
            else
            {
               /* Timeout ? */
               deltaTime = pSeInstance->timeOutTime - IPTVosGetMilliSecTimer() -1;
               if (deltaTime >= pSeInstance->replyTimeOutVal)
               {
                  res = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
                  if (res == IPT_OK)
                  {
                     /* Check that receiving is not going on */
                     if (pSeInstance->recActive == 0)
                     {
                        if (pSeInstance->replyCount > 0)
                        {
                           /* Check again that not the correct numbers has been received */
                           if (pSeInstance->replyCount >= pSeInstance->expectedNoOfReplies )
                           {
                              /* Replies sent to caller by transport layer */
            
                              resultMsg.msgInfo.resultCode = MD_RECEIVE_OK;
                           }
                           else
                           {
                              IPTVosPrint3(IPT_WARN,
                                 "ERROR Missing %d replies before time-out SessionID=%d  pCallerRef=%#x\n",
                                 pSeInstance->expectedNoOfReplies - pSeInstance->replyCount,
                                 pSeInstance->sessionId,
                                 pSeInstance->pCallerRef);

                              resultMsg.msgInfo.resultCode = MD_RESP_MISSING;
                           }
                        }
                        else
                        {
                           IPTVosPrint2(IPT_WARN,
                              "No reply received within time-out SessionID=%d pCallerRef=%#x\n",
                              pSeInstance->sessionId,
                              pSeInstance->pCallerRef);

                           resultMsg.msgInfo.resultCode = MD_RESP_NOT_RECEIVED;
                        }
                        pSeInstance->state = SE_READY;
                     }
                     else
                     {
                        /* receiving is going on wait until next call */
                        if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
                        {
                           IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                        }
                     }
                  }
                  else
                  {
                     IPTVosPrint0(IPT_ERR, "unicastInstance: IPTVosGetSem ERROR\n");
                  }
               }
            } 
         }
         else
         {
            /* Waited the whole time-out time ? */
            deltaTime = pSeInstance->timeOutTime - IPTVosGetMilliSecTimer() -1;
            if (deltaTime >= pSeInstance->replyTimeOutVal)
            {
               res = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
               if (res == IPT_OK)
               {
                  /* Check that receiving is not going on */
                  if (pSeInstance->recActive == 0)
                  {
                     if (pSeInstance->replyCount > 0)
                     {
                        resultMsg.msgInfo.resultCode = MD_RECEIVE_OK;
                     }
                     else
                     {
                        IPTVosPrint3(IPT_WARN,
                           "No reply received within time-out SessionID=%d pCallerRef=%#x timeout=%ud\n",
                           pSeInstance->sessionId,
                           pSeInstance->pCallerRef,
                           pSeInstance->replyTimeOutVal);
                        resultMsg.msgInfo.resultCode = MD_RESP_NOT_RECEIVED;
                     }
                     pSeInstance->state = SE_READY;
                  }
                  else
                  {
                     /* receiving is going on wait until next call */
                     if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                  }
         
               }
               else
               {
                  IPTVosPrint0(IPT_ERR, "unicastInstance: IPTVosGetSem ERROR\n");
               }
            }
         }
         break;
      
      default:
         break;
      
   }    
   
   if (pSeInstance->state == SE_READY)
   {
      if ((pSeInstance->expectedNoOfReplies == 0) ||
           resultMsg.msgInfo.resultCode != MD_RECEIVE_OK)
      {
         /* send result to application */
         resultMsg.msgInfo.comId = pSeInstance->comId;
         resultMsg.msgInfo.msgType = MD_MSGTYPE_RESULT;
         resultMsg.msgInfo.sessionId = pSeInstance->sessionId;
         resultMsg.msgInfo.srcIpAddr = pSeInstance->replierIpAddr;
         resultMsg.msgInfo.noOfResponses = pSeInstance->replyCount;
         resultMsg.msgInfo.pCallerRef = (void *)pSeInstance->pCallerRef;
         resultMsg.msgInfo.userStatus = 0;
         resultMsg.msgInfo.responseTimeout = 0;
         resultMsg.msgInfo.topoCnt = 0;
         resultMsg.msgInfo.destURI[0] = 0;
         resultMsg.msgInfo.srcURI[0] = 0;
         resultMsg.msgLength = 0;
         resultMsg.pMsgData = NULL;
      
         if (pSeInstance->callerQueueId != 0)
         {
            /* Send result on caller queue */
            if (IPTVosSendMsgQueue((IPT_QUEUE *)((void *)&(pSeInstance->callerQueueId)),
                                   (char *)&resultMsg,
                                    sizeof(QUEUE_MSG)) != (int)IPT_OK)
            {
               pTemp = getQueueItemName(pSeInstance->callerQueueId);
               IPTVosPrint4(IPT_ERR,
                           "ERROR sending message on queue ID=%d Name=%s  SessionID=%d pCallerRef=%#x\n",
                           pSeInstance->callerQueueId,
                           (pTemp != NULL)?pTemp:"None",
                           pSeInstance->sessionId,
                           pSeInstance->pCallerRef);
            }
         }
         
         /* Save pointer to call back function to enable release of the
            semaphore before calling the application call-back function */
         callerFunc = pSeInstance->callerFunc;

         /* remove instance */
         deleteSeInstance(pSeInstance);
         if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
         
         if (callerFunc != 0)
         {
            /* Call the caller callback function with send result */
            callerFunc(&(resultMsg.msgInfo), NULL, 0);
         }
      }
      else
      {
         /* remove instance */
         deleteSeInstance(pSeInstance);
         if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
   }
}

/*******************************************************************************
NAME:     multicastInstance
ABSTRACT: State machine for multicast session.
RETURNS:  -
*/
static void multicastInstance(
   SESSION_INSTANCE *pSeInstance) /* Pointer to session instance */
{
   char *pTemp;
   int res;
   QUEUE_MSG resultMsg;
   UINT32 deltaTime;
   IPT_REC_FUNCPTR callerFunc;       /* Pointer to callback function */
  
   resultMsg.msgInfo.resultCode = MD_RECEIVE_OK;
   
   switch (pSeInstance->state)
   {
      case SE_INIT:
      
         pSeInstance->timeOutTime = IPTVosGetMilliSecTimer() +
                                    pSeInstance->replyTimeOutVal;
         pSeInstance->state = SE_WAIT_REPLY;
      
         break;
      
      case SE_WAIT_REPLY:
      
         /* Known number of expected replies? */
         if (pSeInstance->expectedNoOfReplies != 0)
         {
            if (pSeInstance->replyCount >= pSeInstance->expectedNoOfReplies )
            {
               res = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
               if (res == IPT_OK)
               {
                  /* Check that receiving is not going on */
                  if (pSeInstance->recActive == 0)
                  {
                     /* Replies sent to caller by transport layer */
            
                     resultMsg.msgInfo.resultCode = MD_RECEIVE_OK;
                     pSeInstance->state = SE_READY;
                  }
                  else
                  {
                     /* receiving is going on wait until next call */
                     if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                  }
               }
               else
               {
                  IPTVosPrint0(IPT_ERR, "multicastInstance: IPTVosGetSem ERROR\n");
               }
            }
            else
            {
               deltaTime = pSeInstance->timeOutTime - IPTVosGetMilliSecTimer() -1;
               if (deltaTime >= pSeInstance->replyTimeOutVal)
               {
                  res = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
                  if (res == IPT_OK)
                  {
                     /* Check that receiving is not going on */
                     if (pSeInstance->recActive == 0)
                     {
                        if (pSeInstance->replyCount > 0)
                        {
                           /* Check again that not the correct numbers has been received */
                           if (pSeInstance->replyCount >= pSeInstance->expectedNoOfReplies )
                           {
                              /* Replies sent to caller by transport layer */
            
                              resultMsg.msgInfo.resultCode = MD_RECEIVE_OK;
                           }
                           else
                           {
                              IPTVosPrint3(IPT_WARN,
                                 "ERROR Missing %d replies before time-out SessionID=%d  pCallerRef=%#x\n",
                                 pSeInstance->expectedNoOfReplies - pSeInstance->replyCount,
                                 pSeInstance->sessionId,
                                 pSeInstance->pCallerRef);
                              resultMsg.msgInfo.resultCode = MD_RESP_MISSING;
                           }
                        }
                        else
                        {
                           IPTVosPrint2(IPT_WARN,
                              "No reply received within time-out SessionID=%d pCallerRef=%#x\n",
                              pSeInstance->sessionId,
                              pSeInstance->pCallerRef);
                           resultMsg.msgInfo.resultCode = MD_RESP_NOT_RECEIVED;
                        }
                        pSeInstance->state = SE_READY;
                     }
                     else
                     {
                        /* receiving is going on wait until next call */
                        if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
                        {
                           IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                        }
                     }
                  }
                  else
                  {
                     IPTVosPrint0(IPT_ERR, "multicastInstance: IPTVosGetSem ERROR\n");
                  }
               }
            }
         }
         else
         {
            deltaTime = pSeInstance->timeOutTime - IPTVosGetMilliSecTimer() -1;
            if (deltaTime >= pSeInstance->replyTimeOutVal)
            {
               res = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
               if (res == IPT_OK)
               {
                  /* Check that receiving is not going on */
                  if (pSeInstance->recActive == 0)
                  {
                     if (pSeInstance->replyCount > 0)
                     {
                        resultMsg.msgInfo.resultCode = MD_RECEIVE_OK;
                     }
                     else
                     {
                        IPTVosPrint2(IPT_WARN,
                           "No reply received within time-out SessionID=%d pCallerRef=%#x\n",
                           pSeInstance->sessionId,
                           pSeInstance->pCallerRef);
                        resultMsg.msgInfo.resultCode = MD_RESP_NOT_RECEIVED;
                     }
                     pSeInstance->state = SE_READY;
                  }
                  else
                  {
                     /* receiving is going on wait until next call */
                     if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
                     {
                        IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
                     }
                  }
               }
               else
               {
                  IPTVosPrint0(IPT_ERR, "multicastInstance: IPTVosGetSem ERROR\n");
               }
            }
         }
         break;
      
      default:
      
         break;
      
   }    
   
   if (pSeInstance->state == SE_READY)
   {
      if ((pSeInstance->expectedNoOfReplies == 0) ||
           resultMsg.msgInfo.resultCode != MD_RECEIVE_OK)
      {
         /* send result to application */
         resultMsg.msgInfo.comId = pSeInstance->comId;
         resultMsg.msgInfo.msgType = MD_MSGTYPE_RESULT;
         resultMsg.msgInfo.sessionId = pSeInstance->sessionId;
         resultMsg.msgInfo.srcIpAddr = pSeInstance->replierIpAddr;
         resultMsg.msgInfo.noOfResponses = pSeInstance->replyCount;
         resultMsg.msgInfo.pCallerRef = (void *)pSeInstance->pCallerRef;
         resultMsg.msgInfo.userStatus  = 0;
         resultMsg.msgInfo.topoCnt = 0;
         resultMsg.msgInfo.responseTimeout = 0;
         resultMsg.msgInfo.destURI[0] = 0;
         resultMsg.msgInfo.srcURI[0] = 0;
         resultMsg.msgLength = 0;
         resultMsg.pMsgData = NULL;
      
         if (pSeInstance->callerQueueId != 0)
         {
            /* Send result on caller queue */
            if (IPTVosSendMsgQueue((IPT_QUEUE *)((void *)&(pSeInstance->callerQueueId)),
                                   (char *)&resultMsg,
                                    sizeof(QUEUE_MSG)) != (int)IPT_OK)
            {
               pTemp = getQueueItemName(pSeInstance->callerQueueId);
               IPTVosPrint4(IPT_ERR,
                           "ERROR sending Result message on queue ID=%d Name=%s SessionID=%d pCallerRef=%#x\n",
                           pSeInstance->callerQueueId,
                           (pTemp != NULL)?pTemp:"None",
                           pSeInstance->sessionId,
                           pSeInstance->pCallerRef);
            }
         }

         /* Save pointer to call back function to enable release of the
            semaphore before calling the application call-back function */
         callerFunc = pSeInstance->callerFunc;
         
         /* remove instance */
         deleteSeInstance(pSeInstance);
         if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
       
         if (callerFunc != 0)
         {
            /* Call the caller callback function with send result */
            callerFunc(&(resultMsg.msgInfo), NULL, 0);
         }
      }
      else
      {
         /* remove instance */
         deleteSeInstance(pSeInstance);
         if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
   }
}

/*******************************************************************************
* GLOBAL FUNCTIONS */

/*******************************************************************************
NAME:     seInit
ABSTRACT: Initiation of the session layer.
          Creating semaphore.
RETURNS:  0 if OK, !=0 if not.
*/
int seInit(void)
{
   int res;

   /* Create a semaphore for the session layer list resource
   initial state = free */
   res =  IPTVosCreateSem(&IPTGLOBAL(md.seSemId), IPT_SEM_FULL);
   if (res == (int)IPT_OK)
   {
      IPTGLOBAL(md.seInitiated) = 1;
      return((int)IPT_OK);
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "ERROR creating session layer semaphore\n");
      return(res);
   }
}
    
/*******************************************************************************
NAME:     seTerminate
ABSTRACT: Terminate the session layer.
RETURNS:  -
*/
void seTerminate(void)
{
   /* wait until all session instances has been finished */
   while (IPTGLOBAL(md.pFirstSeInstance) != 0)
   {
      IPTVosTaskDelay(10);
   }
   
   IPTGLOBAL(md.seInitiated) = 0;
   
   /* Destroy the semaphore for the session layer list resource */
   IPTVosDestroySem(&IPTGLOBAL(md.seSemId));
}
    
/*******************************************************************************
NAME:     createSeInstance
ABSTRACT: Create a new session instance.
RETURNS:  0 if OK, !=0 if not.
*/
int createSeInstance(
   int castType,                     /* Type of communication,
                                        unicast or multicast */
   int expectedNoOfReplies,          /* No of expected responses, 0 = unknown */
   char *pSendMsg,                   /* Pointer to message */
   const void *pCallerRef,		       /* Caller reference */
   MD_QUEUE callerQueueId,           /* Caller queue ID */
   IPT_REC_FUNCPTR callerFunc,       /* Pointer to callback function */
   UINT32 comId,                     /* ComID */
   MD_COM_PAR comPar,                /* Communication parameters */
   SESSION_INSTANCE **ppNewInstance) /* Pointer to pointer to instance */
{
   int res = (int)IPT_OK;

   /* check queue or function */
   if ((callerQueueId == 0) && (callerFunc == 0))
   {
      res = (int)IPT_INVALID_PAR;
      IPTVosPrint0(IPT_WARN, "ERROR no caller queue or caller function given\n");
   }
   else
   {
      *ppNewInstance = (SESSION_INSTANCE *)(IPTVosMalloc(sizeof(SESSION_INSTANCE)));
    
      if (*ppNewInstance == 0)
      {
         res = (int)IPT_MEM_ERROR;
         IPTVosPrint2(IPT_ERR,
                    "createSeInstance: Out of memory. Requested size=%d pCallerRef=%#x\n",
                    sizeof(SESSION_INSTANCE),
                    pCallerRef);
      }
      else
      {
         (*ppNewInstance)->pNext = 0;
         (*ppNewInstance)->castType = castType;
         (*ppNewInstance)->state = SE_INIT;
         (*ppNewInstance)->callerQueueId = callerQueueId;
         (*ppNewInstance)->callerFunc = callerFunc;
         (*ppNewInstance)->pSendMsg = pSendMsg;
         (*ppNewInstance)->ackReceived = 0;
         (*ppNewInstance)->expectedNoOfReplies = expectedNoOfReplies;
         (*ppNewInstance)->replyCount = 0;
         (*ppNewInstance)->replyTimeOutVal = comPar.replyTimeOutVal;
         (*ppNewInstance)->replierIpAddr = comPar.destIpAddr;
         (*ppNewInstance)->comId = comId;
         (*ppNewInstance)->pCallerRef = pCallerRef;
         (*ppNewInstance)->recActive = 0;
      }
   }

   return(res);    
}
    
/*******************************************************************************
NAME:     searchSeQueue
ABSTRACT: Check if a queue is used by a session instance.
RETURNS:  1 = found
          0 not found
*/
int searchSeQueue(
   MD_QUEUE queue)   /* Queue id */
{
   int ret;
   SESSION_INSTANCE *pSeInstance;  /* Pointer to session instance */
   int found = 0;
   if (IPTGLOBAL(md.seInitiated))
   {
      /* search for sending instance */
      ret = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pSeInstance = IPTGLOBAL(md.pFirstSeInstance);

         while ((pSeInstance) && (found == 0))
         {
            if (pSeInstance->callerQueueId == queue)
            {
               if (pSeInstance->expectedNoOfReplies != 0)
               {
                  /* Not all received? */
                  if (pSeInstance->replyCount < pSeInstance->expectedNoOfReplies )
                  {
                     /* Not ready yet */
                     found = 1;
                  }
               }
               else
               {
                  /* The queue will be used for result message */
                  found = 1;
               }
            }
            pSeInstance = pSeInstance->pNext;
         }
         if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         found = 1;
         IPTVosPrint0(IPT_ERR, "searchSeQueue: IPTVosGetSem ERROR\n");
      }
   }
   return(found);
}

/*******************************************************************************
NAME:     removeSeQueue
ABSTRACT: Check if a queue is used by a session instance.
RETURNS:  1 = found
          0 not found
*/
int removeSeQueue(
   MD_QUEUE queue)   /* Queue id */
{
   int ret = IPT_ERROR;
   SESSION_INSTANCE *pSeInstance;  /* Pointer to session instance */
 
   if (IPTGLOBAL(md.seInitiated))
   {
      /* search for sending instance */
      ret = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pSeInstance = IPTGLOBAL(md.pFirstSeInstance);

         while (pSeInstance)
         {
            if (pSeInstance->callerQueueId == queue)
            {
               /* Check that receiving is not going on */
               if (pSeInstance->recActive == 0)
               {
                  pSeInstance->callerQueueId = 0;   
               }
               else
               {
                  /* Queue used by receive task */
                  ret = IPT_ERROR;
               }
            }
            pSeInstance = pSeInstance->pNext;
         }
         if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "removeSeQueue: IPTVosGetSem ERROR\n");
      }
   }
   return(ret);
}

/*******************************************************************************
NAME:     insertSeInstance
ABSTRACT: Insert a new session instance into the linked list of active sessions.
RETURNS:  -
*/
void insertSeInstance(
   SESSION_INSTANCE *pSeInstance)  /* Pointer to session instance */
{
   int ret;

   /* avoid sessionId number == 0 */
   if (++IPTGLOBAL(md.sessionId) == 0)
   {
      IPTGLOBAL(md.sessionId)++;
   }
   pSeInstance->sessionId = IPTGLOBAL(md.sessionId);

   /* Session ID */
   *((UINT32*)(pSeInstance->pSendMsg + SESSION_ID_OFF)) = TOWIRE32(IPTGLOBAL(md.sessionId));/*lint !e826 pointer-to-pointer conversion is correct */
 
   /* Insert instance in the list */
   ret = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      if (IPTGLOBAL(md.pLastSeInstance) != 0)
      {
         IPTGLOBAL(md.pLastSeInstance)->pNext = pSeInstance;
      }

      IPTGLOBAL(md.pLastSeInstance) = pSeInstance;

      if (IPTGLOBAL(md.pFirstSeInstance) == 0)
      {
         IPTGLOBAL(md.pFirstSeInstance) = pSeInstance;
      }
      if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "insertSeInstance: IPTVosGetSem ERROR\n");
   }
}
    
/*******************************************************************************
NAME:     searchSession
ABSTRACT: Search session instance with a given session id.
RETURNS:  Pointer to a found session instance
          0 = not found
*/
SESSION_INSTANCE* searchSession(
   UINT32 sessionId) /* Session ID */
{
   int ret;
   SESSION_INSTANCE *pSeInstance;
   
   if (IPTGLOBAL(md.seInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.seSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pSeInstance = IPTGLOBAL(md.pFirstSeInstance);
   
         while (pSeInstance)
         {
            if (pSeInstance->sessionId == sessionId)
            {
               /* Set receive flag to mark that receiving is going on */
               pSeInstance->recActive = 1;

               /* finished */
               break;
            }
            else
            {
               /* next */
               pSeInstance = pSeInstance->pNext;
            }
         }
   
         if(IPTVosPutSemR(&IPTGLOBAL(md.seSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         pSeInstance = 0;
         IPTVosPrint0(IPT_ERR, "searchSession: IPTVosGetSem ERROR\n");
      }
   }
   else
   {
      pSeInstance = 0;
   }
   
   return(pSeInstance);    
}

/*******************************************************************************
NAME:     seSendTask
ABSTRACT: Handling active session instances.
          This function shall be call by the cyclic sending task.
RETURNS:  -
*/
void seSendTask( void )
{
   SESSION_INSTANCE *pSeInstance = IPTGLOBAL(md.pFirstSeInstance);
   SESSION_INSTANCE *pNextSeInstance;
   
   while (pSeInstance)
   {
      pNextSeInstance = pSeInstance->pNext;
      switch (pSeInstance->castType)
      {
      case UNICAST_SESSION_TYPE:
         unicastInstance(pSeInstance);
         break;
         
      case MULTICAST_SESSION_TYPE:
         multicastInstance(pSeInstance);
         break;
         
      default:
         break;
      }
      pSeInstance =  pNextSeInstance;
   }  
}
