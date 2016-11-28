/*******************************************************************************
 * $Id:: tdcIptCom.c 11857 2012-04-12 07:20:22Z gweiss   $
 *
 * DESCRIPTION
 *
 * AUTHOR         M.Ritz         PPC/EBT
 *
 * REMARKS
 *
 * DEPENDENCIES   Either the switch VXWORKS, LINUX or WIN32 has to be set
 *
 * MODIFICATIONS
 *
 * CR-3477 (Gerhard Weiss, 2012-04-11)
 * 			TUEV Assessment findings
 *
 * CR-685 (Gerhard Weiss, 2010-11-03) 
 *          Corrected destruction/termination handling.
 *
 * CR-382 (Bernd Loehr, 2010-08-13)
 *          Additional handling for local or remote IPTInfo
 *
 * All rights reserved. Reproduction, modification, use or disclosure
 * to third parties without express authority is forbidden.
 * Copyright Bombardier Transportation GmbH, Germany, 2002-2012.
 *
 ******************************************************************************/

#include "iptcom.h"

/* !!! Use Basetypes defined via IPTCOM.H instead of own declarations */
/* O_XXXXX waiting for global solution !!! */

//#define TDC_BASETYPES_H

#include "tdc.h"
#include "tdcInit.h"
#include "tdcConfig.h"
#include "tdcIptCom.h"

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

static PD_HANDLE           hPD      = 0;
static MD_QUEUE            hQueueMD = 0;
static T_IPT_URI           lUri     = "";

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL    bIPTDirServerEmul                        = FALSE;
static char          comId100BinaryFile[TDC_MAX_FILENAME_LEN] = TEST_IPTDIR_PD_FILE;
static char          comId101BinaryFile[TDC_MAX_FILENAME_LEN] = TEST_IPTDIR_IPT_MD_FILE;
static char          comId102BinaryFile[TDC_MAX_FILENAME_LEN] = TEST_IPTDIR_UIC_MD_FILE;

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

static void tdcDestroyQueueMD    (const char*      pModname);
static void tdcRemoveListenerMD  (const char*      pModname);

static int configPD (const char* pModname, int comId, int datasetId, int size, int cycleTime);
static int configMD (const char* pModname, int comId);

static int myPDComAPI_get (PD_HANDLE    hPD,
                           BYTE*        pBuffer,
                           UINT16       bufSize);
static int myMDComAPI_getMsg (MD_QUEUE     hQueueMD,     
                              MSG_INFO*    pMsgInfo,       
                              char**       ppData,       
                              UINT32*      pDataLength,
                              int          timeOut);

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

void tdcSetComId100Filename (const char* pFilename)
{
   (void) tdcStrNCpy (comId100BinaryFile, pFilename, (UINT32) sizeof (comId100BinaryFile));
}

/* ---------------------------------------------------------------------------- */

void tdcSetComId101Filename (const char* pFilename)
{
   (void) tdcStrNCpy (comId101BinaryFile, pFilename, (UINT32) sizeof (comId101BinaryFile));
}

/* ---------------------------------------------------------------------------- */

void tdcSetComId102Filename (const char* pFilename)
{
   (void) tdcStrNCpy (comId102BinaryFile, pFilename, (UINT32) sizeof (comId102BinaryFile));
}

/* ---------------------------------------------------------------------------- */

const char* tdcGetComId100Filename ()
{
    return comId100BinaryFile;
}

/* ---------------------------------------------------------------------------- */

const char* tdcGetComId101Filename ()
{
    return comId101BinaryFile;
}

/* ---------------------------------------------------------------------------- */

const char* tdcGetComId102Filename ()
{
    return comId102BinaryFile;
}


/* ---------------------------------------------------------------------------- */

void tdcSetIPTDirServerEmul (T_TDC_BOOL   bEmulate)
{
   bIPTDirServerEmul = bEmulate;
}

// CR-382 ---------------------------------------------------------------------

T_TDC_BOOL tdcGetIPTDirServerEmul (void)
{
   return bIPTDirServerEmul;
}

/* ---------------------------------------------------------------------------- */

static int myMDComAPI_getMsg(MD_QUEUE     hQueue,     
                             MSG_INFO*    pMsgInfo,       
                             char**       ppData,       
                             UINT32*      pDataLength,
                             int          timeOut)
{
   int      getResult = MD_QUEUE_EMPTY;

   if (bIPTDirServerEmul)
   {
      static int     readCnt   = 0;
      UINT32         testDelay = 3500;
      UINT32         comId     = comIdIPTDirIptConfMD;
      char*          pFilename = comId101BinaryFile;
      const char*    pHelloMsg = "Simulating Reception of IPTDir IPT message Data Telegram";

      tdcSleep (testDelay);

      // alternatingly read UIC and IPT telegram
      if (((++readCnt) % 2) == 0)
      {
         pFilename = comId102BinaryFile;
         comId     = comIdIPTDirUicConfMD;
         pHelloMsg = "Simulating Reception of IPTDir UIC message Data Telegram";
      }

      DEBUG_INFO (MOD_MD, pHelloMsg);

      if (    ((*ppData = tdcFile2Buffer (MOD_MD, pFilename, pDataLength))  != NULL)
           && (*pDataLength > 0)
         )
      {
         pMsgInfo->comId   = comId;
         pMsgInfo->msgType = MD_MSGTYPE_DATA;
         getResult         = MD_QUEUE_NOT_EMPTY;
      }
      else
      {
         tdcFreeMem (*ppData);
         *ppData      = NULL;
         *pDataLength = 0;
      }
   }
   else if (tdcGetStandaloneSupport() == TRUE)
   {
   		  /*	CR-382: If we are in standalone mode, and we get no data,
   				we must not block!	*/
   		getResult = MDComAPI_getMsg (hQueue, pMsgInfo, ppData, pDataLength, IPT_NO_WAIT);
        if (getResult == MD_QUEUE_EMPTY)
        {
        	tdcSleep(1000);
        }
   }
   else
   {
      getResult = MDComAPI_getMsg (hQueue, pMsgInfo, ppData, pDataLength, timeOut);
   }

   return (getResult);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcReadMsgData (const char*    pModname,
                           T_READ_MD*     pReadMsgData)
{
   T_TDC_BOOL     bLoop = TRUE;
   T_TDC_BOOL     bOK   = FALSE;

   for (; bLoop;)
   {
      MSG_INFO    msgInfo;

      pReadMsgData->pMsgData    = NULL;
      pReadMsgData->msgLen      = 0;
      /*pReadMsgData->timeout     = 800;        set to a limited time, in order to be able to support a termination */
      pReadMsgData->timeout     = IPT_WAIT_FOREVER;  
      pReadMsgData->srcIpAddr   = 0;
      pReadMsgData->msgDataType = RECV_IPT_MSG_DATA;

      if (bTerminate)
      {
         DEBUG_INFO (pModname, "Aborting tdcReadMsgData() due to Terminate-Request");
         bLoop = FALSE;
      }
      else
      {
         switch (myMDComAPI_getMsg (hQueueMD, 
                                    &msgInfo, 
                                    &pReadMsgData->pMsgData, 
                                    &pReadMsgData->msgLen, 
                                    pReadMsgData->timeout))
         {
            case MD_QUEUE_EMPTY:
            {
               /* repeat calling MDComAPI_getMsg */
                  //DEBUG_WARN (pModname, "MDComAPI_getMsg returned with empty message queue!");
               break;
            }
            case MD_QUEUE_NOT_EMPTY:
            {
               if (    (msgInfo.comId == comIdIPTDirIptConfMD)
                    || (msgInfo.comId == comIdIPTDirUicConfMD)
                  )
               {
                  if (msgInfo.msgType != MD_MSGTYPE_DATA)
                  {
                     DEBUG_WARN1 (pModname, "Strange Message Data Type received (%d)", msgInfo.msgType);
                  }
   
                  pReadMsgData->msgDataType = (msgInfo.comId == comIdIPTDirIptConfMD) ? (RECV_IPT_MSG_DATA) 
                                                                                      : (RECV_UIC_MSG_DATA);
                  pReadMsgData->srcIpAddr   = msgInfo.srcIpAddr;
   
                  bOK   = TRUE;
                  bLoop = FALSE;
               }
               else
               {
                  DEBUG_WARN1  (pModname, "Message received for comId %d - but didn't add Listener", msgInfo.comId);
                  (void) tdcFreeMDBuf (pReadMsgData->pMsgData);
               }
               break;
            }
            default:
            {
               DEBUG_WARN (pModname, "Error calling MDQueue_getMsg");
               bLoop = FALSE;
               break;
            }
         }
      }
   }

   return (bOK);
}

/* ---------------------------------------------------------------------------- */

int tdcFreeMDBuf (char *pBuf)
{
   if (bIPTDirServerEmul)
   {
      tdcFreeMem (pBuf);
      return (IPT_OK);
   }
   else
   {
      return (MDComAPI_freeBuf (pBuf));
   }
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcWriteMsgData (const char*         pModname,
                            const T_WRITE_MD*   pWriteMsgData)
{
   static UINT32     callerRef = 0x08154711;
   T_TDC_BOOL        bOk = FALSE;


   switch (pWriteMsgData->msgDataType)
   {
      case SEND_IPT_MSG_DATA:
      {
         MD_QUEUE       callerQueue = 0;

         callerRef++;
         bOk = (MDComAPI_putMsgQ (comIdIPTDirReqDataMD,
                                  (const char *) pWriteMsgData->pMsgData,
                                  pWriteMsgData->msgLen,
                                  callerQueue,
                                  (const void *) callerRef,
                                  0,
                                  pWriteMsgData->destUri,
                                  lUri)                                 == IPT_OK);

         if (!bOk)
         {
            DEBUG_WARN1 (pModname, "Failed to call MDCom_putMsgQ - comId (%d)", comIdIPTDirReqDataMD);
         }

         break;
      }
      default:
      {
         DEBUG_WARN1 (pModname, "Invalid Message Type for send-request (%d)", pWriteMsgData->msgDataType);
         break;
      }
   }

   return (bOk);
}

/* -------------------------------------------------------------------------- */

static int myPDComAPI_get (PD_HANDLE    hPData,
                           BYTE*        pBuffer,
                           UINT16       bufSize)
{
   int   getResult = IPT_ERROR;

   /*	CR-382: If we are in standalone mode, and we get no data,
   		get the local data instead	*/
   if (tdcGetStandaloneSupport() == TRUE)
   {
   	  int invalid = 0;
      getResult = PDComAPI_getWStatus(hPData, pBuffer, bufSize, &invalid);
   	  if (getResult != IPT_OK ||
          invalid != 0)
      {
      		if (bIPTDirServerEmul == FALSE)
            {
				DEBUG_WARN ("myPDComAPI_get", "No PD100 received, turning emulation on");
            	bIPTDirServerEmul = TRUE;
            }
      }
      else
      {
		  DEBUG_WARN ("myPDComAPI_get", "PD100 received, turning emulation off");
          bIPTDirServerEmul = FALSE;
          return getResult;
      }

   }
   if (bIPTDirServerEmul)
   {
      UINT32      readBufSize = 0;
      char*       pReadData   = tdcFile2Buffer (MOD_PD, comId100BinaryFile, &readBufSize);

      /* CR-3477 (Gerhard Weiss, 2012-04-11): TUEV Assessment findings, add check for NULL ptr */
      if (
          (pReadData != NULL)
          &&   (readBufSize >  0)
          &&   (readBufSize <= ((UINT32) bufSize))
         )
      {
         (void) tdcMemCpy (pBuffer, pReadData, readBufSize);

         getResult = IPT_OK;
      }

      tdcFreeMem (pReadData);
   }
   else
   {
      getResult = PDComAPI_get (hPData, pBuffer, bufSize);
   }

   return (getResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_BOOL tdcReadProcData (const char*         pModname,
                            const T_READ_PD*    pReadProcData)
{
   T_TDC_BOOL        bOk = FALSE;

   switch (pReadProcData->procDataType)
   {
      case RECV_IPT_PROC_DATA:
      {
         PDComAPI_sink (schedGrpIPTDirPD);

         if (myPDComAPI_get (hPD, (BYTE *) pReadProcData->pProcData, (UINT16) pReadProcData->msgLen) == IPT_OK)
         {
            bOk = TRUE;
         }
         else
         {
            DEBUG_INFO (pModname, "Error reading Process Data");
         }

         break;
      }
      default:
      {
         DEBUG_INFO (pModname, "Error reading Process Data");
         break;
      }
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL wrapConfigMD (const char*   pModname, 
                                const char*   pMDName,
                                UINT32        comId)
{
   char     text[80];

   (void) tdcSNPrintf (text, sizeof (text), "Try to configure %s", pMDName);
   text[sizeof (text) - 1] = 0;
   DEBUG_INFO (pModname, text); 

   if (configMD (pModname, (int) comId) != IPT_OK)
   {
      (void) tdcSNPrintf (text, sizeof (text), "Failed to configure %s", pMDName);
      text[sizeof (text) - 1] = 0;
      DEBUG_WARN (pModname, text); 
      return (FALSE);
   }

   return (TRUE);
}
/* ---------------------------------------------------------------------------- */

static void tdcDestroyQueueMD (const char*   pModname)
{
   if (hQueueMD != 0)
   {
      DEBUG_INFO (pModname, "Try to delete MD Queue");

      if (MDComAPI_destroyQueue (hQueueMD) != IPT_OK)
      {
         DEBUG_WARN (pModname, "Failed to call MDComAPI_destroyQueue");
      }

      hQueueMD = 0;
   }
}

/* ---------------------------------------------------------------------------- */

static void tdcRemoveListenerMD (const char*      pModname)
{
   if (hQueueMD != 0)
   {
      DEBUG_INFO (pModname, "Try to remove Listener from MD Queue");

      MDComAPI_removeListenerQ (hQueueMD);
   }
}

/* ---------------------------------------------------------------------------- */

void tdcTerminateMD (const char*      pModname)
{
   tdcRemoveListenerMD (pModname);
   tdcDestroyQueueMD   (pModname);
}

/* ---------------------------------------------------------------------------- */

void tdcTerminatePD (const char*   pModname)
{
   if (hPD != 0)
   {
      DEBUG_INFO (pModname, "Try to unsubscribe to IPTDIR_PD"); 
      PDComAPI_unsubscribe (&hPD);

      hPD = 0;
   }
}

/* ---------------------------------------------------------------------------- */

void tdcInitIptCom (const char*      pModname)
{
   UINT32         comIdTab[] = {comIdIPTDirIptConfMD, comIdIPTDirUicConfMD, 0};
   T_TDC_BOOL     bOk;

/* ---------------------------------------------------------------------------- */

   bOk =    wrapConfigMD (pModname, "IPTDIR_IPT_MD", comIdIPTDirIptConfMD)
         && wrapConfigMD (pModname, "IPTDIR_UIC_MD", comIdIPTDirUicConfMD)
         && wrapConfigMD (pModname, "IPTDIR_REQ_MD", comIdIPTDirReqDataMD);

/* ---------------------------------------------------------------------------- */

   if (bOk)
   {
      DEBUG_INFO (pModname, "Try to configure IPTDIR_PD"); 

      if (configPD (pModname, comIdIPTDirPD, comIdIPTDirPD, sizeof (T_IPT_IPTDIR_PD), 1000) != IPT_OK)
      {
         DEBUG_WARN (pModname, "Failed to configure IPTDIR_PD"); 
         bOk = FALSE;
      }
   }

/* ---------------------------------------------------------------------------- */

   if (bOk)
   {
      DEBUG_INFO (pModname, "Try to subscribe to IPTDIR_PD"); 

      if ((hPD = PDComAPI_subscribe (schedGrpIPTDirPD, comIdIPTDirPD, NULL)) == 0)
      {
         DEBUG_WARN (pModname, "Failed to call PDComAPI_subscribe");
         bOk = FALSE;
      }
   }

/* ---------------------------------------------------------------------------- */

   if (bOk)
   {
      DEBUG_INFO (pModname, "Try to create MD Queue"); 

      if ((hQueueMD = MDComAPI_createQueue (mdMsgQueueSize)) == 0)
      {
         DEBUG_WARN1 (pModname, "Failed to call MDQueue_createQueue (%d)", mdMsgQueueSize);
         bOk = FALSE;
      }
   }

/* ---------------------------------------------------------------------------- */

   if (bOk)
   {
      int                  result;
      static UINT32        myCallerRef = 0x87654321;

      DEBUG_INFO (pModname, "Try to add Listener for MD"); 

      if ((result = MDComAPI_addComIdListenerQ (hQueueMD, (void*) &myCallerRef, comIdTab)) != IPT_OK)
      {
         DEBUG_WARN1 (pModname, "Failed to call MDComAPI_addListenerQ (%d)", result);
         bOk = FALSE;
      }
   }

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

   if (!bOk)
   {
      tdcTerminateMD (pModname);
      tdcTerminatePD (pModname);

      bTerminate = TRUE;               /* Unable to work properly */
   }
}

/* ---------------------------------------------------------------------------- */

static char       destGrpAllURI[] = "grpAll.aCar.lCst";
static char       emptySrcURI[]    = "";

static int configPD (const char* pModname, int comId, int datasetId, int size, int cycleTime) 
{
   IPT_CONFIG_EXCHG_PAR    exchg;
   IPT_DATASET_FORMAT      pFormat[3];
   IPT_CONFIG_DATASET      dataset;
   int                     result;

   exchg.comId                      = (UINT32) comId;
   exchg.datasetId                  = (UINT32) datasetId;
   exchg.comParId                   = 1;     /* Always use default com-parameters for PD */
   exchg.mdRecPar.pSourceURI        = NULL;
   exchg.mdSendPar.pDestinationURI  = NULL;
   exchg.pdRecPar.pSourceURI        = NULL;
   exchg.pdRecPar.validityBehaviour = IPT_INVALID_KEEP;
   exchg.pdRecPar.timeoutValue      = (UINT32) (cycleTime * 2);
   exchg.pdSendPar.pDestinationURI  = destGrpAllURI;
   exchg.pdSendPar.cycleTime        = (UINT32) cycleTime;
   exchg.pdSendPar.redundant        = 0;

   if ((result = iptConfigAddExchgPar (&exchg)) != IPT_OK)
   {
      DEBUG_WARN1 (pModname, "Failed to call iptConfigAddExchgPar - %d", result); 
   }
   else
   {
      if (datasetId != 0)
      {
         dataset.datasetId = (UINT32) datasetId;
         dataset.nLines    = 1;

         pFormat[0].id     = IPT_INT8;
         pFormat[0].size   = (UINT32) size;

         if ((result = iptConfigAddDataset (&dataset, pFormat)) != IPT_OK)
         {
            DEBUG_WARN1 (pModname, "Failed to call iptConfigAddDataset - %d", result); 
         }
      }
   }

   return (result);
}

/* ---------------------------------------------------------------------------- */

static int configMD (const char* pModname, int comId)
{
   IPT_CONFIG_EXCHG_PAR    exchg;
   int                     result;

   exchg.comId                      = (UINT32) comId;
   exchg.datasetId                  = 0;
   exchg.comParId                   = 2;        /* Always use default com parameters for MD */
   exchg.mdRecPar.pSourceURI        = emptySrcURI;
   exchg.mdSendPar.pDestinationURI  = destGrpAllURI;
   exchg.pdRecPar.pSourceURI        = NULL;
   exchg.pdRecPar.validityBehaviour = IPT_INVALID_KEEP;
   exchg.pdRecPar.timeoutValue      = 0;
   exchg.pdSendPar.pDestinationURI  = NULL;
   exchg.pdSendPar.cycleTime        = 0;
   exchg.pdSendPar.redundant        = 0;

   if ((result = iptConfigAddExchgPar (&exchg)) != IPT_OK)
   {
      DEBUG_WARN1 (pModname, "Failed to call iptConfigAddExchgPar - %d", result); 
   }

   return (result);
}


