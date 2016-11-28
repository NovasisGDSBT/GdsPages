/*                                                                            */
/*  $Id: tdcLib.c 11853 2012-02-10 17:14:13Z bloehr $                      */
/*                                                                            */
/*  DESCRIPTION    Communication Client for TDC (Library)    			  	  */
/*                 Basic Datatypes have to be defined in advance if LINUX	  */
/*					or WIN32 is chosen.										  */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS                                                                   */
/*                                                                            */
/*  DEPENDENCIES   Either the switch LINUX or WIN32 has to be set	          */
/*                                                                            */
/*  MODIFICATIONS (log starts 2010-08-11)									  */
/*   																		  */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002-2010.             */
/*                                                                            */
/*  CR-3326 (Bernd Loehr, 2012-02-10)                                         */
/*           Improvement for 3rd party use / Darwin port added.               */
/*                                                                            */
/*  CR-3477 (Bernd Löhr, 2012-02-09)                                   */
/* 			TÜV Assessment findings                                    */


// ----------------------------------------------------------------------------



#if !defined (LINUX) && !defined (WIN32) && !defined (DARWIN)
   #error "This is the Windows/Linux Version of tdcLib, i.e. LINUX or WIN32 has to be specified"
#endif


// ----------------------------------------------------------------------------

#include "tdc.h"
#include "tdcIpc.h"

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

typedef struct
{
                  T_TDC_RESULT            tdcResult;
   /*@shared@*/   const char*             pErrInfo;
} T_TDC_ERROR_INFO;

// ----------------------------------------------------------------------------

static const char*               pErrInfoUnknown = "Unknown Errorcode specified";
static const T_TDC_ERROR_INFO    errInfoTab[]    =
{
   {TDC_OK,                   "NO error occured"},
   {TDC_UNSUPPORTED_REQU,     "Request is not supported by TDC"},
   {TDC_UNKNOWN_URI,          "An unknown or invalid URI was specified"},
   {TDC_UNKNOWN_IPADDR,       "An unknown or invalid IP-Address was specified"},
   {TDC_NO_CONFIG_DATA,       "Wait until TDC has got configuration data from IPTDir-Server"},
   {TDC_NULL_POINTER_ERROR,   "A null pointer was detected for a mandatory API parameter"},
   {TDC_NOT_ENOUGH_MEMORY,    "There is not enough memory to store the data"},
   {TDC_NO_MATCHING_ENTRY,    "TDC didn't find a matching entry"},
   {TDC_MUST_FINISH_INIT,     "TDC could not yet finish basic initialization"},
   {TDC_INVALID_LABEL,        "An unknown or invalid Label was specified"},
   {TDC_WRONG_TOPOCOUNT,      "The specified topo counter no longer matches the configuration"},
   {TDC_ERROR,                "A general Error occured"},
};

// ----------------------------------------------------------------------------


/*@null@*/ 
static  T_TDC_MUTEX_ID                    tdcClientMutexId = NULL;
#define MUST_CREATE_MUTEX(mutexId)        (mutexId == NULL)

static  T_TDC_SOCKET                      tdclSockFd = TDC_INVALID_SOCKET;
#define MUST_CREATE_SOCKET(sockId)        (sockId  == TDC_INVALID_SOCKET)
#define LOCAL_HOST                        "127.0.0.1"


// ----------------------------------------------------------------------------

static T_TDC_BOOL   lockTdclMutex       (void);
static T_TDC_BOOL   unlockTdclMutex     (void);
static T_TDC_BOOL   createTdclSocket    (void);
static T_TDC_BOOL   sendAndReceiveReply (T_TDC_IPC_MSG*  pCallMsg,
                                         T_TDC_IPC_MSG*  pReplyMsg,
                                         UINT32          msgLen,
                                         UINT32          expMsgType);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void getServerAddr (char*    pServerAddr, UINT32  addrLen)
{
   const char*    pServerName = tdcGetEnv ("TDC_SERVER_IP_ADDR");

   if (pServerName == NULL)
   {
      pServerName = LOCAL_HOST;
   }

   (void) tdcStrNCpy (pServerAddr, pServerName, addrLen);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL lockTdclMutex (void)
{
   if (MUST_CREATE_MUTEX (tdcClientMutexId))
   {
      T_TDC_MUTEX_STATUS      mutexStatus;

      tdcClientMutexId = tdcCreateMutex (MOD_LIB, "ClientLib", &mutexStatus);

      if (MUST_CREATE_MUTEX (tdcClientMutexId))
      {
         return (FALSE);
      }
   }

   return (tdcMutexLock (MOD_LIB, tdcClientMutexId) == TDC_MUTEX_OK);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL unlockTdclMutex (void)
{
   return (tdcMutexUnlock (MOD_LIB, tdcClientMutexId) == TDC_MUTEX_OK);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL createTdclSocket (void)
{
   T_TDC_BOOL      bResult = TRUE;

   if (MUST_CREATE_SOCKET (tdclSockFd))
   {
      T_TDC_CONNECT_PAR      Connpar;
      T_TDC_SOCK_OPTIONS     clientSockOptions = {NULL, NULL, NULL, NULL};

      tdcMemClear (&Connpar, sizeof (Connpar));

      Connpar.pSockOpt          = &clientSockOptions;
      Connpar.timeout           = 0;                        /* MRi 2005/06/09 use default value */
      Connpar.pSockFd           = &tdclSockFd;
      Connpar.serverAddr.portNo = TDC_IPC_SERVER_PORT;

      (void) getServerAddr (Connpar.serverAddr.ipAddr, sizeof (Connpar.serverAddr.ipAddr));

      tdcInitTcpIpWinsock (MOD_LIB);

      if (!tdcTcpConnect (MOD_LIB, &Connpar))
      {
         bResult = FALSE;
      }
      else
      {
         DEBUG_INFO1 (MOD_LIB, "Socket successfully connected (%d)", tdclSockFd);
      }
   }

   return (bResult);
}


// ----------------------------------------------------------------------------

static T_TDC_BOOL sendAndReceiveReply (T_TDC_IPC_MSG*  pCallMsg,
                                       T_TDC_IPC_MSG*  pReplyMsg,
                                       UINT32          msgLen,
                                       UINT32          expMsgType)
{
   T_TDC_BOOL     bSuccess = FALSE;

   if (lockTdclMutex ())
   {
      if (createTdclSocket ())
      {
         if (!tdcSendIpcMsg (MOD_LIB, tdclSockFd,  pCallMsg))
         {
            (void) tdcCloseSocket (MOD_LIB, &tdclSockFd);
         }
         else
         {
            DEBUG_INFO (MOD_LIB, "Sent Message to TDCD");

            if (!tdcReceiveIpcMsg (MOD_LIB, tdclSockFd, pReplyMsg, msgLen))
            {
               (void) tdcCloseSocket (MOD_LIB, &tdclSockFd);
            }
            else
            {
               DEBUG_INFO (MOD_LIB, "Received Message from TDCD");

               if (pReplyMsg->head.msgType != expMsgType)
               {
                  DEBUG_WARN2 (MOD_LIB, "Unexpected replyMsg-msgType received (%d <-> %d)",
                                        TDC_IPC_REPLY_GET_ADDR_BY_NAME, pReplyMsg->head.msgType);
               }

               bSuccess = TRUE;
            }
         }
      }

      (void) unlockTdclMutex ();
   }

   return (bSuccess);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_RESULT _tdcSetDebugLevel (const char*  pPar0, const char*   pPar1, const char*   pPar2)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   pPar1 = pPar1;
   pPar2 = pPar2;

   if (pPar0 != NULL)
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_SET_DEBUG_LEVEL_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_SET_DEBUG_LEVEL;
      callMsg.head.msgLen  = IPC_CALL_SET_DEBUG_LEVEL_SIZE;

      (void) tdcStrNCpy (callMsg.data.cSetDebugLevel.dbgLevel, pPar0, (UINT32) IPT_DBG_LEVEL_STRING_LEN);

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_SET_DEBUG_LEVEL))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_SET_DEBUG_LEVEL)
              && (replyMsg.head.msgLen  == IPC_REPLY_SET_DEBUG_LEVEL_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

UINT32 tdcGetVersion (void)   
{
   UINT32            tdcVersion = (UINT32) 0;
   T_TDC_IPC_MSG     callMsg;
   T_TDC_IPC_MSG     replyMsg;
   UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_VERSION_SIZE;

   /* Create IPC-MSG, do a call and wait for reply */

   callMsg.head.msgType = TDC_IPC_CALL_GET_VERSION;
   callMsg.head.msgLen  = IPC_CALL_GET_VERSION_SIZE;

   if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_VERSION))
   {
      if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_VERSION)
           && (replyMsg.head.msgLen  == IPC_REPLY_GET_VERSION_SIZE)
         )
      {
         tdcVersion = replyMsg.data.rGetVersion.version;
      }
   }

   return (tdcVersion);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetTrnBackboneType	(UINT8*	            pTbType,
                                     T_IPT_IP_ADDR*      pGatewayIpAddr)   
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pTbType != NULL)
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_TBTYPE_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_GET_TBTYPE;
      callMsg.head.msgLen  = IPC_CALL_GET_TBTYPE_SIZE;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_TBTYPE))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_TBTYPE)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_TBTYPE_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pTbType  = replyMsg.data.rGetTbType.tbType;

            if (pGatewayIpAddr != NULL)
            {
               *pGatewayIpAddr = replyMsg.data.rGetTbType.gatewayIpAddr;
            }
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetIptState (UINT8*       pInaugState,
                             UINT8*       pTopoCnt)   
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pInaugState != NULL)
        && (pTopoCnt    != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_IPT_STATE_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_GET_IPT_STATE;
      callMsg.head.msgLen  = IPC_CALL_GET_IPT_STATE_SIZE;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_IPT_STATE))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_IPT_STATE)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_IPT_STATE_SIZE)
            )
         {
            tdcResult    = replyMsg.head.tdcResult;
            *pInaugState = replyMsg.data.rGetIptState.inaugState;
            *pTopoCnt    = replyMsg.data.rGetIptState.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetOwnIds (T_IPT_LABEL         devId,         /* who am I? */
                           T_IPT_LABEL         carId,         
                           T_IPT_LABEL         cstId)         
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (devId != NULL)
        && (carId != NULL)
        && (cstId != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_OWN_IDS_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_GET_OWN_IDS;
      callMsg.head.msgLen  = IPC_CALL_GET_OWN_IDS_SIZE;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_OWN_IDS))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_OWN_IDS)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_OWN_IDS_SIZE)
            )
         {
            // No endian conversion for strings

            tdcResult = replyMsg.head.tdcResult;
            (void) tdcStrNCpy (devId, replyMsg.data.rGetOwnIds.devId, IPT_LABEL_SIZE);
            (void) tdcStrNCpy (carId, replyMsg.data.rGetOwnIds.carId, IPT_LABEL_SIZE);
            (void) tdcStrNCpy (cstId, replyMsg.data.rGetOwnIds.cstId, IPT_LABEL_SIZE);
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetAddrByName  (const T_IPT_URI       uri,     
                                T_IPT_IP_ADDR*        pIpAddr,
                                UINT8*                pTopoCnt) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (uri      != NULL)
        && (pIpAddr  != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_ADDR_BY_NAME_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType                = TDC_IPC_CALL_GET_ADDR_BY_NAME;
      callMsg.head.msgLen                 = IPC_CALL_GET_ADDR_BY_NAME_SIZE;
      callMsg.data.cGetAddrByName.topoCnt = *pTopoCnt;

      (void) tdcStrNCpy (callMsg.data.cGetAddrByName.uri, uri, IPT_URI_SIZE);

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_ADDR_BY_NAME))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_ADDR_BY_NAME)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_ADDR_BY_NAME_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pIpAddr  = replyMsg.data.rGetAddrByName.ipAddr;
            *pTopoCnt = replyMsg.data.rGetAddrByName.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetAddrByNameExt (const T_IPT_URI       uri,     
                                  T_IPT_IP_ADDR*        pIpAddr,
                                  T_TDC_BOOL*           pIsFRG,
                                  UINT8*                pTopoCnt) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (uri      != NULL)
        && (pIpAddr  != NULL)
        && (pIsFRG   != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_ADDR_BY_NAME_EXT_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType                   = TDC_IPC_CALL_GET_ADDR_BY_NAME_EXT;
      callMsg.head.msgLen                    = IPC_CALL_GET_ADDR_BY_NAME_EXT_SIZE;
      callMsg.data.cGetAddrByNameExt.topoCnt = *pTopoCnt;

      (void) tdcStrNCpy (callMsg.data.cGetAddrByNameExt.uri, uri, IPT_URI_SIZE);

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_ADDR_BY_NAME_EXT))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_ADDR_BY_NAME_EXT)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_ADDR_BY_NAME_EXT_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pIpAddr  = replyMsg.data.rGetAddrByNameExt.ipAddr;
            *pIsFRG   = (T_TDC_BOOL) replyMsg.data.rGetAddrByNameExt.bIsFRG;
            *pTopoCnt = replyMsg.data.rGetAddrByNameExt.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetUriHostPart (T_IPT_IP_ADDR      ipAddr,
                                T_IPT_URI          uri,
                                UINT8*             pTopoCnt)   
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (uri      != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_URI_HOST_PART_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_GET_URI_HOST_PART;
      callMsg.head.msgLen  = IPC_CALL_GET_URI_HOST_PART_SIZE;

      callMsg.data.cGetUriHostPart.ipAddr  = ipAddr;
      callMsg.data.cGetUriHostPart.topoCnt = *pTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_URI_HOST_PART))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_URI_HOST_PART)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_URI_HOST_PART_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;

            (void) tdcStrNCpy (uri, replyMsg.data.rGetUriHostPart.uri, IPT_URI_SIZE);
            *pTopoCnt = replyMsg.data.rGetUriHostPart.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_RESULT tdcLabel2CarId (T_IPT_LABEL           carId,
                             UINT8*                pTopoCnt,  
                             const T_IPT_LABEL     cstLabel,
                             const T_IPT_LABEL     carLabel)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (carId    != NULL)
        && (carLabel != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_LABEL_2_CAR_ID_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType              = TDC_IPC_CALL_LABEL_2_CAR_ID;
      callMsg.head.msgLen               = IPC_CALL_LABEL_2_CAR_ID_SIZE;
      callMsg.data.cLabel2CarId.topoCnt = *pTopoCnt;

      cstLabel = (cstLabel == NULL) ? "" : cstLabel;

      (void) tdcStrNCpy (callMsg.data.cLabel2CarId.carLabel, carLabel, IPT_LABEL_SIZE);
      (void) tdcStrNCpy (callMsg.data.cLabel2CarId.cstLabel, cstLabel, IPT_LABEL_SIZE);

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_LABEL_2_CAR_ID))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_LABEL_2_CAR_ID)
              && (replyMsg.head.msgLen  == IPC_REPLY_LABEL_2_CAR_ID_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;

            (void) tdcStrNCpy (carId, replyMsg.data.rLabel2CarId.carId, IPT_LABEL_SIZE);
            *pTopoCnt = replyMsg.data.rLabel2CarId.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcAddr2CarId  (T_IPT_LABEL        carId,  
                             UINT8*             pTopoCnt,  
                             T_IPT_IP_ADDR      ipAddr) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (carId    != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_ADDR_2_CAR_ID_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_ADDR_2_CAR_ID;
      callMsg.head.msgLen  = IPC_CALL_ADDR_2_CAR_ID_SIZE;

      callMsg.data.cAddr2CarId.ipAddr  = ipAddr;
      callMsg.data.cAddr2CarId.topoCnt = *pTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_ADDR_2_CAR_ID))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_ADDR_2_CAR_ID)
              && (replyMsg.head.msgLen  == IPC_REPLY_ADDR_2_CAR_ID_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;

            (void) tdcStrNCpy (carId, replyMsg.data.rAddr2CarId.carId, IPT_LABEL_SIZE);
            *pTopoCnt = replyMsg.data.rAddr2CarId.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcLabel2CstId (T_IPT_LABEL           cstId,  
                             UINT8*                pTopoCnt,  
                             const T_IPT_LABEL     carLabel) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (cstId    != NULL)
        && (carLabel != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_LABEL_2_CST_ID_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType              = TDC_IPC_CALL_LABEL_2_CST_ID;
      callMsg.head.msgLen               = IPC_CALL_LABEL_2_CST_ID_SIZE;
      callMsg.data.cLabel2CstId.topoCnt = *pTopoCnt;

      (void) tdcStrNCpy (callMsg.data.cLabel2CstId.carLabel, carLabel, IPT_LABEL_SIZE);

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_LABEL_2_CST_ID))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_LABEL_2_CST_ID)
              && (replyMsg.head.msgLen  == IPC_REPLY_LABEL_2_CST_ID_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            (void) tdcStrNCpy (cstId, replyMsg.data.rLabel2CstId.cstId, IPT_LABEL_SIZE);
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcAddr2CstId  (T_IPT_LABEL        cstId,  
                             UINT8*             pTopoCnt,  
                             T_IPT_IP_ADDR      ipAddr) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (cstId    != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_ADDR_2_CST_ID_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_ADDR_2_CST_ID;
      callMsg.head.msgLen  = IPC_CALL_ADDR_2_CST_ID_SIZE;

      callMsg.data.cAddr2CstId.ipAddr  = ipAddr;
      callMsg.data.cAddr2CstId.topoCnt = *pTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_ADDR_2_CST_ID))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_ADDR_2_CST_ID)
              && (replyMsg.head.msgLen  == IPC_REPLY_ADDR_2_CST_ID_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pTopoCnt = replyMsg.data.rAddr2CstId.topoCnt;

            (void) tdcStrNCpy (cstId, replyMsg.data.rAddr2CstId.cstId, IPT_LABEL_SIZE);
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcCstNo2CstId (T_IPT_LABEL     cstId,     
                             UINT8*          pTopoCnt,     
                             UINT8           trnCstNo)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (cstId    != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_CSTNO_2_CST_ID_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_CSTNO_2_CST_ID;
      callMsg.head.msgLen  = IPC_CALL_CSTNO_2_CST_ID_SIZE;

      callMsg.data.cCstNo2CstId.trnCstNo = trnCstNo;
      callMsg.data.cCstNo2CstId.topoCnt  = *pTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_CSTNO_2_CST_ID))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_CSTNO_2_CST_ID)
              && (replyMsg.head.msgLen  == IPC_REPLY_CSTNO_2_CST_ID_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pTopoCnt = replyMsg.data.rCstNo2CstId.topoCnt;

            (void) tdcStrNCpy (cstId, replyMsg.data.rCstNo2CstId.cstId, IPT_LABEL_SIZE);
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcLabel2TrnCstNo (UINT8*                pTrnCstNo,
                                UINT8*                pTopoCnt,  
                                const T_IPT_LABEL     carLabel) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pTrnCstNo != NULL)
        && (carLabel  != NULL)
        && (pTopoCnt  != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_LABEL_2_TRN_CST_NO_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType                 = TDC_IPC_CALL_LABEL_2_TRN_CST_NO;
      callMsg.head.msgLen                  = IPC_CALL_LABEL_2_TRN_CST_NO_SIZE;
      callMsg.data.cLabel2TrnCstNo.topoCnt = *pTopoCnt;

      (void) tdcStrNCpy (callMsg.data.cLabel2TrnCstNo.carLabel, carLabel, IPT_LABEL_SIZE);

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_LABEL_2_TRN_CST_NO))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_LABEL_2_TRN_CST_NO)
              && (replyMsg.head.msgLen  == IPC_REPLY_LABEL_2_TRN_CST_NO_SIZE)
            )
         {
            tdcResult  = replyMsg.head.tdcResult;
            *pTrnCstNo = replyMsg.data.rLabel2TrnCstNo.trnCstNo;
            *pTopoCnt  = replyMsg.data.rLabel2TrnCstNo.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcAddr2TrnCstNo  (UINT8*             pTrnCstNo,
                                UINT8*             pTopoCnt,  
                                T_IPT_IP_ADDR      ipAddr)   
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pTrnCstNo != NULL)
        && (pTopoCnt  != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_ADDR_2_TRN_CST_NO_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_ADDR_2_TRN_CST_NO;
      callMsg.head.msgLen  = IPC_CALL_ADDR_2_TRN_CST_NO_SIZE;

      callMsg.data.cAddr2TrnCstNo.ipAddr  = ipAddr;
      callMsg.data.cAddr2TrnCstNo.topoCnt = *pTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_ADDR_2_TRN_CST_NO))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_ADDR_2_TRN_CST_NO)
              && (replyMsg.head.msgLen  == IPC_REPLY_ADDR_2_TRN_CST_NO_SIZE)
            )
         {
            tdcResult  = replyMsg.head.tdcResult;
            *pTrnCstNo = replyMsg.data.rAddr2TrnCstNo.trnCstNo;
            *pTopoCnt  = replyMsg.data.rAddr2TrnCstNo.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetTrnCstCnt (UINT8*      pCstCnt,
                              UINT8*      pTopoCnt)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pCstCnt  != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_TRN_CST_CNT_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType               = TDC_IPC_CALL_GET_TRN_CST_CNT;
      callMsg.head.msgLen                = IPC_CALL_GET_TRN_CST_CNT_SIZE;
      callMsg.data.cGetTrnCstCnt.topoCnt = *pTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_TRN_CST_CNT))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_TRN_CST_CNT)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_TRN_CST_CNT_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pCstCnt  = replyMsg.data.rGetTrnCstCnt.trnCstCnt;
            *pTopoCnt = replyMsg.data.rGetTrnCstCnt.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetCstCarCnt (UINT8*               pCarCnt,
                              UINT8*               pTopoCnt, 
                              const T_IPT_LABEL    cstLabel)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pCarCnt  != NULL)
        && (cstLabel != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_CST_CAR_CNT_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType               = TDC_IPC_CALL_GET_CST_CAR_CNT;
      callMsg.head.msgLen                = IPC_CALL_GET_CST_CAR_CNT_SIZE;
      callMsg.data.cGetCstCarCnt.topoCnt = *pTopoCnt;

      (void) tdcStrNCpy (callMsg.data.cGetCstCarCnt.cstLabel, cstLabel,  IPT_LABEL_SIZE);

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_CST_CAR_CNT))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_CST_CAR_CNT)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_CST_CAR_CNT_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pCarCnt  = replyMsg.data.rGetCstCarCnt.carCnt;
            *pTopoCnt = replyMsg.data.rGetCstCarCnt.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetCarDevCnt (UINT16*              pDevCnt,
                              UINT8*               pTopoCnt, 
                              const T_IPT_LABEL    cstLabel,
                              const T_IPT_LABEL    carLabel)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pDevCnt  != NULL)
        && (carLabel != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_CAR_DEV_CNT_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType               = TDC_IPC_CALL_GET_CAR_DEV_CNT;
      callMsg.head.msgLen                = IPC_CALL_GET_CAR_DEV_CNT_SIZE;
      callMsg.data.cGetCarDevCnt.topoCnt = *pTopoCnt;

      cstLabel = (cstLabel == NULL) ? ("") : (cstLabel);

      (void) tdcStrNCpy (callMsg.data.cGetCarDevCnt.carLabel, carLabel, IPT_LABEL_SIZE);
      (void) tdcStrNCpy (callMsg.data.cGetCarDevCnt.cstLabel, cstLabel, IPT_LABEL_SIZE);

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_CAR_DEV_CNT))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_CAR_DEV_CNT)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_CAR_DEV_CNT_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pDevCnt  = replyMsg.data.rGetCarDevCnt.devCnt;
            *pTopoCnt = replyMsg.data.rGetCarDevCnt.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetCarInfo (T_TDC_CAR_DATA*        pCarData,
                            UINT8*                 pTopoCnt, 
                            UINT16                 maxDev,   
                            const T_IPT_LABEL      cstLabel, 
                            const T_IPT_LABEL      carLabel) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pCarData != NULL)
        && (carLabel != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_CAR_INFO_SIZE (maxDev);

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_GET_CAR_INFO;
      callMsg.head.msgLen  = IPC_CALL_GET_CAR_INFO_SIZE;

      cstLabel = (cstLabel == NULL)             ? ("")     : (cstLabel);
      maxDev   = (maxDev < IPC_MAX_DEV_PER_CAR) ? (maxDev) : (IPC_MAX_DEV_PER_CAR);

      (void) tdcStrNCpy (callMsg.data.cGetCarInfo.carLabel, carLabel, IPT_LABEL_SIZE);
      (void) tdcStrNCpy (callMsg.data.cGetCarInfo.cstLabel, cstLabel, IPT_LABEL_SIZE);

      callMsg.data.cGetCarInfo.maxDev  = maxDev;
      callMsg.data.cGetCarInfo.topoCnt = *pTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_CAR_INFO))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_CAR_INFO)
              && (replyMsg.head.msgLen  >= IPC_REPLY_GET_CAR_INFO_SIZE (0))
            )
         {
            if ((tdcResult = replyMsg.head.tdcResult) == TDC_OK)
            {
               if (    (replyMsg.head.msgLen == IPC_REPLY_GET_CAR_INFO_SIZE (replyMsg.data.rGetCarInfo.devCnt))
                    && (replyMsg.data.rGetCarInfo.devCnt <= maxDev)
                  )
               {
                  UINT32      i;

                  *pTopoCnt           = replyMsg.data.rGetCarInfo.topoCnt;
                  pCarData->cstCarNo  = replyMsg.data.rGetCarInfo.cstCarNo;
                  pCarData->trnOrient = replyMsg.data.rGetCarInfo.trnOrient;
                  pCarData->cstOrient = replyMsg.data.rGetCarInfo.cstOrient;
                  pCarData->devCnt    = replyMsg.data.rGetCarInfo.devCnt;

                  (void) tdcStrNCpy (pCarData->carId,    replyMsg.data.rGetCarInfo.carId,    IPT_LABEL_SIZE);
                  (void) tdcStrNCpy (pCarData->carType,  replyMsg.data.rGetCarInfo.carType,  IPT_LABEL_SIZE);
                  (void) tdcMemCpy  (pCarData->uicIdent, replyMsg.data.rGetCarInfo.uicIdent, (UINT32) IPT_UIC_IDENTIFIER_CNT);

                  for (i = 0; i < pCarData->devCnt; i++)
                  {
                     pCarData->devData[i].hostId = replyMsg.data.rGetCarInfo.devData[i].hostId;
                     (void) tdcStrNCpy (pCarData->devData[i].devId, 
                                        replyMsg.data.rGetCarInfo.devData[i].devId, 
                                        IPT_LABEL_SIZE);
                  }
               }
               else
               {
                  tdcResult = TDC_UNSUPPORTED_REQU;
               }
            }
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetUicState (UINT8*       pInaugState,
                             UINT8*       pTopoCnt)   
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pInaugState != NULL)
        && (pTopoCnt    != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_UIC_STATE_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_GET_UIC_STATE;
      callMsg.head.msgLen  = IPC_CALL_GET_UIC_STATE_SIZE;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_UIC_STATE))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_UIC_STATE)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_UIC_STATE_SIZE)
            )
         {
            tdcResult    = replyMsg.head.tdcResult;
            *pInaugState = replyMsg.data.rGetUicState.inaugState;
            *pTopoCnt    = replyMsg.data.rGetUicState.topoCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetUicGlobalData (T_TDC_UIC_GLOB_DATA*   pGlobData,
                                  UINT8*                 pTopoCnt)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pGlobData != NULL)
        && (pTopoCnt  != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_UIC_GLOB_DATA_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType                 = TDC_IPC_CALL_GET_UIC_GLOB_DATA;
      callMsg.head.msgLen                  = IPC_CALL_GET_UIC_GLOB_DATA_SIZE;
      callMsg.data.cGetUicGlobData.topoCnt = *pTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_UIC_GLOB_DATA))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_UIC_GLOB_DATA)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_UIC_GLOB_DATA_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pTopoCnt = replyMsg.data.rGetUicGlobData.topoCnt;

            (void) tdcMemCpy (pGlobData->confPos, replyMsg.data.rGetUicGlobData.confPos, IPT_UIC_CONF_POS_CNT);

            pGlobData->confPosAvail   = replyMsg.data.rGetUicGlobData.confPosAvail;
            pGlobData->operatAvail    = replyMsg.data.rGetUicGlobData.operatAvail;
            pGlobData->natApplAvail   = replyMsg.data.rGetUicGlobData.natApplAvail;
            pGlobData->cstPropAvail   = replyMsg.data.rGetUicGlobData.cstPropAvail;
            pGlobData->carPropAvail   = replyMsg.data.rGetUicGlobData.carPropAvail;
            pGlobData->seatResNoAvail = replyMsg.data.rGetUicGlobData.seatResNoAvail;
            pGlobData->inaugFrameVer  = replyMsg.data.rGetUicGlobData.inaugFrameVer;
            pGlobData->rDataVer       = replyMsg.data.rGetUicGlobData.rDataVer;
            pGlobData->inaugState     = replyMsg.data.rGetUicGlobData.inaugState;
            pGlobData->topoCnt        = replyMsg.data.rGetUicGlobData.topoCnt;
            pGlobData->orient         = replyMsg.data.rGetUicGlobData.orient;
            pGlobData->notAllConf     = replyMsg.data.rGetUicGlobData.notAllConf;
            pGlobData->confCancelled  = replyMsg.data.rGetUicGlobData.confCancelled;
            pGlobData->trnCarCnt      = replyMsg.data.rGetUicGlobData.trnCarCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcGetUicCarData (T_TDC_UIC_CAR_DATA*    pCarData,
                               UINT8*                 pTopoCnt, 
                               UINT8                  carSeqNo)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pCarData != NULL)
        && (pTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_GET_UIC_CAR_DATA_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType                 = TDC_IPC_CALL_GET_UIC_CAR_DATA;
      callMsg.head.msgLen                  = IPC_CALL_GET_UIC_CAR_DATA_SIZE;
      callMsg.data.cGetUicCarData.carSeqNo = carSeqNo;
      callMsg.data.cGetUicCarData.topoCnt  = *pTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_GET_UIC_CAR_DATA))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_GET_UIC_CAR_DATA)
              && (replyMsg.head.msgLen  == IPC_REPLY_GET_UIC_CAR_DATA_SIZE)
            )
         {
            tdcResult = replyMsg.head.tdcResult;
            *pTopoCnt = replyMsg.data.rGetUicCarData.topoCnt;

            (void) tdcMemCpy (pCarData->cstProp,  replyMsg.data.rGetUicCarData.cstProp,  IPT_MAX_UIC_CST_NO);
            (void) tdcMemCpy (pCarData->carProp,  replyMsg.data.rGetUicCarData.carProp,  IPT_UIC_CAR_PROPERTY_CNT);
            (void) tdcMemCpy (pCarData->uicIdent, replyMsg.data.rGetUicCarData.uicIdent, IPT_UIC_IDENTIFIER_CNT);

            pCarData->cstSeqNo      = replyMsg.data.rGetUicCarData.trnCstNo;
            pCarData->carSeqNo      = replyMsg.data.rGetUicCarData.carSeqNo;
            pCarData->seatResNo     = replyMsg.data.rGetUicCarData.seatResNo;
            pCarData->contrCarCnt   = replyMsg.data.rGetUicCarData.contrCarCnt;
            pCarData->operat        = replyMsg.data.rGetUicCarData.operat;
            pCarData->owner         = replyMsg.data.rGetUicCarData.owner;
            pCarData->natAppl       = replyMsg.data.rGetUicCarData.natAppl;
            pCarData->natVer        = replyMsg.data.rGetUicCarData.natVer;
            pCarData->trnOrient     = replyMsg.data.rGetUicCarData.trnOrient;
            pCarData->cstOrient     = replyMsg.data.rGetUicCarData.cstOrient;
            pCarData->isLeading     = replyMsg.data.rGetUicCarData.isLeading;
            pCarData->isLeadRequ    = replyMsg.data.rGetUicCarData.isLeadRequ;
            pCarData->trnSwInCarCnt = replyMsg.data.rGetUicCarData.trnSwInCarCnt;
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcLabel2UicCarSeqNo (UINT8*                pCarSeqNo,
                                   UINT8*                pIptTopoCnt,
                                   UINT8*                pUicTopoCnt,
                                   const T_IPT_LABEL     cstLabel, 
                                   const T_IPT_LABEL     carLabel) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pCarSeqNo   != NULL)
        && (carLabel    != NULL)
        && (pIptTopoCnt != NULL)
        && (pUicTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType                       = TDC_IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO;
      callMsg.head.msgLen                        = IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO_SIZE;
      callMsg.data.cLabel2UicCarSeqNo.iptTopoCnt = *pIptTopoCnt;
      callMsg.data.cLabel2UicCarSeqNo.uicTopoCnt = *pUicTopoCnt;

      cstLabel = (cstLabel == NULL) ? ("") : (cstLabel);

      (void) tdcStrNCpy (callMsg.data.cLabel2UicCarSeqNo.carLabel, carLabel, IPT_LABEL_SIZE);
      (void) tdcStrNCpy (callMsg.data.cLabel2UicCarSeqNo.cstLabel, cstLabel, IPT_LABEL_SIZE);

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO)
              && (replyMsg.head.msgLen  == IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO_SIZE)
            )
         {
            tdcResult    = replyMsg.head.tdcResult;
            *pCarSeqNo   = replyMsg.data.rLabel2UicCarSeqNo.carSeqNo;      
            *pIptTopoCnt = replyMsg.data.rLabel2UicCarSeqNo.iptTopoCnt;      
            *pUicTopoCnt = replyMsg.data.rLabel2UicCarSeqNo.uicTopoCnt;      
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcAddr2UicCarSeqNo (UINT8*              pCarSeqNo, 
                                  UINT8*              pIptTopoCnt,
                                  UINT8*              pUicTopoCnt,
                                  T_IPT_IP_ADDR       ipAddr)    
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (pCarSeqNo   != NULL)
        && (pIptTopoCnt != NULL)
        && (pUicTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO;
      callMsg.head.msgLen  = IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO_SIZE;

      callMsg.data.cAddr2UicCarSeqNo.ipAddr     = ipAddr;
      callMsg.data.cAddr2UicCarSeqNo.iptTopoCnt = *pIptTopoCnt;
      callMsg.data.cAddr2UicCarSeqNo.uicTopoCnt = *pUicTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO)
              && (replyMsg.head.msgLen  == IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO_SIZE)
            )
         {
            tdcResult    = replyMsg.head.tdcResult;
            *pCarSeqNo   = replyMsg.data.rAddr2UicCarSeqNo.carSeqNo;      
            *pIptTopoCnt = replyMsg.data.rAddr2UicCarSeqNo.iptTopoCnt;      
            *pUicTopoCnt = replyMsg.data.rAddr2UicCarSeqNo.uicTopoCnt;      
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

T_TDC_RESULT tdcUicCarSeqNo2Ids (T_IPT_LABEL         cstId, 
                                 T_IPT_LABEL         carId, 
                                 UINT8*              pIptTopoCnt,
                                 UINT8*              pUicTopoCnt,
                                 UINT8               carSeqNo) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (    (cstId       != NULL)
        && (carId       != NULL)
        && (pIptTopoCnt != NULL)
        && (pUicTopoCnt != NULL)
      )
   {
      T_TDC_IPC_MSG     callMsg;
      T_TDC_IPC_MSG     replyMsg;
      UINT32            maxReplyLen = TDC_IPC_HEAD_SIZE + IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS_SIZE;

      tdcResult = TDC_ERROR;

      /* Create IPC-MSG, do a call and wait for reply */

      callMsg.head.msgType = TDC_IPC_CALL_UIC_CAR_SEQ_NO_2_IDS;
      callMsg.head.msgLen  = IPC_CALL_UIC_CAR_SEQ_NO_2_IDS_SIZE;

      callMsg.data.cUicCarSeqNo2Ids.carSeqNo   = carSeqNo;
      callMsg.data.cUicCarSeqNo2Ids.iptTopoCnt = *pIptTopoCnt;
      callMsg.data.cUicCarSeqNo2Ids.uicTopoCnt = *pUicTopoCnt;

      if (sendAndReceiveReply (&callMsg, &replyMsg, maxReplyLen, TDC_IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS))
      {
         if (    (replyMsg.head.msgType == TDC_IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS)
              && (replyMsg.head.msgLen  == IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS_SIZE)
            )
         {
            tdcResult    = replyMsg.head.tdcResult;
            *pIptTopoCnt = replyMsg.data.rUicCarSeqNo2Ids.iptTopoCnt;      
            *pUicTopoCnt = replyMsg.data.rUicCarSeqNo2Ids.uicTopoCnt;     

            (void) tdcStrNCpy (cstId, replyMsg.data.rUicCarSeqNo2Ids.cstId, IPT_LABEL_SIZE);
            (void) tdcStrNCpy (carId, replyMsg.data.rUicCarSeqNo2Ids.carId, IPT_LABEL_SIZE);
         }
         else
         {
            tdcResult = TDC_UNSUPPORTED_REQU;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

const char* tdcGetErrorString (int     errCode)
{
   const char*       pErrInfo  = pErrInfoUnknown;
   T_TDC_RESULT      tdcResult = (T_TDC_RESULT) errCode;
   int               i;

   for (i = 0; i < TAB_SIZE (errInfoTab); i++)
   {
      if (tdcResult == errInfoTab[i].tdcResult)
      {
         pErrInfo = errInfoTab[i].pErrInfo;
         break;
      }
   }

   return (pErrInfo);
}

// ----------------------------------------------------------------------------

