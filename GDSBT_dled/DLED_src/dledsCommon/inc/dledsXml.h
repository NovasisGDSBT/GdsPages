/*******************************************************************************
 * COPYRIGHT: (c) <year> Bombardier Transportation
 * Template: 3EST 207-5196 rev 0
 *******************************************************************************
 * %PCMS_HEADER_SUBSTITUTION_START%
 * COMPONENT:     <component name>
 *
 * ITEM-SPEC:     %PID%
 *
 * FILE:          %PM%
 *
 * REQ DOC:       <requirements document identity>
 * REQ ID:        <list of requirement identities>
 *
 * ABSTRACT:      <short description>
 *
 *******************************************************************************
 * HISTORY:
 %PL%
 *%PCMS_HEADER_SUBSTITUTION_END% 
 ******************************************************************************/

#ifndef DLEDS_XML_H
#define DLEDS_XML_H

#ifdef __cplusplus   /* to be compatible with C++ */
extern "C" {
#endif


/*******************************************************************************
 * INCLUDES
 */



/*******************************************************************************
 * DEFINES
 */
#define XML_MAX_LEVEL     10                  /* Max nesting of tokens */

#define XML_MAX_TOKLEN    255                 /* Max length of token string */
#define XML_MAX_TAGLEN    31                  /* Max length of tag string */

#define XML_OK            0
#define XML_ERROR         -1

/*******************************************************************************
 * TYPEDEFS
 */

typedef struct tag_DLU_InfoHeader
{
    char productId[32]; /* type="xs:string" minOccurs="0"/> */
    char type[32];      /* type="xs:string" minOccurs="0"/> */
    char name[32];      /* type="xs:string"/> */
    char fileName[64];  /* type="xs:string"/> */
    char version[32];   /* type="xs:string"/> */
    char created[32];   /* type="xs:unsignedInt" minOccurs="0"/> */
    char modified[32];  /* type="xs:boolean" minOccurs="0"/> */
    char size[32];      /* type="xs:string" minOccurs="0"/> */
    char checksum[32];  /* type="xs:string" minOccurs="0"/> */
    char crc32[32];     /* type="xs:string" minOccurs="0"/> */
    char address[32];   /* type="xs:string" minOccurs="0"/> */
    char supplier[32];  /* type="xs:string" minOccurs="0"/> */
}DLU_InfoHeader;

typedef struct tag_SCI_InfoHeader
{
    UINT16      schemaVersion;
    UINT16      schemaRelease;
    UINT16      schemaUpdate;
    UINT16      schemaEvolution;
    char        sciName[32];
    char        sciVersion[32];
    char        sciDeviceType[32];
    char        sciType[32];
    char        info[32];
    char        created[32];
}SCI_InfoHeader;

typedef struct tag_EDSP_InfoHeader
{
    UINT16      schemaVersion;
    UINT16      schemaRelease;
    UINT16      schemaUpdate;
    UINT16      schemaEvolution;
    char        edspName[32];
    char        edspVersion[32];
    char        edspEndDeviceType[32];
    char        edspType[32];
    char        edspSource[32];
    char        info[32];
    char        created[32];
}EDSP_InfoHeader;

typedef struct tag_DLU_Info
{
    DLU_InfoHeader      dluInfoHeader;
    void*               pNextDluInfo;
}DLU_Info;

typedef struct tag_SCI_Info
{
    SCI_InfoHeader      sciInfoHeader;
    UINT32              noOfDlu;
    DLU_Info*           pDluInfo;
    void*               pNextSciInfo;
}SCI_Info;

typedef struct tag_EDSP_Info
{
    EDSP_InfoHeader     edspInfoHeader;
    UINT32              noOfSci;
    SCI_Info*           pSciInfo;
}EDSP_Info;

/*******************************************************************************
 * GLOBAL VARIABLES
 */



/*******************************************************************************
 * GLOBAL FUNCTIONS
 */
int readDataFromInstStructure(
    char*           fileName,               /*  IN:  XML file */
    char*           pLevel,                 /* OUT: "Function", "EDPackage" or "SCI" */
    UINT32*         pNoTarFiles,            /* OUT: Number of TAR files found */
    char**          pListTarFileName,       /* OUT: String with TAR file names separated with <space> */
    UINT32*         pNoDluFiles,            /* OUT: Number of DLU files found */
    char**          pListDluFileName);      /* OUT: String with DLU file names separated with <space> */

int readDataFromInstEd(
    char*           fileName,              /*  IN:  XML file */
    char*           pLevel,                /* OUT: "EDSubPackage" */
    char*           pSource,               /* OUT: "BT" or "CU" */
    UINT32*         pNoSciFiles,           /* OUT: Number of SCI files found */
    char**          pListSciFileName,      /* OUT: String with SCI file names separated with "," */
    UINT32*         pNoMdluEdFiles,        /* OUT: Number of MDLU ED files found */
    char**          pListMdluEdFileName,   /* OUT: String with MDLU ED file names separated with "," */
    int*            clearNVRAM,            /* OUT: True or False */
    int*            clearFFS);              /* OUT: True or False */


int readDataFromSciInfo(   
    char*            fileName,              /*  IN: XML file */
    SCI_InfoHeader*  pSciInfoHeader);       /* OUT: Values read from XML file */
                            
int readDataFromEdspInfo(   
    char*            fileName,              /*  IN: XML file */
    EDSP_InfoHeader* pEdspInfoHeader);      /* OUT: Values read from XML file */

int dledsRetrieveCompleteSciInfo(   
	char*           fileName,               /*  IN: XML file */
  	SCI_Info*       pSciInfo);              /* OUT: Values read from SCI_Info XML file */

int dledsRetrieveCompleteEdspInfo(
    char*           fileName,               /*  IN: XML file */
    EDSP_Info*      pEdspInfo);             /* OUT: Values read from XML file */


#ifdef __cplusplus   /* to be compatible with C++ */
}
#endif

#endif                         /* DLEDS_XML_H */
