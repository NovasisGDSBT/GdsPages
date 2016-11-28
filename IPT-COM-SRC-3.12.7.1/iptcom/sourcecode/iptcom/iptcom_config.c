/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2014 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     : IPTrain
 *
 *  MODULE      : iptcom_config.c
 *
 *  ABSTRACT    : Configuration function for iptcom
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 * $Id: iptcom_config.c 33666 2014-07-17 14:43:01Z gweiss $
 *
 *  CR-7779 (Gerhard Weiss 2014-07-01)
 *          added check for receiving MD frame len (configurable)
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *       findings from TUEV-Assessment
 *
 *  CR-1356 (Bernd Loehr, 2010-10-08)
 * 			Additional function iptConfigAddDatasetExt to define dataset
 *			dependent un/marshalling. iptConfigAddDatasetID changed to call
 *			iptConfigAddDatasetExt.
 *
 *  Internal (Bernd Loehr, 2010-08-16) 
 * 			Old obsolete CVS history removed
 *
 * 
 ******************************************************************************/

/*******************************************************************************
*  INCLUDES */
#if defined(VXWORKS)
#include <vxWorks.h>
#endif

#include <string.h>
#include "iptcom.h"	      /* Common type definitions for IPT */
#include "vos.h"		      /* OS independent system calls */
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"


/*******************************************************************************
*  DEFINES
*/

/*******************************************************************************
*  TYPEDEFS
*/

typedef struct
{
   int level;
   UINT32 size;
   UINT16 alignment;
   UINT16 varSize;            /* Set TRUE if size is variable */
   UINT16 nLines;               /* No of lines in format table below */
   IPT_DATA_SET_FORMAT_INT *pFormat; /* Pointer to formatting table */
} DATASET_INFO;

/*******************************************************************************
*  LOCAL DATA */

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
NAME:       checkAlignEndian
ABSTRACT:   Checks that the CPU we are running on performs as expected 
            concerning natural alignment and big/little endian.
RETURNS:    0 if OK, !=0 if error
*/
static int checkAlignEndian(void)
{
   unsigned int i;
   union
   {
      struct
      {
         UINT8 a;
         UINT16 b;
         UINT8 c;
         UINT32 d;
      } s;
      UINT8 b[12];
   } data;
#if IS_BIGENDIAN
   UINT8 big_pattern[] = {11, 0, 0, 22, 33, 0, 0, 0, 0, 0, 0, 44};
#else
   UINT8 little_pattern[] = {11, 0, 22, 0, 33, 0, 0, 0, 44, 0, 0, 0};
#endif
   UINT8 *p;

   /* Prepare data structure with data */
   memset(&data, 0, sizeof(data));
   data.s.a = 11;
   data.s.b = 22;
   data.s.c = 33;
   data.s.d = 44;

   /* check that data has been allocated on bytes as expected */
#if IS_BIGENDIAN
   p = big_pattern;
#else
   p = little_pattern;
#endif

   for (i = 0; i < sizeof(data); i++)
   {
      if (data.b[i] != p[i])
      {
         return (int)IPT_ENDIAN_ALIGNMENT_ERROR;
      }
   }

   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       prepareDataset
ABSTRACT:   Calculate the size and alignment of one dataset. 
            Called recursively from iptPrepareDataset.
RETURNS:    0 if OK, !=0 if error, 
            IPT_MARSHALL_VAR_SIZE if variable size (not possible to prepare statically)
*/
static int prepareDataset(
   UINT32 datasetId,        /* Dataset ID */
   DATASET_INFO *pInfo)     /* Pointer to data set info structure */
{
   int ret = (int)IPT_OK;
   UINT16 nLines;
   UINT32 size;
   IPT_DATA_SET_FORMAT_INT *pFormat;
   IPT_CFG_DATASET_INT *pDataset;
   DATASET_INFO dsInfo;
   
   /* Restrict recursion */
   if (++pInfo->level > MAX_FORMAT_LEVEL)
      return (int)IPT_MARSHALL_MAX_LEVEL;
   
   /* Get pointer to dataset for this datasetId */
   pDataset = (IPT_CFG_DATASET_INT *) iptTabFind(&IPTGLOBAL(configDB.datasetTable), datasetId); /*lint !e826 Type cast OK */ 
   if (pDataset == NULL)
      return (int)IPT_MARSHALL_UNKNOWN_DATASETID;

   /* Calculate size and alignment of this dataset, but only if it is not already done */
   if (!pDataset->prepared)
   {
      /* We must calculate */
      pFormat = pDataset->pFormat;
      nLines = pDataset->nLines;
      pDataset->size = 0;
      pDataset->alignment = 1;
      pDataset->varSize = FALSE;

      while (nLines > 0)
      {
         if (pFormat->size == IPT_VAR_SIZE)
         {
            /* Variable data size, size is not possible to calculate but continue to get alignment */
            pDataset->varSize = TRUE;
            size = 0;
         }
         else
         {
            /* Fixed data size, get size from dataset formatting */
            size = pFormat->size;
         }

         if (pFormat->id > 0)
         {
            /* Dataset. Get size and alignment. */
            dsInfo.level = pInfo->level;
            dsInfo.size = 0;
            dsInfo.alignment = 1;
            dsInfo.varSize = FALSE;

            if ((ret = prepareDataset(pFormat->id, &dsInfo)) != 0)
               return ret;

            /* We have a size, alignment and varSize info for subordinate datasets */
            iptAlignStruct((BYTE **)(void *)&pDataset->size, dsInfo.alignment);
            iptAlignStruct((BYTE **)(void *)&dsInfo.size, dsInfo.alignment);
            pDataset->size += dsInfo.size * size;

            if (pDataset->alignment < dsInfo.alignment)
               pDataset->alignment = dsInfo.alignment;

            pDataset->varSize |= dsInfo.varSize;
            pFormat->nLines = dsInfo.nLines;
            pFormat->alignment = dsInfo.alignment;
            pFormat->pFormat = dsInfo.pFormat;
         }
         else
         {
            /* Basic data type, just get size and alignment */
            switch (pFormat->id)
            {
            case IPT_BOOLEAN8:
            case IPT_CHAR8:
            case IPT_INT8:
            case IPT_UINT8:
               /* 1 byte data, 1 byte alignment */
               pDataset->size += size;
               break;
               
            case IPT_INT16:
            case IPT_UINT16:
            case IPT_UNICODE16:
               /* 2 byte data, align destination address to multiple of 2 */
               iptAlign((BYTE **)(void *)&pDataset->size, ALIGNOF(UINT16));
               pDataset->size += 2 * size;
               
               if (pDataset->alignment < ALIGNOF(UINT16))
                  pDataset->alignment = ALIGNOF(UINT16);
               
               break;
               
            case IPT_INT32:
            case IPT_UINT32:
            case IPT_REAL32:
               /* 4 byte data, align destination address to multiple of 4 */
               iptAlign((BYTE **)(void *)&pDataset->size, ALIGNOF(UINT32));
               pDataset->size += 4 * size;
               
               if (pDataset->alignment < ALIGNOF(UINT32))
                  pDataset->alignment = ALIGNOF(UINT32);
               
               break;
               
            case IPT_INT64:
            case IPT_UINT64:
            case IPT_TIMEDATE48:
               /* 8 byte data, align destination address to struct alignment of UINT64 */
               iptAlign((BYTE **)(void *)&pDataset->size, ALIGNOF(UINT64ST));
               pDataset->size += 8 * size;
               
               if (pDataset->alignment < ALIGNOF(UINT64ST))
                  pDataset->alignment = ALIGNOF(UINT64ST);
               
               break;
               
            case IPT_STRING:
               /* 1 byte characters, 1 byte alignment, use maximum string size as size */
               pDataset->size += size;
               break;
               
            default: 
               IPTVosPrint2(IPT_WARN,
                            "prepareDataset ERROR Wrong data type=%d DatasetID=%d\n",
                            pFormat->id, datasetId);
               return(IPT_MARSHALL_UNKNOWN_DATASETID);
            }
         }
         
         pFormat++;  /* Get next formatting line */
         nLines--;
      }

      /* If variable size it is not possible to calculate the size of the dataset */
      if (pDataset->varSize)
         pDataset->size = 0;
      else {
         iptAlignStruct((BYTE **)(void *)&pDataset->size, pDataset->alignment);

      }

      pDataset->prepared = TRUE;
   }

   /* If variable size of dataset, set to 0 */
   if (pDataset->varSize)
      pInfo->size = 0;
   else
   {
      /* Align to size for this dataset, then add size of this dataset and max alignment */
      iptAlignStruct((BYTE **)(void *)&pInfo->size, pDataset->alignment);
      pInfo->size += pDataset->size;
   }
   
   if (pInfo->alignment < pDataset->alignment)
      pInfo->alignment = pDataset->alignment;

   pInfo->varSize |= pDataset->varSize;
   pInfo->pFormat = pDataset->pFormat;
   pInfo->nLines = pDataset->nLines;

   pInfo->level--;
   return (int)IPT_OK;
}


/*******************************************************************************
NAME:       prepareAllDataset
ABSTRACT:   Calculate size and alignment for all datasets.
            Try to prepare all datasets. If there are any that cannot be prepared
            it could be because it contains other datasets that are not yet prepared.
            Then retry until all datasets are prepared.
            Interrupt if we cannot get all ready.

RETURNS:    Numbers of not prepared
*/
static int prepareAllDataset(void)
{
   int i, nUnPrep = 0, nUnPrepOld;
   IPT_CFG_DATASET_INT *pDataset;

   do
   {
      nUnPrepOld = nUnPrep;
      nUnPrep = 0;
      pDataset = (IPT_CFG_DATASET_INT *) IPTGLOBAL(configDB.datasetTable.pTable); /*lint !e826 Type cast OK */
      
      for (i = 0; i < IPTGLOBAL(configDB.datasetTable.nItems); i++)
      {
         if (!pDataset[i].prepared)
         {
            if (iptPrepareDataset(pDataset[i].datasetId) != (int)IPT_OK)
               nUnPrep++;
         }
      }
   } while (nUnPrep > 0 && nUnPrep != nUnPrepOld);

   return nUnPrep;
}

/*******************************************************************************
NAME:       iptConfigAddFilePath
ABSTRACT:   Add configuration file name to IPTCom configuration database
RETURNS:    0 if OK, !=0 if not
*/
static int iptConfigAddFilePath(
   const char *path)   /* File path to XML file */
{
   int i;
   int ret;
   char *newPath;

   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      for (i = 0; i < IPTGLOBAL(configDB.fileTable.nItems); i++)
      {
         /* File name already added? */
         if (strcmp(path, (char *)IPTGLOBAL(configDB.fileTable.pTable)[i].key) == 0)
         {
            if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
            {
               IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
            }
            return((int)IPT_OK);
         }
      }
   
      newPath = (char *)IPTVosMalloc(strlen(path) + 1);
      if (newPath == NULL)
      {
         if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
         return (int)IPT_MEM_ERROR;
      }
   
      strcpy(newPath,path);

      ret = iptTabAdd(&IPTGLOBAL(configDB.fileTable), (IPT_TAB_ITEM_HDR *)((void *)&newPath));

      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      ret = (int)IPT_SEM_ERR;
   }

   return ret;  /*lint !e429 Custodial pointer OK*/
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       iptConfigInit
ABSTRACT:   Init of configuration database for IPTCom 
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigInit(void)
{
   int ret = (int)IPT_OK;
   IPT_CONFIG_COM_PAR comPar = {0, MD_DEF_QOS, MD_DEF_TTL};   /* Default QoS, TTL */

   /* Check alignment and big/little endian */
   if ((ret = checkAlignEndian()) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "CPU does not work with alignment or big/little endian as expected\n");
      return ret;
   }

   if (IPTVosCreateSem(&IPTGLOBAL(configDB.sem), IPT_SEM_FULL) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "Could not create config semaphore\n");
      return (int)IPT_ERROR;
   }

   if ((ret = iptTabInit(&IPTGLOBAL(configDB.exchgParTable), sizeof(IPT_CONFIG_EXCHG_PAR_EXT))) != (int)IPT_OK)
      return ret;

   if ((ret = iptTabInit(&IPTGLOBAL(configDB.pdSrcFilterParTable), sizeof(IPT_CONFIG_COMID_SRC_FILTER_PAR))) != (int)IPT_OK)
      return ret;

   if ((ret = iptTabInit(&IPTGLOBAL(configDB.destIdParTable), sizeof(IPT_CONFIG_COMID_DEST_ID_PAR))) != (int)IPT_OK)
      return ret;

   if ((ret = iptTabInit(&IPTGLOBAL(configDB.datasetTable), sizeof(IPT_CFG_DATASET_INT))) != (int)IPT_OK)
      return ret;

   if ((ret = iptTabInit(&IPTGLOBAL(configDB.comParTable), sizeof(IPT_CONFIG_COM_PAR_EXT))) != (int)IPT_OK)
      return ret;

   if ((ret = iptTabInit(&IPTGLOBAL(configDB.fileTable), sizeof(char *))) != (int)IPT_OK)
      return ret;

   /* Add default com-parameter for PD and MD */
   comPar.comParId = IPT_DEF_COMPAR_PD_ID;
   comPar.qos = PD_DEF_QOS;      /* Default QoS for PD */
   comPar.ttl = PD_DEF_TTL;      /* Default TTL for PD */
   if ((ret = iptConfigAddComPar(&comPar)) != (int)IPT_OK)
      return ret;

   comPar.comParId = IPT_DEF_COMPAR_MD_ID;
   comPar.qos = MD_DEF_QOS;      /* Default QoS for MD */
   comPar.ttl = MD_DEF_TTL;      /* Default TTL for MD */
   if ((ret = iptConfigAddComPar(&comPar)) != (int)IPT_OK)
      return ret;

   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptConfigDestroy
ABSTRACT:   Destroy configuration database for IPTCom 
RETURNS:    -
*/
void iptConfigDestroy(void)
{

   IPTGLOBAL(configDB.exchgParTable.initialized) = FALSE;
   IPTGLOBAL(configDB.pdSrcFilterParTable.initialized) = FALSE;
   IPTGLOBAL(configDB.destIdParTable.initialized) = FALSE;
   IPTGLOBAL(configDB.datasetTable.initialized) = FALSE;
   IPTGLOBAL(configDB.comParTable.initialized) = FALSE;
   IPTGLOBAL(configDB.fileTable.initialized) = FALSE;

   /* Leave allocated memory, it will be removed by IPTVosMem */

   IPTVosDestroySem(&IPTGLOBAL(configDB.sem));
}

/*******************************************************************************
NAME:       iptPrepareDataset
ABSTRACT:   Calculate size and alignment for a dataset.
RETURNS:    0 if OK, !=0 if error
*/
int iptPrepareDataset(
   UINT32 datasetId)  /* Dataset ID */
{
   int ret;
   DATASET_INFO info;
   
   /* Set up all info needed, then call next level function */
   info.level = 0;
   info.alignment = 1;
   info.size = 0;
   info.varSize = FALSE;
   
   ret = prepareDataset(datasetId, &info);
   
   iptAlignStruct((BYTE **)(void *)&info.size, info.alignment);

   return ret;
}

/*******************************************************************************
NAME:       iptPrepareAllDataset
ABSTRACT:   Calculate size and alignment for all datasets.
            Try to prepare all datasets. If there are any that cannot be prepared
            it could be because it contains other datasets that are not yet prepared.
            Then retry until all datasets are prepared.
            Interrupt if we cannot get all ready.

RETURNS:    0 if OK, !=0 if not
*/
int iptPrepareAllDataset(void)
{
   int i, nUnPrep = 0, nUnPrepOld;
   IPT_CFG_DATASET_INT *pDataset;

   do
   {
      nUnPrepOld = nUnPrep;
      nUnPrep = 0;
      pDataset = (IPT_CFG_DATASET_INT *) IPTGLOBAL(configDB.datasetTable.pTable); /*lint !e826 Type cast OK */
      
      for (i = 0; i < IPTGLOBAL(configDB.datasetTable.nItems); i++)
      {
         if (!pDataset[i].prepared)
         {
            if (iptPrepareDataset(pDataset[i].datasetId) != (int)IPT_OK)
               nUnPrep++;
         }
      }
   } while (nUnPrep > 0 && nUnPrep != nUnPrepOld);

   if (nUnPrep > 0)
   {
      IPTVosPrint0(IPT_CONFIG, "Could not calculate size/alignment for all datasets\n");
      return (int)IPT_ERROR;
   }

   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       iptAddConfig
ABSTRACT:   Parse IPTCom XML configuration file and add IPTCom configuration
            data.
RETURNS:    0 if OK, !=0 if not
*/
int iptAddConfig(
   const char *pXMLPath) /* path to configuration XML file */
{
   int ret;
   int res = (int)IPT_OK;
   int nUnPrep;   /* No of unprepared datasets */

   if (pXMLPath == NULL)
   {
      IPTVosPrint0(IPT_ERR, "No config file given\n");
      return((int)IPT_INVALID_PAR);   
   }
   
   /* Get number of already excisting unprepared dataset */
   nUnPrep = prepareAllDataset();

   if ((ret = iptConfigParseXML(pXMLPath)) != (int)IPT_OK)
      res = ret;

   /* Calculate size and alignment for all datasets 
      and check if the number of unprepared dataset has been increased */
   if ((prepareAllDataset()) > nUnPrep)
   {
      IPTVosPrint0(IPT_ERR, "Could not calculate size/alignment for all datasets\n");
      res = ret;
   }

   /* Save path of config file */
   if ((ret = iptConfigAddFilePath(pXMLPath)) != (int)IPT_OK)
      res = ret;

   return(res);
}

/*******************************************************************************
NAME:       IPTCom_addConfig
ABSTRACT:   Parse IPTCom XML configuration file and add IPTCom configuration
            data.
RETURNS:    0 if OK, !=0 if not
*/
int IPTCom_addConfig(
   const char *pXMLPath) /* path to configuration XML file */
{
   int ret;

   if (pXMLPath == NULL)
   {
      IPTVosPrint0(IPT_ERR, "No config file given\n");
      return((int)IPT_INVALID_PAR);   
   }
   
   /* Debug settings */
   ret = iptConfigParseXML4Dbg(pXMLPath);
   if (ret != (int)IPT_OK)
      return ret;
   
   if ((ret = iptAddConfig(pXMLPath)) != (int)IPT_OK)
      return ret;

   return((int)IPT_OK);
}

/*******************************************************************************
NAME:       iptConfigAddExchgParExt
ABSTRACT:   Add exchange parameters to IPTCom configuration database.
            The structure should be filled with relevant data before call.
            Strings will be copied to IPTCom memory and can therefore be temporary.
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigAddExchgParExt(
   const IPT_CONFIG_EXCHG_PAR_EXT *pExchgPar)   /* Pointer to structure */
{
   int ret = (int)IPT_OK, len;
   IPT_CONFIG_EXCHG_PAR_EXT par, *pExistPar;
   char *pString;

   memcpy(&par, pExchgPar, sizeof(par));

   /* Copy all strings to dynamic memory pool */

   /* MD send parameters  */
   if (pExchgPar->mdSendPar.pSourceURI == NULL || 
      (len = strlen(pExchgPar->mdSendPar.pSourceURI)) == 0)
   {
      par.mdSendPar.pSourceURI = (char *)NULL;
   }
   else
   {
      pString = (char *)IPTVosMalloc(len + 1);
      if (pString == NULL)
         return (int)IPT_MEM_ERROR;

      memcpy(pString, pExchgPar->mdSendPar.pSourceURI, len + 1);
      par.mdSendPar.pSourceURI = pString;
   }

   if (pExchgPar->mdSendPar.pDestinationURI == NULL ||
      (len = strlen(pExchgPar->mdSendPar.pDestinationURI)) == 0)
   {
      par.mdSendPar.pDestinationURI = (char *)NULL;
   }
   else
   {
      pString = (char *)IPTVosMalloc(len + 1);
      if (pString == NULL)
         return (int)IPT_MEM_ERROR;

      memcpy(pString, pExchgPar->mdSendPar.pDestinationURI, len + 1);
      par.mdSendPar.pDestinationURI = pString;
   }

   /* PD receive parameters */
   if (pExchgPar->pdRecPar.pSourceURI == NULL ||
      (len = strlen(pExchgPar->pdRecPar.pSourceURI)) == 0)
   {
      par.pdRecPar.pSourceURI = (char *)NULL;
   }
   else
   {
      pString = (char *)IPTVosMalloc(len + 1);
      if (pString == NULL)
         return (int)IPT_MEM_ERROR;

      memcpy(pString, pExchgPar->pdRecPar.pSourceURI, len + 1);
      par.pdRecPar.pSourceURI = pString;
   }

   /* PD send parameters */
   if (pExchgPar->pdSendPar.pDestinationURI == NULL ||
      (len = strlen(pExchgPar->pdSendPar.pDestinationURI)) == 0)
   {
      par.pdSendPar.pDestinationURI = (char *)NULL;
   }
   else
   {
      pString = (char *)IPTVosMalloc(len + 1);
      if (pString == NULL)
         return (int)IPT_MEM_ERROR;

      memcpy(pString, pExchgPar->pdSendPar.pDestinationURI, len + 1);
      par.pdSendPar.pDestinationURI = pString;
   }

   /* Add structure to Exchange Parameter table in Configuration DB */
   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      ret = iptTabAdd(&IPTGLOBAL(configDB.exchgParTable), (IPT_TAB_ITEM_HDR *)((void *)&par));
      if (ret != (int)IPT_OK)
      {
         if (ret == (int)IPT_TAB_ERR_EXISTS)
         {
            /*lint -save -sem( iptTabFind, @p==1n)   iptTabFind will return a pointer because of the error IPT_TAB_ERR_EXISTS */   
            /* check if it contains the same values */
            pExistPar = (IPT_CONFIG_EXCHG_PAR_EXT *)iptTabFind(&IPTGLOBAL(configDB.exchgParTable), par.comId); /*lint !e826 Type cast OK */
           
            /*
             TCMS_PLATFORM_CR_3477, findings from TUEV-Assessment
             
             GW, 2012-04-11:
             The if statement below checks, if the two structures contain equivalent info.
             The original code was hard to read, as it consisted of 64 parenthesis, 23 And and 4 Or operators...
             I decided to write down two #define macros in order to get a bigger picture, although now
             these look ugly.
             */
             
/* #define() to check if two parameters are equal */
#define equalPar(x)  (((par.x) == (pExistPar->x)))
            
/* #define() to check, if two strings are both NULL or equal (strcmp is not defined for NULL parameters) */
#define equalStr(x)  ((((par.x) == NULL) && (pExistPar->x) == NULL) \
                     || ((par.x != NULL) && (pExistPar->x != NULL) \
                        && iptStrcmp(par.x,pExistPar->x) == 0))
            
            /* now we can start */   
            if (   equalPar(datasetId)
                && equalPar(comParId)
                && equalStr(mdSendPar.pSourceURI)
                && equalPar(mdSendPar.ackTimeOut)
                && equalPar(mdSendPar.responseTimeOut)
                && equalStr(mdSendPar.pDestinationURI)
                && equalPar(pdRecPar.timeoutValue)
                && equalPar(pdRecPar.validityBehaviour)
                && equalStr(pdRecPar.pSourceURI)
                && equalPar(pdSendPar.cycleTime)
                && equalPar(pdSendPar.redundant)
                && equalStr(pdSendPar.pDestinationURI)
                )
            {
               ret = (int)IPT_OK;
            }
            else
            {
               IPTVosPrint1(IPT_ERR, "Definitions for comId=%d already exist with different values\n",par.comId);
            }
            /*lint -restore */
         }
         /* Problem adding to table, free allocated strings */
         if (par.mdSendPar.pSourceURI != NULL)
            IPTVosFree((unsigned char *)par.mdSendPar.pSourceURI);
         if (par.mdSendPar.pDestinationURI != NULL)
            IPTVosFree((unsigned char *)par.mdSendPar.pDestinationURI);
         if (par.pdRecPar.pSourceURI != NULL)
            IPTVosFree((unsigned char *)par.pdRecPar.pSourceURI);
         if (par.pdSendPar.pDestinationURI != NULL)
            IPTVosFree((unsigned char *)par.pdSendPar.pDestinationURI);
      }

      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      ret = (int)IPT_SEM_ERR;
   }

   return ret;
}

/*******************************************************************************
NAME:       iptConfigAddExchgPar
ABSTRACT:   Add exchange parameters to IPTCom configuration database.
            The structure should be filled with relevant data before call.
            Strings will be copied to IPTCom memory and can therefore be temporary.
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigAddExchgPar(
   const IPT_CONFIG_EXCHG_PAR *pExchgPar)   /* Pointer to structure */
{
   int ret;
   IPT_CONFIG_EXCHG_PAR_EXT par;
   
   par.comId = pExchgPar->comId;
   par.datasetId = pExchgPar->datasetId;
   par.comParId = pExchgPar->comParId;
   
   /* MD send parameters, use default values for parameters not included in the
      structure IPT_CONFIG_EXCHG_PAR */
   
   par.mdSendPar.ackTimeOut = 0;
   par.mdSendPar.responseTimeOut = 0;
   
   par.mdSendPar.pDestinationURI = pExchgPar->mdSendPar.pDestinationURI;
   par.mdSendPar.pSourceURI = pExchgPar->mdRecPar.pSourceURI;

   /* PD receive parameters */
   par.pdRecPar.pSourceURI = pExchgPar->pdRecPar.pSourceURI;
   par.pdRecPar.timeoutValue = pExchgPar->pdRecPar.timeoutValue;
   par.pdRecPar.validityBehaviour = pExchgPar->pdRecPar.validityBehaviour;
   
   /* PD send parameters */
   par.pdSendPar.pDestinationURI = pExchgPar->pdSendPar.pDestinationURI;
   par.pdSendPar.cycleTime = pExchgPar->pdSendPar.cycleTime;
   par.pdSendPar.redundant = pExchgPar->pdSendPar.redundant;
   
   ret = iptConfigAddExchgParExt(&par);

   return ret;
}

/*******************************************************************************
NAME:       iptConfigAddPdSrcFilterPar
ABSTRACT:   Add PD source filter parameters to IPTCom configuration database.
            String will be copied to IPTCom memory and can therefore be temporary.
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigAddPdSrcFilterPar(
   const UINT32 comId,        /* ComID */
   const UINT32 filterId,     /* Filter ID */
   const char *pSourceURI)   /* Pointer to source filter URI string */
{
   int ret = (int)IPT_OK, len;
   IPT_CONFIG_COMID_SRC_FILTER_PAR comIdPar, *pExistcomIdPar;
   IPT_CONFIG_SRC_FILTER_PAR filterPar, *pExistFiltPar;
   char *pString;

   if (comId == 0)
   {
      IPTVosPrint0(IPT_ERR, "iptConfigAddPdSrcFilterPar: ComID=0 not allowed\n");
      return (int)IPT_INVALID_PAR;
   }
   if (filterId == 0)
   {
      IPTVosPrint0(IPT_ERR, "iptConfigAddPdSrcFilterPar: filterId=0 not allowed\n");
      return (int)IPT_INVALID_PAR;
   }
   /* Copy all strings to dynamic memory pool */

   /* PD receive parameters */
   if (pSourceURI == NULL ||
      (len = strlen(pSourceURI)) == 0)
   {
      IPTVosPrint1(IPT_ERR,
                   "iptConfigAddPdSrcFilterPar: No source URI for ComID=%d\n",
                   comId);
      return (int)IPT_INVALID_PAR;
   }
   else
   {
      pString = (char *)IPTVosMalloc(len + 1);
      if (pString == NULL)
         return (int)IPT_MEM_ERROR;

      memcpy(pString, pSourceURI, len + 1);
   }
   
   filterPar.filterId = filterId;
   filterPar.pSourceURI = pString;

   /* Add structure to Exchange Parameter table in Configuration DB */
   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      /* Filters for comid already exists? */
      pExistcomIdPar = (IPT_CONFIG_COMID_SRC_FILTER_PAR *)iptTabFind(&IPTGLOBAL(configDB.pdSrcFilterParTable), comId); /*lint !e826 Type cast OK */
      if (pExistcomIdPar != NULL)
      {
         ret = iptTabAdd(pExistcomIdPar->pFiltTab, (IPT_TAB_ITEM_HDR *)((void *)&filterPar));
         if (ret != (int)IPT_OK)
         {
            if (ret == (int)IPT_TAB_ERR_EXISTS)
            {
               /*lint -save -sem( iptTabFind, @p==1n)   iptTabFind will return a pointer because of the error */   
               /* check if it contains the same values */
               pExistFiltPar = (IPT_CONFIG_SRC_FILTER_PAR *)iptTabFind(pExistcomIdPar->pFiltTab, filterId); /*lint !e826 Type cast OK */
              
               if (   (filterPar.filterId == pExistFiltPar->filterId) 
                   && (iptStrcmp(filterPar.pSourceURI,pExistFiltPar->pSourceURI) == 0)
                  )
               {
                  ret = (int)IPT_OK;
               }
               else
               {
                  IPTVosPrint2(IPT_ERR,
                   "Definitions for comId=%d filterID=%d already exist with different values\n",
                   comId,filterId);
               }
               /*lint -restore */
           }
            /* Problem adding to table or already exist, free allocated strings */
            IPTVosFree((unsigned char *)pString);
         }
      }
      else
      {
         comIdPar.pFiltTab = (IPT_TAB_HDR *)IPTVosMalloc(sizeof(IPT_TAB_HDR));
         if (comIdPar.pFiltTab != NULL)
         {
            comIdPar.pFiltTab->initialized = FALSE;
            ret = iptTabInit(comIdPar.pFiltTab , sizeof(IPT_CONFIG_SRC_FILTER_PAR));
            if (ret == (int)IPT_OK)
            {
               ret = iptTabAdd(comIdPar.pFiltTab, (IPT_TAB_ITEM_HDR *)((void *)&filterPar));
               if (ret == (int)IPT_OK)
               {
                  comIdPar.comId = comId;  
                  ret = iptTabAdd(&IPTGLOBAL(configDB.pdSrcFilterParTable), (IPT_TAB_ITEM_HDR *)((void *)&comIdPar));
               }
            }
         }
         else
         {
            ret = (int)IPT_MEM_ERROR;
         }
   
         if (ret != (int)IPT_OK)
         {
            /* Problem adding to table or already exist, free allocated strings */
            IPTVosFree((unsigned char *)pString);
            if (comIdPar.pFiltTab != NULL)
            {
               IPTVosFree((BYTE *)comIdPar.pFiltTab);
            }
         }
      }

      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      ret = (int)IPT_SEM_ERR;
   }

   return ret;
}

/*******************************************************************************
NAME:       iptConfigAddDestIdPar
ABSTRACT:   Add destination Id parameters to IPTCom configuration database.
            String will be copied to IPTCom memory and can therefore be temporary.
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigAddDestIdPar(
   const UINT32 comId,       /* ComID */
   const UINT32 destId,      /* Destination ID */
   const char *pDestURI)   /* Pointer to destination URI string */
{
   int ret = (int)IPT_OK, len;
   IPT_CONFIG_COMID_DEST_ID_PAR comIdPar, *pExistcomIdPar;
   IPT_CONFIG_DEST_ID_PAR destIdPar, *pExistDestIdPar;
   char *pString;

   if (comId == 0)
   {
      IPTVosPrint0(IPT_ERR, "iptConfigAddDestIdPar: ComID=0 not allowed\n");
      return (int)IPT_INVALID_PAR;
   }
   if (destId == 0)
   {
      IPTVosPrint0(IPT_ERR, "iptConfigAddDestIdPar: destId=0 not allowed\n");
      return (int)IPT_INVALID_PAR;
   }

   /* Copy string to dynamic memory pool */
   if (pDestURI == NULL ||
      (len = strlen(pDestURI)) == 0)
   {
      IPTVosPrint2(IPT_ERR,
                   "iptConfigAddDestIdPar: No destination URI for ComID=%d DestID=%d\n",
                   comId, destId);
      return (int)IPT_INVALID_PAR;
   }
   else
   {
      pString = (char *)IPTVosMalloc(len + 1);
      if (pString == NULL)
         return (int)IPT_MEM_ERROR;

      memcpy(pString, pDestURI, len + 1);
   }
   
   destIdPar.destId = destId;
   destIdPar.pDestURI = pString;

   /* Add structure to Exchange Parameter table in Configuration DB */
   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      /* Filters for comid already exists? */
      pExistcomIdPar = (IPT_CONFIG_COMID_DEST_ID_PAR *)iptTabFind(&IPTGLOBAL(configDB.destIdParTable), comId); /*lint !e826 Type cast OK */
      if (pExistcomIdPar != NULL)
      {
         ret = iptTabAdd(pExistcomIdPar->pDestIdTab, (IPT_TAB_ITEM_HDR *)((void *)&destIdPar));
         if (ret != (int)IPT_OK)
         {
            if (ret == (int)IPT_TAB_ERR_EXISTS)
            {
               /*lint -save -sem( iptTabFind, @p==1n)   iptTabFind will return a pointer because of the error */   
               /* check if it contains the same values */
               pExistDestIdPar = (IPT_CONFIG_DEST_ID_PAR *)iptTabFind(pExistcomIdPar->pDestIdTab, destId); /*lint !e826 Type cast OK */
              
               if (   (destIdPar.destId == pExistDestIdPar->destId) 
                   && (iptStrcmp(destIdPar.pDestURI,pExistDestIdPar->pDestURI) == 0)
                  )
               {
                  ret = (int)IPT_OK;
               }
               else
               {
                  IPTVosPrint2(IPT_ERR,
                   "Definitions for ComId=%d Destination ID=%d already exist with different values\n",
                   comId,destId);
               }
               /*lint -restore */
            }
            /* Problem adding to table or already exist, free allocated strings */
            IPTVosFree((unsigned char *)pString);
         }
      }
      else
      {
         comIdPar.pDestIdTab = (IPT_TAB_HDR *)IPTVosMalloc(sizeof(IPT_TAB_HDR));
         if (comIdPar.pDestIdTab != NULL)
         {
            comIdPar.pDestIdTab->initialized = FALSE;
            ret = iptTabInit(comIdPar.pDestIdTab , sizeof(IPT_CONFIG_DEST_ID_PAR));
            if (ret == (int)IPT_OK)
            {
               ret = iptTabAdd(comIdPar.pDestIdTab, (IPT_TAB_ITEM_HDR *)((void *)&destIdPar));
               if (ret == (int)IPT_OK)
               {
                  comIdPar.comId = comId;  
                  ret = iptTabAdd(&IPTGLOBAL(configDB.destIdParTable), (IPT_TAB_ITEM_HDR *)((void *)&comIdPar));
               }
            }
         }
         else
         {
            ret = (int)IPT_MEM_ERROR;
         }
   
         if (ret != (int)IPT_OK)
         {
            /* Problem adding to table or already exist, free allocated strings */
            IPTVosFree((unsigned char *)pString);
            if (comIdPar.pDestIdTab != NULL)
            {
               IPTVosFree((BYTE *)comIdPar.pDestIdTab);
            }
         }
      }

      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      ret = (int)IPT_SEM_ERR;
   }

   return ret;
}

/*******************************************************************************
NAME:       iptConfigAddDatasetId
ABSTRACT:   Add dataset (parameters + format) to IPTCom configuration database
            The formatting table will be copied to IPTCom memory and can
            therefore be temporary.
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigAddDatasetId(
   UINT32 datasetId,            /* Dataset ID */
   UINT16 nLines,               /* No of lines in format table below */
   const IPT_DATASET_FORMAT *pDatasetFormat) /* Pointer to dataset format table */
{
   IPT_CONFIG_DATASET_EXT	dataSet;

   memset(&dataSet, 0, sizeof(dataSet));

   dataSet.datasetId = datasetId;
   dataSet.nLines = nLines;
   dataSet.disableMarshalling = 0;
   dataSet.reserved1 = 0;
   dataSet.reserved2 = 0;

   return iptConfigAddDatasetExt(&dataSet, pDatasetFormat);
}

/*******************************************************************************
NAME:       iptConfigAddDatasetExt
ABSTRACT:   Add dataset (parameters + format) to IPTCom configuration database
            The formatting table will be copied to IPTCom memory and can
            therefore be temporary.
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigAddDatasetExt(
   const IPT_CONFIG_DATASET_EXT *pConfigData,	/* extended config data	*/
   const IPT_DATASET_FORMAT *pDatasetFormat)	/* Pointer to dataset format table */
{
   int i;
   int ret = (int)IPT_OK, len;
   IPT_CFG_DATASET_INT dataset;            
   IPT_CFG_DATASET_INT *pExistDataset;

   /* Check parameters	*/

   if (pConfigData == NULL || pDatasetFormat == NULL ||
       pConfigData->reserved1 != 0 ||
       pConfigData->reserved2 != 0)
   {
      IPTVosPrint0(IPT_ERR, "iptConfigAddDatasetExt: Parameter error\n");
      return IPT_INVALID_PAR;
   }

   /* Set size and alignment to 0, these will be calculated later in 
      iptPrepareAllDataset */
   dataset.datasetId = pConfigData->datasetId;
   dataset.size = 0;
   dataset.alignment = 0;
   dataset.nLines = pConfigData->nLines;
   dataset.prepared = FALSE;
   dataset.varSize = FALSE;
   dataset.disableMarshalling = pConfigData->disableMarshalling;
  
   /* Create a dataset format area that is big enough, including leading and 
      trailing lines. */
   len = dataset.nLines * sizeof(IPT_DATA_SET_FORMAT_INT);
   dataset.pFormat = (IPT_DATA_SET_FORMAT_INT *) IPTVosMalloc(len); /*lint !e433 !e826 Size is OK but calculated */
   if (dataset.pFormat == NULL)
   {
      IPTVosPrint2(IPT_ERR, "iptConfigAddDatasetExt: Couldn't allocate memory for dataset Id=%d. Requested size=%d\n",
                      dataset.datasetId, len);
      return (int)IPT_MEM_ERROR;
   }

   /* Copy format from caller */
   for (i=0; i<dataset.nLines; i++)
   {
      dataset.pFormat[i].id =  pDatasetFormat[i].id;  
      dataset.pFormat[i].size =  pDatasetFormat[i].size;  
      dataset.pFormat[i].nLines =  0;  
      dataset.pFormat[i].alignment =  0;  
      dataset.pFormat[i].pFormat =  NULL;  
   }

   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      ret = iptTabAdd(&IPTGLOBAL(configDB.datasetTable), (IPT_TAB_ITEM_HDR *)((void *)&dataset));

      if (ret == (int)IPT_OK)
      {
         /* Check all datasets to calculate size and alignment */
         (void)iptPrepareAllDataset();
      }
      else
      {
         if (ret == (int)IPT_TAB_ERR_EXISTS)
         {
            /*lint -save -sem( iptTabFind, @p==1n)   iptTabFind will return a pointer because of the error */   
            /* check if it contains the same values */
            pExistDataset = (IPT_CFG_DATASET_INT *)iptTabFind(&IPTGLOBAL(configDB.datasetTable),
                                                             dataset.datasetId);  /*lint !e826 Type cast OK */

            ret = (int)IPT_OK;
            for (i=0; i<dataset.nLines; i++)
            {
               if (dataset.pFormat[i].id != pExistDataset->pFormat[i].id)
               {
                  ret = (int)IPT_TAB_ERR_EXISTS;
                  break;   
               }
               if (dataset.pFormat[i].size != pExistDataset->pFormat[i].size)
               {
                  ret = (int)IPT_TAB_ERR_EXISTS;
                  break;   
               }
            }
            
            if (ret != (int)IPT_OK)
            {
               IPTVosPrint1(IPT_ERR,
               "Definitions for dataSetId=%d already exist with different values\n",
                            dataset.datasetId);
            }
            /*lint -restore */
         }
         IPTVosFree((BYTE *)(dataset.pFormat));
      }

      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosFree((BYTE *)(dataset.pFormat));
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      ret = (int)IPT_SEM_ERR;
   }

   return ret;
}

/*******************************************************************************
NAME:       iptConfigAddDataset
ABSTRACT:   Add dataset (parameters + format) to IPTCom configuration database
            The structures should be filled with relevant data before call.
            The formatting table will be copied to IPTCom memory and can
            therefore be temporary.
            In the Dataset structure following data should be set: datasetId, 
            nLines and pointer to formatting table. 
            The rest is calculated by IPTCom.
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigAddDataset(
   IPT_CONFIG_DATASET *pDataset,             /* Pointer to dataset structure */
   const IPT_DATASET_FORMAT *pDatasetFormat) /* Pointer to dataset format table */
{
   return(iptConfigAddDatasetId(pDataset->datasetId, pDataset->nLines, pDatasetFormat));
}

/*******************************************************************************
NAME:       iptConfigAddComPar
ABSTRACT:   Add communication parameters to IPTCom configuration database
            The structure should be filled with relevant data before call.
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigAddComPar(
   const IPT_CONFIG_COM_PAR *pComPar)   /* Pointer to structure */
{
   int ret = (int)IPT_OK;
   int res;
   IPT_CONFIG_COM_PAR_EXT comPar, *pExistComPar;

   /* Copy the communication parameters from the call */
   comPar.comParId = pComPar->comParId;
   comPar.qos = pComPar->qos;
   comPar.ttl = pComPar->ttl;
   comPar.pdSendSocket = 0;
   comPar.mdSendSocket = 0;

   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      ret = iptTabAdd(&IPTGLOBAL(configDB.comParTable), (IPT_TAB_ITEM_HDR *) &comPar);

      if (ret == (int)IPT_OK)
      {
         pExistComPar = (IPT_CONFIG_COM_PAR_EXT *)iptTabFind(&IPTGLOBAL(configDB.comParTable),
                                                             comPar.comParId);   /*lint !e826 Type cast OK */

#ifdef IF_WAIT_ENABLE
         if (IPTGLOBAL(configDB.finish_socket_creation) == 0)
         {
            if (!IPTGLOBAL(ifWaitSend))
            {
               /* Create send sockets for this communication pararameters */
               res = IPTPDSendSocketCreate(pExistComPar);
               if (res != IPT_OK)
               {
                  ret = res;
               }
               res = IPTMDSendSocketCreate(pExistComPar);
               if (res != IPT_OK)
               {
                  ret = res;
               }
            }
            else
            {
               IPTGLOBAL(configDB.finish_socket_creation) = 1;
            }
         }
#else
         /* Create send sockets for this communication pararameters */
         res = IPTPDSendSocketCreate(pExistComPar);
         if (res != IPT_OK)
         {
            ret = res;
         }
         res = IPTMDSendSocketCreate(pExistComPar);
         if (res != IPT_OK)
         {
            ret = res;
         }
#endif 
      }
      else if (ret == (int)IPT_TAB_ERR_EXISTS)
      {
         /*lint -save -sem( iptTabFind, @p==1n)   iptTabFind will return a pointer because of the error */   
         /* check if it contains the same values */
         pExistComPar = (IPT_CONFIG_COM_PAR_EXT *)iptTabFind(&IPTGLOBAL(configDB.comParTable),
                                                         pComPar->comParId); /*lint !e826 Type cast OK */

         if (   (pComPar->qos == pExistComPar->qos) 
             && (pComPar->ttl == pExistComPar->ttl))
         {
            ret = (int)IPT_OK;
         }
         else
         {
            IPTVosPrint1(IPT_ERR,
               "Definitions for comParId=%d already exist with different values\n",
                         pComPar->comParId);
         }
         /*lint -restore */
     }
  
      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      ret = (int)IPT_SEM_ERR;
   }

   return ret;
}

/*******************************************************************************
NAME:       iptFinishConfig
ABSTRACT:   Add ownn configuration to IPTCom configuration database
            Called after TDC is ready
RETURNS:    -
*/
void iptFinishConfig(void)
{
   int i;
   int res;
   int ret = (int)IPT_OK;
   char *path;

   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      for (i = 0; i < IPTGLOBAL(configDB.fileTable.nItems); i++)
      {
         path = (char *)IPTGLOBAL(configDB.fileTable.pTable)[i].key;
         if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }

         res = iptConfigParseOwnUriXML(path);
         if (res == (int)IPT_TDC_NOT_READY)
         {
            IPTGLOBAL(configDB.finish_addr_resolv) = 1;
            return;
         }
         ret = IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER);
         if (ret != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
            return;
         }
      }

      IPTGLOBAL(configDB.finish_addr_resolv) = 0;

      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }
}

/*******************************************************************************
NAME:       iptConfigGetExchgPar
ABSTRACT:   Gets the exchange parameters for a specified ComId
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigGetExchgPar(
   UINT32 comId,                      /* Com ID */
   IPT_CONFIG_EXCHG_PAR_EXT *pExchgPar)   /* Pointer to output structure */
{
   int ret = (int)IPT_OK;
   IPT_CONFIG_EXCHG_PAR_EXT *p;

   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      p = (IPT_CONFIG_EXCHG_PAR_EXT *)iptTabFind(&IPTGLOBAL(configDB.exchgParTable),
                                             comId);  /*lint !e826 Type cast OK */
   
      if (p == NULL)
      {
         memset(pExchgPar, 0, sizeof(IPT_CONFIG_EXCHG_PAR_EXT));
         if (IPTGLOBAL(configDB.finish_addr_resolv) != 0)
         {
            ret = (int)IPT_TDC_NOT_READY;
         }
         else
         {
            ret = (int)IPT_NOT_FOUND;
         }
      }
      else
      {
         memcpy(pExchgPar, p, sizeof(IPT_CONFIG_EXCHG_PAR_EXT));
      }

      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      ret = (int)IPT_SEM_ERR;
   }

   return ret;
}

/*******************************************************************************
NAME:       iptConfigGetPdSrcFiltPar
ABSTRACT:   Gets the source filter URI string for for a specified ComId and
            filter Id
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigGetPdSrcFiltPar(
   UINT32 comId,       /* Com ID */
   UINT32 filterId,    /* Filter ID */
   const char **ppSourceUri)   /* Pointer to output PD filter source URI */
{
   int ret;
   IPT_CONFIG_COMID_SRC_FILTER_PAR *pComidFiltTab;
   IPT_CONFIG_SRC_FILTER_PAR *pFiltPar = (IPT_CONFIG_SRC_FILTER_PAR *)NULL;

   ret = IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      pComidFiltTab = (IPT_CONFIG_COMID_SRC_FILTER_PAR *)iptTabFind(&IPTGLOBAL(configDB.pdSrcFilterParTable),
                                                                    comId);  /*lint !e826 Type cast OK */
   
      if (pComidFiltTab != NULL)
      {
         pFiltPar = (IPT_CONFIG_SRC_FILTER_PAR *)iptTabFind(pComidFiltTab->pFiltTab,
                                                            filterId); /*lint !e826 Type cast OK */
      }

      if (pFiltPar != NULL)
      {
         *ppSourceUri = pFiltPar->pSourceURI;   
      }
      else
      {
         if (IPTGLOBAL(configDB.finish_addr_resolv) != 0)
         {
            ret = (int)IPT_TDC_NOT_READY;
         }
         else
         {
            IPTVosPrint2(IPT_ERR,
                        "iptConfigGetPdSrcFiltPar: Could not find Filter ID=%d for ComId=%d in config DB\n",
                        filterId, comId);
            ret = (int)IPT_NOT_FOUND;
         }           
         *ppSourceUri = (char *)NULL;
      }
      
      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "iptConfigGetPdSrcFiltPar: IPTVosGetSem ERROR\n");
   }
   
   
   return ret;
}

/*******************************************************************************
NAME:       iptConfigGetDestIdPar
ABSTRACT:   Gets the destination URI string for for a specified ComId and
            destination Id
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigGetDestIdPar(
   UINT32 comId,       /* Com ID */
   UINT32 destId,    /* Filter ID */
   const char **ppDestUri)   /* Pointer to output PD filter source URI */
{
   int ret;
   IPT_CONFIG_COMID_DEST_ID_PAR *pComidDestIdTab;
   IPT_CONFIG_DEST_ID_PAR *pDestIdPar = (IPT_CONFIG_DEST_ID_PAR *)NULL;

   ret = IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      pComidDestIdTab = (IPT_CONFIG_COMID_DEST_ID_PAR *)iptTabFind(&IPTGLOBAL(configDB.destIdParTable),
                                                                    comId);  /*lint !e826 Type cast OK */
   
      if (pComidDestIdTab != NULL)
      {
         pDestIdPar = (IPT_CONFIG_DEST_ID_PAR *)iptTabFind(pComidDestIdTab->pDestIdTab,
                                                            destId); /*lint !e826 Type cast OK */
      }

      if (pDestIdPar != NULL)
      {
         *ppDestUri = pDestIdPar->pDestURI;   
      }
      else
      {
         if (IPTGLOBAL(configDB.finish_addr_resolv) != 0)
         {
            ret = (int)IPT_TDC_NOT_READY;
         }
         else
         {
            ret = (int)IPT_NOT_FOUND;
         }           
         *ppDestUri = (char *)NULL;
      }
      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "iptConfigGetDestIdPar: IPTVosGetSem ERROR\n");
   }
   
   
   return ret;
}

/*******************************************************************************
NAME:       iptConfigGetComPar
ABSTRACT:   Gets the communication parameters for a specified Communication 
            parameter ID
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigGetComPar(
   UINT32 comParId,               /* Communication parameter ID */
   IPT_CONFIG_COM_PAR_EXT *pComPar)   /* Pointer to output structure */
{
   int ret;
   IPT_CONFIG_COM_PAR_EXT *p;

   ret = IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      p = (IPT_CONFIG_COM_PAR_EXT *) iptTabFind(&IPTGLOBAL(configDB.comParTable),
                                            comParId); /*lint !e826 Type cast OK */
   
      if (p == NULL)
      {
         memset(pComPar, 0, sizeof(IPT_CONFIG_COM_PAR_EXT));
         if (IPTGLOBAL(configDB.finish_addr_resolv) != 0)
         {
            IPTVosPrint0(IPT_WARN,
                         "IPTCom configuration waiting for TDC to be ready\n");
            ret = (int)IPT_TDC_NOT_READY;
         }
         else
         {
            IPTVosPrint1(IPT_ERR,
                  "Could not find comminication parameters ID = %d in config DB\n",
                         comParId);
            ret = (int)IPT_NOT_FOUND;
         }
      }
      else
      {
         memcpy(pComPar, p, sizeof(IPT_CONFIG_COM_PAR_EXT));
      }

      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }

   return ret;
}

/*******************************************************************************
NAME:       iptConfigGetDatasetId
ABSTRACT:   Finds the datasetId for a comId
RETURNS:    0 if ok else !=0
*/
int iptConfigGetDatasetId(
   UINT32 comId,      /* com ID */
   UINT32 *pdatasetId) /* Pointer to datasetID */
{
   int ret;
   IPT_CONFIG_EXCHG_PAR_EXT *pExchg;

   ret = IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      /* Get pointer to exchange parameters */
      pExchg = (IPT_CONFIG_EXCHG_PAR_EXT *) iptTabFind(&IPTGLOBAL(configDB.exchgParTable),
                                                   comId);  /*lint !e826 Type cast OK */
   
      if (pExchg != NULL)
      {
         *pdatasetId = pExchg->datasetId;
         ret = (int)IPT_OK;
      }
      else
      {
         *pdatasetId = 0;
         if (IPTGLOBAL(configDB.finish_addr_resolv) != 0)
         {
            ret = (int)IPT_TDC_NOT_READY;
         }
         else
         {
            ret = (int)IPT_NOT_FOUND;
         }
      }

      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }

   return ret;
}

/*******************************************************************************
NAME:       iptConfigGetDataset
ABSTRACT:   Finds the dataset structure for a DatasetId
RETURNS:    Pointer to dataset structure, NULL if error
*/
IPT_CFG_DATASET_INT *iptConfigGetDataset(
   UINT32 datasetId)      /* dataset ID */
{
   IPT_CFG_DATASET_INT *pDataset = (IPT_CFG_DATASET_INT *)NULL;

   /* Get pointer to dataset parameters */
   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      pDataset = (IPT_CFG_DATASET_INT *) iptTabFind(&IPTGLOBAL(configDB.datasetTable),
                                                   datasetId); /*lint !e826 Type cast OK */
      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
      if (pDataset != NULL && !pDataset->prepared)
      {
         IPTVosPrint1(IPT_ERR, "DatasetId=%d not prepared\n",datasetId);
         pDataset = (IPT_CFG_DATASET_INT *)NULL;
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }

   return pDataset;
}

/*******************************************************************************
NAME:       iptConfigShowComid
ABSTRACT:   Display comid's in data base
RETURNS:    -
*/
void iptConfigShowComid(void)
{
   int i;
   IPT_CONFIG_EXCHG_PAR_EXT *pComidTab;
   IPT_TAB_HDR *pTab = &IPTGLOBAL(configDB.exchgParTable);

   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      return;
   }

   if (pTab->initialized)
   {
      pComidTab = (IPT_CONFIG_EXCHG_PAR_EXT *)(pTab->pTable); /*lint !e826 Type cast OK */
      
      MON_PRINTF("ComID table: nItems=%d maxItems=%d itemSize=%d tableSize=%d\n",
              pTab->nItems,pTab->maxItems,pTab->itemSize,pTab->tableSize);
  
      for (i=0; i<pTab->nItems; i++)
      {
         MON_PRINTF("ComId=%d DSid=%d CPid=%d PDTO=%d VB=%d PDCY=%d Red=%d"
                " AckTO=%d RespTO=%d\n",
                pComidTab[i].comId, pComidTab[i].datasetId, 
                pComidTab[i].comParId,
                pComidTab[i].pdRecPar.timeoutValue,
                pComidTab[i].pdRecPar.validityBehaviour,   
                pComidTab[i].pdSendPar.cycleTime,
                pComidTab[i].pdSendPar.redundant,
                pComidTab[i].mdSendPar.ackTimeOut, 
                pComidTab[i].mdSendPar.responseTimeOut);

         if (pComidTab[i].mdSendPar.pSourceURI != NULL)
         {
            MON_PRINTF(" MD: SourceURI=%s\n", pComidTab[i].mdSendPar.pSourceURI);
         }

         if (pComidTab[i].mdSendPar.pDestinationURI != NULL)
         {
            MON_PRINTF(" MD: DestinationURI=%s\n", pComidTab[i].mdSendPar.pDestinationURI);
         }

         if (pComidTab[i].pdRecPar.pSourceURI != NULL)
         {
            MON_PRINTF(" PD: SourceURI=%s\n", pComidTab[i].pdRecPar.pSourceURI);
         }

         if (pComidTab[i].pdSendPar.pDestinationURI != NULL)
         {
            MON_PRINTF(" PD: DestinationURI=%s\n", pComidTab[i].pdSendPar.pDestinationURI);
         }
      }
   }
   else
   {
      MON_PRINTF("ComId table not initiated\n");
   }

   if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
   }
}

/*******************************************************************************
NAME:       iptConfigShowDataset
ABSTRACT:   Display datasets in data base
RETURNS:    -
*/
void iptConfigShowDataset(void)
{
   int i,j;
   IPT_CFG_DATASET_INT *pDatasetTab;
   IPT_TAB_HDR *pTab = &IPTGLOBAL(configDB.datasetTable);

   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      return;
   }

   if (pTab->initialized)
   {
      pDatasetTab = (IPT_CFG_DATASET_INT *)(pTab->pTable);  /*lint !e826 Type cast OK */
      
      MON_PRINTF("Dataset table: nItems=%d maxItems=%d itemSize=%d tableSize=%d\n",
         pTab->nItems,pTab->maxItems,pTab->itemSize,pTab->tableSize);
      
      for (i=0; i<pTab->nItems; i++)
      {
         MON_PRINTF("DatasetId=%d Size=%d Alignment=%d nLines=%d Prepared=%d VarSize=%d\n",
            pDatasetTab[i].datasetId, pDatasetTab[i].size, pDatasetTab[i].alignment,
            pDatasetTab[i].nLines, pDatasetTab[i].prepared, pDatasetTab[i].varSize);
         
         for (j=0; j < pDatasetTab[i].nLines; j++)
         {
            if (pDatasetTab[i].pFormat[j].id < 0)
            {
               MON_PRINTF(" DataType=%10s Size=%5d\n",
                  datatypeInt2String(pDatasetTab[i].pFormat[j].id),
                  pDatasetTab[i].pFormat[j].size);
            }
            else
            {
               MON_PRINTF(" DataSet=%11d Size=%5d\n",
                  pDatasetTab[i].pFormat[j].id,
                  pDatasetTab[i].pFormat[j].size);
            }
         }
      }
   }
   else
   {
      MON_PRINTF("Dataset table not initiated\n");
   }

   if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
   }
}

/*******************************************************************************
NAME:       iptConfigShowComPar
ABSTRACT:   Display comunication parameters in data base
RETURNS:    -
*/
void iptConfigShowComPar(void)
{
   int i;
   IPT_CONFIG_COM_PAR_EXT *pComParTab;
   IPT_TAB_HDR *pTab = &IPTGLOBAL(configDB.comParTable);

   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
      return;
   }

   if (pTab->initialized)
   {
      pComParTab = (IPT_CONFIG_COM_PAR_EXT *)(pTab->pTable); /*lint !e826 Type cast OK */
      
      MON_PRINTF("ComID table: nItems=%d maxItems=%d itemSize=%d tableSize=%d\n",
         pTab->nItems,pTab->maxItems,pTab->itemSize,pTab->tableSize);
      
      for (i=0; i<pTab->nItems; i++)
      {
         MON_PRINTF("ComParId=%d qos=%d ttl=%d\n",
            pComParTab[i].comParId, pComParTab[i].qos, pComParTab[i].ttl);
      }
   }
   else
   {
      MON_PRINTF("ComId table not initiated\n");
   }
   
   if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
   }
}



