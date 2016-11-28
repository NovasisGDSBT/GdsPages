/************************************************************************/
/*  (C) COPYRIGHT 2015 by Bombardier Transportation                     */
/*                                                                      */
/*  Bombardier Transportation Switzerland Dep. PPC/EUT                  */
/************************************************************************/
/*                                                                      */
/*  PROJECT:      Remote download                                       */
/*                                                                      */
/*  MODULE:       dledsServer                                           */
/*                                                                      */
/*  ABSTRACT:     This is the main module file for dleds.               */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/*  REMARKS:                                                            */
/*                                                                      */
/*  DEPENDENCIES: See include list                                      */
/*                                                                      */
/*  ACCEPTED:                                                           */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/*  HISTORY:                                                            */
/*                                                                      */
/*  version  yy-mm-dd  name       depart.  ref   status                 */
/*  -------  --------  ---------  -------  ----  ---------              */
/*    1.0    10-04-06  S.Eriksson PPC/TET1S   --   created              */
/*                                                                      */
/************************************************************************/


/*******************************************************************************
 * INCLUDES
 */

#include <stdio.h>
#include <stdlib.h>
#include "iptcom.h"
#include "vos.h"
#include "tdcApi.h"
#include "dledsVersion.h"
#include "dleds.h"
#include "dledsInstall.h"
#include "dledsDbg.h"
#include "dledsCrc32.h"
#include "dledsPlatform.h"
#include "dleds_icd.h"


/*******************************************************************************
 * DEFINES
 */


/*******************************************************************************
 * TYPEDEFS
 */
typedef struct STRUCT_VERSION_INFO
{
    char    versionAsStr [VERSION_STR_MAX_LEN];
    UINT8   major;
    UINT8   revision;
    UINT8   update;
    UINT8   evolution;
} TYPE_VERSION;


/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 */
static int                  dleds_get_operation_mode(void);
static int                  dleds_getDlAllowed(void);
static int                  dleds_forced_reboot(int rebootReason);
static int                  dleds_getAppMode(int* IEdMode, int* IConfigStatus);
DLEDS_RESULT                dleds_storageFileExists(void);
static DLEDS_RESULT         dleds_storageFileValid(void);
DLEDS_RESULT                dleds_readStorageFile(TYPE_DLEDS_SESSION_DATA* pSessionData);
DLEDS_RESULT                dleds_writeStorageFile(TYPE_DLEDS_SESSION_DATA* pSessionData);
static DLEDS_RESULT         dleds_ftpCopyFile(void);
static DLEDS_RESULT         dleds_setDledsTempPath(void);
static void                 dleds_versionRetrieve(EDIdent* pPdBuffer);
static DLEDS_HOST_URI_FQDN  dleds_validFqdn(char* hostname);
DLEDS_RESULT                dleds_wait_for_request(void);
DLEDS_RESULT                dleds_send_status_message(void);
DLEDS_RESULT                dleds_send_response_message(void);
DLEDS_RESULT                dleds_wait_for_result(void);
DLEDS_RESULT                dleds_download_mode(void);
DLEDS_RESULT                dleds_uninit(void);
DLEDS_RESULT                dleds_checkDownloadModeStatus(void);
DLEDS_RESULT                dleds_checkRunModeStatus(void);
void                        dleds_resetToRunMode(void);
void                        dleds_resetToDownloadMode(void);
void                        dleds_sendEchoMessage(void);


/*******************************************************************************
 * LOCAL VARIABLES
 */
static UINT32 localRequestTransactionId = 0;
static char localRequestFileName[256] = "";
static UINT32 localRequestFileCrc = 0;

/*******************************************************************************
 * GLOBAL VARIABLES
 */
/*
 * This variable is initiated with the name of the file that saves session
 * information before reset to download
 */
char DLEDS_TEMP_STORAGE_FILE_NAME[128] = DLEDS_STORAGE_FILE_NAME;

UINT8 localRequest = FALSE;
UINT8 localRequestInProgress = FALSE;
INT32 localRequestResultCode = -900;

STATE DLEDS_STATE;
TYPE_DLEDS_SESSION_DATA sessionData = {0,0,0,"",0,0,0,{0,"","","",0,0,0},0,0,0,0};
INT32 appResultCode = DL_OK;
UINT8 CFavailable;
char dledsTempDirectory[256];

MD_QUEUE requestQueueId = (MD_QUEUE) NULL;
MD_QUEUE resultQueueId = (MD_QUEUE) NULL;
MD_QUEUE echoQueueId = (MD_QUEUE) NULL;
MD_QUEUE progressQueueId = (MD_QUEUE) NULL;
PD_HANDLE pdHandle;
/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 */

/*******************************************************************************
 *
 * Function name:   dleds_get_operation_mode
 *
 * Abstract:        This function evaluates the operation mode in which the
 *                  operating system is currently running.
 *
 * Return value:    DLEDS_UNKNOWN_MODE:
 *                      Unknown operation mode.
 *
 *                  DLEDS_OSIDLE_MODE:
 *                      The operating system currently runs in idle mode,
 *                      i.e. there are no applications started.
 *
 *                  DLEDS_OSRUN_MODE:
 *                      The operating system currently runs in full
 *                      operational mode, i.e. all applications configured
 *                      for automatic startup are actually started.
 *
 *                  DLEDS_OSDOWNLOAD_MODE:
 *                      The operating system currently runs in a dedicated
 *                      (application-) download mode. Some mandatory system
 *                      software packages have been started, but application
 *                      software startup is intentionally blocked.
 *
 * Globals:         None
 */
static int dleds_get_operation_mode( void )
{
    int operationMode = DLEDS_UNKNOWN_MODE;

    /* Call function that contains Platform Specific code */
    operationMode = dledsPlatformGetOperationMode();

    return operationMode;
}

/*******************************************************************************
 *
 * Function name:   dleds_getDlAllowed
 *
 * Abstract:        This function retrieves the actual state of the
 *                  download allowed status.
 *
 * Return value:    FALSE:  Download is not allowed
 *                  TRUE:   Download is allowed
 *
 * Globals:         None
 */
static int dleds_getDlAllowed(void)
{
    int result = FALSE;

    /* Call function that contains Platform Specific code */
    result = dledsPlatformGetDlAllowed();

    return result;
}

/*******************************************************************************
 *
 * Function name:   dleds_forced_reboot
 *
 * Abstract:        This function shall perform a reboot of the device to
 *                  the defined start up mode mode.
 *
 * Return value:    On success there is no return from this routine as the target
 *                  device will perfom the reset within this function.
 *                  DLEDS_ERROR: Failed to reset device
 *
 * Globals:         None
 */
static int dleds_forced_reboot(
    int     rebootReason)           /* IN: Wanted start up mode
                                            DLEDS_OSRUN_MODE:
                                                On restart of the device, operating system
                                                shall be started in full operational mode.

                                            DLEDS_OSDOWNLOAD_MODE:
                                                On restart of the device, operating system
                                                shall be started in download mode, where
                                                some software required for application software
                                                download services is started.
                                    */
{
    int result = DLEDS_ERROR;

    /* Call function that contains Platform Specific code */
    result = dledsPlatformForcedReboot(rebootReason);
    DebugError1("***PLATFORM: Operation mode (%d)", rebootReason);

    return result;
}

/*******************************************************************************
 *
 * Function name:   dleds_getAppMode
 *
 * Abstract:        This function shall get the global variables
 *                  EnddeviceMode and ConfigStatus from the system.
 *
 * Return value:    DLEDS_OK: operation successfull
 *                  DLEDS_ERROR: operation encountered an error
 *
 * Globals:         None
 */
static int dleds_getAppMode(
    int*    IEdMode,            /* IN/OUT: Value for the enddevice mode
                                    0 : Running with application
                                    1 : Application is suspended
                                    2 : Application less mode
                                    3..255: invalid */
    int*    IConfigStatus)      /* IN/OUT: Value for the configuration status
                                    0 : OK
                                    1 : No config. file available
                                    2 : Configuration error
                                    3 : exception in component
                                    4..255: invalid */
{
    int result = DLEDS_ERROR;
    *IEdMode = 255;
    *IConfigStatus = 255;

    /* Call function that contains Platform Specific code */
    result = dledsPlatformGetAppMode(IEdMode, IConfigStatus);

    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_handleStatusRequests
 *
 * Abstract:      This function handles the two versions of status requests.
 *                Version 1 is sent with ComID 272 and version 2 is sent with
 *                ComID 278. They should both use version 1.0.0.0 of the data set
 *                EDReqStatus.
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *                DLEDS_EDSTATUS
 *
 * Globals:       -
 */
static DLEDS_RESULT dleds_handleStatusRequests(
    MSG_INFO*   pMsgInfo,
    char*       pReqData,
    UINT32      reqDataSize,
    EDIdent*    pPdBuffer)
{
    DLEDS_RESULT    result = DLEDS_ERROR;
    INT32           iptResult;
    EDReqStatus*    pReqStatus;
    UINT8           topoCounter = 0;
    UINT32          ownIpAddress;
    char            tempSrcUri[102];
    char            srcUri[102];

    /* Verify that input parameters are valid */
    if ((pMsgInfo == NULL) || (pReqData == NULL) ||
        (reqDataSize != sizeof(EDReqStatus)) || (pPdBuffer == NULL))
    {
        DebugError0("Invalid input parameters to function dleds_handleStatusRequests()");
        DebugError1("    pMsgInfo= 0x%X", pMsgInfo);
        DebugError1("    pReqData= 0x%X", pReqData);
        DebugError2("    reqDataSize= 0x%X, expected value= 0x%X", reqDataSize, sizeof(EDReqStatus));
        DebugError1("    pPdBuffer= 0x%X", pPdBuffer);
        result = DLEDS_ERROR;
    }
    else
    {
        /* Handle received status request message */
        pReqStatus = (EDReqStatus*) pReqData;
        /* Check that version is valid */
        if (pReqStatus->appVersion != EDReqStatus_appVersion)
        {
            DebugError1("Version (0x%x) of status request message not supported",
                pReqStatus->appVersion);
            result = DLEDS_EDSTATUS;
        }
        else
        {
            /* Convert own IP address to URI string to be used as source URI */
            ownIpAddress = IPTCom_getOwnIpAddr();
            DebugError1("Own IP address to be converted to URI (0x%x)", ownIpAddress);
            topoCounter = 0;
            iptResult = IPTCom_getUriHostPart(
                ownIpAddress,      /* IP address */
                tempSrcUri,        /* Pointer to resulting URI */
                &topoCounter);     /* Topo counter */
            strcpy(srcUri,"DLEDService@");
            strcat(srcUri,tempSrcUri);

            if (pMsgInfo->comId == iEDReqStatus2)
            {
                /* Use communication protocol version 2 */
                sessionData.protocolVersion = DLEDS_PROTOCOL_VERSION_2;
            }
            else
            {
                /* Use communication protocol version 1 */
                sessionData.protocolVersion = DLEDS_PROTOCOL_VERSION_1;
            }

            if (sessionData.protocolVersion == DLEDS_PROTOCOL_VERSION_1)
            {
                iptResult = MDComAPI_putRespMsg(
                    oEDRepStatus,           /* ComId (273)*/
                    0,                      /* UserStatus */
                    (char*) pPdBuffer,      /* pData */
                    sizeof(EDIdent),        /* DataLength */
                    pMsgInfo->sessionId,    /* SessionID */
                    0,                      /* CallerQueue */
                    NULL,                   /* Pointer to callback function */
                    NULL,                   /* Caller reference value */
                    0,                      /* Topo counter */
                    pMsgInfo->srcIpAddr,    /* Destination IP address */
                    0,                      /* DestId */
                    NULL,                   /* Pointer to Dest URI */
                    srcUri);                /* Pointer to Src URI */
            }
            else
            {
                iptResult = MDComAPI_putRespMsg(
                    oEDRepStatus2,          /* ComId (279)*/
                    0,                      /* UserStatus */
                    (char*) pPdBuffer,      /* pData */
                    sizeof(EDIdent),        /* DataLength */
                    pMsgInfo->sessionId,    /* SessionID */
                    0,                      /* CallerQueue */
                    NULL,                   /* Pointer to callback function */
                    NULL,                   /* Caller reference value */
                    0,                      /* Topo counter */
                    pMsgInfo->srcIpAddr,    /* Destination IP address */
                    0,                      /* DestId */
                    NULL,                   /* Pointer to Dest URI */
                    srcUri);                /* Pointer to Src URI */
            }
            if (iptResult == IPT_OK)
            {
                DebugError0("MDComAPI_putRespMsg() returned OK");
                result = DLEDS_EDSTATUS;
            }
            else
            {
                DebugError1("MDComAPI_putRespMsg() failed with error (%d)", iptResult);
                /* forget this request, wait for the next request */
                result = DLEDS_EDSTATUS;
            }
        }
    }
    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_handleSendFileRequests
 *
 * Abstract:      This function handles three versions of send file requests.
 *                The one with a fixed size array (ComID= 850), the one with
 *                a variable sized array (ComID= 852) and the one with a fixed
 *                size array (ComID= 276). The first two should both use version
 *                2.0.0.0 of the data set EDReqStatus.
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *                DLEDS_EDSTATUS
 *
 * Globals:       appResultCode
 *                sessionData
 *                localRequest
 *                localRequestInProgress
 */
static DLEDS_RESULT dleds_handleSendFileRequests(
    MSG_INFO*   pMsgInfo,
    char*       pReqData,
    UINT32      reqDataSize)
{
    DLEDS_RESULT        result = DLEDS_ERROR;
    MCGReqSendFile*     pReqSendFile;
    BOOL                invalidFileType = FALSE;

    /* Verify that input parameters are valid */
    if ((pMsgInfo == NULL) || (pReqData == NULL) || (reqDataSize != sizeof(MCGReqSendFile)))
    {
        DebugError0("Invalid input parameters to function dleds_handleSendFileRequests()");
        DebugError1("    pMsgInfo= 0x%X", pMsgInfo);
        DebugError1("    pReqData= 0x%X", pReqData);
        DebugError1("    reqDataSize= 0x%X", reqDataSize);
        result = DLEDS_ERROR;
    }
    else
    {
        /* Handle received download request message */
        sessionData.downloadResetReason = DLEDS_DOWNLOAD;
        if (pMsgInfo->comId == iEDReqSendFile2)
        {
            /* Use communication protocol version 2 during download */
            sessionData.protocolVersion = DLEDS_PROTOCOL_VERSION_2;
        }
        else
        {
            /* Use communication protocol version 1 during download */
            sessionData.protocolVersion = DLEDS_PROTOCOL_VERSION_1;
        }

        sessionData.sessionId = pMsgInfo->sessionId;
        sessionData.srcIpAddress = pMsgInfo->srcIpAddr;
        strcpy(sessionData.srcUri,pMsgInfo->srcURI);
        sessionData.requestReceiveTime = IPVosGetSecTimer();

        pReqSendFile = (MCGReqSendFile*) pReqData;
        sessionData.requestInfo.transactionId = pReqSendFile->transactionId;
        strcpy(sessionData.requestInfo.fileName, pReqSendFile->fileName);
        strcpy(sessionData.requestInfo.fileServerHostName, pReqSendFile->fileServerHostName);
        strcpy(sessionData.requestInfo.fileServerPath, pReqSendFile->fileServerPath);
        sessionData.requestInfo.fileSize = pReqSendFile->fileSize;
        sessionData.requestInfo.fileCRC = pReqSendFile->fileCRC;
        sessionData.requestInfo.fileVersion = pReqSendFile->fileVersion;
        if (strstr(sessionData.requestInfo.fileName,".edsp2") != NULL)
        {
            sessionData.packageVersion = DLEDS_PACKAGE_VERSION_2;
        }
        else if (strstr(sessionData.requestInfo.fileName,".edsp") != NULL)
        {
            sessionData.packageVersion = DLEDS_PACKAGE_VERSION_1;
        }
        else if (strstr(sessionData.requestInfo.fileName,".edp") != NULL)
        {
            sessionData.packageVersion = DLEDS_PACKAGE_VERSION_1;
        }
        else
        {
            /* Flag that the file type is not supported */
            invalidFileType = TRUE;
        }

        /* Local request will get source IP address 127.0.0.1 */
        if ( sessionData.srcIpAddress == 0x7f000001 )
        {
            localRequest = TRUE;
            sessionData.localTransfer = TRUE;
            DebugError1("HANDLE_SEND_FILE_REQUEST: localRequestInProgress= (%d)", localRequestInProgress);
            DebugError1("HANDLE_SEND_FILE_REQUEST: localRequestTransactionId= (%d)", localRequestTransactionId);
            DebugError1("HANDLE_SEND_FILE_REQUEST: session.localRequestTransactionId= (%d)", sessionData.requestInfo.transactionId);
            DebugError1("HANDLE_SEND_FILE_REQUEST: localRequestFileName= (%s)", localRequestFileName);
            DebugError1("HANDLE_SEND_FILE_REQUEST: session.localRequestFileName= (%s)", sessionData.requestInfo.fileName);
            DebugError1("HANDLE_SEND_FILE_REQUEST: localRequestFileCrc= (0x%x)", localRequestFileCrc);
            DebugError1("HANDLE_SEND_FILE_REQUEST: session.localRequestFileCrc= (0x%x)", sessionData.requestInfo.fileCRC);
            /* check if this is a resend of the previous local request */
            if ( (localRequestInProgress == TRUE) &&
                ( (localRequestTransactionId != sessionData.requestInfo.transactionId) ||
                (strcmp(localRequestFileName, sessionData.requestInfo.fileName) != 0) ||
                (localRequestFileCrc != sessionData.requestInfo.fileCRC)) )
            {
                /* This is a new local request */
                localRequestInProgress = FALSE;
                sessionData.localTransfer = FALSE;
            }
        }
        else
        {
            localRequestInProgress = FALSE;
            sessionData.localTransfer = FALSE;
        }

        /* Do not check Download allowed if this is a local request */
        if ( (localRequestInProgress == FALSE) && (dleds_getDlAllowed() == FALSE) )
        {
            DebugError0("Download not allowed");
            appResultCode = DL_NOT_ALLOWED;
            /*
            * Make sure that response message is sent.
            * Error code should be sent in this case
            */
            result = DLEDS_OK;
        }
        /* Version of message is dependent on ComID */
        /*
        else if (((pMsgInfo->comId == iMCGReqSendFile) && (pReqSendFile->appVersion != MCGReqSendFile_appVersion)) ||
                 ((pMsgInfo->comId == iEDReqSendFile2) && (pReqSendFile->appVersion != EDReqSendFile2_appVersion)))
        {
            DebugError1("Version (0x%x) of request message not supported",
                pReqSendFile->appVersion);
            appResultCode = DL_INCOMPATIBLE_VERSION;
            *
            * Make sure that response message is sent.
            * Error code should be sent in this case
            *
            result = DLEDS_OK;
        }
        */
        else if ( (localRequestInProgress == FALSE) &&
                (CFavailable == 1) &&
                (dleds_diskSpaceAvailable(dledsTempDirectory, pReqSendFile->fileSize) == DLEDS_ERROR) )
        {
            DebugError1("dleds_handleSendFileRequests: Not enought disk space, filesize is %d kByte", pReqSendFile->fileSize);
            appResultCode = DL_DISK_FULL;
            /*
            * Make sure that response message is sent.
            * Error code should be sent in this case
            */
            result = DLEDS_OK;
        }
        else if ( pReqSendFile->transactionId == 0)
        {
            DebugError0("Transaction ID = 0 is not accepted");
            appResultCode = DL_PARAM_ERROR;
            /*
            * Make sure that response message is sent.
            * Error code should be sent in this case
            */
            result = DLEDS_OK;
        }
        else if ( dleds_validFqdn(pReqSendFile->fileServerHostName) != Valid )
        {
            DebugError1("FileServerHostName is not a FQDN (%s)", pReqSendFile->fileServerHostName);
            appResultCode = DL_URI_ERROR;
            /*
            * Make sure that response message is sent.
            * Error code should be sent in this case
            */
            result = DLEDS_OK;
        }
        else if (invalidFileType == TRUE)
        {
            DebugError1("File type not supported (%s)", sessionData.requestInfo.fileName);
            appResultCode = DL_REQUEST_FAILED;
            /*
            * Make sure that response message is sent.
            * Error code should be sent in this case
            */
            result = DLEDS_OK;
        }
        else
        {
            appResultCode = DL_OK;
            sessionData.transferInProgress = TRUE;
            result = dleds_writeStorageFile(&sessionData);
            if (result == DLEDS_OK)
            {
                appResultCode = DL_OK;
            }
            else
            {
                /* Failed to write session data to temp storage file */
                appResultCode = DL_DISK_ERROR;
                /*
                * Make sure that resonse message is sent.
                * Error code should be sent in this case
                */
                result = DLEDS_OK;
            }
        }
    }
    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_handleVersionInfoRequest
 *
 * Abstract:      This function handles the Version Info request. Requested
 *                version info, HW or SW, is retrieved from and sent in
 *                the response message.
 *
 * Return value:  DLEDS_EDSTATUS    This request has been handled, Wait for next request
 *
 * Globals:       -
 */
static DLEDS_RESULT dleds_handleVersionInfoRequest(
    MSG_INFO*   pMsgInfo,
    char*       pReqData,
    UINT32      reqDataSize)
{
    DLEDS_RESULT        result = DLEDS_ERROR;
    INT32               iptResult;
    EDReqVersionInfo*   pReqVersionInfo;
    UINT32              versionInfoActSize = 0;
    char*               pVersionInfoBuffer = NULL;
    UINT8               dataSetAllocated = 0;
    char*               pDataSet = NULL;
    UINT32              dataSetSize = 0;
    EDRepVersionInfo    repVersionInfo;
    UINT8               topoCounter = 0;
    UINT32              ownIpAddress;
    char                tempSrcUri[102];
    char                srcUri[102];
    EDRepVersionInfo*   pTemp;

    /* Verify that input parameters are valid */
    if ((pMsgInfo == NULL) || (pReqData == NULL) || (reqDataSize != sizeof(EDReqVersionInfo)))
    {
        DebugError0("Invalid input parameters to function dleds_handleCleanUpRequest()");
        DebugError1("    pMsgInfo= 0x%X", pMsgInfo);
        DebugError1("    pReqData= 0x%X", pReqData);
        DebugError2("    reqDataSize= 0x%X, expected value= 0x%X", reqDataSize, sizeof(EDReqVersionInfo));
        result = DLEDS_EDSTATUS;
    }
    else
    {
        /* Handle received version info request message */
        pReqVersionInfo = (EDReqVersionInfo*) pReqData;

        /* Check that version is valid */
        if (!(pReqVersionInfo->appVersion == EDReqVersionInfo_appVersion))
        {
            DebugError1("Version (0x%x) of status request message not supported",
                pReqVersionInfo->appVersion);
            result = DLEDS_EDSTATUS;
        }
        else
        {
            /* Get version information from DVS */
            if (pReqVersionInfo->typeOfVersionInfo == VERSION_INFO_HW)
            {
                /* Buffer allocated by function */
                result = dledsPlatformGetHwInfoXml(&versionInfoActSize, &pVersionInfoBuffer);
                if (result != DLEDS_OK || versionInfoActSize == 0 || pVersionInfoBuffer == NULL)
                {
                    DebugError0("dledsPlatformGetHwInfoXml() failed");
                    result = DLEDS_ERROR;
                    pVersionInfoBuffer = NULL;
                    versionInfoActSize = 0;
                }
            }
            else if (pReqVersionInfo->typeOfVersionInfo == VERSION_INFO_SW)
            {
                /* Buffer allocated by function */
                result = dledsPlatformGetSwInfoXml(&versionInfoActSize, &pVersionInfoBuffer);
                if (result != DLEDS_OK || versionInfoActSize == 0 || pVersionInfoBuffer == NULL)
                {
                    DebugError0("dledsPlatformGetSwInfoXml() failed");
                    result = DLEDS_ERROR;
                    pVersionInfoBuffer = NULL;
                    versionInfoActSize = 0;
                }
            }

            /* Convert own IP address to URI string to be used as source URI */
            ownIpAddress = IPTCom_getOwnIpAddr();
            DebugError1("Own IP address to be converted to URI (0x%x)", ownIpAddress);
            topoCounter = 0;
            iptResult = IPTCom_getUriHostPart(
                ownIpAddress,      /* IP address */
                tempSrcUri,        /* Pointer to resulting URI */
                &topoCounter);     /* Topo counter */
            strcpy(srcUri,"DLEDService@");
            strcat(srcUri,tempSrcUri);


            /* Prepare dataset for response message */
            memset(&repVersionInfo, 0, sizeof(EDRepVersionInfo));
            repVersionInfo.appVersion = EDRepVersionInfo_appVersion;
            if (pVersionInfoBuffer != 0)
            {
                /* Version information was successfully retrieved */
                /* Compensate for one extra byte in declaration of EDRepVersionInfo */
                dataSetSize = (sizeof(EDRepVersionInfo) - 1) + versionInfoActSize;
                pDataSet = (char*) malloc(dataSetSize);
                if (pDataSet != NULL)
                {
                    memset(pDataSet, 0, dataSetSize);
                    dataSetAllocated = 1;   /* Remember to free allocated buffer */
                    pTemp = (EDRepVersionInfo*)pDataSet;
                    pTemp->appVersion = EDRepVersionInfo_appVersion;
                    pTemp->reserved[0] = 0;
                    pTemp->reserved[1] = 0;
                    pTemp->reserved[2] = 0;
                    pTemp->reserved[3] = 0;
                    pTemp->reserved[4] = 0;
                    pTemp->reserved[5] = 0;
                    pTemp->returnValue = VERSION_INFO_OK;
                    pTemp->versionInfolength = versionInfoActSize;
                    memcpy(&(pTemp->startVersionInfo), pVersionInfoBuffer, versionInfoActSize);
                }
                else
                {
                    /* Failed to allocate memory for dataset with version info */
                    /* Send minimum dataset in response */
                    DebugError0("Failed to allocate memory for buffer with retrieve version XML data");
                    dataSetSize = sizeof(EDRepVersionInfo) - 1;
                    repVersionInfo.returnValue = VERSION_INFO_NOT_AVAILABLE;
                    repVersionInfo.versionInfolength = 0;
                    pDataSet = (char*) &repVersionInfo;
                }
                /* Buffer allocated no longer needed */
                if (pVersionInfoBuffer != NULL)
                {
                    dledsPlatformReleaseInfoXml(pVersionInfoBuffer);
                }
            }
            else
            {
                /* No version info retrieved from DVS */
                DebugError0("No version data retrieved from DVS");
                dataSetSize = sizeof(EDRepVersionInfo) - 1;
                repVersionInfo.returnValue = VERSION_INFO_NOT_AVAILABLE;
                repVersionInfo.versionInfolength = 0;
                pDataSet = (char*) &repVersionInfo;
            }

            iptResult = MDComAPI_putRespMsg(
                oEDRepVersionInfo,      /* ComId (275)*/
                0,                      /* UserStatus */
                pDataSet,               /* pData */
                dataSetSize,            /* DataLength */
                pMsgInfo->sessionId,    /* SessionID */
                0,                      /* CallerQueue */
                NULL,                   /* Pointer to callback function */
                NULL,                   /* Caller reference value */
                0,                      /* Topo counter */
                pMsgInfo->srcIpAddr,    /* Destination IP address */
                0,                      /* DestId */
                NULL,                   /* Pointer to Dest URI */
                srcUri);                /* Pointer to Src URI */

            if (iptResult == IPT_OK)
            {
                DebugError0("MDComAPI_putRespMsg() returned OK");
                result = DLEDS_EDSTATUS;
            }
            else
            {
                /*    */
                DebugError1("MDComAPI_putRespMsg() failed with error (%d)",
                    iptResult);
                /*
                * forget this request,
                * wait for the next request
                */
                result = DLEDS_EDSTATUS;
            }

            if (dataSetAllocated == 1)
            {
                free(pDataSet);
            }
        }
    }
    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_sendProgressRequest
 *
 * Abstract:      This function sends the progress request message.
 *
 * Return value:  DLEDS_OK      - Progress message is sent successfully
 *                DLEDS_ERROR   - Failure when
 *                                Creating own URI string
 *                                Error from IPTCom when sending message
 *
 * Globals:       sessionData
 *                progressQueueId
 */
DLEDS_RESULT dleds_sendProgressRequest(
    UINT8   ongoingOperation,
    UINT8   progressInfo,
    UINT8   requestReset,
    UINT32  resetTimeout,
    INT32   errorCode,
    CHAR8*  infoText)
{
    DLEDS_RESULT        result = DLEDS_ERROR;
    int                 iptResult = IPT_OK;
    EDReqDLProgress     data;
    char                destUri[102];
    char                srcUri[102];
    char                tempSrcUri[102];
    UINT8               topoCounter = 0;
    UINT32              ownIpAddress;

    memset(&data, 0, sizeof(data));
    data.appVersion = EDReqDLProgress_appVersion;
    data.ongoingOperation = ongoingOperation;
    data.progressInfo = progressInfo;
    data.requestReset = requestReset;
    data.resetTimeout = resetTimeout;
    data.errorCode = errorCode;

    if (infoText != NULL)
    {
        strncpy(data.infoText, infoText, EDReqDLProgress_InfoTextSize);
    }
    else
    {
        data.infoText[0] = 0;
    }

    /* Use source URI from the latest request message received from the tool
       as destination URI for progress message */
    strcpy(destUri,sessionData.srcUri);

    /* Convert own IP address to URI string to be used as source URI */
    ownIpAddress = IPTCom_getOwnIpAddr();
    topoCounter = 0;
    iptResult = IPTCom_getUriHostPart(
                    ownIpAddress,      /* IP address */
                    tempSrcUri,        /* Pointer to resulting URI */
                    &topoCounter);     /* Topo counter */

    if (iptResult == IPT_OK)
    {
        strcpy(srcUri,"DLEDService@");
        strcat(srcUri,tempSrcUri);
        if (strstr(sessionData.srcUri, "@") == NULL)
        {
            strcpy(destUri,"DLEDClient@");
            strcat(destUri,sessionData.srcUri);
        }
        else
        {
            strcpy(destUri,sessionData.srcUri);
        }

        DebugError1("MDComAPI_putReqMsg() pRefValue(ongoingOperation)= (%d)", ongoingOperation);


        iptResult = MDComAPI_putReqMsg(
            oEDReqDLProgress,           /* ComId 290, DL Progress Request */
            (char*)&data,               /* Data buffer */
            sizeof(data),               /* Number of data to be send */
            1,                          /* Number of expected replies.
                                            0=unspecified */
            PROGRESS_REPLY_TIMEOUT,     /* Time-out value in milliseconds
                                            for receiving replies
                                            0=default value */
            progressQueueId,            /* Queue for communication result */
            NULL,                       /* Pointer to callback function */
            (void*)((UINT32)ongoingOperation),  /* Caller reference value */
            0,                          /* Topo counter */
            0,                          /* Dest ID */
            destUri,                    /* Destination URI */
            srcUri);                    /* Source URI */

        if (iptResult != IPT_OK)
        {
            /* The sending couldn't be started. */
            /* Error handling */
            DebugError2("MDComAPI_putReqMsg() FAILED with (%d)(%s) when sending progress message", iptResult, IPTCom_getErrorString(iptResult));
            result = DLEDS_ERROR;
        }
        else
        {
            DebugError0("MDComAPI_putReqMsg() OK for DL Progress Request");
            result = DLEDS_OK;
        }
    }
    else
    {
        DebugError1("IPTCom_getUriHostPart() FAILED with (%d) when sending progress message", iptResult);
        result = DLEDS_ERROR;
    }
    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_waitForProgressResponse
 *
 * Abstract:      This function waits for the progress response message. The
 *                message contains information if request in progress should
 *                be aborted or continue.
 *
 * Return value:  DLEDS_OK      - Response message is received successfully
 *                DLEDS_ERROR   -
 *
 * Globals:       progressQueueId
 */

DLEDS_RESULT dleds_waitForProgressResponse(
    UINT8* abortRequest)    /* OUT: Value from the response message
                                    Valid when function returns DLEDS_OK
                                    0 - Continue
                                    1 - Abort */
{
    DLEDS_RESULT        result = DLEDS_ERROR;
    int                 iptResult;
    UINT32              size;
    MSG_INFO            msgInfo;
    char*               pRecBuf;
    EDRepDLProgress*    pRepDLprogress;

    if (abortRequest == NULL)
    {
        return DLEDS_ERROR;
    }

    pRecBuf = NULL;  /* Use IPTCom allocated buffer */
    size = 0;

    iptResult = MDComAPI_getMsg(
            progressQueueId,    /* Queue ID */
            &msgInfo,           /* Message info */
            &pRecBuf,           /* Pointer to pointer to data
                                    buffer */
            &size,              /* Pointer to size. Size shall
                                    be set to own buffer size at
                                    the call. The IPTCom will
                                    return the number of received
                                    bytes */
            IPT_WAIT_FOREVER);  /* Wait for result */

    if (iptResult == MD_QUEUE_NOT_EMPTY)
    {
        if (msgInfo.msgType == MD_MSGTYPE_RESPONSE )
        {
            DebugError1("Progress response message received (%d)", msgInfo.pCallerRef);

            /* Take care of received response data */
            pRepDLprogress = (EDRepDLProgress*) pRecBuf;

            /* Check that version is valid */
            if (pRepDLprogress->appVersion != EDRepDLProgress_appVersion)
            {
                DebugError1("Version (0x%x) of progress response message not supported",
                    pRepDLprogress->appVersion);
                *abortRequest = PROGRESS_CONTINUE;
                result = DLEDS_ERROR;
            }
            else
            {
                /* Response message contains status on abortRequest */
                *abortRequest = pRepDLprogress->abortRequest;
                result = DLEDS_OK;
            }
        }
        else if (msgInfo.msgType == MD_MSGTYPE_RESULT)
        {
            DebugError1("Progress response result received (%d)", msgInfo.pCallerRef);

            /* Result message indicate that time out has run out */
            /* Take care of received result message */
            DebugError2("Result message with resultCode (%d) for comID (%d)", msgInfo.resultCode, msgInfo.comId);
            *abortRequest = PROGRESS_CONTINUE;
            result = DLEDS_ERROR;
        }
        else
        {
            DebugError2("Unexpected message type (%d), resultCode (%d)",
                msgInfo.msgType, msgInfo.resultCode);
            *abortRequest = PROGRESS_CONTINUE;
            result = DLEDS_ERROR;
        }
    }
    else
    {
        DebugError1("MDComAPI_getMsg() FAILED with (%d) when waiting for progress response message", iptResult);
        *abortRequest = PROGRESS_CONTINUE;
        result = DLEDS_ERROR;
    }

    if (pRecBuf != NULL)
    {
        iptResult = MDComAPI_freeBuf(pRecBuf);
        if (iptResult != IPT_OK)
        {
            /* error */
            DebugError1("MDComAPI_freeBuf returns error code (%d)", iptResult);
        }
    }

    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_reportProgress
 *
 * Abstract:      This function sends the progress request message and waits
 *                for the response message.
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       -
 */
DLEDS_RESULT dleds_reportProgress(
    UINT8   ongoingOperation,
    UINT8   progressInfo,
    UINT8   requestReset,
    UINT32  resetTimeout,
    INT32   errorCode,
    CHAR8*  infoText,
    UINT8*  pAbortRequest)
{
    DLEDS_RESULT result = DLEDS_OK;
    *pAbortRequest = 0;

    if (sessionData.protocolVersion == DLEDS_PROTOCOL_VERSION_2)
    {
        /* Send progress request and wait for response message */
        result = dleds_sendProgressRequest(
            ongoingOperation, progressInfo, requestReset,
            resetTimeout, errorCode, infoText);

        if (result == DLEDS_OK)
        {
            /* To Do: Always wait for response when implemented in MTVD */
            result = dleds_waitForProgressResponse(pAbortRequest);
            if (result != DLEDS_OK)
            {
                DebugError1("Receive Progress respons failed (%d)", result);
            }

        }
        else
        {
            DebugError1("Send Progress request failed (%d)", result);
        }
    }

    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_handleCleanUpRequest
 *
 * Abstract:      This function handles the Clean Up request. Download has to be
 *                allowed. The device is reset to download mode before any clean
 *                up is done. All End Device Sub packages previously downloaded
 *                with DLEDS are deleted from the device.
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *                DLEDS_EDSTATUS
 *                DLEDS_DL_RESET
 *
 * Globals:       -
 */
static DLEDS_RESULT dleds_handleCleanUpRequest(
    MSG_INFO*   pMsgInfo,
    char*       pReqData,
    UINT32      reqDataSize)
{
    DLEDS_RESULT    result = DLEDS_ERROR;
    DLEDS_RESULT    reportResult;
    int             iptResult;
    EDReqCleanUp*   pReqCleanUp;
    UINT8           topoCounter = 0;
    UINT32          ownIpAddress;
    char            tempSrcUri[102];
    char            srcUri[102];
    EDRepCleanUp    repCleanUp;
    CHAR8           infoText[115];
    UINT8           abortRequest;

    /* Verify that input parameters are valid */
    if ((pMsgInfo == NULL) || (pReqData == NULL) || (reqDataSize != sizeof(EDReqCleanUp)))
    {
        DebugError0("Invalid input parameters to function dleds_handleCleanUpRequest()");
        DebugError1("    pMsgInfo= 0x%X", pMsgInfo);
        DebugError1("    pReqData= 0x%X", pReqData);
        DebugError2("    reqDataSize= 0x%X, expected value= 0x%X", reqDataSize, sizeof(EDReqCleanUp));
        result = DLEDS_ERROR;
    }
    else
    {
        /* Handle received clean up request message */
        pReqCleanUp = (EDReqCleanUp*) pReqData;

        /* Check that version is valid */
        if (!(pReqCleanUp->appVersion == EDReqCleanUp_appVersion))
        {
            DebugError1("Version (0x%x) of clean up request message not supported",
                pReqCleanUp->appVersion);
            result = DLEDS_EDSTATUS;
        }
        else if (pReqCleanUp->functionCode != CLEAN_UP_ALL_PACKAGES)
        {
            DebugError1("Function code (0x%x) of clean up request message not supported",
                pReqCleanUp->appVersion);
            result = DLEDS_EDSTATUS;
        }
        else
        {
            /* Handle received clean up request message */
            /* Store session data needed for Clean Up after reset to DOWNLOAD MODE */
            sessionData.downloadResetReason = DLEDS_CLEAN_UP;
            sessionData.protocolVersion = DLEDS_PROTOCOL_VERSION_2;
            sessionData.sessionId = pMsgInfo->sessionId;
            sessionData.srcIpAddress = pMsgInfo->srcIpAddr;
            strcpy(sessionData.srcUri,pMsgInfo->srcURI);
            sessionData.requestReceiveTime = IPVosGetSecTimer();
            /* Fill in dummy values in session data, not used by Clean Up */
            sessionData.requestInfo.transactionId = 0;
            strcpy(sessionData.requestInfo.fileName, "dummy");
            strcpy(sessionData.requestInfo.fileServerHostName, "dummy");
            strcpy(sessionData.requestInfo.fileServerPath, "dummy");
            sessionData.requestInfo.fileSize = 0;
            sessionData.requestInfo.fileCRC = 0;
            sessionData.requestInfo.fileVersion = 0;


            /* Convert own IP address to URI string to be used as source URI */
            ownIpAddress = IPTCom_getOwnIpAddr();
            DebugError1("Own IP address to be converted to URI (0x%x)", ownIpAddress);
            topoCounter = 0;
            iptResult = IPTCom_getUriHostPart(
                ownIpAddress,      /* IP address */
                tempSrcUri,        /* Pointer to resulting URI */
                &topoCounter);     /* Topo counter */
            strcpy(srcUri,"DLEDService@");
            strcat(srcUri,tempSrcUri);

            /* Start writing data into response message */
            memset(&repCleanUp, 0, sizeof(EDRepCleanUp));
            repCleanUp.appVersion = EDRepCleanUp_appVersion;

            /* Download must be allowed to allow clean up */
            if (dleds_getDlAllowed() == FALSE)
            {
                repCleanUp.status = CLEAN_UP_NOT_ALLOWED;
            }
            else
            {
                repCleanUp.status = CLEAN_UP_OK;
            }

            iptResult = MDComAPI_putRespMsg(
                oEDRepCleanUp,          /* ComId (293)*/
                0,                      /* UserStatus */
                (char*) &repCleanUp,    /* pData */
                sizeof(EDRepCleanUp),   /* DataLength */
                pMsgInfo->sessionId,    /* SessionID */
                0,                      /* CallerQueue */
                NULL,                   /* Pointer to callback function */
                NULL,                   /* Caller reference value */
                0,                      /* Topo counter */
                pMsgInfo->srcIpAddr,    /* Destination IP address */
                0,                      /* DestId */
                NULL,                   /* Pointer to Dest URI */
                srcUri);                /* Pointer to Src URI */

            if (iptResult == IPT_OK)
            {
                DebugError0("MDComAPI_putRespMsg() returned OK");
                if (repCleanUp.status == CLEAN_UP_OK)
                {
                    /* Send progress request and wait for response message */
                    strcpy(infoText, "Clean up request received");
                    /* Add delay for response message above to be sent */
                    IPTVosTaskDelay(1000);
                    reportResult = dleds_reportProgress(PROGRESS_CLEAN_UP_INITIALIZED, PROGRESS_3, PROGRESS_NO_RESET_IN_PROGRESS,
                                                        PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, infoText, &abortRequest);
                    if (abortRequest == PROGRESS_CONTINUE)
                    {
                        reportResult = dleds_reportProgress(PROGRESS_ED_RESTART_PROGRESS, PROGRESS_5, PROGRESS_RESET_REQUEST,
                                                            PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);
                        if (abortRequest == PROGRESS_CONTINUE)
                        {
                            /* Make a reset to download mode and make a clean up */
                            result = dleds_writeStorageFile(&sessionData);
                            if (result == DLEDS_OK)
                            {
                                result = DLEDS_CLEAN_UP_RESET;
                            }
                            else
                            {
                                /* Failed to write session data to temp storage file */
                                /* Abort clean up and wait for next request */
                                result = DLEDS_EDSTATUS;
                            }
                        }
                        else
                        {
                            /* Abort clean up and wait for next request */
                            result = DLEDS_EDSTATUS;
                        }
                    }
                    else
                    {
                        /* Abort clean up and wait for next request */
                        result = DLEDS_EDSTATUS;
                    }
                }
                else
                {
                    /* DL not allowed, abort clean up and wait for next request */
                    result = DLEDS_EDSTATUS;
                }
            }
            else
            {
                /*    */
                DebugError1("MDComAPI_putRespMsg() failed with error (%d)",
                    iptResult);
                /*
                * forget this request,
                * wait for the next request
                */
                result = DLEDS_EDSTATUS;
            }
        }
    }
    /* To avoid compiler warning */
    (void)reportResult;

    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_wait_for_request
 *
 * Abstract:      This function waits for one of the request messages.
 *                - Request Status (version 1), ComID = 272
 *                - Request Status (version 2), ComID = 278
 *                - Request Version Info, ComID = 274
 *                - Request Send File (version 2), ComID = 276
 *                - Request Clean Up, ComID = 292
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       appResultCode - is set to to be included in response message
 *                sessionData - updated with information from the request message
 */
DLEDS_RESULT dleds_wait_for_request(
    void )
{
    DLEDS_RESULT        result = DLEDS_OK;
    int                 iptResult = IPT_OK;
    MSG_INFO            msgInfo;
    char*               pReqData;
    EDIdent             pdBuffer;
    UINT32              reqDataSize = 0;

    /* Retrieve information to be included in the PD message */
    dleds_versionRetrieve(&pdBuffer);
    PDComAPI_put(pdHandle,(BYTE*)&pdBuffer);
    PDComAPI_source(DLEDS_SCHEDULE_GROUP_ID);

    pReqData = NULL;    /* Use buffer allocated by IPTCom */
    iptResult = MDComAPI_getMsg(
                    requestQueueId, /* Queue identification */
                    &msgInfo,       /* Pointer to message information */
                    &pReqData,      /* Pointer to a pointer to data buffer */
                    &reqDataSize,   /* Pointer to data length */
                    IPT_NO_WAIT);   /* Timeout */

    if (iptResult == MD_QUEUE_NOT_EMPTY)
    {
        if (msgInfo.msgType == MD_MSGTYPE_REQUEST)
        {
            DebugError0("Message Request is received");
            DebugError1("COM ID= %d", msgInfo.comId);
            DebugError1("Session ID= %d", msgInfo.sessionId);
            DebugError1("Source IP address= 0x%x", msgInfo.srcIpAddr);
            DebugError1("Source URI= %s", msgInfo.srcURI);
            DebugError1("Destination URI= %s", msgInfo.destURI);

            switch(msgInfo.comId)
            {
                case iEDReqStatus:
                case iEDReqStatus2:
                {
                    result = dleds_handleStatusRequests(&msgInfo, pReqData, reqDataSize, &pdBuffer);
                    break;
                }
                case iMCGReqSendFile:
                case iMCGReqSendFile2:
                case iEDReqSendFile2:
                {
                    result = dleds_handleSendFileRequests(&msgInfo, pReqData, reqDataSize);
                    break;
                }
                case iEDReqVersionInfo:
                {
                    result = dleds_handleVersionInfoRequest(&msgInfo, pReqData, reqDataSize);
                    break;
                }
                case iEDReqCleanUp:
                {
                    result = dleds_handleCleanUpRequest(&msgInfo, pReqData, reqDataSize);
                    break;
                }
                default:
                {
                    /* Unexpected COM ID */
                    /* Do nothing, stay in this state and wait for next message */
                    DebugError1("Unexpected COM ID (%d) received", msgInfo.comId);
                    result = DLEDS_ERROR;
                    break;
                }
            }
        }
        else
        {
            /* Unexpected message type */
            /* Do nothing, stay in this state and wait for next message */
            DebugError1("Unexpected message type(%d) received", msgInfo.msgType);

            result = DLEDS_ERROR;
        }
    }
    else if (iptResult == MD_QUEUE_EMPTY)
    {
        /* Queue is empty, wait 1 second until next check of queue */
        IPTVosTaskDelay(1000);
        result = DLEDS_ERROR;
    }
    else
    {
        /* error */
        DebugError1("MDComAPI_getMsg returns error code (%d)", iptResult);

        /* Wait 1 second until next check of queue */
        IPTVosTaskDelay(1000);
        result = DLEDS_ERROR;
    }

    /* Make sure that buffer allocated by IPTCom is returned */
    if (pReqData != NULL)
    {
        /* Free buffer allocated by IPTCom */
        iptResult = MDComAPI_freeBuf(pReqData);
        if (iptResult != 0)
        {
            /* error */
            DebugError1("MDComAPI_freeBuf returns error code (%d)", iptResult);
        }
    }

    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_send_status_message
 *
 * Abstract:      This function .
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       appResultCode
 *                sessionData
 */
DLEDS_RESULT dleds_send_status_message(
    void )
{
    DLEDS_RESULT result = DLEDS_OK;
    int iptResult = IPT_OK;
    MCGDLFTfSt sendStatusData;
    char destUri[102];
    char srcUri[102];
    char tempSrcUri[102];
    static int retryCounter = 0;
    UINT8 topoCounter = 0;
    UINT32 ownIpAddress;

    if (sessionData.protocolVersion == DLEDS_PROTOCOL_VERSION_1)
    {
        /* Status message should be sent */
        sendStatusData.appVersion = MCGDLFTfSt_appVersion;
        sendStatusData.appResultCode = appResultCode;
        sendStatusData.transactionId = sessionData.requestInfo.transactionId;

        /* Use source URI from Request message as destination URI for STATUS message */
        strcpy(destUri,sessionData.srcUri);

        /* Convert own IP address to URI string to be used as source URI */
        ownIpAddress = IPTCom_getOwnIpAddr();
        topoCounter = 0;
        iptResult = IPTCom_getUriHostPart(
                        ownIpAddress,      /* IP address */
                        tempSrcUri,        /* Pointer to resulting URI */
                        &topoCounter);     /* Topo counter */

        if (iptResult == IPT_OK)
        {
            strcpy(srcUri,"DLEDService@");
            strcat(srcUri,tempSrcUri);

            iptResult = MDComAPI_putDataMsg(
                iMCGDLFileTfST,             /* ComId (854)*/
                (char*) &sendStatusData,    /* pData */
                sizeof(MCGDLFTfSt),         /* dataLength */
                resultQueueId,              /* CallerQueue */
                NULL,                       /* Pointer to callback function */
                NULL,                       /* Caller reference value */
                0,                          /* Topo counter */
                0,                          /* DestId */
                destUri,                    /* Pointer to Dest URI */
                srcUri);                    /* Pointer to Src URI */

            if (iptResult == IPT_OK)
            {
                retryCounter = 0;
                result = DLEDS_OK;
            }
            else if (iptResult == (int)IPT_QUEUE_ERR)
            {
                DebugError0("MDComAPI_putDataMsg() failed with IPT_QUEUE_ERROR");
                /* Check if we should make a retry */
                if (retryCounter < 3)
                {
                    retryCounter++;
                    IPTVosTaskDelay(10);
                    result = DLEDS_ERROR;
                }
                else
                {
                    /*
                     * forget this request,
                     * reset to RUN mode and wait for the next
                     */
                    retryCounter = 0;
                    result = DLEDS_NO_OF_RETRIES_EXCEEDED;
                }
            }
            else
            {
                /*  Other error returned from MDComAPI_putDataMsg */
                DebugError1("MDComAPI_putDataMsg() failed with error (%d)",
                    iptResult);
                /*
                 * forget this request,
                 * reset to RUN mode and wait for the next request
                 */
                retryCounter = 0;
                DLEDS_STATE = RESET_MODE;
            }
        }
        else
        {
            /* Failed to convert own IP address to URI string*/
            DebugError1("Conversion of own IP address to URI failed (%d)",
                iptResult);
            retryCounter = 0;
            result = DLEDS_ERROR;
        }
    }
    else
    {
        DLEDS_STATE = RESET_MODE;
        result = DLEDS_ERROR;
    }
    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_send_response_message
 *
 * Abstract:      This function .
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       appResultCode
 *                sessionData
 */
DLEDS_RESULT dleds_send_response_message(
    void )
{
    DLEDS_RESULT result = DLEDS_OK;
    DLEDS_RESULT reportResult;
    int iptResult = IPT_OK;
    MCGRepSendFile responseData;
    static int retryCounter = 0;
    UINT8 topoCounter = 0;
    char tempSrcUri[102];
    char srcUri[102];
    UINT32 ownIpAddress;
    UINT8 abortRequest;

    /* Response message should be sent */
    responseData.appResultCode = appResultCode;

    /* Convert own IP address to URI string to be used as source URI */
    ownIpAddress = IPTCom_getOwnIpAddr();
    topoCounter = 0;
    iptResult = IPTCom_getUriHostPart(
                    ownIpAddress,      /* IP address */
                    tempSrcUri,        /* Pointer to resulting URI */
                    &topoCounter);     /* Topo counter */
    strcpy(srcUri,"DLEDService@");
    strcat(srcUri,tempSrcUri);
    if (sessionData.protocolVersion == DLEDS_PROTOCOL_VERSION_1)
    {
        responseData.appVersion = MCGRepSendFile_appVersion;
        iptResult = MDComAPI_putRespMsg(
            oMCGRepSendFile,            /* ComId (851)*/
            0,                          /* UserStatus */
            (char*) &responseData,      /* pData */
            sizeof(MCGRepSendFile),     /* DataLength */
            sessionData.sessionId,      /* SessionID */
            resultQueueId,              /* CallerQueue */
            NULL,                       /* Pointer to callback function */
            NULL,                       /* Caller reference value */
            0,                          /* Topo counter */
            sessionData.srcIpAddress,   /* Destination IP address */
            0,                          /* DestId */
            NULL,                       /* Pointer to Dest URI */
            srcUri);                    /* Pointer to Src URI */
    }
    else
    {
        responseData.appVersion = EDReqSendFile2_appVersion;
        iptResult = MDComAPI_putRespMsg(
            oEDRepSendFile2,            /* ComId (277)*/
            0,                          /* UserStatus */
            (char*) &responseData,      /* pData */
            sizeof(MCGRepSendFile),     /* DataLength */
            sessionData.sessionId,      /* SessionID */
            resultQueueId,              /* CallerQueue */
            NULL,                       /* Pointer to callback function */
            NULL,                       /* Caller reference value */
            0,                          /* Topo counter */
            sessionData.srcIpAddress,   /* Destination IP address */
            0,                          /* DestId */
            NULL,                       /* Pointer to Dest URI */
            srcUri);                    /* Pointer to Src URI */

        if (appResultCode != DL_NOT_ALLOWED)
        {
            reportResult = dleds_reportProgress(PROGRESS_DL_INITIALIZED, PROGRESS_3, PROGRESS_NO_RESET_IN_PROGRESS,
                                                PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);
            /* To Do: Handle abort request */
        }
    }
    if (iptResult == IPT_OK)
    {
        retryCounter = 0;
        result = DLEDS_OK;
        if (appResultCode != DL_NOT_ALLOWED)
        {
            reportResult = dleds_reportProgress(PROGRESS_ED_RESTART_PROGRESS, PROGRESS_5, PROGRESS_RESET_REQUEST,
                                                PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);

            /* To Do: Handle abort request */
        }
    }
    else if (iptResult == (int)IPT_QUEUE_ERR)
    {
        DebugError0("MDComAPI_putRespMsg() failed with IPT_QUEUE_ERROR");
        /* Check if we should make a retry */
        if (retryCounter < 3)
        {
            retryCounter++;
            result = DLEDS_ERROR;
            IPTVosTaskDelay(10);
        }
        else
        {
            /*
             * forget this request session,
             * wait for the next request
             */
            retryCounter = 0;
            result = DLEDS_NO_OF_RETRIES_EXCEEDED;
        }
    }
    else
    {
        /*   */
        DebugError1("MDComAPI_putRespMsg() failed with error (%d)",
            iptResult);
        /*
         * forget this request,
         * wait for the next request
         */
        retryCounter = 0;
        result = DLEDS_ERROR;
    }
    /* To avoid compiler warning */
    (void)reportResult;

    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_wait_for_result
 *
 * Abstract:      This function waits for the result of previous response or
 *                status message. Which message type that was sent depends on
 *                the current operation mode.
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       sessionData
 */
DLEDS_RESULT dleds_wait_for_result(
    void)
{
    DLEDS_RESULT    result = DLEDS_OK;
    int             iptResult = IPT_OK;
    MSG_INFO        msgInfo;
    char*           pResultData;
    UINT32          resultDataSize = 0;
    static int      retryCounter = 0;

    /*
     * Let IPTCom allocate buffer
     * Use IPT_WAIT_FOREVER since there will always be
     * a result received within a limited time
     */
    pResultData = NULL;
    iptResult = MDComAPI_getMsg(resultQueueId,
        &msgInfo,
        &pResultData,
        &resultDataSize,
        IPT_WAIT_FOREVER);

    if (iptResult == MD_QUEUE_NOT_EMPTY)
    {
        if (msgInfo.msgType == MD_MSGTYPE_RESULT)
        {
            if (msgInfo.resultCode == MD_SEND_OK)
            {
                if (msgInfo.comId == oMCGRepSendFile)
                {
                    /* Result of response message received (COMID = 851) */
                    retryCounter = 0;
                    result = DLEDS_OK;
                }
                else if (msgInfo.comId == iMCGDLFileTfST)
                {
                    /* Result of status message received (COMID = 854) */
                    retryCounter = 0;
                    result = DLEDS_OK;
                }
            }
            else
            {
                /* Check if we should make a retry */
                DebugError1("Error code (%d) returned as result message sent",
                    msgInfo.resultCode);
                if (retryCounter < 3)
                {
                    retryCounter++;
                    result = DLEDS_ERROR;
                    IPTVosTaskDelay(10);
                }
                else
                {
                    /*
                     * forget this request session,
                     * reset to RUN mode and wait for the next request
                     */
                    retryCounter = 0;
                    result = DLEDS_NO_OF_RETRIES_EXCEEDED;
                }
            }
        }
        else
        {
            /* Unexpected message type */
            /* Do nothing, stay in this state and wait for next message */
            DebugError1("Unexpected message type(%d) received",
                msgInfo.msgType);
            if (pResultData != NULL)
            {
                /* Free buffer allocated by IPTCom */
                iptResult = MDComAPI_freeBuf(pResultData);
                if (iptResult != 0)
                {
                    /* error */
                    DebugError1("MDComAPI_freeBuf returns error code (%d)",
                        iptResult);
                }
            }
            result = DLEDS_ERROR;
        }
    }
    else
    {
        DebugError0("Queue is empty");
        result = DLEDS_ERROR;
    }
    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_handlePackageVersion1
 *
 * Abstract:      This function
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       dledsTempDirectory
 *                sessionData
 *                appResultCode
 */
static DLEDS_RESULT dleds_handlePackageVersion1(
    void)
{
    DLEDS_RESULT result = DLEDS_OK;

    /* Select where to store temporary files for FTP and unpacking of ED package */
    if (dleds_setDledsTempPath() != DLEDS_OK)
    {
        /* Failed to create temporary directory to be used when receiving ED package */
        /* Save this status to be able to return DiskErr as resultcode on download request */
        result = DL_DISK_FULL;
    }

    if (result == DLEDS_OK)
    {
        /* Uninstall any previously installed sub package to  */
        /* Check if BT sub package is to be installed */
        if (strstr(sessionData.requestInfo.fileName, "_bt_") != NULL)
        {
            if (dledsCleanupBT() != DLEDS_OK)
            {
                DebugError0("Failed to clean up previously installed BT package");
            }
        }
        /* Check if CU sub package is to be installed */
        if (strstr(sessionData.requestInfo.fileName, "_cu_") != NULL)
        {
            if (dledsCleanupCU() != DLEDS_OK)
            {
                DebugError0("Failed to clean up previously installed CU package");
            }
        }

        /* Check if there is disk space available on file system to download, unpack and install ED package */
        if (dleds_diskSpaceAvailable(dledsTempDirectory, sessionData.requestInfo.fileSize) == DLEDS_ERROR)
        {
            DebugError1("dleds_handlePackageVersion1: Not enought disk space, filesize is %u kByte", sessionData.requestInfo.fileSize);
            /*
            * Make sure that response message is sent.
            * Error code should be sent in this case
            */
            result = DL_DISK_FULL;
        }
    }

    if (result == DLEDS_OK)
    {
        /* Use FTP to copy file from host */
        result = dleds_ftpCopyFile();
    }

    if (result == DLEDS_OK)
    {
        /* Call DVS function to install ED package */
        result = dledsInstallEDPackage(sessionData.requestInfo.fileName, dledsTempDirectory);
        if (result == DLEDS_OK)
        {
            appResultCode = DL_OK;
        }
        else if (result == DLEDS_SCI_ERROR)
        {
            appResultCode = DL_SCI_ERROR;
        }
        else if (result == DLEDS_INSTALLATION_ERROR)
        {
            appResultCode = DL_INSTALLATION_ERROR;
        }
        else
        {
            appResultCode = DL_ED_ERROR;
        }
    }
    else
    {
        if (result == DLEDS_CRC_ERROR)
        {
            appResultCode = DL_CRC_ERROR;
            result = DLEDS_ERROR;
        }
        else if (result == DL_DISK_FULL)
        {
            appResultCode = DL_DISK_FULL;
            result = DLEDS_ERROR;
        }
        else
        {
            appResultCode = DL_FTP_ERROR;
            result = DLEDS_ERROR;
        }
    }
    if (sessionData.protocolVersion == DLEDS_PROTOCOL_VERSION_2)
    {
        /* Save information for final Progress Request after reboot */
        sessionData.completedOperation = 1;
        sessionData.appResultCode = appResultCode;
        result = dleds_writeStorageFile(&sessionData);
    }
    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_handlePackageVersion2
 *
 * Abstract:      This function
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *                DLEDS_STATE_CHANGE
 *
 * Globals:       dledsTempDirectory
 *                sessionData
 *                appResultCode
 */
static DLEDS_RESULT dleds_handlePackageVersion2(
    void)
{
    DLEDS_RESULT result = DLEDS_OK;
    DLEDS_RESULT tempResult = DLEDS_OK;

#if 0   /* Temporarily remove to test if Package version 2 could be handled wit protocol version 1 */
    if (sessionData.protocolVersion == DLEDS_PROTOCOL_VERSION_2)
    {
#endif
        if (strstr(sessionData.requestInfo.fileName,".edsp2") == NULL)
        {
            DebugError1("Not an EDSP2 package file (%s)", sessionData.requestInfo.fileName);
            result = DL_ED_ERROR;
        }

        if (result == DLEDS_OK)
        {
            /* Select where to store temporary files for FTP and unpacking of ED package */
            if(dleds_setDledsTempPath() != DLEDS_OK)
            {
                /* Failed to create temporary directory to be used when receiving EDSP package */
                /* Save this status to be able to return DiskErr as resultcode on download request */
                result = DL_DISK_FULL;
            }
        }

        if (result == DLEDS_OK)
        {
            /* Check if there is disk space available on file system to download, unpack and install ED package */
            if (dleds_diskSpaceAvailable(dledsTempDirectory, sessionData.requestInfo.fileSize) == DLEDS_ERROR)
            {
                DebugError1("dleds_handlePackageVersion2: Not enought disk space, filesize is %d kByte", sessionData.requestInfo.fileSize);
                /* Make sure that response message is sent. Error code should be sent in this case */
                result = DL_DISK_FULL;
            }
        }

        if (result == DLEDS_OK)
        {
            /* Use FTP to copy file from host */
            result = dleds_ftpCopyFile();
        }

        if (result == DLEDS_OK)
        {
            result = dledsInstallEDSP2Package(sessionData.requestInfo.fileName, dledsTempDirectory);

            if (result == DLEDS_OK)
            {
                appResultCode = DL_OK;
            }
            else if (result == DLEDS_SCI_ERROR)
            {
                appResultCode = DL_SCI_ERROR;
            }
            else if (result == DLEDS_INSTALLATION_ERROR)
            {
                appResultCode = DL_INSTALLATION_ERROR;
            }
            else if (result == DLEDS_STATE_CHANGE)
            {
                appResultCode = DL_OK;
            }
            else
            {
                appResultCode = DL_ED_ERROR;
            }

            /* make sure that progress request is sent when restarting in RUN/IDLE */
            if (result != DLEDS_STATE_CHANGE)
            {
                sessionData.completedOperation = 1;
                sessionData.appResultCode = appResultCode;
                tempResult = dleds_writeStorageFile(&sessionData);
            }
        }
        else
        {
            if (result == DLEDS_CRC_ERROR)
            {
                appResultCode = DL_CRC_ERROR;
                result = DLEDS_ERROR;
            }
            else if (result == DL_DISK_FULL)
            {
                appResultCode = DL_DISK_FULL;
                result = DLEDS_ERROR;
            }
            else if (result == DL_FTP_ERROR)
            {
                appResultCode = DL_FTP_ERROR;
                result = DLEDS_ERROR;
            }
            else
            {
                appResultCode = DL_ED_ERROR;
                result = DLEDS_ERROR;
            }
            sessionData.completedOperation = 1;
            sessionData.appResultCode = appResultCode;
            tempResult = dleds_writeStorageFile(&sessionData);
        }
#if 0   /* Temporarily remove to test if Package version 2 could be handled wit protocol version 1 */
    }
#endif

    /* To avoid compiler warning */
    (void)tempResult;

    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_ping
 *
 * Abstract:      This function
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       sessionData
 */
static void dleds_ping(
    void)
{
    int     iptResult = IPT_OK;
    EDPing  sendPingData;
    char    destUri[102];
    char    srcUri[102];
    char    tempSrcUri[102];
    UINT8   topoCounter = 0;
    UINT32  ownIpAddress;
    UINT8   first = 0;

    /* Fill in data in ping message */
    memset(&sendPingData, 0, sizeof(EDPing));
    sendPingData.appVersion = EDPing_appVersion;

    /* Use source URI from Request message as destination URI for STATUS message */
    strcpy(destUri,sessionData.srcUri);

    /* Convert own IP address to URI string to be used as source URI */
    ownIpAddress = IPTCom_getOwnIpAddr();
    topoCounter = 0;
    iptResult = IPTCom_getUriHostPart(
                    ownIpAddress,      /* IP address */
                    tempSrcUri,        /* Pointer to resulting URI */
                    &topoCounter);     /* Topo counter */


    if (iptResult == IPT_OK)
    {
        strcpy(srcUri,"DLEDService@");
        strcat(srcUri,tempSrcUri);

        while(1)
        {
            iptResult = MDComAPI_putDataMsg(
                oEDPing,                    /* ComId (294)*/
                (char*) &sendPingData,      /* pData */
                sizeof(EDPing),             /* dataLength */
                0,                          /* CallerQueue */
                NULL,                       /* Pointer to callback function */
                NULL,                       /* Caller reference value */
                0,                          /* Topo counter */
                0,                          /* DestId */
                destUri,                    /* Pointer to Dest URI */
                srcUri);                    /* Pointer to Src URI */

            /* Only write to log file for first error in sequence */
            if ((iptResult != IPT_OK) && (first == 0))
            {
                DebugError1("MDComAPI_putDataMsg() failed with error (%d)",
                    iptResult);
            }
            else
            {
                first = 0;
            }

            IPTVosTaskDelay(5000);
        }
    }
    else
    {
        /* Failed to convert own IP address to URI string*/
        DebugError1("Conversion of own IP address to URI failed (%d)", iptResult);
        DebugError0("Ping task is aborted");
    }
}



/*******************************************************************************
 *
 * Function name: dleds_activatePingTask
 *
 * Abstract:      This function
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       -
 */
static void dleds_activatePingTask(
    void)
{
    VOS_THREAD_ID pingThread;

    if((pingThread = IPTVosThreadSpawn("pingTask",
                            DLEDS_THREAD_POLICY,
                            DLEDS_THREAD_PRIORITY,
                            DLEDS_THREAD_STACK_SIZE,
                            (IPT_THREAD_ROUTINE)dleds_ping,
                            NULL)) == 0)
    {
        DebugError0("Unable to spawn thread for Ping task");
    }
    else
    {
        DebugError0("Thread spawned for Ping task");
    }
}


/*******************************************************************************
 *
 * Function name: dleds_download_mode
 *
 * Abstract:      This function uninstalls any previously installed package type
 *                that is about to be installed. Checks that there is enough disk
 *                space available to retrieve ED package from FTP server, unpack
 *                and install it. At least
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *                DLEDS_STATE_CHANGE
 *
 * Globals:       dledsTempDirectory
 *                sessionData
 *                appResultCode
 */
DLEDS_RESULT dleds_download_mode(
    void)
{
    DLEDS_RESULT result = DLEDS_OK;
    DLEDS_RESULT reportResult;
    EDIdent      pdBuffer;
    UINT8        abortRequest = 0;

    /* Should not be sent if preparing to run in DLEDS Idle Mode for special package handling */
    if (sessionData.downloadResetReason != DLEDS_DL_IDLE_MODE)
    {
        reportResult = dleds_reportProgress(PROGRESS_ED_IN_DL_MODE, PROGRESS_10, PROGRESS_NO_RESET_IN_PROGRESS,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);
    }

    if (abortRequest == 1)
    {
        DebugError0("Download Operation cancelled by User");
        appResultCode = -200;
        return DLEDS_ERROR;
    }
    if (sessionData.protocolVersion == DLEDS_PROTOCOL_VERSION_2)
    {
        /* Activate pingTask */
        dleds_activatePingTask();
    }
    /* Retrieve information to be included in the IPTCom PD message (comid=271) */
    dleds_versionRetrieve(&pdBuffer);
    PDComAPI_put(pdHandle,(BYTE*)&pdBuffer);
    PDComAPI_source(DLEDS_SCHEDULE_GROUP_ID);

    /* Find out the task to be done in download mode */
    if (sessionData.downloadResetReason == DLEDS_DOWNLOAD)
    {
        if (sessionData.packageVersion == DLEDS_PACKAGE_VERSION_1)
        {
            result = dleds_handlePackageVersion1();
            reportResult = dleds_reportProgress(PROGRESS_INSTALL_COMPLETED, PROGRESS_90, PROGRESS_NO_RESET_IN_PROGRESS,
                                                PROGRESS_USE_DEFAULT_TIMEOUT, appResultCode, NULL, &abortRequest);
            reportResult = dleds_reportProgress(PROGRESS_ED_RESTART_PROGRESS, PROGRESS_95, PROGRESS_RESET_REQUEST,
                                                PROGRESS_USE_DEFAULT_TIMEOUT, appResultCode, NULL, &abortRequest);
        }
        else if (sessionData.packageVersion == DLEDS_PACKAGE_VERSION_2)
        {
            result = dleds_handlePackageVersion2();
            /* Dont't send progress if installation handled by target in DLEDS IDLE MODE */
            if (result != DLEDS_STATE_CHANGE)
            {
                reportResult = dleds_reportProgress(PROGRESS_INSTALL_COMPLETED, PROGRESS_90, PROGRESS_NO_RESET_IN_PROGRESS,
                                                PROGRESS_USE_DEFAULT_TIMEOUT, appResultCode, NULL, &abortRequest);
                reportResult = dleds_reportProgress(PROGRESS_ED_RESTART_PROGRESS, PROGRESS_95, PROGRESS_RESET_REQUEST,
                                                PROGRESS_USE_DEFAULT_TIMEOUT, appResultCode, NULL, &abortRequest);
            }
        }
        else
        {
            /* Unknown package */
            result = DLEDS_ERROR;
        }
    }
    else if (sessionData.downloadResetReason == DLEDS_CLEAN_UP)
    {
        reportResult = dleds_reportProgress(PROGRESS_CLEAN_UP_PROGRESS, PROGRESS_70, PROGRESS_NO_RESET_IN_PROGRESS,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);
        result = dledsPlatformRemovePackages();
        if (result != DLEDS_OK)
        {
            DebugError1("dledsPlatformRemovePackages return error code (%d)", result);
            appResultCode = DL_REQUEST_FAILED;
        }
        else
        {
            appResultCode = DL_OK;
        }
        reportResult = dleds_reportProgress(PROGRESS_CLEAN_UP_COMPLETED, PROGRESS_90, PROGRESS_NO_RESET_IN_PROGRESS,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, appResultCode, NULL, &abortRequest);
        reportResult = dleds_reportProgress(PROGRESS_ED_RESTART_PROGRESS,PROGRESS_95, PROGRESS_RESET_REQUEST,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, appResultCode, NULL, &abortRequest);
        sessionData.completedOperation = 1;
        sessionData.appResultCode = appResultCode;
        result = dleds_writeStorageFile(&sessionData);
        /* ToDo: Check result value. What happens if dledsPlatformRemovePackages fails */
    }
    else if (sessionData.downloadResetReason == DLEDS_DL_IDLE_MODE)
    {
        result = DLEDS_STATE_CHANGE;
    }
    else
    {
        /* Unknown task */
        result = DLEDS_ERROR;
    }
    /* To avoid compiler warning */
    (void)reportResult;

    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_add_ipt_config
 *
 * Abstract:      This function .
 *
 * Return value:  IPT_OK
 *                IPT_ERROR
 *
 * Globals:
 *
 */
int dleds_add_ipt_config(
    void)
{
    int iptRes = IPT_ERROR;
    IPT_CONFIG_EXCHG_PAR exchg;
    IPT_CONFIG_DATASET_EXT datasetExt;
    IPT_DATASET_FORMAT datasetFormat[40];
    UINT32 dataSetId = 0;
    char *pURI = "grpAll.aCar.lCst";

    /* datasetId 850 iMCGReqSendFile */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT32;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_CHAR8;
    datasetFormat[2].size = 256;
    datasetFormat[3].id = IPT_CHAR8;
    datasetFormat[3].size = 64;
    datasetFormat[4].id = IPT_CHAR8;
    datasetFormat[4].size = 256;
    datasetFormat[5].id = IPT_UINT32;
    datasetFormat[5].size = 1;
    datasetFormat[6].id = IPT_UINT32;
    datasetFormat[6].size = 1;
    datasetFormat[7].id = IPT_UINT32;
    datasetFormat[7].size = 1;
    datasetFormat[8].id = IPT_UINT16;
    datasetFormat[8].size = 1;
    datasetFormat[9].id = IPT_CHAR8;
    datasetFormat[9].size = 512;
    datasetExt.datasetId = iMCGReqSendFile;
    datasetExt.nLines = 10;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 851 oMCGRepSendFile */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_INT32;
    datasetFormat[1].size = 1;
    datasetExt.datasetId = oMCGRepSendFile;
    datasetExt.nLines = 2;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 852 iMCGReqSendFile2 */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT32;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_CHAR8;
    datasetFormat[2].size = 256;
    datasetFormat[3].id = IPT_CHAR8;
    datasetFormat[3].size = 64;
    datasetFormat[4].id = IPT_CHAR8;
    datasetFormat[4].size = 256;
    datasetFormat[5].id = IPT_UINT32;
    datasetFormat[5].size = 1;
    datasetFormat[6].id = IPT_UINT32;
    datasetFormat[6].size = 1;
    datasetFormat[7].id = IPT_UINT32;
    datasetFormat[7].size = 1;
    datasetFormat[8].id = IPT_UINT16;
    datasetFormat[8].size = 1;
    datasetFormat[9].id = IPT_CHAR8;
    datasetFormat[9].size = 0;
    datasetExt.datasetId = iMCGReqSendFile2;
    datasetExt.nLines = 10;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 854 iMCGDLFileTfST */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_INT32;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_UINT32;
    datasetFormat[2].size = 1;
    datasetExt.datasetId = iMCGDLFileTfST;
    datasetExt.nLines = 3;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 271 oEDIdent */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT8;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_UINT8;
    datasetFormat[2].size = 1;
    datasetFormat[3].id = IPT_UINT8;
    datasetFormat[3].size = 1;
    datasetFormat[4].id = IPT_UINT8;
    datasetFormat[4].size = 1;
    datasetFormat[5].id = IPT_UINT8;
    datasetFormat[5].size = 12;
    datasetFormat[6].id = IPT_UINT8;
    datasetFormat[6].size = 1;
    datasetFormat[7].id = IPT_UINT8;
    datasetFormat[7].size = 1;
    datasetFormat[8].id = IPT_UINT8;
    datasetFormat[8].size = 1;
    datasetFormat[9].id = IPT_UINT8;
    datasetFormat[9].size = 1;
    datasetFormat[10].id = IPT_UINT8;
    datasetFormat[10].size = 12;
    datasetFormat[11].id = IPT_UINT8;
    datasetFormat[11].size = 1;
    datasetFormat[12].id = IPT_UINT8;
    datasetFormat[12].size = 1;
    datasetFormat[13].id = IPT_UINT8;
    datasetFormat[13].size = 1;
    datasetFormat[14].id = IPT_UINT8;
    datasetFormat[14].size = 1;
    datasetFormat[15].id = IPT_UINT8;
    datasetFormat[15].size = 12;
    datasetFormat[16].id = IPT_UINT8;
    datasetFormat[16].size = 1;
    datasetFormat[17].id = IPT_UINT8;
    datasetFormat[17].size = 1;
    datasetFormat[18].id = IPT_UINT8;
    datasetFormat[18].size = 1;
    datasetFormat[19].id = IPT_UINT8;
    datasetFormat[19].size = 1;
    datasetFormat[20].id = IPT_UINT8;
    datasetFormat[20].size = 12;
    datasetFormat[21].id = IPT_UINT8;
    datasetFormat[21].size = 1;
    datasetFormat[22].id = IPT_UINT8;
    datasetFormat[22].size = 1;
    datasetFormat[23].id = IPT_UINT8;
    datasetFormat[23].size = 1;
    datasetFormat[24].id = IPT_UINT8;
    datasetFormat[24].size = 1;
    datasetFormat[25].id = IPT_UINT8;
    datasetFormat[25].size = 12;
    datasetFormat[26].id = IPT_CHAR8;
    datasetFormat[26].size = 16;
    datasetFormat[27].id = IPT_CHAR8;
    datasetFormat[27].size = 16;
    datasetFormat[28].id = IPT_CHAR8;
    datasetFormat[28].size = 16;
    datasetFormat[29].id = IPT_CHAR8;
    datasetFormat[29].size = 16;
    datasetFormat[30].id = IPT_CHAR8;
    datasetFormat[30].size = 16;
    datasetFormat[31].id = IPT_CHAR8;
    datasetFormat[31].size = 16;
    datasetFormat[32].id = IPT_UINT8;
    datasetFormat[32].size = 1;
    datasetFormat[33].id = IPT_UINT8;
    datasetFormat[33].size = 1;
    datasetFormat[34].id = IPT_UINT8;
    datasetFormat[34].size = 1;
    datasetFormat[35].id = IPT_UINT8;
    datasetFormat[35].size = 13;
    datasetFormat[36].id = IPT_CHAR8;
    datasetFormat[36].size = 16;
    datasetFormat[37].id = IPT_CHAR8;
    datasetFormat[37].size = 128;
    datasetFormat[38].id = IPT_UINT8;
    datasetFormat[38].size = 44;
    datasetExt.datasetId = oEDIdent;
    datasetExt.nLines = 39;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 272 iEDReqStatus */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT8;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_UINT8;
    datasetFormat[2].size = 1;
    datasetFormat[3].id = IPT_CHAR8;
    datasetFormat[3].size = 26;
    datasetExt.datasetId = iEDReqStatus;
    datasetExt.nLines = 4;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 274 iEDReqVersionInfo */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT8;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_UINT8;
    datasetFormat[2].size = 1;
    datasetFormat[3].id = IPT_CHAR8;
    datasetFormat[3].size = 26;
    datasetExt.datasetId = iEDReqVersionInfo;
    datasetExt.nLines = 4;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 275 oEDRepVersionInfo */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT16;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_UINT8;
    datasetFormat[2].size = 6;
    datasetFormat[3].id = IPT_UINT16;
    datasetFormat[3].size = 1;
    datasetFormat[4].id = IPT_CHAR8;
    datasetFormat[4].size = 0;
    datasetExt.datasetId = oEDRepVersionInfo;
    datasetExt.nLines = 5;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 290 oEDReqDLProgress */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT8;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_UINT8;
    datasetFormat[2].size = 1;
    datasetFormat[3].id = IPT_UINT8;
    datasetFormat[3].size = 1;
    datasetFormat[4].id = IPT_UINT8;
    datasetFormat[4].size = 1;
    datasetFormat[5].id = IPT_UINT32;
    datasetFormat[5].size = 1;
    datasetFormat[6].id = IPT_UINT32;
    datasetFormat[6].size = 1;
    datasetFormat[7].id = IPT_CHAR8;
    datasetFormat[7].size = 115;
    datasetExt.datasetId = oEDReqDLProgress;
    datasetExt.nLines = 8;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 291 iEDRepDLProgress */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT8;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_UINT8;
    datasetFormat[2].size = 1;
    datasetFormat[3].id = IPT_CHAR8;
    datasetFormat[3].size = 26;
    datasetExt.datasetId = iEDRepDLProgress;
    datasetExt.nLines = 4;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 292 iEDReqCleanUp */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT32;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_CHAR8;
    datasetFormat[2].size = 28;
    datasetExt.datasetId = iEDReqCleanUp;
    datasetExt.nLines = 3;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 293 oEDRepCleanUp */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[1].id = IPT_UINT8;
    datasetFormat[1].size = 1;
    datasetFormat[2].id = IPT_UINT8;
    datasetFormat[2].size = 1;
    datasetFormat[3].id = IPT_UINT8;
    datasetFormat[3].size = 26;
    datasetExt.datasetId = oEDRepCleanUp;
    datasetExt.nLines = 4;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    /* datasetId 294 oEDPing */
    datasetFormat[0].id = IPT_UINT32;
    datasetFormat[0].size = 1;
    datasetFormat[3].id = IPT_CHAR8;
    datasetFormat[3].size = 28;
    datasetExt.datasetId = oEDPing;
    datasetExt.nLines = 2;
    datasetExt.disableMarshalling = 0;
    datasetExt.reserved1 = 0;
    datasetExt.reserved2 = 0;
    iptRes = iptConfigAddDatasetExt(&datasetExt, &datasetFormat[0]);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }


    exchg.comId = iMCGReqSendFile; /* 850 */
    exchg.datasetId = 850;
    exchg.comParId = IPT_DEF_COMPAR_MD_ID;
    exchg.pdRecPar.pSourceURI        = NULL;
    exchg.pdRecPar.validityBehaviour = IPT_INVALID_KEEP;
    exchg.pdRecPar.timeoutValue      = 0;
    exchg.pdSendPar.pDestinationURI  = NULL;
    exchg.pdSendPar.cycleTime        = 0;
    exchg.pdSendPar.redundant        = 0;
    exchg.mdRecPar.pSourceURI        = NULL;
    exchg.mdSendPar.pDestinationURI  = NULL;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = oMCGRepSendFile; /* 851 */
    exchg.datasetId = 851;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = iMCGReqSendFile2; /* 852 */
    exchg.datasetId = 852;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = iMCGDLFileTfST; /* 854 */
    exchg.datasetId = 854;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = iEDReqStatus; /* 272 */
    exchg.datasetId = 272;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }
    exchg.comId = oEDRepStatus; /* 273 */
    exchg.datasetId = 271;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = iEDReqVersionInfo; /* 274 */
    exchg.datasetId = 274;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }
    exchg.comId = oEDRepVersionInfo; /* 275 */
    exchg.datasetId = 275;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = iEDReqSendFile2; /* 276 */
    exchg.datasetId = 850;
    exchg.comParId = IPT_DEF_COMPAR_MD_ID;
    exchg.pdRecPar.pSourceURI        = NULL;
    exchg.pdRecPar.validityBehaviour = IPT_INVALID_KEEP;
    exchg.pdRecPar.timeoutValue      = 0;
    exchg.pdSendPar.pDestinationURI  = NULL;
    exchg.pdSendPar.cycleTime        = 0;
    exchg.pdSendPar.redundant        = 0;
    exchg.mdRecPar.pSourceURI        = NULL;
    exchg.mdSendPar.pDestinationURI  = NULL;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = oEDRepSendFile2; /* 277 */
    exchg.datasetId = 851;
    exchg.comParId = IPT_DEF_COMPAR_MD_ID;
    exchg.mdRecPar.pSourceURI        = NULL;
    exchg.mdSendPar.pDestinationURI  = NULL;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = iEDReqStatus2; /* 278 */
    exchg.datasetId = 272;
    exchg.comParId = IPT_DEF_COMPAR_MD_ID;
    exchg.pdRecPar.pSourceURI        = NULL;
    exchg.pdRecPar.validityBehaviour = IPT_INVALID_KEEP;
    exchg.pdRecPar.timeoutValue      = 0;
    exchg.pdSendPar.pDestinationURI  = NULL;
    exchg.pdSendPar.cycleTime        = 0;
    exchg.pdSendPar.redundant        = 0;
    exchg.mdRecPar.pSourceURI        = NULL;
    exchg.mdSendPar.pDestinationURI  = NULL;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = oEDRepStatus2; /* 279 */
    exchg.datasetId = 271;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = oEDReqDLProgress; /* 290 */
    exchg.datasetId = 290;
    exchg.comParId = IPT_DEF_COMPAR_MD_ID;
    exchg.mdRecPar.pSourceURI        = NULL;
    exchg.mdSendPar.pDestinationURI  = NULL;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = iEDRepDLProgress; /* 291 */
    exchg.datasetId = 291;
    exchg.comParId = IPT_DEF_COMPAR_MD_ID;
    exchg.pdRecPar.pSourceURI        = NULL;
    exchg.pdRecPar.validityBehaviour = IPT_INVALID_KEEP;
    exchg.pdRecPar.timeoutValue      = 0;
    exchg.pdSendPar.pDestinationURI  = NULL;
    exchg.pdSendPar.cycleTime        = 0;
    exchg.pdSendPar.redundant        = 0;
    exchg.mdRecPar.pSourceURI        = NULL;
    exchg.mdSendPar.pDestinationURI  = NULL;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = iEDReqCleanUp; /* 292 */
    exchg.datasetId = 292;
    exchg.comParId = IPT_DEF_COMPAR_MD_ID;
    exchg.mdRecPar.pSourceURI        = NULL;
    exchg.mdSendPar.pDestinationURI  = NULL;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }

    exchg.comId = oEDRepCleanUp; /* 293 */
    exchg.datasetId = 293;
    exchg.comParId = IPT_DEF_COMPAR_MD_ID;
    exchg.pdRecPar.pSourceURI        = NULL;
    exchg.pdRecPar.validityBehaviour = IPT_INVALID_KEEP;
    exchg.pdRecPar.timeoutValue      = 0;
    exchg.pdSendPar.pDestinationURI  = NULL;
    exchg.pdSendPar.cycleTime        = 0;
    exchg.pdSendPar.redundant        = 0;
    exchg.mdRecPar.pSourceURI        = NULL;
    exchg.mdSendPar.pDestinationURI  = NULL;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }


    exchg.comId = oEDPing; /* 294 */
    exchg.datasetId = 294;
    exchg.comParId = IPT_DEF_COMPAR_MD_ID;
    exchg.mdRecPar.pSourceURI        = NULL;
    exchg.mdSendPar.pDestinationURI  = NULL;
    iptRes = iptConfigAddExchgPar(&exchg);
    if ((iptRes != IPT_OK) && (iptRes != (int)IPT_TAB_ERR_EXISTS))
    {
        return iptRes;
    }


    /* Check if ED Identification Data telegram is re-defined in ipt_config */
    iptRes = iptConfigGetDatasetId(EDIDENT_COMID, &dataSetId);
    if ((iptRes != IPT_OK) || (dataSetId != EDIDENT_COMID))
    {
        exchg.comId = EDIDENT_COMID;
        exchg.datasetId = 271;
        exchg.comParId = IPT_DEF_COMPAR_PD_ID;
        exchg.pdSendPar.pDestinationURI = pURI;
        exchg.pdSendPar.cycleTime = 1000;
        exchg.pdSendPar.redundant = 0;
        iptRes = iptConfigAddExchgPar(&exchg);
    }
    else
    {
        iptRes = IPT_OK;
    }
    return iptRes;
}


/*******************************************************************************
 *
 * Function name: dleds_uninit
 *
 * Abstract:      This function makes a clean up before termination.
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       pdHandle
 *                requestQueueId
 *                resultQueueId
 */
DLEDS_RESULT dleds_uninit(
    void)
{
    DLEDS_RESULT result = DLEDS_OK;
    int iptResult = IPT_OK;

    DebugError0("DLEDS daemon uninit started");

    PDComAPI_unpublish(&pdHandle);
    MDComAPI_removeListenerQ(requestQueueId);
    if (resultQueueId != (MD_QUEUE) NULL)
    {
        iptResult = MDComAPI_removeQueue(resultQueueId,1);
        if (iptResult != IPT_OK)
        {
            DebugError1("Failed to remove Result queue (0x%x)",
                iptResult);
        }
    }
    if (requestQueueId != (MD_QUEUE) NULL)
    {
        iptResult = MDComAPI_removeQueue(requestQueueId,1);
        if (iptResult != IPT_OK)
        {
            DebugError1("Failed to remove Request queue (0x%x)",
                iptResult);
        }
    }

    DebugError0("DLEDS daemon uninit finished");
    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_sendEchoMessage
 *
 * Abstract:      This function .
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       sessionData
 */
void dleds_sendEchoMessage(void)
{
    int         iptResult;
    char        data[] = "Echo message";
    char        destUri[102];
    char        tempDestUri[102];
    UINT8       topoCounter = 0;
    UINT32      size;
    MSG_INFO    msgInfo;
    char*       pRecBuf;

    /* Convert source IP address to URI string to be used as destination URI */

    iptResult = IPTCom_getUriHostPart(
                sessionData.srcIpAddress,       /* IP address */
                tempDestUri,                    /* Pointer to resulting URI */
                &topoCounter);                  /* Topo counter */
    if (iptResult == IPT_OK)
    {
        strcpy(destUri,"@");
        strcat(destUri,tempDestUri);

        iptResult = MDComAPI_putReqMsg(
            110,                    /* ComId, Echo message */
            (char*)&data,           /* Data buffer */
            sizeof(data),           /* Number of data to be send */
            1,                      /* Number of expected replies.
                                        0=unspecified */
            0,                      /* Time-out value in milliseconds
                                        for receiving replies
                                        0=default value */
            echoQueueId,            /* Queue for communication result */
            NULL,                   /* Pointer to callback function */
            NULL,                   /* Caller reference value */
            0,                      /* Topo counter */
            0,                      /* Dest ID */
            destUri,                /* Destination URI */
            NULL);                  /* No overriding of source URI */

        if (iptResult != IPT_OK)
        {
            /* The sending couldn't be started. */
            /* Error handling */
            DebugError1("MDComAPI_putReqMsg() FAILED with (%d) when sending echo message", iptResult);
        }
        else
        {
            pRecBuf = NULL;  /* Use IPTCom allocated buffer */
            size = 0;

            iptResult = MDComAPI_getMsg(
                    echoQueueId,        /* Queue ID */
                    &msgInfo,           /* Message info */
                    &pRecBuf,           /* Pointer to pointer to data
                                            buffer */
                    &size,              /* Pointer to size. Size shall
                                            be set to own buffer size at
                                            the call. The IPTCom will
                                            return the number of received
                                            bytes */
                    IPT_WAIT_FOREVER);  /* Wait for result */

            if (iptResult == MD_QUEUE_NOT_EMPTY)
            {
                if (msgInfo.msgType != MD_MSGTYPE_RESPONSE )
                {
                    /* Take care of received response data */
                    DebugError3("Unexpected message type received, response expected (%d), received (%d), resultCode (%d)",
                        MD_MSGTYPE_RESPONSE, msgInfo.msgType, msgInfo.resultCode);
                }
            }
            else
            {
                DebugError1("MDComAPI_getMsg() FAILED with (%d) when waiting for echo response message", iptResult);
            }

            if (pRecBuf != NULL)
            {
                iptResult = MDComAPI_freeBuf(pRecBuf);
                if (iptResult != 0)
                {
                    /* error */
                    DebugError1("MDComAPI_freeBuf returns error code (%d)", iptResult);
                }
            }
        }
    }
    else
    {
        DebugError1("IPTCom_getUriHostPart() FAILED with (%d) when sending echo message", iptResult);
    }
}


/*******************************************************************************
 *
 * Function name: dleds_checkDownloadStatus
 *
 * Abstract:      This function checks if it is OK to continue in DOWNLOAD mode.
 *                It is checked that the temporary storage file exists and
 *                could be read. This is important since the session
 *                information in this file is neccessary to be able to continue
 *                the download transfer in download mode.
 *
 * Return value:  DLEDS_OK
 *                DLEDS_ERROR
 *
 * Globals:       sessionData
 *                localRequest
 *                DLEDS_TEMP_STORAGE_FILE_NAME
 */
DLEDS_RESULT dleds_checkDownloadModeStatus(
    void)   /* No arguments */
{
    DLEDS_RESULT result = DLEDS_ERROR;

    /* Check that temp storage file exists */
    if (dleds_storageFileExists() == DLEDS_OK)
    {
        /* Check that file is OK */
        if (dleds_storageFileValid() == DLEDS_OK)
        {
            if (dleds_readStorageFile(&sessionData) == DLEDS_OK)
            {
                if ( sessionData.srcIpAddress == 0x7F000001 )
                {
                    localRequest = TRUE;
                }
                result = DLEDS_OK;
            }
            else
            {
                /*
                * Information from temporary storage file could not be read.
                * Information in this file is neccessary to be able to
                * continue in download mode.
                */
                DebugError1("Could not read information from file with download session information (%s)",
                    DLEDS_TEMP_STORAGE_FILE_NAME);
                result = DLEDS_ERROR;
            }
        }
        else
        {
            /*
             * Temporary storage file is corrupt. Information in this
             * file is neccessary to be able to continue in download mode.
             */
            DebugError1("File with download session information is corrupt (%s)",
                DLEDS_TEMP_STORAGE_FILE_NAME);
            result = DLEDS_ERROR;
        }
    }
    else
    {
        /*
         * Temporary storage file is missing. Information in this
         * file is neccessary to be able to continue in download mode.
         */
        DebugError1("File with download session information is missing (%s)",
            DLEDS_TEMP_STORAGE_FILE_NAME);
        result = DLEDS_ERROR;
    }
    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_checkRunModeStatus
 *
 * Abstract:      This function checks if it is OK to continue in RUN mode. This
 *                is always the case. Removes any temporary storage file that
 *                has not been deleted since previous transfer session. This
 *                could be the case if previous session was disruppted due to
 *                any error situation.
 *
 * Return value:  DLEDS_OK
 *
 * Globals:       sessionData
 *                localRequestInProgress
 *                localRequestTransactionId
 *                localRequestFileName
 *                localRequestFileCrc
 *                localRequestResultCode
 *                DLEDS_TEMP_STORAGE_FILE_NAME
 *                DLEDS_INSTALLATION_FILE
 *                dledsTempDirectory
 */
DLEDS_RESULT dleds_checkRunModeStatus(
    void)   /* No arguments */
{
    FILE*   fp;
    char    ftpDir[256];
    int     rDirStatus;
    DLEDS_RESULT reportResult;
    UINT8 abortRequest;


    /* Check if there is any temporary storage file */
    localRequestInProgress = FALSE;
    if (dleds_storageFileExists() == DLEDS_OK)
    {
        /* Check if local request is in progress */
        if (dleds_readStorageFile(&sessionData) == DLEDS_OK)
        {
            DebugError2("CHECK_RUN_MODE_STATUS: localTransfer= (%d), transferInProgress= (%d)",
                    sessionData.localTransfer, sessionData.transferInProgress);
            if ( (sessionData.localTransfer == TRUE) && (sessionData.transferInProgress == FALSE) )
            {
                localRequestInProgress = TRUE;
                localRequestTransactionId = sessionData.requestInfo.transactionId;
                strcpy(localRequestFileName, sessionData.requestInfo.fileName);
                localRequestFileCrc = sessionData.requestInfo.fileCRC;
                localRequestResultCode = sessionData.appResultCode;
            }
            else
            {
                if (sessionData.completedOperation == 1)
                {
                    /* Send final DL Progress Request for Download Operation */
                    reportResult = dleds_reportProgress(PROGRESS_ED_IN_RUN_MODE, PROGRESS_100, PROGRESS_NO_RESET_IN_PROGRESS,
                                                        PROGRESS_USE_DEFAULT_TIMEOUT, sessionData.appResultCode, NULL, &abortRequest);
                }

                /*
                * No transfer in progress. This file should not exist in RUN MODE in this case.
                */
                DebugError1("No transfer in progress. Delete temporary storage file (%s)",
                    DLEDS_TEMP_STORAGE_FILE_NAME);

                /* Delete temporary storage file */
                if (remove(DLEDS_TEMP_STORAGE_FILE_NAME) != 0)
                {
                    DebugError1("Failed to delete temporary storage file (%s)",
                    DLEDS_TEMP_STORAGE_FILE_NAME);
                }

                /* Clear sessionData */
                DebugError0("Clearing sessionData");
                memset(&sessionData, 0, sizeof(sessionData));

                /* Delete temporary installation file if it exists */
                fp = fopen(DLEDS_INSTALLATION_FILE, "r" );
                if (fp != NULL)
                {
                    if (remove(DLEDS_INSTALLATION_FILE) != 0)
                    {
                        DebugError1("Could not delete temporary file (%s)", DLEDS_INSTALLATION_FILE);
                    }
                }

                /* Delete any FTP directory if existing */
                /* Find out where the temporary FTP directory was created on this device */
                /* This directory is normally created and deleted in Download Mode */
                if ( dleds_setDledsTempPath() == DLEDS_OK )
                {
                    strcpy(ftpDir, dledsTempDirectory);
                    strcat(ftpDir, "/ftp");
                    rDirStatus = removeDirectory(ftpDir);
                    if (rDirStatus != 0)
                    {
                        DebugError1("Could not delete FTP directory (%s)", ftpDir);
                    }
                }
                else
                {
                    /* Failed to find the temporary ftp directory */
                    DebugError0("Could not find temporary FTP directory to be deleted");
                }
            }
        }
        else
        {
            /*
            * Temporary storage file can't be read. Delete the file.
            */
            DebugError1("File with download session information can't be read (%s)",
                DLEDS_TEMP_STORAGE_FILE_NAME);
        }
    }
    /* To avoid compiler warning */
    (void)reportResult;

    return DLEDS_OK;
}

/*******************************************************************************
 *
 * Function name: dleds_storageFileExists
 *
 * Abstract:      This function checks if the temporary storage file with
 *                session information exists.
 *
 * Return value:  DLEDS_OK      - File exists
 *                DLEDS_ERROR   - File is missing
 *
 * Globals:       DLEDS_TEMP_STORAGE_FILE_NAME
 */
DLEDS_RESULT dleds_storageFileExists(
    void)   /* No arguments */
{
    FILE*           fp;
    DLEDS_RESULT    result = DLEDS_OK;

    fp = fopen(DLEDS_TEMP_STORAGE_FILE_NAME, "r" );
    if (fp == NULL)
    {
        result = DLEDS_ERROR;
    }
    else
    {
        fclose(fp);
    }
    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_storageFileValid
 *
 * Abstract:      This function checks if the temporary storage file is OK.
 *
 * Return value:  DLEDS_OK      - File is OK
 *                DLEDS_ERROR   - File is corrupt
 *
 * Globals:       -
 */
static DLEDS_RESULT dleds_storageFileValid(
    void)   /* No arguments */
{
    FILE*           fp;
    DLEDS_RESULT    result = DLEDS_OK;

    fp = fopen(DLEDS_TEMP_STORAGE_FILE_NAME, "r" );
    if (fp == NULL)
    {
        DebugError1("Failed to open file (%s)",
            DLEDS_TEMP_STORAGE_FILE_NAME);
        result = DLEDS_ERROR;
    }
    else
    {
        /* Insert some sort of checksum control for the file content */
        result = DLEDS_OK;
        fclose(fp);
    }
    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_readStorageFile
 *
 * Abstract:      This function reads session information from the temporary
 *                storage file.
 *
 * Return value:  DLEDS_OK      - Information from file has been read
 *                DLEDS_ERROR   - No information read from file
 *
 * Globals:       DLEDS_TEMP_STORAGE_FILE_NAME
 */
DLEDS_RESULT dleds_readStorageFile(
    TYPE_DLEDS_SESSION_DATA* pSessionData)  /* OUT: Data read from file */
{
    FILE*           fp;
    DLEDS_RESULT    result = DLEDS_OK;

    fp = fopen(DLEDS_TEMP_STORAGE_FILE_NAME, "r" );
    if (fp == NULL)
    {
        DebugError1("Failed to open file (%s)",
            DLEDS_TEMP_STORAGE_FILE_NAME);
        result = DLEDS_ERROR;
    }
    else
    {
        fscanf(fp,"%u", &(pSessionData->requestReceiveTime));
        fscanf(fp,"%u", &(pSessionData->sessionId));
        fscanf(fp,"%u", &(pSessionData->srcIpAddress));
        fscanf(fp,"%s", &(pSessionData->srcUri[0]));
        fscanf(fp,"%u", &(pSessionData->transferInProgress));
        fscanf(fp,"%u", &(pSessionData->localTransfer));
        fscanf(fp,"%d", &(pSessionData->appResultCode));
        fscanf(fp,"%u", &(pSessionData->requestInfo.transactionId));
        fscanf(fp,"%s", &(pSessionData->requestInfo.fileName[0]));
        fscanf(fp,"%s", &(pSessionData->requestInfo.fileServerHostName[0]));
        fscanf(fp,"%s", &(pSessionData->requestInfo.fileServerPath[0]));
        fscanf(fp,"%u", &(pSessionData->requestInfo.fileSize));
        fscanf(fp,"%u", &(pSessionData->requestInfo.fileCRC));
        fscanf(fp,"%u", &(pSessionData->requestInfo.fileVersion));
        fscanf(fp,"%u", &(pSessionData->downloadResetReason));
        fscanf(fp,"%u", &(pSessionData->protocolVersion));
        fscanf(fp,"%u", &(pSessionData->packageVersion));
        fscanf(fp,"%u", &(pSessionData->completedOperation));
        fclose(fp);
    }
    return result;

}


/*******************************************************************************
 *
 * Function name: dleds_writeStorageFile
 *
 * Abstract:      This function writes session information to the temporary
 *                storage file.
 *
 * Return value:  DLEDS_OK      - Information from file has been written
 *                DLEDS_ERROR   - No information written to file
 *
 * Globals:       -
 */
DLEDS_RESULT dleds_writeStorageFile(
    TYPE_DLEDS_SESSION_DATA* pSessionData)  /* IN: Data to be written to file */
{
    FILE*           fp;
    DLEDS_RESULT    result = DLEDS_OK;

    fp = fopen(DLEDS_TEMP_STORAGE_FILE_NAME, "w" );
    if (fp == NULL)
    {
        DebugError1("Failed to open file (%s)",
            DLEDS_TEMP_STORAGE_FILE_NAME);

        result = DLEDS_ERROR;
    }
    else
    {
        fprintf(fp,"%u\n", pSessionData->requestReceiveTime);
        fprintf(fp,"%u\n", pSessionData->sessionId);
        fprintf(fp,"%u\n", pSessionData->srcIpAddress);
        fprintf(fp,"%s\n", pSessionData->srcUri);
        fprintf(fp,"%u\n", pSessionData->transferInProgress);
        fprintf(fp,"%u\n", pSessionData->localTransfer);
        fprintf(fp,"%d\n", pSessionData->appResultCode);
        fprintf(fp,"%u\n", pSessionData->requestInfo.transactionId);
        fprintf(fp,"%s\n", pSessionData->requestInfo.fileName);
        fprintf(fp,"%s\n", pSessionData->requestInfo.fileServerHostName);
        fprintf(fp,"%s\n", pSessionData->requestInfo.fileServerPath);
        fprintf(fp,"%u\n", pSessionData->requestInfo.fileSize);
        fprintf(fp,"%u\n", pSessionData->requestInfo.fileCRC);
        fprintf(fp,"%u\n", pSessionData->requestInfo.fileVersion);
        fprintf(fp,"%u\n", pSessionData->downloadResetReason);
        fprintf(fp,"%u\n", pSessionData->protocolVersion);
        fprintf(fp,"%u\n", pSessionData->packageVersion);
        fprintf(fp,"%u\n", pSessionData->completedOperation);
        fclose(fp);
    }
    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_checkCRC
 *
 * Abstract:      This function checks the CRC of the file.
 *
 *
 * Return value:  DLEDS_OK      - Message information structure is OK
 *                DLEDS_ERROR   - Message information structure is corrupt
 *
 * Globals:       -
 */
DLEDS_RESULT dleds_checkCRC(
    char* pDestFile,
    UINT32 fileCRC,
    UINT32 startValue)
{
    DLEDS_RESULT result;
    unsigned long ulCRC = startValue;

    FILE *fSource = 0;
    unsigned char sBuf[1024];
    int iBytesRead = 0;

    fSource = fopen(pDestFile, "rb");
    if( fSource == 0)
    {
        DebugError1("Could not open file %s", pDestFile);
        return DLEDS_ERROR;
    }

    do
    {
        iBytesRead = fread(sBuf, sizeof(char), sizeof(sBuf), fSource);
        ulCRC = dleds_crc32(ulCRC, sBuf, iBytesRead);
    }while(iBytesRead == sizeof(sBuf));

    fclose(fSource);

    if(ulCRC == fileCRC)
    {
        result = DLEDS_OK;
    }
    else
    {
        DebugError2("CRC error on ED Package expected= 0x%x, calculated= 0x%x",
            fileCRC, ulCRC);
        result = DLEDS_ERROR;
    }

    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_getIpString
 *
 * Abstract:      This function converts the IP address in a UINT32 format to a
 *                character string of format <>.<>.<>.<>
 *
 * Return value:  DLEDS_OK      - Message information structure is OK
 *                DLEDS_ERROR   - Message information structure is corrupt
 *
 * Globals:       sessionData
 */
static DLEDS_RESULT dleds_getIpString(char* pIpString, UINT32 ipAddress)
{
    DLEDS_RESULT result = DLEDS_OK;
    UINT8 part1,part2,part3,part4;

    part1 = (UINT8)(0xFF & (ipAddress >> 24));
    part2 = (UINT8)(0xFF & (ipAddress >> 16));
    part3 = (UINT8)(0xFF & (ipAddress >> 8));
    part4 = (UINT8)(0xFF & (ipAddress));
    sprintf (pIpString, "%u.%u.%u.%u", part1, part2, part3, part4);

    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_ftpCopyFile
 *
 * Abstract:      This function retrieves the file from the FTP server and
 *                checks the CRC.
 *
 * Return value:  DLEDS_OK          - The file is retrieved successfully
 *                DLEDS_ERROR       - Failed to retrieve file
 *                DLEDS_CRC_ERROR   - Checksum error
 *
 * Globals:       dledsTempDirectory
 *                sessionData
 */
static DLEDS_RESULT dleds_ftpCopyFile(
    void)
{
    DLEDS_RESULT        result = DLEDS_OK;
    DLEDS_RESULT        reportResult;
    char                destFile[512] = "";
    T_TDC_RESULT        tdcResult;
    UINT8               topoCount;
    T_IPT_IP_ADDR       ipAddr;
    char                ipAddrStr[16] = "";
    UINT8               abortRequest;

    topoCount = 0;

    /* Set directory where to download ED package with FTP */
    strcpy(destFile, dledsTempDirectory);
    strcat(destFile, "/ftp");

    tdcResult = tdcGetAddrByName(sessionData.requestInfo.fileServerHostName, &ipAddr, &topoCount);
    DebugError2("Fileserver Hostname= %s, IP address= 0x%x",
                sessionData.requestInfo.fileServerHostName, ipAddr);
    if (tdcResult != 0)
    {
        DebugError2("Error (0x%x) from tdcGetAddrByName(), input URI = %s",
            tdcResult, sessionData.requestInfo.fileServerHostName);
        return DLEDS_ERROR;
    }
    else
    {
        /* convert IP address in UINT32 to string */
        if (dleds_getIpString(ipAddrStr, ipAddr) != DLEDS_OK)
        {
            DebugError1("Error from function dleds_getIpString(0x%x)", ipAddr);
            return DLEDS_ERROR;
        }
        DebugError2("IP address= 0x%x, IP string= %s", ipAddr, ipAddrStr);
    }

    DebugError1("***PLATFORM: Local FTP directory set to (%s)", destFile);
    DebugError2("***PLATFORM: IP address of FTP server (%s) (0x%x)", ipAddrStr, ipAddr);
    DebugError1("***PLATFORM: File path on server (%s)", sessionData.requestInfo.fileServerPath);
    DebugError1("***PLATFORM: File name on server (%s)", sessionData.requestInfo.fileName);
    /* Send Progress Request */
    if (sessionData.protocolVersion == DLEDS_PROTOCOL_VERSION_2)
    {
        reportResult = dleds_reportProgress(PROGRESS_FTP_PROGRESS,PROGRESS_20, PROGRESS_NO_RESET_IN_PROGRESS,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);
        if (abortRequest == 1)
        {
            DebugError0("Download Operation cancelled by User");
            appResultCode = -200;
            return DLEDS_ERROR;
        }
    }
    /* Call function that contains Platform Specific code */
    result = dledsPlatformFtp(destFile, ipAddrStr, ipAddr,
                              sessionData.requestInfo.fileServerPath,
                              sessionData.requestInfo.fileName);

    if (result == DLEDS_OK)
    {
        appResultCode = DL_OK;
    }
    else
    {
        appResultCode = DL_FTP_ERROR;
    }

    /* Send Progress Request */
    reportResult = dleds_reportProgress(PROGRESS_FTP_COMPLETED,PROGRESS_30, PROGRESS_NO_RESET_IN_PROGRESS,
                                        PROGRESS_USE_DEFAULT_TIMEOUT, appResultCode, NULL, &abortRequest);

    if (result == DLEDS_OK)
    {
        if (abortRequest == 1)
        {
            DebugError0("Download Operation cancelled by User");
            appResultCode = -200;
            return DLEDS_ERROR;
        }
        DebugError1("***PLATFORM: Expected CRC-32 for retrieved file (0x%x)", sessionData.requestInfo.fileCRC);
        result = dleds_checkCRC(destFile, sessionData.requestInfo.fileCRC, 0x0);
        if (result != DLEDS_OK)
        {
            return DLEDS_CRC_ERROR;
        }
    }

    DebugError1("***PLATFORM: FTP result is (%d)", result);

    /* To avoid compiler warning */
    (void)reportResult;

    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_setDledsTempPath
 *
 * Abstract:      Creates path to temporary DLEDS directory where ED package will
 *                be stored and unpacked.
 *
 * Return value:  DLEDS_OK      - if OK
 *                DLEDS_ERROR   - otherwise
 *
 * Globals:       dledsTempDirectory
 */
static DLEDS_RESULT dleds_setDledsTempPath(void)
{
    DLEDS_RESULT    result = DLEDS_ERROR;

    /* Call function that contains Platform Specific code */
    result = dledsPlatformSetDledsTempPath(dledsTempDirectory);
    DebugError2("***PLATFORM: Set DLEDS temp path (%s), result (%d)", dledsTempDirectory, result);

    if ( (result == DLEDS_OK) && (strlen(dledsTempDirectory) != 0) )
    {
        /* DLEDS directory for temporary files selected */
        strcat(dledsTempDirectory, "dleds");
        if ( createSubDirectory(dledsTempDirectory) != DLEDS_OK )
        {
            DebugError1("Could not create  = %s", dledsTempDirectory);
            result = DLEDS_ERROR;
        }
    }

    DebugError1("DLEDS directory for temporary files (%s)", dledsTempDirectory);
    return result;
}

/*******************************************************************************
 *
 * Function name: dleds_diskSpaceAvailable
 *
 * Abstract:      This function checks if there is enough free space in the
 *                file system to store and unpack the file. There has to be
 *                at least 2 times the file size available as free disk space.
 *
 * Return value:  DLEDS_OK      - Enough disk space available
 *                DLEDS_ERROR   - Not enough disk space available
 *
 * Globals:       -
 */
DLEDS_RESULT dleds_diskSpaceAvailable(
    char*  filePath,                    /* IN: path to file system */
    UINT32 fileSize)                    /* IN: size in kBytes */
{
    UINT32 freeDiskSpace;

    /* Call function that contains Platform Specific code */
    freeDiskSpace = dledsPlatformCalculateFreeDiskSpace(filePath);
    DebugError2("***PLATFORM: Free disk space (%d)kByte in (%s)", freeDiskSpace, filePath);

    /* There has to be free disk space to the amount of 2 times the TAR file size */
    if ( (fileSize * 2) > freeDiskSpace )
    {
        DebugError3("Not enough amount of free disk space on (%s) (Requested= %d kByte, Free= %d kByte)",
                    filePath, fileSize * 2, freeDiskSpace);
        return DLEDS_ERROR;
    }
    else
    {
        DebugError3("Amount of free disk space on (%s) ((Requested= %d kByte, Free= %d kByte)",
                    filePath, fileSize * 2, freeDiskSpace);
    }

    return DLEDS_OK;
}

/*******************************************************************************
 *
 * Function name: isMarkChar
 *
 * Abstract:      This function
 *
 * Return value:
 *
 *
 * Globals:       -
 */
static int isMarkChar(
   int c)
{
    switch(c)
    {
        case '-':
            break;
        case '_':
            break;
        case '!':
            break;
        case '~':
            break;
        case '*':
            break;
        case 0x27:  /* ' */
            break;
        case '(':
            break;
        case ')':
            break;
        default:
            return 0;
    }
    return c;
}

/*******************************************************************************
 *
 * Function name: dleds_validFqdn
 *
 * Abstract:      This function checks if the input parameter is a valid
 *                hostname FQDN.
 *
 * Return value:  DLEDS_OK      -
 *                DLEDS_ERROR   -
 *
 * Globals:       -
 */
static DLEDS_HOST_URI_FQDN dleds_validFqdn(char* pUri)
{
    DLEDS_HOST_URI_FQDN fqdn = Init;

    /* validate input */
    if(pUri == NULL)
    {
        DebugError0("ERROR URI pointer is null");
        fqdn = Error;
    }
    else if(*pUri == 0)
    {
        DebugError0("ERROR no uri specified");
        fqdn = Error;
    }
    else if(strlen(pUri) > 64)
    {
        DebugError0("ERROR to long uri specified");
        fqdn = Error;
    }
    else
    {
        const char* pTmp = pUri;

        while(*pTmp != 0)
        {
            switch(fqdn)
            {
                case Init:
                    if(*pTmp == '@')
                        fqdn = AtDelim;
                    else if(isalpha(*pTmp) == 0)
                        fqdn = Error;
                    else
                        fqdn = DevId;
                    break;
                case AtDelim:
                    if(isalnum(*pTmp) == 0)
                        fqdn = Error;
                    else
                        fqdn = DevId;
                    break;
                case DevId:
                    if(*pTmp == '.')
                        fqdn = DevDelim;
                    else if(isalnum(*pTmp) != 0 || isMarkChar(*pTmp) != 0)
                        ;   /* do nada */
                    else
                        fqdn = Error;
                    break;
                case DevDelim:
                    if(isalnum(*pTmp) == 0)
                        fqdn = Error;
                    else
                        fqdn = CarId;
                    break;
                case CarId:
                    if(*pTmp == '.')
                        fqdn = CarDelim;
                    else if(isalnum(*pTmp) != 0 || isMarkChar(*pTmp) != 0)
                        ;   /* do nada */
                    else
                        fqdn = Error;
                    break;
                case CarDelim:
                    if(isalnum(*pTmp) == 0)
                        fqdn = Error;
                    else
                        fqdn = CstId;
                    break;
                case CstId:
                    if(*pTmp == '.')
                        fqdn = CstDelim;
                    else if(isalnum(*pTmp) != 0 || isMarkChar(*pTmp) != 0)
                        ;   /* do nada */
                    else
                        fqdn = Error;
                    break;
                case CstDelim:
                    if(isalnum(*pTmp) == 0)
                        fqdn = Error;
                    else
                        fqdn = TrainId;
                    break;
                case TrainId:
                    if(*pTmp == '.')
                        fqdn = Error;
                    else if(isalnum(*pTmp) != 0 || isMarkChar(*pTmp) != 0)
                        ;   /* do nada */
                    else
                        fqdn = Error;
                    break;
                case Error:
                    DebugError0("ERROR incorrect FQDN detected in data URI");
                    return Error;
                default:
                    DebugError0("ERROR unknown FQDN when checking URI");
                    return Error;
            }
            ++pTmp;
        }
        if((fqdn == CstId) || (fqdn == TrainId))
        {
            /* the uri ended with consist id */
            fqdn = Valid;
        }
    }
    return fqdn;
}


/*******************************************************************************
 *
 * Function name: dleds_get_hwinfo
 *
 * Abstract:      This function returns HW info
 *
 * Return value:  DLEDS_OK      - HW info found
 *                DLEDS_ERROR   - No HW info found
 *
 * Globals:       -
 */
static DLEDS_RESULT dleds_get_hwinfo(
    char* unit_type,            /* OUT: Pointer to unit type string (16 characters, including '\0') */
    char* serial_number,        /* OUT: Pointer to serial number string (16 characters, including '\0') */
    char* delivery_revision)    /* OUT: Pointer to delivered revision (16 characters, including '\0') */
{
    DLEDS_RESULT    result = DLEDS_ERROR;

    if ((unit_type == NULL) || (serial_number == NULL) || (delivery_revision == NULL))
    {
        return result;
    }

    /******************************************************************************
    Retrieve HW info for unit type, serial number and delivered revision
    ******************************************************************************/
    result = dledsPlatformGetHwInfo(unit_type, serial_number, delivery_revision);
    DebugError0("***PLATFORM: Find HW info for PD Version Info message");

    return result;
}


/*******************************************************************************
 *
 * Function name: dleds_versionRetrieve
 *
 * Abstract:      This function retrieves information to be put in the ED
 *                 Identification Data sent as a Process Data message.
 *
 * Return value:  DLEDS_OK      -
 *                DLEDS_ERROR   -
 *
 * Globals:       -
 */
static void dleds_versionRetrieve(
    EDIdent* pPdBuffer)
{
    int i;
    int ohalResult;
    int iptResult;
    int     IEdMode;
    int     IConfigStatus;
    UINT32  ipAddress;
    UINT8   version;
    UINT8   release;
    UINT8   update;
    UINT8   evolution;
    char    ipString[16];
    char    uri[128];
    UINT8   topoCounter= 0;
    static UINT8        first = TRUE;
    static UINT32       btDluVersion = 0x00FFFFFF;
    static UINT32       cuDluVersion = 0x00FFFFFF;
    static UINT32       osDluVersion = 0x00FFFFFF;
    static UINT32       blcfgUluVersion = 0x00FFFFFF;
    static UINT32       ubootUluVersion = 0x00FFFFFF;
    static char         deviceType[16];
    static char         serialNumber[16];
    static char         hwRevision[16];

    pPdBuffer->appVersion = EDRepVersionInfo_appVersion;

    /* This function is called with a cycle of 1 second */
    if (first == TRUE)
    {
        /* Read version information that could be retrieved once because they are static */

        /* Get SW versions used for ED Ident message */
        /* Call function that contains Platform Specific code */
        dledsPlatformGetEdIdentSwVersions(&btDluVersion, &cuDluVersion, &osDluVersion,
            &blcfgUluVersion, &ubootUluVersion);
        DebugError4("***PLATFORM: SW versions BT(0x%x), CU(0x%x), BLFCG(0x%x), UBOOT(0x%x)",
            btDluVersion, cuDluVersion, blcfgUluVersion, ubootUluVersion);

        memset(deviceType, 0, sizeof(deviceType));
        memset(serialNumber, 0, sizeof(serialNumber));
        memset(hwRevision, 0, sizeof(hwRevision));
        (void) dleds_get_hwinfo(deviceType, serialNumber, hwRevision);

        first = FALSE;
    }

    /* Write BT/SU SW version to PD message */
    pPdBuffer->BTSwNumber.ISysBTSwVersion   = (UINT8)(0xFF & (btDluVersion >> 24));
    pPdBuffer->BTSwNumber.ISysBTSwRelease   = (UINT8)(0xFF & (btDluVersion >> 16));
    pPdBuffer->BTSwNumber.ISysBTSwUpdate    = (UINT8)(0xFF & (btDluVersion >> 8));
    pPdBuffer->BTSwNumber.ISysBTSwEvolution = (UINT8)(0xFF & (btDluVersion));
    memset(pPdBuffer->BTSwNumber.Reserved, 0, sizeof(pPdBuffer->BTSwNumber.Reserved));

    /* Write CU SW version to PD message */
    pPdBuffer->CUSwNumber.ISysCUSwVersion   = (UINT8)(0xFF & (cuDluVersion >> 24));
    pPdBuffer->CUSwNumber.ISysCUSwRelease   = (UINT8)(0xFF & (cuDluVersion >> 16));
    pPdBuffer->CUSwNumber.ISysCUSwUpdate    = (UINT8)(0xFF & (cuDluVersion >> 8));
    pPdBuffer->CUSwNumber.ISysCUSwEvolution = (UINT8)(0xFF & (cuDluVersion));
    memset(pPdBuffer->CUSwNumber.Reserved, 0, sizeof(pPdBuffer->CUSwNumber.Reserved));

    /* Write OS Base SW version to PD message */
    pPdBuffer->OSBaseSwNumber.ISysOSBaseSwVersion   =  (UINT8)(0xFF & (osDluVersion >> 24));
    pPdBuffer->OSBaseSwNumber.ISysOSBaseSwRelease   =  (UINT8)(0xFF & (osDluVersion >> 16));
    pPdBuffer->OSBaseSwNumber.ISysOSBaseSwUpdate    =  (UINT8)(0xFF & (osDluVersion >> 8));
    pPdBuffer->OSBaseSwNumber.ISysOSBaseSwEvolution =  (UINT8)(0xFF & (osDluVersion));
    memset(pPdBuffer->OSBaseSwNumber.Reserved, 0, sizeof(pPdBuffer->OSBaseSwNumber.Reserved));

    /* Write Boot loader configuration SW version to PD message */
    pPdBuffer->BLConfigSwNumber.ISysBLConfigSwVersion   = (UINT8)(0xFF & (blcfgUluVersion >> 24));
    pPdBuffer->BLConfigSwNumber.ISysBLConfigSwRelease   = (UINT8)(0xFF & (blcfgUluVersion >> 16));
    pPdBuffer->BLConfigSwNumber.ISysBLConfigSwUpdate    = (UINT8)(0xFF & (blcfgUluVersion >> 8));
    pPdBuffer->BLConfigSwNumber.ISysBLConfigSwEvolution = (UINT8)(0xFF & (blcfgUluVersion));
    memset(pPdBuffer->BLConfigSwNumber.Reserved, 0, sizeof(pPdBuffer->BLConfigSwNumber.Reserved));

    /* Write Boot loader (U-boot) SW version to PD message */
    pPdBuffer->BLSwNumber.ISysBLSwVersion   = (UINT8)(0xFF & (ubootUluVersion >> 24));
    pPdBuffer->BLSwNumber.ISysBLSwRelease   = (UINT8)(0xFF & (ubootUluVersion >> 16));
    pPdBuffer->BLSwNumber.ISysBLSwUpdate    = (UINT8)(0xFF & (ubootUluVersion >> 8));
    pPdBuffer->BLSwNumber.ISysBLSwEvolution = (UINT8)(0xFF & (ubootUluVersion));
    memset(pPdBuffer->BLSwNumber.Reserved,0,sizeof(pPdBuffer->BLSwNumber.Reserved));

    /* Write HW info to PD message */
    strcpy(pPdBuffer->HWInfo.IHwRevision,   hwRevision);
    strcpy(pPdBuffer->HWInfo.ISerialNumber, serialNumber);
    strcpy(pPdBuffer->HWInfo.IDeviceType,   deviceType);
    strcpy(pPdBuffer->HWInfo.ISupplierName, "");
    strcpy(pPdBuffer->HWInfo.ISupplierData, "");
    memset(pPdBuffer->HWInfo.Reserved, 0, sizeof(pPdBuffer->HWInfo.Reserved));;


    /* Write ED Mode to PD message */
    ohalResult = dleds_getAppMode(&IEdMode, &IConfigStatus);
    if (ohalResult == DLEDS_OK)
    {
        pPdBuffer->EndDeviceMode.IEdMode = IEdMode;
        pPdBuffer->EndDeviceMode.IConfigStatus = IConfigStatus;
    }
    else
    {
        pPdBuffer->EndDeviceMode.IEdMode = 255;
        pPdBuffer->EndDeviceMode.IConfigStatus = 255;
    }
    if (dleds_getDlAllowed() == TRUE)
    {
        pPdBuffer->EndDeviceMode.DownloadAllowed = 1;
    }
    else
    {
        pPdBuffer->EndDeviceMode.DownloadAllowed = 0;
    }
    if (dleds_get_operation_mode() == DLEDS_OSDOWNLOAD_MODE)
    {
        /* download in progress */
        pPdBuffer->EndDeviceMode.DownloadAllowed = 2;
    }
    memset(pPdBuffer->EndDeviceMode.Reserved, 0, sizeof(pPdBuffer->EndDeviceMode.Reserved));



    /* Write IP config to PD message */
    /* Convert own IP address to IP string */
    ipAddress = IPTCom_getOwnIpAddr();
    version     = (UINT8)(0xFF & (ipAddress >> 24));
    release     = (UINT8)(0xFF & (ipAddress >> 16));
    update      = (UINT8)(0xFF & (ipAddress >> 8));
    evolution   = (UINT8)(0xFF & (ipAddress));
    sprintf (ipString, "%u.%u.%u.%u", version, release, update, evolution);
    /* Get own URI string */
    for (i=0; i<128; i++)
    {
        uri[i] = 0x00;
    }
    iptResult = IPTCom_getUriHostPart(
                    ipAddress,      /* IP address */
                    uri,            /* Pointer to resulting URI */
                    &topoCounter);  /* Topo counter */
    if (iptResult != IPT_OK)
    {
        strcpy(uri,"");
    }

    for (i=0; i<16; i++)
    {
        pPdBuffer->IPConfig.IDevIPAddress[i] = 0;
    }
    strcpy(pPdBuffer->IPConfig.IDevIPAddress, ipString);

    for (i=0; i<128; i++)
    {
        pPdBuffer->IPConfig.IDevFQDN[i] = 0;
    }
    strcpy(pPdBuffer->IPConfig.IDevFQDN, uri);
    memset(pPdBuffer->IPConfig.Reserved, 0, sizeof(pPdBuffer->IPConfig.Reserved));
}

void dleds_resetToRunMode(void)
{
    int         result;
    FILE*       fp;
    UINT32      delayValue = 0;

    /* Delete temporary storage file that has been used in
     * download mode to read session data. This data was
     * previously stored in run mode when download request
     * was received.
     *
     * Do not delete this file if there is a local transfer
     * request in progress.
     */
    if ( localRequest == FALSE )
    {
        if (dleds_storageFileExists() == DLEDS_OK)
        {
            /*
             * Delete temporary storage file,
             * should only exist in DOWNLOAD operation mode
             * when there is a download transfer in progress
             */
             /*
            if (remove(DLEDS_TEMP_STORAGE_FILE_NAME) != 0)
            {
                DebugError1("Failed to delete temporary storage file (%s)",
                    DLEDS_TEMP_STORAGE_FILE_NAME);
            }
            */
        }
    }

    /* Temporary delay for test purpose */
    /* read delay value in seconds from file dledsDelay.txt */
    fp = fopen(DLEDS_DELAY_FILE_NAME, "r" );
    if (fp == NULL)
    {
        DebugError1("Failed to open DLEDS delay file (%s)",
            DLEDS_DELAY_FILE_NAME);
    }
    else
    {
        fscanf(fp,"%u", &delayValue);
        fclose(fp);
    }

    /* This delay is added due to problem when making a reset to RUN mode to short after */
    /* having removed temporary directories on NRTOS2 devices. */
    /* This delay could be removed when problem has been fixed in NRTOS2 */
    if (delayValue < 6)
    {
        delayValue = 6;       /* Delay set to 3 seconds */
    }

    DebugError1("DLEDS delay before reset, Delay= %d seconds", delayValue);
    if (delayValue > 0)
    {
        IPTVosTaskDelay(delayValue * 1000);
    }


    result = dleds_forced_reboot(DLEDS_OSRUN_MODE);
    if (result == DLEDS_ERROR)
    {
        DebugError0("Failed to reset to Run mode in dleds_resetToRunMode()");
    }
}


void dleds_resetToDownloadMode(void)
{
    int result;
    UINT32      delayValue = 6;

    DebugError1("DLEDS delay before reset, Delay= %d seconds", delayValue);
    IPTVosTaskDelay(delayValue * 1000);


    result = dleds_forced_reboot(DLEDS_OSDOWNLOAD_MODE);
    if (result == DLEDS_ERROR)
    {
        DebugError1("Failed to reset to Download mode (result= %ld)", result);
    }
}


