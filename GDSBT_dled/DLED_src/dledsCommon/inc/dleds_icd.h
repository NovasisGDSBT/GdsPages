/************************************************************************/
/*  (C) COPYRIGHT 2015 by Bombardier Transportation                     */
/*                                                                      */
/*  Bombardier Transportation Switzerland Dep. PPC/EUT                  */
/************************************************************************/
/*                                                                      */
/*  PROJECT:      Remote download                                       */
/*                                                                      */
/*  MODULE:       dleds_icd                                             */
/*                                                                      */
/*  ABSTRACT:     This file contains definitions for the ComIDs and     */
/*                Datasets defined for DLEDS                            */
/*                                                                      */
/*  REMARKS:      -                                                     */
/*                                                                      */
/*  DEPENDENCIES: -                                                     */
/*                                                                      */
/*  ACCEPTED:     -                                                     */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/*  HISTORY:                                                            */
/*                                                                      */
/*  version  yy-mm-dd  name       depart.    ref   status               */
/*  -------  --------  ---------  ---------  ----  ---------            */
/*    1.0    15-06-10  S.Eriksson GSC/NETPS   --   created              */
/*                                                                      */
/************************************************************************/
#ifndef DLEDS_ICD_H
#define DLEDS_ICD_H

/*
 * ComIDs for DLEDS telegrams
 * ComIDs and data-set IDs are defined to the same value
 *
 * i<......>      i indicates direction Deployment Tool --> ED
 * o<......>      o indicates direction ED --> Deployment Tool
 *
 */
#define iTISDownload            270
#define oEDIdent                271
#define iEDReqStatus            272
#define oEDRepStatus            273
#define iEDReqVersionInfo       274
#define oEDRepVersionInfo       275
#define iEDReqSendFile2         276
#define oEDRepSendFile2         277
#define iEDReqStatus2           278
#define oEDRepStatus2           279

#define oEDReqDLProgress        290
#define iEDRepDLProgress        291
#define iEDReqCleanUp           292
#define oEDRepCleanUp           293
#define oEDPing                 294
#define iMCGReqSendFile         850
#define oMCGRepSendFile         851
#define iMCGReqSendFile2        852
#define iMCGDLFileTfST          854



/*
 * Dataset TISDownload for 
 *      ComId 270 (oTISDownload)
 *
 */
#define TISDownload_appVersion      0x01000000

typedef struct tag_TISDownload 
{
    UINT32 appVersion;
    UINT8  ISysSoftwareDownload;
    UINT8  reserved1;
    CHAR8  reserved2[26];
} TISDownload;



/*
 * Dataset EDIdent for 
 *      ComId 271 (oEDIdent)
 *      ComId 273 (oEDRepStatus)
 *      ComId 279 (oEDRepStatus2)
 *
 */
typedef struct tag_EDIdent_BTSwNumber       /* Version for BT package */
{
    UINT8   ISysBTSwVersion;
    UINT8   ISysBTSwRelease;
    UINT8   ISysBTSwUpdate;
    UINT8   ISysBTSwEvolution;
    UINT8   Reserved[12];
} EDIdent_BTSwNumber;

typedef struct tag_EDIdent_CUSwNumber       /* Version for CU package*/
{
    UINT8   ISysCUSwVersion;
    UINT8   ISysCUSwRelease;
    UINT8   ISysCUSwUpdate;
    UINT8   ISysCUSwEvolution;
    UINT8   Reserved[12];
} EDIdent_CUSwNumber;

typedef struct tag_EDIdent_OSBaseSwNumber   /* Version for OS Base */
{
    UINT8   ISysOSBaseSwVersion;
    UINT8   ISysOSBaseSwRelease;
    UINT8   ISysOSBaseSwUpdate;
    UINT8   ISysOSBaseSwEvolution;
    UINT8   Reserved[12];
} EDIdent_OSBaseSwNumber;

typedef struct tag_EDIdent_BLConfigSwNumber /* Version for BootLoader Configuration */
{
    UINT8   ISysBLConfigSwVersion;
    UINT8   ISysBLConfigSwRelease;
    UINT8   ISysBLConfigSwUpdate;
    UINT8   ISysBLConfigSwEvolution;
    UINT8   Reserved[12];
} EDIdent_BLConfigSwNumber;

typedef struct tag_EDIdent_BLSwNumber       /* Version for BootLoader */
{
    UINT8   ISysBLSwVersion;
    UINT8   ISysBLSwRelease;
    UINT8   ISysBLSwUpdate;
    UINT8   ISysBLSwEvolution;
    UINT8   Reserved[12];
} EDIdent_BLSwNumber;

#define EDIdent_HWInfo_IHwRevisionSize         16
#define EDIdent_HWInfo_ISerialNumberSize       16
#define EDIdent_HWInfo_IDeviceTypeSize         16
#define EDIdent_HWInfo_ISupplierNameSize       16
#define EDIdent_HWInfo_ISupplierDataSize       16

typedef struct tag_EDIdent_HWInfo           /* HW version info */
{
    CHAR8   IHwRevision[EDIdent_HWInfo_IHwRevisionSize];
    CHAR8   ISerialNumber[EDIdent_HWInfo_ISerialNumberSize];
    CHAR8   IDeviceType[EDIdent_HWInfo_IDeviceTypeSize];
    CHAR8   ISupplierName[EDIdent_HWInfo_ISupplierNameSize];
    CHAR8   ISupplierData[EDIdent_HWInfo_ISupplierDataSize];
    CHAR8   Reserved[16];
} EDIdent_HWInfo;

typedef struct tag_EDIdent_EndDeviceMode    /* End device status information */
{
    UINT8   IEdMode;
    UINT8   IConfigStatus;
    UINT8   DownloadAllowed;
    UINT8   Reserved[13];
} EDIdent_EndDeviceMode;

#define EDIdent_IPConfig_IDevIPAddressSize      16
#define EDIdent_IPConfig_IDevFQDNSize           128

typedef struct tag_EDIdent_IPConfig         /* IP configuration information */
{
    CHAR8   IDevIPAddress[EDIdent_IPConfig_IDevIPAddressSize];
    CHAR8   IDevFQDN[EDIdent_IPConfig_IDevFQDNSize];
    UINT8   Reserved[44];
} EDIdent_IPConfig;

#define EDIdent_appVersion      0x01000000

typedef struct tag_EDIdent
{
    UINT32                      appVersion;
    EDIdent_BTSwNumber          BTSwNumber;
    EDIdent_CUSwNumber          CUSwNumber;
    EDIdent_OSBaseSwNumber      OSBaseSwNumber;
    EDIdent_BLConfigSwNumber    BLConfigSwNumber;
    EDIdent_BLSwNumber          BLSwNumber;
    EDIdent_HWInfo              HWInfo;
    EDIdent_EndDeviceMode       EndDeviceMode;
    EDIdent_IPConfig            IPConfig;
}EDIdent;



/*
 * Dataset EDReqStatus for
 *      ComId 272 (iEDReqStatus)
 *      ComId 278 (iEDReqStatus2)
 *
 */
#define EDReqStatus_appVersion          0x01000000

typedef struct tag_EDReqStatus
{
    UINT32  appVersion;
    UINT8   reserved1;
    UINT8   reserved2;
    CHAR8   reserved3[26];
} EDReqStatus;



/*
 * Dataset EDReqVersionInfo for
 *      ComId 274 (iEDReqVersionInfo)
 *
 */
#define EDReqVersionInfo_appVersion         0x01000000

#define VERSION_INFO_NO_DATA                0
#define VERSION_INFO_SW                     1
#define VERSION_INFO_HW                     2

typedef struct tag_EDReqVersionInfo
{
    UINT32  appVersion;
    UINT8   typeOfVersionInfo;
    UINT8   reserved1;
    CHAR8   reserved2[26];
} EDReqVersionInfo;



/*
 * Dataset EDRepVersionInfo for
 *      ComId 275 (oEDRepVersionInfo)
 *
 */
#define EDRepVersionInfo_appVersion         0x01000000

#define VERSION_INFO_UNDEFINED              0
#define VERSION_INFO_OK                     1
#define VERSION_INFO_NOT_AVAILABLE          2
#define VERSION_INFO_NOT_SUPPORTED          3

typedef struct tag_EDRepVersionInfo
{
    UINT32  appVersion;
    UINT16  returnValue;
    UINT8   reserved[6];
    UINT16  versionInfolength;
    CHAR8   startVersionInfo;
} EDRepVersionInfo;



/*
 * Dataset EDReqSendFile2 for
 *      ComId 276 (iEDReqSendFile2)
 *
 */
#define EDReqSendFile2_appVersion           0x01000000
#define EDReqSendFile2_FilenameSize         256
#define EDReqSendFile2_FileserverName       64
#define EDReqSendFile2_FileserverPath       256
#define EDReqSendFile2_SpecificParamSize    512

typedef struct tag_EDReqSendFile2
{
    UINT32 appVersion;
    UINT32 transactionId;
    CHAR8  fileName[EDReqSendFile2_FilenameSize];
    CHAR8  fileServerHostName[EDReqSendFile2_FileserverName];
    CHAR8  fileServerPath[EDReqSendFile2_FileserverPath];
    UINT32 fileSize;
    UINT32 fileCRC;
    UINT32 fileVersion;
    UINT16 noOfChars;
    CHAR8  specificParam[EDReqSendFile2_SpecificParamSize];
} EDReqSendFile2;



/*
 * Dataset EDRepSendFile2 for
 *      ComId 277 (iEDRepSendFile2)
 *
 */
#define EDRepSendFile2_appVersion             0x01000000

typedef struct tag_EDRepSendFile2
{
    UINT32 appVersion;
    INT32  appResultCode;
} EDRepSendFile2;



/*
 * Dataset EDReqDLProgress for
 *      ComId 290 (oEDReqDLProgress)
 *
 */
#define EDReqDLProgress_appVersion      0x01000000
#define EDReqDLProgress_InfoTextSize    115

#define PROGRESS_ED_IN_RUN_MODE           0
#define PROGRESS_DL_INITIALIZED           1
#define PROGRESS_ED_IN_DL_MODE            2
#define PROGRESS_FTP_PROGRESS             3
#define PROGRESS_FTP_COMPLETED            4
#define PROGRESS_PACKAGE_OK               5
#define PROGRESS_VERIFICATION             6
#define PROGRESS_INSTALL_PROGRESS         7
#define PROGRESS_INSTALL_COMPLETED        8
#define PROGRESS_ED_RESTART_PROGRESS      9
#define PROGRESS_CLEAN_UP_INITIALIZED    10
#define PROGRESS_CLEAN_UP_PROGRESS       11
#define PROGRESS_CLEAN_UP_COMPLETED      12

#define PROGRESS_3                        3
#define PROGRESS_5                        5
#define PROGRESS_10                      10
#define PROGRESS_20                      20
#define PROGRESS_30                      30
#define PROGRESS_40                      40
#define PROGRESS_45                      45
#define PROGRESS_60                      60
#define PROGRESS_70                      70
#define PROGRESS_80                      80
#define PROGRESS_90                      90
#define PROGRESS_95                      95
#define PROGRESS_100                    100


#define PROGRESS_NO_RESET_IN_PROGRESS   0
#define PROGRESS_RESET_REQUEST          1

#define PROGRESS_USE_DEFAULT_TIMEOUT    300000       /* Default timeout 5 minutes should be used by MTVD */
#define PROGRESS_REPLY_TIMEOUT          5000    /* milliseconds */

typedef struct tag_EDReqDLProgress
{
    UINT32  appVersion;
    UINT8   ongoingOperation;
    UINT8   progressInfo;
    UINT8   requestReset;
    UINT8   reserved;
    UINT32  resetTimeout;
    INT32   errorCode;
    CHAR8   infoText[EDReqDLProgress_InfoTextSize];
} EDReqDLProgress;



/*
 * Dataset EDRepDLProgress for
 *      ComId 291 (oEDRepDLProgress)
 *
 */
#define EDRepDLProgress_appVersion      0x01000000

#define PROGRESS_CONTINUE               0
#define PROGRESS_ABORT                  1

typedef struct tag_EDRepDLProgress
{
    UINT32  appVersion;
    UINT8   abortRequest;
    UINT8   acknowledgeReset;
    CHAR8   reserved[26];
} EDRepDLProgress;



/*
 * Dataset EDReqCleanUp for
 *      ComId 292 (iEDReqCleanUp)
 *
 */
#define EDReqCleanUp_appVersion         0x01000000

#define CLEAN_UP_ALL_PACKAGES           1

typedef struct tag_EDReqCleanUp
{
    UINT32  appVersion;
    UINT32  functionCode;
    CHAR8   reserved[28];
} EDReqCleanUp;



/*
 * Dataset EDRepCleanUp for
 *      ComId 293 (oEDRepCleanUp)
 *
 */
#define EDRepCleanUp_appVersion         0x01000000

#define CLEAN_UP_OK                     0
#define CLEAN_UP_NOT_ALLOWED            1
#define CLEAN_UP_NOT SUPPORTED          2

typedef struct tag_EDRepCleanUp
{
    UINT32  appVersion;
    UINT8   status;
    UINT8   reserved1;
    UINT8   reserved2[26];
} EDRepCleanUp;



/*
 * Dataset EDPing for
 *      ComId 294 (oEDPing)
 *
 */
#define EDPing_appVersion         0x01000000

typedef struct tag_EDPing
{
    UINT32  appVersion;
    CHAR8   reserved[28];
} EDPing;


/*
 * Dataset MCGReqSendFile for 
 *      ComId 850 (iMCGReqSendFile)
 *
 */
#define MCGReqSendFile_appVersion           0x02000000
#define MCGReqSendFile_FilenameSize         256
#define MCGReqSendFile_FileserverName       64
#define MCGReqSendFile_FileserverPath       256
#define MCGReqSendFile_SpecificParamSize    512

typedef struct tag_MCGReqSendFile
{
    UINT32 appVersion;
    UINT32 transactionId;
    CHAR8  fileName[MCGReqSendFile_FilenameSize];
    CHAR8  fileServerHostName[MCGReqSendFile_FileserverName];
    CHAR8  fileServerPath[MCGReqSendFile_FileserverPath];
    UINT32 fileSize;
    UINT32 fileCRC;
    UINT32 fileVersion;
    UINT16 noOfChars;
    CHAR8  specificParam[MCGReqSendFile_SpecificParamSize];
} MCGReqSendFile;



/*
 * Dataset MCGRepSendFile for 
 *      ComId 851 (oMCGRepSendFile)
 *
 */
#define MCGRepSendFile_appVersion             0x02000000

typedef struct tag_MCGRepSendFile
{
    UINT32 appVersion;
    INT32  appResultCode;
} MCGRepSendFile;



/*
 * Dataset MCGReqSendFile2 for 
 *      ComId 852 (iMCGReqSendFile2)
 *
 */
#define MCGReqSendFile2_appVersion             0x02000000
#define MCGReqSendFile2_FilenameSize           256
#define MCGReqSendFile2_FileserverName         64
#define MCGReqSendFile2_FileserverPath         256
#define MCGReqSendFile2_SpecificParamSize      640

typedef struct tag_MCGReqSendFile2
{
    UINT32 appVersion;
    UINT32 transactionId;
    CHAR8  fileName[MCGReqSendFile2_FilenameSize];
    CHAR8  fileServerHostName[MCGReqSendFile2_FileserverName];
    CHAR8  fileServerPath[MCGReqSendFile2_FileserverPath];
    UINT32 fileSize;
    UINT32 fileCRC;
    UINT32 fileVersion;
    UINT16 noOfChars;
    CHAR8  specificParam[MCGReqSendFile2_SpecificParamSize];
} MCGReqSendFile2;



/*
 * Dataset MCGDLFTfSt for 
 *      ComId 854 (iMCGDLFTfSt)
 *
 */
#define MCGDLFTfSt_appVersion             0x02000000

typedef struct tag_MCGDLFTfSt
{
    UINT32 appVersion;
    INT32  appResultCode;
    UINT32 transactionId;
} MCGDLFTfSt;

#endif  /* DLEDS_ICD_H */
