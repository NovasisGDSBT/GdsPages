/************************************************************************/
/*  (C) COPYRIGHT 2008 by Bombardier Transportation                     */
/*                                                                      */
/*  Bombardier Transportation Switzerland Dep. PPC/EUT                  */
/************************************************************************/
/*                                                                      */
/*  PROJECT:      Remote download                                       */
/*                                                                      */
/*  MODULE:       dledsInstall                                          */
/*                                                                      */
/*  ABSTRACT:     This is the header file for download ED service       */
/*                install part                                          */
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
#ifndef DLEDSINSTALL_H
#define DLEDSINSTALL_H


/*******************************************************************************
*   INCLUDES
*/

/*******************************************************************************
*  TYPEDEFS
*/
typedef struct STR_DLEDS_DLU_DATA
{
  char    dluFilePath[256];
  char    dluFileName[256];
  char    edPackageName[32];
  UINT32  edPackageVersion;
  char    sciName[32];
  UINT32  sciVersion;
  char    packageSource[3];
}  TYPE_DLEDS_DLU_DATA;

typedef struct STR_DLEDS_DLU_DATA2
{
  char    dluFilePath[256];
  char    dluFileName[256];
  char    edPackageName[32];
  UINT32  edPackageVersion;
  char    sciName[32];
  UINT32  sciVersion;
  char    packageSource[3];
  UINT32  edCrc;
  char    edInfo[32];
  char    edTimeStamp[32];
  UINT32  sciCrc;
  char    sciInfo[32];
  char    sciTimestamp[32];
}  TYPE_DLEDS_DLU_DATA2;

typedef union {
    UINT32 version;
#ifdef O_LE
    struct {
        unsigned char b;
        unsigned char u;
        unsigned char r;
        unsigned char v;
    } s;
#else
    struct {
        UINT8 v;
        UINT8 r;
        UINT8 u;
        UINT8 b;
    } s;
#endif
}VRUE_TYPE;

/*******************************************************************************
*   GLOBAL FUNCTIONS
*/
DLEDS_RESULT  dledsInstallEDPackage(
    char* downloadFileName,      /* IN: TAR file containing ED package to be installed */
    char* tempDir);              /* IN: Path to temporary directory */
    
DLEDS_RESULT  dledsInstallEDSP2Package(
    char* downloadFileName,      /* IN: TAR file containing ED package to be installed */
    char* tempDir);              /* IN: Path to temporary directory */

DLEDS_RESULT createSubDirectory(char* dirName);
DLEDS_RESULT dledsCleanupBT(void);
DLEDS_RESULT dledsCleanupCU(void);
    

/*******************************************************************************
*  MACROS
*/


/*******************************************************************************
*   DEFINES
*/ 


#endif /* DLEDSINSTALL_H */
