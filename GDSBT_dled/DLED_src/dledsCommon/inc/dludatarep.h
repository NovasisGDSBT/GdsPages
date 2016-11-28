/************************************************************************/
/*  (C) COPYRIGHT 2006 by Bombardier Transportation                     */
/*                                                                      */
/*  Bombardier Transportation Switzerland Dep. PPC/EUT                  */
/************************************************************************/
/*                                                                      */
/*  PROJECT:      DVS Download and Versioning System                    */
/*                                                                      */
/*  MODULE:       dludatarep                                            */
/*                                                                      */
/*  ABSTRACT:                                                           */
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
/*  version  yyyy-mm-dd  name       depart.   ref   status              */
/*  -------  ----------  ---------  -------   ----  ---------           */
/*    1.0    2006-07-19  M.Brotz    PPC/EUT    --   created             */
/*    1.1    2010-05-19  M.Brotz    PPC/TET3P  --   created             */
/*                       > Some new DLU types and DLU file classes      */
/*                       > introduced                                   */
/*                                                                      */
/*                                                                      */
/************************************************************************/
/************************************************************************
 * $History: dludatarep.h $
 *
 * *****************  Version 31  *****************
 * User: Brotz        Date: 16.05.11   Time: 17:03
 * Updated in $/MCP/Backend/MCP/McpLib/Src
 *
 * *****************  Version 30  *****************
 * User: Brotz        Date: 18.06.10   Time: 17:45
 * Updated in $/DVS_SW/DVS/src
 *
 * *****************  Version 29  *****************
 * User: Brotz        Date: 19.05.10   Time: 11:04
 * Updated in $/DVS_SW/DVS/src
 * Some new DLU types and DLU file classes introduced
 *
 *  *****************  Version 1  *****************
 *  User: Pkuenzle     Date: 24.08.06   Time: 15:22
 *  Created in $/CSS/CSSLib/COMMON/services/dvs
 ************************************************************************/


#ifndef DLUDATAREP_H
#define DLUDATAREP_H


//#include "dvs_types.h"



#ifdef _MSC_VER
  #pragma pack( push, before_dludatarep_h )
  #pragma pack (1)
  #ifndef ATTR_PACKED
    #define ATTR_PACKED
  #endif
#else
  #ifndef ATTR_PACKED
    #define ATTR_PACKED __attribute__(( packed ))
  #endif
#endif /* defined _MSC_VER */





/* The definition of same basic common data types */

#ifndef CHAR8
  #define CHAR8 char
#endif

#ifndef UINT32
  #define UINT32 unsigned int
#endif


#define DLU_PACKAGE_IDENT     "DLUPACK\0"         /* The package identifier                      */

#define DLU_PACKAGE_IDENT_SIZE         8          /* The size of the package identifier          */

#define DLU_HEADER_VERSION    0x01000000          /* The current version of the header structure */

#define DLU_HEADER_LENGTH     0x00000044          /* The size of the DLU header, must be equal   */
                                                  /* to sizeof(TYPE_DLU_HEADER).                 */

#define DLU_CRC32_HEADER_LEN  0x00000034          /* The size of the DLU header which is checked */
                                                  /* by CRC                                      */

#define DLU_NAME_SIZE                 32          /* The size of the DLU name                    */

#define DLU_TARGET_PATH_SIZE        1024          /* The size of the target path entry. The      */
                                                  /* target path is a null terminated string     */
                                                  /* with up to DLU_TARGET_PATH_SIZE-1 chars.    */

#define DLU_CRC_START_VAL     0xFFFFFFFF          /* The start value for all CRC calculation     */
                                                  /* related to the DLU package format           */

/* ULU related */
#define ULU_TARGET_HW_TYPE_SIZE       32          /* The size of the target HW Type name         */

#define ULU_HEADER_RESERVE            24          /* The size of the ULU header reserve          */

#define ULU_HEADER_LENGTH             60          /* The size of the ULU header                  */



#define DLU_TPATH_HEADER_LENGTH       (DLU_TARGET_PATH_SIZE + 12)

/* DLU File Classes */

#define DLU_FILE_CLASS_MASK    0xFF000000         /* That mask extracts the DLU File Class part  */
                                                  /* from a given DLU type entry within the      */
                                                  /* DLU file.                                   */

#define DLU_FILE_CLASS_DLU           0x00         /* Standard DLU                                */
#define DLU_FILE_CLASS_ULU           0x01         /* DLU contains an ULU a DLU Header extension  */
#define DLU_FILE_CLASS_DLU_EXT       0x02         /* DLU contains DLU with extended target       */
                                                  /* information like target path and target     */
                                                  /* file permission.                            */
#define DLU_FILE_CLASS_EDDC          0x03         /* DLU contains an End Device Download         */
                                                  /* Configuration file. There exists different  */
                                                  /* kind of such configuration files. Its       */
                                                  /* specific type can be determined by          */
                                                  /* evaluating the least significant bytes      */
                                                  /* of the DLU type code field.                 */


#define DLU_TYPE_ULU_FLASH      0x01000000        /* File contains a FLASH based ULU             */

#define DLU_TYPE_ULU_FLASH_PROT 0x01000001        /* File contains a FLASH based ULU for a       */
                                                  /* protected range                             */

#define DLU_TYPE_ULU_RAM        0x01000002        /* File contains a RAM based ULU               */

#define DLU_TYPE_ULU_BLOAD      0x01000003        /* File contains a FLASH based ULU for a       */
                                                  /* protected range. The file contains a        */
                                                  /* a boot loader image, so a erroneous         */
                                                  /* download of such a file may damage the      */
                                                  /* target device so that it must be repaired   */
                                                  /* in factory. Thus the download of such an    */
                                                  /* image shall only be possible with special   */
                                                  /* tooling for experienced users which knows   */
                                                  /* that fact, resp. with standard tooling      */
                                                  /* in specific expert mode which is foreseen   */
                                                  /* for such users.                             */


                                                  /* The following type codes identify a group     */
                                                  /* of End Device Download Configuration files.   */
#define DLU_TYPE_EDDC_MDLU_FUNCTION   0x03000001  /* An MDLU Function configuration file           */
#define DLU_TYPE_EDDC_INST_STRUCTURE  0x03000002  /* An Installation Structure configuration file  */
#define DLU_TYPE_EDDC_INST_ED         0x03000003  /* An End Device Installation package file       */




#define DLU_TYPE_MASTER_MASK        0x00800000    /* If the corresponding bit is set within a    */
                                                  /* DLU type code, the DLU is a so-called       */
                                                  /* Master-DLU with special constraints         */
                                                  /* according handling of it. If the bit is not */
                                                  /* set, we have a Slave-DLU.                   */

#define DLU_CAT_MASK                0x007F0000    /* The DLU_CAT_MASK shall be used to extract   */
                                                  /* the DLU category value from the dluTypeCode */
                                                  /* field within the DLU header structure.      */

#define DLU_BASE_TYPE_MASK          0x0000FFFF    /* The DLU_BASE_TYPE_MASK shall be used to     */
                                                  /* extract the pure DLU base type value (i.e.  */
                                                  /* the type without category, without master   */
                                                  /* bit and without DLU file class) from the    */
                                                  /* dluTypeCode field within the DLU header     */
                                                  /* structure.                                  */


#define DLU_TYPE_MASK               0x00FFFFFF    /* The DLU_TYPE_MASK shall be used to extract  */
                                                  /* the pure DLU type value (without DLU file   */
                                                  /* class information) from the dluTypeCode     */
                                                  /* field within the DLU header structure.      */



#define DLU_CAT_REM_UPGR_APPL_SW    0x00000000    /* Remote upgradable application software      */
                                                  /* package.                                    */

#define DLU_CAT_LOC_UPGR_APPL_SW    0x00010000    /* Locally upgradable application software     */
                                                  /* package.                                    */

#define DLU_CAT_LOC_UPGR_SYS_SW     0x00020000    /* Locally upgradable system software package. */




#define DLU_TYPE_UNSPECIFIC           0x00000000  /* No specific meaning                         */
#define DLU_TYPE_OS_CONFIG            0x00000001  /* Operating system configuration file (e.g.   */
                                                  /* css.ini for CSS operating system)           */
#define DLU_TYPE_APPL_CONFIG          0x00000002  /* Application configuration file. A file      */
                                                  /* which typically contains references to all  */
                                                  /* applications started on operational         */
                                                  /* startup of the device (e.g. objects.lst     */
                                                  /* for CSS operating system.                   */
#define DLU_TYPE_COMPONENT            0x00000003  /* A software component within the responsa-   */
                                                  /* bility of the component manager             */
#define DLU_TYPE_CSS_OBJ              0x00000004  /* A CSS loadable object file                  */
#define DLU_TYPE_DATA                 0x00000005  /* A pure data file object (i.e. no executable */
                                                  /* code                                        */
#define DLU_TYPE_UNPACK_ONCE          0x00000006  /* A (packed) container for multiple payload   */
                                                  /* data files which will be unpacked           */
                                                  /* once after download of the DLU. The original*/
                                                  /* container will be removed after unpack      */
                                                  /* operation. DVS keeps track of the different */
                                                  /* files derived from that archive so that     */
                                                  /* they can be removed by the remove DLU       */
                                                  /* command.                                    */
#define DLU_TYPE_UNPACK_ON_STARTUP    0x00000007  /* A (packed) container for multiple payload   */
                                                  /* data files. The container will be unpacked  */
                                                  /* on each startup of the DVS. Typically the   */
                                                  /* unpack operation target is a volatile       */
                                                  /* ramdisk so that the unpacked files will be  */
                                                  /* removed completel by a restart of the       */
                                                  /* device. The container will not be removed   */
                                                  /* after unpack, so t DLU delete operation     */
                                                  /* will remove the container, but not          */
                                                  /* explicitly the unpacked files.              */
#define DLU_TYPE_EXECUTE_ONCE         0x00000008  /* The payload file will be executed once      */
                                                  /* after download and the DLU will be removed  */
                                                  /* automatically on next restart of the        */
                                                  /* device.                                     */
#define DLU_TYPE_EXECUTE_ON_STARTUP   0x00000009  /* The payload file will be executed on each   */
                                                  /* startup of the DVS.                         */

#define DLU_TYPE_CM_MASTER            0x00000030  /* The master configuration file evaluated     */
                                                  /* by Commponent Manager. That file contains   */
                                                  /* references to all components which shall    */
                                                  /* actually be handled by Component Manager.   */
#define DLU_TYPE_CM_COMPONENT         0x00000031  /* That type code shall be used for all        */
                                                  /* components which are loaded by Component    */
                                                  /* Manager.                                    */
#define DLU_TYPE_CM_CONFIG            0x00000032  /* Components in scope of the Component        */
                                                  /* Manager might use additional DLUs which     */
                                                  /* are not handled by the Component Manager    */
                                                  /* itself (e.g. some configuration of other    */
                                                  /* data files). That type code shall be used   */
                                                  /* for all those DLUs.                         */

#define DLU_TYPE_ED_BT_MASTER         0x00000040  /* An End Device Master DLU by Bombardier      */
#define DLU_TYPE_ED_CU_MASTER         0x00000041  /* An End Device Master DLU by Customer        */
#define DLU_TYPE_SCI_MASTER           0x00000042  /* A Software Configuration Item Master DLU    */
#define DLU_TYPE_IPTCOM_CONFIG        0x00000043  /* An IPTCOM Configuration DLU                 */

#define DLU_TYPE_SAFE_IDENTITY        0x00000050  /* On safety related devices, the device       */
                                                  /* identification as well as the consist       */
                                                  /* identification are stored as DL2 file on    */
                                                  /* targets ECP-Plug. The content of this DL2   */
                                                  /* file is unpacked and integrated into the    */
                                                  /* DVS DLU repository on special request.      */
                                                  /* Such kind of DLUs need to be handled in     */
                                                  /* a special manner (e.g. DVS shall not allow  */
                                                  /* to remove such DLU by via MCP command, host */
                                                  /* based tooling shall not allow to download   */
                                                  /* such kind of DL2 file). That's why a        */
                                                  /* special DLU type is reserved for that kind  */
                                                  /* of DLUs                                     */

#define DLU_TYPE_LINUX_TAR            0x00001001  /* A (packed) container for multiple payload   */
                                                  /* data files which will be unpacked           */
                                                  /* once after download of the DLU. The original*/
                                                  /* container will be removed after unpack      */
                                                  /* operation. The container will be unpacked   */
                                                  /* by the LINUX tar utility which must be      */
                                                  /* available on target.                        */
#define DLU_TYPE_LINUX_GZIP_TAR       0x00001002  /* A (packed) container for multiple payload   */
                                                  /* data files which will be unpacked           */
                                                  /* once after download of the DLU. The original*/
                                                  /* container will be removed after unpack      */
                                                  /* operation. The container will be unpacked   */
                                                  /* by the LINUX tar utility which must be      */
                                                  /* available on target. It's assumed that the  */
                                                  /* container is gzipped.                       */

#define DLU_TYPE_SAFE_TAR_IMAGE       0x00001003  /* New DLU type for safety related devices that */
                                                  /* download software images in TAR file. The    */
                                                  /* purpose with this type is to tell the DVS to */
                                                  /* retrieve TAR file contents from OHAL and not */
                                                  /* from files system (ECP-USB). There exists no */
                                                  /* files on USB as they are deleted when they   */
                                                  /* have been flashed. USB is just a temporary   */
                                                  /* storage for software images in the new safe  */
                                                  /* download concept. */


/*   The DLU header         */

typedef struct STR_DLU_HEADER
{
  CHAR8   packageIdent [DLU_PACKAGE_IDENT_SIZE];                /* Fixed "DLUPACK" identification */
  UINT32  headerLength                           ATTR_PACKED;   /* Fixed header length (==0x44)   */
  UINT32  headerCrc32                            ATTR_PACKED;   /* The header checksum            */
  UINT32  headerVersion                          ATTR_PACKED;   /* The header version             */
  UINT32  dluVersion                             ATTR_PACKED;   /* The DLU version                */
  UINT32  dluDataSize                            ATTR_PACKED;   /* The size of the DLU data       */
  UINT32  dluTypeCode                            ATTR_PACKED;   /* The DLU type code              */
  UINT32  dluCrc32                               ATTR_PACKED;   /* The DLU data checksum          */
  CHAR8   dluName [DLU_NAME_SIZE];                              /* The DLU name (7-bit ASCII)     */
} TYPE_DLU_HEADER;

#define TYPE_DLU_HEADER_EXPECTED_SIZE (DLU_PACKAGE_IDENT_SIZE + DLU_NAME_SIZE + 28)



/*   The ULU header         */

typedef struct STR_DLU_ULU_HEADER
{
  UINT32  targetAddress                          ATTR_PACKED;   /* Target address for ULU                */
  CHAR8   targetHWType [ULU_TARGET_HW_TYPE_SIZE];               /* The target HW Type name (7-bit ASCII) */
  CHAR8   reserve [ULU_HEADER_RESERVE];                         /* Reserved area                         */
} TYPE_DLU_ULU_HEADER;

#define TYPE_DLU_ULU_HEADER_EXPECTED_SIZE (ULU_TARGET_HW_TYPE_SIZE + ULU_HEADER_RESERVE + 4)



/*   The DLU TPATH          */

typedef struct STR_DLU_TPATH_HEADER
{
  UINT32  usePermValue                           ATTR_PACKED;   /* == 0   -> don't use permission value  */
                                                                /* != 0   -> use permission value        */
  UINT32  useTargetPath                          ATTR_PACKED;   /* == 0   -> don't use target path info  */
                                                                /* != 0   -> use target path info        */
  UINT32  permValue                              ATTR_PACKED;   /* The numeric permission value          */
  CHAR8   targetPath [DLU_TARGET_PATH_SIZE];                    /* The target path for the DLU payload   */
} TYPE_DLU_TPATH_HEADER;

#define TYPE_DLU_TPATH_HEADER_EXPECTED_SIZE (DLU_TARGET_PATH_SIZE + 12)










#define DLUDATAREP_STRUCTURE_SIZE_CHECK                                    \
{                                                                          \
  { sizeof(TYPE_DLU_HEADER),       TYPE_DLU_HEADER_EXPECTED_SIZE },        \
  { sizeof(TYPE_DLU_ULU_HEADER),   TYPE_DLU_ULU_HEADER_EXPECTED_SIZE },    \
  { sizeof(TYPE_DLU_TPATH_HEADER), TYPE_DLU_TPATH_HEADER_EXPECTED_SIZE },  \
  { 0,                             0 }                                     \
}







#ifdef _MSC_VER
  /* restore safed packing alignment for structure members */
  #pragma pack( pop, before_dludatarep_h )
#endif /* defined _MSC_VER */





#endif /* defined DLUDATAREP_H */
