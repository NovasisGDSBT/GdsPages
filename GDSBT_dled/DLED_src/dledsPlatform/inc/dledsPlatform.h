/************************************************************************/
/*  (C) COPYRIGHT 2008 by Bombardier Transportation                     */
/*                                                                      */
/*  Bombardier Transportation Switzerland Dep. PPC/EUT                  */
/************************************************************************/
/*                                                                      */
/*  PROJECT:      Remote download                                       */
/*                                                                      */
/*  MODULE:       dledsPlatform                                         */
/*                                                                      */
/*  ABSTRACT:     This is the header file for platform specific code    */
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
/*  version  yy-mm-dd  name       depart.    ref   status               */
/*  -------  --------  ---------  -------    ----  ---------            */
/*    1.0    15-03-27  S.Eriksson GSC/NETPS   --   created              */
/*                                                                      */
/************************************************************************/
#ifndef DLEDS_PLATFORM_H
#define DLEDS_PLATFORM_H

#include "iptcom.h"
#include "vos.h"
#include "dledsPlatformDefines.h"

/******************************************************************************
*   ADD CODE FOR PLATFORM (START)
******************************************************************************/
/* Define DLEDS root directory */
#define FILE_SYSTEM_NAME                "/"
#define DLEDS_ROOT_DIRECTORY            "/tmp/dleds"

/* Files used by DLEDS */
#define DLEDS_XML_TMP_FILE              "/tmp/dleds/tempXmlFile.xml"
#define DLEDS_INSTALLATION_FILE         "/tmp/dleds/dledsDluInst.txt"
#define DLEDS_DELAY_FILE_NAME           "/tmp/dleds/dledsDelay.txt"
#define DLEDS_STORAGE_FILE_NAME         "/tmp/dleds/dledsStgFile.tmp"
#define DLEDS_DEBUG_ACTIVE              "/tmp/dleds/DLEDS_DEBUG_ACTIVE"

/* Files used by DLEDS for Debugging */
#define DBG_EVENT_LOG         "/tmp/dleds/minchiadifiledilog.txt"
#define DBG_EVENT_POS         "/tmp/dleds/dledsEventPos.txt"
#define DBG_FILE_PATH         "/tmp/dleds"
#define MAX_EVENTLOG_SIZE     102400   /* 100kb */

/*
 * These defines are used when calling the IPT-COM SDK function IPTVosThreadSpawn
 * and need to be defined of the user to be adapted to the platform. For more
 * information see IPT-COM documentation and actual platform code added in the
 * IPT-COM SDK file vos.c for the function IPTVosThreadSpawn.
 */
#define DLEDS_THREAD_POLICY             1

/* Add value and remove comments
#define DLEDS_THREAD_POLICY             <value>
*/
#ifndef DLEDS_THREAD_POLICY
  #error "DLEDS_THREAD_POLICY not defined"
#endif

#define DLEDS_THREAD_PRIORITY           100

/* Add value and remove comments
#define DLEDS_THREAD_PRIORITY           <value>
*/
#ifndef DLEDS_THREAD_PRIORITY
  #error "DLEDS_THREAD_PRIORITY not defined"
#endif

/*****************************************************************************
*   ADD CODE FOR PLATFORM (END)
******************************************************************************/


int dledsPlatformGetOperationMode(void);
int dledsPlatformGetDlAllowed(void);
int dledsPlatformForcedReboot(int rebootReason);
int dledsPlatformGetAppMode(int* IEdMode, int* IConfigStatus);
DLEDS_RESULT dledsPlatformFtp(char* destFile,
                              char* ipAddrStr,
                              T_IPT_IP_ADDR ipAddr,
                              char* fileServerPath,
                              char* fileName);
DLEDS_RESULT dledsPlatformSetDledsTempPath(char* dledsDirectoryPath);
UINT32 dledsPlatformCalculateFreeDiskSpace(char*  filePath);
DLEDS_RESULT dledsPlatformGetSwInfoXml(UINT32 *pActSize,
                                       char   **ppInfoBuffer);
DLEDS_RESULT dledsPlatformGetHwInfoXml(UINT32 *pActSize,
                                       char   **ppInfoBuffer);
void dledsPlatformReleaseInfoXml(char *pInfoBuffer);
DLEDS_RESULT dledsPlatformGetHwInfo(char* unit_type,
                                    char* serial_number,
                                    char* delivery_revision);
void dledsPlatformGetEdIdentSwVersions(UINT32* btDluVersion,
                                       UINT32* cuDluVersion,
                                       UINT32* osDluVersion,
                                       UINT32* blcfgUluVersion,
                                       UINT32* ubootUluVersion);
DLEDS_RESULT dledsPlatformCreateSubDirectory(char* dirName);
DLEDS_RESULT dledsPlatformCleanupBT(void);
DLEDS_RESULT dledsPlatformCleanupCU(void);
DLEDS_RESULT dledsPlatformCleanupEDSP2(char* edspName);
DLEDS_RESULT dledsPlatformRemovePackages(void);
DLEDS_RESULT dledsPlatformInstallDlu(TYPE_DLEDS_DLU_DATA* pDluData);
DLEDS_RESULT dledsPlatformInstallDlu2(TYPE_DLEDS_DLU_DATA2* pDluData);
DLEDS_RESULT dledsPlatformUnpackTarFile(char*   fileName,
                                        char*   destinationDir,
                                        UINT8   compressed);
DLEDS_RESULT dleds_initialize(void);

int dleds_add_ipt_config(void);

DLEDS_RESULT dledsPlatformInspectPackage(char* downloadFileName);
DLEDS_RESULT dledsPlatformPassPackageProcessing(void);

#endif /* DLEDS_PLATFORM_H */
