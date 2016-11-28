/*
 *  $Id: tdcTIpc.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    Ipc acces to Daemon for IP-Train Directory Client (TDC)
 *
 *  AUTHOR         M.Ritz
 *
 *  REMARKS
 *
 *  DEPENDENCIES   Either the switch VXWORKS, INTEGRITY, LINUX or WIN32 has to be set
 *
 *  MODIFICATIONS (log starts 2010-08-11)
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *          Adding corrections for TÜV-Assessment
 *
 *  All rights reserved. Reproduction, modification, use or disclosure
 *  to third parties without express authority is forbidden.
 *  Copyright Bombardier Transportation GmbH, Germany, 2002-2012.
 *
 */

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#include "tdc.h"
#include "tdcThread.h"
#include "tdcIpc.h"
#include "tdcInit.h"

// ----------------------------------------------------------------------------

static const T_TDC_BOOL             always      = TRUE;
static const T_THREAD_FRAME        ipcClientId =
{
   &always,
   APP_NAME,
   TASKNAME_IPCT,
   TASK_TYPE,
   DEFAULT_PRIORITY,
   DEFAULT_STACK_SIZE,
   0,
   serviceThread,
   0
};

// ----------------------------------------------------------------------------

typedef struct
{
   T_THREAD_ID             threadId;
   T_TDC_SOCKET            connFd;
} T_IPC_CLIENT_THREAD_TAB;

#define MAX_TDC_CLIENT_THREADS            10
static T_IPC_CLIENT_THREAD_TAB            tdcClientThreadTab[MAX_TDC_CLIENT_THREADS];

// ----------------------------------------------------------------------------

static       T_TDC_SOCKET                 listenFd     = TDC_INVALID_SOCKET;
static const T_TDC_SOCKET_REUSEADDR       reuseAddrOpt = {TRUE};

static const T_TDC_SOCK_OPTIONS           servSockOpt  = {NULL, NULL, NULL, &reuseAddrOpt};

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleIpcCall           (const T_TDC_IPC_MSG*   pCallMsg,
                                                 T_TDC_IPC_MSG*   pReplyMsg,
                                                 T_TDC_SOCKET     fd);

static T_TDC_BOOL handleGetVersion        (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetTrnBackboneType(const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetIptState       (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetOwnIds         (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetAddrByName     (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetAddrByNameExt  (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetUriHostPart    (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleLabel2CarId       (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleAddr2CarId        (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleLabel2CstId       (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleAddr2CstId        (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleCstNo2CstId       (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleLabel2TrnCstNo    (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleAddr2TrnCstNo     (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetTrnCstCnt      (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetCstCarCnt      (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetCarDevCnt      (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetCarInfo        (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetUicState       (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetUicGlobData    (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleGetUicCarData     (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleLabel2UicCarSeqNo (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleAddr2UicCarSeqNo  (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleUicCarSeqNo2Ids   (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);
static T_TDC_BOOL handleSetDebugLevel     (const T_TDC_IPC_MSG*   pCallMsg, T_TDC_IPC_MSG*   pReplyMsg, T_TDC_SOCKET     fd);

// ----------------------------------------------------------------------------

static INT32      getFreeThreadIndex      (void);
static void       doService               (void);
static T_TDC_BOOL terminateIpcServ        (void);
static T_TDC_BOOL terminateAllIpcClients  (void);

static void       invalidMsgLenWarning    (const char*      pFuncName,
                                           UINT32           expLen,
                                           UINT32           recvLen);

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void invalidMsgLenWarning (const char*      pFuncName,
                                  UINT32           expLen,
                                  UINT32           recvLen)
{
   char     text[81];

   (void) tdcSNPrintf (text, (UINT32) sizeof (text), "%s: Invalid Message-Len - %d <-> %d", pFuncName, expLen, recvLen);

   DEBUG_WARN (MOD_IPC, text);
}

// ----------------------------------------------------------------------------
/*lint -save -esym(429, pReplyMsg) pReplyMsg is not custotory (this is also true for the rest of the functions!*/
static T_TDC_BOOL handleGetVersion (const T_TDC_IPC_MSG*    pCallMsg, 
                                          T_TDC_IPC_MSG*    pReplyMsg, 
                                          T_TDC_SOCKET      fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_VERSION_SIZE)
   {
      DEBUG_INFO (MOD_IPC, "handleGetVersion called");

      pReplyMsg->head.msgType             = TDC_IPC_REPLY_GET_VERSION;
      pReplyMsg->head.msgLen              = IPC_REPLY_GET_VERSION_SIZE;
      pReplyMsg->head.tdcResult           = TDC_OK;
      pReplyMsg->data.rGetVersion.version = tdcGetVersion ();

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetVersion", IPC_CALL_GET_VERSION_SIZE, pCallMsg->head.msgLen);
   }

   return (FALSE);
}
// ----------------------------------------------------------------------------
static T_TDC_BOOL handleGetTrnBackboneType (const T_TDC_IPC_MSG*     pCallMsg, 
                                                  T_TDC_IPC_MSG*     pReplyMsg, 
                                                  T_TDC_SOCKET       fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_TBTYPE_SIZE)
   {
      DEBUG_INFO (MOD_IPC, "handleGetTrnBackboneType called");

      pReplyMsg->head.msgType   = TDC_IPC_REPLY_GET_TBTYPE;
      pReplyMsg->head.msgLen    = IPC_REPLY_GET_TBTYPE_SIZE;
      pReplyMsg->head.tdcResult = tdcGetTrnBackboneType (&pReplyMsg->data.rGetTbType.tbType,
                                                         &pReplyMsg->data.rGetTbType.gatewayIpAddr);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetTrnBackboneType", IPC_CALL_GET_TBTYPE_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------
static T_TDC_BOOL handleGetIptState (const T_TDC_IPC_MSG*   pCallMsg,
                                           T_TDC_IPC_MSG*   pReplyMsg,
                                           T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_IPT_STATE_SIZE)
   {
      UINT8             inaugState;
      UINT8             topoCnt;

      DEBUG_INFO (MOD_IPC, "handleGetIptState called");

      pReplyMsg->head.msgType                 = TDC_IPC_REPLY_GET_IPT_STATE;
      pReplyMsg->head.msgLen                  = IPC_REPLY_GET_IPT_STATE_SIZE;
      pReplyMsg->head.tdcResult               = tdcGetIptState (&inaugState, &topoCnt);
      pReplyMsg->data.rGetIptState.inaugState = inaugState;
      pReplyMsg->data.rGetIptState.topoCnt    = topoCnt;

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetIptState", IPC_CALL_GET_IPT_STATE_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetOwnIds (const T_TDC_IPC_MSG*   pCallMsg,
                                         T_TDC_IPC_MSG*   pReplyMsg,
                                         T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_OWN_IDS_SIZE)
   {
      DEBUG_INFO (MOD_IPC, "handleGetOwnIds called");
      pReplyMsg->data.rGetOwnIds.devId[0]  = '\0';
      pReplyMsg->data.rGetOwnIds.carId[0]  = '\0';
      pReplyMsg->data.rGetOwnIds.cstId[0]  = '\0';

      pReplyMsg->head.msgType   = TDC_IPC_REPLY_GET_OWN_IDS;
      pReplyMsg->head.msgLen    = IPC_REPLY_GET_OWN_IDS_SIZE;
      pReplyMsg->head.tdcResult = tdcGetOwnIds (pReplyMsg->data.rGetOwnIds.devId,
                                                pReplyMsg->data.rGetOwnIds.carId,
                                                pReplyMsg->data.rGetOwnIds.cstId);

      // No endian conversion for strings

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetOwnIds", IPC_CALL_GET_OWN_IDS_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetAddrByName (const T_TDC_IPC_MSG*    pCallMsg,
                                             T_TDC_IPC_MSG*    pReplyMsg,
                                             T_TDC_SOCKET      fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_ADDR_BY_NAME_SIZE)
   {
      T_IPT_URI         uri;

      (void) tdcStrNCpy (uri, pCallMsg->data.cGetAddrByName.uri, IPT_URI_SIZE);

      {
         char              text[sizeof (T_IPT_URI) + 40];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text), "handleGetAddrByName (%s) called", uri);
         text[sizeof (text) - 1] = '\0';

         DEBUG_INFO (MOD_IPC, text);
      }

      pReplyMsg->data.rGetAddrByName.topoCnt = pCallMsg->data.cGetAddrByName.topoCnt;

      pReplyMsg->head.msgType   = TDC_IPC_REPLY_GET_ADDR_BY_NAME;
      pReplyMsg->head.msgLen    = IPC_REPLY_GET_ADDR_BY_NAME_SIZE;
      pReplyMsg->head.tdcResult = tdcGetAddrByName (uri, 
                                                    &pReplyMsg->data.rGetAddrByName.ipAddr, 
                                                    &pReplyMsg->data.rGetAddrByName.topoCnt);

      return (tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetAddrByName", IPC_CALL_GET_ADDR_BY_NAME_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetAddrByNameExt (const T_TDC_IPC_MSG*    pCallMsg,
                                                T_TDC_IPC_MSG*    pReplyMsg,
                                                T_TDC_SOCKET      fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_ADDR_BY_NAME_EXT_SIZE)
   {
      T_IPT_URI         uri;
      T_TDC_BOOL        bIsFRG;

      (void) tdcStrNCpy (uri, pCallMsg->data.cGetAddrByNameExt.uri, IPT_URI_SIZE);

      {
         char              text[sizeof (T_IPT_URI) + 40];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text), "handleGetAddrByNameExt (%s) called", uri);
         text[sizeof (text) - 1] = '\0';

         DEBUG_INFO (MOD_IPC, text);
      }

      pReplyMsg->data.rGetAddrByNameExt.topoCnt = pCallMsg->data.cGetAddrByNameExt.topoCnt;

      pReplyMsg->head.msgType   = TDC_IPC_REPLY_GET_ADDR_BY_NAME_EXT;
      pReplyMsg->head.msgLen    = IPC_REPLY_GET_ADDR_BY_NAME_EXT_SIZE;
      pReplyMsg->head.tdcResult = tdcGetAddrByNameExt (uri, 
                                                       &pReplyMsg->data.rGetAddrByNameExt.ipAddr, 
                                                       &bIsFRG,
                                                       &pReplyMsg->data.rGetAddrByNameExt.topoCnt);
      pReplyMsg->data.rGetAddrByNameExt.bIsFRG = (INT32) bIsFRG;

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetAddrByNamExte", IPC_CALL_GET_ADDR_BY_NAME_EXT_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetUriHostPart (const T_TDC_IPC_MSG*   pCallMsg,
                                              T_TDC_IPC_MSG*   pReplyMsg,
                                              T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_URI_HOST_PART_SIZE)
   {
      DEBUG_INFO1 (MOD_IPC, "handleGetUriHostPart (x%08x) called", pCallMsg->data.cGetUriHostPart.ipAddr);

      pReplyMsg->data.rGetUriHostPart.topoCnt = pCallMsg->data.cGetUriHostPart.topoCnt;

      pReplyMsg->data.rGetUriHostPart.uri[0]  = '\0';
      pReplyMsg->head.msgType                 = TDC_IPC_REPLY_GET_URI_HOST_PART;
      pReplyMsg->head.msgLen                  = IPC_REPLY_GET_URI_HOST_PART_SIZE;
      pReplyMsg->head.tdcResult               = tdcGetUriHostPart (pCallMsg->data.cGetUriHostPart.ipAddr,
                                                                   pReplyMsg->data.rGetUriHostPart.uri,
                                                                   &pReplyMsg->data.rGetUriHostPart.topoCnt);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetUriHostPart", IPC_CALL_GET_URI_HOST_PART_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleLabel2CarId (const T_TDC_IPC_MSG*      pCallMsg,
                                           T_TDC_IPC_MSG*      pReplyMsg,
                                           T_TDC_SOCKET        fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_LABEL_2_CAR_ID_SIZE)
   {
      T_IPT_LABEL    carLabel;
      T_IPT_LABEL    cstLabel;

      (void) tdcStrNCpy (carLabel, pCallMsg->data.cLabel2CarId.carLabel, IPT_LABEL_SIZE);
      (void) tdcStrNCpy (cstLabel, pCallMsg->data.cLabel2CarId.cstLabel, IPT_LABEL_SIZE);

      {
         char              text[80];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text),
                      "handleLabel2CarId cst(%s), car(%s) called", cstLabel, carLabel);
         text[sizeof (text) - 1] = '\0';

         DEBUG_INFO (MOD_IPC, text);
      }

      pReplyMsg->data.rLabel2CarId.topoCnt   = pCallMsg->data.cLabel2CarId.topoCnt;
      pReplyMsg->data.rLabel2CarId.carId[0]  = '\0';
      pReplyMsg->head.msgType                = TDC_IPC_REPLY_LABEL_2_CAR_ID;
      pReplyMsg->head.msgLen                 = IPC_REPLY_LABEL_2_CAR_ID_SIZE;
      pReplyMsg->head.tdcResult              = tdcLabel2CarId (pReplyMsg->data.rLabel2CarId.carId,
                                                               &pReplyMsg->data.rLabel2CarId.topoCnt,
                                                               cstLabel,
                                                               carLabel);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("Label2CarId", IPC_CALL_LABEL_2_CAR_ID_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleAddr2CarId (const T_TDC_IPC_MSG*    pCallMsg,
                                          T_TDC_IPC_MSG*    pReplyMsg,
                                          T_TDC_SOCKET      fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_ADDR_2_CAR_ID_SIZE)
   {
      DEBUG_INFO1 (MOD_IPC, "handleAddr2CarId (x%08x) called", pCallMsg->data.cAddr2CarId.ipAddr);

      pReplyMsg->data.rAddr2CarId.topoCnt  = pCallMsg->data.cAddr2CarId.topoCnt;
      pReplyMsg->data.rAddr2CarId.carId[0] = '\0';
      pReplyMsg->head.msgType              = TDC_IPC_REPLY_ADDR_2_CAR_ID;
      pReplyMsg->head.msgLen               = IPC_REPLY_ADDR_2_CAR_ID_SIZE;
      pReplyMsg->head.tdcResult            = tdcAddr2CarId (pReplyMsg->data.rAddr2CarId.carId,
                                                            &pReplyMsg->data.rAddr2CarId.topoCnt,
                                                            pCallMsg->data.cAddr2CarId.ipAddr);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("Addr2CarId", IPC_CALL_ADDR_2_CAR_ID_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleLabel2CstId (const T_TDC_IPC_MSG*   pCallMsg,
                                           T_TDC_IPC_MSG*   pReplyMsg,
                                           T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_LABEL_2_CST_ID_SIZE)
   {
      T_IPT_LABEL    carLabel;

      (void) tdcStrNCpy (carLabel, pCallMsg->data.cLabel2CstId.carLabel, IPT_LABEL_SIZE);

      {
         char              text[120];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text), "handleLabel2CstId car(%s) called", carLabel);
         text[sizeof (text) - 1] = '\0';

         DEBUG_INFO (MOD_IPC, text);
      }

      pReplyMsg->data.rLabel2CstId.topoCnt  =pCallMsg->data.cLabel2CstId.topoCnt;
      pReplyMsg->data.rLabel2CstId.cstId[0] = '\0';
      pReplyMsg->head.msgType               = TDC_IPC_REPLY_LABEL_2_CST_ID;
      pReplyMsg->head.msgLen                = IPC_REPLY_LABEL_2_CST_ID_SIZE;
      pReplyMsg->head.tdcResult             = tdcLabel2CstId (pReplyMsg->data.rLabel2CstId.cstId,
                                                              &pReplyMsg->data.rLabel2CstId.topoCnt,
                                                              carLabel);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("Label2CstId", IPC_CALL_LABEL_2_CST_ID_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleAddr2CstId  (const T_TDC_IPC_MSG*   pCallMsg,
                                           T_TDC_IPC_MSG*   pReplyMsg,
                                           T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_ADDR_2_CST_ID_SIZE)
   {
      DEBUG_INFO1 (MOD_IPC, "handleAddr2CstId (x%08x) called", pCallMsg->data.cAddr2CstId.ipAddr);

      pReplyMsg->data.rAddr2CstId.topoCnt  = pCallMsg->data.cAddr2CstId.topoCnt;
      pReplyMsg->data.rAddr2CstId.cstId[0] = '\0';
      pReplyMsg->head.msgType              = TDC_IPC_REPLY_ADDR_2_CST_ID;
      pReplyMsg->head.msgLen               = IPC_REPLY_ADDR_2_CST_ID_SIZE;
      pReplyMsg->head.tdcResult            = tdcAddr2CstId (pReplyMsg->data.rAddr2CstId.cstId,
                                                            &pReplyMsg->data.rAddr2CstId.topoCnt,
                                                            pCallMsg->data.cAddr2CstId.ipAddr);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("Addr2CstId", IPC_CALL_ADDR_2_CST_ID_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleCstNo2CstId (const T_TDC_IPC_MSG*   pCallMsg,
                                           T_TDC_IPC_MSG*   pReplyMsg,
                                           T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_CSTNO_2_CST_ID_SIZE)
   {
      DEBUG_INFO2 (MOD_IPC, "handleCstNo2CstId (cstNo=%d, topoCnt=%d) called", 
                   pCallMsg->data.cCstNo2CstId.trnCstNo, pCallMsg->data.cCstNo2CstId.topoCnt);

      pReplyMsg->data.rCstNo2CstId.topoCnt  = pCallMsg->data.cCstNo2CstId.topoCnt;
      pReplyMsg->data.rCstNo2CstId.cstId[0] = '\0';
      pReplyMsg->head.msgType               = TDC_IPC_REPLY_CSTNO_2_CST_ID;
      pReplyMsg->head.msgLen                = IPC_REPLY_CSTNO_2_CST_ID_SIZE;
      pReplyMsg->head.tdcResult             = tdcCstNo2CstId (pReplyMsg->data.rCstNo2CstId.cstId,
                                                              &pReplyMsg->data.rCstNo2CstId.topoCnt,
                                                              pCallMsg->data.cCstNo2CstId.trnCstNo);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("CstNo2CstId", IPC_CALL_ADDR_2_CST_ID_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleLabel2TrnCstNo (const T_TDC_IPC_MSG*   pCallMsg,
                                              T_TDC_IPC_MSG*   pReplyMsg,
                                              T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_LABEL_2_TRN_CST_NO_SIZE)
   {
      T_IPT_LABEL    carLabel;

      (void) tdcStrNCpy (carLabel, pCallMsg->data.cLabel2TrnCstNo.carLabel, IPT_LABEL_SIZE);

      {
         char              text[80];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text), "handleLabel2TrnCstNo car(%s) called", carLabel);
         text[sizeof (text) - 1] = '\0';

         DEBUG_INFO (MOD_IPC, text);
      }

      pReplyMsg->data.rLabel2TrnCstNo.topoCnt = pCallMsg->data.cLabel2TrnCstNo.topoCnt;
      pReplyMsg->head.msgType                 = TDC_IPC_REPLY_LABEL_2_TRN_CST_NO;
      pReplyMsg->head.msgLen                  = IPC_REPLY_LABEL_2_TRN_CST_NO_SIZE;
      pReplyMsg->head.tdcResult               = tdcLabel2TrnCstNo (&pReplyMsg->data.rLabel2TrnCstNo.trnCstNo, 
                                                                   &pReplyMsg->data.rLabel2TrnCstNo.topoCnt,
                                                                   carLabel);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("Label2CstNo", IPC_CALL_LABEL_2_TRN_CST_NO_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------


static T_TDC_BOOL handleAddr2TrnCstNo (const T_TDC_IPC_MSG*   pCallMsg,
                                             T_TDC_IPC_MSG*   pReplyMsg,
                                             T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_ADDR_2_TRN_CST_NO_SIZE)
   {
      DEBUG_INFO1 (MOD_IPC, "handleAddr2TrnCstNo (x%08x) called", pCallMsg->data.cAddr2TrnCstNo.ipAddr);

      pReplyMsg->data.rAddr2TrnCstNo.topoCnt = pCallMsg->data.cAddr2TrnCstNo.topoCnt;
      pReplyMsg->head.msgType                = TDC_IPC_REPLY_ADDR_2_TRN_CST_NO;
      pReplyMsg->head.msgLen                 = IPC_REPLY_ADDR_2_TRN_CST_NO_SIZE;
      pReplyMsg->head.tdcResult              = tdcAddr2TrnCstNo (&pReplyMsg->data.rAddr2TrnCstNo.trnCstNo,
                                                                 &pReplyMsg->data.rAddr2TrnCstNo.topoCnt,
                                                                 pCallMsg->data.cAddr2TrnCstNo.ipAddr);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("Addr2TrnCstNo", IPC_CALL_ADDR_2_TRN_CST_NO_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetTrnCstCnt (const T_TDC_IPC_MSG*   pCallMsg,
                                            T_TDC_IPC_MSG*   pReplyMsg,
                                            T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_TRN_CST_CNT_SIZE)
   {
      DEBUG_INFO (MOD_IPC, "handleGetTrnCstCnt called");

      pReplyMsg->data.rGetTrnCstCnt.topoCnt = pCallMsg->data.cGetTrnCstCnt.topoCnt;
      pReplyMsg->head.msgType               = TDC_IPC_REPLY_GET_TRN_CST_CNT;
      pReplyMsg->head.msgLen                = IPC_REPLY_GET_TRN_CST_CNT_SIZE;
      pReplyMsg->head.tdcResult             = tdcGetTrnCstCnt (&pReplyMsg->data.rGetTrnCstCnt.trnCstCnt,
                                                               &pReplyMsg->data.rGetTrnCstCnt.topoCnt);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetTrnCstCnt", IPC_CALL_GET_TRN_CST_CNT_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetCstCarCnt (const T_TDC_IPC_MSG*   pCallMsg,
                                            T_TDC_IPC_MSG*   pReplyMsg,
                                            T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_CST_CAR_CNT_SIZE)
   {
      T_IPT_LABEL    cstLabel;

      (void) tdcStrNCpy (cstLabel, pCallMsg->data.cGetCstCarCnt.cstLabel, IPT_LABEL_SIZE);

      {
         char              text[120];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text), "handleGetCstCarCnt cst(%s) called", cstLabel);
         text[sizeof (text) - 1] = '\0';

         DEBUG_INFO (MOD_IPC, text);
      }

      pReplyMsg->data.rGetCstCarCnt.topoCnt = pCallMsg->data.cGetCstCarCnt.topoCnt;
      pReplyMsg->head.msgType               = TDC_IPC_REPLY_GET_CST_CAR_CNT;
      pReplyMsg->head.msgLen                = IPC_REPLY_GET_CST_CAR_CNT_SIZE;
      pReplyMsg->head.tdcResult             = tdcGetCstCarCnt (&pReplyMsg->data.rGetCstCarCnt.carCnt,
                                                               &pReplyMsg->data.rGetCstCarCnt.topoCnt,
                                                               cstLabel);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetCstCarCnt", IPC_CALL_GET_CST_CAR_CNT_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetCarDevCnt (const T_TDC_IPC_MSG*   pCallMsg,
                                            T_TDC_IPC_MSG*   pReplyMsg,
                                            T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_CAR_DEV_CNT_SIZE)
   {
      T_IPT_LABEL       carLabel;
      T_IPT_LABEL       cstLabel;

      (void) tdcStrNCpy (carLabel, pCallMsg->data.cGetCarDevCnt.carLabel, IPT_LABEL_SIZE);
      (void) tdcStrNCpy (cstLabel, pCallMsg->data.cGetCarDevCnt.cstLabel, IPT_LABEL_SIZE);

      {
         char              text[80];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                             "handleGetCarDevCnt cst(%s), car(%s) called", cstLabel, carLabel);
         text[sizeof (text) - 1] = '\0';

         DEBUG_INFO (MOD_IPC, text);
      }

      pReplyMsg->data.rGetCarDevCnt.topoCnt =  pCallMsg->data.cGetCarDevCnt.topoCnt;
      pReplyMsg->head.msgType               = TDC_IPC_REPLY_GET_CAR_DEV_CNT;
      pReplyMsg->head.msgLen                = IPC_REPLY_GET_CAR_DEV_CNT_SIZE;
      pReplyMsg->head.tdcResult             = tdcGetCarDevCnt (&pReplyMsg->data.rGetCarDevCnt.devCnt, 
                                                               &pReplyMsg->data.rGetCarDevCnt.topoCnt,
                                                               cstLabel, carLabel);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetCarDevCnt", IPC_CALL_GET_CAR_DEV_CNT_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetCarInfo (const T_TDC_IPC_MSG*   pCallMsg,
                                          T_TDC_IPC_MSG*   pReplyMsg,
                                          T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_CAR_INFO_SIZE)
   {
      //UINT8             carDataBuf[TDC_CAR_DATA_SIZE (IPC_MAX_DEV_PER_CAR)];

      T_IPT_LABEL       carLabel;
      T_IPT_LABEL       cstLabel;
      UINT8             carDataBuf[sizeof (T_TDC_CAR_DATA) + (IPC_MAX_DEV_PER_CAR * sizeof (T_TDC_DEV_DATA))]; // aprox. 3200 Bytes
      T_TDC_CAR_DATA*   pCarData = (T_TDC_CAR_DATA *) carDataBuf;
      UINT16            maxDev   = pCallMsg->data.cGetCarInfo.maxDev;

      if (maxDev > IPC_MAX_DEV_PER_CAR)
      {
         DEBUG_WARN2 (MOD_IPC, "Adapted maxDev to reasonable Value: %d <-> %d", IPC_MAX_DEV_PER_CAR, maxDev);

         maxDev = IPC_MAX_DEV_PER_CAR;
      }

      (void) tdcStrNCpy (carLabel, pCallMsg->data.cGetCarInfo.carLabel, IPT_LABEL_SIZE);
      (void) tdcStrNCpy (cstLabel, pCallMsg->data.cGetCarInfo.cstLabel, IPT_LABEL_SIZE);

      {
         char  text[100];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text), "handleGetCarInfo maxDev(%d), cst(%s), car(%s) called",
                             maxDev, cstLabel, carLabel);
         text[sizeof (text) - 1] = '\0';

         DEBUG_INFO (MOD_IPC, text);
      }

      pReplyMsg->data.rGetCarInfo.topoCnt = pCallMsg->data.cGetCarInfo.topoCnt;
      pReplyMsg->head.msgType             = TDC_IPC_REPLY_GET_CAR_INFO;
      pReplyMsg->head.tdcResult           = tdcGetCarInfo (pCarData, &pReplyMsg->data.rGetCarInfo.topoCnt,
                                                           maxDev, cstLabel, carLabel);

      if (pReplyMsg->head.tdcResult == TDC_OK)
      {
         UINT32            i;

         (void) tdcStrNCpy (pReplyMsg->data.rGetCarInfo.carId,   pCarData->carId,    IPT_LABEL_SIZE);
         (void) tdcStrNCpy (pReplyMsg->data.rGetCarInfo.carType, pCarData->carType,  IPT_LABEL_SIZE);
         (void) tdcMemCpy (pReplyMsg->data.rGetCarInfo.uicIdent, pCarData->uicIdent, (UINT32) IPT_UIC_IDENTIFIER_CNT);

         pReplyMsg->data.rGetCarInfo.cstCarNo  = pCarData->cstCarNo;
         pReplyMsg->data.rGetCarInfo.trnOrient = pCarData->trnOrient;
         pReplyMsg->data.rGetCarInfo.cstOrient = pCarData->cstOrient;
         pReplyMsg->data.rGetCarInfo.devCnt    = pCarData->devCnt;

         pReplyMsg->head.msgLen  = IPC_REPLY_GET_CAR_INFO_SIZE (pCarData->devCnt);

         for (i = 0; i < pCarData->devCnt; i++)
         {
            pReplyMsg->data.rGetCarInfo.devData[i].hostId = pCarData->devData[i].hostId;
            (void) tdcStrNCpy (pReplyMsg->data.rGetCarInfo.devData[i].devId,
                               pCarData->devData[i].devId,
                               IPT_LABEL_SIZE);
         }
      }
      else
      {
         pReplyMsg->head.msgLen  = IPC_REPLY_GET_CAR_INFO_SIZE (0);

         pReplyMsg->data.rGetCarInfo.carId[0]   = '\0';
         pReplyMsg->data.rGetCarInfo.carType[0] = '\0';
         pReplyMsg->data.rGetCarInfo.cstCarNo   = (UINT8)  0;
         pReplyMsg->data.rGetCarInfo.trnOrient  = (UINT8)  0;
         pReplyMsg->data.rGetCarInfo.cstOrient  = (UINT8)  0;
         pReplyMsg->data.rGetCarInfo.devCnt     = (UINT16) 0;
      }

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetCarInfo", IPC_CALL_GET_CAR_INFO_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetUicState (const T_TDC_IPC_MSG*   pCallMsg,
                                           T_TDC_IPC_MSG*   pReplyMsg,
                                           T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_UIC_STATE_SIZE)
   {
      DEBUG_INFO (MOD_IPC, "handleGetUicState called");

      pReplyMsg->head.msgType   = TDC_IPC_REPLY_GET_UIC_STATE;
      pReplyMsg->head.msgLen    = IPC_REPLY_GET_UIC_STATE_SIZE;
      pReplyMsg->head.tdcResult = tdcGetUicState (&pReplyMsg->data.rGetUicState.inaugState,
                                                  &pReplyMsg->data.rGetUicState.topoCnt);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetUicState", IPC_CALL_GET_UIC_STATE_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetUicGlobData (const T_TDC_IPC_MSG*   pCallMsg,
                                              T_TDC_IPC_MSG*   pReplyMsg,
                                              T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_UIC_GLOB_DATA_SIZE)
   {
      T_TDC_UIC_GLOB_DATA        globData;

      DEBUG_INFO (MOD_IPC, "handleGetUicGlobData called");

      pReplyMsg->data.rGetUicGlobData.topoCnt = pCallMsg->data.cGetUicGlobData.topoCnt;
      pReplyMsg->head.msgType                 = TDC_IPC_REPLY_GET_UIC_GLOB_DATA;
      pReplyMsg->head.msgLen                  = IPC_REPLY_GET_UIC_GLOB_DATA_SIZE;
      pReplyMsg->head.tdcResult               = tdcGetUicGlobalData (&globData, &pReplyMsg->data.rGetUicGlobData.topoCnt);

      (void) tdcMemCpy (pReplyMsg->data.rGetUicGlobData.confPos, globData.confPos, IPT_UIC_CONF_POS_CNT);

      pReplyMsg->data.rGetUicGlobData.confPosAvail   = globData.confPosAvail;
      pReplyMsg->data.rGetUicGlobData.operatAvail    = globData.operatAvail;
      pReplyMsg->data.rGetUicGlobData.natApplAvail   = globData.natApplAvail;
      pReplyMsg->data.rGetUicGlobData.cstPropAvail   = globData.cstPropAvail;
      pReplyMsg->data.rGetUicGlobData.carPropAvail   = globData.carPropAvail;
      pReplyMsg->data.rGetUicGlobData.seatResNoAvail = globData.seatResNoAvail;
      pReplyMsg->data.rGetUicGlobData.inaugFrameVer  = globData.inaugFrameVer;
      pReplyMsg->data.rGetUicGlobData.rDataVer       = globData.rDataVer;
      pReplyMsg->data.rGetUicGlobData.inaugState     = globData.inaugState;
      pReplyMsg->data.rGetUicGlobData.topoCnt        = globData.topoCnt;
      pReplyMsg->data.rGetUicGlobData.orient         = globData.orient;
      pReplyMsg->data.rGetUicGlobData.notAllConf     = globData.notAllConf;
      pReplyMsg->data.rGetUicGlobData.confCancelled  = globData.confCancelled;
      pReplyMsg->data.rGetUicGlobData.trnCarCnt      = (UINT8) globData.trnCarCnt;

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetUicGlobalData", IPC_CALL_GET_UIC_GLOB_DATA_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleGetUicCarData (const T_TDC_IPC_MSG*   pCallMsg,
                                             T_TDC_IPC_MSG*   pReplyMsg,
                                             T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_GET_UIC_CAR_DATA_SIZE)
   {
      T_TDC_UIC_CAR_DATA         carData;

      DEBUG_INFO1 (MOD_IPC, "handleGetUicCarData (%d) called", pCallMsg->data.cGetUicCarData.carSeqNo);

      pReplyMsg->data.rGetUicCarData.topoCnt = pCallMsg->data.cGetUicCarData.topoCnt;
      pReplyMsg->head.msgType                = TDC_IPC_REPLY_GET_UIC_CAR_DATA;
      pReplyMsg->head.msgLen                 = IPC_REPLY_GET_UIC_CAR_DATA_SIZE;
      pReplyMsg->head.tdcResult              = tdcGetUicCarData (&carData, 
                                                                 &pReplyMsg->data.rGetUicCarData.topoCnt,
                                                                 pCallMsg->data.cGetUicCarData.carSeqNo);

      (void) tdcMemCpy (pReplyMsg->data.rGetUicCarData.cstProp,  carData.cstProp,  IPT_MAX_UIC_CST_NO);
      (void) tdcMemCpy (pReplyMsg->data.rGetUicCarData.carProp,  carData.carProp,  IPT_UIC_CAR_PROPERTY_CNT);
      (void) tdcMemCpy (pReplyMsg->data.rGetUicCarData.uicIdent, carData.uicIdent, IPT_UIC_IDENTIFIER_CNT);

      pReplyMsg->data.rGetUicCarData.trnCstNo      = carData.cstSeqNo;
      pReplyMsg->data.rGetUicCarData.seatResNo     = carData.seatResNo;
      pReplyMsg->data.rGetUicCarData.carSeqNo      = carData.carSeqNo;
      pReplyMsg->data.rGetUicCarData.contrCarCnt   = carData.contrCarCnt;
      pReplyMsg->data.rGetUicCarData.operat        = carData.operat;
      pReplyMsg->data.rGetUicCarData.owner         = carData.owner;
      pReplyMsg->data.rGetUicCarData.natAppl       = carData.natAppl;
      pReplyMsg->data.rGetUicCarData.natVer        = carData.natVer;
      pReplyMsg->data.rGetUicCarData.trnOrient     = carData.trnOrient;
      pReplyMsg->data.rGetUicCarData.cstOrient     = carData.cstOrient;
      pReplyMsg->data.rGetUicCarData.isLeading     = carData.isLeading;
      pReplyMsg->data.rGetUicCarData.isLeadRequ    = carData.isLeadRequ;
      pReplyMsg->data.rGetUicCarData.trnSwInCarCnt = carData.trnSwInCarCnt;

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("GetUicCarData", IPC_CALL_GET_UIC_CAR_DATA_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleLabel2UicCarSeqNo (const T_TDC_IPC_MSG*   pCallMsg,
                                                 T_TDC_IPC_MSG*   pReplyMsg,
                                                 T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO_SIZE)
   {
      T_IPT_LABEL    carLabel;
      T_IPT_LABEL    cstLabel;

      (void) tdcStrNCpy (carLabel, pCallMsg->data.cLabel2UicCarSeqNo.carLabel, IPT_LABEL_SIZE);
      (void) tdcStrNCpy (cstLabel, pCallMsg->data.cLabel2UicCarSeqNo.cstLabel, IPT_LABEL_SIZE);

      {
         char              text[100];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text),
                             "handleLabel2UicCarSeqNo cst(%s), car(%s) called", cstLabel, carLabel);
         text[sizeof (text) - 1] = '\0';
         DEBUG_INFO (MOD_IPC, text);
      }

      pReplyMsg->data.rLabel2UicCarSeqNo.iptTopoCnt = pCallMsg->data.cLabel2UicCarSeqNo.iptTopoCnt;
      pReplyMsg->data.rLabel2UicCarSeqNo.uicTopoCnt = pCallMsg->data.cLabel2UicCarSeqNo.uicTopoCnt;
      pReplyMsg->head.msgType                       = TDC_IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO;
      pReplyMsg->head.msgLen                        = IPC_REPLY_LABEL_2_UIC_CAR_SEQ_NO_SIZE;
      pReplyMsg->head.tdcResult                     = tdcLabel2UicCarSeqNo (&pReplyMsg->data.rLabel2UicCarSeqNo.carSeqNo, 
                                                                            &pReplyMsg->data.rLabel2UicCarSeqNo.iptTopoCnt,
                                                                            &pReplyMsg->data.rLabel2UicCarSeqNo.uicTopoCnt,
                                                                            cstLabel, carLabel);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("Label2UicCarSeqNo", IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleAddr2UicCarSeqNo (const T_TDC_IPC_MSG*   pCallMsg,
                                                T_TDC_IPC_MSG*   pReplyMsg,
                                                T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO_SIZE)
   {
      DEBUG_INFO1 (MOD_IPC, "handleAddr2UicCarSeqNo (x%08x) called", pCallMsg->data.cAddr2UicCarSeqNo.ipAddr);

      pReplyMsg->data.rAddr2UicCarSeqNo.iptTopoCnt = pCallMsg->data.cAddr2UicCarSeqNo.iptTopoCnt;
      pReplyMsg->data.rAddr2UicCarSeqNo.uicTopoCnt = pCallMsg->data.cAddr2UicCarSeqNo.uicTopoCnt;
      pReplyMsg->head.msgType                      = TDC_IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO;
      pReplyMsg->head.msgLen                       = IPC_REPLY_ADDR_2_UIC_CAR_SEQ_NO_SIZE;
      pReplyMsg->head.tdcResult                    = tdcAddr2UicCarSeqNo (&pReplyMsg->data.rAddr2UicCarSeqNo.carSeqNo,
                                                                          &pReplyMsg->data.rAddr2UicCarSeqNo.iptTopoCnt,
                                                                          &pReplyMsg->data.rAddr2UicCarSeqNo.uicTopoCnt,
                                                                          pCallMsg->data.cAddr2UicCarSeqNo.ipAddr);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("Addr2UicCarSeqNo", IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleUicCarSeqNo2Ids (const T_TDC_IPC_MSG*   pCallMsg,
                                               T_TDC_IPC_MSG*   pReplyMsg,
                                               T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_UIC_CAR_SEQ_NO_2_IDS_SIZE)
   {
      DEBUG_INFO1 (MOD_IPC, "handleUicCarSeqNo2Ids (x%08x) called", pCallMsg->data.cUicCarSeqNo2Ids.carSeqNo);

      pReplyMsg->data.rUicCarSeqNo2Ids.iptTopoCnt = pCallMsg->data.cUicCarSeqNo2Ids.iptTopoCnt;
      pReplyMsg->data.rUicCarSeqNo2Ids.uicTopoCnt = pCallMsg->data.cUicCarSeqNo2Ids.uicTopoCnt;
      pReplyMsg->head.msgType                     = TDC_IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS;
      pReplyMsg->head.msgLen                      = IPC_REPLY_UIC_CAR_SEQ_NO_2_IDS_SIZE;
      pReplyMsg->head.tdcResult                   = tdcUicCarSeqNo2Ids (pReplyMsg->data.rUicCarSeqNo2Ids.cstId,
                                                                        pReplyMsg->data.rUicCarSeqNo2Ids.carId,
                                                                        &pReplyMsg->data.rUicCarSeqNo2Ids.iptTopoCnt,
                                                                        &pReplyMsg->data.rUicCarSeqNo2Ids.uicTopoCnt,
                                                                        pCallMsg->data.cUicCarSeqNo2Ids.carSeqNo);

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("UicCarSeqNo2Ids", IPC_CALL_UIC_CAR_SEQ_NO_2_IDS_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleSetDebugLevel (const T_TDC_IPC_MSG*   pCallMsg,
                                             T_TDC_IPC_MSG*   pReplyMsg,
                                             T_TDC_SOCKET     fd)
{
   if (pCallMsg->head.msgLen == IPC_CALL_SET_DEBUG_LEVEL_SIZE)
   {
      {
         char              text[80];

         (void) tdcSNPrintf (text, (UINT32) sizeof (text), 
                             "handleSetDebugLevel (%s) called", pCallMsg->data.cSetDebugLevel.dbgLevel);
         text[sizeof (text) - 1] = '\0';
         DEBUG_INFO (MOD_IPC, text);
      }

      tdcSetDebugLevel (pCallMsg->data.cSetDebugLevel.dbgLevel, NULL, NULL);

      pReplyMsg->head.msgType   = TDC_IPC_REPLY_SET_DEBUG_LEVEL;
      pReplyMsg->head.msgLen    = IPC_REPLY_SET_DEBUG_LEVEL_SIZE;
      pReplyMsg->head.tdcResult = TDC_OK;

      return(tdcSendIpcMsg (MOD_IPC, fd, pReplyMsg));
   }
   else
   {
      invalidMsgLenWarning ("SetDebugLevel", IPC_CALL_SET_DEBUG_LEVEL_SIZE, pCallMsg->head.msgLen);
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL handleIpcCall (const T_TDC_IPC_MSG*   pCallMsg,
                                       T_TDC_IPC_MSG*   pReplyMsg,
                                       T_TDC_SOCKET     fd)
{
   T_TDC_BOOL     bOk = FALSE;

   switch (pCallMsg->head.msgType)
   {
      case TDC_IPC_CALL_GET_IPT_STATE:
      {
         bOk = handleGetIptState (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_OWN_IDS:
      {
         bOk = handleGetOwnIds (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_ADDR_BY_NAME:
      {
         bOk = handleGetAddrByName (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_URI_HOST_PART:
      {
         bOk = handleGetUriHostPart (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_LABEL_2_CAR_ID:
      {
         bOk = handleLabel2CarId (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_ADDR_2_CAR_ID:
      {
         bOk = handleAddr2CarId (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_LABEL_2_CST_ID:
      {
         bOk = handleLabel2CstId (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_ADDR_2_CST_ID:
      {
         bOk = handleAddr2CstId (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_LABEL_2_TRN_CST_NO:
      {
         bOk = handleLabel2TrnCstNo (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_ADDR_2_TRN_CST_NO:
      {
         bOk = handleAddr2TrnCstNo (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_TRN_CST_CNT:
      {
         bOk = handleGetTrnCstCnt (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_CST_CAR_CNT:
      {
         bOk = handleGetCstCarCnt (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_CAR_DEV_CNT:
      {
         bOk = handleGetCarDevCnt (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_CAR_INFO:
      {
         bOk = handleGetCarInfo (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_UIC_STATE:
      {
         bOk = handleGetUicState (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_UIC_GLOB_DATA:
      {
         bOk = handleGetUicGlobData (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_UIC_CAR_DATA:
      {
         bOk = handleGetUicCarData (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_LABEL_2_UIC_CAR_SEQ_NO:
      {
         bOk = handleLabel2UicCarSeqNo (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_ADDR_2_UIC_CAR_SEQ_NO:
      {
         bOk = handleAddr2UicCarSeqNo (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_UIC_CAR_SEQ_NO_2_IDS:
      {
         bOk = handleUicCarSeqNo2Ids (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_SET_DEBUG_LEVEL:
      {
         bOk = handleSetDebugLevel (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_ADDR_BY_NAME_EXT:
      {
         bOk = handleGetAddrByNameExt (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_CSTNO_2_CST_ID:
      {
         bOk = handleCstNo2CstId (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_VERSION:
      {
         bOk = handleGetVersion (pCallMsg, pReplyMsg, fd);
         break;
      }
      case TDC_IPC_CALL_GET_TBTYPE:
      {
         bOk = handleGetTrnBackboneType (pCallMsg, pReplyMsg, fd);
         break;
      }
      default:
      {
         break;
      }
   }

   return(bOk);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

static void closeIpcClientSocket (INT32    threadIdx)
{
   if (threadIdx < MAX_TDC_CLIENT_THREADS)
   {
      if (tdcClientThreadTab[threadIdx].threadId == NULL)
      {
         if (tdcClientThreadTab[threadIdx].connFd != TDC_INVALID_SOCKET)
         {
            DEBUG_WARN1 (MOD_MAIN, "Inconsistent IpcClient-State during terminate request (%d)",
                         tdcClientThreadTab[threadIdx].connFd);
         }
      }
      else
      {
         if (tdcClientThreadTab[threadIdx].connFd == TDC_INVALID_SOCKET)
         {
            DEBUG_WARN (MOD_MAIN, "Inconsistent IpcClient-State during terminate request (0)");
         }
      }

      // Closing the Client Socket should terminate the thread within a short time

      (void) tdcCloseSocket (MOD_MAIN, &tdcClientThreadTab[threadIdx].connFd);
   }
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL terminateIpcClient (INT32    threadIdx)
{
   int            i;

   if (tdcMutexLock (MOD_MAIN, taskIdMutexId) == TDC_MUTEX_OK)
   {
      closeIpcClientSocket (threadIdx);

      (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
   }

   // awaiting end of client thread

   for (i = 0; i < 10; i++, tdcSleep (100))      /* The Client should Terminate within 1 second! */
   {
      if (tdcMutexLock (MOD_MAIN, taskIdMutexId) == TDC_MUTEX_OK)
      {
         if (tdcClientThreadTab[threadIdx].threadId == NULL)
         {
            (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
            return (TRUE);
         }

         (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
      }
   }

   return (FALSE);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL terminateAllIpcClients (void)
{
   int            i;
   T_TDC_BOOL     bOK = FALSE;

   if (tdcMutexLock (MOD_MAIN, taskIdMutexId) == TDC_MUTEX_OK)
   {
      INT32    threadIdx;

      for (threadIdx = 0; threadIdx < MAX_TDC_CLIENT_THREADS; threadIdx++)
      {
         closeIpcClientSocket (threadIdx);
      }

      (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
   }

   // All Client Sockets are closed by now. Wait until the threads terminated.
   // Wait no longer than 1 second

   for (i = 0; (i < 10)   &&   !bOK; i++, tdcSleep (100))
   {
      int      terminateCnt = 0;

      if (tdcMutexLock (MOD_MAIN, taskIdMutexId) == TDC_MUTEX_OK)
      {
         INT32    threadIdx;

         for (threadIdx = 0; threadIdx < MAX_TDC_CLIENT_THREADS; threadIdx++)
         {
            if (tdcClientThreadTab[threadIdx].threadId == NULL)
            {
               terminateCnt++;      // This thread already terminated
            }
         }

         (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
      }

      bOK = (terminateCnt == MAX_TDC_CLIENT_THREADS);
   }

   return (bOK);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL terminateIpcServ (void)
{
   int            i;

   (void) tdcCloseSocket (MOD_MAIN, &listenFd);

   for (i = 0; i < 10; i++, tdcSleep (100))            /* The Server should Terminate within 1 second! */
   {
      if (tdcMutexLock (MOD_MAIN, taskIdMutexId) == TDC_MUTEX_OK)
      {
         if (tdcThreadIdTab[T_IPCSERV_INDEX] == NULL)
         {
            (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
            return(TRUE);
         }

         (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
      }
   }

   return(FALSE);
}

// ----------------------------------------------------------------------------

// Function getFreeThreadIndex is blocking until an Index is vacant
static INT32 getFreeThreadIndex (void)
{
   INT32    threadIdx = 0;

   for (; !bTerminate; )
   {
      if (tdcMutexLock (MOD_MAIN, taskIdMutexId) == TDC_MUTEX_OK)
      {
         for (threadIdx = 0; threadIdx < MAX_TDC_CLIENT_THREADS; threadIdx++)
         {
            if (    (tdcClientThreadTab[threadIdx].connFd   == TDC_INVALID_SOCKET)
                    && (tdcClientThreadTab[threadIdx].threadId == NULL)
               )
            {
               break;
            }
         }

         (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);

         if (threadIdx < MAX_TDC_CLIENT_THREADS)
         {
            break;         // vacant index found
         }
      }

      tdcSleep (250);      // Wait a bit and try it again
   }

   return(threadIdx);
}

// ----------------------------------------------------------------------------

static void doService (void)
{
   for (; !bTerminate;)
   {
      INT32       threadIdx = getFreeThreadIndex ();

      if (!bTerminate)
      {
         T_TDC_SOCK_ADDR*           pCliAddr  = NULL;  // We aren't interested in the client addresses
         T_TDC_SOCKLEN_T*           pLen      = NULL;  // We aren't interested in the client addresses
         T_THREAD_FRAME             ipcClient;
         T_TDC_SOCKET               connFd;

         ipcClient       = ipcClientId;
         ipcClient.pArgV = (void *) threadIdx;

         DEBUG_INFO1 (MOD_IPC, "Waiting for new Client connection, calling accept: listenFd = %d", (int) listenFd);

         connFd = Accept (MOD_IPC, listenFd, pCliAddr, pLen);

         if (connFd != TDC_INVALID_SOCKET)
         {
            DEBUG_INFO1 (MOD_IPC, "Accepted Client, spawning new thread - connFd = %d", (T_TDC_SOCKET) ipcClient.pArgV);

            if (tdcMutexLock (MOD_MAIN, taskIdMutexId) == TDC_MUTEX_OK)
            {
               tdcClientThreadTab[threadIdx].connFd   = connFd;
               tdcClientThreadTab[threadIdx].threadId = startupTdcSingleThread (&ipcClient);

               if (tdcClientThreadTab[threadIdx].threadId == NULL)
               {
                  DEBUG_WARN1 (MOD_IPC, "Couldn't spawn new thread, closing client connection (%d)", tdcClientThreadTab[threadIdx].connFd);
                  (void) tdcCloseSocket (MOD_IPC, &tdcClientThreadTab[threadIdx].connFd);
               }

               (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
            }
            else
            {
               (void) tdcCloseSocket (MOD_IPC, &connFd);
            }
         }
      }
   }
}

// ----------------------------------------------------------------------------

void tdcTIpcBody (T_TDC_SOCKET      connFd)
{
   T_TDC_IPC_MSG*     pCallMsg  = (T_TDC_IPC_MSG *) tdcAllocMem ((UINT32) sizeof (T_TDC_IPC_MSG));
   T_TDC_IPC_MSG*     pReplyMsg = (T_TDC_IPC_MSG *) tdcAllocMem ((UINT32) sizeof (T_TDC_IPC_MSG));

   if (    (pCallMsg  == NULL)
           || (pReplyMsg == NULL)
      )
   {
      DEBUG_WARN1 (MOD_IPC, "Failed to alloc memory for Call Message (%d)", sizeof (T_TDC_IPC_MSG));
      tdcFreeMem  (pCallMsg);
      tdcFreeMem  (pReplyMsg);
      return;
   }

   for (;;)
   {
      if (!tdcReceiveIpcMsg (MOD_IPC, connFd, pCallMsg, (UINT32) sizeof (T_TDC_IPC_MSG)))
      {
         break;
      }

      if (!handleIpcCall (pCallMsg, pReplyMsg, connFd))
      {
         break;
      }
   }

   tdcFreeMem (pCallMsg);
   tdcFreeMem (pReplyMsg);
}

// ----------------------------------------------------------------------------

T_TDC_BOOL tdcTerminateIpcServ (void)
{
   T_TDC_BOOL     bOK = TRUE;

   DEBUG_WARN (MOD_IPC, "Termination requested");

   bOK = terminateAllIpcClients ();

   // Now terminate the IPC server itself

   if (!terminateIpcServ ())
   {
      bOK = FALSE;
   }

   DEBUG_WARN1 (MOD_IPC, "Termination finished (%d)", bOK);

   return(bOK);
}

// ----------------------------------------------------------------------------

void tdcTIpcServInit (void*       pArgV)
{
   int         i;

   TDC_UNUSED (pArgV)

    /* gweiss 2010-09-11 abandone this initialziation  here since sockets are used */
    /* during DetermineOwnDevice (and now done there */
    //tdcInitTcpIpWinsock (MOD_MAIN);

   for (i = 0; i < MAX_TDC_CLIENT_THREADS; i++)
   {
      tdcClientThreadTab[i].threadId = NULL;
      tdcClientThreadTab[i].connFd   = TDC_INVALID_SOCKET;
   } 
}    

//-----------------------------------------------------------------------------

void tdcTIpcServ (void*    pArgV)
{
   T_TDC_BOOL              bOK = TRUE;
   T_TDC_LISTEN_PAR        listenPar;
   INT32                   threadIdx;

   TDC_UNUSED(pArgV)
   TDC_UNUSED(bOK)

   listenPar.pSockOpt  = &servSockOpt;
   listenPar.portNo    = TDC_IPC_SERVER_PORT;
   listenPar.addrLen   = 0;
   listenPar.pListenFd = &listenFd;

   if (tdcTcpListen (MOD_IPC, &listenPar))
   {
      tdcSleep    (250);
      DEBUG_INFO1 (MOD_IPC, "IPC-Service started up, waiting for clients to connect - sock(%d)", listenFd);
      doService   ();
   }

   // wait until all client conncetions closed

   for (threadIdx = 0; threadIdx < MAX_TDC_CLIENT_THREADS; threadIdx++)
   {
      bOK &= terminateIpcClient (threadIdx);
   }

   (void) tdcCloseSocket (MOD_IPC, &listenFd);
}                 

// ----------------------------------------------------------------------------

void* serviceThread (void*    pArgV)
{
   INT32             threadIdx = (INT32) pArgV;

   DEBUG_INFO1 (MOD_IPC, "New serviceThread started up successfully - connFd = %d", tdcClientThreadTab[threadIdx].connFd);
   tdcTIpcBody (tdcClientThreadTab[threadIdx].connFd);
   DEBUG_INFO1 (MOD_IPC, "Closing serviceThread and leaving connFd (%d)", tdcClientThreadTab[threadIdx].connFd);

   if (tdcMutexLock (MOD_MD, taskIdMutexId) == TDC_MUTEX_OK)
   {
      (void) tdcCloseSocket (MOD_IPC, &tdcClientThreadTab[threadIdx].connFd);
      tdcFreeMem (tdcClientThreadTab[threadIdx].threadId);
      tdcClientThreadTab[threadIdx].threadId = NULL;

      (void) tdcMutexUnlock (MOD_MD, taskIdMutexId);
   }

   return (NULL);
}

// ----------------------------------------------------------------------------















