/************************************************************************/
/*  (C) COPYRIGHT 2008 by Bombardier Transportation                     */
/*                                                                      */
/*  Bombardier Transportation Switzerland Dep. PPC/EUT                  */
/************************************************************************/
/*                                                                      */
/*  PROJECT:      Remote download                                       */
/*                                                                      */
/*  MODULE:       dleds                                                 */
/*                                                                      */
/*  ABSTRACT:     This is the main header file for download ED service  */
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
#ifndef DLEDS_H
#define DLEDS_H

#include "dledsPlatformDefines.h"


/* User part of URI for Download End Device Service */
#define DLEDS_SERVICE_NAME              "DLEDService"


/* DLEDS_RESULT codes */
#define DLEDS_OK                        0
#define DLEDS_NO_OF_RETRIES_EXCEEDED    1
#define DLEDS_CRC_ERROR                 2
#define DLEDS_ED_ERROR                  3
#define DLEDS_SCI_ERROR                 4
#define DLEDS_INSTALLATION_ERROR        5
#define DLEDS_EDSTATUS                  6
#define DLEDS_CLEAN_UP_RESET            7
#define DLEDS_STATE_CHANGE              8
#define DLEDS_ERROR                     -1

/* Symbolic constants for reboot reason and operation mode encoding */
#define DLEDS_UNKNOWN_MODE              0
#define DLEDS_OSIDLE_MODE               1
#define DLEDS_OSRUN_MODE                2
#define DLEDS_OSDOWNLOAD_MODE           3
#define DLEDS_OSRESCUE_MODE             4

/* Symbolic constants for download reset reason in session data */
#define DLEDS_NORMAL_RESET              0
#define DLEDS_DOWNLOAD                  1
#define DLEDS_CLEAN_UP                  2
#define DLEDS_DL_IDLE_MODE              3
#define DLEDS_DL_HMI_ERROR              4

/* Symbolic constants for protocol version in session data */
#define DLEDS_PROTOCOL_VERSION_1         1
#define DLEDS_PROTOCOL_VERSION_2         2

/* Symbolic constants for package version in session data */
#define DLEDS_PACKAGE_VERSION_1          1
#define DLEDS_PACKAGE_VERSION_2          2

/* Symbolic constants for actual state of the download allowed status */
#define DLEDS_DL_NOT_ALLOWED            0
#define DLEDS_DL_ALLOWED                1


/* Wait Constants (in ms)*/
#define IPTCOM_STARTUP_WAIT             2000
#define SHARED_MEMORY_ATTACHED_WAIT     2000
#define IPTCOM_GET_STATUS_WAIT          2000
#define QUEUE_CREATE_WAIT               2000
#define ADD_LISTENER_WAIT               2000
#define IPTCOM_ADD_CONFIG_WAIT          2000
#define REGISTER_PUBLISHER_WAIT         2000

#define DLEDS_QUEUE_SIZE                16

/* First try is not included in this value, This value is number of retries */
#define MAX_NO_OF_RETRIES               2
#define MAX_NO_OF_GET_STATUS_RETRIES    14

/* Application result code in Response, Status and Progress messages */
#define DL_OK                           0
#define DL_TRANSFER_IN_PROGRESS         1
#define DL_INCOMPATIBLE_VERSION         -1
#define DL_URI_ERROR                    -8
#define DL_PARAM_ERROR                  -9
#define DL_REQUEST_FAILED               -21
#define DL_CRC_ERROR                    -50
#define DL_DISK_FULL                    -53
#define DL_FTP_ERROR                    -54
#define DL_DISK_ERROR                   -55
#define DL_NOT_ALLOWED                  -200
#define DL_ED_ERROR                     -201
#define DL_SCI_ERROR                    -202
#define DL_INSTALLATION_ERROR           -203
#define DL_MSG_INFO_INVALID             -204

#define FTP_MAX_SIZE_FILENAME           256
#define FTP_MAX_SIZE_PATH               256
#define FTP_MAX_SIZE_HOSTNAME           64
#define FTP_MAX_SIZE_USERNAME           64
#define FTP_MAX_SIZE_PASSWORD           64

#define DEFAULT_ED_TYPE_BT              "NOEDT_BT"
#define DEFAULT_ED_TYPE_CU              "NOEDT_CU"

#ifndef CHAR8
#define CHAR8 char
#endif

#define DLEDS_THREAD_STACK_SIZE         0x8000
#define DLEDS_THREAD_CYCLE              5000    /* cycle time in milli seconds */

/*******************************************************************************
 * TYPEDEFS
 */

/*
 * Declaration of states for the DLEDS main state machine
 */
typedef enum
{
    UNDEFINED=0, 
    INITIALIZED, 
    WAIT_FOR_REQUEST, 
    SEND_RESPONSE, 
    SEND_STATUS, 
    STARTED, 
    DOWNLOAD_MODE, 
    RESET_TO_RUN_MODE,
    WAIT_FOR_RESPONSE_RESULT, 
    WAIT_FOR_STATUS_RESULT, 
    PROCESS_RESULT,
    RESET_MODE,
    STOP, 
    TERMINATED,
    IDLE_MODE
} STATE;

typedef int DLEDS_RESULT;

typedef struct STR_DLEDS_REQUEST_INFO
{
    UINT32  transactionId;
    char    fileName[256];
    char    fileServerHostName[64];
    char    fileServerPath[256];
    UINT32  fileSize;
    UINT32  fileCRC;
    UINT32  fileVersion;    
} TYPE_DLEDS_REQUEST_INFO;

typedef struct STR_DLEDS_SESSION_DATA
{
    UINT32                      requestReceiveTime;
    UINT32                      sessionId;
    UINT32                      srcIpAddress;
    char                        srcUri[102];
    UINT32                      transferInProgress;
    UINT32                      localTransfer;
    INT32                       appResultCode;
    TYPE_DLEDS_REQUEST_INFO     requestInfo;
    UINT32                      downloadResetReason;    /* 0 = Normal reset, 1 = Download,  2 = Cleanup,  3 = Ongoing */
    UINT32                      protocolVersion;        /* 1 = Version 1, 2 = Version 2 */
    UINT32                      packageVersion;         /* 1 = Version 1, 2 = Version 2 */
    UINT32                      completedOperation;     /* 0 = No,        1 = Yes, see downloadResetReason for operation */
} TYPE_DLEDS_SESSION_DATA;

typedef enum
{
    Init = 0,
    AtDelim,
    DevId,
    DevDelim,
    CarId,
    CarDelim,
    CstId,
    CstDelim,
    TrainId,
    Valid,
    Error
} DLEDS_HOST_URI_FQDN;

         
DLEDS_RESULT dleds_initialize(void);
DLEDS_RESULT dleds_storageFileExists(void);
DLEDS_RESULT dleds_readStorageFile(TYPE_DLEDS_SESSION_DATA* pSessionData);
DLEDS_RESULT dleds_writeStorageFile(TYPE_DLEDS_SESSION_DATA* pSessionData);
DLEDS_RESULT dleds_wait_for_request(void);
DLEDS_RESULT dleds_send_status_message(void);
DLEDS_RESULT dleds_send_response_message(void);
DLEDS_RESULT dleds_wait_for_result(void);
DLEDS_RESULT dleds_download_mode(void);
DLEDS_RESULT dleds_uninit(void);
void dleds_sendEchoMessage(void);
DLEDS_RESULT dleds_checkDownloadModeStatus(void);
DLEDS_RESULT dleds_checkRunModeStatus(void);
void dleds_resetToRunMode(void);
void dleds_resetToDownloadMode(void);

int removeDirectory(const char* path);
DLEDS_RESULT dleds_checkCRC(char* pDestFile, UINT32 fileCRC, UINT32 startValue);
DLEDS_RESULT dleds_reportProgress(
    UINT8   ongoingOperation,
    UINT8   progressInfo,
    UINT8   requestReset,
    UINT32  resetTimeout,
    INT32   errorCode,
    CHAR8*  infoText,
    UINT8*  pAbortRequest);
DLEDS_RESULT dleds_diskSpaceAvailable(char*  filePath,UINT32 fileSize);

#endif /* DLEDS_H */
