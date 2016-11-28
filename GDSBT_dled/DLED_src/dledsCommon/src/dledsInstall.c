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
/*  ABSTRACT:     This is the dleds module that handles the             */
/*                installation of the ED package                        */
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
#include <string.h>
#include "iptcom.h"
#include "vos.h"
#include "dleds.h"
#include "dledsInstall.h"
#include "dledsXml.h"
#include "dledsDbg.h"
#include "dledsCrc32.h"
#include "dledsPlatform.h"
#include "dleds_icd.h"

/*******************************************************************************
 * DEFINES
 */
#ifdef WIN32
 #define snprintf _snprintf
#endif


/*******************************************************************************
 * TYPEDEFS
 */



/*******************************************************************************
 * LOCAL FUNCTION DECLARATIONS
 */



/*******************************************************************************
 * LOCAL VARIABLES
 */


/*******************************************************************************
 * GLOBAL VARIABLES
 */
 char currentDir[256];

 char installedBtPackageName[64] = "";
 UINT32 installedBtPackageVersion = 0;
 char installedCuPackageName[64] = "";
 UINT32 installedCuPackageVersion = 0;

 UINT32 totalNoOfDLU = 0;

/*******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 */

/*******************************************************************************
 * 
 * Function name: setRootDirectory
 *
 * Abstract:      This function set current directory to the DLEDS root directory.
 *                Checks that the directory exists.  
 *
 * Return value:  DLEDS_OK      - Global variable for current directory updated
 *                DLEDS_ERROR   - Could not set current directory
 *
 * Globals:       currentDir
 */
static DLEDS_RESULT setRootDirectory(
    void)                               /* IN: None */
{
    DLEDS_RESULT    result = DLEDS_OK;
    struct stat     statbuf;
    
    strcpy(currentDir, DLEDS_ROOT_DIRECTORY);
    if (!stat( currentDir, &statbuf ))
    {
        if (!S_ISDIR( statbuf.st_mode ))
        {
            DebugError1("DLEDS root directory is missing (%s)", currentDir);
            result = DLEDS_ERROR;
        }
    }
    
    return result;
}


/*******************************************************************************
 * 
 * Function name: createSubDirectory
 *
 * Abstract:      This function creates a sub directory if it does not exist. 
 *
 * Return value:  DLEDS_OK      -  Directory exists or has been created
 *                DLEDS_ERROR   -  Could not create directory
 *
 * Globals:       
 */
DLEDS_RESULT createSubDirectory(
    char* dirName)                      /* IN: Full path to directory */
{
    DLEDS_RESULT    result = DLEDS_ERROR;

    /******************************************************************************
    Create new sub directory
    ******************************************************************************/ 
    /* Call function that contains Platform Specific code */
    result = dledsPlatformCreateSubDirectory(dirName);   
    DebugError2("***PLATFORM: Create new sub directory (%s), result (%d)", dirName, result);

    return result;
}

/*******************************************************************************
 * 
 * Function name: downCurrentDirectory
 *
 * Abstract:      This function change global variable currentDir to requested sub directory.
 *                Creates the new directory if it doesn't exists. Global variable currentDir
 *                is not changed if the function fails.  
 *
 * Return value:  DLEDS_OK      - Global variable for current directory updated
 *                DLEDS_ERROR   - Could not change current directory
 *
 * Globals:       currentDir
 */
static DLEDS_RESULT downCurrentDirectory(
    char* subDirectory)                 /* IN: Name of sub directory */
{
    DLEDS_RESULT    result = DLEDS_OK;
    char            newDirectory[256];
    struct stat     statbuf;

    strcpy(newDirectory, currentDir);
    strcat(newDirectory, "/");
    strcat(newDirectory, subDirectory);
    /* Make sure that directory exists */
    if (!stat( newDirectory, &statbuf ))
    {
        if (!S_ISDIR( statbuf.st_mode ))
        {
            DebugError1("This is not a directory (%s)", newDirectory);
            result = DLEDS_ERROR;
        }
        else
        {
            strcpy(currentDir,newDirectory);
        }
    }
    else
    {
        if (createSubDirectory(newDirectory) == DLEDS_OK)
        {
            /* Change current directory to sub directory */
            strcpy(currentDir, newDirectory);
        }
        else
        {
            DebugError1("Could not create directory (%s)", newDirectory);
            result = DLEDS_ERROR;
        }
    }
    
    return result;
}

/*******************************************************************************
 * 
 * Function name: upCurrentDirectory
 *
 * Abstract:      This function change current directory to parent directory. 
 *
 * Return value:  DLEDS_OK      -  Global variable for current directory updated
 *                DLEDS_ERROR   -  Could not change current directory
 *
 * Globals:       currentDir
 */
static DLEDS_RESULT upCurrentDirectory(
    void)                               /* IN: No input parameter */
{
    DLEDS_RESULT    result = DLEDS_OK;
    char*           pch;

    /* Make sure that current directory is NOT equal to DLEDS root directory and at least one sub level down */
    if ((strcmp(currentDir, DLEDS_ROOT_DIRECTORY) != 0) && (strlen(currentDir) > strlen(DLEDS_ROOT_DIRECTORY)))
    {
        pch = strrchr(currentDir,'/');
        if (pch != NULL)
        {
            currentDir[pch-currentDir] = '\0';
        }
        else
        {
            DebugError1("Could not move up from current directory (%s)", currentDir);   
            result = DLEDS_ERROR;
            strcpy(currentDir, DLEDS_ROOT_DIRECTORY);
        }
    }
    else
    {
        result = DLEDS_ERROR;
        strcpy(currentDir, DLEDS_ROOT_DIRECTORY);
    }
    /*DebugError1("UP - NEW CURRENT DIR: (%s)", currentDir);*/   
    return result;
}

/*******************************************************************************
 * 
 * Function name: toVersion
 *
 * Abstract:      This function converts the version string on the format
 *                "V.R.U.B" to a UINT32 value.
 *
 * Return value:  version  -  Both paramId and value found
 *
 * Globals:       none
 */
static UINT32 toVersion(const char *version)
{
    unsigned int v,r,u,b;
    VRUE_TYPE ver;
    char c;

    if (sscanf(version, "%u%c%u%c%u%c%u", &v, &c, &r, &c, &u, &c, &b) == 7)
    {
        ver.s.v = (unsigned char)v;
        ver.s.r = (unsigned char)r;
        ver.s.u = (unsigned char)u;
        ver.s.b = (unsigned char)b;
        return ver.version;
    }
    else
        return atoi(version);
}



/*******************************************************************************
 * 
 * Function name: unpackTarFile
 *
 * Abstract:      This function unpack a TAR file. The files within the TAR file
 *                are extracted to the directory pointed out by input parameter.
 *                The TAR file is deleted when it has been unpacked.
 *
 * Return value:  DLEDS_OK      -  TAR file extracted successfully
 *                DLEDS_ERROR   -  TAR file not extracted
 *
 * Globals:       currentDir - point to directory where TAR file resides
 */
static DLEDS_RESULT unpackTarFile(
    char*   fileName,                   /* IN: TAR file with full path */
    char*   destinationDir,             /* IN: Directory to extract to */
    UINT8   compressed)                 /* IN: 1= TAR file is compressed */
{
    DLEDS_RESULT    result = DLEDS_ERROR;
        
    /******************************************************************************
    Unpack TAR file to destination directory
    ******************************************************************************/    
    result = dledsPlatformUnpackTarFile(fileName, destinationDir, compressed);    
    DebugError3("***PLATFORM: TAR file(%s) unpacked in directory (%s), result (%d)", fileName, destinationDir, result);

    remove(fileName);
    return result;
}


/*******************************************************************************
 * 
 * Function name: dledsCleanupEDSP2
 *
 * Abstract:      This function deletes all DLU:s that are part of the
 *                installed EDSP version 2.
 *
 * Return value:  DLEDS_OK      -  Installed BT package deleted
 *                DLEDS_ERROR   -  Could not delete installed BT package
 *
 * Globals:       None
 */
DLEDS_RESULT dledsCleanupEDSP2(
    char*  edspName)          /* IN: Name of EDSP to be deleted */
{
    DLEDS_RESULT        result = DLEDS_ERROR;

    /******************************************************************************
    Delete files from previosly installed EDSP version 2
    ******************************************************************************/
    /* Call function that contains Platform Specific code */
    result = dledsPlatformCleanupEDSP2(edspName);
    DebugError2("***PLATFORM: Delete all installed DLU:s from EDSP %s, result (%d)", edspName, result);
    
    return result;    
}



/*******************************************************************************
 * 
 * Function name: dledsCleanupBT
 *
 * Abstract:      This function deletes all DLU:s that are part of the
 *                previously installed BT package.
 *
 * Return value:  DLEDS_OK      -  Installed BT package deleted
 *                DLEDS_ERROR   -  Could not delete installed BT package
 *
 * Globals:       None
 */
DLEDS_RESULT dledsCleanupBT(
    void)                               /* IN: None */
{
    DLEDS_RESULT        result = DLEDS_ERROR;

    /******************************************************************************
    Delete files from previosly installed BT package
    ******************************************************************************/
    /* Call function that contains Platform Specific code */
    result = dledsPlatformCleanupBT();
    DebugError1("***PLATFORM: Delete all installed DLU:s from BT sub package, result (%d)", result);
    
    return result;    
}


/*******************************************************************************
 * 
 * Function name: dledsCleanupCU
 *
 * Abstract:      This function deletes all DLU:s that are part of the
 *                previously installed CU package.
 *
 * Return value:  DLEDS_OK      -  Installed CU package deleted
 *                DLEDS_ERROR   -  Could not delete installed CU package
 *
 * Globals:       None
 */
DLEDS_RESULT dledsCleanupCU(
    void)                               /* IN: None */
{
    DLEDS_RESULT        result = DLEDS_ERROR;

    /******************************************************************************
    Delete files from previosly installed CU package
    ******************************************************************************/    
    /* Call function that contains Platform Specific code */
    result = dledsPlatformCleanupCU();
    DebugError1("***PLATFORM: Delete all installed DLU:s from CU sub package, result (%d)", result);

    return result;
}


/*******************************************************************************
 * 
 * Function name: dledsWriteInstallationEntry2
 *
 * Abstract:      This function writes an version 2 entry to the installation
 *                information file. 
 *
 * Return value:  DLEDS_OK      - Information from file has been written
 *                DLEDS_ERROR   - No information written to file
 *
 * Globals:       -
 */
static DLEDS_RESULT dledsWriteInstallationEntry2(
    const char*          fileName,      /* IN: Name of installation information file */
    TYPE_DLEDS_DLU_DATA2* pDluData)     /* IN: Data to write to file */
{
    FILE*           fp;
    DLEDS_RESULT    result = DLEDS_OK;
    char edInfo[32];
    char edTimeStamp[32];
    char sciInfo[32];
    char sciTimestamp[32];
    
    fp = fopen(fileName, "a" );
    if (fp == NULL)  
    {
        DebugError1("Failed to open installation information file (%s)",
            fileName);
        result = DLEDS_ERROR;
    }
    else
    {
        /* Check for parameters that are optional and could contain an empty string */
        /* In the installation file the empty string is represented by the NULL string */
        strcpy(edInfo, pDluData->edInfo);
        if (strlen(pDluData->edInfo) == 0)
        {
            strcpy(edInfo,"NULL");
        }
        strcpy(edTimeStamp, pDluData->edTimeStamp);
        if (strlen(pDluData->edTimeStamp) == 0)
        {
            strcpy(edTimeStamp,"NULL");
        }
        strcpy(sciInfo, pDluData->sciInfo);
        if (strlen(pDluData->sciInfo) == 0)
        {
            strcpy(sciInfo,"NULL");
        }
        strcpy(sciTimestamp, pDluData->sciTimestamp);
        if (strlen(pDluData->sciTimestamp) == 0)
        {
            strcpy(sciTimestamp,"NULL");
        }

        /* <dluFilepath> <dluFilename> <edPackageName> <edPackageVersion> <sciName> <sciVersion> <packageSource> */
        /* <edCrc> <edInfo><ETX> <edTimeStamp> <sciCrc> <sciInfo><ETX> <sciTimestamp> */
        /* Use <ETX> (ASCII value= 0x03) as stop character for the 2 Info strings since they could contain space characters */
        if ((fprintf(fp,"%s %s %s %u %s %u %s %u %s\3 %s %u %s\3 %s\n",
                pDluData->dluFilePath, 
                pDluData->dluFileName,
                pDluData->edPackageName,
                pDluData->edPackageVersion,
                pDluData->sciName,
                pDluData->sciVersion,
                pDluData->packageSource,
                pDluData->edCrc,
                edInfo,
                edTimeStamp,
                pDluData->sciCrc,
                sciInfo,
                sciTimestamp)) <= 0)
        {
            DebugError1("Failed to write to installation information file (%s)",
                fileName);
            result = DLEDS_ERROR;
        }
        fclose(fp);
    }

    return result;    
}


/*******************************************************************************
 * 
 * Function name: dledsWriteInstallationEntry
 *
 * Abstract:      This function writes an entry to the installation
 *                information file. 
 *
 * Return value:  DLEDS_OK      - Information from file has been written
 *                DLEDS_ERROR   - No information written to file
 *
 * Globals:       -
 */
static DLEDS_RESULT dledsWriteInstallationEntry(
    const char*          fileName,      /* IN: Name of installation information file */
    TYPE_DLEDS_DLU_DATA* pDluData)      /* IN: Data to write to file */
{
    FILE*           fp;
    DLEDS_RESULT    result = DLEDS_OK;
    char            sciName[64];
    
    fp = fopen(fileName, "a" );
    if (fp == NULL)  
    {
        DebugError1("Failed to open installation information file (%s)",
            fileName);
        result = DLEDS_ERROR;
    }
    else
    {
        /* Master DLU for ED package does not belong to any SCI */
        /* In the installation file the empty string is represented by the NULL string */
        strcpy(sciName, pDluData->sciName);
        if (strlen(pDluData->sciName) == 0)
        {
            strcpy(sciName,"NULL");
        }
        /* <dluFilepath> <dluFilename> <edPackageName> <edPackageVersion> <sciName> <sciVersion> <packageSource> */
        if ((fprintf(fp,"%s %s %s %u %s %u %s\n",
                pDluData->dluFilePath, 
                pDluData->dluFileName,
                pDluData->edPackageName,
                pDluData->edPackageVersion,
                sciName,
                pDluData->sciVersion,
                pDluData->packageSource)) <= 0)
        {
            DebugError1("Failed to write to installation information file (%s)",
                fileName);
            result = DLEDS_ERROR;
        }
        fclose(fp);
    }

    return result;    
}


/*******************************************************************************
 * 
 * Function name: dledsReadInstallationEntry2
 *
 * Abstract:      This function reads an entry from the version 2 installation
 *                information file. pNextFilePos is set to zero if
 *                EOF is reached. In this case no DLU data are returned.
 *
 * Return value:  DLEDS_OK      - Information from file has been read
 *                DLEDS_ERROR   - No information read from file
 *
 * Globals:       -
 */
static DLEDS_RESULT dledsReadInstallationEntry2(
    char*   fileName,                   /*  IN: Name of installation file */
    long    filePos,                    /*  IN: Position in file for entry */
    long*   pNextFilePos,               /* OUT: Position in file to next entry */
    TYPE_DLEDS_DLU_DATA2* pDluData)     /* OUT: Data read from file */
{
    FILE*           fp;
    DLEDS_RESULT    result = DLEDS_OK;
    
    fp = fopen(fileName, "r" );
    if (fp == NULL)  
    {
        DebugError1("Failed to open installation information file (%s)",
            fileName);
        result = DLEDS_ERROR;
    }
    else
    {
       if(fseek(fp, filePos, SEEK_SET) != 0)
       {
            DebugError1("Failed to position to entry in installation information file (%s)",
                fileName);
            result = DLEDS_ERROR;
       }
       else
       {
            if (feof(fp) != 0)
            {
                *pNextFilePos = 0;
            }
            else
            {        
                /* <dluFilepath> <dluFilename> <edPackageName> <edPackageVersion> <sciName> <sciVersion> <packageSource> */
                /* <edCrc> <edInfo><ETX> <edTimeStamp> <sciCrc> <sciInfo><ETX> <sciTimestamp> */
                /* <ETX> (ASCII value= 0x03) is used as stop character for the 2 Info strings since they could contain space characters */
                if (fscanf(fp,"%s %s %s %u %s %u %s %u %[^\3]\3 %s %u %[^\3]\3 %s", 
                    (pDluData->dluFilePath),
                    (pDluData->dluFileName),
                    (pDluData->edPackageName), 
                    &(pDluData->edPackageVersion), 
                    (pDluData->sciName), 
                    &(pDluData->sciVersion), 
                    (pDluData->packageSource),
                    &(pDluData->edCrc),
                    (pDluData->edInfo),
                    (pDluData->edTimeStamp),
                    &(pDluData->sciCrc),
                    (pDluData->sciInfo),
                    (pDluData->sciTimestamp)) == 13)
                {
                    /* All items for this entry read OK */
                    /* In the installation file the empty string is represented by the NULL string */
                    if (strcmp(pDluData->edInfo,"NULL") == 0)
                    {
                        strcpy(pDluData->edInfo,"");
                    }
                    if (strcmp(pDluData->edTimeStamp,"NULL") == 0)
                    {
                        strcpy(pDluData->edTimeStamp,"");
                    }
                    if (strcmp(pDluData->sciInfo,"NULL") == 0)
                    {
                        strcpy(pDluData->sciInfo,"");
                    }
                    if (strcmp(pDluData->sciTimestamp,"NULL") == 0)
                    {
                        strcpy(pDluData->sciTimestamp,"");
                    }

                    *pNextFilePos = ftell(fp);
                }
                else
                {
                    /* Not all items read, normally EOF reached */
                    *pNextFilePos = 0;
                }
            }
        }
        fclose(fp);
    }
    return result;    
}




/*******************************************************************************
 * 
 * Function name: dledsReadInstallationEntry
 *
 * Abstract:      This function reads an entry from the installation
 *                information file. pNextFilePos is set to zero if
 *                EOF is reached. In this case no DLU data are returned.
 *
 * Return value:  DLEDS_OK      - Information from file has been read
 *                DLEDS_ERROR   - No information read from file
 *
 * Globals:       -
 */
static DLEDS_RESULT dledsReadInstallationEntry(
    char*   fileName,                   /*  IN: Name of installation file */
    long    filePos,                    /*  IN: Position in file for entry */
    long*   pNextFilePos,               /* OUT: Position in file to next entry */
    TYPE_DLEDS_DLU_DATA* pDluData)      /* OUT: Data read from file */
{
    FILE*           fp;
    DLEDS_RESULT    result = DLEDS_OK;
    
    fp = fopen(fileName, "r" );
    if (fp == NULL)  
    {
        DebugError1("Failed to open installation information file (%s)",
            fileName);
        result = DLEDS_ERROR;
    }
    else
    {
       if(fseek(fp, filePos, SEEK_SET) != 0)
       {
            DebugError1("Failed to position to entry in installation information file (%s)",
                fileName);
            result = DLEDS_ERROR;
       }
       else
       {
            if (feof(fp) != 0)
            {
                *pNextFilePos = 0;
            }
            else
            {        
                /* <dluFilepath> <dluFilename> <edPackageName> <edPackageVersion> <sciName> <sciVersion> <packageSource> */
                if (fscanf(fp,"%s %s %s %u %s %u %s", 
                    (pDluData->dluFilePath),
                    (pDluData->dluFileName),
                    (pDluData->edPackageName), 
                    &(pDluData->edPackageVersion), 
                    (pDluData->sciName), 
                    &(pDluData->sciVersion), 
                    (pDluData->packageSource)) == 7)
                {
                    /* All items for this entry read OK */
                    /* Master DLU for ED package does not belong to any SCI */
                    /* In the installation file the empty string is represented by the NULL string */
                    if (strcmp(pDluData->sciName,"NULL") == 0)
                    {
                        strcpy(pDluData->sciName,"");
                    }
                    
                    *pNextFilePos = ftell(fp);
                }
                else
                {
                    /* Not all items read, normally EOF reached */
                    *pNextFilePos = 0;
                }
            }
        }
        fclose(fp);
    }
    return result;    
}



/*******************************************************************************
 * 
 * Function name: verifyDluHeader
 *
 * Abstract:      This function is just a dummy function at the moment.
 *
 * Return value:  DLEDS_OK      
 *
 * Globals:       -
 */
static DLEDS_RESULT verifyDluHeader(
    TYPE_DLU_HEADER*    dluHeader,
    UINT32              expectedDluVersion,
    UINT32              expectedDluTypeCode,
    char*               expectedDluName)
{
    dluHeader           = dluHeader;
    expectedDluVersion  = expectedDluVersion;
    expectedDluTypeCode = expectedDluTypeCode;
    expectedDluName     = expectedDluName;
    return DLEDS_OK;
}


/*******************************************************************************
 * 
 * Function name: verifyDluNameAndVersion
 *
 * Abstract:      This function verifies that the DLU file exists and contains
 *                the expected name and version.
 *
 * Return value:  DLEDS_OK      - Search successful
 *                DLEDS_ERROR   - Failed to search directory
 *
 * Globals:       -
 */
DLEDS_RESULT verifyDluNameAndVersion(
    const char* path,           /*  IN: Directory to search in */
    char*       dluFileName,    /*  IN: Name of DLU file */
    char*       dluName,        /*  IN: Name of DLU */
    UINT32      dluVersion)     /*  IN: Version of DLU */
{
    FILE*               fp;
    DLEDS_RESULT        result = DLEDS_OK;
    TYPE_DLU_HEADER     dluHeader;
    char                fileWithPath[256];
    size_t              read;

    strncpy(fileWithPath, path, sizeof(fileWithPath));
    strcat(fileWithPath, "/");
    strcat(fileWithPath, dluFileName);
    fp = fopen(fileWithPath, "rb");
    if (fp == NULL)
    {
        DebugError1("Could not open DLU file (%s)",
            fileWithPath);
        return DLEDS_ERROR;
    }
    
    /* Retrieve DLU header */
    read = fread(&dluHeader, 1, sizeof(dluHeader), fp);
    if( read != sizeof(dluHeader) )
    {
        /* Could not read header from DLU */
        DebugError3("Could not read DLU header(%s), fread returned %d (Expected = %d)",
            fileWithPath, read, sizeof(dluHeader)); 
        fclose(fp);
        return DLEDS_ERROR;
    }
    fclose(fp); 

    /* Check that name and version in DLU header are the expected */ 
    if ((strcmp(dluHeader.dluName, dluName) != 0) || (ntohl(dluHeader.dluVersion) != dluVersion))
    {
        /* Mismatch between name/version */
        DebugError3("Unexpected values in DLU (%s), name (%s) version (0x%X)",
            fileWithPath, dluHeader.dluName, ntohl(dluHeader.dluVersion)); 
        DebugError3("Expected values in DLU (%s), name (%s) version (0x%X)",
            fileWithPath, dluName, dluVersion); 
        fclose(fp);
        return DLEDS_ERROR;
    }
       
    return result;     
}


/*******************************************************************************
 * 
 * Function name: verifySciNameAndVersion
 *
 * Abstract:      This function verifies that the SCI file exists and contains
 *                the expected name and version.
 *
 * Return value:  DLEDS_OK      - Search successful
 *                DLEDS_ERROR   - Failed to search directory
 *
 * Globals:       -
 */
DLEDS_RESULT verifySciNameAndVersion(
    const char* path,       /*  IN: Directory to search in */
    char*       name,       /*  IN: Name of SCI file */
    char*       version)    /*  IN: Version of SCI file */
{
    DLEDS_RESULT result = DLEDS_OK;

    /* To avoid compiler warning */
    (void)path;
    (void)name;
    (void)version;

    return result;    
}

/*******************************************************************************
 * 
 * Function name: verifySciContent
 *
 * Abstract:      This function verifies that the content of the EDSP Info file
 *                matches the SCI files regarding name and version.
 *
 * Return value:  DLEDS_OK      - Search successful
 *                DLEDS_ERROR   - Failed to search directory
 *
 * Globals:       -
 */
DLEDS_RESULT verifySciContent(
    const char* path,                   /*  IN: Directory to search in */
    EDSP_Info*  pEdspInfo)              /* OUT: Content of EDSP Info file */
{
    DLEDS_RESULT result = DLEDS_OK; 
    UINT32 noOfSci;
    SCI_Info* pSci;
    char sciName[32];
    char sciVersion[32];

    /* Get number of expected SCI files in package */
    noOfSci = pEdspInfo->noOfSci;

    /* Check that all expected SCI files exists and has correct name and version */
    pSci = pEdspInfo->pSciInfo;
    while ((noOfSci > 0) && (result == DLEDS_OK))
    {
        strncpy(sciName, pSci->sciInfoHeader.sciName, sizeof(sciName));
        strncpy(sciVersion, pSci->sciInfoHeader.sciVersion, sizeof(sciVersion));

        /* Search for SCI file <sciName> and extension ".sci2" or ".sci2g" */
        /* and verify that name and version are the expected */
        result = verifySciNameAndVersion(path, sciName, sciVersion);

        noOfSci--;
        pSci = pSci->pNextSciInfo;
    }

    return result;
}


/*******************************************************************************
 * 
 * Function name: verifyDluContent
 *
 * Abstract:      This function verifies that the content of the SCI Info file
 *                matches the DLU files regarding name and version.
 *
 * Return value:  DLEDS_OK      - Search successful
 *                DLEDS_ERROR   - Failed to search directory
 *
 * Globals:       -
 */
DLEDS_RESULT verifyDluContent(
    const char* path,               /*  IN: Directory to search in */
    SCI_Info*  pSciInfo)            /* OUT: Content of EDSP Info file */
{
    DLEDS_RESULT result = DLEDS_OK; 
    UINT32 noOfDlu;
    DLU_Info* pDlu;
    char dluFileName[64];
    char dluName[32];
    UINT32 dluVersion;

    /* Get number of expected DLU files in package */
    noOfDlu = pSciInfo->noOfDlu;

    /* Check that all expected DLU files exists and has correct name and version */
    pDlu = pSciInfo->pDluInfo;
    while ((noOfDlu > 0) && (result == DLEDS_OK))
    {
        strncpy(dluFileName, pDlu->dluInfoHeader.fileName, sizeof(dluFileName));
        strncpy(dluName, pDlu->dluInfoHeader.name, sizeof(dluName));
        dluVersion = toVersion(pDlu->dluInfoHeader.version);

        /* Search for DLU file <dluName> and extension ".dl2" */
        /* and verify that name and version are the expected */
        result = verifyDluNameAndVersion(path, dluFileName, dluName, dluVersion);
        noOfDlu--;
        pDlu = pDlu->pNextDluInfo;
    }

    return result;
}

/*******************************************************************************
 * 
 * Function name: countDluFiles
 *
 * Abstract:      This function count the number of files with suffix ".dl2" in
 *                a directory.
 *
 * Return value:  DLEDS_OK      - Search successful
 *                DLEDS_ERROR   - Failed to search directory
 *
 * Globals:       -
 */
DLEDS_RESULT countDluFiles(
    const char* path,               /*  IN: Directory to search in */
    UINT16* pCounter)               /* OUT: No of files found */
{
    int             len;
    struct dirent*  pDirent;
    DIR*            pDir = opendir(path);
    UINT16          fileCounter = 0;
    DLEDS_RESULT    result = DLEDS_ERROR;

    if (pDir != NULL)
    {
        while ((pDirent = readdir(pDir)) != NULL)
        {
            len = strlen (pDirent->d_name);
            if (len >= 4)
            {
                if (strcmp (".dl2", &(pDirent->d_name[len - 4])) == 0)
                {
                    /* file with suffix .sci2 found */
                    fileCounter++;
                    result = DLEDS_OK;
                }
            }
        }
        closedir (pDir);
    }

    *pCounter = fileCounter;
    return result;
}


/*******************************************************************************
 * 
 * Function name: countSci2Files
 *
 * Abstract:      This function count the number of files with suffix ".sci2" in
 *                a directory.
 *
 * Return value:  DLEDS_OK      - Search successful
 *                DLEDS_ERROR   - Failed to search directory
 *
 * Globals:       -
 */
DLEDS_RESULT countSci2Files(
    const char* path,               /*  IN: Directory to search in */
    UINT16* pCounter)               /* OUT: No of files found */
{
    int             len;
    struct dirent*  pDirent;
    DIR*            pDir = opendir(path);
    UINT16          fileCounter = 0;
    DLEDS_RESULT    result = DLEDS_ERROR;

    if (pDir != NULL)
    {
        while ((pDirent = readdir(pDir)) != NULL)
        {
            len = strlen (pDirent->d_name);
            if (len >= 5)
            {
                if (strcmp (".sci2", &(pDirent->d_name[len - 5])) == 0)
                {
                    /* file with suffix .sci2 found */
                    fileCounter++;
                    result = DLEDS_OK;
                }
                else if (strcmp (".sci2g", &(pDirent->d_name[len - 6])) == 0)
                {
                    /* file with suffix .sci2g found */
                    fileCounter++;
                    result = DLEDS_OK;
                }
            }
        }
        closedir (pDir);
    }

    *pCounter = fileCounter;
    return result;
}


/*******************************************************************************
 * 
 * Function name: removeDirectory
 *
 * Abstract:      This function removes a directory and all it's sub directories.
 *
 * Return value:  0      - All directories removed successfully
 *                others - Failed to remove directories
 *
 * Globals:       -
 */
int removeDirectory(
    const char* path)                   /* IN: Directory to remove */
{
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;
    
    if (d)
    {
        struct dirent* p;
        r = 0;
        while (!r && (p=readdir(d)))
        {
            int r2 = -1;
            char* buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
            {
                continue;
            }
            len = path_len + strlen(p->d_name) + 2;
            buf = malloc(len);
            if (buf)
            {
                struct stat statbuf;
                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf))
                {
                    if( S_ISDIR(statbuf.st_mode))
                    {
                        r2 = removeDirectory(buf);
                    }
                    else
                    {
                        r2 = remove(buf);
                    }
                }
                free(buf);
            }
            r = r2;
        }
        
        closedir(d);
    }
    if (!r)
    {
        r = remove(path);
    }
    return r;
}


/*******************************************************************************
 * 
 * Function name: dledsDluInstall2
 *
 * Abstract:      This function installs a DLU on target.
 *
 * Return value:  DLEDS_OK      - Information from file has been read
 *                DLEDS_ERROR   - No information read from file
 *
 * Globals:       -
 */
static DLEDS_RESULT dledsDluInstall2(
    TYPE_DLEDS_DLU_DATA2* pDluData)      /* IN: DLU data */
{
    DLEDS_RESULT            result = DLEDS_ERROR;

    /******************************************************************************
    Install payload data from DLU file on target
    ******************************************************************************/
    /* Call function that contains Platform Specific code */
    result = dledsPlatformInstallDlu2(pDluData);
    
    DebugError3("***PLATFORM: DLU file (%s) installed in directory (%s) on platform, result (%d)", pDluData->dluFileName, pDluData->dluFilePath, result);
            
    return result;
}

/*******************************************************************************
 * 
 * Function name: dledsDluInstall
 *
 * Abstract:      This function installs a DLU on target.
 *
 * Return value:  DLEDS_OK      - Information from file has been read
 *                DLEDS_ERROR   - No information read from file
 *
 * Globals:       -
 */
static DLEDS_RESULT dledsDluInstall(
    TYPE_DLEDS_DLU_DATA* pDluData)      /* IN: DLU data */
{
    DLEDS_RESULT            result = DLEDS_ERROR;

    /******************************************************************************
    Install payload data from DLU file on target
    ******************************************************************************/
    /* Call function that contains Platform Specific code */
    result = dledsPlatformInstallDlu(pDluData);
    
    DebugError3("***PLATFORM: DLU file (%s) installed in directory (%s) on platform, result (%d)", pDluData->dluFileName, pDluData->dluFilePath, result);
            
    return result;
}

/*******************************************************************************
 * 
 * Function name: dledsUnpackSCI2
 *
 * Abstract:      This function unpacks an SCI vesion 2 to requested directory. 
 *                Information neccessary for installation of the slave DLU's
 *                are written to the installation information file.
 *
 * Return value:  DLEDS_OK      - SCI has been unpacked
 *                DLEDS_ERROR   - Failed to unpack SCI
 *
 * Globals:       currentDir
 */
DLEDS_RESULT dledsUnpackSCI2(
    char* tarFile,                      /* IN: TAR file containing SCI to be installed */
    char* subDirectoryName,             /* IN: Sub directory where SCI TAR file will be unpacked */
    SCI_InfoHeader* pSciInfoHeader,     /* IN: Pointer to SCI info read from the EDSP_Info.xml file */
    UINT8 compressed,                   /* IN: 1 = TAR file is compressed, 0 = uncompressed */
    EDSP_InfoHeader* pEdspInfoHeader,   /* IN: Pointer to EDSP info header */
    UINT32 edspCrc32)                   /* IN: The CRC32 of the ED package */
{
    DLEDS_RESULT            result = DLEDS_OK;
    FILE*                   fp;
    char                    destinationDir[256];
    char                    fileWithPath[256];
    char                    dluFile[64];
    char                    infoFile[256];
    SCI_Info                sciInfo;
    TYPE_DLEDS_DLU_DATA2    dluData;
    UINT32                  sciCrc32;
    UINT32                  sciInfoCrc32;
    UINT16                  noDluFiles;
    DLU_Info*               pCurrentDluInfo;
    char                    sciName[32];
    char*                   pos;

    /* Syntax for tarFile:  <SCI_Name>.sci2(g) */
    /* Unpack TAR file */
    strcpy(fileWithPath, currentDir);
    strcat(fileWithPath,"/");
    strcat(fileWithPath,tarFile);
    strcpy(destinationDir, currentDir); 
    strcat(destinationDir,"/");
    strcat(destinationDir, subDirectoryName);

    /* Change current directory to sub directory */
    result = downCurrentDirectory(subDirectoryName);
    if (result == DLEDS_ERROR)
    {
        setRootDirectory();
        return DLEDS_ERROR;
    }

    if (unpackTarFile(fileWithPath, destinationDir, compressed) != DLEDS_OK)
    {
        setRootDirectory();
        return DLEDS_ERROR;
    }

    /* Extract SCI name from file name:  <SCI_Name>.sci2g */
    pos = strchr(tarFile, '.');
    strncpy(sciName, tarFile, pos - tarFile);
    sciName[pos - tarFile] = '\0';

    /* Check that file SCI.crc exists */
    strcpy(infoFile, currentDir);
    strcat(infoFile, "/");
    strcat(infoFile, "SCI.crc");
    fp = fopen(infoFile, "rb");
    if (fp == NULL)
    {
        DebugError1("Could not open SCI.crc file (%s)", infoFile);
        setRootDirectory();
        return DLEDS_ERROR;
    }
    /* Read CRC32 value from file SCI.crc */
    if (fscanf(fp,"%x", &sciCrc32) != 1)
    {
        DebugError1("Could not read CRC32 from SCI.crc file (%s)", infoFile);
        setRootDirectory();
        fclose(fp);
        return DLEDS_ERROR;
    }
    fclose(fp);

    /* Check that file SCI_Info.crc exists */
    strcpy(infoFile, currentDir);
    strcat(infoFile, "/");
    strcat(infoFile, "SCI_Info.crc");
    fp = fopen(infoFile, "rb");
    if (fp == NULL)
    {
        DebugError1("Could not open SCI_Info.crc file (%s)", infoFile);
        setRootDirectory();
        return DLEDS_ERROR;
    }
    /* Read CRC32 value from file SCI_Info.crc */
    if (fscanf(fp,"%x", &sciInfoCrc32) != 1)
    {
        DebugError1("Could not read CRC32 from SCI_Info.crc file (%s)", infoFile);
        setRootDirectory();
        fclose(fp);
        return DLEDS_ERROR;
    }
    fclose(fp);

    /* Check that file SCI_Info.xml exists */
    strcpy(infoFile, currentDir);
    strcat(infoFile, "/");
    strcat(infoFile, "SCI_Info.xml");
    fp = fopen(infoFile, "rb");
    if (fp == NULL)
    {
        DebugError1("Could not open SCI_Info.xml file (%s)", infoFile);
        setRootDirectory();
        return DLEDS_ERROR;
    }
    fclose(fp);

    /* Verify that SCI_Info.xml is valid 
       by calculating and checking CRC32 against expected value */
    result = dleds_checkCRC(infoFile, sciInfoCrc32, 0xFFFFFFFF);
    if (result != DLEDS_OK)
    {
        DebugError1("Incorrect CRC32 for SCI_Info file (%s)", infoFile);
        setRootDirectory();
        return DLEDS_ERROR;
    }

    /* Read data from SCI Info file */
    memset(&sciInfo,0,sizeof(sciInfo));
    result = dledsRetrieveCompleteSciInfo(infoFile, &sciInfo);
    if (result != DLEDS_OK)
    {
        DebugError1("Could not retrieve complete information from SCI info file (%s)", infoFile);
        setRootDirectory();
        return DLEDS_ERROR;
    }

    /* Check that SCI name in file match the file name */
    if (strcmp(sciName, sciInfo.sciInfoHeader.sciName) != 0)
    {
        DebugError2("File name (%s) does not match SCI name (%s) in SCI_Info.xml", sciName, sciInfo.sciInfoHeader.sciName);
        setRootDirectory();
        return DLEDS_ERROR;
    }

    /* Verify that content in SCI info match the one in EDSP info */
    if ((strcmp(sciInfo.sciInfoHeader.sciName, pSciInfoHeader->sciName) != 0) &&
        (strcmp(sciInfo.sciInfoHeader.sciVersion, pSciInfoHeader->sciVersion) != 0) &&
        (strcmp(sciInfo.sciInfoHeader.sciDeviceType, pSciInfoHeader->sciDeviceType) != 0))
    {
        DebugError1("Mismatch in SCI information between EDSP_Info file and SCI_Info file (%s)", infoFile);
        setRootDirectory();
        return DLEDS_ERROR;
    }

    /* Count number of DLU files in SCI Package */
    result = countDluFiles(currentDir, &noDluFiles);
    if (result != DLEDS_OK)
    {
        DebugError1("Could not count number of DLU files in SCI file (%s)", currentDir);
        setRootDirectory();
        return DLEDS_ERROR;
    }

    /* Check against expected number of DLU files */
    if (noDluFiles > sciInfo.noOfDlu)
    {
        /* To many DLU files compared to DLU list */
        DebugError2("Number of DLUs in SCI (%u) exceeds expected (%u)", noDluFiles, sciInfo.noOfDlu);
        setRootDirectory();
        return DLEDS_ERROR;
    }
    else if (noDluFiles < sciInfo.noOfDlu)
    {
        /* Missing DLU files compared to DLU list */
        DebugError2("Number of DLUs in SCI (%u) less than expected (%u)", noDluFiles, sciInfo.noOfDlu);
        setRootDirectory();
        return DLEDS_ERROR;
    }
    else
    {
        /* Correct number of DLU files in SCI package */

        /* ToDo: Verify that the SCI_Info file contains the correct information
           regarding name and version of DLUs */
        result = verifyDluContent(currentDir, &sciInfo);
        if (result != DLEDS_OK)
        {
            setRootDirectory();
            return DLEDS_ERROR;
        } 
        
        /* Store EDSP and SCI information for later installation of DLUs */
        strncpy(dluData.dluFilePath, currentDir, sizeof(dluData.dluFilePath));
        strncpy(dluData.edPackageName, pEdspInfoHeader->edspName, sizeof(dluData.edPackageName));
        /* Convert version string to UINT32 */
        dluData.edPackageVersion = toVersion(pEdspInfoHeader->edspVersion);
        strncpy(dluData.sciName, sciInfo.sciInfoHeader.sciName, sizeof(dluData.sciName));
        /* Convert version string to UINT32 */
        dluData.sciVersion = toVersion(sciInfo.sciInfoHeader.sciVersion);
        strncpy(dluData.packageSource, pEdspInfoHeader->edspSource, sizeof(dluData.packageSource));
        dluData.edCrc = edspCrc32;
        strncpy(dluData.edInfo, pEdspInfoHeader->info, sizeof(dluData.edInfo));
        strncpy(dluData.edTimeStamp, pEdspInfoHeader->created, sizeof(dluData.edTimeStamp));
        dluData.sciCrc = sciCrc32;
        strncpy(dluData.sciInfo, sciInfo.sciInfoHeader.info, sizeof(dluData.sciInfo));
        strncpy(dluData.sciTimestamp, sciInfo.sciInfoHeader.created, sizeof(dluData.sciTimestamp));
    
/*---------------------------------------------------------------------*/
        /* Retrieve first slave DLU name from list*/
        if ((noDluFiles > 0) && (sciInfo.pDluInfo != NULL))
        {
            pCurrentDluInfo = sciInfo.pDluInfo;       
        }
        else
        {
            /* Missing DLU files compared to DLU list */
            DebugError2("No DLUs in SCI (%u) or NULL pointer to first DLU Info (0x%X)", noDluFiles, sciInfo.pDluInfo);
            setRootDirectory();
            return DLEDS_ERROR;
        }

        /* Store information about all DLU files from the SCI package */
        /* This information is later used for installation */
        while ((noDluFiles > 0) && (pCurrentDluInfo != NULL))
        {
            strncpy(dluFile,pCurrentDluInfo->dluInfoHeader.fileName, sizeof(dluFile));
            strncpy(fileWithPath, currentDir, sizeof(fileWithPath));
            strcat(fileWithPath, "/");
            strcat(fileWithPath, dluFile);
            /* Check that DLU file exists */
            fp = fopen(fileWithPath,"r");
            if (fp == NULL)
            {                                                                                                                        
                DebugError1("Expected DLU file not found (%s)",
                    fileWithPath);
                setRootDirectory();
                return DLEDS_ERROR;   
            }
            fclose(fp);
                      
            /*  Structure dluData is already partly filled in when SCI Master DLU was processed */        
            strncpy(dluData.dluFileName, dluFile, sizeof(dluData.dluFileName));
        
            result = dledsWriteInstallationEntry2(DLEDS_INSTALLATION_FILE, &dluData);
            if (result != DLEDS_OK)
            {
                setRootDirectory();
                return DLEDS_ERROR;
            }

            /* Increment global variable counting total number of DLUs in EDSP */
            totalNoOfDLU++;

            /* prepare for next DLU */
            noDluFiles--;
            pCurrentDluInfo = pCurrentDluInfo->pNextDluInfo; 
            if ((noDluFiles > 0) && (pCurrentDluInfo == NULL)) 
            {
                DebugError1("Illegal pointer to DLU Info, No of DLUs left to check (%u)",
                    noDluFiles);
                setRootDirectory();
                return DLEDS_ERROR;   
            }     
        
        } /* while DLU files available in SCI package */
    }

    result = upCurrentDirectory();
    return result;
}

/*******************************************************************************
 * 
 * Function name: dledsUnpackSCI
 *
 * Abstract:      This function unpacks an SCI to requested directory. Information
 *                neccessary for installation of the SCI MASTER and the slave DLU's
 *                are written to the installation information file.
 *
 * Return value:  DLEDS_OK      - SCI has been unpacked
 *                DLEDS_ERROR   - Failed to unpack SCI
 *
 * Globals:       currentDir
 */
DLEDS_RESULT dledsUnpackSCI(
    char* tarFile,                      /* IN: TAR file containing SCI to be installed */
    char* subDirectoryName,             /* IN: Sub directory where TAR file will be unpacked */
    UINT8 compressed,                   /* IN: 1 = TAR file is compressed, 0 = uncompressed */
    char* source,                       /* IN: The SCI is part of this Source (BT or CU) */
    char* edPackageName,                /* IN: The SCI is part of this ED sub package */
    UINT32 edPackageVersion )           /* IN: The version of the ED sub package */ 
{
    DLEDS_RESULT result = DLEDS_OK;
    TYPE_DLU_HEADER dluHeader;
    size_t  read;
    size_t  write;
    FILE*   fp;
    char    dluName[64];
    char    instFile[256];
    char    destinationDir[256];
    char    fileWithPath[256];
    char    sciName[64];
    char    sciMasterDluFile[64];
    char    dluFile[64];
    UINT32  expectedDluVersion;
    UINT32  expectedDluTypeCode;
    char    expectedDluName[64];
    UINT32  dluFileClass;
    TYPE_DLU_TPATH_HEADER tpathHeader;
    char*   pos;
    char*   nextPos;
    void*   pPayloadBuffer;
    char    level[16];
    UINT32  noTarFiles;
    char*   pListTarFileName;
    UINT32  noDluFiles;
    char*   pListDluFileName;
    char*   pNewListDluFileName;
    TYPE_DLEDS_DLU_DATA dluData;

    /* Syntax for tarFile:  <Function>_<Subfunction>_<Device Type>_<source>.sci(g) */
    /* Unpack TAR file */
    strcpy(fileWithPath, currentDir);
    strcat(fileWithPath,"/");
    strcat(fileWithPath,tarFile);
    strcpy(destinationDir, currentDir); 
    strcat(destinationDir,"/");
    strcat(destinationDir, subDirectoryName);

    /* Change current directory to sub directory */
    result = downCurrentDirectory(subDirectoryName);
    if (result == DLEDS_ERROR)
    {
        setRootDirectory();
        return DLEDS_ERROR;
    }

    if (unpackTarFile(fileWithPath, destinationDir, compressed) != DLEDS_OK)
    {
        setRootDirectory();
        return DLEDS_ERROR;
    }
            
    /* Check DLU header for installation file (INST_Structure) */
    /* Check that file INST_<Function>_<Subfunction>_<Device Type>_<source>.dl2 exists */
    pos = strchr(tarFile,'.');
    strncpy(sciName,tarFile,pos - tarFile);
    sciName[pos - tarFile] = '\0';
    strcpy(instFile, currentDir);
    strcat(instFile, "/INST_");
    strcat(instFile, sciName);
    strcat(instFile, ".dl2");

    fp = fopen(instFile, "rb");
    if (fp == NULL)
    {
        /* Check if INST is in lower case */
        strcpy(instFile, currentDir);
        strcat(instFile, "/inst_");
        strcat(instFile, sciName);
        strcat(instFile, ".dl2");
        fp = fopen(instFile, "rb");
        if (fp == NULL)
        {
            DebugError1("Could not open SCI Installation file (%s)",                             
                instFile);
            return DLEDS_ERROR;
        }
    }
    
    /* Retrieve DLU header from extracted file INST_<Function>_<Subfunction>_<Device Type>_<source>.dl2 */
    read = fread(&dluHeader, 1, sizeof(dluHeader), fp);
    if( read != sizeof(dluHeader) )
    {
        /* Could not read header from DLU */
        DebugError3("Could not read DLU header from SCI installation file (%s), fread returned %d (Expected = %d)",
            instFile, read, sizeof(dluHeader)); 
        fclose(fp);
        return DLEDS_ERROR;
    }

    expectedDluVersion = 0x00000000;    /* No valid version in DLU header for ED package (BT and CU version in file name) */
    expectedDluTypeCode = 0x03000002;    /* DLU_TYPE_EDDC_INST_STRUCTURE */
    strcpy(expectedDluName, "INST_");
    strcat(expectedDluName, sciName);
    result = verifyDluHeader(&dluHeader, expectedDluVersion, expectedDluTypeCode, expectedDluName);
    if (result == DLEDS_ERROR)
    {
        fclose(fp);
        return DLEDS_ERROR;
    }

    /* Check payload data for installation file (INST_Structure)  */
    /* Check if there is any Target Path Header in DLU */
    dluFileClass = (ntohl(dluHeader.dluTypeCode) & DLU_FILE_CLASS_MASK) >> 24;
    if (dluFileClass == DLU_FILE_CLASS_DLU_EXT)
    {
        /*  Yes we have an extended DLU header */
        read =  fread(&tpathHeader,1,sizeof(tpathHeader), fp);
        if (read != sizeof(tpathHeader))
        {
            /* Could not read extended DLU header */
            DebugError1("Could not read TPATH in DLU header extension in (%s)", instFile);
            fclose(fp);
            return DLEDS_ERROR;
        }
    }
    /* Read payload data in XML format */
    pPayloadBuffer = malloc(ntohl(dluHeader.dluDataSize));
    if (pPayloadBuffer == NULL)
    {
        DebugError1("Could not allocate memory for Payload buffer (size= %d)",
            ntohl(dluHeader.dluDataSize));
        fclose(fp);
        return DLEDS_ERROR;        
    }
    read =  fread(pPayloadBuffer,1,ntohl(dluHeader.dluDataSize), fp);
    if (read != ntohl(dluHeader.dluDataSize))
    {
        DebugError1("Could not read payload data from (%s)", instFile);
        free(pPayloadBuffer);
        fclose(fp);
        return DLEDS_ERROR;        
    }
    fclose(fp);
    remove(instFile);
    
    /* Copy payload XML data to file to be able to use iptcom XML package */
    fp = fopen ( DLEDS_XML_TMP_FILE , "wb" );
    if (fp == NULL)
    {
        DebugError1("Could not open temporary file (%s)", DLEDS_XML_TMP_FILE);                             
        free(pPayloadBuffer);
        return DLEDS_ERROR;
    }
    write = fwrite (pPayloadBuffer , 1 , ntohl(dluHeader.dluDataSize) , fp );
    free(pPayloadBuffer);        
    fclose (fp);
    if (write != ntohl(dluHeader.dluDataSize))
    {
        DebugError3("Failed to write to (%s), fwrite returned %d (Expected = %d)",
            DLEDS_XML_TMP_FILE, write, ntohl(dluHeader.dluDataSize)); 
        return DLEDS_ERROR;        
    }
    
    /* Read value from XML tag "Level" in payloadData */
    pListTarFileName = NULL;
    pListDluFileName = NULL;
    if ((result = readDataFromInstStructure(
                DLEDS_XML_TMP_FILE,
                level,
                &noTarFiles, &pListTarFileName, 
                &noDluFiles, &pListDluFileName)) != DLEDS_OK)
    {
        DebugError1("Failed to retrieve XML data from SCI INST_struct file (%s)",
            instFile);
        if (pListTarFileName != NULL)
        {
            free(pListTarFileName);
        }
        if (pListDluFileName != NULL)
        {
            free(pListDluFileName);
        }
        return DLEDS_ERROR;                    
    }
    
    if ((strcmp(level,"SCI") != 0) || (noTarFiles != 0) || (noDluFiles == 0))
    {   
        DebugError1("Error in SCI INST_struct file (%s)", instFile);   
        if (pListTarFileName != NULL)
        {
            free(pListTarFileName);
        }
        if (pListDluFileName != NULL)
        {
            free(pListDluFileName);
        }
        return DLEDS_ERROR;                    
    }
    
    /* Find SCI Master DLU and retrieve information about SCI name and SCI version */
    /* from the DLU header. This information is nesseccary when processing the slave DLU's */
    if ((pos = strstr(pListDluFileName, sciName)) == NULL)
    {
        DebugError2("Could not find MDLU_SCI (%s) in INST_struct file (%s)",
            sciName, instFile);
        if (pListTarFileName != NULL)
        {
            free(pListTarFileName);
        }
        if (pListDluFileName != NULL)
        {
            free(pListDluFileName);
        }
        return DLEDS_ERROR;                            
    }
    strcpy(sciMasterDluFile, sciName);
    strcat(sciMasterDluFile, ".dl2");
    strcpy(fileWithPath, currentDir);
    strcat(fileWithPath, "/");
    strcat(fileWithPath, sciMasterDluFile);
    fp = fopen(fileWithPath, "rb");
    if (fp == NULL)
    {
        DebugError1("Could not open SCI MDLU file (%s)",
            fileWithPath);
        if (pListTarFileName != NULL)
        {
            free(pListTarFileName);
        }
        if (pListDluFileName != NULL)
        {
            free(pListDluFileName);
        }
        return DLEDS_ERROR;
    }
    
    /* Retrieve DLU header from SCI Master DLU file */
    read = fread(&dluHeader, 1, sizeof(dluHeader), fp);
    if( read != sizeof(dluHeader) )
    {
        /* Could not read header from DLU */
        DebugError2("Could not read DLU header from SCI MDLU file, fread returned %d (Expected = %d)",
            read, sizeof(dluHeader)); 
        fclose(fp);
        if (pListTarFileName != NULL)
        {
            free(pListTarFileName);
        }
        if (pListDluFileName != NULL)
        {
            free(pListDluFileName);
        }
        return DLEDS_ERROR;
    }
        
    /* Store information for later installation */
    strcpy(dluData.dluFilePath, currentDir);
    strcpy(dluData.dluFileName, sciMasterDluFile);
    strcpy(dluData.edPackageName,edPackageName);
    dluData.edPackageVersion = edPackageVersion;
    strcpy(dluData.sciName,dluHeader.dluName);
    dluData.sciVersion = ntohl(dluHeader.dluVersion);
    strcpy(dluData.packageSource,source);
    
    result = dledsWriteInstallationEntry(DLEDS_INSTALLATION_FILE, &dluData);
    if (result != DLEDS_OK)
    {
        if (pListTarFileName != NULL)
        {
            free(pListTarFileName);
        }
        if (pListDluFileName != NULL)
        {
            free(pListDluFileName);
        }
        return DLEDS_ERROR;
    }
    
    /* Remove SCI master DLU name from DLU file list */
    noDluFiles--;
    /* Check if any other DLU names left in list */
    /* NOTE: pos is already pointing at start of Master DLU name */
    if (noDluFiles > 0)
    {
        /* Check if there are DLU's after SCI Master DLU in DLU list */
        if ((nextPos = strchr(pos,',')) == NULL)
        {
            /* SCI Master DLU is last in list, and at least one entry ahead in list */
            /* Set new end of string before Master DLU name */
            pListDluFileName[pos - pListDluFileName-1] = '\0';
        }
        else
        {
            /* SCI Master DLU is first or in the middle of the DLU list */
            /* Replace SCI Master DLU with string starting at first entry after Master DLU */
            strcpy(pos,nextPos+1);
        }
        
    }
    
    /* Retrieve first slave DLU name from list*/
    if (noDluFiles > 1)
    {       
        pos = strchr(pListDluFileName,',');
        strncpy(dluName,pListDluFileName,pos - pListDluFileName);
        dluName[pos - pListDluFileName] = '\0';
    }
    else
    {
        strcpy(dluName,pListDluFileName);
    }

    /* Store information about all DLU files from the SCI package */
    /* This information is later used for installation */
    while (noDluFiles > 0)
    {
        strcpy(dluFile,dluName);
        strcat(dluFile,".dl2");
        strcpy(fileWithPath, currentDir);
        strcat(fileWithPath, "/");
        strcat(fileWithPath, dluFile);
        /* Check that DLU file exists */
        fp = fopen(fileWithPath,"r");
        if (fp == NULL)
        {
            DebugError1("Expected DLU file not found (%s)",
                fileWithPath);
            if (pListTarFileName != NULL)
            {
                free(pListTarFileName);
            }
            if (pListDluFileName != NULL)
            {
                free(pListDluFileName);
            }
            return DLEDS_ERROR;   
        }
        fclose(fp);
                      
        /*  Structure dluData is already partly filled in when SCI Master DLU was processed */        
        strcpy(dluData.dluFileName, dluFile);
        strcpy(dluData.packageSource,source);
        
        result = dledsWriteInstallationEntry(DLEDS_INSTALLATION_FILE, &dluData);
        if (result != DLEDS_OK)
        {
            if (pListTarFileName != NULL)
            {
                free(pListTarFileName);
            }
            if (pListDluFileName != NULL)
            {
                free(pListDluFileName);
            }
            return DLEDS_ERROR;
        }
        noDluFiles--;
        
        /* Check if any DLU file left */
        if (noDluFiles > 0)
        {
            pNewListDluFileName = pos+1;
            /* Retrieve next DLU name from list*/
            if (noDluFiles > 1)
            {       
                pos = strchr(pNewListDluFileName,',');
                strncpy(dluName,pNewListDluFileName,pos - pNewListDluFileName);
                dluName[pos - pNewListDluFileName] = '\0';
            }   
            else
            {
                strcpy(dluName,pNewListDluFileName);
            }
        }
        
    } /* while DLU files available in SCI package */

    result = upCurrentDirectory();
    return result;
}

/*******************************************************************************
 * 
 * Function name: dledsUnpackEDSubPackage
 *
 * Abstract:      This function unpacks the ED sub package. Information
 *                neccessary for installation of the MASTER DLU is written
 *                to the installation information file. For each SCI a call is
 *                made for the SCI unpacking function.
 *
 * Return value:  DLEDS_OK        - ED sub package has been unpacked
 *                DLEDS_SCI_ERROR - Failed to unpack SCI
 *                DLEDS_ERROR     - Failed to unpack ED sub package
 *
 * Globals:       currentDir
 */
DLEDS_RESULT dledsUnpackEDSubPackage(
    char* fileName,             /* IN: Name of TAR file containing ED sub package to be installed */
    char* subDirectoryName)     /* IN: Sub directory where TAR file will be unpacked */
{
    DLEDS_RESULT result = DLEDS_OK;
    char    instFile[256];
    char    mdluEdFile[256];
    char    fileWithPath[256];
    char    destinationDir[256];
    char*   pos;
    char    EDtypeSource[32];
    FILE*   fp;
    char    dirName[256];
    char    sciFile[64];
    UINT8   sciCompressed;
    TYPE_DLU_HEADER dluHeader;
    TYPE_DLU_TPATH_HEADER tpathHeader;
    size_t  read;
    size_t  write;
    UINT32  expectedDluVersion;
    UINT32  expectedDluTypeCode;
    char    expectedDluName[32];
    UINT32  dluFileClass;
    void*   pPayloadBuffer;
    char    level[16];         
    char    source[16];
    UINT32  noSciFiles;         
    char*   pListSciFileName;   
    char*   pNewListSciFileName;
    UINT32  noMdluEdFiles;      
    char*   pListMdluEdFileName;
    int     clearNVRAM;         
    int     clearFFS;
    TYPE_DLEDS_DLU_DATA dluData;
    
    /* Syntax for FileName:  <ED type>_<source>_<version>.edsp */
    /* Files are extracted to the user root directory of the file system */
    /* TAR file is removed after it has been unpacked */
    strcpy(fileWithPath, currentDir);
    strcat(fileWithPath,"/");
    strcat(fileWithPath,fileName);
    strcpy(destinationDir, currentDir); 
    strcat(destinationDir,"/");
    strcat(destinationDir, subDirectoryName);

    /* Change current DLEDS directory to sub directory */
    result = downCurrentDirectory(subDirectoryName);

    if (unpackTarFile(fileWithPath, destinationDir, 0) != DLEDS_OK)
    {
        setRootDirectory();
        return DLEDS_ERROR;
    }
            
    /* Check DLU header for  installation file (INST_ED) */
    /* Check that file INST_<ED type>_<source>.dl2 exists */
    pos = strrchr(fileName,'_');
    strcpy(EDtypeSource,"");
    strncat(EDtypeSource, fileName, pos - fileName);
    EDtypeSource[pos - fileName] = '\0';
    strcpy(instFile, currentDir);
    strcat(instFile, "/INST_");
    strcat(instFile, EDtypeSource);
    strcat(instFile, ".dl2");
    
    fp = fopen(instFile, "rb");
    if (fp == NULL)
    {
        /* Check if INST is in lower case */
        strcpy(instFile, currentDir);
        strcat(instFile, "/inst_");
        strcat(instFile, EDtypeSource);
        strcat(instFile, ".dl2");
        fp = fopen(instFile, "rb");
        if (fp == NULL)
        {
            DebugError1("Could not open Installation file (%s)",                             
                instFile);
            return DLEDS_ERROR;
        }
    }
    
    /* Retrieve DLU header from extracted file INST_<ED type>_<source>.dl2 */
    read = fread(&dluHeader, 1, sizeof(dluHeader), fp);
    if( read != sizeof(dluHeader) )
    {
        /* Could not read header from DLU */
        DebugError3("Could not read DLU header from installation file (%s), fread returned %d (Expected = %d)",
            instFile, read, sizeof(dluHeader)); 
        fclose(fp);
        return DLEDS_ERROR;
    }

    expectedDluVersion = 0x00000000;    /* No valid version in DLU header for ED package (BT and CU version in file name) */
    expectedDluTypeCode = 0x03000003;    /* DLU_TYPE_EDDC_INST_ED */
    strcpy(expectedDluName, "INST_");
    strcat(expectedDluName, EDtypeSource);
    result = verifyDluHeader(&dluHeader, expectedDluVersion, expectedDluTypeCode, expectedDluName);
    if (result == DLEDS_ERROR)
    {
         fclose(fp);
         return DLEDS_ERROR;
    }
    
    /* Check payload data for installation file (INST_Structure)  */
    /* Check if there is any Target Path Header in DLU */
    dluFileClass = (ntohl(dluHeader.dluTypeCode) & DLU_FILE_CLASS_MASK) >> 24;
    if (dluFileClass == DLU_FILE_CLASS_DLU_EXT)
    {
        /*  Yes we have an extended DLU header */
        read =  fread(&tpathHeader,1,sizeof(tpathHeader), fp);
        if (read != sizeof(tpathHeader))
        {
            /* Could not read extended DLU header */
            DebugError1("Could not read TPATH in DLU header extension in (%s)", instFile);
            fclose(fp);
            return DLEDS_ERROR;
        }
    }
    /* Read payload data in XML format */
    pPayloadBuffer = malloc(ntohl(dluHeader.dluDataSize));
    if (pPayloadBuffer == NULL)
    {
        DebugError1("Could not allocate memory for Payload buffer (size= %d)",
            ntohl(dluHeader.dluDataSize));
        fclose(fp);
        return DLEDS_ERROR;        
    }
    read =  fread(pPayloadBuffer,1,ntohl(dluHeader.dluDataSize), fp);
    if (read != ntohl(dluHeader.dluDataSize))
    {
        DebugError1("Could not read payload data from (%s)", instFile);
        free(pPayloadBuffer);
        fclose(fp);
        return DLEDS_ERROR;        
    }
    fclose(fp);
    remove(instFile);
    
    /* Copy payload XML data to file to be able to use iptcom XML package */
    fp = fopen ( DLEDS_XML_TMP_FILE , "wb" );
    if (fp == NULL)
    {
        DebugError1("Could not open temporary file (%s)", DLEDS_XML_TMP_FILE);                             
        free(pPayloadBuffer);
        return DLEDS_ERROR;
    }
    write = fwrite (pPayloadBuffer , 1 , ntohl(dluHeader.dluDataSize) , fp );
    free(pPayloadBuffer);        
    fclose (fp);
    if (write != ntohl(dluHeader.dluDataSize))
    {
        DebugError3("Failed to write to (%s), fwrite returned %d (Expected = %d)",
            DLEDS_XML_TMP_FILE, write, ntohl(dluHeader.dluDataSize)); 
        return DLEDS_ERROR;        
    }
        
    /* Read value from XML tag "Level" in payloadData */
    pListSciFileName = NULL;
    pListMdluEdFileName = NULL;
    if ((result = readDataFromInstEd(
                DLEDS_XML_TMP_FILE,
                level,
                source,
                &noSciFiles,
                &pListSciFileName,
                &noMdluEdFiles,
                &pListMdluEdFileName,
                &clearNVRAM,
                &clearFFS)) != DLEDS_OK)
    {
        DebugError1("Failed to retrieve XML data from sub package INST_ED file (%s)",
            instFile);
        if (pListSciFileName != NULL)
        {
            free(pListSciFileName);
        }
        if (pListMdluEdFileName != NULL)
        {
            free(pListMdluEdFileName);
        }
        return DLEDS_ERROR;                    
    }

    if ((strcmp(level,"EDSubPackage") != 0) ||
        ((strcmp(source,"BT") != 0) && (strcmp(source,"CU") != 0)) ||
        (noSciFiles == 0) || 
        (noMdluEdFiles != 1))
    {
        DebugError1("Error in sub package INST_ED file (%s)", instFile);   
        if (pListSciFileName != NULL)
        {
            free(pListSciFileName);
        }
        if (pListMdluEdFileName != NULL)
        {
            free(pListMdluEdFileName);
        }
        return DLEDS_ERROR;                    
    }

    /* Store information about Master DLU file from this package */
    /* This information is later used during installation */
    /* Check DLU header for  Master DLU file (MDLU_ED) */
    /* Check that file <ED type>_<source>.dl2 exists */
    strcpy(mdluEdFile, pListMdluEdFileName);
    strcat(mdluEdFile,".dl2");
    strcpy(fileWithPath, currentDir);
    strcat(fileWithPath, "/");
    strcat(fileWithPath,mdluEdFile);
    fp = fopen(fileWithPath, "rb");
    if (fp == NULL)
    {
        DebugError1("failed to open MDLU file (%s)",
            fileWithPath);   
        if (pListSciFileName != NULL)
        {
            free(pListSciFileName);
        }
        if (pListMdluEdFileName != NULL)
        {
            free(pListMdluEdFileName);
        }
        return DLEDS_ERROR;
    }
    
    /* Retrieve DLU header from extracted MDLU file <ED type>_<source>.dl2 */
    read = fread(&dluHeader, 1, sizeof(dluHeader), fp);
    fclose(fp);
    if( read != sizeof(dluHeader) )
    {
        /* Could not read header from DLU */
        DebugError2("Could not read DLU header from MDLU file, fread returned %d (Expected = %d)",
            read, sizeof(dluHeader)); 
        if (pListSciFileName != NULL)
        {
            free(pListSciFileName);
        }
        if (pListMdluEdFileName != NULL)
        {
            free(pListMdluEdFileName);
        }
        return DLEDS_ERROR;
    }
       
    /* save information to be used for clean up */
    if ( strcmp(source, "BT") == 0 )
    {
        strcpy( installedBtPackageName, dluHeader.dluName);
        installedBtPackageVersion = ntohl(dluHeader.dluVersion);
    }
    else
    {
        if ( strcmp(source, "CU") == 0 )
        {
            strcpy( installedCuPackageName, dluHeader.dluName);
            installedCuPackageVersion = ntohl(dluHeader.dluVersion);
        }
    }
    
    
    /* Store information for later installation */
    strcpy(dluData.dluFilePath, currentDir);
    strcpy(dluData.dluFileName, mdluEdFile);
    strcpy(dluData.edPackageName,dluHeader.dluName);
    dluData.edPackageVersion = ntohl(dluHeader.dluVersion);
    strcpy(dluData.sciName,"");         /* Master DLU ED doesn't belong to any SCI */
    dluData.sciVersion = 0x00000000;    /* Master DLU ED doesn't belong to any SCI */
    strcpy(dluData.packageSource,source);
    
    result = dledsWriteInstallationEntry(DLEDS_INSTALLATION_FILE, &dluData);
    if (result != DLEDS_OK)
    {
        if (pListSciFileName != NULL)
        {
            free(pListSciFileName);
        }
        if (pListMdluEdFileName != NULL)
        {
            free(pListMdluEdFileName);
        }
        return DLEDS_ERROR;
    }
    
   
    /* Create sub directory name where first SCI TAR file will be unpacked */
    if (noSciFiles > 1)
    {       
        pos = strchr(pListSciFileName,',');
        strncpy(dirName,pListSciFileName,pos - pListSciFileName);
        dirName[pos - pListSciFileName] = '\0';
    }
    else
    {
        strcpy(dirName,pListSciFileName);
    }

    /* Unpack all SCI packages from the ED sub package */
    while ((dirName != NULL) && (noSciFiles > 0))
    {
        /* Assume SCI file to be uncompressed indicated by file extension "sci" */
        sciCompressed = 0;
        strcpy(sciFile,dirName);
        strcat(sciFile,".sci");
        strcpy(fileWithPath, currentDir);
        strcat(fileWithPath,"/");
        strcat(fileWithPath,sciFile);
        /* Check if uncompressed SCI file exists */
        fp = fopen(fileWithPath,"r");
        if (fp == NULL)
        {
            /* Assume SCI file to be compressed indicated by file extension ".scig" */
            strcpy(sciFile,dirName);
            strcat(sciFile,".scig");
            strcpy(fileWithPath, currentDir);
            strcat(fileWithPath,"/");
            strcat(fileWithPath,sciFile);
            /* Check if compressed SCI file exists */
            fp = fopen(fileWithPath,"r");            
            if (fp == NULL)
            {
                /* Expected SCI file is missing */
                strcpy(fileWithPath, currentDir);
                strcat(fileWithPath,"/");
                strcat(fileWithPath,dirName);
                strcat(fileWithPath,".sci(g)");
                DebugError1("Expected SCI TAR file not found (%s)",
                    fileWithPath);
                if (pListSciFileName != NULL)
                {
                    free(pListSciFileName);
                }
                if (pListMdluEdFileName != NULL)
                {
                    free(pListMdluEdFileName);
                }
                return DLEDS_ERROR; 
            }
            /* SCI found to be compressed */
            sciCompressed = 1;  
        }
        fclose(fp);
    
        /* Syntax for dirName:  <function>_<subFunction>_<device type>_<source> */
        /* Move to sub directory */
        result = dledsUnpackSCI(sciFile, dirName, sciCompressed, source, dluData.edPackageName, dluData.edPackageVersion);
        if (result != DLEDS_OK)
        {
            return DLEDS_SCI_ERROR;
        }
        noSciFiles--;
        
        /* Check if any SCI file left */
        if (noSciFiles > 0)
        {
            pNewListSciFileName = pos+1;
            /* Create sub directory name where to unpack next SCI TAR file */
            if (noSciFiles > 1)
            {       
                pos = strchr(pNewListSciFileName,',');
                strncpy(dirName,pNewListSciFileName,pos - pNewListSciFileName);
                dirName[pos - pNewListSciFileName] = '\0';
            }
            else
            {
                strcpy(dirName,pNewListSciFileName);
            }
        }
    } /* while SCI packages available in ED sub package */
    
    if (pListSciFileName != NULL)
    {
        free(pListSciFileName);
    }
    if (pListMdluEdFileName != NULL)
    {
        free(pListMdluEdFileName);
    }
    result = upCurrentDirectory();    
    return result;    
}

/*******************************************************************************
 * 
 * Function name: dledsUnpackEDPackage
 *
 * Abstract:      This function unpacks the ED package. For each ED sub package
 *                there is a call for the unpacking function.
 *
 * Return value:  DLEDS_OK        - ED package has been unpacked
 *                DLEDS_SCI_ERROR - Failed to unpack SCI
 *                DLEDS_ERROR     - Failed to unpack ED package
 *
 * Globals:       currentDir
 *                DLEDS_XML_TMP_FILE
 */
DLEDS_RESULT dledsUnpackEDPackage(
    char* fileName)      /* IN: TAR file containing ED package to be installed */
{
    DLEDS_RESULT result = DLEDS_OK;
    FILE*   fp;
    char    dirName[64];
    char    instFile[256];
    char    tarFile[256];
    char    fileWithPath[256];
    char    EDtype[16];
    TYPE_DLU_HEADER dluHeader;
    size_t  read;
    size_t  write;
    UINT32  expectedDluVersion;
    UINT32  expectedDluTypeCode;
    char    expectedDluName[64];
    UINT32  dluFileClass;
    TYPE_DLU_TPATH_HEADER tpathHeader;
    char*   pos;
    void*   pPayloadBuffer;
    char    level[16];
    UINT32  noTarFiles;
    char*   pListTarFileName;
    char*   pNewListTarFileName;
    UINT32  noDluFiles;
    char*   pListDluFileName;
    
    /* Check if it is an ED package */
    if (strstr(fileName,".edp") != NULL)
    {
        /* Syntax for fileName:  <ED type>_BT_<version>_CU_<version>.edp */
        /* Unpack TAR file placed in current directory. */
        strcpy(tarFile, currentDir);
        strcat(tarFile, "/");
        strcat(tarFile, fileName);
        
        /* Files are extracted to current directory of the file system */
        /* TAR file is removed after it has been unpacked */
        if (unpackTarFile(tarFile, currentDir, 0) != DLEDS_OK)
        {
            setRootDirectory();
            return DLEDS_ERROR;
        }
                
        /* Check DLU header for  installation file (INST_Structure) */
        /* Check that file INST_<ED type>.dl2 exists */
        pos = strchr(fileName,'_');
        strncpy(EDtype, fileName, pos - fileName);
        EDtype[pos - fileName] = '\0';
        strcpy(instFile, currentDir);
        strcat(instFile, "/INST_");
        strcat(instFile, EDtype);
        strcat(instFile, ".dl2");

        fp = fopen(instFile, "rb");
        if (fp == NULL)
        {
            /* Check if INST is in lower case */
            strcpy(instFile, currentDir);
            strcat(instFile, "/inst_");
            strcat(instFile, EDtype);
            strcat(instFile, ".dl2");
            fp = fopen(instFile, "rb");
            if (fp == NULL)
            {
                DebugError1("Could not open INST file (%s)", instFile);
                setRootDirectory();
                return DLEDS_ERROR;
            }
        }
        
        /* Retrieve DLU header from extracted file INST_<ED type>.dl2 */
        read = fread(&dluHeader, 1, sizeof(dluHeader), fp);
        if( read != sizeof(dluHeader) )
        {
            /* Could not read header from DLU */
            DebugError3("Could not read DLU header from installation file (%s), fread returned %d (Expected = %d)",
                instFile, read, sizeof(dluHeader)); 
            fclose(fp);
            return DLEDS_ERROR;
        }
        expectedDluVersion = 0x00000000;    /* No valid version in DLU header for ED package (BT and CU version in file name) */
        expectedDluTypeCode = 0x03000002;    /* DLU_TYPE_EDDC_INST_STRUCTURE */
        strcpy(expectedDluName, "INST_");
        strcat(expectedDluName, EDtype);

        result = verifyDluHeader(&dluHeader, expectedDluVersion, expectedDluTypeCode, expectedDluName);
        if (result == DLEDS_ERROR)
        {
            fclose(fp);
            setRootDirectory();
            return DLEDS_ERROR;
        }
        
        /* Check payload data for installation file (INST_Structure)  */
        /* Check if there is any Target Path Header in DLU */
        dluFileClass = (ntohl(dluHeader.dluTypeCode) & DLU_FILE_CLASS_MASK) >> 24;
        if (dluFileClass == DLU_FILE_CLASS_DLU_EXT)
        {
            /*  Yes we have an extended DLU header */
            /*DebugError1("DLU type code indicate TPATH header, DLU file class = 0x%x",
                dluFileClass);*/
            read =  fread(&tpathHeader,1,sizeof(tpathHeader), fp);
            if (read != sizeof(tpathHeader))
            {
                /* Could not read extended DLU header */
                DebugError1("Could not read TPATH in DLU header extension in (%s)", instFile);
                fclose(fp);
                setRootDirectory();
                return DLEDS_ERROR;
            }
        }
        /* Read payload data in XML format */
        pPayloadBuffer = malloc(ntohl(dluHeader.dluDataSize));
        if (pPayloadBuffer == NULL)
        {
            DebugError1("Could not allocate memory for Payload buffer (size= %d)",
                ntohl(dluHeader.dluDataSize));
            fclose(fp);
            setRootDirectory();
            return DLEDS_ERROR;        
        }
        read =  fread(pPayloadBuffer,1,ntohl(dluHeader.dluDataSize), fp);
        if (read != ntohl(dluHeader.dluDataSize))
        {
            DebugError1("Could not read payload data from (%s)", instFile);
            free(pPayloadBuffer);
            fclose(fp);
            setRootDirectory();
            return DLEDS_ERROR;        
        }
        fclose(fp);
        remove(instFile);
        
        /* Copy payload XML data to file to be able to use iptcom XML package */
        fp = fopen ( DLEDS_XML_TMP_FILE , "wb" );
        if (fp == NULL)
        {
            DebugError1("Could not open temporary file (%s)", DLEDS_XML_TMP_FILE);                             
            free(pPayloadBuffer);
            setRootDirectory();
            return DLEDS_ERROR;
        }
        write = fwrite (pPayloadBuffer, 1 , ntohl(dluHeader.dluDataSize) , fp );
        free(pPayloadBuffer);        
        fclose (fp);
        if (write != ntohl(dluHeader.dluDataSize))
        {
            DebugError3("Failed to write to (%s), fwrite returned %d (Expected = %d)",
                DLEDS_XML_TMP_FILE, write, ntohl(dluHeader.dluDataSize)); 
            setRootDirectory();
            return DLEDS_ERROR;        
        }
        
        /* Read value from XML tag "Level" in payloadData */
        pListTarFileName = NULL;
        pListDluFileName = NULL;
        if ((result = readDataFromInstStructure(
                    DLEDS_XML_TMP_FILE,
                    level,
                    &noTarFiles, &pListTarFileName, 
                    &noDluFiles, &pListDluFileName)) != DLEDS_OK)
        {
            DebugError1("Failed to retrieve XML data from ED INST_struct file (%s)",
                instFile);
            if (pListTarFileName != NULL)
            {
                free(pListTarFileName);
            }
            if (pListDluFileName != NULL)
            {
                free(pListDluFileName);
            }
            setRootDirectory();
            return DLEDS_ERROR;                    
        }

        /* Check that ED INST_Structure is valid */
        if ((strcmp(level,"EDPackage") != 0) || (noTarFiles == 0) || (noDluFiles != 0))
        {   
            DebugError1("Error in ED INST_struct file (%s)", instFile);   
            if (pListTarFileName != NULL)
            {
                free(pListTarFileName);
            }
            if (pListDluFileName != NULL)
            {
                free(pListDluFileName);
            }
            setRootDirectory();
            return DLEDS_ERROR;                    
        }
                
        /* Create name for sub directory where to unpack first ED package TAR file */
        if (noTarFiles > 1)
        {       
            pos = strchr(pListTarFileName,',');
            strncpy(dirName,pListTarFileName,pos - pListTarFileName);
            dirName[pos - pListTarFileName] = '\0';
        }
        else
        {
            strcpy(dirName,pListTarFileName);
        }
                
        /* Unpack all TAR files from the ED package */
        while ((dirName != NULL) && (noTarFiles > 0))
        {
            strcpy(tarFile, dirName);
            strcat(tarFile, ".edsp");
            strcpy(fileWithPath, currentDir);
            strcat(fileWithPath, "/");
            strcat(fileWithPath, tarFile);
            /* Check that ED sub-package exists */
            fp = fopen(fileWithPath,"r");
            if (fp == NULL)
            {
                DebugError1("Expected ED sub-package not found (%s)",
                    fileWithPath);
                if (pListTarFileName != NULL)
                {
                    free(pListTarFileName);
                }
                if (pListDluFileName != NULL)
                {
                    free(pListDluFileName);
                }
                return DLEDS_ERROR;   
            }
            fclose(fp);
    
            /* Syntax for dirName:  <ED type>_<source>_<version> */
            result = dledsUnpackEDSubPackage(tarFile, dirName);
            if (result != DLEDS_OK)
            {
                if (result == DLEDS_SCI_ERROR)
                {
                    DebugError0("Issue to unpack SCI package");
                }
                else
                { 
                    DebugError1("Issue to unpack ED sub package (%s)", tarFile);
                } 
                return result;
            }
            noTarFiles--;
            
            /* Check if any TAR file left */
            if (noTarFiles > 0)
            {
                pNewListTarFileName = pos+1;
                /* Create sub directory name where to unpack next ED package TAR file */
                if (noTarFiles > 1)
                {       
                    pos = strchr(pNewListTarFileName,',');
                    strncpy(dirName,pNewListTarFileName,pos - pNewListTarFileName);
                    dirName[pos - pNewListTarFileName] = '\0';
                }
                else
                {
                    strcpy(dirName,pNewListTarFileName);
                }
            }
        } /* while TAR files available in ED package */
        
        if (pListTarFileName != NULL)
        {
            free(pListTarFileName);
        }
        if (pListDluFileName != NULL)
        {
            free(pListDluFileName);
        }
    }
    /* Check if it is an ED sub package */
    else if (strstr(fileName,".edsp") != NULL)
    {
        /* Syntax for file name:  <ED type>_<source>_<version>.edsp */
        pos = strchr(fileName, '.');
        strncpy(dirName, fileName, pos - fileName);
        dirName[pos - fileName] = '\0';

        /* Unpack TAR file */
        result = dledsUnpackEDSubPackage(fileName, dirName);
        upCurrentDirectory();
        if (result != DLEDS_OK)
        {
            if (result == DLEDS_SCI_ERROR)
            {
                DebugError0("Issue to unpack SCI package");
            }
            else
            { 
                DebugError1("Issue to unpack ED sub package (%s)", fileName);
            } 
            return result;
        }
    }
    else
    {
        /* Illegal name on ED package */
        DebugError1("Illegal file name of ED package (%s)", fileName);
        result = DLEDS_ERROR;
    }    
    return result;    
}


/*******************************************************************************
 * 
 * Function name: dledsUnpackEDPackage2
 *
 * Abstract:      This function unpacks an EDSP2 package. 
 *
 * Return value:  DLEDS_OK        - ED package has been unpacked
 *                DLEDS_SCI_ERROR - Failed to unpack SCI
 *                DLEDS_ERROR     - Failed to unpack ED package
 *
 * Globals:       currentDir
 *                DLEDS_XML_TMP_FILE
 */
DLEDS_RESULT dledsUnpackEDPackage2(
    char* fileName)      /* IN: TAR file containing ED package to be installed */
{
    DLEDS_RESULT        result = DLEDS_OK;
    FILE*               fp;
    char                infoFile[256];
    char                fileWithPath[256];
    EDSP_Info           edspInfo;
    UINT32              edspInfoCrc32;
    UINT32              edspCrc32;
    UINT16              noOfSciFiles;
    UINT16              tempNoSciFiles;
    EDSP_InfoHeader*    pEdspInfoHeader;            /* Pointer to EDSP Info header in EDSP_Info.xml */
    SCI_Info*           pCurrentEdspSciInfo;        /* Pointer to SCI Info structure in EDSP_Info.xml */
    char*               pos;
    char                subDirectoryName[64];
    char                destinationDir[256];
    UINT8               sciCompressed;
    char                sciName[64];
    char                sciFile[64];
    char                edspName[32];


    /* Check if it is an ED package version 2 */
    if (strstr(fileName,".edsp2") != NULL)
    {
        /* Extract directory name from file name:  <EDSP_Name>.edsp2 */
        pos = strchr(fileName, '.');
        strncpy(subDirectoryName, fileName, pos - fileName);
        subDirectoryName[pos - fileName] = '\0';

        /* prepare parameters for unpacking of TAR file */
        strcpy(fileWithPath, currentDir);
        strcat(fileWithPath,"/");
        strcat(fileWithPath,fileName);
        strcpy(destinationDir, currentDir); 
        strcat(destinationDir,"/");
        strcat(destinationDir, subDirectoryName);

        /* Change current DLEDS directory to sub directory */
        result = downCurrentDirectory(subDirectoryName);

        /* TAR file is removed after it has been unpacked */
        if (unpackTarFile(fileWithPath, destinationDir, 0) != DLEDS_OK)
        {
            setRootDirectory();
            return DLEDS_ERROR;
        }

        /* Extract EDSP name from file name:  <EDSP_Name>.edsp2 */
        pos = strchr(fileName, '.');
        strncpy(edspName, fileName, pos - fileName);
        edspName[pos - fileName] = '\0';

        /* Check that file EDSP.crc exists */
        strcpy(infoFile, currentDir);
        strcat(infoFile, "/");
        strcat(infoFile, "EDSP.crc");
        fp = fopen(infoFile, "rb");
        if (fp == NULL)
        {
            DebugError1("Could not open EDSP.crc file (%s)", infoFile);
            setRootDirectory();
            return DLEDS_ERROR;
        }
        /* Read CRC32 value from file EDSP.crc */
        if (fscanf(fp,"%x", &edspCrc32) != 1)
        {
            DebugError1("Could not read CRC32 from EDSP.crc file (%s)", infoFile);
            setRootDirectory();
            fclose(fp);
            return DLEDS_ERROR;
        }
        fclose(fp);


        /* Check that file EDSP_Info.crc exists */
        strcpy(infoFile, currentDir);
        strcat(infoFile, "/");
        strcat(infoFile, "EDSP_Info.crc");
        fp = fopen(infoFile, "rb");
        if (fp == NULL)
        {
            DebugError1("Could not open EDSP_Info.crc file (%s)", infoFile);
            setRootDirectory();
            return DLEDS_ERROR;
        }
        /* Read CRC32 value from file EDSP_Info.crc */
        if (fscanf(fp,"%x", &edspInfoCrc32) != 1)
        {
            DebugError1("Could not read CRC32 from EDSP_Info.crc file (%s)", infoFile);
            setRootDirectory();
            fclose(fp);
            return DLEDS_ERROR;
        }
        fclose(fp);

        /* Check that file EDSP_Info.xml exists */
        strcpy(infoFile, currentDir);
        strcat(infoFile, "/");
        strcat(infoFile, "EDSP_Info.xml");
        fp = fopen(infoFile, "rb");
        if (fp == NULL)
        {
            DebugError1("Could not open EDSP_Info.xml file (%s)", infoFile);
            setRootDirectory();
            return DLEDS_ERROR;
        }
        fclose(fp);

        /* Verify that EDSP_Info.xml is valid 
           by calculating and checking CRC32 against expected value */
        result = dleds_checkCRC(infoFile, edspInfoCrc32, 0xFFFFFFFF);
        if (result != DLEDS_OK)
        {
            DebugError1("Incorrect CRC32 for (%s)", infoFile);
            setRootDirectory();
            return DLEDS_ERROR;
        }


        /* Read data from file EDSP_Info.xml */
        memset(&edspInfo,0,sizeof(edspInfo));
        result = dledsRetrieveCompleteEdspInfo(infoFile, &edspInfo);
        if (result != DLEDS_OK)
        {
            DebugError1("Could not retrieve complete information from EDSP info file (%s)", infoFile);
            setRootDirectory();
            return DLEDS_ERROR;
        }

        /* Check that EDSP name in file match the file name */
        if (strcmp(edspName, edspInfo.edspInfoHeader.edspName) != 0)
        {
            DebugError2("File name (%s) does not match EDSP name (%s) in EDSP_Info.xml", edspName, edspInfo.edspInfoHeader.edspName);
            setRootDirectory();
            return DLEDS_ERROR;
        }

        /* Count number of SCI files in ED Package */
        result = countSci2Files(currentDir, &noOfSciFiles);
        if (result != DLEDS_OK)
        {
            DebugError1("Could not count number of SCI files in package (%s)", currentDir);
            setRootDirectory();
            return DLEDS_ERROR;
        }

        /* Check against expected number of SCI files */
        if (noOfSciFiles > edspInfo.noOfSci)
        {
            /* To many SCI files compared to SCI list */
            DebugError2("Number of SCIs in package(%u) exceeds expected (%u) ", noOfSciFiles, edspInfo.noOfSci);
            setRootDirectory();
            return DLEDS_ERROR;
        }
        else if (noOfSciFiles < edspInfo.noOfSci)
        {
            /* Missing SCI files compared to SCI list */
            DebugError2("Number of SCIs in package(%u) less than expected (%u) ", noOfSciFiles, edspInfo.noOfSci);
            setRootDirectory();
            return DLEDS_ERROR;
        }
        else
        {
            /* Correct number of SCI files in ED package */

            /* Create pointer to EDSP Info */
            pEdspInfoHeader = &(edspInfo.edspInfoHeader);

            /* Create pointer to first SCI info structure */
            pCurrentEdspSciInfo = edspInfo.pSciInfo;

            /* Unpack all SCI files from the ED package */
            tempNoSciFiles = noOfSciFiles;
            totalNoOfDLU = 0;  /* Used for calculating number of progress request messages during installation */
            while ((pCurrentEdspSciInfo != NULL) && (tempNoSciFiles > 0))
            {
                strcpy(sciName, pCurrentEdspSciInfo->sciInfoHeader.sciName);

                /* Assume SCI file to be uncompressed indicated by file extension "sci" */
                sciCompressed = 0;
                strcpy(sciFile,sciName);
                strcat(sciFile,".sci2");
                strcpy(fileWithPath, currentDir);
                strcat(fileWithPath,"/");
                strcat(fileWithPath,sciFile);
                /* Check if uncompressed SCI file exists */
                fp = fopen(fileWithPath,"r");
                if (fp == NULL)
                {
                    /* Assume SCI file to be compressed indicated by file extension ".scig" */
                    strcpy(sciFile,sciName);
                    strcat(sciFile,".sci2g");
                    strcpy(fileWithPath, currentDir);
                    strcat(fileWithPath,"/");
                    strcat(fileWithPath,sciFile);
                    /* Check if compressed SCI file exists */
                    fp = fopen(fileWithPath,"r");            
                    if (fp == NULL)
                    {
                        /* Expected SCI file is missing */
                        strcpy(fileWithPath, currentDir);
                        strcat(fileWithPath,"/");
                        strcat(fileWithPath,sciName);
                        strcat(fileWithPath,".sci2(g)");
                        DebugError1("Expected SCI TAR file not found (%s)",
                            fileWithPath);
                        return DLEDS_ERROR; 
                    }
                    /* SCI found to be compressed */
                    sciCompressed = 1;  
                }
                fclose(fp);
                    
                /* Move to sub directory */
                result = dledsUnpackSCI2(sciFile, sciName, (SCI_InfoHeader*)&(pCurrentEdspSciInfo->sciInfoHeader), sciCompressed, pEdspInfoHeader, edspCrc32);
                if (result != DLEDS_OK)
                {
                    return DLEDS_SCI_ERROR;
                }
        
                /* Check if any SCI file left */
                tempNoSciFiles--;
                if (tempNoSciFiles > 0)
                {
                    /* Find next SCI */
                    pCurrentEdspSciInfo = pCurrentEdspSciInfo->pNextSciInfo;
                }
            } /* while SCI packages available in ED sub package */
        } 
    }
    else
    {
        /* Illegal name on ED package */
        DebugError1("Illegal file name of ED package version 2 (%s)", fileName);
        result = DLEDS_ERROR;
    }    
    
    return result;
}


/*******************************************************************************
 * GLOBAL FUNCTIONS
 */

/*******************************************************************************
 * 
 * Function name: dledsInstallEDPackage
 *
 * Abstract:      This function is called when a ED package has been retrieved 
 *                by FTP an is ready to be installed on target.
 *
 * Return value:  DLEDS_OK                 - ED package has been successfully installed
 *                DLEDS_SCI_ERROR          - Failed to unpack SCI
 *                DLEDS_ED_ERROR           - Failed to unpack EDP or EDSP
 *                DLEDS_INSTALLATION_ERROR - Failed to install DLU
 *
 * Globals:       currentDir
 */    
DLEDS_RESULT  dledsInstallEDPackage(
    char* downloadFileName,      /* IN: TAR file containing ED package to be installed */
    char* tempDir)               /* IN: Path to DLEDS temporary directory */
{
    DLEDS_RESULT         result = DLEDS_OK;
    FILE*                fp;
    char                 installationFileName[256];
    char                 fullPathFileName[256];
    char                 ftpDir[256];
    long                 filePos;
    long                 nextFilePos;
    TYPE_DLEDS_DLU_DATA  dluData; 
    int                  rDirStatus;

    /* Clear values for packages to be installed */
    strcpy(installedBtPackageName, "");
    installedBtPackageVersion = 0;
    strcpy(installedCuPackageName, "");
    installedCuPackageVersion = 0;
      
    /* Set DLEDS FTP directory as current working directory */
    strcpy(currentDir, tempDir);
    
    if (downCurrentDirectory("ftp") != DLEDS_OK)
    {
        DebugError0("Could not change current directory to DLEDS FTP directory");
        return DLEDS_ERROR;
    }
    /* Check that ED package exists in DLEDS FTP directory */
    strcpy(fullPathFileName, currentDir);
    strcat(fullPathFileName, "/");
    strcat(fullPathFileName, downloadFileName);
    fp = fopen(fullPathFileName,"r");
    if (fp == NULL)
    {
        DebugError1("Could not open ED package file (%s)", fullPathFileName);
        return DLEDS_ERROR;   
    }
    fclose(fp);

    result = dledsUnpackEDPackage(downloadFileName);
    if (result != DLEDS_OK )
    {
        if (result == DLEDS_SCI_ERROR)
        {
            DebugError0("Failed to unpack SCI package");
        }
        else
        {
            DebugError1("Failed to unpack ED package (%s)",
                downloadFileName);
            result = DLEDS_ED_ERROR;
        }
        
        /* Clear all temporary files if unpack fails */
        strcpy(ftpDir, tempDir);
        strcat(ftpDir, "/ftp");
        rDirStatus = removeDirectory(ftpDir);
        if (rDirStatus != 0)
        {
            DebugError1("Could not delete FTP directory (%s)", ftpDir);
        }
        if (remove(DLEDS_XML_TMP_FILE) != 0)
        {
            DebugError1("Could not delete temporary file (%s)", DLEDS_XML_TMP_FILE);
        }
        if (remove(DLEDS_INSTALLATION_FILE) != 0)
        {
            DebugError1("Could not delete temporary file (%s)", DLEDS_INSTALLATION_FILE);
        }
        /*DebugError1("Result of removeDirectory() == %d", rdirStatus);*/
        
        return result;   
    }
    
    /* 
     * Unpacking was OK. Before Installation of Master and Slave DLUs. Remove any
     * previously installed version of same package type (BT or CU).
     */
    if ( (strlen(installedBtPackageName) != 0) && (installedBtPackageVersion != 0))
    {
        /* Delete all Master and Slave DLU's that are not part of the BT package to be installed */
        if ( dledsCleanupBT() != DLEDS_OK)
        {
            DebugError0("Failed to clean up previously installed BT package");
        }
    }

    if ( (strlen(installedCuPackageName) != 0) && (installedCuPackageVersion != 0))
    {
        /* Delete all Master and Slave DLU's that are not part of the CU package to be installed */
        if ( dledsCleanupCU() != DLEDS_OK)
        {
            DebugError0("Failed to clean up previously installed CU package");
        }
    }
    
    /* 
     * There should be an installation information file available with
     * the name dledsDluInstallation.txt 
     * Check that this file exists in the ftp directory 
     */
    
    strcpy(installationFileName, DLEDS_INSTALLATION_FILE);
    fp = fopen(installationFileName, "r");
    if (fp == NULL)
    {
        DebugError1("No installation information file found (%s)",
            installationFileName);
        return DLEDS_INSTALLATION_ERROR;   
    }
    fclose(fp);
    
    /* Read first entry from installation information file */
    filePos = 0;
    result = dledsReadInstallationEntry(installationFileName,filePos,&nextFilePos,&dluData);
    if (result != DLEDS_OK )
    {
        result = DLEDS_INSTALLATION_ERROR;
    }
    
    while ((result == DLEDS_OK) && (nextFilePos != 0))
    {
        DebugError1("INSTALL DLU= %s", dluData.dluFileName);
        result = dledsDluInstall(&dluData);
        if (result == DLEDS_OK)
        {
            filePos = nextFilePos;
            result = dledsReadInstallationEntry(installationFileName,filePos,&nextFilePos,&dluData);
        }
        else
        {
            DebugError0("Failure in dledsReadInstallationEntry");
            /* 
             * Installation failed. 
             * Remove any previously installed version of same package type (BT or CU).
             */
            if ( (strlen(installedBtPackageName) != 0) && (installedBtPackageVersion != 0))
            {
                /* Delete all Master and Slave DLU's that are not part of the BT package to be installed */
                if ( dledsCleanupBT() != DLEDS_OK)
                {
                    DebugError0("Failed to clean up previously installed BT package");
                }
            }

            if ( (strlen(installedCuPackageName) != 0) && (installedCuPackageVersion != 0))
            {
                /* Delete all Master and Slave DLU's that are not part of the CU package to be installed */
                if ( dledsCleanupCU() != DLEDS_OK)
                {
                    DebugError0("Failed to clean up previously installed CU package");
                }
            }
            
            result = DLEDS_INSTALLATION_ERROR;
        }
        
    }
    
    /* Clean up after installation */
    strcpy(ftpDir, tempDir);
    strcat(ftpDir, "/ftp");
    rDirStatus = removeDirectory(ftpDir);
    if (rDirStatus != 0)
    {
        DebugError1("Could not delete FTP directory (%s)", ftpDir);
    }

    if (remove(DLEDS_XML_TMP_FILE) != 0)
    {
        DebugError1("Could not delete temporary file (%s)", DLEDS_XML_TMP_FILE);
    }

    if (remove(DLEDS_INSTALLATION_FILE) != 0)
    {
        DebugError1("Could not delete temporary file (%s)", DLEDS_INSTALLATION_FILE);
    }
        
    return result;  
}


/*******************************************************************************
 * 
 * Function name: dledsInstallEDSP2Package
 *
 * Abstract:      This function is called when a EDSP2 package has been retrieved 
 *                by FTP an is ready to be installed on target.
 *
 * Return value:  DLEDS_OK                 - ED package has been successfully installed
 *                DLEDS_SCI_ERROR          - Failed to unpack SCI
 *                DLEDS_ED_ERROR           - Failed to unpack EDSP
 *                DLEDS_INSTALLATION_ERROR - Failed to install DLU
 *                DLEDS_STATE_CHANGE       - Installation done by someone else, change state
 *
 * Globals:       currentDir
 */    
DLEDS_RESULT  dledsInstallEDSP2Package(
    char* downloadFileName,      /* IN: TAR file containing EDSP2 package to be installed */
    char* tempDir)               /* IN: Path to DLEDS temporary directory */
{
    DLEDS_RESULT            result = DLEDS_OK;
    DLEDS_RESULT            reportResult;
    char                    fullPathFileName[256];
    char                    installationFileName[256];
    struct stat             stStat;
    char                    edspName[32];
    char                    ftpDir[256];
    FILE*                   fp;
    long                    filePos;
    long                    nextFilePos;
    TYPE_DLEDS_DLU_DATA2    dluData; 
    int                     rDirStatus;
    char*                   pos;
    UINT8                   abortRequest;
    UINT8                   previousProcentageSent;
    UINT8                   procentage;
    UINT32                  noOfDluInstalled = 0;
    INT32                   errorCode;

    /* Set DLEDS FTP directory as current working directory */
    strcpy(currentDir, tempDir);

    if (downCurrentDirectory("ftp") != DLEDS_OK)
    {
        DebugError0("Could not change current directory to DLEDS FTP directory");
        return DLEDS_ERROR;
    }
    /* Check that EDSP2 package exists in DLEDS FTP directory */
    strcpy(fullPathFileName, currentDir);
    strcat(fullPathFileName, "/");
    strcat(fullPathFileName, downloadFileName);
    if (stat(fullPathFileName, &stStat) != 0)
    {
        DebugError1("EDSP2 package file (%s) missing", fullPathFileName);
        result = DLEDS_ERROR;
    }
    else if ((result = dledsPlatformInspectPackage(downloadFileName)) == DLEDS_STATE_CHANGE)
    {
        /* Package has been received and analyzed */
        reportResult = dleds_reportProgress(PROGRESS_PACKAGE_OK, PROGRESS_45, PROGRESS_NO_RESET_IN_PROGRESS,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);

        /* DLEDS should enter DLEDS IDLE MODE and pass the processing of the package to the target */
        if (dledsPlatformPassPackageProcessing() == DLEDS_OK)
        {
            DebugError0("Starting installation, going to DLEDS IDLE_MODE");
        }
        else
        {
            DebugError0("Error from call to target installer");
            result = DLEDS_ERROR;
        }
    }
    else
    {
        result = dledsUnpackEDPackage2(downloadFileName);

        if (result == DLEDS_OK)
        {
            errorCode = DL_OK;
        }
        else if (result == DLEDS_SCI_ERROR)
        {
            errorCode = DL_SCI_ERROR;
        }
        else
        {
            errorCode =  DL_ED_ERROR;
        }

        /* Package has been received and analyzed */
        /* Ignore abortRequest when we got this far */
        reportResult = dleds_reportProgress(PROGRESS_PACKAGE_OK, PROGRESS_45, PROGRESS_NO_RESET_IN_PROGRESS,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, errorCode, NULL, &abortRequest);

        if (result == DLEDS_OK)
        {
            /* Ignore abortRequest when we got this far */
            reportResult = dleds_reportProgress(PROGRESS_VERIFICATION, PROGRESS_60, PROGRESS_NO_RESET_IN_PROGRESS,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);
        }

        if (result != DLEDS_OK )
        {
            if (result == DLEDS_SCI_ERROR)
            {
                DebugError0("Failed to unpack SCI package");
            }
            else
            {
                DebugError1("Failed to unpack ED package (%s)",
                    downloadFileName);
                result = DLEDS_ED_ERROR;
            }
        
            /* Clear all temporary files if unpack fails */
            strcpy(ftpDir, tempDir);
            strcat(ftpDir, "/ftp");
            rDirStatus = removeDirectory(ftpDir);
            if (rDirStatus != 0)
            {
                DebugError1("Could not delete FTP directory (%s)", ftpDir);
            }
            if (remove(DLEDS_XML_TMP_FILE) != 0)
            {
                DebugError1("Could not delete temporary file (%s)", DLEDS_XML_TMP_FILE);
            }
            if (remove(DLEDS_INSTALLATION_FILE) != 0)
            {
                DebugError1("Could not delete temporary file (%s)", DLEDS_INSTALLATION_FILE);
            }
            /*DebugError1("Result of removeDirectory() == %d", rdirStatus);*/
        
            return result;   
        }

        /* 
        * Unpacking was OK. Before Installation of slave DLUs. Remove all
        * previously installed DLUs that was part of the same EDSP.
        */

        /* Extract EDSP name from file name:  <EDSP_Name>.edsp2 */
        pos = strchr(downloadFileName, '.');
        strncpy(edspName, downloadFileName, pos - downloadFileName);
        edspName[pos - downloadFileName] = '\0';

        result = dledsCleanupEDSP2(edspName);
        if (result != DLEDS_OK)
        {
            DebugError1("Cleanup failed for EDSP (%s)", edspName);
            return DLEDS_INSTALLATION_ERROR;   
        }
        
        /* 
        * There should be an installation information file available with
        * the name dledsDluInstallation.txt 
        * Check that this file exists in the ftp directory 
        */
    
        strcpy(installationFileName, DLEDS_INSTALLATION_FILE);
        fp = fopen(installationFileName, "r");
        if (fp == NULL)
        {
            DebugError1("No installation information file found (%s)",
                installationFileName);
            return DLEDS_INSTALLATION_ERROR;   
        }
        fclose(fp);

        /* Ignore abortRequest when we got this far */
        reportResult = dleds_reportProgress(PROGRESS_INSTALL_PROGRESS, PROGRESS_70, PROGRESS_NO_RESET_IN_PROGRESS,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);
    
        /* Read first entry from installation information file */
        filePos = 0;
        result = dledsReadInstallationEntry2(installationFileName,filePos,&nextFilePos,&dluData);
        if (result != DLEDS_OK )
        {
            result = DLEDS_INSTALLATION_ERROR;
        }
    
        previousProcentageSent = PROGRESS_70;
        DebugError1("Total Number of DLU to install= %d", totalNoOfDLU);
        while ((result == DLEDS_OK) && (nextFilePos != 0))
        {
            /* calculate if progress request should be sent */
            procentage = (UINT8)(((noOfDluInstalled * 20)/totalNoOfDLU) + PROGRESS_70);
            DebugError1("noOfDluInstalled= %d", noOfDluInstalled);
            DebugError1("procentage= %d", procentage);
            DebugError1("previousProcentageSent= %d", previousProcentageSent);

            if (procentage > previousProcentageSent)
            {
                reportResult = dleds_reportProgress(PROGRESS_INSTALL_PROGRESS, procentage, PROGRESS_NO_RESET_IN_PROGRESS,
                                            PROGRESS_USE_DEFAULT_TIMEOUT, DL_OK, NULL, &abortRequest);
                previousProcentageSent = procentage;
            }
            noOfDluInstalled++;

            DebugError1("INSTALL DLU= %s", dluData.dluFileName);
            result = dledsDluInstall2(&dluData);
            if (result == DLEDS_OK)
            {
                filePos = nextFilePos;
                result = dledsReadInstallationEntry2(installationFileName,filePos,&nextFilePos,&dluData);
            }
            else
            {
                DebugError0("Failure in dledsReadInstallationEntry2");
                /* 
                * Installation failed. 
                * Remove any previously installed version of same EDSP.
                */
                result = dledsCleanupEDSP2(edspName);
                if (result != DLEDS_OK)
                {
                    DebugError1("Cleanup failed for EDSP (%s)", edspName);
                }            
                result = DLEDS_INSTALLATION_ERROR;
            }
        
        } /* end while */
    
        /* Clean up after installation */
        strcpy(ftpDir, tempDir);
        strcat(ftpDir, "/ftp");
        rDirStatus = removeDirectory(ftpDir);
        if (rDirStatus != 0)
        {
            DebugError1("Could not delete FTP directory (%s)", ftpDir);
        }

        if (remove(DLEDS_XML_TMP_FILE) != 0)
        {
            DebugError1("Could not delete temporary file (%s)", DLEDS_XML_TMP_FILE);
        }

        if (remove(DLEDS_INSTALLATION_FILE) != 0)
        {
            DebugError1("Could not delete temporary file (%s)", DLEDS_INSTALLATION_FILE);
        }         
    }
    (void)reportResult;           
    return result;  
}
