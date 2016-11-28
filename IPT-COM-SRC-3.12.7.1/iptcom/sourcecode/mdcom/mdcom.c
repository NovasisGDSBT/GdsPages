/*******************************************************************************
 *  COPYRIGHT   :  (C) 2006-2014 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     :  IPTrain
 *
 *  MODULE      :  mdcom.c
 *
 *  ABSTRACT    :  Public C methods for MD communication class MDCom
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 * $Id: mdcom.c 36144 2015-03-24 08:47:23Z gweiss $
 *
 *  CR-9382 (Gerhard Weiss, 2014-04-22) Init loop counter so DEST URI field is
 *          inited correctly
 *
 *  CR-3477 (Bernd Loehr, Gerhard Weiss, 2012-04-18)
 *          Findings during TUEV-Assessment here:
 *            Unsafe use of strcpy / account for trailing zero!
 *
 *  CR-432 (Gerhard Weiss, 2010-11-24)
 *          Corrected position of UNUSED Parameter Macros
 *
 *  CR-1356 (Bernd Loehr, 2010-10-08)
 * 			Additional function iptConfigAddDatasetExt to define dataset
 *          dependent un/marshalling. iptMarshallDSInt() used.
 *
 *  Internal (Bernd Loehr, 2010-08-13) 
 * 			Old obsolete CVS history removed
 *
 *
 ******************************************************************************/


/*******************************************************************************
*  INCLUDES */
#include <stdio.h>
#include <string.h>
#include "iptcom.h"
#include "vos.h"
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"
#include "mdcom_priv.h"
#include "netdriver.h"
#include "mdses.h"
#include "mdtrp.h"
#include "vos_priv.h"

/*******************************************************************************
*  DEFINES
*/

/* Maximum number of queue message for sendtask queue */
#define MAX_NO_SEND_QUEUE_MSG 100 /* Send queue size */

#define MD_DATA_MSG     0         /* Data message, i.e. one way */
#define MD_REQ_MSG      1         /* Request message*/
#define MD_RESP_MSG     2         /* Response message */

#define MD_MSG_NO_COM_RES     0   /* No result of communication expected by the application*/
#define MD_FRG_MSG_NO_COM_RES 1   /* FRG msg. No result of communication expected by the application*/
#define MD_MSG_COM_RES        2   /* Result of communication expected by the application */
#define MD_FRG_MSG_COM_RES    3   /* FRG msg. Result of communication expected by the application */
#define MD_MSG_REQUEST        4   /* Send a request and a reply from receiver expected */
#define MD_FRG_MSG_REQUEST    5   /* FRG msg. Send a request and a reply from receiver expected */
#define MD_MSG_RESPONSE       6   /* Response message */
#define MD_MSG_RESPONSE_RES   7   /* Response message. Result of communication expected by the application */

#define OLD_OVERRIDE_SYNTAX 0
#define NEW_OVERRIDE_SYNTAX 1

/* Maximum UDP payload, i.e. 64k - IP header - UDP header */
#define MAX_UDP_PAYLOAD 65507

/*******************************************************************************
*  TYPEDEFS
*/

typedef  struct
{
   long type;  /* Only used for Linux */
   SESSION_INSTANCE *pSeInstance;
   TRANSPORT_INSTANCE *pTrInstance;
} SEND_QUEUE_MSG;

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
NAME:     prepareMsg
ABSTRACT: First preparation of the header and data frames of the message to be
          send. Calculation of total message length
RETURNS:  0 if OK, !=0 if not.
*/
static int prepareMsg(
   UINT32 comId,           /* ComId to be send */
   UINT16 userStatus,      /* The value is transported with response msg */
   UINT32 responseTimeout, /* Time-out value in milliseconds for receiving
                              replies */
   const char *pData,      /* Pointer to application data to be send */
   UINT32 dataLength,      /* Length of application data to be send */
   const char *pSrcURI,    /* Pointer to source URI string */
   const char *pDestURI,   /* Pointer to destination URI string */
   BYTE **ppSendMsg,       /* Pointer to pointer to message frame */
   UINT32 destIpAddr,      /* Destination IP address */
   UINT32 *pMsgLength,     /* Pointer to total message length */
   UINT32 topoCnt
#ifdef TARGET_SIMU
   ,
   const char *pSimUri
#endif
   )         /* Topo counter value */
{
   const char *pIn;
   char *pOut;
   int res;
   BYTE *pTemp = 0;
   UINT8 srcURILength;
   UINT8 destURILength;
   UINT16 headerLength;
   UINT32 srcURIStringLength;
   UINT32 destURIStringLength;
   UINT32 i;
   UINT32 msgDataLength;
   UINT32 datasetId = 0;
   UINT32 timeStamp;
#ifndef TARGET_SIMU
   T_TDC_RESULT tdcRes = (int)IPT_ERROR;
   char srcURI[IPT_MAX_URI_LEN];
   UINT8 topoCntShort = 0;
#endif

   /* Check that there is any data to be sent */
   if ((pData == 0) || (dataLength == 0))
   {
      IPTVosPrint2(IPT_WARN, "Wrong parameters: pData=%#x msgLength=%d\n",
                   pData, dataLength );
      return((int)IPT_INVALID_PAR); 
   }

   if (pSrcURI)
   {
      srcURIStringLength = strlen(pSrcURI);
   }
   else
   {
#ifdef TARGET_SIMU
      pSrcURI = pSimUri;
      srcURIStringLength = strlen(pSrcURI);
#else
      /* Get own URI from TDC  or TDCEmulator*/
      topoCntShort = 0; /* Set to zero to not check the topo counter by TDC */
      tdcRes = iptGetUriHostPart (0x7f000001, srcURI, &topoCntShort);

      if (tdcRes == 0)
      {
         pSrcURI = srcURI;
         srcURIStringLength = strlen(pSrcURI);
      }
      else
      {
         IPTVosPrint1(IPT_WARN,
                      "Failed to get own source URI. iptGetUriHostPart res=%#x\n",
                      tdcRes );
        /* No source URI found or no valid IP-address */
         srcURIStringLength = 0;
      }
#endif
   }

   /* Source URI greater than maximum for the Wire protocol header? */
   if (srcURIStringLength > 4 * 256 - 1)
   {
      IPTVosPrint1(IPT_WARN,
                   "Wrong parameters: Source URI length = %d to long\n",
                   srcURIStringLength );
      return((int)IPT_INVALID_PAR); 
   }
   
   if (pDestURI)
   {
      destURIStringLength  = strlen(pDestURI);
      /* Destination URI greater than maximum for the Wire protocol header? */
      if (destURIStringLength > 4 * 256 - 1)
      {
         IPTVosPrint1(IPT_WARN,
                      "Wrong parameters: Destination URI length = %d to long\n",
                      destURIStringLength );
         return((int)IPT_INVALID_PAR); 
      }
   }
   else
   {
      destURIStringLength  = 0;
   }

   /* Calculate URI string length in 32 bit word with at least 1 position for
      string termination */
   srcURILength = (UINT8)((srcURIStringLength+1+3)/4);
   destURILength = (UINT8)((destURIStringLength+1+3)/4);

   /* Calculate header length in 32 bit word and data length in bytes */
   headerLength = MIN_MD_HEADER_SIZE + 4*srcURILength + 4*destURILength;
  
   /* Get dataset  and marshall */
   res = iptConfigGetDatasetId(comId, &datasetId);
   if (res == (int)IPT_OK)
   {
      if (datasetId != 0)
      {
         /* Allocate memory for marshalled data. The length for unpacked data
            is used as the size of the marshalled and packed data is unknown */
         pTemp = IPTVosMalloc(dataLength);

         if (pTemp == 0)
         {
            IPTVosPrint1(IPT_ERR, "prepareMsg: Out of memory. Requested size=%d\n",
                         dataLength);
            return((int)IPT_MEM_ERROR);
         }

			/*	Honor conditional marshalling	*/
         res = iptMarshallDSInt(1, datasetId, (const BYTE *)pData, pTemp, &dataLength);
         if (res != (int)IPT_OK)
         {
            (void)IPTVosFree(pTemp);
            return(res);
         }
      }
   }
   else if (res == (int)IPT_TDC_NOT_READY)
   {
      IPTVosPrint1(IPT_WARN, "prepareMsg IPTCom config waiting for TDC to be ready comId=%d\n",comId);
      return(res);
   }

   /* Calculate data size including checksum */
   msgDataLength = iptCalcSendBufferSize(dataLength);

   if (headerLength + msgDataLength > MAX_UDP_PAYLOAD)
   {
      IPTVosPrint2(IPT_ERR, "prepareMsg too big data size=%d -> UDP payload = %d\n",
                   dataLength, headerLength + msgDataLength);
      
      if (pTemp)
      {
         (void)IPTVosFree(pTemp);
      }
      
      return((int)IPT_ILLEGAL_SIZE);
   }
 
   *ppSendMsg = IPTVosMalloc(headerLength + msgDataLength);
   if (*ppSendMsg == 0)
   {
      
      IPTVosPrint1(IPT_ERR, "prepareMsg: Out of memory. Requested size=%d\n",
                   headerLength + msgDataLength);
      
      if (pTemp)
      {
         (void)IPTVosFree(pTemp);
      }
      
      return((int)IPT_MEM_ERROR);
   }

   /* time  stamp */
   timeStamp = IPTVosGetMicroSecTimer();
   *((UINT32*)(*ppSendMsg + TIMESTAMP_OFF)) = TOWIRE32(timeStamp);/*lint !e826  Ignore casting warning */

   /* protocol version  */
   *((UINT32*)(*ppSendMsg + PROT_VER_OFF)) = TOWIRE32(MD_PROTOCOL_VERSION);/*lint !e826  Ignore casting warning */
   
   /* topo  counter  */
   if(isOwnConsistAddr(destIpAddr))
   {
      *((UINT32*)(*ppSendMsg + TOPO_COUNT_OFF)) = 0;/*lint !e826  Ignore casting warning */
   }
   else
   {
      if (topoCnt != 0)
      {
         *((UINT32*)(*ppSendMsg + TOPO_COUNT_OFF)) = TOWIRE32(topoCnt);/*lint !e826  Ignore casting warning */
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "prepareMsg: Current topo counter is zero. No ETB message can be send\n");
      
         if (pTemp)
         {
            (void)IPTVosFree(pTemp);
         }

         (void)IPTVosFree(*ppSendMsg);
         *ppSendMsg = NULL;
         *pMsgLength = 0;
      
         return((int)IPT_TDC_NOT_READY);
      }
   }
   
   /* comId */
   *((UINT32*)(*ppSendMsg + COMID_OFF)) = TOWIRE32(comId);/*lint !e826  Ignore casting warning */

   /* Message type   Filled in later   by checkSendQueue */

   /* Dataset Length in 8 bit words */
   *((UINT16*)(*ppSendMsg + DATA_LENGTH_OFF)) = TOWIRE16(dataLength);/*lint !e826  Ignore casting warning */

   /* User Status */
   *((UINT16*)(*ppSendMsg + USER_STATUS_OFF)) = TOWIRE16(userStatus);/*lint !e826  Ignore casting warning */

   /* Header length in  8 bit words */
   *((UINT16*)(*ppSendMsg + HEAD_LENGTH_OFF)) = TOWIRE16(headerLength - FCS_SIZE);/*lint !e826  Ignore casting warning */

   /* Source URI length */
   *((UINT8*)(*ppSendMsg + SRC_URI_LENGTH_OFF)) = srcURILength;

   /* Destination URI length */
   *((UINT8*)(*ppSendMsg + DEST_URI_LENGTH_OFF)) = destURILength;

   /* Index */
   /* TODO when large message shall  be implemented */
   *((INT16*)(*ppSendMsg + INDEX_OFF)) = TOWIRE16(-1);/*lint !e826  Ignore casting warning */

   /* Sequence number filled in later by transport layer */

   /* Total length   of a unsplit message (dataset) in blocks of 1024 bytes */
   *((UINT16*)(*ppSendMsg + MSG_LENGTH_OFF)) =  TOWIRE16((dataLength + 1023)/1024);/*lint !e826  Ignore casting warning */

   /* Session ID, may be changed by the session layer */
   *((UINT32*)(*ppSendMsg + SESSION_ID_OFF)) =  0;/*lint !e826  Ignore casting warning */

   /* Source URI string */
   pOut = (char *)(*ppSendMsg + SRC_URI_OFF);
   i =   0;
   if (pSrcURI)
   {
      pIn   =  pSrcURI;
      while (i < srcURIStringLength)
      {
         *pOut++  = *pIn++;
         i++;
      }
   }

   /* Terminate the string with zeroes */
   while (i < (UINT32)4*srcURILength)
   {
      *pOut++  = 0;
      i++;
   }

   /* Destination URI string */
   i = 0;   /* CR 9382: Init i to zero for all cases */
   if (pDestURI)
   {
      pIn   =  pDestURI;
      while (i < destURIStringLength)
      {
         *pOut++  = *pIn++;
         i++;
      }
   }
   
   /* Terminate the string with zeroes */
   while (i < (UINT32)4*destURILength)
   {
      *pOut++  = 0;
      i++;
   }

   /* Response timeout */
   *((UINT32 *)(pOut))  = TOWIRE32(responseTimeout);/*lint !e826  Ignore casting warning */

   /* Destination IP address */
   *((UINT32 *)(pOut + RESPONSE_TIMEOUT_SIZE))  = TOWIRE32(destIpAddr);/*lint !e826  Ignore casting warning */

   /* Header checksum calculated later when the header is complete */

   if ((datasetId != 0) && (pTemp))
   {
      /* Copy  the   data and calculate data checksum */
      res = iptLoadSendData(pTemp, dataLength,
                            *ppSendMsg +  headerLength,
                            &msgDataLength);
      (void)IPTVosFree(pTemp);
   }
   else
   {
      /* Copy  the   data and calculate data checksum */
      res = iptLoadSendData((unsigned char *)pData, dataLength,
                            *ppSendMsg +  headerLength,
                            &msgDataLength);
   }
                             
   if (res == (int)IPT_OK)
   {
      *pMsgLength = headerLength + msgDataLength;
   }
   else
   {
      (void)IPTVosFree(*ppSendMsg);
      *ppSendMsg = NULL;
      *pMsgLength = 0;
   }
   return(res);
}

/*******************************************************************************
NAME:     createComInstances
ABSTRACT: Called when a new message shall be send.
          Creates a transport layer instance and in some cases a session layer
          instance.
          Finally is a message send on the sender task queue
RETURNS:  0 if OK, !=0 if not.
*/
static int   createComInstances(
   MD_QUEUE callerQueueId,     /* Caller queue ID */
   IPT_REC_FUNCPTR callerFunc, /* Pointer to callback function */
   const void *pCallerRef,     /* Caller reference */
   MD_COM_PAR  comPar,         /* Communication parameters */
   UINT16 msgType,             /* Type of message to be send */
   UINT32 sessionId,           /* Session ID for response message */
   UINT32 comId,               /* ComID */
   UINT32 msgLength,           /* Length of the message */
   UINT16 noOfResponses,       /* Number of expected responses */
   BYTE *pSendMsg )          /* Pointer to the message */
{
   int   res   = (int)IPT_OK;
   int   res2;
   SEND_QUEUE_MSG newMsg;

   newMsg.pSeInstance = 0;
   newMsg.pTrInstance = 0;

   switch (msgType)
   {
      case MD_MSG_NO_COM_RES:
         /* Multicast ? */
         if (isMulticastIpAddr(comPar.destIpAddr))
         {
            /* Set send message  type */
            *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_NOACK_MSG);/*lint !e826  Ignore casting warning */

            res = createTrInstance(MULTICAST_TRANSPORT_TYPE,
                                   pCallerRef,
                                   0,
                                   0,
                                   comId,
                                   msgLength,
                                   (char *)pSendMsg,
                                   comPar,
                                   &newMsg.pTrInstance);
         }
         else
         {
            /* Set send message  type */
            *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_ACK_MSG);/*lint !e826  Ignore casting warning */

            res = createTrInstance(UNICAST_OR_FRG_NO_REPORT_TRANSPORT_TYPE,
                                   pCallerRef,
                                   0,
                                   0,
                                   comId,
                                   msgLength,
                                   (char *)pSendMsg,
                                   comPar,
                                   &newMsg.pTrInstance);
         }
         break;

      case MD_FRG_MSG_NO_COM_RES:
         /* Set send message  type */
         *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_FRG_MSG);/*lint !e826  Ignore casting warning */

         res = createTrInstance(UNICAST_OR_FRG_NO_REPORT_TRANSPORT_TYPE,
                                pCallerRef,
                                0,
                                0,
                                comId,
                                msgLength,
                                (char *)pSendMsg,
                                comPar,
                                &newMsg.pTrInstance);
         break;

      case MD_MSG_COM_RES:
         /* Multicast ? */
         if (isMulticastIpAddr(comPar.destIpAddr))
         {
            res = (int)IPT_INVALID_COMPAR;
            IPTVosPrint0(IPT_ERR,
            "ERROR Multicast address for sending with communication result expected\n");
         }
         else
         {
            /* Set send message  type */
            *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_ACK_MSG);/*lint !e826  Ignore casting warning */

            res = createTrInstance(UNICAST_OR_FRG_REPORT_TRANSPORT_TYPE,
                                   pCallerRef,
                                   callerQueueId,
                                   callerFunc,
                                   comId,
                                   msgLength,
                                   (char *)pSendMsg,
                                   comPar,
                                   &newMsg.pTrInstance);
         }
         break;

      case MD_FRG_MSG_COM_RES:
         /* Set send message  type */
        *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_FRG_MSG);/*lint !e826  Ignore casting warning */

         res = createTrInstance(UNICAST_OR_FRG_REPORT_TRANSPORT_TYPE,
                                pCallerRef,
                                callerQueueId,
                                callerFunc,
                                comId,
                                msgLength,
                                (char *)pSendMsg,
                                comPar,
                                &newMsg.pTrInstance);
         break;

      case MD_MSG_REQUEST:
         /* Multicast ? */
         if (isMulticastIpAddr(comPar.destIpAddr))
         {
            res = createSeInstance(MULTICAST_SESSION_TYPE,
                                   noOfResponses,
                                   (char *)pSendMsg,
                                   pCallerRef,
                                   callerQueueId,
                                   callerFunc,
                                   comId,
                                   comPar,
                                   &newMsg.pSeInstance);
            if (res == (int)IPT_OK)
            {
               /* Set send message  type */
               *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_REQ_NOACK_MSG);/*lint !e826  Ignore casting warning */

               res = createTrInstance(MULTICAST_TRANSPORT_TYPE,
                                      pCallerRef,
                                      0,
                                      0,
                                      comId,
                                      msgLength,
                                      (char *)pSendMsg,
                                      comPar,
                                      &newMsg.pTrInstance);
            }
         }
         else
         {
            res = createSeInstance(UNICAST_SESSION_TYPE,
                                   noOfResponses,
                                   (char *)pSendMsg,
                                   pCallerRef,
                                   callerQueueId,
                                   callerFunc,
                                   comId,
                                   comPar,
                                   &newMsg.pSeInstance);
            if (res == (int)IPT_OK)
            {
               /* Set send message  type */
               *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_REQ_ACK_MSG);/*lint !e826  Ignore casting warning */

                res = createSeTrInstance(UNICAST_OR_FRG_REQUEST_TRANSPORT_TYPE,
                                         pCallerRef,
                                         newMsg.pSeInstance,
                                         comId,
                                         msgLength,
                                         (char *)pSendMsg,
                                         comPar,
                                         &newMsg.pTrInstance);
            }
         }
         break;

      case MD_FRG_MSG_REQUEST:
         res = createSeInstance(UNICAST_SESSION_TYPE,
                                noOfResponses,
                                (char *)pSendMsg,
                                pCallerRef,
                                callerQueueId,
                                callerFunc,
                                comId,
                                comPar,
                                &newMsg.pSeInstance);
         if (res == (int)IPT_OK)
         {
            /* Set send message  type */
            *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_REQ_FRG_MSG);/*lint !e826  Ignore casting warning */

             res = createSeTrInstance(UNICAST_OR_FRG_REQUEST_TRANSPORT_TYPE,
                                      pCallerRef,
                                      newMsg.pSeInstance,
                                      comId,
                                      msgLength,
                                      (char *)pSendMsg,
                                      comPar,
                                      &newMsg.pTrInstance);
         }
         break;

      case MD_MSG_RESPONSE:
         /* Multicast ? */
         if (isMulticastIpAddr(comPar.destIpAddr))
         {
            IPTVosPrint0(IPT_WARN,
                  "ERROR Multicast address for response message\n");
            res   = (int)IPT_INVALID_COMPAR;
         }
         else
         {
            /* Set send message  type */
            *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_REPLY_MSG);/*lint !e826  Ignore casting warning */
            
            /* Session ID */
            *((UINT32*)(pSendMsg + SESSION_ID_OFF)) =  TOWIRE32(sessionId);/*lint !e826  Ignore casting warning */

            res = createTrInstance(UNICAST_OR_FRG_NO_REPORT_TRANSPORT_TYPE,
                                   pCallerRef,
                                   0,
                                   0,
                                   comId,
                                   msgLength,
                                   (char *)pSendMsg,
                                   comPar,
                                   &newMsg.pTrInstance);
         }
         break;

      case MD_MSG_RESPONSE_RES:
         /* Multicast ? */
         if (isMulticastIpAddr(comPar.destIpAddr))
         {
            IPTVosPrint0(IPT_WARN,
                  "ERROR Multicast address for response message\n");
            res   = (int)IPT_INVALID_COMPAR;
         }
         else
         {
            /* Set send message  type */
            *((UINT16*)(pSendMsg + TYPE_OFF)) = TOWIRE16(MD_REPLY_MSG);/*lint !e826  Ignore casting warning */
            
            /* Session ID */
            *((UINT32*)(pSendMsg + SESSION_ID_OFF)) =  TOWIRE32(sessionId);/*lint !e826  Ignore casting warning */

            res = createTrInstance(UNICAST_OR_FRG_REPORT_TRANSPORT_TYPE,
                                   pCallerRef,
                                   callerQueueId,
                                   callerFunc,
                                   comId,
                                   msgLength,
                                   (char *)pSendMsg,
                                   comPar,
                                   &newMsg.pTrInstance);
         }
         break;

      default:
         IPTVosPrint0(IPT_WARN, "Illegal message type\n");
         res   = (int)IPT_ERROR;
         
         break;
   }

   if (res == (int)IPT_OK)
   {
      /* send  queue message to sender task */
      res = IPTVosSendMsgQueue(&IPTGLOBAL(md.mdSendQueueId), (char *)&newMsg,
                               sizeof(SEND_QUEUE_MSG));
      if (res != (int)IPT_OK)
      {
         IPTVosPrint1(IPT_ERR, "ERROR sending queue message to MD send thread. ComId=%d\n", comId);
      }
   }
  
   if (res != (int)IPT_OK)
   {
      if (pSendMsg)
      {
         /* deallocate send message buffer */
         res2 = IPTVosFree(pSendMsg);
         if(res2 != 0)
         {
           IPTVosPrint1(IPT_ERR,
                    "createComInstances failed to free data memory, code=%#x\n",
                        res2);
         }
      }

      if (newMsg.pTrInstance)
      {
         res2 = IPTVosFree((BYTE *)newMsg.pTrInstance);
         if(res2 != 0)
         {
           IPTVosPrint1(IPT_ERR,
                    "createComInstances failed to free data memory, code=%#x\n",
                        res2);
         }
      }
  
      if (newMsg.pSeInstance)
      {
         res2 = IPTVosFree((BYTE *)newMsg.pSeInstance);
         if(res2 != 0)
         {
           IPTVosPrint1(IPT_ERR,
                    "createComInstances failed to free data memory, code=%#x\n",
                        res2);
         }
      }
   }
  
   return(res);
}

/*******************************************************************************
NAME:     checkSendQueue
ABSTRACT: Check the sendtask queue for new transport and session instances
          created for new messages. The instances are put into respectively
          linked list of active instances.
RETURNS:  -
*/
static void checkSendQueue(void)
{
   int res;
   SEND_QUEUE_MSG newMsg;

   /* get new message from send queue */
   while (IPTVosReceiveMsgQueue(&IPTGLOBAL(md.mdSendQueueId),
                                (char*)&newMsg,
                                sizeof(SEND_QUEUE_MSG),
                                IPT_NO_WAIT) > 0)
   {
      if (newMsg.pTrInstance)
      {
         insertTrInstance(newMsg.pTrInstance);

         if (newMsg.pSeInstance)
         {
            insertSeInstance(newMsg.pSeInstance);
         }
      }
      else if  (newMsg.pSeInstance)
      {
         IPTVosPrint0(IPT_ERR,
                     "ERROR Transport instance missing for Session instance\n");
         res = IPTVosFree((BYTE *)newMsg.pSeInstance);
         if(res != 0)
         {
           IPTVosPrint1(IPT_ERR,
                        "checkSendQueue failed to free data memory, code=%#x\n",
                        res);
         }
      }
   }
}

/*******************************************************************************
NAME:     getComIdPar
ABSTRACT: Get communication parameter for a given comID
RETURNS:  0 if OK, !=0 if not
*/
static int getComIdPar(
    UINT32 comId,     /* IN: comID */
    MD_COM_PAR *pComPar, /* OUT: pointer to communication parameter structure */
    const char **ppSrcURI,  /* OUT: pointer to pointer to source URI string */
    const char **ppDestURI) /* OUT: pointer to pointer to destination URI string */
{
   int ret;
   IPT_CONFIG_EXCHG_PAR_EXT exchgPar;
   IPT_CONFIG_COM_PAR_EXT configComPar;

   ret = iptConfigGetExchgPar(comId, &exchgPar);
   if (ret == (int)IPT_OK)
   {
      pComPar->ackTimeOutVal = exchgPar.mdSendPar.ackTimeOut;
      pComPar->maxResend = MAX_RESEND;
      pComPar->replyTimeOutVal = exchgPar.mdSendPar.responseTimeOut;
      *ppSrcURI = exchgPar.mdSendPar.pSourceURI;
      *ppDestURI = exchgPar.mdSendPar.pDestinationURI ;
   
      if (exchgPar.comParId != 0)
      {
         ret = iptConfigGetComPar(exchgPar.comParId, &configComPar);
         if ( ret == (int)IPT_OK)
         {
            pComPar->mdSendSocket = configComPar.mdSendSocket;
         }
      }
      else
      {
         ret = iptConfigGetComPar(IPT_DEF_COMPAR_MD_ID, &configComPar);
         if (ret == (int)IPT_OK)
         {
            pComPar->mdSendSocket = configComPar.mdSendSocket;
         }
      }

      /* Check acknowledge time-out value */
      if (pComPar->ackTimeOutVal == 0)
      {
         pComPar->ackTimeOutVal = IPTGLOBAL(md.defAckTimeOut);    
      }
      
      /* Check reply time-out value */
      if (pComPar->replyTimeOutVal ==  0)
      {
         pComPar->replyTimeOutVal = IPTGLOBAL(md.defResponseTimeOut);   
      }
   }
   else if(ret == (int)IPT_NOT_FOUND)
   {
      pComPar->destIpAddr = 0;
      pComPar->ackTimeOutVal = IPTGLOBAL(md.defAckTimeOut);
      pComPar->maxResend = MAX_RESEND;
      pComPar->replyTimeOutVal = IPTGLOBAL(md.defResponseTimeOut);
      *ppSrcURI = 0;
      *ppDestURI = 0;

      ret = iptConfigGetComPar(IPT_DEF_COMPAR_MD_ID, &configComPar);
      if (ret == (int)IPT_OK)
      {
         pComPar->mdSendSocket = configComPar.mdSendSocket;
      }
   }
   else if (ret == (int)IPT_TDC_NOT_READY)
   {
      IPTVosPrint0(IPT_WARN,
                   "IPTCom configuration waiting for TDC to be ready\n");
   }

   if (ret == IPT_OK)
   {
      if (pComPar->mdSendSocket == 0)
      {
         IPTVosPrint0(IPT_WARN,
                      "IPTCom waiting for ethernet to be initiated\n");
         ret = IPT_ERR_NO_ETH_IF;
      }
   }

   return(ret);
}

/*******************************************************************************
NAME:     overrideUri
ABSTRACT: Override the user part of the URI if it exist 
RETURNS:  0 if OK, !=0 if not
*/
static int overrideUri(
   const char *pOverridURI,  /* Pointer to override URI string */
   const char **ppOutputURI, /* Pointer to pointer to origin and to output URI string */
   char       *pUri,         /* Pointer to URI string array to be used for a new URI */
   int        maxLen )       /* Length of the array poite to by pUri */  
{
   const char *pUriHost;
   const char *pOriginUri = *ppOutputURI;
   const char *pTemp;
   int ret = IPT_OK;
   int len;

   len = strlen(pOverridURI);
   if (len == 0)
   {
      IPTVosPrint0(IPT_ERR, "Override URI zero length\n");
      return((int)IPT_INVALID_PAR);
   }
   if (len > MD_URILEN+1)
   {
      IPTVosPrint0(IPT_ERR, "Override URI to long\n");
      return((int)IPT_INVALID_PAR);
   }
   
   /* Ignore any ipt:// */
   pTemp = strrchr(pOverridURI,'/');
   if (pTemp)
   {
      if (len > pTemp + 1 - pOverridURI)
      {
         pTemp = pTemp + 1;
         len = strlen(pTemp); 
      }
      else
      {
         return((int)IPT_INVALID_PAR);
      }
   }
   else
   {
      pTemp = pOverridURI;
   }

   /* Search for host part in the override URI*/
   pUriHost = strchr(pTemp,'@');
   if (pUriHost)
   {
      /* Any user part in override URI? */
      if (pUriHost > pTemp)
      {
        /* Use the complete override URI, i.e. both user and host part */
        *ppOutputURI = pOverridURI;
      }
      else /* No user part in the override URI */
      {
         /* Any origin URI? */
         if (pOriginUri)
         {
            /* Search for '@' in the origin URI */
            pTemp = strchr(pOriginUri,'@');

            /* Any user part in the origin URI? */
            if ((pTemp) && (pTemp > pOriginUri))
            {
               if (pTemp - pOriginUri > 2*IPT_MAX_LABEL_LEN)
               {
                  IPTVosPrint0(IPT_ERR, "User URI to long\n");
                  ret = (int)IPT_INVALID_PAR;
               }
               else if ((pTemp - pOriginUri) + len + 1 > maxLen)
               {
                  IPTVosPrint0(IPT_ERR, "Output buffer to small\n");
                  ret = (int)IPT_INVALID_PAR;
               }
               else
               {
                  /* Copy user part of the origin URI */
                  memcpy(pUri, pOriginUri, pTemp - pOriginUri);
            
                  /* Copy host part of the override URI */
                  memcpy(pUri + (pTemp - pOriginUri), pUriHost, len);
            
                  /* Terminate string */
                  pUri[pTemp - pOriginUri+ len] = 0;

                  /* Use the new URI */
                  *ppOutputURI = pUri;
               }
            }
            else /* No user part in the origin URI or in the override URI */
            {
              /* Use the complete override URI except '@'*/
              *ppOutputURI = pUriHost + 1;
            }
         }
         else
         {
           /* Use the complete override URI except '@'*/
           *ppOutputURI = pUriHost + 1;
         }
      }
   }
   else  /* Ony user part in the override URI */
   {
      if (len + 4*IPT_MAX_LABEL_LEN + 1 > maxLen)
      {
         IPTVosPrint0(IPT_ERR, "Output buffer to small\n");
         ret = (int)IPT_INVALID_PAR;
      }
      else
      {
         /* Copy user part of the override URI */
         strcpy(pUri, pTemp);

         if (pOriginUri)
         {
            /* Search for '@' in origin URI*/
            pTemp = strchr(pOriginUri,'@');

            /* Any user part in origin URI? */
            if (pTemp)
            {
               /* Copy host part of the origin URI */
               strncat(pUri, pTemp, 4*IPT_MAX_LABEL_LEN+1);
            }
            else
            {
               /* Copy host part of the origin URI */
               strcat(pUri, "@");
               strncat(pUri, pOriginUri, 4*IPT_MAX_LABEL_LEN);
            }
         }
         else
         {
            strcat(pUri, "@");
         }

         /* Use the new URI */
         *ppOutputURI = pUri;
      }
   }

   return(ret);
}

/*******************************************************************************
NAME:     putMsg
ABSTRACT: Send a MD message.
RETURNS:  0 if OK, !=0 if not.
*/
int putMsg(
   UINT32     comId,       /* ComID */
   const char *pData,      /* Pointer to buffer with data to be send */
   UINT32     dataLength,  /* Number of bytes to be send */
   UINT16     noOfResponses,  
   UINT32     responseTimeout,
   MD_QUEUE   callerQueue, /* Caller queue ID. 
                              0 = No result will be reported back */
   IPT_REC_FUNCPTR func,   /* Pointer to callback function.
                              0 = No result will be reported back */
   const void *pCallerRef, /* Caller reference */
   UINT32     topoCnt,     /* Topo counter */
   UINT32     destId,      /* Destination URI ID */
   const char *pDestURI,   /* Pointer to overriding destination URI string.
                              0 = use URI string defined for the comID */
   const char *pSrcURI,    /* Pointer to overriding source URI string.
                              0 = use URI string defined for the comID */
   int        syntax,      /* 0 = old [[user]@]host 1 = new [user][@host] */
   UINT16 msg_Type         /* Type of message to be send */
#ifdef TARGET_SIMU				
   ,         
	const char *pSimUri
#endif
   )        
{
   const char *pSourceURI;
   const char *pDestinationURI = NULL;
   char destURI[MD_URILEN+1];
   char srcURI[MD_URILEN+1];
#ifndef TARGET_SIMU
   T_IPT_URI srcHostURI;
#endif
   int resultCode = (int)IPT_OK;
   BYTE *pSendMsg;
   UINT8  tdcTopoCnt = (UINT8)topoCnt;
	UINT8 dummy;
   UINT32 msgLength;
   UINT16 msgType;
   T_TDC_BOOL frg;
   MD_COM_PAR  comPar;
   T_TDC_RESULT tdcRes;

#ifdef LINUX_MULTIPROC
   if (func != 0)
   {
      IPTVosPrint0(IPT_ERR,
         "putMsg: Call-back function not allowed for Linux multi process\n");
      return((int)IPT_INVALID_PAR);
   }
#endif

   if (IPTGLOBAL(md.mdComInitiated))
   {
      resultCode = getComIdPar(comId,&comPar,&pSourceURI,&pDestinationURI);
      if (resultCode != (int)IPT_OK)
      {
         return(resultCode);
      }

#ifdef TARGET_SIMU				
		if (pSimUri != NULL)
		{
			dummy = 0;
			tdcRes = iptGetAddrByName(pSimUri, &comPar.simDevIpAddr, &dummy);
			if (tdcRes != (int)IPT_OK)
			{
				return(tdcRes);
			}
		}
		else
      {
			comPar.simDevIpAddr = 0;
      }
#endif     
      if (destId != 0)
      {
         resultCode = iptConfigGetDestIdPar(comId, destId, &pDestinationURI);
         if (resultCode != (int)IPT_OK)
         {
            IPTVosPrint2(IPT_ERR,
               "putMsg: Destination ID=%d not configured for ComId=%d\n",
               destId, comId);
            return(resultCode);
         }
      }

      /* Override destination URI? */
      if (pDestURI != NULL)
      {
         if (syntax == NEW_OVERRIDE_SYNTAX)
         {
            resultCode = overrideUri(pDestURI, &pDestinationURI, destURI, sizeof(destURI));
            if (resultCode != (int)IPT_OK)
            {
               return(resultCode);
            }
         }
         else
         {
            pDestinationURI = pDestURI;
         }
      }
      
      /* Override source URI? */
      if (pSrcURI != NULL)
      {
         if (syntax == NEW_OVERRIDE_SYNTAX)
         {
            /* Source URI not configured? */
            if (pSourceURI == NULL)
            {
#ifdef TARGET_SIMU
               pSourceURI = pSimUri;
#else
               /* Get own host URI from TDC  or TDCEmulator*/
			      dummy = 0;
               tdcRes = iptGetUriHostPart (0x7f000001, srcHostURI, &dummy);

               if (tdcRes == 0)
               {
                  pSourceURI = srcHostURI;
               }
               else
               {
                  IPTVosPrint1(IPT_WARN,
                               "Failed to get own source URI. iptGetUriHostPart res=%#x\n",
                               tdcRes );
               }
#endif
            }

            resultCode = overrideUri(pSrcURI, &pSourceURI, srcURI, sizeof(srcURI));
            if (resultCode != (int)IPT_OK)
            {
               return(resultCode);
            }
         }
         else
         {
            pSourceURI = pSrcURI;
         }
      }
      else
      {
         /* Source URI not configured? */
         if (pSourceURI == NULL)
         {
#ifdef TARGET_SIMU
            pSourceURI = pSimUri;
#else
            /* Get own host URI from TDC  or TDCEmulator*/
			   dummy = 0;
            tdcRes = iptGetUriHostPart (0x7f000001, srcHostURI, &dummy);
            if (tdcRes == 0)
            {
               pSourceURI = srcHostURI;
            }
            else
            {
               IPTVosPrint1(IPT_WARN,
                            "Failed to get own source URI. iptGetUriHostPart res=%#x\n",
                            tdcRes );
            }
#endif
         }
      }
    
      if (pDestinationURI != NULL)
      {
         /* Get IP address from IPTDir based on the destination URI */
#ifdef TARGET_SIMU
         if (comPar.simDevIpAddr != 0)
         {
            tdcRes = iptGetAddrByNameExtSim(pDestinationURI, comPar.simDevIpAddr,
                                            &comPar.destIpAddr,
                                            &frg, &tdcTopoCnt);
         }
         else
         {
            tdcRes = iptGetAddrByNameExt(pDestinationURI, &comPar.destIpAddr,
                                         &frg, &tdcTopoCnt);
         }
#else
         tdcRes = iptGetAddrByNameExt(pDestinationURI, &comPar.destIpAddr,
                                      &frg, &tdcTopoCnt);
#endif
         if (tdcRes != TDC_OK)
         {
            if (tdcRes == TDC_WRONG_TOPOCOUNT)
            {
               IPTVosPrint3(IPT_ERR, "Wrong topoCnt value=%d Current value=%d. TDC ERROR=%#x\n",
                            topoCnt, tdcTopoCnt, tdcRes);
            }
            else
            {
               IPTVosPrint2(IPT_ERR, "No IP address for URI = %s. TDC ERROR=%#x\n",
                            pDestinationURI,tdcRes);
            }
            return(tdcRes);
         }
      }
      else
      {
         IPTVosPrint1(IPT_ERR, "No destination URI specified for ComId=%u\n",
                      comId);
         comPar.destIpAddr = 0;
         return((int)IPT_INVALID_COMPAR);
      }
      
      
      if (msg_Type == MD_REQ_MSG)
      {
         if (responseTimeout)
         {
            comPar.replyTimeOutVal = responseTimeout;
         }
      }
      else
      {
         comPar.replyTimeOutVal = 0;
      }

#ifdef TARGET_SIMU
      resultCode = prepareMsg(comId, 0, comPar.replyTimeOutVal, pData, dataLength, pSourceURI,
                              pDestinationURI, &pSendMsg, comPar.destIpAddr,
                              &msgLength, tdcTopoCnt, pSimUri);
#else
      resultCode = prepareMsg(comId, 0, comPar.replyTimeOutVal, pData, dataLength, pSourceURI,
                              pDestinationURI, &pSendMsg, comPar.destIpAddr,
                              &msgLength, tdcTopoCnt);
#endif
     
      if (resultCode == (int)IPT_OK)
      {
         if (msg_Type == MD_DATA_MSG)
         {
            if ((callerQueue != 0) || (func != 0))
            {
               if (frg)
               {
                  msgType = MD_FRG_MSG_COM_RES;
               }
               else
               {
                  msgType = MD_MSG_COM_RES;
               }
            }
            else
            {
               if (frg)
               {
                  msgType = MD_FRG_MSG_NO_COM_RES;
               }
               else
               {
                  msgType = MD_MSG_NO_COM_RES;
               }
            }
            resultCode= createComInstances(callerQueue, func, pCallerRef,
                                           comPar, msgType, 0, comId, msgLength,
                                           0, pSendMsg);
         }
         else
         {
            if (frg)
            {
               msgType = MD_FRG_MSG_REQUEST;
            }
            else
            {
               msgType = MD_MSG_REQUEST;
            }
            resultCode= createComInstances(callerQueue, func, pCallerRef,
                                           comPar, msgType, 0, comId, msgLength,
                                           noOfResponses, pSendMsg);
         }
      }

      return(resultCode);
   }
   else
   {
      return((int)IPT_MD_NOT_INIT);
   }
}

/*******************************************************************************
NAME:     putRespMsg
ABSTRACT: Send a response MD message.
RETURNS:  0 if OK, !=0 if not.
*/
int putRespMsg(
   UINT32     comId,       /* ComID */
   UINT16 userStatus,      /* The value is transported with response msg */
   const char *pData,      /* Pointer to buffer with data to be send */
   UINT32     dataLength,  /* Number of bytes to be send */
   UINT32     sessionId,   /* Session ID for response message */                      
   MD_QUEUE   callerQueue, /* Caller queue ID. 
                              0 = No result will be reported back */
   IPT_REC_FUNCPTR func,   /* Pointer to callback function.
                              0 = No result will be reported back */
   const void *pCallerRef, /* Caller reference */
   UINT32     topoCnt,     /* Topo counter */
   UINT32     destIpAddr,  /* Destination IP address for response message */
   UINT32     destId,      /* Destination URI ID */
   const char *pDestURI,   /* Pointer to overriding destination URI string.
                              0 = use URI string defined for the comID */
   const char *pSrcURI,    /* Pointer to overriding source URI string.
                              0 = use URI string defined for the comID */
   int        syntax       /* 0 = old [[user]@]host 1 = new [user][@host] */
#ifdef TARGET_SIMU				
   ,         
	const char *pSimUri
#endif
   )        
{
   const char *pSourceURI;
   const char *pDestinationURI = NULL;
   const char *pConfiguredDestURI;
   const char *pAt;
   char *pUserUri = NULL;
   char *pHostUri = NULL;
   char destURI[2*IPT_MAX_LABEL_LEN + 6 + MD_URILEN + 1];
   char srcURI[2*IPT_MAX_LABEL_LEN + 6 + MD_URILEN + 1];
#ifndef TARGET_SIMU
   T_IPT_URI srcHostURI;
#endif
   int resultCode = (int)IPT_OK;
   int len;
   BYTE *pSendMsg;
   UINT8  tdcTopoCnt = (UINT8)topoCnt;
	UINT8 dummy = 0;
   UINT32 msgLength;
   UINT16 msgType;
   T_TDC_BOOL frg;
   MD_COM_PAR  comPar;
   T_TDC_RESULT tdcRes;

#ifdef LINUX_MULTIPROC
   if (func != 0)
   {
      IPTVosPrint0(IPT_ERR,
         "putRespMsg: Call-back function not allowed for Linux multi process\n");
      return((int)IPT_INVALID_PAR);
   }
#endif

   if (IPTGLOBAL(md.mdComInitiated))
   {
      resultCode = getComIdPar(comId,&comPar,&pSourceURI,&pConfiguredDestURI);
      if (resultCode != (int)IPT_OK)
      {
         return(resultCode);
      }
      comPar.destIpAddr = 0;
#ifdef TARGET_SIMU				
		if (pSimUri != NULL)
		{
			dummy = 0;
			tdcRes = iptGetAddrByName(pSimUri, &comPar.simDevIpAddr, &dummy);
			if (tdcRes != (int)IPT_OK)
			{
				return(tdcRes);
			}
		}
		else
      {
			comPar.simDevIpAddr = 0;
      }
#endif     
      if (syntax == NEW_OVERRIDE_SYNTAX)
      {
 
         if (destIpAddr)
         {
            comPar.destIpAddr = destIpAddr;
            pDestinationURI = destURI;

            /* Override URI? */
            if (pDestURI)
            {
               /* Any host part in override URI? */
               pAt = strchr(pDestURI,'@');
               if (pAt)
               {
                  /* Any user part in override URI? */
                  if (pAt > pDestURI)
                  {
                     if (pAt - pDestURI > 2*IPT_MAX_LABEL_LEN + 6)
                     {
                        IPTVosPrint0(IPT_ERR,
                           "putRespMsg: user part of override URI to long\n");
                        return((int)IPT_INVALID_PAR);
                     }
                     /* Copy user part of the override URI */
                     memcpy(destURI, pDestURI, pAt - pDestURI+ 1);
                     pHostUri = &destURI[pAt - pDestURI + 1];
                     pUserUri = destURI;
                  }
               }
               else
               {
                  /* Copy user part of the override URI */
                  len = strlen(pDestURI);
                  if (len > 0)
                  {
                     if (len > 2*IPT_MAX_LABEL_LEN + 6)
                     {
                        IPTVosPrint0(IPT_ERR,
                           "putRespMsg: user part of override URI to long\n");
                        return((int)IPT_INVALID_PAR);
                     }
                     strcpy(destURI, pDestURI);
                     destURI[len] = '@';
                     pHostUri = &destURI[len+1];
                     pUserUri = destURI;
                  }
               }
            }
          
            if (pUserUri == NULL)
            {
               if (destId != 0)
               {
                  resultCode = iptConfigGetDestIdPar(comId, destId, &pConfiguredDestURI);
                  if (resultCode != (int)IPT_OK)
                  {
                     IPTVosPrint2(IPT_ERR,
                        "putRespMsg: Destination ID=%d not configured for ComId=%d\n",
                        destId, comId);
                     return(resultCode);
                  }
               }
            
               /* Any configured destination URI? */
               if (pConfiguredDestURI)
               {
                  /* Search for user part in the configured destination URI */
                  /* Any host part in configured URI? */
                  pAt = strchr(pConfiguredDestURI,'@');
                  if (pAt)
                  {
                     /* Any user part in configured URI? */
                     if (pAt > pConfiguredDestURI)
                     {
                        if (pAt - pConfiguredDestURI > 2*IPT_MAX_LABEL_LEN + 6)
                        {
                           IPTVosPrint0(IPT_ERR,
                              "putRespMsg: user part of configured URI to long\n");
                           return((int)IPT_INVALID_PAR);
                        }
                        /* Copy user part of the configured URI */
                        memcpy(destURI, pConfiguredDestURI, pAt - pConfiguredDestURI + 1);
                        pHostUri = &destURI[pAt - pConfiguredDestURI + 1];
                     }
                  }
               }
            }
          
            if (!pHostUri)
            {
               pHostUri = destURI;
            }

            /* Get the destination URI host part */
   #ifdef TARGET_SIMU
            if (comPar.simDevIpAddr != 0)
            {
               tdcRes = iptGetUriHostPartSim(destIpAddr, comPar.simDevIpAddr, pHostUri, &tdcTopoCnt);
            }
            else
            {
               tdcRes = iptGetUriHostPart(destIpAddr, pHostUri, &tdcTopoCnt);
            }
   #else
            tdcRes = iptGetUriHostPart(destIpAddr, pHostUri, &tdcTopoCnt);
   #endif
            if (tdcRes != TDC_OK)
            {
               if (tdcRes == TDC_WRONG_TOPOCOUNT)
               {
                  IPTVosPrint3(IPT_ERR, "Wrong topoCnt value=%d Current value=%d. TDC ERROR=%#x\n",
                               topoCnt, tdcTopoCnt, tdcRes);
                  return(tdcRes);
               }
            }
         }
         else /* destIpAddr == 0 */
         {
            /* Override destination URI? */
            if (pDestURI != NULL)
            {
               if (destId != 0)
               {
                  resultCode = iptConfigGetDestIdPar(comId, destId, &pDestinationURI);
                  if (resultCode != (int)IPT_OK)
                  {
                     IPTVosPrint2(IPT_ERR,
                        "putRespMsg: Destination ID=%d not configured for ComId=%d\n",
                        destId, comId);
                     return(resultCode);
                  }
               }
               else
               {
                  pDestinationURI = pConfiguredDestURI;   
               }
              
               resultCode = overrideUri(pDestURI, &pDestinationURI, destURI, sizeof(destURI));
               if (resultCode != (int)IPT_OK)
               {
                  return(resultCode);
               }
            }
           
            if (pDestinationURI != NULL)
            {
               /* Get IP address from IPTDir based on the destination URI */
      #ifdef TARGET_SIMU
               if (comPar.simDevIpAddr != 0)
               {
                  tdcRes = iptGetAddrByNameExtSim(pDestinationURI, comPar.simDevIpAddr,
                                                  &comPar.destIpAddr,
                                                  &frg, &tdcTopoCnt);
               }
               else
               {
                  tdcRes = iptGetAddrByNameExt(pDestinationURI, &comPar.destIpAddr,
                                               &frg, &tdcTopoCnt);
               }
      #else
               tdcRes = iptGetAddrByNameExt(pDestinationURI, &comPar.destIpAddr,
                                            &frg, &tdcTopoCnt);
      #endif
               if (tdcRes != TDC_OK)
               {
                  if (tdcRes == TDC_WRONG_TOPOCOUNT)
                  {
                     IPTVosPrint3(IPT_ERR, "Wrong topoCnt value=%d Current value=%d. TDC ERROR=%#x\n",
                                  topoCnt, tdcTopoCnt, tdcRes);
                  }
                  else
                  {
                     IPTVosPrint2(IPT_ERR, "No IP address for URI = %s. TDC ERROR=%#x\n",
                                  pDestinationURI,tdcRes);
                  }
                  return(tdcRes);
               }
            }
            else
            {
               IPTVosPrint1(IPT_ERR, "No destination URI specified for ComId=%u\n",
                            comId);
               return((int)IPT_INVALID_COMPAR);
            }
         }
      }
      else  /* Old syntax */
      {
         /* Override destination URI? */
         if (pDestURI != NULL)
         {
            pDestinationURI = pDestURI;
          
            /* Get IP address from IPTDir based on the destination URI */
   #ifdef TARGET_SIMU
            if (comPar.simDevIpAddr != 0)
            {
               tdcRes = iptGetAddrByNameSim(pDestinationURI, comPar.simDevIpAddr,
                                            &comPar.destIpAddr, &tdcTopoCnt);
            }
            else
            {
               tdcRes = iptGetAddrByName(pDestinationURI, &comPar.destIpAddr,
                                         &tdcTopoCnt);
            }
   #else
            tdcRes = iptGetAddrByName(pDestinationURI, &comPar.destIpAddr,
                                      &tdcTopoCnt);
   #endif
            if (tdcRes != TDC_OK)
            {
               IPTVosPrint2(IPT_ERR, "No IP address for URI = %s TDC ERROR=%#x\n",
                            pDestinationURI, tdcRes);
               comPar.destIpAddr = 0;
               return((int)IPT_INVALID_COMPAR);
            }
         }
         else
         {
            /* Use destination IP address defined in the call */
            comPar.destIpAddr = destIpAddr;

            /* Get the destination URI host part */
   #ifdef TARGET_SIMU
            if (comPar.simDevIpAddr != 0)
            {
               tdcRes = iptGetUriHostPartSim(destIpAddr, comPar.simDevIpAddr, destURI, &tdcTopoCnt);
            }
            else
            {
               tdcRes = iptGetUriHostPart(destIpAddr, destURI, &tdcTopoCnt);
            }
   #else
            tdcRes = iptGetUriHostPart(destIpAddr, destURI, &tdcTopoCnt);
   #endif
            if (tdcRes == TDC_OK)
            {
               pDestinationURI = destURI; 
            }
         }
      }

      /* Override source URI? */
      if (pSrcURI != NULL)
      {
         if (syntax == NEW_OVERRIDE_SYNTAX)
         {
            /* Source URI not configured? */
            if (pSourceURI == NULL)
            {
#ifdef TARGET_SIMU
               pSourceURI = pSimUri;
#else
               /* Get own host URI from TDC  or TDCEmulator*/
			      dummy = 0;
               tdcRes = iptGetUriHostPart (0x7f000001, srcHostURI, &dummy);

               if (tdcRes == 0)
               {
                  pSourceURI = srcHostURI;
               }
               else
               {
                  IPTVosPrint1(IPT_WARN,
                               "Failed to get own source URI. iptGetUriHostPart res=%#x\n",
                               tdcRes );
               }
#endif
            }

            resultCode = overrideUri(pSrcURI, &pSourceURI, srcURI, sizeof(srcURI));
            if (resultCode != (int)IPT_OK)
            {
               return(resultCode);
            }
         }
         else
         {
            pSourceURI = pSrcURI;
         }
      }
      else
      {
         /* Source URI not configured? */
         if (pSourceURI == NULL)
         {
#ifdef TARGET_SIMU
            pSourceURI = pSimUri;
#else
            /* Get own host URI from TDC  or TDCEmulator*/
			   dummy = 0;
            tdcRes = iptGetUriHostPart (0x7f000001, srcHostURI, &dummy);
            if (tdcRes == 0)
            {
               pSourceURI = srcHostURI;
            }
            else
            {
               IPTVosPrint1(IPT_WARN,
                            "Failed to get own source URI. iptGetUriHostPart res=%#x\n",
                            tdcRes );
            }
#endif
         }
      }
    
      /* Check destination IP addresses */
      if (comPar.destIpAddr == 0)
      {
         IPTVosPrint0(IPT_ERR, "No Destionation IP address given for response message\n");
         return((int)IPT_INVALID_COMPAR);
      }

#ifdef TARGET_SIMU
      resultCode = prepareMsg(comId, userStatus, 0, pData, dataLength, pSourceURI,
                              pDestinationURI, &pSendMsg, comPar.destIpAddr,
                              &msgLength, tdcTopoCnt, pSimUri);
#else
      resultCode = prepareMsg(comId, userStatus, 0, pData, dataLength, pSourceURI,
                              pDestinationURI, &pSendMsg, comPar.destIpAddr,
                              &msgLength, tdcTopoCnt);
#endif
     
      if (resultCode == (int)IPT_OK)
      {
            if ((callerQueue != 0) || (func != 0))
            {
               msgType = MD_MSG_RESPONSE_RES;
            }
            else
            {
               msgType = MD_MSG_RESPONSE;
            }
            resultCode= createComInstances(callerQueue, func, pCallerRef,
                                           comPar, msgType, sessionId, comId, msgLength,
                                           0, pSendMsg);
      }

      return(resultCode);
   }
   else
   {
      return((int)IPT_MD_NOT_INIT);
   }
}

/*******************************************************************************
NAME:     deleteQueueItem
ABSTRACT: Delete queue item.
RETURNS:  -
*/
static void deleteQueueItem(
   MD_QUEUE queueId)
{
   int res;
   MD_QUEUE_ITEM *pQueueItem;
   
   pQueueItem = (MD_QUEUE_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.queueTableHdr),
                                              (UINT32)queueId)); /*lint !e826  Ignore casting warning */
   if (pQueueItem)
   {
      if (pQueueItem->pName)
      {
         (void)IPTVosFree((BYTE *)pQueueItem->pName);
      }

      res = iptTabDelete(&IPTGLOBAL(md.queueTableHdr),
                         (UINT32)queueId);
      if (res != IPT_OK)
      {
         IPTVosPrint1(IPT_ERR,
            "deleteQueueItem: Failed to delete queue from table. Error=%#x\n",
            res);
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR,
         "deleteQueueItem: Failed to delete queue not found in\n");
   }
}

/*******************************************************************************
NAME:     removeQueue
ABSTRACT: Remove queues
RETURNS:  -
*/
static void removeQueue(void)
{
   char *pData;
   int ret;
   UINT32 dataLength;
   MSG_INFO msgInfo;
   REM_QUEUE_ITEM **ppQueueItem;
   REM_QUEUE_ITEM *pQueueItem;
   MD_QUEUE queue;

   ret = IPTVosGetSem(&IPTGLOBAL(md.remQueueSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      ppQueueItem = &IPTGLOBAL(md.pFirstRemQueue);
      pQueueItem = *ppQueueItem;
      /* Any queues to be removed ? */
      while (pQueueItem)
      {
         queue = pQueueItem->queueId;
         removeTrQueue(queue);
         ret = removeSeQueue(queue);
         if (ret == IPT_OK)
         {
            /* Empty the queue */
            pData = NULL; /* Use buffer allocated by IPTCom */
            while ((ret = MDComAPI_getMsg(queue, &msgInfo, &pData, &dataLength,
                                          IPT_NO_WAIT)) != MD_QUEUE_EMPTY)
            {
               if (ret == MD_QUEUE_NOT_EMPTY)
               {
                  if (pData != NULL)
                  {
                     (void)IPTVosFree((unsigned char *)pData);
                     pData = NULL;
                  }
               }
               else
               {
                  break;
               }
            }

            deleteQueueItem(queue);
           
            /* Destroy the queue */
            ret = IPTVosDestroyMsgQueue((IPT_QUEUE *)((void *)&queue));
            if (ret != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "removeQueue: failed to destroy queue\n");
            }

            *ppQueueItem = pQueueItem->pNext;

            (void)IPTVosFree((unsigned char *)pQueueItem);
         }
         else
         {
            ppQueueItem = &(pQueueItem->pNext);
         }   
         
         pQueueItem = *ppQueueItem;
      }

      if(IPTVosPutSemR(&IPTGLOBAL(md.remQueueSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "removeQueue: IPTVosGetSem ERROR\n");
   }
   
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:     MDCom_prepareInit
ABSTRACT: Initializes MD communication.
          The mdControl data is set up.
RETURNS:  0 if OK, !=0 if not
*/
int MDCom_prepareInit(void)
{
   int res;

   IPTGLOBAL(md.mdComInitiated) = 0;
   IPTGLOBAL(md.seInitiated) = 0;
   IPTGLOBAL(md.pFirstSeInstance) = 0;
   IPTGLOBAL(md.pLastSeInstance) = 0;
   IPTGLOBAL(md.trInitiated) = 0;
   IPTGLOBAL(md.pFirstTrInstance) = 0;
   IPTGLOBAL(md.pLastTrInstance) = 0;
   IPTGLOBAL(md.pFirstRecSeqCnt) = 0;
   IPTGLOBAL(md.pFirstFrgRecSeqCnt) = 0;
   IPTGLOBAL(md.pFirstSendSeqCnt) = 0;
   IPTGLOBAL(md.pFirstRemQueue) = 0;

#ifdef LINUX
   /* Create send task  queue , a queue with maximum no of messages will be
      created*/
   res = IPTVosCreateMsgQueue(&IPTGLOBAL(md.mdSendQueueId),
                                 1,
                                 sizeof(SEND_QUEUE_MSG)); 
#else
   /* Create send task  queue */
   res = IPTVosCreateMsgQueue(&IPTGLOBAL(md.mdSendQueueId),
                                 MAX_NO_SEND_QUEUE_MSG,
                                 sizeof(SEND_QUEUE_MSG)); 
#endif
   if (res != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "ERROR creating  mdSendQueue\n");
      return res;
   }
   
   res =  IPTVosCreateSem(&IPTGLOBAL(md.remQueueSemId), IPT_SEM_FULL);
   if (res != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "ERROR creating remQueueSemId\n");
      return res;
   }
   
   res = seInit();
   if (res  != (int)IPT_OK)
   {
      return res;
   }

   res = trInit();
   if (res != (int)IPT_OK)
   {
      return res;
   }

   /* Create a semaphore for the listeners list resource
    initial state = free */
   res = IPTVosCreateSem(&IPTGLOBAL(md.listenerSemId), IPT_SEM_FULL);
   if (res != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "ERROR creating  listenerSemId\n");
      return res;
   }
   IPTGLOBAL(md.pFirstListenerQueue) = NULL;
   IPTGLOBAL(md.pFirstListenerFunc) = NULL;

   if ((res = iptTabInit(&IPTGLOBAL(md.frgTableHdr), sizeof(FRG_ITEM))) != (int)IPT_OK)
      return res;

   if ((res = iptTabInit(&IPTGLOBAL(md.queueTableHdr), sizeof(MD_QUEUE_ITEM))) != (int)IPT_OK)
      return res;

#ifdef TARGET_SIMU
   if ((res = iptTabUriInit(&IPTGLOBAL(md.simuDevListTableHdr), sizeof(SIMU_DEV_ITEM))) != (int)IPT_OK)
      return res;
#endif
   if ((res = iptTabInit(&IPTGLOBAL(md.listTables.comidListTableHdr), sizeof(COMID_ITEM))) != (int)IPT_OK)
      return res;

   if ((res = iptUriLabelTab2Init(&IPTGLOBAL(md.listTables.instXFuncNListTableHdr), sizeof(INSTX_FUNCN_ITEM))) != (int)IPT_OK)
      return res;

   if ((res = iptUriLabelTabInit(&IPTGLOBAL(md.listTables.aInstFuncNListTableHdr), sizeof(AINST_FUNCN_ITEM))) != (int)IPT_OK)
      return res;

   if ((res = iptUriLabelTabInit(&IPTGLOBAL(md.listTables.instXaFuncListTableHdr), sizeof(INSTX_AFUNC_ITEM))) != (int)IPT_OK)
      return res;

   IPTGLOBAL(md.listTables.aInstAfunc).pQueueList = NULL;
   IPTGLOBAL(md.listTables.aInstAfunc).pQueueFrgList = NULL;
   IPTGLOBAL(md.listTables.aInstAfunc).pFuncList = NULL;
   IPTGLOBAL(md.listTables.aInstAfunc).pFuncFrgList = NULL;
   IPTGLOBAL(md.listTables.aInstAfunc).mdInPackets = 0;
   IPTGLOBAL(md.listTables.aInstAfunc).mdFrgInPackets = 0;

   IPTGLOBAL(md.mdComInitiated) = 1;
   return 0;
}

/*******************************************************************************
NAME:     MDCom_process
ABSTRACT: Process method for MDCom component.
          Only here for compability reasons 
RETURNS:  - 
*/
void MDCom_process(void)
{
   /* Do nothing */
}

/*******************************************************************************
NAME:     MDCom_send
ABSTRACT: The mainroutine for the MD send task
          Cyclic called to send MD frames.
RETURNS:  - 
*/
void MDCom_send(void)
{
   int ret;
#if defined(IF_WAIT_ENABLE)
   UINT32 i;
   IPT_CONFIG_COM_PAR_EXT *pComPar;
#endif

   if (IPTGLOBAL(md.mdComInitiated))
   {
#if defined(IF_WAIT_ENABLE)
      if (IPTGLOBAL(configDB.finish_socket_creation))
      {
         if (ip_status_get(NULL, IPT_NO_WAIT) != IP_RUNNING)
         {
            return;
         }
         if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
         {
            pComPar = (IPT_CONFIG_COM_PAR_EXT *)(IPTGLOBAL(configDB.comParTable.pTable)); /*lint !e826 Type cast OK */ 
            for (i=0; i<IPTGLOBAL(configDB.comParTable.nItems) ; i++)
            {
               if (pComPar[i].pdSendSocket == 0)
               {
                  /* Create send sockets for this communication pararameters */
                  (void)IPTPDSendSocketCreate(&pComPar[i]);
               }
               if (pComPar[i].mdSendSocket == 0)
               {
                  /* Create send sockets for this communication pararameters */
                  (void)IPTMDSendSocketCreate(&pComPar[i]);
               }
            }
        
            IPTGLOBAL(configDB.finish_socket_creation) = 0;
            IPTGLOBAL(ifWaitSend) = 0;
            if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "MDCom_send: IPTVosGetSem(&IPTGLOBAL(configDB.sem)) ERROR\n");
            return;
         }
      }
#endif 

      if ((IPTGLOBAL(configDB.finish_addr_resolv)) ||
          (IPTGLOBAL(md.finish_addr_resolv)) ||
          (IPTGLOBAL(pd.finishRecAddrResolv)) ||
          (IPTGLOBAL(pd.finishSendAddrResolv)))
      {
         ret = IPTCom_getStatus();
         /* TDC ready ? */
#if defined(IF_WAIT_ENABLE)
         if (ret != IPTCOM_NOT_INITIATED)
#else
         if (ret == IPTCOM_RUN)
#endif
         {
            if (IPTGLOBAL(configDB.finish_addr_resolv))
            {
               iptFinishConfig();
            }

#if defined(IF_WAIT_ENABLE)
            if ((IPTGLOBAL(md.finish_addr_resolv)) && (IPTGLOBAL(ifRecReadyMD)))
#else
            if (IPTGLOBAL(md.finish_addr_resolv))
#endif
            {
               MD_finish_addListener();
            }

            if ((IPTGLOBAL(pd.finishRecAddrResolv)) ||
                (IPTGLOBAL(pd.finishSendAddrResolv)))
            {
               PD_finish_subscribe_publish();   
            }
         }
      }

      checkSendQueue();
      trSendTask();
      seSendTask();
      removeQueue();    
   }
}

/*******************************************************************************
NAME:     MDCom_destroy
ABSTRACT: Destroy method for MDCom component. 
RETURNS:  -
*/
void MDCom_destroy(void)
{
   int   res;

   IPTGLOBAL(md.mdComInitiated) = 0;

   /* Remove listener */
   mdTerminateListener();

   /* Destroy a semaphore for the listeners list resource */
   IPTVosDestroySem(&IPTGLOBAL(md.listenerSemId));
   
   seTerminate();
   trTerminate();

   /* Destroy send task  queue */
   res   =  IPTVosDestroyMsgQueue(&IPTGLOBAL(md.mdSendQueueId)); 
   if (res  != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "ERROR destroying  mdSendQueue\n");
   }
}


/*******************************************************************************
NAME:     MDComAPI_putDataMsg
ABSTRACT: Send a message without any expected reply from the receiving
          application.
          Unicast communication result will be reported back to the sending 
          application if the parameter callerQueue or func is given.

          Can be  used for:
          - Unicast  message. 
            Without  expected reply from receiving end. 
            Communication result can be reported to sender application.  
          - Multicast and unicast message. 
            Without expected reply from receiving end.  
            Communication result can not be reported to sender application.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_putDataMsg(
   UINT32     comId,       /* ComID */
   const char *pData,      /* Pointer to buffer with data to be send */
   UINT32     dataLength,  /* Number of bytes to be send */
   MD_QUEUE   callerQueue, /* Caller queue ID. 
                              0 = No result will be reported back */
   IPT_REC_FUNCPTR func,   /* Pointer to callback function.
                              0 = No result will be reported back */
   const void *pCallerRef, /* Caller reference */
   UINT32     topoCnt,     /* Topo counter */
   UINT32     destId,      /* Destination URI ID */
   const char *pDestURI,   /* Pointer to overriding destination URI string.
                              0 = use URI string defined for the comID */
   const char *pSrcURI)    /* Pointer to overriding source URI string.
                              0 = use URI string defined for the comID */
#ifndef TARGET_SIMU				
{
	return putMsg(comId, pData, dataLength, 0, 0, callerQueue, func, pCallerRef,
                 topoCnt, destId, pDestURI, pSrcURI, NEW_OVERRIDE_SYNTAX, MD_DATA_MSG);
}
#else
{
	return putMsg(comId, pData, dataLength, 0, 0, callerQueue, func, pCallerRef,
                 topoCnt, destId, pDestURI, pSrcURI, NEW_OVERRIDE_SYNTAX, MD_DATA_MSG, NULL);
}

int MDComAPI_putDataMsgSim(
   UINT32     comId,
   const char *pData,
   UINT32     dataLength, 
   MD_QUEUE   callerQueue,
   IPT_REC_FUNCPTR func,   
   const void *pCallerRef,
   UINT32     topoCnt,
   UINT32     destId,      
   const char *pDestURI,
	const char *pSrcURI,
	const char *pSimUri)

{
	return putMsg(comId, pData, dataLength, 0, 0, callerQueue, func, pCallerRef,
                 topoCnt, destId, pDestURI, pSrcURI, NEW_OVERRIDE_SYNTAX, MD_DATA_MSG, pSimUri);
}
#endif	

/*******************************************************************************
NAME:     MDComAPI_putMsgQ
ABSTRACT: Send a message without any expected reply from the receiving
          application.
          Unicast communication result will be reported back to the sending 
          application if the parameter callerQueue is given.

          Can be  used for:
          - Unicast  message. 
            Without  expected reply from receiving end. 
            Communication result can be reported to sender application.  
          - Multicast and unicast message. 
            Without expected reply from receiving end.  
            Communication result can not be reported to sender application.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_putMsgQ(
   UINT32     comId,       /* ComID */
   const char *pData,      /* Pointer to buffer with data to be send */
   UINT32     dataLength,  /* Number of bytes to be send */
   MD_QUEUE   callerQueue, /* Caller queue ID. 
                              0 = No result will be reported back */
   const void *pCallerRef, /* Caller reference */
   UINT32     topoCnt,     /* Topo counter */
   const char *pDestURI,   /* Pointer to overriding destination URI string.
                              0 = use URI string defined for the comID */
   const char *pSrcURI)    /* Pointer to overriding source URI string.
                              0 = use URI string defined for the comID */

#ifndef TARGET_SIMU				
{
	return putMsg(comId, pData, dataLength, 0, 0, callerQueue, NULL, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_DATA_MSG);
}
#else
{
	return putMsg(comId, pData, dataLength, 0, 0, callerQueue, NULL, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_DATA_MSG, NULL);
}

int MDComAPI_putMsgQSim(
   UINT32     comId,
   const char *pData,
   UINT32     dataLength, 
   MD_QUEUE   callerQueue,
   const void *pCallerRef,
   UINT32     topoCnt,
   const char *pDestURI,
	const char *pSrcURI,
	const char *pSimUri)
{
	return putMsg(comId, pData, dataLength, 0, 0, callerQueue, NULL, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_DATA_MSG, pSimUri);
}
#endif	

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_putMsgF
ABSTRACT: Send a message without any expected reply from the receiving
          application.
          Unicast communication result will be reported back to the sending 
          application if the parameter func is given.

          Can be  used for:
          - Unicast  message. 
            Without  expected reply from receiving end. 
            Communication result can be reported to sender callback function.
          - Multicast and unicast message. 
            Without expected reply from receiving end.  
            Communication result can not be reported to sender application.
RETURNS:  0 if OK, !=0 if not.
*/

int MDComAPI_putMsgF(
   UINT32          comId,       /* ComID */
   const char      *pData,      /* Pointer to buffer with data to be send */
   UINT32          dataLength,  /* Number of bytes to be send */
   IPT_REC_FUNCPTR func,        /* Pointer to callback function.
                                   0 = No result will be reported back */
   const void      *pCallerRef, /* Caller reference */
   UINT32          topoCnt,     /* Topo counter */
   const char      *pDestURI,   /* Pointer to overriding destination URI string.
                                   0 = use URI string defined for the comID */
   const char      *pSrcURI)    /* Pointer to overriding source URI string.
                                   0 = use URI string defined for the comID */
#ifndef TARGET_SIMU				
{
	return putMsg(comId, pData, dataLength, 0, 0, 0, func, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_DATA_MSG);
}
#else
{
	return putMsg(comId, pData, dataLength, 0, 0, 0, func, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_DATA_MSG, NULL);
}

int MDComAPI_putMsgFSim(
   UINT32          comId,
   const char      *pData,
   UINT32          dataLength, 
   IPT_REC_FUNCPTR func,
   const void      *pCallerRef,
   UINT32          topoCnt,
   const char      *pDestURI,
	const char      *pSrcURI,
	const char      *pSimUri)
{
	return putMsg(comId, pData, dataLength, 0, 0, 0, func, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_DATA_MSG, pSimUri);
}
#endif	
#endif

/*******************************************************************************
NAME:     MDComAPI_putReqMsg
ABSTRACT: Send a message with expected reply/ies from the receiving
          application(s).

          Can be  used for:
          - Multicast and unicast message. 
            With expected reply  from receiving end.  
            Communication result and replies reported to sender application.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_putReqMsg(
   UINT32     comId,           /* ComID */
   const char *pData,          /* Pointer to buffer with data to be send */
   UINT32     dataLength,      /* Number of bytes to be send */
   UINT16     noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32     responseTimeout, /* Time-out value  in milliseconds for receiving 
                                  replies 0=default value */
   MD_QUEUE   callerQueue,     /* Caller queue ID */
   IPT_REC_FUNCPTR func,       /* Pointer to callback function.
                                  0 = No result will be reported back */
   const void *pCallerRef,     /* Caller reference */
   UINT32     topoCnt,         /* Topo counter */
   UINT32     destId,      /* Destination URI ID */
   const char *pDestURI,       /* Pointer to overriding destination URI string.
                                  0 = use URI string defined for the comID */
   const char *pSrcURI)        /* Pointer to overriding source URI string.
                                  0 = use URI string defined for the comID */
#ifndef TARGET_SIMU				
{
	return putMsg(comId, pData, dataLength, noOfResponses, responseTimeout, callerQueue, func, pCallerRef,
                 topoCnt, destId, pDestURI, pSrcURI, NEW_OVERRIDE_SYNTAX, MD_REQ_MSG);
}
#else
{
	return putMsg(comId, pData, dataLength, noOfResponses, responseTimeout, callerQueue, func, pCallerRef,
                 topoCnt, destId, pDestURI, pSrcURI, NEW_OVERRIDE_SYNTAX, MD_REQ_MSG, NULL);
}

int MDComAPI_putReqMsgSim(
   UINT32    comId,
   const char *pData,
   UINT32     dataLength,
   UINT16     noOfResponses,  
   UINT32     responseTimeout,
   MD_QUEUE   callerQueue,
   IPT_REC_FUNCPTR func,
   const void *pCallerRef,
   UINT32     topoCnt,
   UINT32     destId,
   const char *pDestURI,
	const char *pSrcURI,
	const char *pSimUri)
{
	return putMsg(comId, pData, dataLength, noOfResponses, responseTimeout, callerQueue, func, pCallerRef,
                 topoCnt, destId, pDestURI, pSrcURI, NEW_OVERRIDE_SYNTAX, MD_REQ_MSG, pSimUri);
}
#endif	

/*******************************************************************************
NAME:     MDComAPI_putRequestMsgQ
ABSTRACT: Send a message with expected reply/ies from the receiving
          application(s).

          Can be  used for:
          - Multicast and unicast message. 
            With expected reply  from receiving end.  
            Communication result and replies reported to sender application.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_putRequestMsgQ(
   UINT32     comId,           /* ComID */
   const char *pData,          /* Pointer to buffer with data to be send */
   UINT32     dataLength,      /* Number of bytes to be send */
   UINT16     noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32     responseTimeout, /* Time-out value  in milliseconds for receiving 
                                  replies 0=default value */
   MD_QUEUE   callerQueue,     /* Caller queue ID */
   const void *pCallerRef,     /* Caller reference */
   UINT32     topoCnt,         /* Topo counter */
   const char *pDestURI,       /* Pointer to overriding destination URI string.
                                  0 = use URI string defined for the comID */
   const char *pSrcURI)        /* Pointer to overriding source URI string.
                                  0 = use URI string defined for the comID */
#ifndef TARGET_SIMU				
{
	return putMsg(comId, pData, dataLength, noOfResponses, responseTimeout, callerQueue, NULL, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_REQ_MSG);
}
#else
{
	return putMsg(comId, pData, dataLength, noOfResponses, responseTimeout, callerQueue, NULL, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_REQ_MSG, NULL);
}

int MDComAPI_putRequestMsgQSim(
   UINT32    comId,
   const char *pData,
   UINT32     dataLength,
   UINT16     noOfResponses,  
   UINT32     responseTimeout,
   MD_QUEUE   callerQueue,
   const void *pCallerRef,
   UINT32     topoCnt,
   const char *pDestURI,
	const char *pSrcURI,
	const char *pSimUri)
{
	return putMsg(comId, pData, dataLength, noOfResponses, responseTimeout, callerQueue, NULL, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_REQ_MSG, pSimUri);
}
#endif	

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_putRequestMsgF
ABSTRACT: Send a message with expected reply/ies from the receiving
          application(s).

          Can be  used for:
          - Multicast and unicast message. 
            With expected reply  from receiving end.  
            Communication result and replies reported via sender callback
            function.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_putRequestMsgF(
   UINT32          comId,           /* ComID */
   const char      *pData,          /* Pointer to buffer with data to be send */
   UINT32          dataLength,      /* Number of bytes to be send */
   UINT16          noOfResponses,   /* Number of expected replies. 0=undefined */
   UINT32          responseTimeout, /* Time-out value  in milliseconds for 
                                       receiving replies 0=default value */
   IPT_REC_FUNCPTR func,            /* Pointer to callback function */
   const void      *pCallerRef,     /* Caller reference */
   UINT32          topoCnt,         /* Topo counter */
   const char      *pDestURI,       /* Pointer to overriding destination URI string.
                                       0 = use URI string defined for the comID */
   const char      *pSrcURI)        /* Pointer to overriding source URI string.
                                       0 = use URI string defined for the comID */
#ifndef TARGET_SIMU				
{
	return putMsg(comId, pData, dataLength, noOfResponses, responseTimeout, 0, func, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_REQ_MSG);
}
#else
{
	return putMsg(comId, pData, dataLength, noOfResponses, responseTimeout, 0, func, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_REQ_MSG, NULL);
}

int MDComAPI_putRequestMsgFSim(
   UINT32          comId,
   const char      *pData,
   UINT32          dataLength,
   UINT16          noOfResponses,  
   UINT32          responseTimeout,
   IPT_REC_FUNCPTR func,
   const void      *pCallerRef,
   UINT32          topoCnt,
   const char      *pDestURI,
	const char		  *pSrcURI,
	const char      *pSimUri)
{
	return putMsg(comId, pData, dataLength, noOfResponses, responseTimeout, 0, func, pCallerRef,
                 topoCnt, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, MD_REQ_MSG, pSimUri);
}
#endif	
#endif

/*******************************************************************************
NAME:     MDComAPI_putRespMsg
ABSTRACT: Send a response message.

          Can be  used for:
          - Reply message (Unicast).
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_putRespMsg(
   UINT32     comId,      /* ComID */
   UINT16     userStatus, /* The value is transported to the requesting 
                             application */
   const char *pData,     /* Pointer to buffer with data to be send */
   UINT32     dataLength, /* Number of bytes to be send */
   UINT32     sessionID,  /* Session ID, has to be the same as in the received
                             request */                             
   MD_QUEUE   callerQueue, /* Caller queue ID. 
                              0 = No result will be reported back */
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
#ifndef TARGET_SIMU				
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, callerQueue, func, pCallerRef,
                 topoCnt, destIpAddr, destId, pDestURI, pSrcURI, NEW_OVERRIDE_SYNTAX);
}
#else
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, callerQueue, func, pCallerRef,
                 topoCnt, destIpAddr, destId, pDestURI, pSrcURI, NEW_OVERRIDE_SYNTAX, NULL);
}

int MDComAPI_putRespMsgSim(
   UINT32     comId,
   UINT16     userStatus,
   const char *pData,
   UINT32     dataLength,
   UINT32     sessionID,                          
   MD_QUEUE   callerQueue,
   IPT_REC_FUNCPTR func,   /* Pointer to callback function.
                              0 = No result will be reported back */
   const void *pCallerRef,
   UINT32     topoCnt,
   UINT32     destIpAddr,
   UINT32     destId,      /* Destination URI ID */
   const char *pDestURI,
	const char *pSrcURI,
	const char *pSimUri)
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, callerQueue, func,  pCallerRef,
                 topoCnt, destIpAddr, destId, pDestURI, pSrcURI, NEW_OVERRIDE_SYNTAX, pSimUri);
}
#endif	

/*******************************************************************************
NAME:     MDComAPI_putResponseMsg
ABSTRACT: Send a response message.

          Can be  used for:
          - Reply message (Unicast).
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_putResponseMsg(
   UINT32     comId,      /* ComID */
   UINT16     userStatus, /* The value is transported to the requesting 
                             application */
   const char *pData,     /* Pointer to buffer with data to be send */
   UINT32     dataLength, /* Number of bytes to be send */
   UINT32     sessionID,  /* Session ID, has to be the same as in the received
                             request */                             
   UINT32     topoCnt,    /* Topo counter */
   UINT32     destIpAddr, /* IP address, has to be the same as in the received
                             request if pDestURI = 0 */
   const char *pDestURI,  /* Pointer to destination URI string.
                             <> 0 Get destination IP address from IPTDir and not
                             from the parameter destIpAddr. Override dest.
                             URI string defeined for the comID
                             0 = use parameter destIpAddr and URI string defined 
                             for the comID */
   const char *pSrcURI)   /* Pointer to overriding source URI string.
                             0 = use URI string defined for the comID */
#ifndef TARGET_SIMU				
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, 0, 0, 0,
                 topoCnt, destIpAddr, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX);
}
#else
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, 0, 0, 0,
                 topoCnt, destIpAddr, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, NULL);
}

int MDComAPI_putResponseMsgSim(
   UINT32     comId,
   UINT16     userStatus,
   const char *pData,
   UINT32     dataLength,
   UINT32     sessionID,                          
   UINT32     topoCnt,
   UINT32     destIpAddr,
   const char *pDestURI,
	const char *pSrcURI,
	const char *pSimUri)
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, 0, 0, 0,
                 topoCnt, destIpAddr, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, pSimUri);
}
#endif	

/*******************************************************************************
NAME:     MDComAPI_putResponseMsgQ
ABSTRACT: Send a response message.

          Can be  used for:
          - Reply message (Unicast).
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_putResponseMsgQ(
   UINT32     comId,      /* ComID */
   UINT16     userStatus, /* The value is transported to the requesting 
                             application */
   const char *pData,     /* Pointer to buffer with data to be send */
   UINT32     dataLength, /* Number of bytes to be send */
   UINT32     sessionID,  /* Session ID, has to be the same as in the received
                             request */                             
   MD_QUEUE   callerQueue, /* Caller queue ID. 
                              0 = No result will be reported back */
   const void *pCallerRef, /* Caller reference */
   UINT32     topoCnt,    /* Topo counter */
   UINT32     destIpAddr, /* IP address, has to be the same as in the received
                             request if pDestURI = 0 */
   const char *pDestURI,  /* Pointer to destination URI string.
                             <> 0 Get destination IP address from IPTDir and not
                             from the parameter destIpAddr. Override dest.
                             URI string defeined for the comID
                             0 = use parameter destIpAddr and URI string defined 
                             for the comID */
   const char *pSrcURI)   /* Pointer to overriding source URI string.
                             0 = use URI string defined for the comID */
#ifndef TARGET_SIMU				
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, callerQueue, 0, pCallerRef,
                 topoCnt, destIpAddr, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX);
}
#else
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, callerQueue, 0, pCallerRef,
                 topoCnt, destIpAddr, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, NULL);
}

int MDComAPI_putResponseMsgQSim(
   UINT32     comId,
   UINT16     userStatus,
   const char *pData,
   UINT32     dataLength,
   UINT32     sessionID,                          
   MD_QUEUE   callerQueue,
   const void *pCallerRef,
   UINT32     topoCnt,
   UINT32     destIpAddr,
   const char *pDestURI,
	const char *pSrcURI,
	const char *pSimUri)
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, callerQueue, 0,  pCallerRef,
                 topoCnt, destIpAddr, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, pSimUri);
}
#endif	

#ifndef LINUX_MULTIPROC
/*******************************************************************************
NAME:     MDComAPI_putResponseMsgF
ABSTRACT: Send a response message.

          Can be  used for:
          - Reply message (Unicast).
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_putResponseMsgF(
   UINT32     comId,      /* ComID */
   UINT16     userStatus, /* The value is transported to the requesting 
                             application */
   const char *pData,     /* Pointer to buffer with data to be send */
   UINT32     dataLength, /* Number of bytes to be send */
   UINT32     sessionID,  /* Session ID, has to be the same as in the received
                             request */                             
   IPT_REC_FUNCPTR func,        /* Pointer to callback function.
                                   0 = No result will be reported back */
   const void      *pCallerRef, /* Caller reference */
   UINT32     topoCnt,    /* Topo counter */
   UINT32     destIpAddr, /* IP address, has to be the same as in the received
                             request if pDestURI = 0 */
   const char *pDestURI,  /* Pointer to destination URI string.
                             <> 0 Get destination IP address from IPTDir and not
                             from the parameter destIpAddr. Override dest.
                             URI string defeined for the comID
                             0 = use parameter destIpAddr and URI string defined 
                             for the comID */
   const char *pSrcURI)   /* Pointer to overriding source URI string.
                             0 = use URI string defined for the comID */
#ifndef TARGET_SIMU				
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, 0, func, pCallerRef,
                 topoCnt, destIpAddr, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX);
}
#else
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, 0, func, pCallerRef,
                 topoCnt, destIpAddr, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, NULL);
}

int MDComAPI_putResponseMsgFSim(
   UINT32         comId,
   UINT16          userStatus,
   const char      *pData,
   UINT32          dataLength,
   UINT32          sessionID,                          
   IPT_REC_FUNCPTR func,
   const void      *pCallerRef,
   UINT32          topoCnt,
   UINT32          destIpAddr,
   const char      *pDestURI,
	const char      *pSrcURI,
	const char      *pSimUri)
{
	return putRespMsg(comId, userStatus, pData, dataLength, sessionID, 0, func, pCallerRef,
                 topoCnt, destIpAddr, 0, pDestURI, pSrcURI, OLD_OVERRIDE_SYNTAX, pSimUri);
}
#endif	
#endif

/*******************************************************************************
NAME:     getQueueItem
ABSTRACT: Search queue name. 
RETURNS:  Pointer to the queue name
*/
char *getQueueItemName(
   MD_QUEUE queueId)   
{
   MD_QUEUE_ITEM *pQueueItem;
   
   pQueueItem = (MD_QUEUE_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.queueTableHdr),
                                              (UINT32)queueId)); /*lint !e826  Ignore casting warning */
   if (pQueueItem)
   {
      return(pQueueItem->pName);
   }
   else
   {
      return((char *)NULL);
   }
}

/*******************************************************************************
NAME:     MDComAPI_queueCreate
ABSTRACT: Create message data queue to be used for receiving message data.
RETURNS:  Returns a MD_QUEUE handle to be used in subsequent calls.
         NULL if error.
*/
MD_QUEUE MDComAPI_queueCreate(
   int noOfMsg,      /* Number of queue members */
   const char *name) /* Queue name */
{
   int res;
   char *pTmp;
   MD_QUEUE queueId;
   MD_QUEUE_ITEM queueItem;
   
   res =  IPTVosCreateMsgQueue((IPT_QUEUE *)((void *)&queueId), noOfMsg, sizeof(QUEUE_MSG)); 

   if (res == (int)IPT_OK)
   {
      queueItem.queueId = queueId;
      if ((name) && (name[0] != 0))
      {
      	 /* CR-3477: Findings during TÜV-Assessment - Unsafe use of strcpy / account for trailing zero!	*/
         pTmp = (char *) IPTVosMalloc(strlen(name) + 1);
         if (pTmp == NULL)
         {
            IPTVosPrint2(IPT_ERR,
                         "MDComAPI_queueCreate could not allocate memory size=%d for queue %s\n",
                         strlen(name), name);
            queueItem.pName = NULL;
         }
         else
         {
            strcpy(pTmp,name);
            queueItem.pName = pTmp;
         }
      }
      else
      {
         queueItem.pName = NULL;
      }
     
      res = IPTVosGetSem(&IPTGLOBAL(md.remQueueSemId), IPT_WAIT_FOREVER);
      if (res == IPT_OK)
      {
         res = iptTabAdd(&IPTGLOBAL(md.queueTableHdr), (IPT_TAB_ITEM_HDR *)((void *)&queueItem));
         if (res != IPT_OK)
         {
               IPTVosPrint2(IPT_ERR,
                  "MDComAPI_queueCreate: Failed to add Queue %s to table. Error=%#x\n",
                  (name?name:""), res);
         }
         if(IPTVosPutSemR(&IPTGLOBAL(md.remQueueSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "MDComAPI_queueCreate: IPTVosGetSem ERROR\n");
      }
      
      return(queueId);
   }
   else
   {
      IPTVosPrint1(IPT_ERR, "ERROR creating queue %s\n",(name?name:""));
      return(0);
   }
}

/*******************************************************************************
NAME:     MDComAPI_createQueue
ABSTRACT: Create message data queue to be used for receiving message data.
RETURNS:  Returns a MD_QUEUE handle to be used in subsequent calls.
         NULL if error.
*/
MD_QUEUE MDComAPI_createQueue(
   int noOfMsg)  /* Number of queue members */
{
   return(MDComAPI_queueCreate(noOfMsg, NULL));
}

/*******************************************************************************
NAME:     MDComAPI_destroyQueue
ABSTRACT: Destroys a message data queue.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_destroyQueue(
   MD_QUEUE queue)   /* Queue to destroy */
{
   return(MDComAPI_removeQueue(queue,0));
}

/*******************************************************************************
NAME:     MDComAPI_removeQueue
ABSTRACT: Remove all use of and destroys a message data queue.
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_removeQueue(  MD_QUEUE queue,   /* Queue to destroy */
                           UINT8    allUse)  /* Flag set if all use of the queue shall be terminated */
{
   char *pData;
   char *pTemp;
   int res;
   UINT32 dataLength;
   MSG_INFO msgInfo;
   REM_QUEUE_ITEM *pNew;
#ifdef TARGET_SIMU
   int j;
   SIMU_DEV_ITEM  *pSimDevItem;
#endif

   if (queue != 0)
   {
      /* Terminate all use of the queue? */
      if (allUse)
      {
         /* Remove any listeners from the queue */
         MDComAPI_removeListenerQ(queue);
#ifdef TARGET_SIMU
         pSimDevItem = (SIMU_DEV_ITEM *)(IPTGLOBAL(md.simuDevListTableHdr.pTable));/*lint !e826 Type cast OK */
         for (j = 0; j < IPTGLOBAL(md.simuDevListTableHdr.nItems); j++)
         {
            MDComAPI_removeListenerQSim(queue, pSimDevItem->simUri);   
         }
#endif
         res = IPTVosGetSem(&IPTGLOBAL(md.remQueueSemId), IPT_WAIT_FOREVER);
         if (res == IPT_OK)
         {
            pNew = (REM_QUEUE_ITEM *)(IPTVosMalloc(sizeof(REM_QUEUE_ITEM)));
          
            if (pNew == 0)
            {
               res = (int)IPT_MEM_ERROR;
               IPTVosPrint1(IPT_ERR,
                          "MDComAPI_removeQueue: Out of memory. Requested size=%d\n",
                          sizeof(REM_QUEUE_ITEM));
            }
            else
            {
               /* Add queue to be removed to the queue remove list checked by the MD send task */
               pNew->queueId = queue;
               pNew->pNext = IPTGLOBAL(md.pFirstRemQueue); 
               IPTGLOBAL(md.pFirstRemQueue) = pNew;
            }


            if(IPTVosPutSemR(&IPTGLOBAL(md.remQueueSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "deleteTrInstance: IPTVosGetSem ERROR\n");
         }

         return(res);
      }
      else
      {
         /* Any listener using the queue ? */
         if (mdAnyListenerOnQueue(queue))
         {
            pTemp = getQueueItemName(queue);
            IPTVosPrint2(IPT_ERR,
                "ERROR destroy queue ID=%#x Name=%s. A listerner is registered on the queue\n",
                         queue, (pTemp != NULL)?pTemp:"None" );
            return((int)IPT_QUEUE_IN_USE);
         }

         /* Any active transport instance using the queue ? */
         if (searchTrQueue(queue))
         {
            pTemp = getQueueItemName(queue);
            IPTVosPrint2(IPT_ERR,
            "ERROR destroy queue ID=%#x Name=%s. Transport layer using the sender application queue\n",
                         queue, (pTemp != NULL)?pTemp:"None" );
            return((int)IPT_QUEUE_IN_USE);
         }
      
         /* Any active session instance using the queue ? */
         if (searchSeQueue(queue))
         {
            pTemp = getQueueItemName(queue);
            IPTVosPrint2(IPT_ERR,
            "ERROR destroy queue ID=%#x Name=%s. Session layer using the sender application queue\n",
                         queue, (pTemp != NULL)?pTemp:"None" );
            return((int)IPT_QUEUE_IN_USE);
         }

         /* Empty the queue */
         pData = NULL; /* Use buffer allocated by IPTCom */
         while ((res = MDComAPI_getMsg(queue, &msgInfo, &pData, &dataLength,
                                       IPT_NO_WAIT)) != MD_QUEUE_EMPTY)
         {
            if (res == MD_QUEUE_NOT_EMPTY)
            {
               if (pData != NULL)
               {

                  (void)IPTVosFree((unsigned char *)pData);
                  pData = NULL;
               }
            }
            else
            {
               break;
            }
         }
         res = IPTVosGetSem(&IPTGLOBAL(md.remQueueSemId), IPT_WAIT_FOREVER);
         if (res == IPT_OK)
         {
            deleteQueueItem(queue);

            if(IPTVosPutSemR(&IPTGLOBAL(md.remQueueSemId)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
         }
         else
         {
            IPTVosPrint0(IPT_ERR, "MDComAPI_removeQueue: IPTVosGetSem ERROR\n");
         }

         return (IPTVosDestroyMsgQueue((IPT_QUEUE *)((void *)&queue)));
      }
   }
   else
   {
      IPTVosPrint0(IPT_WARN,
                   "ERROR MDComAPI_removeQueue called with queue=0\n");
      return((int)IPT_INVALID_PAR);
   }
}

/*******************************************************************************
NAME:     MDComAPI_getMsg
ABSTRACT: Get message from a message queue, if there is any.
RETURNS:  MD_QUEUE_NOT_EMPTY(= 1) if there was a message
          MD_QUEUE_EMPTY(= 0) if no message available
          else error code
*/
int MDComAPI_getMsg(
   MD_QUEUE queue,      /* Queue */
   MSG_INFO *pMsg,      /* Buffer for received message */
   char   **ppData,     /* Pointer to pointer to application data buffer.
                           If <> 0 data is copied to the caller buffer, 
                           otherwise the pointer will be set to the buffer 
                           allocated by IPTCom and the application has to call 
                           MDComAPI_freeBuf to free the buffer */
   UINT32 *pDataLength, /* Pointer to size of buffer. 
                           At entry size of buffer, 
                           at exit no of bytes written. */
   int    timeOut)      /* Time-out value.  0 = No wait
                                           -1 = Wait until there is a message */
{
   int res;
   int res2;
   UINT32 i;
   QUEUE_MSG queueMsg;

   /* Parameter check */
   if (!queue)
   {
      IPTVosPrint0(IPT_ERR, "ERROR MDComAPI_getMsg called with queue=0 \n");
      return((int)IPT_INVALID_PAR);
   }
   else if (!pMsg)
   {
      IPTVosPrint0(IPT_ERR, "ERROR MDComAPI_getMsg called with pMsg=0\n");
      return((int)IPT_INVALID_PAR);
   }
   else if ((timeOut != IPT_WAIT_FOREVER) &&
            (timeOut != IPT_NO_WAIT))
   {
      IPTVosPrint1(IPT_ERR,
                   "ERROR MDComAPI_getMsg called with wrong timeout value=%d\n",
                   timeOut);
      return((int)IPT_INVALID_PAR);
   }

   queueMsg.pMsgData = NULL;
   res = IPTVosReceiveMsgQueue((IPT_QUEUE *)((void *)&queue), (char*)&queueMsg,
                               sizeof(QUEUE_MSG), timeOut);

   if (res == sizeof(QUEUE_MSG))
   {
      /* Any queue data buffer? */
      if (queueMsg.pMsgData != NULL)
      {
         if (!ppData)
         {
            IPTVosPrint0(IPT_ERR,
                         "ERROR MDComAPI_getMsg called with ppData=0\n");
            return((int)IPT_INVALID_PAR);
         }
         
         /* Buffer allocated by the caller? */
         if (*ppData != 0)
         {
            /* Size of the caller buffer great enough? */
            if (queueMsg.msgLength <= *pDataLength)
            {
               for (i=0; i<queueMsg.msgLength; i++)
               {
                  (*ppData)[i] = queueMsg.pMsgData[i];
               }

               *pDataLength = queueMsg.msgLength;   
         
               res = MD_QUEUE_NOT_EMPTY;   
            }
            else
            {
               IPTVosPrint2(IPT_WARN,
                     "Received data length=%d greater than appl buf length=%d\n",
                            queueMsg.msgLength,
                            *pDataLength);
               *pDataLength  = 0;   
         
               res = (int)IPT_ILLEGAL_SIZE;
            }
           
            res2 = IPTVosFree((unsigned char *)queueMsg.pMsgData);
            if(res2 != 0)
            {
              IPTVosPrint1(IPT_ERR,
                        "MDComAPI_getMsg failed to free data memory, code=%#x\n",
                           res);
            }
         }
         else
         {
            IPTGLOBAL(md.mdCnt.mdNotReturnedBuffers)++;
            *ppData = queueMsg.pMsgData;
            *pDataLength = queueMsg.msgLength;   
            res = MD_QUEUE_NOT_EMPTY;   
         } 
      }
      else
      {
         if (pDataLength != 0)
         {
            *pDataLength = 0;   
         }
         res = MD_QUEUE_NOT_EMPTY;   
      }
      
      *pMsg = queueMsg.msgInfo;
      
      return(res);
   }
   else if (res == 0)
   {
      return MD_QUEUE_EMPTY;
   }
   else if (res > 0)
   {
      return((int)IPT_QUEUE_ERR);
   }
   else
   {
      return(res);
   }
}

/*******************************************************************************
NAME:     MDComAPI_freeBuf
ABSTRACT: Free allocated memory  
RETURNS:  0 if OK, !=0 if not.
*/
int MDComAPI_freeBuf(
   char *pBuf)  /* Pointer to buffer to be released */
{
   int res;
 
   res = IPTVosFree((unsigned char *)pBuf);
   if(res != 0)
   {
     IPTVosPrint1(IPT_ERR,
                  "MDComAPI_freeBuf failed to free data memory, code=%#x\n",
                  res);
     return((int)IPT_ERROR);
   }
   else
   {
      IPTGLOBAL(md.mdCnt.mdNotReturnedBuffers)--;
      return((int)IPT_OK);
   }
}

/*******************************************************************************
NAME:     MDCom_clearStatistic
ABSTRACT: Clear stastistic counters.
RETURNS:  -
*/
int MDCom_clearStatistic(void)
{
   int ret = IPT_OK;
   UINT32 i;
   COMID_ITEM       *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;

   IPTGLOBAL(md.mdCnt.mdStatisticsStarttime) = IPTVosGetMilliSecTimer();
   IPTGLOBAL(md.mdCnt.mdInPackets) = 0;         
   IPTGLOBAL(md.mdCnt.mdInFCSErrors) = 0;       
   IPTGLOBAL(md.mdCnt.mdInProtocolErrors) = 0;  
   IPTGLOBAL(md.mdCnt.mdInTopoErrors) = 0;      
   IPTGLOBAL(md.mdCnt.mdInNoListeners) = 0;     
/* Shall not be cleared 
   IPTGLOBAL(md.mdCnt.mdNotReturnedBuffers) = 0;
*/
   IPTGLOBAL(md.mdCnt.mdOutPackets) = 0;        
   IPTGLOBAL(md.mdCnt.mdOutRetransmissions) = 0;
   
   if (IPTGLOBAL(md.mdComInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pComIdItem = (COMID_ITEM*)(IPTGLOBAL(md.listTables.comidListTableHdr.pTable));/*lint !e826 Type cast OK */
         if (pComIdItem)
         {
            for (i=0; i<IPTGLOBAL(md.listTables.comidListTableHdr.nItems); i++)
            {
               pComIdItem[i].lists.mdInPackets = 0;
               pComIdItem[i].lists.mdFrgInPackets = 0;
            }
         }
   
         pInstXFuncN = (INSTX_FUNCN_ITEM *)IPTGLOBAL(md.listTables.instXFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
         if (pInstXFuncN)
         {
            for (i=0; i<IPTGLOBAL(md.listTables.instXFuncNListTableHdr.nItems); i++)
            {
               pInstXFuncN[i].lists.mdInPackets = 0;
               pInstXFuncN[i].lists.mdFrgInPackets = 0;
            }
         }
   
         pAinstFuncN = (AINST_FUNCN_ITEM *)IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
         if (pAinstFuncN)
         {
            for (i=0; i<IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.nItems); i++)
            {
               pAinstFuncN[i].lists.mdInPackets = 0;
               pAinstFuncN[i].lists.mdFrgInPackets = 0;
            }
         }
   
         pInstXaFunc = (INSTX_AFUNC_ITEM *)IPTGLOBAL(md.listTables.instXaFuncListTableHdr.pTable);/*lint !e826 Type cast OK */
         if (pInstXaFunc)
         {
            for (i=0; i<IPTGLOBAL(md.listTables.instXaFuncListTableHdr.nItems); i++)
            {
               pInstXaFunc[i].lists.mdInPackets = 0;
               pInstXaFunc[i].lists.mdFrgInPackets = 0;
            }
         }

         IPTGLOBAL(md.listTables.aInstAfunc.mdInPackets) = 0;
         IPTGLOBAL(md.listTables.aInstAfunc.mdFrgInPackets) = 0;
      
         if (IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "MDCom_clearStatistic: IPTVosPutSem(listenerSemId) ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "MDCom_clearStatistic: IPTVosGetSem ERROR\n");
      }
   }
   else
   {
      ret = IPT_ERROR;
   }
   return(ret);
}

/*******************************************************************************
NAME:     MDCom_showStatistic
ABSTRACT: Print stastistic counters.
RETURNS:  -
*/
void MDCom_showStatistic(void)
{
   int ret;
   UINT32 i;
   COMID_ITEM       *pComIdItem;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
  
   MON_PRINTF("MD statistic\n");
   MON_PRINTF("Time in seconds since last clearing of statistics = %d\n",
          IPTVosGetMilliSecTimer() - IPTGLOBAL(md.mdCnt.mdStatisticsStarttime)); 
   MON_PRINTF("No of received MD packets = %d\n",
          IPTGLOBAL(md.mdCnt.mdInPackets));
   MON_PRINTF("No of received MD packets with FCS errors = %d\n",
          IPTGLOBAL(md.mdCnt.mdInFCSErrors));
   MON_PRINTF("No of received MD packets with wrong protocol = %d\n",
          IPTGLOBAL(md.mdCnt.mdInProtocolErrors));
   MON_PRINTF("No of received MD packets with wrong topocounter = %d\n",
          IPTGLOBAL(md.mdCnt.mdInTopoErrors));
   MON_PRINTF("No of received MD packets without listener = %d\n",
          IPTGLOBAL(md.mdCnt.mdInNoListeners));
   MON_PRINTF("No of not returned buffers = %d\n",
          IPTGLOBAL(md.mdCnt.mdNotReturnedBuffers));
   MON_PRINTF("No of transmitted MD packets = %d\n",
          IPTGLOBAL(md.mdCnt.mdOutPackets));
   MON_PRINTF("No of re-transmitted MD packets = %d\n",
          IPTGLOBAL(md.mdCnt.mdOutRetransmissions));
   
   if (IPTGLOBAL(md.mdComInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         MON_PRINTF("No of received MD packets for listener of\n");
         pComIdItem = (COMID_ITEM*)(IPTGLOBAL(md.listTables.comidListTableHdr.pTable));/*lint !e826 Type cast OK */
         if (pComIdItem)
         {
            for (i=0; i<IPTGLOBAL(md.listTables.comidListTableHdr.nItems); i++)
            {
               MON_PRINTF("ComId = %d normal listener = %d frg listener = %d\n",
                          pComIdItem[i].keyComId,
                          pComIdItem[i].lists.mdInPackets,
                          pComIdItem[i].lists.mdFrgInPackets);
            }
         }
   
         pInstXFuncN = (INSTX_FUNCN_ITEM *)IPTGLOBAL(md.listTables.instXFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
         if (pInstXFuncN)
         {
            for (i=0; i<IPTGLOBAL(md.listTables.instXFuncNListTableHdr.nItems); i++)
            {
               MON_PRINTF("URI = %s.%s normal listener = %d frg listener = %d\n",
                          pInstXFuncN[i].instName, 
                          pInstXFuncN[i].funcName,
                          pInstXFuncN[i].lists.mdInPackets,
                          pInstXFuncN[i].lists.mdFrgInPackets);
            }
         }
   
         pAinstFuncN = (AINST_FUNCN_ITEM *)IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
         if (pAinstFuncN)
         {
            for (i=0; i<IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.nItems); i++)
            {
               MON_PRINTF("URI = aInst.%s normal listener = %d frg listener = %d\n",
                          pAinstFuncN[i].funcName,
                          pAinstFuncN[i].lists.mdInPackets,
                          pAinstFuncN[i].lists.mdFrgInPackets);
            }
         }
   
         pInstXaFunc = (INSTX_AFUNC_ITEM *)IPTGLOBAL(md.listTables.instXaFuncListTableHdr.pTable);/*lint !e826 Type cast OK */
         if (pInstXaFunc)
         {
            for (i=0; i<IPTGLOBAL(md.listTables.instXaFuncListTableHdr.nItems); i++)
            {
               MON_PRINTF("URI = %s.aFunc normal listener = %d frg listener = %d\n",
                          pInstXaFunc[i].instName,
                          pInstXaFunc[i].lists.mdInPackets,
                          pInstXaFunc[i].lists.mdFrgInPackets);
            }
         }

         if ((IPTGLOBAL(md.listTables.aInstAfunc).pQueueList) ||
             (IPTGLOBAL(md.listTables.aInstAfunc).pFuncList) ||
             (IPTGLOBAL(md.listTables.aInstAfunc).pQueueFrgList) ||
             (IPTGLOBAL(md.listTables.aInstAfunc).pFuncFrgList))
         {
            MON_PRINTF("URI = aInst.aFunc normal listener = %d frg listener = %d\n",
            IPTGLOBAL(md.listTables.aInstAfunc.mdInPackets),
            IPTGLOBAL(md.listTables.aInstAfunc.mdFrgInPackets));
         }

         if (IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "MDCom_showStatistic: IPTVosPutSem(listenerSemId) ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "MDCom_showStatistic: IPTVosGetSem ERROR\n");
      }
   }
}
 
