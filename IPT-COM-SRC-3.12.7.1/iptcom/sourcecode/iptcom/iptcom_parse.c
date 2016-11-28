/*******************************************************************************
 *  COPYRIGHT   : (C) 2006 Bombardier Transportation
 ********************************************************************************
 *  PROJECT     : IPTrain
 *
 *  MODULE      : iptcom_parse.c
 *
 *  ABSTRACT    : Parsing of IPTCom XML configuration file
 *
 ********************************************************************************
 *  HISTORY     :
 *
 * $Id: iptcom_parse.c 33666 2014-07-17 14:43:01Z gweiss $
 *
 *  CR-8123 (Bernd Löhr, 2013-09-12)
 * 			EOF error while parsing XML files will result in IPT_PARSE_ERROR
 *
 *  CR-1356 (Bernd Loehr, 2010-10-08)
 * 			Changes to define dataset dependent un/marshalling.
 *			Calling iptConfigAddDataset changed to iptConfigAddDatasetExt.
 *
 *  Internal (Bernd Loehr, 2010-08-13)
 * 			Old obsolete CVS history removed
 *
 *
 *
 *******************************************************************************/

/*******************************************************************************
*  INCLUDES */

#if defined(VXWORKS)
 #include <vxWorks.h>
 #include <hostLib.h>
#elif defined(LINUX)
 #include <unistd.h>
#elif defined(__INTEGRITY)
 #include <unistd.h>
#elif defined(DARWIN)
 #include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include "iptcom.h"	      /* Common type definitions for IPT  */
#include "vos.h"		      /* OS independent system calls      */
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"


/*******************************************************************************
*  DEFINES
*/
#define MAX_DATASET_LINES 8192


/*******************************************************************************
*  TYPEDEFS
*/

typedef struct
{
   const char *pStr;
   int type;
} DATATYPE;

/*******************************************************************************
*  LOCAL DATA */

static const DATATYPE datatypeTable[] =
{
   {"BOOLEAN8"    , IPT_BOOLEAN8    },
   {"BOOLEAN1"    , IPT_BOOLEAN8    },       /* Kept for backward compatibility, replaced by  IPT_BOOLEAN8 */
   {"CHAR8"       , IPT_CHAR8       },
   {"UNICODE16"   , IPT_UNICODE16   },
   {"INT8"        , IPT_INT8        },
   {"INT16"       , IPT_INT16       },
   {"INT32"       , IPT_INT32       },
   {"UINT8"       , IPT_UINT8       },
   {"UINT16"      , IPT_UINT16      },
   {"UINT32"      , IPT_UINT32      },
   {"REAL32"      , IPT_REAL32      },
   {"STRING"      , IPT_STRING      },
   {"ARRAY"       , IPT_ARRAY       },
   {"RECORD"      , IPT_RECORD      },
   {"INT64"       , IPT_INT64       },
   {"UINT64"      , IPT_UINT64      },
   {"TIMEDATE48"  , IPT_TIMEDATE48  }
};

static const char unknown[] = "UNKNOWN";

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/
static int handleExchangePar(XML_LOCAL *pLoc);
static int handleDataset(XML_LOCAL *pLoc);
static int handleCommunicationPar(XML_LOCAL *pLoc);

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
NAME:       datatypeString2Int
ABSTRACT:   Convert datatype as string to IPTCom integer codes
RETURNS:    datatype (<0) if found, 0 if not found
*/
static int datatypeString2Int(
   char *pDatatype)     /* Pointer to string with symbolic datatype */
{
   int i, max;
   
   max = sizeof(datatypeTable) / sizeof(DATATYPE);
   
   for (i = 0; i < max; i++)
   {
      if (iptStrcmp(pDatatype, datatypeTable[i].pStr) == 0)
         return datatypeTable[i].type;
   }
   
   /* No such datatype was found, should have been taken care of by schema check */
   IPTVosPrint1(IPT_CONFIG, "XML error, illegal datatype: %s\n", pDatatype);
   return 0;         
}

/*******************************************************************************
NAME:       handleExchangePar
ABSTRACT:   Get data for exchange parameters from XML file and put in configuration db
RETURNS:    0 if found, !=0 if not or error
*/
static int handleExchangePar(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   char *pMdSendSourceURI;
   char *pMdSendDestinationURI;
   char *pPdRecSourceURI;
   char *pTempURI;
   char *pPdSendDestinationURI;
   IPT_CONFIG_EXCHG_PAR_EXT exchgPar;
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   int valueInt;
   int token;
   int ret = (int)IPT_OK;
   int res;
   UINT32 comId = 0;
   UINT32 filterId;
   UINT32 destId;

   pMdSendSourceURI = (char *)IPTVosMalloc(MAX_TOKLEN+1);
   pMdSendDestinationURI = (char *)IPTVosMalloc(MAX_TOKLEN+1);
   pPdRecSourceURI = (char *)IPTVosMalloc(MAX_TOKLEN+1);
   pTempURI = (char *)IPTVosMalloc(MAX_TOKLEN+1);
   pPdSendDestinationURI = (char *)IPTVosMalloc(MAX_TOKLEN+1);
   
   if ((pMdSendSourceURI != NULL) && (pMdSendDestinationURI != NULL) &&
       (pPdRecSourceURI != NULL) && (pTempURI != NULL) &&
       (pPdSendDestinationURI != NULL))
   {
      /* Terminate strings */
      pMdSendSourceURI[0] = 0;
      pMdSendDestinationURI[0] = 0;
      pPdRecSourceURI[0] = 0;
      pTempURI[0] = 0;
      pPdSendDestinationURI[0] = 0;
   
      /* Initiate exchgPar */
      memset(&exchgPar, 0, sizeof(IPT_CONFIG_EXCHG_PAR_EXT));

      exchgPar.comId = 0;
      exchgPar.datasetId = 0;
      exchgPar.comParId = 0;
   
      exchgPar.mdSendPar.pDestinationURI = pMdSendDestinationURI;
      exchgPar.mdSendPar.pSourceURI = pMdSendSourceURI;
      exchgPar.mdSendPar.ackTimeOut = 0;
      exchgPar.mdSendPar.responseTimeOut = 0;

      exchgPar.pdRecPar.pSourceURI = pPdRecSourceURI;
      exchgPar.pdRecPar.validityBehaviour = IPTGLOBAL(pd.defInvalidBehaviour); 
      exchgPar.pdRecPar.timeoutValue = IPTGLOBAL(pd.defTimeout);          
    
      exchgPar.pdSendPar.pDestinationURI = pPdSendDestinationURI;
      exchgPar.pdSendPar.cycleTime = IPTGLOBAL(pd.defCycle);            
   
      /* Get attribute data */
      while ((token = iptXmlGetAttribute(pLoc, attribute, &valueInt, value)) == TOK_ATTRIBUTE)
      {
         if (strcmp(attribute, "com-id") == 0)
         {
            comId = valueInt;
            exchgPar.comId = valueInt;
         }
         else if (strcmp(attribute, "data-set-id") == 0)
            exchgPar.datasetId = valueInt;
         else if (strcmp(attribute, "com-parameter-id") == 0)
            exchgPar.comParId = valueInt;
      }
   
      /* If next token is "/>" there is no body */
      if (token != TOK_CLOSE_EMPTY)
      {
         /* Get all elements with communication parameters */
         iptXmlEnter(pLoc);
         
         while (iptXmlSeekStartTagAny(pLoc, tag, sizeof(tag)) == 0)
         {
         /* For compability reason the MD send parameter source URI can be given 
            in the tag md-receive-parameter */
            if (strcmp(tag, "md-receive-parameter") == 0)
            {
               /* Get attribute data */
               while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
               {
                  if (strcmp(attribute, "source-uri") == 0)
                     strncpy(pMdSendSourceURI, value, sizeof(value));
               }
            }
            else if (strcmp(tag, "md-send-parameter") == 0)
            {
               /* Get attribute data */
               while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
               {
                  if (strcmp(attribute, "destination-uri") == 0)
                     strncpy(pMdSendDestinationURI, value, sizeof(value));
                  else if (strcmp(attribute, "ack-timeout") == 0)
                     exchgPar.mdSendPar.ackTimeOut = valueInt;
                  else if (strcmp(attribute, "response-timeout") == 0)
                     exchgPar.mdSendPar.responseTimeOut = valueInt;
                  else if (strcmp(attribute, "source-uri") == 0)
                     strncpy(pMdSendSourceURI, value, sizeof(value));
               }
            }
            else if (strcmp(tag, "pd-receive-parameter") == 0)
            {
               /* Get attribute data */
               while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
               {
                  if (strcmp(attribute, "timeout-value") == 0)
                     exchgPar.pdRecPar.timeoutValue = valueInt;
                  else if (strcmp(attribute, "validity-behavior") == 0) /* Yes, it is misspelled... */
                     exchgPar.pdRecPar.validityBehaviour = valueInt;
                  else if (strcmp(attribute, "source-uri") == 0)
                     strncpy(pPdRecSourceURI, value, sizeof(value));
               }
            }
            else if (strcmp(tag, "pd-send-parameter") == 0)
            {
               /* Get attribute data */
               while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
               {
                  if (strcmp(attribute, "destination-uri") == 0)
                     strncpy(pPdSendDestinationURI, value, sizeof(value));
                  else if (strcmp(attribute, "cycle-time") == 0)
                     exchgPar.pdSendPar.cycleTime = valueInt;
                  else if (strcmp(attribute, "redundant") == 0)
                     exchgPar.pdSendPar.redundant = valueInt;
               }
            }
            else if (strcmp(tag, "pd-source-filter") == 0)
            {
               filterId = 0;
               pTempURI[0] = 0;
               /* Get attribute data */
               while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
               {
                  if (strcmp(attribute, "filter-id") == 0)
                     filterId = valueInt;
                  else if (strcmp(attribute, "source-uri") == 0)
                     strncpy(pTempURI, value, sizeof(value));
               }
               
               res = iptConfigAddPdSrcFilterPar(comId, filterId, pTempURI);
               if (res != (int)IPT_OK)
               {
               /* Earlier return values Ok or 
                  only already existed parameters with differnt value */
                  if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                  {
                     ret = res;
                  }
               }
            }
            else if (strcmp(tag, "destination-id") == 0)
            {
               destId = 0;
               pTempURI[0] = 0;
               /* Get attribute data */
               while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
               {
                  if (strcmp(attribute, "destination-id") == 0)
                     destId = valueInt;
                  else if (strcmp(attribute, "destination-uri") == 0)
                     strncpy(pTempURI, value, sizeof(value));
               }
               
               res = iptConfigAddDestIdPar(comId, destId, pTempURI);
               if (res != (int)IPT_OK)
               {
               /* Earlier return values Ok or 
                  only already existed parameters with differnt value */
                  if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                  {
                     ret = res;
                  }
               }
            }
         }

         iptXmlLeave(pLoc);

      }

      /* Call configuration DB API to load new exchange parameters */
      res = iptConfigAddExchgParExt(&exchgPar);
      if (res != (int)IPT_OK)
      {
         /* Everthing ok until now or filter parameters already existed with
           differnt parameters? */
         if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
         {
            /* Use error code from iptConfigAddExchgParExt */
            ret = res;
         }
      }
   }
   else
   {
         IPTVosPrint0(IPT_ERR, "handleExchangePar Could not allocate memory\n");
         ret = (int)IPT_MEM_ERROR;
   }
  
   /* Cleanup temporary storage */
   if (pMdSendSourceURI)
   {
      IPTVosFree((BYTE *)pMdSendSourceURI);
   }
   if (pMdSendDestinationURI)
   {
      IPTVosFree((BYTE *)pMdSendDestinationURI);
   }
   if (pPdRecSourceURI)
   {
      IPTVosFree((BYTE *)pPdRecSourceURI);
   }
   if (pTempURI)
   {
      IPTVosFree((BYTE *)pTempURI);
   }
   if (pPdSendDestinationURI)
   {
      IPTVosFree((BYTE *)pPdSendDestinationURI);
   }

   return(ret);
}

/*******************************************************************************
NAME:       handleDataset
ABSTRACT:   Get data for dataset from XML file and put in configuration db
RETURNS:    0 if found, !=0 if not or error
*/
static int handleDataset(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   IPT_DATASET_FORMAT *formatTable;
   IPT_CONFIG_DATASET_EXT dataset;
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   int valueInt;
   int ret = (int)IPT_OK;

   formatTable = (IPT_DATASET_FORMAT *) IPTVosMalloc(MAX_DATASET_LINES * sizeof(IPT_DATASET_FORMAT));
   if (formatTable == NULL)
   {
      IPTVosPrint1(IPT_ERR, "handleDataset: Out of memory. Requested size=%d\n",
                      MAX_DATASET_LINES * sizeof(IPT_DATASET_FORMAT));
      return((int)IPT_MEM_ERROR);
      
   }
  
   memset(&dataset, 0, sizeof (dataset));
   
   /* Get attribute data */
   while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
   {
      if (strcmp(attribute, "data-set-id") == 0)
         dataset.datasetId = valueInt;
      if (strcmp(attribute, "disable-marshalling") == 0)
         dataset.disableMarshalling = valueInt;
   }
   
   /* Get all elements with process variables */
   iptXmlEnter(pLoc);
   
   while (iptXmlSeekStartTagAny(pLoc, tag, sizeof(tag)) == 0)
   {
      if (strcmp(tag, "process-variable") == 0)
      {
         if (dataset.nLines >= MAX_DATASET_LINES)
         {
            IPTVosPrint1(IPT_ERR, "To many process-variable for data set id=%d\n", dataset.datasetId);
            
            iptXmlLeave(pLoc);

            (void) IPTVosFree((BYTE *)formatTable);
            return((int)IPT_TAB_ERR_ILLEGAL_SIZE);
         }
         
         formatTable[dataset.nLines].id = 0;
         formatTable[dataset.nLines].size = 1;
         
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
            if (strcmp(attribute, "type") == 0)
            {
               if (valueInt != 0)
                  formatTable[dataset.nLines].id = valueInt;     /* Integer, indicating datasetId */
               else
                  formatTable[dataset.nLines].id = datatypeString2Int(value);   /* datatype string */
            }
            else if (strcmp(attribute, "array-size") == 0)
               formatTable[dataset.nLines].size = valueInt;
         }
         
         dataset.nLines++;
      }
   }
   
   /* Call configuration DB API to load new exchange parameters */
   ret = iptConfigAddDatasetExt(&dataset, formatTable);

   iptXmlLeave(pLoc);

   (void) IPTVosFree((BYTE *)formatTable);

   return(ret);
}

/*******************************************************************************
NAME:       handleCommunicationPar
ABSTRACT:   Get communication parameters from XML file and put in configuration db
RETURNS:    0 if found, !=0 if not or error
*/
static int handleCommunicationPar(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   IPT_CONFIG_COM_PAR comPar = {0, MD_DEF_QOS, MD_DEF_TTL};   /* Default QoS, TTL */
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   int valueInt;
   int ret = (int)IPT_OK;
   /* Get all elements with communication parameters */
   iptXmlEnter(pLoc);
   
   while ((iptXmlSeekStartTagAny(pLoc, tag, sizeof(tag)) == 0) && 
          ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS)))
   {
      if (strcmp(tag, "network-parameter-ip") == 0)
      {
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
            if (strcmp(attribute, "com-parameter-id") == 0)
               comPar.comParId = valueInt;
            else if (strcmp(attribute, "qos") == 0)
               comPar.qos = valueInt;
            else if (strcmp(attribute, "time-to-live") == 0)
               comPar.ttl = valueInt;
            else if (strcmp(attribute, "ttl") == 0)
               comPar.ttl = valueInt;
         }
      }
 
      /* Call configuration DB API to load new exchange parameters */
      ret = iptConfigAddComPar(&comPar);
   }
   
   iptXmlLeave(pLoc);

   return(ret);
}

/*******************************************************************************
NAME:       handleMemPar
ABSTRACT:   Get memory parameters from XML file
RETURNS:    0 if found, !=0 if not or error
*/
static void handleMemPar(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   int valueInt;
   UINT32 size;
   UINT32 minNoOfBlocks;
   unsigned char *p[MEM_MAX_PREALLOCATE];
   UINT32 i,j;

   /* Get all elements with memory block parameters */
   iptXmlEnter(pLoc);
   
   while (iptXmlSeekStartTagAny(pLoc, tag, sizeof(tag)) == 0)
   {
      if (strcmp(tag, "mem-block") == 0)
      {
         size = 0;
         minNoOfBlocks = 0;
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
            if (strcmp(attribute, "size") == 0)
               size = valueInt;
            else if (strcmp(attribute, "preallocate") == 0)
               minNoOfBlocks = valueInt;
         }

         if (size)
         {
            i = 0;
            while ((i < IPTGLOBAL(mem.noOfBlocks)) && 
                   (IPTGLOBAL(mem.freeBlock)[i].size < size))
            {
              i++;
            }
            j = IPTGLOBAL(mem.noOfBlocks);
            while (j > i)
            {
               IPTGLOBAL(mem.freeBlock)[j].pFirst = IPTGLOBAL(mem.freeBlock)[j-1].pFirst;
               IPTGLOBAL(mem.freeBlock)[j].size = IPTGLOBAL(mem.freeBlock)[j-1].size;
               IPTGLOBAL(mem.memCnt.blockCnt)[j] = IPTGLOBAL(mem.memCnt.blockCnt)[j-1];
               j--;
            }
            IPTGLOBAL(mem.freeBlock)[j].pFirst = (MEM_BLOCK *)NULL;
            IPTGLOBAL(mem.freeBlock)[j].size = size;
            IPTGLOBAL(mem.noOfBlocks)++;

            if (minNoOfBlocks > MEM_MAX_PREALLOCATE)
               minNoOfBlocks = MEM_MAX_PREALLOCATE;

            for (j = 0; j < minNoOfBlocks; j++)
            {
               p[j] = IPTVosMalloc(size);
            }

            for (j = 0; j < minNoOfBlocks; j++)
            {
               IPTVosFree(p[j]);
            }
         }
      }
   }
   
   iptXmlLeave(pLoc);

   return;
}

/* Windows XP */
#if defined(WIN32)
/*******************************************************************************
NAME:       setWindowPriority
ABSTRACT:   Set Window task/thread priority if a valid string is read from
            the XML file.
RETURNS:    -
*/
static void setWindowPriority(
   char *pValueString,  /* Pointer to value string */
   int  *pOut)          /* Pointer to output */
{

   if (iptStrcmp(pValueString, "THREAD_PRIORITY_NORMAL") == 0)
   {
      *pOut = THREAD_PRIORITY_NORMAL;
   }
   else if (iptStrcmp(pValueString, "THREAD_PRIORITY_ABOVE_NORMAL") == 0)
   {
      *pOut = THREAD_PRIORITY_ABOVE_NORMAL;
   }
   else if (iptStrcmp(pValueString, "THREAD_PRIORITY_BELOW_NORMAL") == 0)
   {
      *pOut = THREAD_PRIORITY_BELOW_NORMAL;
   }
   else if (iptStrcmp(pValueString, "THREAD_PRIORITY_HIGHEST") == 0)
   {
      *pOut = THREAD_PRIORITY_HIGHEST;
   }
   else if (iptStrcmp(pValueString, "THREAD_PRIORITY_LOWEST") == 0)
   {
      *pOut = THREAD_PRIORITY_LOWEST;
   }
   else if (iptStrcmp(pValueString, "THREAD_PRIORITY_TIME_CRITICAL") == 0)
   {
      *pOut = THREAD_PRIORITY_TIME_CRITICAL;
   }
   else if (iptStrcmp(pValueString, "THREAD_PRIORITY_IDLE") == 0)
   {
      *pOut = THREAD_PRIORITY_IDLE;
   }
}
#endif

/*******************************************************************************
NAME:       handleDevConfig1
ABSTRACT:   Parse the configuration XML file for the device configuration to be
            used at start-up
RETURNS:    0 if OK, !=0 if not.
*/
static void handleDevConfig1(
   XML_LOCAL *pLoc, /* Pointer to local data */
#if defined(LINUX) || defined(DARWIN)
   UINT32 *pMemSize,   /* Pointer to Memory size, output */
   UINT32 *pIfWait,   /* Pointer to interface wait, output */
   char *pLinuxFile)   /* Pointer to Linux file name and path, output */
#else
   UINT32 *pMemSize,   /* Pointer to Memory size, output */
   UINT32 *pIfWait)   /* Pointer to interface wait, output */
#endif
{
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   int valueInt;

   IPT_UNUSED (pIfWait)

   /* Get attribute data */
   while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value)
          == TOK_ATTRIBUTE)
   {
      if ((strcmp(attribute, "ipt-memory-size") == 0) && valueInt > 0)
      {
         *pMemSize = (UINT32) valueInt;
         break;
      }
   }

   /* Get all elements with linux file  parameters */
   iptXmlEnter(pLoc);

   while (iptXmlSeekStartTagAny(pLoc, tag, sizeof(tag)) == 0)
   {
#if defined(IF_WAIT_ENABLE) 
      if (strcmp(tag, "start-up") == 0)
      {
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value)
                 == TOK_ATTRIBUTE)
         {
            if (strcmp(attribute, "if-wait") == 0)
            {
               *pIfWait = (UINT32) valueInt;
            }
         }
      }
#endif
#if defined(LINUX) || defined(DARWIN)
      if (strcmp(tag, "linux-file") == 0)
      {
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value)
                 == TOK_ATTRIBUTE)
         {
            if (strcmp(attribute, "file-name") == 0)
            {
               if (strlen(value) < LINUX_FILE_SIZE)
               {
                  strcpy(pLinuxFile, value);
               }
               else
               {
                  strncpy(pLinuxFile, value, LINUX_FILE_SIZE - 1);
                  pLinuxFile[LINUX_FILE_SIZE - 1] = 0;
               }
            }
         }
      }
#endif
   }

   iptXmlLeave(pLoc);
}

/*******************************************************************************
NAME:       handleDevConfig2
ABSTRACT:   Parse the configuration XML file for the device configuration to be stored in the 
            global IPTCom structure
RETURNS:    0 if OK, !=0 if not.
*/
static void handleDevConfig2(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   int valueInt;
   
   /* Get all elements with device parameters */
   iptXmlEnter(pLoc);

   while (iptXmlSeekStartTagAny(pLoc, tag, sizeof(tag)) == 0)
   {
      if (strcmp(tag, "mem-block-list") == 0)
      {
         handleMemPar(pLoc);
      }
      else if (strcmp(tag, "pd-com-parameter") == 0)
      {
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
            if (strcmp(attribute, "timeout-value") == 0)
            {
               IPTGLOBAL(pd.defTimeout) = valueInt;
            }
            else if (strcmp(attribute, "validity-behavior") == 0)
            {
               IPTGLOBAL(pd.defInvalidBehaviour) = valueInt;
            }
            else if (strcmp(attribute, "cycle-time") == 0)
            {
               IPTGLOBAL(pd.defCycle) = valueInt;
            }
         }
      }
      else if (strcmp(tag, "md-com-parameter") == 0)
      {
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
            if (strcmp(attribute, "ack-timeout") == 0)
            {
               IPTGLOBAL(md.defAckTimeOut) = valueInt;
            }
            else if (strcmp(attribute, "response-timeout") == 0)
            {
               IPTGLOBAL(md.defResponseTimeOut) = valueInt;
            }
            else if (strcmp(attribute, "max-seq-no") == 0)
            {
               IPTGLOBAL(md.maxStoredSeqNo) = valueInt;
            }
         }
      }
      else if (strcmp(tag, "pd-receive") == 0)
      {
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
/* Windows XP */
#if defined(WIN32)
            if (strcmp(attribute, "windows-priority") == 0)
            {
               setWindowPriority(value, &(IPTGLOBAL(task.pdRecPriority)));
            }
/* LINUX */
#elif defined(LINUX) || defined(DARWIN)
            if (strcmp(attribute, "linux-priority") == 0)
            {
               IPTGLOBAL(task.pdRecPriority) = valueInt;
            }
/* VXWORKS */
#elif defined(VXWORKS)
            if (strcmp(attribute, "vxworks-priority") == 0)
            {
               IPTGLOBAL(task.pdRecPriority) = valueInt;
            }
#endif
         }
      }
      else if (strcmp(tag, "pd-process") == 0)
      {
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
/* Windows XP */
#if defined(WIN32)
            if (strcmp(attribute, "windows-priority") == 0)
            {
               setWindowPriority(value, &(IPTGLOBAL(task.pdProcPriority)));
            }
/* LINUX */
#elif defined(LINUX) || defined(DARWIN)
            if (strcmp(attribute, "linux-priority") == 0)
            {
               IPTGLOBAL(task.pdProcPriority) = valueInt;
            }
/* VXWORKS */
#elif defined(VXWORKS)
            if (strcmp(attribute, "vxworks-priority") == 0)
            {
               IPTGLOBAL(task.pdProcPriority) = valueInt;
            }
#endif
            if (strcmp(attribute, "cycle-time") == 0)
            {
               IPTGLOBAL(task.pdProcCycle) = valueInt;
            }
         }
      }
      else if (strcmp(tag, "md-receive") == 0)
      {
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
/* Windows XP */
#if defined(WIN32)
            if (strcmp(attribute, "windows-priority") == 0)
            {
               setWindowPriority(value, &(IPTGLOBAL(task.mdRecPriority)));
            }
/* LINUX */
#elif defined(LINUX) || defined(DARWIN)
            if (strcmp(attribute, "linux-priority") == 0)
            {
               IPTGLOBAL(task.mdRecPriority) = valueInt;
            }
/* VXWORKS */
#elif defined(VXWORKS)
            if (strcmp(attribute, "vxworks-priority") == 0)
            {
               IPTGLOBAL(task.mdRecPriority) = valueInt;
            }
#endif
         }
      }
      else if (strcmp(tag, "md-process") == 0)
      {
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
/* Windows XP */
#if defined(WIN32)
            if (strcmp(attribute, "windows-priority") == 0)
            {
               setWindowPriority(value, &(IPTGLOBAL(task.mdProcPriority)));
            }
/* LINUX */
#elif defined(LINUX) || defined(DARWIN)
            if (strcmp(attribute, "linux-priority") == 0)
            {
               IPTGLOBAL(task.mdProcPriority) = valueInt;
            }
/* VXWORKS */
#elif defined(VXWORKS)
            if (strcmp(attribute, "vxworks-priority") == 0)
            {
               IPTGLOBAL(task.mdProcPriority) = valueInt;
            }
#endif
            if (strcmp(attribute, "cycle-time") == 0)
            {
               IPTGLOBAL(task.mdProcCycle) = valueInt;
            }
         }
      }
      else if (strcmp(tag, "iptcom-process") == 0)
      {
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
/* Windows XP */
#if defined(WIN32)
            if (strcmp(attribute, "windows-priority") == 0)
            {
               setWindowPriority(value, &(IPTGLOBAL(task.iptComProcPriority)));
            }
/* LINUX */
#elif defined(LINUX) || defined(DARWIN)
            if (strcmp(attribute, "linux-priority") == 0)
            {
               IPTGLOBAL(task.iptComProcPriority) = valueInt;
            }
/* VXWORKS */
#elif defined(VXWORKS)
            if (strcmp(attribute, "vxworks-priority") == 0)
            {
               IPTGLOBAL(task.iptComProcPriority) = valueInt;
            }
#endif
            if (strcmp(attribute, "cycle-time") == 0)
            {
               IPTGLOBAL(task.iptComProcCycle) = valueInt;
            }
         }
      }
      else if (strcmp(tag, "snmp-receive") == 0)
      {
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
/* Windows XP */
#if defined(WIN32)
            if (strcmp(attribute, "windows-priority") == 0)
            {
               setWindowPriority(value, &(IPTGLOBAL(task.snmpRecPriority)));
            }
/* LINUX */
#elif defined(LINUX) || defined(DARWIN)
            if (strcmp(attribute, "linux-priority") == 0)
            {
               IPTGLOBAL(task.snmpRecPriority) = valueInt;
            }
/* VXWORKS */
#elif defined(VXWORKS)
            if (strcmp(attribute, "vxworks-priority") == 0)
            {
               IPTGLOBAL(task.snmpRecPriority) = valueInt;
            }
#endif
         }
      }
#ifdef LINUX_MULTIPROC
      else if (strcmp(tag, "net_ctrl") == 0)
      {
         /* Get attribute data */
         while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
         {
            if (strcmp(attribute, "linux-priority") == 0)
            {
               IPTGLOBAL(task.netCtrlPriority) = valueInt;
            }
         }
      }
#endif
   }

   iptXmlLeave(pLoc);

}

/*******************************************************************************
NAME:       handleDebug
ABSTRACT:   Parse the configuration XML file for the debug attribute.
RETURNS:    0 if OK, !=0 if not.
*/
static void handleDebug(
   XML_LOCAL *pLoc) /* Pointer to local data */
{
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   unsigned int i;
   int valueInt;
   char debugFilename[MAX_TOKLEN+1];
   UINT16 debugMask;
   UINT16 debugInfoMask;
 
   /* Get all elements with debug parameters */
   iptXmlEnter(pLoc);

   /* Get attribute data */
   while (iptXmlGetAttribute(pLoc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
   {
      if (strcmp(attribute, "file-name") == 0)
      {
         strncpy(debugFilename, value, sizeof(value));
         IPTVosDFile(debugFilename);
      }
      else if (strcmp(attribute, "file-size") == 0)
      {
         IPTVosSetLogFileSize(valueInt);
      }
      else if (strcmp(attribute, "level-iptcom") == 0)
      {
         debugMask = 0;
         for(i=0; i < strlen(value); ++i)
         {
            switch(value[i])
            {
                case 'E':
                case 'e':
                    debugMask |= IPT_ERR;
                    break;
                case 'w':
                case 'W':
                    debugMask |= IPT_ERR | IPT_WARN;
                    break;

                default:
                    break;
            }
         }
         IPTVosSetPrintMask(debugMask);
      }
      else if (strcmp(attribute, "info-iptcom") == 0)
      {
         debugInfoMask = 0;
         for(i=0; i < strlen(value); ++i)
         {
            switch(value[i])
            {
                case 'A':
                case 'a':
                    debugInfoMask |= INF_ALL;
                    break;
                case 'D':
                case 'd':
                case 'T':
                case 't':
                    debugInfoMask |= INF_DATETIME;
                    break;
                case 'C':
                case 'c':
                    debugInfoMask |= INF_CATEGORY;
                    break;
                case 'F':
                case 'f':
                    debugInfoMask |= INF_FILE;
                    break;
                case 'L':
                case 'l':
                    debugInfoMask |= INF_LINE;
                    break;
                default:
                    break;
            }
         }
         IPTVosSetInfoMask(debugInfoMask);
      }
   }
   
   iptXmlLeave(pLoc);

   return;
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       iptConfigParseXMLInit
ABSTRACT:   Parse the configuration XML file for the memory, debug and Linux
            file name and path attribute.
RETURNS:    0 if OK, !=0 if not.
*/
#if defined(LINUX) || defined(DARWIN) 
int iptConfigParseXMLInit(
   const char *path,  /* File path to XML file */
   UINT32 *pMemSize,   /* Pointer to Memory size, output */
   UINT32 *pIfWait,   /* Pointer to interface wait, output */
   char *pLinuxFile)   /* Pointer to Linux file name and path, output */
#else
int iptConfigParseXMLInit(
   const char *path,  /* File path to XML file */
   UINT32 *pMemSize,   /* Memory size, output */
   UINT32 *pIfWait)   /* Pointer to interface wait, output */
#endif
{
   XML_LOCAL loc;    /* Local data */
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   char hostname[MAX_TOKLEN+1];
   int valueInt;

   if (iptXmlOpen(&loc, path) != (int)IPT_OK)
   {
      IPTVosPrint1(IPT_ERR, "Cannot open XML configuration file: %s\n", path);
      return (int)IPT_NOT_FOUND;
   }
   iptXmlEnter(&loc);
   
   if (iptXmlSeekStartTag(&loc, "cpu") == 0)
   {
      iptXmlEnter(&loc);
      
      while (iptXmlSeekStartTagAny(&loc, tag, sizeof(tag)) == 0)
      {
         if (strcmp(tag, "device-configuration") == 0)
         {
#if defined(LINUX) || defined(DARWIN)
            handleDevConfig1(&loc, pMemSize, pIfWait, pLinuxFile);
#else
            handleDevConfig1(&loc, pMemSize, pIfWait);
#endif
         }
         else if (strcmp(tag, "host") == 0)
         {
            if (gethostname(hostname, sizeof(hostname)) == 0)
            {
               /* Get attribute data */
               while (iptXmlGetAttribute(&loc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
               {
                  if (strcmp(attribute, "host-name") == 0)
                  {
                     if (strncmp(hostname, value, sizeof(value)) == 0)
                     {
                        iptXmlEnter(&loc);
                        
                        while (iptXmlSeekStartTagAny(&loc, tag, sizeof(tag)) == 0)
                        {
                           if (strcmp(tag, "device-configuration") == 0)
                           {
#if defined(LINUX) || defined(DARWIN)
                              handleDevConfig1(&loc, pMemSize, pIfWait, pLinuxFile);
#else
                              handleDevConfig1(&loc, pMemSize, pIfWait);
#endif
                           }
                        }

                        iptXmlLeave(&loc);
                     }
                  }
               }
            }
         }
         else if (strcmp(tag, "debug") == 0)
         {
            handleDebug(&loc);
         }
      }
   }
   
   (void)iptXmlClose(&loc);
   
   if (loc.error != IPT_OK)
   {
       IPTVosPrint1(IPT_ERR, "Unexpected end of XML configuration file: %s\n", path);
   }
   return loc.error;
}

/*******************************************************************************
NAME:       iptConfigParseXML
ABSTRACT:   Parse the configuration XML file and load the configuration DB tables.
RETURNS:    0 if OK, !=0 if not
*/
int iptConfigParseXML(
   const char *path)   /* File path to XML file */
{
   int ret = (int)IPT_OK;
   int ret2;
   XML_LOCAL loc;    /* Local data */
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   int valueInt;
   UINT8 dummy;
   UINT32 ipAddr;
   T_TDC_RESULT res;
   
   if (iptXmlOpen(&loc, path) != (int)IPT_OK)
   {
      IPTVosPrint1(IPT_ERR, "Cannot open XML configuration file: %s\n", path);
      return (int)IPT_NOT_FOUND;
   }
   
   iptXmlEnter(&loc);
   
   if (iptXmlSeekStartTag(&loc, "cpu") == 0)
   {
      iptXmlEnter(&loc);
      
      while ((iptXmlSeekStartTagAny(&loc, tag, sizeof(tag)) == 0) && 
             ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS)))
      {
         if (strcmp(tag, "bus-interface-list") == 0)
         {
            iptXmlEnter(&loc);
            if (iptXmlSeekStartTag(&loc, "bus-interface") == 0)
            {
               iptXmlEnter(&loc);
               
               while (iptXmlSeekStartTag(&loc, "telegram") == 0)
               {
                  ret2 = handleExchangePar(&loc);
                  if (ret2 != (int)IPT_OK)
                  {
                     /* Earlier return values Ok or 
                       only already existed parameters with differnt value */
                     if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                     {
                        /* Use new return value */
                        ret = ret2;
                     }
                  }
               }
               
               iptXmlLeave(&loc);
            }
            iptXmlLeave(&loc);
         }
         else if (strcmp(tag, "data-set-list") == 0)
         {
            iptXmlEnter(&loc);
            
            while (iptXmlSeekStartTag(&loc, "data-set") == 0)
            {
               ret2 = handleDataset(&loc);
               if (ret2 != (int)IPT_OK)
               {
                  /* Earlier return values Ok or 
                    only already existed parameters with differnt value */
                  if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                  {
                     /* Use new return value */
                     ret = ret2;
                  }
               }
            }
            
            iptXmlLeave(&loc);
         }
         else if (strcmp(tag, "com-parameter-list") == 0)
         {
            ret2 = handleCommunicationPar(&loc);
            if (ret2 != (int)IPT_OK)
            {
               /* Earlier return values Ok or 
                 only already existed parameters with differnt value */
               if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
               {
                  /* Use new return value */
                  ret = ret2;
               }
            }
         }
         else if (strcmp(tag, "device") == 0)
         {
            /* Get attribute data */
            while ((iptXmlGetAttribute(&loc, attribute, &valueInt, value) == TOK_ATTRIBUTE) &&
                   ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS)))
            {
               if (strcmp(attribute, "device-uri") == 0)
               {
                  /* Get IP address from IPTDir based on the destination URI */
                  dummy = 0;
                  res = iptGetAddrByName(value, &ipAddr, &dummy);
                  if (res == TDC_OK)
                  {
                     /* Own device? */
                     if (ipAddr == 0x7f000001)
                     {
                        iptXmlEnter(&loc);
            
                        while ((iptXmlSeekStartTagAny(&loc, tag, sizeof(tag)) == 0) &&
                               ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS)))
                        {
                           if (strcmp(tag, "bus-interface-list") == 0)
                           {
                              iptXmlEnter(&loc);
                              if (iptXmlSeekStartTag(&loc, "bus-interface") == 0)
                              {
                                 iptXmlEnter(&loc);
               
                                 while (iptXmlSeekStartTag(&loc, "telegram") == 0)
                                 {
                                    ret2 = handleExchangePar(&loc);
                                    if (ret2 != (int)IPT_OK)
                                    {
                                       /* Earlier return values Ok or 
                                         only already existed parameters with differnt value */
                                       if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                                       {
                                          /* Use new return value */
                                          ret = ret2;
                                       }
                                    }
                                 }
               
                                 iptXmlLeave(&loc);
                              }
                              iptXmlLeave(&loc);
                           }
                           else if (strcmp(tag, "data-set-list") == 0)
                           {
                              iptXmlEnter(&loc);
            
                              while (iptXmlSeekStartTag(&loc, "data-set") == 0)
                              {
                                 ret2 = handleDataset(&loc);
                                 if (ret2 != (int)IPT_OK)
                                 {
                                    /* Earlier return values Ok or 
                                      only already existed parameters with differnt value */
                                    if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                                    {
                                       /* Use new return value */
                                       ret = ret2;
                                    }
                                 }
                              }
            
                              iptXmlLeave(&loc);
                           }
                           else if (strcmp(tag, "com-parameter-list") == 0)
                           {
                              ret2 = handleCommunicationPar(&loc);
                              if (ret2 != (int)IPT_OK)
                              {
                                 /* Earlier return values Ok or 
                                   only already existed parameters with differnt value */
                                 if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                                 {
                                    /* Use new return value */
                                    ret = ret2;
                                 }
                              }
                           }
                        }
            
                        iptXmlLeave(&loc);
                     }
                  }
                  else
                  {
                     if ((res == TDC_NO_CONFIG_DATA) || (res == TDC_MUST_FINISH_INIT))
                     {
                        if ((IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER)) == IPT_OK)
                        {
                           /* Indicate that address resolving has to be done later when TDC has
                              got data from IPTDir */
                           IPTGLOBAL(configDB.finish_addr_resolv) = 1;

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
                     }
                     else
                     {
                        ret = res;
                        IPTVosPrint2(IPT_ERR,
                         "Could not convert destination URI= %s to IP address. TDC result = %#x\n",
                                    value, res);
                        break;
                     }
                  }
               }
            }
         }
      }
      iptXmlLeave(&loc);
   }
   
   (void)iptXmlClose(&loc);

   if (loc.error != IPT_OK)
   {
       IPTVosPrint1(IPT_ERR, "Unexpected end of XML configuration file: %s\n", path);
       ret = loc.error;
   }
   return(ret);
}

/*******************************************************************************
NAME:       iptConfigParseXML
ABSTRACT:   Parse the configuration XML file and load the configuration DB tables.
RETURNS:    0 if OK, !=0 if not  
*/
int iptConfigParseOwnUriXML(
   const char *path)   /* File path to XML file */
{
   int ret = (int)IPT_OK;
   int ret2;
   XML_LOCAL loc;    /* Local data */
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   int valueInt;
   UINT8 dummy;
   UINT32 ipAddr;
   T_TDC_RESULT res;
   
   if (iptXmlOpen(&loc, path) != (int)IPT_OK)
   {
      IPTVosPrint1(IPT_ERR, "Cannot open XML configuration file: %s\n", path);
      return (int)IPT_NOT_FOUND;
   }
   
   iptXmlEnter(&loc);
   
   if (iptXmlSeekStartTag(&loc, "cpu") == 0)
   {
      iptXmlEnter(&loc);
      
      while ((iptXmlSeekStartTagAny(&loc, tag, sizeof(tag)) == 0) && 
             ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS)))
      {
         if (strcmp(tag, "device") == 0)
         {
            /* Get attribute data */
            while ((iptXmlGetAttribute(&loc, attribute, &valueInt, value) == TOK_ATTRIBUTE) &&
                   ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS)))
            {
               if (strcmp(attribute, "device-uri") == 0)
               {
                  /* Get IP address from IPTDir based on the destination URI */
                  dummy = 0;
                  res = iptGetAddrByName(value, &ipAddr, &dummy);
                  if (res == TDC_OK)
                  {
                     /* Own device? */
                     if (ipAddr == 0x7f000001)
                     {
                        iptXmlEnter(&loc);
            
                        while ((iptXmlSeekStartTagAny(&loc, tag, sizeof(tag)) == 0) &&
                               ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS)))
                        {
                           if (strcmp(tag, "bus-interface-list") == 0)
                           {
                              iptXmlEnter(&loc);
                              if (iptXmlSeekStartTag(&loc, "bus-interface") == 0)
                              {
                                 iptXmlEnter(&loc);
               
                                 while (iptXmlSeekStartTag(&loc, "telegram") == 0)
                                 {
                                    ret2 = handleExchangePar(&loc);
                                    if (ret2 != (int)IPT_OK)
                                    {
                                       /* Earlier return values Ok or 
                                         only already existed parameters with differnt value */
                                       if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                                       {
                                          /* Use new return value */
                                          ret = ret2;
                                       }
                                    }
                                 }
               
                                 iptXmlLeave(&loc);
                              }
                              iptXmlLeave(&loc);
                           }
                           else if (strcmp(tag, "data-set-list") == 0)
                           {
                              iptXmlEnter(&loc);
            
                              while (iptXmlSeekStartTag(&loc, "data-set") == 0)
                              {
                                 ret2 = handleDataset(&loc);
                                 if (ret2 != (int)IPT_OK)
                                 {
                                    /* Earlier return values Ok or 
                                      only already existed parameters with differnt value */
                                    if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                                    {
                                       /* Use new return value */
                                       ret = ret2;
                                    }
                                 }
                              }
            
                              iptXmlLeave(&loc);
                           }
                           else if (strcmp(tag, "com-parameter-list") == 0)
                           {
                              ret2 = handleCommunicationPar(&loc);
                              if (ret2 != (int)IPT_OK)
                              {
                                 /* Earlier return values Ok or 
                                   only already existed parameters with differnt value */
                                 if ((ret == (int)IPT_OK) || (ret == (int)IPT_TAB_ERR_EXISTS))
                                 {
                                    /* Use new return value */
                                    ret = ret2;
                                 }
                              }
                           }
                        }
            
                        iptXmlLeave(&loc);
                     }
                  }
                  else
                  {
                     if ((res == TDC_NO_CONFIG_DATA) || (res == TDC_MUST_FINISH_INIT))
                     {
                        if ((IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER)) == IPT_OK)
                        {
                           /* Indicate that address resolving has to be done later when TDC has
                              got data from IPTDir */
                           IPTGLOBAL(configDB.finish_addr_resolv) = 1;
                           ret = (int)IPT_TDC_NOT_READY;

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
                        break;
                     }
                     else
                     {
                        ret = res;
                        IPTVosPrint2(IPT_ERR,
                         "Could not convert destination URI= %s to IP address. TDC result = %#x\n",
                                    value, res);
                        break;
                     }
                  }
               }
            }
         }
      }
      iptXmlLeave(&loc);
   }
   
   (void)iptXmlClose(&loc);
   
   if (loc.error != IPT_OK)
   {
       IPTVosPrint1(IPT_ERR, "Unexpected end of XML configuration file: %s\n", path);
       ret = loc.error;
   }
   return(ret);
}

/*******************************************************************************
NAME:       iptConfigParseXML4DevConfig
ABSTRACT:   Parse the configuration XML file for the device attribute.
RETURNS:    0 if OK, !=0 if not.
*/
int iptConfigParseXML4DevConfig(
   const char *path)  /* File path to XML file */
{
   XML_LOCAL loc;    /* Local data */
   char tag[MAX_TAGLEN+1];
   char attribute[MAX_TOKLEN+1];
   char value[MAX_TOKLEN+1];
   int valueInt;
   char hostname[MAX_TOKLEN+1];

   if (iptXmlOpen(&loc, path) != (int)IPT_OK)
   {
      IPTVosPrint1(IPT_ERR, "Cannot open XML configuration file: %s\n", path);
      return (int)IPT_NOT_FOUND;
   }
   
   iptXmlEnter(&loc);
   
   if (iptXmlSeekStartTag(&loc, "cpu") == 0)
   {
      iptXmlEnter(&loc);
      
      while (iptXmlSeekStartTagAny(&loc, tag, sizeof(tag)) == 0)
      {
         if (strcmp(tag, "device-configuration") == 0)
         {
            handleDevConfig2(&loc);
         }
         if (strcmp(tag, "host") == 0)
         {
            if (gethostname(hostname, sizeof(hostname)) == 0)
            {
               /* Get attribute data */
               while (iptXmlGetAttribute(&loc, attribute, &valueInt, value) == TOK_ATTRIBUTE)
               {
                  if (strcmp(attribute, "host-name") == 0)
                  {
                     if (strncmp(hostname, value, sizeof(value)) == 0)
                     {
                        iptXmlEnter(&loc);
                        
                        while (iptXmlSeekStartTagAny(&loc, tag, sizeof(tag)) == 0)
                        {
                           if (strcmp(tag, "device-configuration") == 0)
                           {
                              handleDevConfig2(&loc);
                           }
                        }

                        iptXmlLeave(&loc);
                     }
                  }
               }
            }
         }
      }
   }
   
   (void)iptXmlClose(&loc);
   
   if (loc.error != IPT_OK)
   {
       IPTVosPrint1(IPT_ERR, "Unexpected end of XML configuration file: %s\n", path);
   }
   return loc.error;
}

/*******************************************************************************
NAME:       iptConfigParseXML4Dbg
ABSTRACT:   Parse the configuration XML file for the memory attribute.
RETURNS:    0 if OK, !=0 if not.
*/
int iptConfigParseXML4Dbg(
   const char *path)  /* File path to XML file */
{
   XML_LOCAL loc;    /* Local data */
   char tag[MAX_TAGLEN+1];
   if (iptXmlOpen(&loc, path) != (int)IPT_OK)
   {
      IPTVosPrint1(IPT_ERR, "Cannot open XML configuration file: %s\n", path);
      return (int)IPT_NOT_FOUND;
   }
   
   iptXmlEnter(&loc);
   
   if (iptXmlSeekStartTag(&loc, "cpu") == 0)
   {
      iptXmlEnter(&loc);
      
      while (iptXmlSeekStartTagAny(&loc, tag, sizeof(tag)) == 0)
      {
         if (strcmp(tag, "debug") == 0)
         {
            handleDebug(&loc);
         }
      }
   }
   
   (void)iptXmlClose(&loc);
   
   if (loc.error != IPT_OK)
   {
       IPTVosPrint1(IPT_ERR, "Unexpected end of XML configuration file: %s\n", path);
   }
   return loc.error;
}

/*******************************************************************************
NAME:       datatypeInt2String
ABSTRACT:   Convert IPTCom integer codes to datatype as string 
RETURNS:    Data type string if found else empty string
*/
const char* datatypeInt2String(
   INT32 type)     /* IPTCom integer codes for datatype */
{
   int i, max;
      
   max = sizeof(datatypeTable) / sizeof(DATATYPE);
   
   for (i = 0; i < max; i++)
   {
      if (type == datatypeTable[i].type)
         return datatypeTable[i].pStr;
   }
   
   return unknown;         
}

