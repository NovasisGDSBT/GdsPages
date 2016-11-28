/*******************************************************************************
*  COPYRIGHT   : (C) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT     :  IPTrain
*
*  MODULE      :  mdcom_cls.cpp
*
*  ABSTRACT    :  Public C++ methods for MD communication classes:
*                 - MDCom
*                 - MDQueue
*
********************************************************************************
*  HISTORY     :
*	
* $Id: mdcom_cls.cpp 11620 2010-08-16 13:06:37Z bloehr $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*
*******************************************************************************/

/*******************************************************************************
*  INCLUDES */
#include "iptcom.h"     /* Common type definitions for IPT */
#include "vos.h"     

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:     MDQueue::create
ABSTRACT: Creates a message data queue
RETURNS:  Object reference to be used for subsequent calls.
*/
MDQueue *MDQueue::createQueue(
   int noOfMsg) /* Number of queue members */
{
   MDQueue *q = new MDQueue();

   q->queue = MDComAPI_createQueue(noOfMsg);

   if (q->queue == (MD_QUEUE) NULL)
   {
		throw MDComAPIException((int)IPT_ERROR, __FILE__, __LINE__ );
   }

   return q;
}

/*******************************************************************************
NAME:     MDQueue::create
ABSTRACT: Creates a message data queue with name
RETURNS:  Object reference to be used for subsequent calls.
*/
MDQueue *MDQueue::queueCreate(
   int noOfMsg, /* Number of queue members */
   const char *name) /* Queue Name */       
{
   MDQueue *q = new MDQueue();

   q->queue = MDComAPI_queueCreate(noOfMsg, name);

   if (q->queue == (MD_QUEUE) NULL)
   {
		throw MDComAPIException((int)IPT_ERROR, __FILE__, __LINE__ );
   }

   return q;
}

/*******************************************************************************
NAME:     MDQueue::destroy
ABSTRACT: Destroys a message data queue
RETURNS:  0 if OK, !=0 if not.
*/
int MDQueue::destroyQueue(void)
{
   int res;

   res = MDComAPI_destroyQueue(this->queue);

   if (res == IPT_OK)
   {
      delete(this);
   }
   else
   {
      IPTVosPrint2(IPT_ERR,
          "ERROR destroy queue=%#x res=%#x\n",
                   this->queue, res);
   }

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

/*******************************************************************************
NAME:     MDQueue::removeQueue
ABSTRACT: Remove all use of and destroys a message data queue.
RETURNS:  0 if OK, !=0 if not.
*/
int MDQueue::removeQueue(UINT8 allUse)
{
   int res;

   res = MDComAPI_removeQueue(this->queue, allUse);

   if (res == IPT_OK)
   {
      delete(this);
   }
   else
   {
      IPTVosPrint2(IPT_ERR,
          "ERROR destroy queue=%#x res=%#x\n",
                   this->queue, res);
   }

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

/*******************************************************************************
NAME:     MDQueue::freeBuf
ABSTRACT: Deallocate buffer allocated by IPTCom used for received data
RETURNS:  0 if OK, !=0 if not.
*/
int MDQueue::freeBuf(
   char *pBuf) /* Pointer to buffer to be released */
{
   int res;

   res = MDComAPI_freeBuf(pBuf);

   return res;
}

/*******************************************************************************
NAME:     MDComAPI::comidListener
ABSTRACT: Add a listener for message data with the given comid's. Join 
          multicast groups for comid's with multicast destinations. 
          IPTCom will put received messages on the queue 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::comIdListener(
   const MDQueue *pQueue,     /* Reference to queue object to put received
                                 message data in */
   IPT_REC_FUNCPTR func,        /* Call back function */
   const void    *pCallerRef, /* Caller reference */
   const UINT32  comId[],     /* Array with comId's to listen to, ended by 0 */
   const void   *pRedFuncRef, /* Redundancy function reference */
   UINT32          destId,
   const char      *pDestURI)
{
   int res;

   res = MDComAPI_comIdListener((pQueue?pQueue->queue:0), func, pCallerRef, comId, pRedFuncRef, destId, pDestURI);

   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

/*******************************************************************************
NAME:     MDComAPI::uriListener
ABSTRACT: Add a listener for message data with the given comid's. Join 
          multicast groups for comid's with multicast destinations. 
          IPTCom will put received messages on the queue 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::uriListener(
   const MDQueue *pQueue,         /* Reference to queue object to put received
                                     message data in */
   IPT_REC_FUNCPTR func,          /* Call back function */
   const void      *pCallerRef,   /* Caller reference */
   const UINT32    comId,         /* ComId */
   UINT32          destId,        /* Destination URI ID */
   const char      *pDestURI,     /* Pointer to destination URI string */
   const void      *pRedFuncRef)  /* Redundancy function reference */
{
   int res;

   res = MDComAPI_uriListener((pQueue?pQueue->queue:0), func, pCallerRef, comId, destId, pDestURI, pRedFuncRef);

   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

/*******************************************************************************
NAME:     MDComAPI::addListener
ABSTRACT: Add a listener for message data with the given comid's. Join 
          multicast groups for comid's with multicast destinations. 
          IPTCom will put received messages on the queue 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::addListener(
   const MDQueue *pQueue,     /* Reference to queue object to put received
                                 message data in */
   const void    *pCallerRef, /* Caller reference */
   const UINT32  comid[])     /* Array with comId's to listen to, ended by 0 */
{
   int res;

   if (pQueue != NULL)
   {
      res = MDComAPI_addComIdListenerQ(pQueue->queue, pCallerRef, comid);
   }
   else
   {
      res = IPT_INVALID_PAR;
   }

   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI::addListener
ABSTRACT: Add a listener for message data with the given comid's. Join 
          multicast groups for comid's with multicast destinations. 
          IPTCom will call the call-back function when messages are received 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::addListener(
   IPT_REC_FUNCPTR func,        /* Call back function */
   const void      *pCallerRef, /* Caller reference */
   const UINT32    comid[])     /* Array with comId's to listen to, ended by 0 */
{
   int res;

   res = MDComAPI_addComIdListenerF(func, pCallerRef, comid);

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}
#endif

/*******************************************************************************
NAME:     MDComAPI::addListener
ABSTRACT: Add a listener for message data with the given destination URI. Join 
          multicast groups if a destination URI with a multicast destinations 
          host part is given. IPTCom will put received messages on the queue  
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::addListener(
   const MDQueue *pQueue,     /* Reference to queue object to put received
                                 message data in */
   const void    *pCallerRef, /* Caller reference */
   const char    *pDestURI)   /* Pointer to destination URI string */
{
   int res;

   if (pQueue != NULL)
   {
      res = MDComAPI_addUriListenerQ(pQueue->queue, pCallerRef, pDestURI);
   }
   else
   {
      res = IPT_INVALID_PAR;
   }

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI::addListener
ABSTRACT: Add a listener for message data with the given destination URI. 
          Join multicast groups if a destination URI with a multicast 
          destinations host part is given. 
          IPTCom will call the call-back function when messages are received 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::addListener(
   IPT_REC_FUNCPTR func,        /* Call back function */
   const void      *pCallerRef, /* Caller reference */
   const char      *pDestURI)   /* Pointer to destination URI string */
{
   int res;

   res = MDComAPI_addUriListenerF(func, pCallerRef, pDestURI);

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}
#endif

/*******************************************************************************
NAME:     MDComAPI::addListener
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
int MDComAPI::addListener(
   const MDQueue *pQueue,     /* Reference to queue object to put received
                                 message data in */
   const void   *pCallerRef,  /* Caller reference */
   const UINT32 comid[],      /* Array with comid's, ended by 0 */
   const void   *pRedFuncRef) /* Redundancy function reference */
{
   int res;

   if (pQueue != NULL)
   {
      res = MDComAPI_addFrgComIdListenerQ(pQueue->queue, pCallerRef, comid, pRedFuncRef);
   }
   else
   {
      res = IPT_INVALID_PAR;
   }

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI::addListener
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
int MDComAPI::addListener(
   IPT_REC_FUNCPTR func, /* Call back function */
   const void      *pCallerRef, /* Caller reference */
   const UINT32    comid[],     /* Array with comid's, ended by 0 */
   const void   *pRedFuncRef)    /* Redundancy function reference */
{
   int res;

   res = MDComAPI_addFrgComIdListenerF(func, pCallerRef, comid, pRedFuncRef);

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}
#endif

/*******************************************************************************
NAME:     MDComAPI::addListener
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
int MDComAPI::addListener(
   const MDQueue *pQueue,     /* Reference to queue object to put received
                                 message data in */
   const void  *pCallerRef,   /* Caller reference */
   const char  *pDestURI,     /* Pointer to destination URI string */
   const void   *pRedFuncRef) /* Redundancy function reference */
{
   int res;

   if (pQueue != NULL)
   {
      res = MDComAPI_addFrgUriListenerQ(pQueue->queue, pCallerRef, pDestURI, pRedFuncRef);
   }
   else
   {
      res = IPT_INVALID_PAR;
   }

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI::addListener
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
int MDComAPI::addListener(
   IPT_REC_FUNCPTR func,        /* Pointer to callback function */
   const void      *pCallerRef, /* Caller reference */
   const char      *pDestURI,   /* Pointer to destination URI string */
   const void   *pRedFuncRef)    /* Redundancy function reference */
{
   int res;

   res = MDComAPI_addFrgUriListenerF(func, pCallerRef, pDestURI, pRedFuncRef);

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}
#endif

/*******************************************************************************
NAME:     MDComAPI::removeListener
ABSTRACT: Removes a listener for queued received message data. 
RETURNS:  -
*/
void MDComAPI::removeListener(
   const MDQueue *pQueue)  /* Reference to queue object to be removed from 
                              listener list */
{
   int res = IPT_OK;

   if (pQueue != NULL)
   {
      MDComAPI_removeListenerQ(pQueue->queue);
   }
   else
   {
      res = IPT_INVALID_PAR;
   }

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );
}

/*******************************************************************************
NAME:     MDComAPI::removeListener
ABSTRACT: Removes a listener for queued received message data. 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::removeListener(
   const MDQueue *pQueue,  /* Reference to queue object to be removed from 
                              listener list */
   const void    *pCallerRef, /* Caller reference */
   const UINT32  comid[])     /* Array with comId's to listen to, ended by 0 */
{
   int res;

   if (pQueue != NULL)
   {
      res = MDComAPI_removeComIdListenerQ(pQueue->queue,pCallerRef,comid);
   }
   else
   {
      res = IPT_INVALID_PAR;
   }

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

/*******************************************************************************
NAME:     MDComAPI::removeListener
ABSTRACT: Removes a listener for queued received message data. 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::removeListener(
   const MDQueue *pQueue,  /* Reference to queue object to be removed from 
                              listener list */
   const void    *pCallerRef, /* Caller reference */
   const char    *pDestURI)   /* Pointer to destination URI string */
{
   int res;

   if (pQueue != NULL)
   {
      res = MDComAPI_removeUriListenerQ(pQueue->queue,pCallerRef,pDestURI);
   }
   else
   {
      res = IPT_INVALID_PAR;
   }

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI::removeListener
ABSTRACT: Removes a call back function listener for received message data. 
RETURNS:  -
*/
void MDComAPI::removeListener(
   IPT_REC_FUNCPTR func) /* Call back function to be removed from listener
                            list */
{
   MDComAPI_removeListenerF(func);
}

/*******************************************************************************
NAME:     MDComAPI::removeListener
ABSTRACT: Removes a call back function listener for received message data. 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::removeListener(
   IPT_REC_FUNCPTR func, /* Call back function to be removed from listener
                            list */
   const void    *pCallerRef, /* Caller reference */
   const UINT32  comid[])     /* Array with comId's to listen to, ended by 0 */
{
   int res;

   res = MDComAPI_removeComIdListenerF(func,pCallerRef,comid);

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

/*******************************************************************************
NAME:     MDComAPI::removeListener
ABSTRACT: Removes a call back function listener for received message data. 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::removeListener(
   IPT_REC_FUNCPTR func, /* Call back function to be removed from listener
                            list */
   const void    *pCallerRef, /* Caller reference */
   const char    *pDestURI)   /* Pointer to destination URI string */
{
   int res;

   res = MDComAPI_removeUriListenerF(func,pCallerRef,pDestURI);

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}
#endif

/*******************************************************************************
NAME:     MDComAPI::unblockFrgListener
ABSTRACT: Set a FRG listener in leader state, i.e. received FRG messages will
          be forwarded to the listener application and the messages will be
          acknowledged. 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::unblockFrgListener(
   const void *pRedFuncRef)  /* Redendancy function reference value */
{
   int res;

   res = MDComAPI_unblockFrgListener(pRedFuncRef);

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

/*******************************************************************************
NAME:     MDComAPI::blockFrgListener
ABSTRACT: Set a FRG listener in follower state, i.e. received FRG messages will
          be forwarded to the listener application and the messages will be
          acknowledged. 
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::blockFrgListener(
   const void *pRedFuncRef)  /* Redendancy function reference value */
{
   int res;

   res = MDComAPI_blockFrgListener(pRedFuncRef);

   if isException(res)
      throw MDComAPIException(res, __FILE__, __LINE__ );

   return res;
}

/*******************************************************************************
NAME:     MDComAPI::putDataMsg
ABSTRACT: Send message data. Communication result reported back to
          the caller via the caller queue or call-back function. The destination  and source URI
          for the comId is override by the caller URI strings.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putDataMsg(
   UINT32        comId,         /* ComID */
   const char    *pData,        /* Pointer to buffer with data to be send */
   UINT32        dataLength,    /* Number of bytes to be send */
   const MDQueue *pCallerQueue, /* Pointer to caller queue object */
   IPT_REC_FUNCPTR func,        /* Callback function */
   const void    *pCallerRef,   /* Caller reference */
   UINT32        topoCnt,       /* Topo counter */
   UINT32        destId,        /* Destination URI ID */
   const char    *pDestURI,     /* Pointer to overriding destination URI string */
   const char    *pSrcURI)      /* Pointer to overriding source URI string */
{
   int res;

   res = MDComAPI_putDataMsg(comId, pData, dataLength,
                          (pCallerQueue?pCallerQueue->queue:0), func, pCallerRef,
                          topoCnt, destId, pDestURI, pSrcURI);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putMsg
ABSTRACT: Send message data. No communication result reported back to
          the caller.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putMsg( 
   UINT32     comId,      /* ComID */
   const char *pData,     /* Pointer to buffer with data to be send */
   UINT32     dataLength) /* Number of bytes to be send */
{
   int res;

   res = MDComAPI_putMsgQ(comId, pData, dataLength, 0, 0, 0,
                          (char *)NULL, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putMsg
ABSTRACT: Send message data. Communication result reported back to
          the caller via the caller queue.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putMsg(
   UINT32        comId,         /* ComID */
   const char    *pData,        /* Pointer to buffer with data to be send */
   UINT32        dataLength,    /* Number of bytes to be send */
   const MDQueue *pCallerQueue, /* Pointer to caller queue object */
   const void    *pCallerRef)   /* Caller reference */
{
   int res;

   res = MDComAPI_putMsgQ(comId, pData, dataLength,
                          (pCallerQueue?pCallerQueue->queue:0), pCallerRef,
                          0, (char *)NULL, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putMsg
ABSTRACT: Send message data. Communication result reported back to
          the caller via the caller queue.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putMsg(
   UINT32        comId,        /* ComID */
   const char    *pData,       /* Pointer to buffer with data to be send */
   UINT32        dataLength,   /* Number of bytes to be send */
   const MDQueue *pCallerQueue, /* Pointer to caller queue object */
   const void    *pCallerRef,   /* Caller reference */
   UINT32        topoCnt)       /* Topo counter */
{
   int res;

   res = MDComAPI_putMsgQ(comId, pData, dataLength,
                          (pCallerQueue?pCallerQueue->queue:0), pCallerRef,
                          topoCnt, (char *)NULL, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putMsg
ABSTRACT: Send message data. Communication result reported back to
          the caller via the caller queue. The destination URI for the 
          comId is override by the caller URI string.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putMsg(
   UINT32        comId,         /* ComID */
   const char    *pData,        /* Pointer to buffer with data to be send */
   UINT32        dataLength,    /* Number of bytes to be send */
   const MDQueue *pCallerQueue, /* Pointer to caller queue object */
   const void    *pCallerRef,   /* Caller reference */
   UINT32        topoCnt,       /* Topo counter */
   const char    *pDestURI)     /* Pointer to overriding destination URI string */
{
   int res;

   res = MDComAPI_putMsgQ(comId, pData, dataLength,
                          (pCallerQueue?pCallerQueue->queue:0), pCallerRef,
                          topoCnt, pDestURI, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putMsg
ABSTRACT: Send message data. Communication result reported back to
          the caller via the caller queue. The destination  and source URI
          for the comId is override by the caller URI strings.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putMsg(
   UINT32        comId,         /* ComID */
   const char    *pData,        /* Pointer to buffer with data to be send */
   UINT32        dataLength,    /* Number of bytes to be send */
   const MDQueue *pCallerQueue, /* Pointer to caller queue object */
   const void    *pCallerRef,   /* Caller reference */
   UINT32        topoCnt,       /* Topo counter */
   const char    *pDestURI,     /* Pointer to overriding destination URI string */
   const char    *pSrcURI)      /* Pointer to overriding source URI string */
{
   int res;

   res = MDComAPI_putMsgQ(comId, pData, dataLength,
                          (pCallerQueue?pCallerQueue->queue:0), pCallerRef,
                          topoCnt, pDestURI, pSrcURI);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI::putMsg
ABSTRACT: Send message data. Communication result reported back to
          the caller via the call-back function.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putMsg(
   UINT32          comId,       /* ComID */
   const char      *pData,      /* Pointer to buffer with data to be send */
   UINT32          dataLength,  /* Number of bytes to be send */
   IPT_REC_FUNCPTR func,        /* Callback function */
   const void      *pCallerRef) /* Caller reference */
{
   int res;

   res = MDComAPI_putMsgF(comId, pData, dataLength, func, pCallerRef, 0,
                          (char *)NULL, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putMsg
ABSTRACT: Send message data. Communication result reported back to
          the caller via the call-back function.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putMsg(
   UINT32          comId,       /* ComID */
   const char      *pData,      /* Pointer to buffer with data to be send */
   UINT32          dataLength,  /* Number of bytes to be send */
   IPT_REC_FUNCPTR func,        /* Callback function */
   const void      *pCallerRef, /* Caller reference */
   UINT32          topoCnt)      /* Topo counter */
{
   int res;

   res = MDComAPI_putMsgF(comId, pData, dataLength, func, pCallerRef, topoCnt,
                          (char *)NULL, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putMsg
ABSTRACT: Send message data. Communication result reported back to
          the caller via the caller queue. The destination URI for the 
          comId is override by the caller URI string.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putMsg(
   UINT32          comId,       /* ComID */
   const char      *pData,      /* Pointer to buffer with data to be send */
   UINT32          dataLength,  /* Number of bytes to be send */
   IPT_REC_FUNCPTR func,        /* Callback function  */
   const void      *pCallerRef, /* Caller reference */
   UINT32          topoCnt,      /* Topo counter */
   const char      *pDestURI)   /* Pointer to overriding destination URI string */
{
   int res;

   res = MDComAPI_putMsgF(comId, pData, dataLength, func, pCallerRef, topoCnt,
                          pDestURI, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putMsg
ABSTRACT: Send message data. Communication result reported back to
          the caller via the caller queue. The destination  and source URI
          for the comId is override by the caller URI strings.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putMsg(
   UINT32          comId,       /* ComID */
   const char      *pData,      /* Pointer to buffer with data to be send */
   UINT32          dataLength,  /* Number of bytes to be send */
   IPT_REC_FUNCPTR func,        /* Callback function  */
   const void      *pCallerRef, /* Caller reference */
   UINT32          topoCnt,      /* Topo counter */
   const char      *pDestURI,   /* Pointer to overriding destination URI string */
   const char      *pSrcURI)    /* Pointer to overriding source URI string */
{
   int res;

   res = MDComAPI_putMsgF(comId, pData, dataLength, func, pCallerRef, topoCnt,
                          pDestURI, pSrcURI);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}
#endif

/*******************************************************************************
NAME:     MDComAPI::putReqMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the caller queue.
          Number of expected replies can be set in the call.
          Time-out for receiving replies can be set in the call.
          The destination  and source URI for the comId is override by the 
          caller URI strings.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putReqMsg(
   UINT32     comId,           /* ComID */
   const char *pData,          /* Pointer to buffer with data to be send */
   UINT32     dataLength,      /* Number of bytes to be send */
   UINT16     noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32     responseTimeout, /* Time-out value  in milliseconds for receiving 
                                  replies 0=default value */
   const MDQueue *pCallerQueue,   /* Caller queue */
   IPT_REC_FUNCPTR func,       /* Pointer to callback function.
                                  0 = No result will be reported back */
   const void *pCallerRef,     /* Caller reference */
   UINT32     topoCnt,         /* Topo counter */
   UINT32     destId,      /* Destination URI ID */
   const char *pDestURI,       /* Pointer to overriding destination URI string.
                                  0 = use URI string defined for the comID */
   const char *pSrcURI)        /* Pointer to overriding source URI string.
                                  0 = use URI string defined for the comID */
{
   int res;

   res = MDComAPI_putReqMsg(comId, pData, dataLength, noOfResponses,
                            responseTimeout, (pCallerQueue?pCallerQueue->queue:0), func, 
                            pCallerRef, topoCnt, destId, 
                            pDestURI, pSrcURI);
   
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the caller queue.
          Number of expected replies unknown.
          Default time-out for receiving replies.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg(
   UINT32        comId,         /* ComID */
   const char    *pData,        /* Pointer to buffer with data to be send */
   UINT32        dataLength,    /* Number of bytes to be send */
   const MDQueue *pCallerQueue, /* Caller queue */
   const void    *pCallerRef)   /* Caller reference */
{
   int res;

   if (pCallerQueue != NULL)
   {
      res = MDComAPI_putRequestMsgQ(comId, pData, dataLength, 0,
                                    0, pCallerQueue->queue, pCallerRef, 0,
                                    (char *)NULL,(char *)NULL);
   }
   else
   {
      res =(int)IPT_INVALID_PAR;
   }

   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the caller queue.
          Number of expected replies can be set in the call.
          Time-out for receiving replies can be set in the call.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg(
   UINT32        comId,           /* ComID */
   const char    *pData,          /* Pointer to buffer with data to be send */
   UINT32        dataLength,      /* Number of bytes to be send */
   UINT16        noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32        responseTimeout, /* Time-out value  in milliseconds for 
                                     receiving replies. 0=default value */
   const MDQueue *pCallerQueue,   /* Caller queue */
   const void    *pCallerRef)     /* Caller reference */
{
   int res;

   if (pCallerQueue != NULL)
   {
      res = MDComAPI_putRequestMsgQ(comId, pData, dataLength, noOfResponses,
                                    responseTimeout, pCallerQueue->queue, 
                                    pCallerRef, 0, 
                                    (char *)NULL, (char *)NULL);
   }
   else
   {
      res =(int)IPT_INVALID_PAR;
   }
   
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the caller queue.
          Number of expected replies can be set in the call.
          Time-out for receiving replies can be set in the call.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg(
   UINT32        comId,           /* ComID */
   const char    *pData,          /* Pointer to buffer with data to be send */
   UINT32        dataLength,      /* Number of bytes to be send */
   UINT16        noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32        responseTimeout, /* Time-out value  in milliseconds for 
                                     receiving replies. 0=default value */
   const MDQueue *pCallerQueue,   /* Caller queue */
   const void    *pCallerRef,     /* Caller reference */
   UINT32        topoCnt)         /* Topo counter */
{
   int res;

   if (pCallerQueue != NULL)
   {
      res = MDComAPI_putRequestMsgQ(comId, pData, dataLength, noOfResponses,
                                    responseTimeout, pCallerQueue->queue, 
                                    pCallerRef, topoCnt, 
                                    (char *)NULL, (char *)NULL);
   }
   else
   {
      res =(int)IPT_INVALID_PAR;
   }
   
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the caller queue.
          Number of expected replies can be set in the call.
          Time-out for receiving replies can be set in the call.
          The destination URI for the comId is override by the caller URI
          string.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg(
   UINT32        comId,           /* ComID */
   const char    *pData,          /* Pointer to buffer with data to be send */
   UINT32        dataLength,      /* Number of bytes to be send */
   UINT16        noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32        responseTimeout, /* Time-out value  in milliseconds for 
                                     receiving replies. 0=default value */
   const MDQueue *pCallerQueue,   /* Caller queue */
   const void    *pCallerRef,     /* Caller reference */
   UINT32        topoCnt,         /* Topo counter */
   const char    *pDestURI)       /* Pointer to overriding destination URI string */
{
   int res;

   if (pCallerQueue != NULL)
   {
      res = MDComAPI_putRequestMsgQ(comId, pData, dataLength, noOfResponses,
                                    responseTimeout, pCallerQueue->queue, 
                                    pCallerRef, topoCnt, 
                                    pDestURI, (char *)NULL);
   }
   else
   {
      res =(int)IPT_INVALID_PAR;
   }
   
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the caller queue.
          Number of expected replies can be set in the call.
          Time-out for receiving replies can be set in the call.
          The destination  and source URI for the comId is override by the 
          caller URI strings.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg(
   UINT32        comId,           /* ComID */
   const char    *pData,          /* Pointer to buffer with data to be send */
   UINT32        dataLength,      /* Number of bytes to be send */
   UINT16        noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32        responseTimeout, /* Time-out value  in milliseconds for 
                                     receiving replies. 0=default value */
   const MDQueue *pCallerQueue,   /* Caller queue */
   const void    *pCallerRef,     /* Caller reference */
   UINT32        topoCnt,         /* Topo counter */
   const char    *pDestURI,       /* Pointer to overriding destination URI string */
   const char    *pSrcURI)        /* Pointer to overriding source URI string */
{
   int res;

   if (pCallerQueue != NULL)
   {
      res = MDComAPI_putRequestMsgQ(comId, pData, dataLength, noOfResponses,
                                    responseTimeout, pCallerQueue->queue, 
                                    pCallerRef, topoCnt, 
                                    pDestURI, pSrcURI);
   }
   else
   {
      res =(int)IPT_INVALID_PAR;
   }
   
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the call-back function.
          Number of expected replies unknown.
          Default time-out for receiving replies.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg(
   UINT32          comId,       /* ComID */
   const char      *pData,      /* Pointer to buffer with data to be send */
   UINT32          dataLength,  /* Number of bytes to be send */
   IPT_REC_FUNCPTR func,        /* Callback function */
   const void      *pCallerRef) /* Caller reference */
{
   int res;

   res = MDComAPI_putRequestMsgF(comId, pData, dataLength, 0,
                                 0, func, pCallerRef, 0, 
                                 (char *)NULL,(char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the call-back function.
          Number of expected replies can be set in the call.
          Time-out for receiving replies can be set in the call.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg(
   UINT32          comId,           /* ComID */
   const char      *pData,          /* Pointer to buffer with data to be send */
   UINT32          dataLength,      /* Number of bytes to be send */
   UINT16          noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32          responseTimeout, /* Time-out value  in milliseconds for 
                                       receiving replies. 0=default value */
   IPT_REC_FUNCPTR func,            /* Callback function */
   const void      *pCallerRef)     /* Caller reference */
{
   int res;

   res = MDComAPI_putRequestMsgF(comId, pData, dataLength, noOfResponses,
                                 responseTimeout, func, pCallerRef, 0,
                                 (char *)NULL, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the call-back function.
          Number of expected replies can be set in the call.
          Time-out for receiving replies can be set in the call.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg(
   UINT32          comId,           /* ComID */
   const char      *pData,          /* Pointer to buffer with data to be send */
   UINT32          dataLength,      /* Number of bytes to be send */
   UINT16          noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32          responseTimeout, /* Time-out value  in milliseconds for 
                                       receiving replies. 0=default value */
   IPT_REC_FUNCPTR func,            /* Callback function */
   const void      *pCallerRef,     /* Caller reference */
   UINT32          topoCnt)         /* Topo counter */
{
   int res;

   res = MDComAPI_putRequestMsgF(comId, pData, dataLength, noOfResponses,
                                 responseTimeout, func, pCallerRef, topoCnt,
                                 (char *)NULL, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the call-back function.
          Number of expected replies can be set in the call.
          Time-out for receiving replies can be set in the call.
          The destination URI for the comId is override by the caller URI
          string.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg(
   UINT32          comId,           /* ComID */
   const char      *pData,          /* Pointer to buffer with data to be send */
   UINT32          dataLength,      /* Number of bytes to be send */
   UINT16          noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32          responseTimeout, /* Time-out value  in milliseconds for 
                                       receiving replies. 0=default value */
   IPT_REC_FUNCPTR func,            /* Callback function */
   const void      *pCallerRef,     /* Caller reference */
   UINT32          topoCnt,         /* Topo counter */
   const char      *pDestURI)       /* Pointer to overriding destination URI string */
{
   int res;

   res = MDComAPI_putRequestMsgF(comId, pData, dataLength, noOfResponses,
                                 responseTimeout, func, pCallerRef, topoCnt,
                                 pDestURI, (char *)NULL);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putRequestMsg
ABSTRACT: Send a message with expected reply/ies from the receiving 
          application(s). Replies and/or communication result reported back 
          to the caller via the call-back function.
          Number of expected replies can be set in the call.
          Time-out for receiving replies can be set in the call.
          The destination  and source URI for the comId is override by the 
          caller URI strings.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRequestMsg( 
   UINT32          comId,           /* ComID */
   const char      *pData,          /* Pointer to buffer with data to be send */
   UINT32          dataLength,      /* Number of bytes to be send */
   UINT16          noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32          responseTimeout, /* Time-out value  in milliseconds for 
                                       receiving replies. 0=default value */
   IPT_REC_FUNCPTR func,            /* Callback function */
   const void      *pCallerRef,     /* Caller reference */
   UINT32          topoCnt,         /* Topo counter */
   const char      *pDestURI,       /* Pointer to overriding destination URI
                                       string */
   const char      *pSrcURI)        /* Pointer to overriding source URI string */
{
   int res;

   res = MDComAPI_putRequestMsgF(comId, pData, dataLength, noOfResponses,
                                 responseTimeout, func, pCallerRef, topoCnt, 
                                 pDestURI, pSrcURI);
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

#endif
/*******************************************************************************
NAME:     MDComAPI::putRespMsg
ABSTRACT: Send a response message.
          Defining destination URI
          Overriding source URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putRespMsg(
   UINT32     comId,      /* ComID */
   UINT16     userStatus, /* The value is transported to the requesting 
                             application */
   const char *pData,     /* Pointer to buffer with data to be send */
   UINT32     dataLength, /* Number of bytes to be send */
   UINT32     sessionID,  /* Session ID, has to be the same as in the received
                             request */                             
   const MDQueue *pCallerQueue,   /* Caller queue */
   IPT_REC_FUNCPTR func,   /* Pointer to callback function.
                              0 = No result will be reported back */
   const void *pCallerRef, /* Caller reference */
   UINT32     topoCnt,    /* Topo counter */
   UINT32     destIpAddr, /* IP address, has to be the same as in the received
                             request if pDestURI = 0 */
   UINT32     destId,      /* Destination URI ID */
   const char *pDestURI,  /* Pointer to destination URI string.
                             <> 0 Get destination IP address from IPTDir and not
                             from the parameter destIpAddr. Override dest.
                             URI string defeined for the comID
                             0 = use parameter destIpAddr and URI string defined 
                             for the comID */
   const char *pSrcURI)   /* Pointer to overriding source URI string.
                             0 = use URI string defined for the comID */
{
   int res;

   res = MDComAPI_putRespMsg(comId, userStatus, pData, dataLength, sessionID,
                             (pCallerQueue?pCallerQueue->queue:0), func, pCallerRef, topoCnt,
                             destIpAddr, destId, pDestURI, pSrcURI );
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsg
ABSTRACT: Send a response message.
          Using the IP address from the received request
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsg(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   UINT32       topoCnt,    /* Topo counter */
   UINT32       destIpAddr) /* IP address, has to be the same as in the received
                               request */
{
   int res;

   res = MDComAPI_putResponseMsg(comId, userStatus, pData, dataLength, sessionID, 
                               topoCnt, destIpAddr,
                               0, 0 );
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsg
ABSTRACT: Send a response message.
          Defining destination URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsg(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   UINT32       topoCnt,    /* Topo counter */
   const char   *pDestURI)  /* Pointer to destination URI string */
{
   int res;

   res = MDComAPI_putResponseMsg(comId, userStatus, pData, dataLength, sessionID,
                                 topoCnt, 0,
                                 pDestURI, 0 );
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsg
ABSTRACT: Send a response message.
          Using the IP address from the received request
          Overriding source URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsg(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   UINT32       topoCnt,    /* Topo counter */
   UINT32       destIpAddr, /* IP address, has to be the same as in the received
                               request */
   const char   *pSrcURI)   /* Pointer to overriding source URI string */
{
   int res;

   res = MDComAPI_putResponseMsg(comId, userStatus, pData, dataLength, sessionID,
                                 topoCnt, destIpAddr,
                                 0, pSrcURI );
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsg
ABSTRACT: Send a response message.
          Defining destination URI
          Overriding source URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsg(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   UINT32       topoCnt,    /* Topo counter */
   const char   *pDestURI,  /* Pointer to destination URI string */
   const char   *pSrcURI)   /* Pointer to overriding source URI string */
{
   int res;

   res = MDComAPI_putResponseMsg(comId, userStatus, pData, dataLength, sessionID,
                                 topoCnt, 0,
                                 pDestURI, pSrcURI );
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsgQ
ABSTRACT: Send a response message.
          Using the IP address from the received request
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsgQ(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   const MDQueue *pCallerQueue,   /* Caller queue */
   const void    *pCallerRef,     /* Caller reference */
   UINT32       topoCnt,    /* Topo counter */
   UINT32       destIpAddr) /* IP address, has to be the same as in the received
                               request */
{
   int res;

   if (pCallerQueue != NULL)
   {
      res = MDComAPI_putResponseMsgQ(comId, userStatus, pData, dataLength, sessionID, 
                                  pCallerQueue->queue, pCallerRef, topoCnt, destIpAddr,
                                  0, 0 );
   }
   else
   {
      res =(int)IPT_INVALID_PAR;
   }
   
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsgQ
ABSTRACT: Send a response message.
          Defining destination URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsgQ(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   const MDQueue *pCallerQueue,   /* Caller queue */
   const void    *pCallerRef,     /* Caller reference */
   UINT32       topoCnt,    /* Topo counter */
   const char   *pDestURI)  /* Pointer to destination URI string */
{
   int res;

   if (pCallerQueue != NULL)
   {
      res = MDComAPI_putResponseMsgQ(comId, userStatus, pData, dataLength, sessionID,
                                    pCallerQueue->queue, pCallerRef, topoCnt, 0,
                                    pDestURI, 0 );
   }
   else
   {
      res =(int)IPT_INVALID_PAR;
   }

   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsgQ
ABSTRACT: Send a response message.
          Using the IP address from the received request
          Overriding source URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsgQ(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   const MDQueue *pCallerQueue,   /* Caller queue */
   const void    *pCallerRef,     /* Caller reference */
   UINT32       topoCnt,    /* Topo counter */
   UINT32       destIpAddr, /* IP address, has to be the same as in the received
                               request */
   const char   *pSrcURI)   /* Pointer to overriding source URI string */
{
   int res;

   if (pCallerQueue != NULL)
   {
      res = MDComAPI_putResponseMsgQ(comId, userStatus, pData, dataLength, sessionID,
                                    pCallerQueue->queue, pCallerRef, topoCnt, destIpAddr,
                                    0, pSrcURI );
   }
   else
   {
      res =(int)IPT_INVALID_PAR;
   }
   
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsgQ
ABSTRACT: Send a response message.
          Defining destination URI
          Overriding source URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsgQ(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   const MDQueue *pCallerQueue,   /* Caller queue */
   const void    *pCallerRef,     /* Caller reference */
   UINT32       topoCnt,    /* Topo counter */
   const char   *pDestURI,  /* Pointer to destination URI string */
   const char   *pSrcURI)   /* Pointer to overriding source URI string */
{
   int res;

   if (pCallerQueue != NULL)
   {
      res = MDComAPI_putResponseMsgQ(comId, userStatus, pData, dataLength, sessionID,
                                    pCallerQueue->queue, pCallerRef, topoCnt, 0,
                                    pDestURI, pSrcURI );
   }
   else
   {
      res =(int)IPT_INVALID_PAR;
   }
   
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI::putResponseMsgF
ABSTRACT: Send a response message.
          Using the IP address from the received request
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsgF(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   IPT_REC_FUNCPTR func,            /* Callback function */
   const void    *pCallerRef,     /* Caller reference */
   UINT32       topoCnt,    /* Topo counter */
   UINT32       destIpAddr) /* IP address, has to be the same as in the received
                               request */
{
   int res;

   res = MDComAPI_putResponseMsgF(comId, userStatus, pData, dataLength, sessionID, 
                               func, pCallerRef, topoCnt, destIpAddr,
                               0, 0 );
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsgF
ABSTRACT: Send a response message.
          Defining destination URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsgF(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   IPT_REC_FUNCPTR func,            /* Callback function */
   const void    *pCallerRef,     /* Caller reference */
   UINT32       topoCnt,    /* Topo counter */
   const char   *pDestURI)  /* Pointer to destination URI string */
{
   int res;

   res = MDComAPI_putResponseMsgF(comId, userStatus, pData, dataLength, sessionID,
                                 func, pCallerRef, topoCnt, 0,
                                 pDestURI, 0 );
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsgF
ABSTRACT: Send a response message.
          Using the IP address from the received request
          Overriding source URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsgF(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   IPT_REC_FUNCPTR func,            /* Callback function */
   const void    *pCallerRef,     /* Caller reference */
   UINT32       topoCnt,    /* Topo counter */
   UINT32       destIpAddr, /* IP address, has to be the same as in the received
                               request */
   const char   *pSrcURI)   /* Pointer to overriding source URI string */
{
   int res;

   res = MDComAPI_putResponseMsgF(comId, userStatus, pData, dataLength, sessionID,
                                 func, pCallerRef, topoCnt, destIpAddr,
                                 0, pSrcURI );
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}

/*******************************************************************************
NAME:     MDComAPI::putResponseMsgF
ABSTRACT: Send a response message.
          Defining destination URI
          Overriding source URI
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI::putResponseMsgF(
   UINT32       comId,      /* ComID */
   UINT16       userStatus, /* The value is transported to the requesting
                               application */
   const char   *pData,     /* Pointer to buffer with data to be send */
   UINT32       dataLength, /* Number of bytes to be send */
   UINT32       sessionID,  /* Session ID, has to be the same as in the received
                               request */                        
   IPT_REC_FUNCPTR func,            /* Callback function */
   const void    *pCallerRef,     /* Caller reference */
   UINT32       topoCnt,    /* Topo counter */
   const char   *pDestURI,  /* Pointer to destination URI string */
   const char   *pSrcURI)   /* Pointer to overriding source URI string */
{
   int res;

   res = MDComAPI_putResponseMsgF(comId, userStatus, pData, dataLength, sessionID,
                                 func, pCallerRef, topoCnt, 0,
                                 pDestURI, pSrcURI );
   if isException(res)
		throw MDComAPIException(res, __FILE__, __LINE__ );

   return(res);
}
#endif
