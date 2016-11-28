/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2014 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     : IPTrain
 *
 *  MODULE      : iptcom_util.c
 *
 *  ABSTRACT    : Utilities for iptcom
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 *  $Id: iptcom_util.c 36888 2015-05-08 15:40:27Z gweiss $
 *
 *  CR_4092 (Gerhard Weiss 2015-03-10)
 *          Added missing Error Return Case
 *
 *  CR-7779 (Gerhard Weiss 2014-07-01)
 *          added check for receiving MD frame len (configurable)
 *
 *  CR-3477 (Bernd Loehr, 2012-02-18)
 * 			TÜV Assessment findings
 *
 *  CR-1356 (Bernd Loehr, 2010-10-08)
 * 			Additional function iptConfigAddDatasetExt to define dataset
 *          dependent un/marshalling. iptConfigAddDatasetID changed to call
 *          iptConfigAddDatasetExt.
 *
 *  Internal (Bernd Loehr, 2010-08-13) 
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
#include <ctype.h>
#include "iptcom.h"        /* Common type definitions for IPT  */
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"
#ifdef TARGET_SIMU
#include "tdcApiPiq.h" 
#endif

/*******************************************************************************
*  DEFINES */

/* FCS constants */
#define INITFCS  0xffffffff   /* Initial FCS value */
#define GOODFCS  0xdebb20e3   /* Good final FCS value */

#define FCS_CHECK
#undef NO_FCS_CHECK

/*******************************************************************************
*  TYPEDEFS */

/* Marshalling info, used to and from wire */
typedef struct
{
   int level;
   const BYTE *pSrc;
   BYTE *pDst, *pDstEnd;
} MARSHALL_INFO;

typedef struct
{
   int  code;
   char *pStr;
} ERROR_INFO;

/*******************************************************************************
*  GLOBALS
*/

/* Extracted from tdcResolve.h */
extern UINT32 tdcGetOwnIpAddrs (const char* pModname, UINT32 maxAddrCnt, UINT32 ipAddrs[]);

#ifdef LINUX_MULTIPROC
/* Global data for complete IPTCom */
IPT_GLOBAL *pIptGlobal = 0;
#endif

const ERROR_INFO errorInfoTable[] =
{
   {(int)IPT_ERROR                         ,"General error value"},
   {(int)IPT_NOT_FOUND                     ,"Item was not found"},
   {(int)IPT_ILLEGAL_SIZE                  ,"Illegal size"},
   {(int)IPT_FCS_ERROR                     ,"Frame Check Sequence error"},
   {(int)IPT_MEM_ERROR                     ,"Out of memory"},
   {(int)IPT_PARSE_ERROR                   ,"Error during parsing of XML file"},
   {(int)IPT_MARSHALL_ERROR                ,"Error during marshalling"},
   {(int)IPT_MARSHALL_MAX_LEVEL            ,"Reached max level of recursion during marshalling"},
   {(int)IPT_MARSHALL_UNKNOWN_DATASETID    ,"Dataset ID unknown"},
   {(int)IPT_MARSHALL_TOO_BIG              ,"Attempt to write outside boundary"},
   {(int)IPT_ENDIAN_ALIGNMENT_ERROR        ,"This device does not work as expected concerning alignment or endian"},
   {(int)IPT_TAB_ERR_INIT                  ,"Table not initialized"},
   {(int)IPT_TAB_ERR_NOT_FOUND             ,"Item was not found"},
   {(int)IPT_TAB_ERR_ILLEGAL_SIZE          ,"Illegal size"},
   {(int)IPT_TAB_ERR_EXISTS                ,"Item alreade exists"},
   {(int)IPT_SEM_ERR                       ,"Semaphore error"},
   {(int)IPT_MEM_ILL_RETURN                ,"Return of memory not previously allocated"},
   {(int)IPT_QUEUE_ERR                     ,"Queue error"},
   {(int)IPT_QUEUE_IN_USE                  ,"Attempt to destroy a queue in use"},
   {(int)IPT_TDC_NOT_READY                 ,"TDC is waiting for configuration from IPTDir"},
   {(int)IPT_ERR_NO_IPADDR                 ,"Own IP address not found"},
   {(int)IPT_ERR_NO_ETH_IF                 ,"Ethernet interface not initiated"},
   {(int)IPT_INVALID_PAR                   ,"Invalid parameters"},
   {(int)IPT_INVALID_COMPAR                ,"Invalid communication parameters"},
   {(int)IPT_INVALID_DATA                  ,"Invalid application data, e.g. no data"},
   {(int)IPT_MD_NOT_INIT                   ,"MD communication not initiated"},
   {(int)IPT_WRONG_TOPO_CNT                ,"Current topo counter not equal to application topo counter"},
   {(int)IPT_ERR_INIT_DRIVER_FAILED        ,"Method call to init driver failed"},
   {(int)IPT_ERR_EXIT_DRIVER_FAILED        ,"Method call to exit driver failed"},
   {(int)IPT_ERR_DRIVER_ALREADY_STARTED    ,"Driver is already open"},
   {(int)IPT_ERR_DRIVER_ALREADY_CLOSED     ,"Driver is already closed"},
   {(int)IPT_ERR_MD_SOCKET_CREATE_FAILED   ,"Could not create MD socket"},
   {(int)IPT_ERR_MD_SOCKET_BIND_FAILED     ,"Could not bind MD socket"},
   {(int)IPT_ERR_PD_SOCKET_CREATE_FAILED   ,"Could not create PD socket"},
   {(int)IPT_ERR_PD_SOCKET_BIND_FAILED     ,"Could not bind PD socket"},
   {(int)IPT_ERR_SEM_CREATE_FAILED         ,"Could not create semaphore"},
   {(int)IPT_ERR_PD_SOCKOPT_FAILED         ,"SetSockOpt for PD socket failed"},
   {(int)IPT_ERR_MD_SOCKOPT_FAILED         ,"SetSockOpt for MD socket failed"},
   {(int)IPT_ERR_MD_SENDTO_FAILED          ,"Sendto on MD socket failed"},
   {(int)IPT_ERR_SNMP_SENDTO_FAILED        ,"Sendto on SNMP socket failed"},
   {(int)IPT_ERR_PD_SENDTO_FAILED          ,"Sendto on PD socket failed"},
   {(int)IPT_ERR_SNMP_SOCKET_CREATE_FAILED ,"Could not create SNMP socket"},
   {(int)IPT_ERR_SNMP_SOCKET_BIND_FAILED   ,"Could not bind SNMP socket"},
   {(int)IPT_SIM_ERR_PCAP_IFOPEN           ,"PCap could not access/open interface"},
};

/*******************************************************************************
*  LOCAL DATA */


/*
* The FCS-32 generator polynomial: x**0 + x**1 + x**2 + x**4 + x**5
*                      + x**7 + x**8 + x**10 + x**11 + x**12 + x**16
*                      + x**22 + x**23 + x**26 + x**32.
*/

static const UINT32 fcstab[256] =
{
   0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
      0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
      0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
      0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
      0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
      0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
      0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
      0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
      0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
      0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
      0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
      0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
      0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
      0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
      0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
      0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
      0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
      0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
      0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
      0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
      
      0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
      0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
      0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
      0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
      0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
      0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
      0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
      0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
      0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
      0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
      0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
      0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
      0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
      0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
      0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
      0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
      0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
      0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
      0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
      0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
      0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
      0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
      0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
      0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
      0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
      0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
      0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
      0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
      0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
      0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
      0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
      0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
      0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
      0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
      0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
      0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
      0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
      0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
      0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
      0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
      0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
      0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
      0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
      0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};


/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
NAME:       marshall
ABSTRACT:   Marshalls PD or MD data and packs into a send buffer.
RETURNS:    0 if OK, !=0 if error
*/
static int marshall(
   UINT16 nLines,
   UINT16 alignment,
   IPT_DATA_SET_FORMAT_INT *pFormat,
   MARSHALL_INFO *pInfo)
{
   int ret = 0, zeroTerminated;
   UINT32 size;
   UINT16 variableSize = 0;
   UINT8 b;
   union
   {
      UINT16 d;
      UINT8 b[2];
   } data2;
   union
   {
      UINT32 d;
      UINT8 b[4];
   } data4;
   union
   {
      UINT64 d;
      UINT8 b[8];
   } data8;
   
   /* Restrict recursion */
   pInfo->level++;
   if (pInfo->level > MAX_FORMAT_LEVEL)
      return (int)IPT_MARSHALL_MAX_LEVEL;
   
   /* Align pSrc to size for this dataset */
   iptAlignStruct((BYTE **)(&pInfo->pSrc), alignment);
   
   /* Format src according to formatting rules */
   while (nLines > 0 && ret == 0)
   {
      if (pFormat->size == IPT_VAR_SIZE)
      {
         /* Use previous UINT16 that should have been just before */
         size = variableSize;
      }
      else
      {
         /* Fixed data size, get size from dataset formatting */
         size = pFormat->size;
      }
      
      if (pFormat->id > 0)
      {
         if (pFormat->pFormat == NULL)
         {
            return (int)IPT_MARSHALL_UNKNOWN_DATASETID;
         }
         while (size-- > 0)
         {
            /* Dataset, call ourself recursively */
            if ((ret = marshall(pFormat->nLines, pFormat->alignment, pFormat->pFormat, pInfo)) != (int)IPT_OK)
               return ret;
         }
      }
      else
      {
         /* Basic data type, just copy to destination */
         switch (pFormat->id)
         {
         case IPT_BOOLEAN8:
         case IPT_CHAR8:
         case IPT_INT8:
         case IPT_UINT8:
            /* 1 byte data, 1 byte alignment, no big/little endian */
            if ((pInfo->pDst + size) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            while (size-- > 0)
               *pInfo->pDst++ = *pInfo->pSrc++;
           break;
            
         case IPT_INT16:
         case IPT_UINT16:
            /* 2 byte data, 2 byte alignment, big/little endian */
            if ((pInfo->pDst + size * 2) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            /* Align source address to multiple of 2 */
            iptAlign((BYTE **)(&pInfo->pSrc), ALIGNOF(UINT16));
            
            /* Save as possible variable data size */
            variableSize = * (UINT16 *) pInfo->pSrc;  /*lint !e826 Correct parsing of src data */
            
            while (size-- > 0)
            {
               /* Read source data as 2 byte data */
               data2.d = * (UINT16 *) pInfo->pSrc;    /*lint !e826 Correct parsing of src data */
               pInfo->pSrc += 2;
               
#if IS_BIGENDIAN
               *pInfo->pDst++ = data2.b[0];
               *pInfo->pDst++ = data2.b[1];
#else
               *pInfo->pDst++ = data2.b[1];
               *pInfo->pDst++ = data2.b[0];
#endif
            }
            break;
            
         case IPT_INT32:
         case IPT_UINT32:
         case IPT_REAL32:
            /* 4 byte data, 4 byte alignment, big/little endian */
            if ((pInfo->pDst + size * 4) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            /* Align source address to multiple of 4 */
            iptAlign((BYTE **)(&pInfo->pSrc), ALIGNOF(UINT32));
          
            while (size-- > 0)
            {
               /* Read source data as 4 byte data */
               data4.d = * (UINT32 *) pInfo->pSrc;    /*lint !e826 Correct parsing of src data */
               pInfo->pSrc += 4;
               
#if IS_BIGENDIAN
               *pInfo->pDst++ = data4.b[0];
               *pInfo->pDst++ = data4.b[1];
               *pInfo->pDst++ = data4.b[2];
               *pInfo->pDst++ = data4.b[3];
#else
               *pInfo->pDst++ = data4.b[3];
               *pInfo->pDst++ = data4.b[2];
               *pInfo->pDst++ = data4.b[1];
               *pInfo->pDst++ = data4.b[0];
#endif
           }
            break;
            
         case IPT_TIMEDATE48:
            /* 6 byte data (4+2), 4 byte alignment, big/little endian */
            if ((pInfo->pDst + size * 8) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            /* Align source address to struct alignment of UINT64 */
            iptAlign((BYTE **)(&pInfo->pSrc), ALIGNOF(UINT64ST));

            while (size-- > 0)
            {
               /* Read source data as 8 byte data */
               data8.d = * (UINT64 *) pInfo->pSrc;    /*lint !e826 Correct parsing of src data */
               pInfo->pSrc += 8;
               
#if IS_BIGENDIAN
               *pInfo->pDst++ = data8.b[0];  /* seconds, MSP */
               *pInfo->pDst++ = data8.b[1];
               *pInfo->pDst++ = data8.b[2];
               *pInfo->pDst++ = data8.b[3];  /* seconds, MSP */
               *pInfo->pDst++ = data8.b[4];  /* ticks, MSP */
               *pInfo->pDst++ = data8.b[5];  /* ticks, MSP */
#else
               *pInfo->pDst++ = data8.b[3];  /* seconds, MSP */
               *pInfo->pDst++ = data8.b[2];
               *pInfo->pDst++ = data8.b[1];
               *pInfo->pDst++ = data8.b[0];  /* seconds, MSP */
               *pInfo->pDst++ = data8.b[5];  /* ticks, MSP */
               *pInfo->pDst++ = data8.b[4];  /* ticks, MSP */
#endif
            }
            break;
            
         case IPT_INT64:
         case IPT_UINT64:
            /* 8 byte data, 4 byte alignment, big/little endian */
            if ((pInfo->pDst + size * 8) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

       /* Align source address to to struct alignment of UINT64 */
            iptAlign((BYTE **)(&pInfo->pSrc), ALIGNOF(UINT64ST));
            
        while (size-- > 0)
            {
               /* Read source data as 8 byte data */
               data8.d = * (UINT64 *) pInfo->pSrc;    /*lint !e826 Correct parsing of src data */
               pInfo->pSrc += 8;
               
#if IS_BIGENDIAN
               *pInfo->pDst++ = data8.b[0];
               *pInfo->pDst++ = data8.b[1];
               *pInfo->pDst++ = data8.b[2];
               *pInfo->pDst++ = data8.b[3];
               *pInfo->pDst++ = data8.b[4];
               *pInfo->pDst++ = data8.b[5];
               *pInfo->pDst++ = data8.b[6];
               *pInfo->pDst++ = data8.b[7];
#else
               *pInfo->pDst++ = data8.b[7];
               *pInfo->pDst++ = data8.b[6];
               *pInfo->pDst++ = data8.b[5];
               *pInfo->pDst++ = data8.b[4];
               *pInfo->pDst++ = data8.b[3];
               *pInfo->pDst++ = data8.b[2];
               *pInfo->pDst++ = data8.b[1];
               *pInfo->pDst++ = data8.b[0];
#endif
            }
            break;
            
         case IPT_STRING:
            if ((pInfo->pDst + size) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            /* Load string up until and including zero-termination */
            zeroTerminated = FALSE;
            while (size-- > 0)
            {
               /* 1 byte data, 1 byte alignment, no big/little endian */
               b = *pInfo->pSrc++;
               *pInfo->pDst++ = b;
               
               if (b == 0)
               {
                  zeroTerminated = TRUE;
                  break;           /* Break if we have found a zero termination */
               }
            }

            /* If STRING was not zero terminated it is not possible to send */
            if (!zeroTerminated)
               return (int)IPT_MARSHALL_ERROR;

            pInfo->pSrc += size;   /* Skip rest of string */
            break;
            
         case IPT_UNICODE16:
            /* 2 byte data, 2 byte alignment, big/little endian */
            if ((pInfo->pDst + size * 2) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            /* Align source address to multiple of 2 */
            iptAlign((BYTE **)(&pInfo->pSrc), ALIGNOF(UINT16));
            
            while (size > 0)
            {
               /* Read source data as 2 byte data */
               data2.d = * (UINT16 *) pInfo->pSrc;    /*lint !e826 Correct parsing of src data */
               pInfo->pSrc += 2;
               
#if IS_BIGENDIAN
               *pInfo->pDst++ = data2.b[0];
               *pInfo->pDst++ = data2.b[1];
#else
               *pInfo->pDst++ = data2.b[1];
               *pInfo->pDst++ = data2.b[0];
#endif
               size--;

               if (data2.d == 0)
                  break;         /* Break if we have found a zero termination */
            }

            pInfo->pSrc += 2 * size;  /* Skip rest of string */
            break;

         
         default:
            IPTVosPrint1(IPT_WARN,
                         "marshall ERROR Wrong data type=%d\n",
                         pFormat->id);
            ret = IPT_MARSHALL_UNKNOWN_DATASETID;
            break;
         }
      }
      
      pFormat++;  /* Get next formatting line */
      nLines--;
   }
   
   iptAlignStruct((BYTE **)(&pInfo->pSrc), alignment);
   
   pInfo->level--;
   return ret;
}

/*******************************************************************************
NAME:       unmarshall
ABSTRACT:   Unmarshalls PD or MD data from wire to native buffer.
RETURNS:    0 if OK, !=0 if error
*/
static int unmarshall(
   UINT16 nLines,
   UINT16 alignment,
   IPT_DATA_SET_FORMAT_INT *pFormat,
   MARSHALL_INFO *pInfo)
{
   int ret = (int)IPT_OK;
   INT32 size;
   UINT16 variableSize = 0;

   union
   {
      UINT16 d;
      UINT8 b[2];
   } data2;
   union
   {
      UINT32 d;
      UINT8 b[4];
   } data4;
   union
   {
      UINT64 d;
      UINT8 b[8];
   } data8;
   
   /* Restrict recursion */
   if (++pInfo->level > MAX_FORMAT_LEVEL)
      return (int)IPT_MARSHALL_MAX_LEVEL;
   
   iptAlignStructZero(&pInfo->pDst, alignment);
   
   /* Format src according to formatting rules */
   while (nLines > 0 && ret == 0)
   {
      if (pFormat->size == IPT_VAR_SIZE)
      {
         /* Use previous UINT16 that should have been just before */
         size = variableSize;
      }
      else
      {
         /* Fixed data size, get size from dataset formatting */
         size = pFormat->size;
      }
      
      if (pFormat->id > 0)
      {
         if (pFormat->pFormat == NULL)
         {
            return (int)IPT_MARSHALL_UNKNOWN_DATASETID;
         }
         while (size-- > 0)
         {
            /* Dataset, call ourself recursively */
            if ((ret = unmarshall(pFormat->nLines, pFormat->alignment, pFormat->pFormat, pInfo)) != (int)IPT_OK)
               return ret;
         }
      }
      else
      {
         /* Basic data type, just copy to destination */
         switch (pFormat->id)
         {
         case IPT_BOOLEAN8:
         case IPT_CHAR8:
         case IPT_INT8:
         case IPT_UINT8:
            /* 1 byte data, 1 byte alignment */
            if ((pInfo->pDst + size) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            while (size-- > 0)
            {
               *pInfo->pDst++ = *pInfo->pSrc++;
            }
            break;
            
         case IPT_INT16:
         case IPT_UINT16:
            /* 2 byte data, align destination address to multiple of 2 */
            iptAlignZero(&pInfo->pDst, ALIGNOF(UINT16));
            
            if ((pInfo->pDst + size * 2) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            while (size-- > 0)
            {
#if IS_BIGENDIAN
               data2.b[0] = *pInfo->pSrc++;
               data2.b[1] = *pInfo->pSrc++;
#else
               data2.b[1] = *pInfo->pSrc++;
               data2.b[0] = *pInfo->pSrc++;
#endif               
               /* Write destination data as 2 byte data */
               * (UINT16 *) pInfo->pDst = data2.d;    /*lint !e826 Write protected with size check above */
               pInfo->pDst += 2;
            }

            /* Save (last) UINT16 as possible variable data size */
            variableSize = data2.d;
            break;
            
         case IPT_INT32:
         case IPT_UINT32:
         case IPT_REAL32:
            /* 4 byte data, align destination address to multiple of 4 */
            iptAlignZero(&pInfo->pDst, ALIGNOF(UINT32));
            
            if ((pInfo->pDst + size * 4) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            while (size-- > 0)
            {
#if IS_BIGENDIAN
               data4.b[0] = *pInfo->pSrc++;
               data4.b[1] = *pInfo->pSrc++;
               data4.b[2] = *pInfo->pSrc++;
               data4.b[3] = *pInfo->pSrc++;
#else
               data4.b[3] = *pInfo->pSrc++;
               data4.b[2] = *pInfo->pSrc++;
               data4.b[1] = *pInfo->pSrc++;
               data4.b[0] = *pInfo->pSrc++;
#endif
               
               /* Write destination data as 4 byte data */
               * (UINT32 *) pInfo->pDst = data4.d;    /*lint !e826 Write protected with size check above */    
               pInfo->pDst += 4;
            }            
            break;
            
         case IPT_TIMEDATE48:
            /* 6 byte data (4+2), align destination address to struct alignment of UINT64 */
            iptAlignZero(&pInfo->pDst, ALIGNOF(UINT64ST));
            
            if ((pInfo->pDst + size * 8) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            while (size-- > 0)
            {
#if IS_BIGENDIAN
               data8.b[0] = *pInfo->pSrc++;  /* seconds, MSP */
               data8.b[1] = *pInfo->pSrc++;
               data8.b[2] = *pInfo->pSrc++;
               data8.b[3] = *pInfo->pSrc++;  /* seconds, LSP */
               data8.b[4] = *pInfo->pSrc++;  /* ticks, MSP */
               data8.b[5] = *pInfo->pSrc++;  /* ticks, LSP */
               data8.b[6] = 0;
               data8.b[7] = 0;
#else
               data8.b[3] = *pInfo->pSrc++;  /* seconds, MSP */
               data8.b[2] = *pInfo->pSrc++;
               data8.b[1] = *pInfo->pSrc++;
               data8.b[0] = *pInfo->pSrc++;  /* seconds, LSP */
               data8.b[5] = *pInfo->pSrc++;  /* ticks, MSP */
               data8.b[4] = *pInfo->pSrc++;  /* ticks, LSP */
               data8.b[7] = 0;
               data8.b[6] = 0;
#endif
               /* Write destination data as 8 byte data */
               * (UINT64 *) pInfo->pDst = data8.d;    /*lint !e826 Write protected with size check above */
               pInfo->pDst += 8;
            }            
            break;
            
         case IPT_INT64:
         case IPT_UINT64:
            /* 8 byte data, align destination address to struct alignment of UINT64 */
            iptAlignZero(&pInfo->pDst, ALIGNOF(UINT64ST));
            
            if ((pInfo->pDst + size * 8) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            while (size-- > 0)
            {
#if IS_BIGENDIAN
               data8.b[0] = *pInfo->pSrc++;
               data8.b[1] = *pInfo->pSrc++;
               data8.b[2] = *pInfo->pSrc++;
               data8.b[3] = *pInfo->pSrc++;
               data8.b[4] = *pInfo->pSrc++;
               data8.b[5] = *pInfo->pSrc++;
               data8.b[6] = *pInfo->pSrc++;
               data8.b[7] = *pInfo->pSrc++;
#else
               data8.b[7] = *pInfo->pSrc++;
               data8.b[6] = *pInfo->pSrc++;
               data8.b[5] = *pInfo->pSrc++;
               data8.b[4] = *pInfo->pSrc++;
               data8.b[3] = *pInfo->pSrc++;
               data8.b[2] = *pInfo->pSrc++;
               data8.b[1] = *pInfo->pSrc++;
               data8.b[0] = *pInfo->pSrc++;
#endif
               /* Write destination data as 8 byte data */
               * (UINT64 *) pInfo->pDst = data8.d;    /*lint !e826 Write protected with size check above */
               pInfo->pDst += 8;
            }            
            break;
            
         case IPT_STRING:
            if ((pInfo->pDst + size) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            /* Load string up to zero-termination */
            while (*pInfo->pSrc != 0 && size-- > 0)
            {
               /* 1 byte data, 1 byte alignment, no big/little endian */
               *pInfo->pDst++ = *pInfo->pSrc++;
            }
            pInfo->pSrc++;    /* Skip incoming zero termination */

            /* Pad with zeroes */
            while (size-- > 0)
               *pInfo->pDst++ = 0;

            break;
            
         case IPT_UNICODE16:
            /* 2 byte data, align destination address to multiple of 2 */
            iptAlignZero(&pInfo->pDst, ALIGNOF(UINT16));
            
            if ((pInfo->pDst + size * 2) > pInfo->pDstEnd)
               return (int)IPT_MARSHALL_TOO_BIG;

            /* Load string up to zero-termination */
            while (size > 0)
            {
#if IS_BIGENDIAN
               data2.b[0] = *pInfo->pSrc++;
               data2.b[1] = *pInfo->pSrc++;
#else
               data2.b[1] = *pInfo->pSrc++;
               data2.b[0] = *pInfo->pSrc++;
#endif
               
               /* Write destination data as 2 byte data */
               * (UINT16 *) pInfo->pDst = data2.d;    /*lint !e826 Write protected with size check above */    
               pInfo->pDst += 2;
               size--;

               if (data2.d == 0)
                  break;            /* Break if zero termination */
            }

            /* Pad with zeroes */
            while (size-- > 0)
            {
               * (UINT16 *) pInfo->pDst = 0;     /*lint !e826 Write protected with size check above */
               pInfo->pDst += 2;
            }

            break;

         default:
            IPTVosPrint1(IPT_WARN,
                         "unmarshall ERROR Wrong data type=%d\n",
                         pFormat->id);
            ret = IPT_MARSHALL_UNKNOWN_DATASETID;
            break;
         }
      }
      
      pFormat++;  /* Get next formatting line */
      nLines--;
   }
   
   iptAlignStructZero(&pInfo->pDst, alignment);
   
   pInfo->level--;
   return ret;
}

/*******************************************************************************
NAME:       calcDatasetSize
ABSTRACT:   Calculate the size of a marshalled dataset. Called recursively from iptCalcDatasetSize.
            Do the same thing as if unmarshalling, except for copying.
RETURNS:    0 if OK, !=0 if error, 
*/
static int calcDatasetSize(
   UINT16 nLines,
   UINT16 alignment,
   IPT_DATA_SET_FORMAT_INT *pFormat,
   MARSHALL_INFO *pInfo)
{
   int ret = 0;
   UINT32 size;
   UINT16 variableSize = 0;
   union
   {
      UINT16 d;
      UINT8 b[2];
   } data2;

   /* Restrict recursion */
   if (++pInfo->level > MAX_FORMAT_LEVEL)
      return (int)IPT_MARSHALL_MAX_LEVEL;
   
   /* Align pDst to alignment size for this dataset */
   iptAlignStruct(&pInfo->pDst, alignment);
   
   /* Format src according to formatting rules */
   while (nLines > 0 && ret == 0)
   {
      if (pFormat->size == IPT_VAR_SIZE)
      {
         /* Variable data size, get size from previous saved UINT16 */
         size = variableSize;
      }
      else
      {
         /* Fixed data size, get size from dataset formatting */
         size = pFormat->size;
      }
      
      if (pFormat->id > 0)
      {
         if (pFormat->pFormat == NULL)
         {
            return (int)IPT_MARSHALL_UNKNOWN_DATASETID;
         }
         while (size-- > 0)
         {
            if ((ret = calcDatasetSize(pFormat->nLines, pFormat->alignment, pFormat->pFormat, pInfo)) != (int)IPT_OK)
            {
               return ret;
            }
         }
      }
      else
      {
         /* Basic data type, just copy to destination */
         switch (pFormat->id)
         {
         case IPT_BOOLEAN8:
         case IPT_CHAR8:
         case IPT_INT8:
         case IPT_UINT8:
            /* 1 byte data, 1 byte alignment */
            pInfo->pSrc += size;
            pInfo->pDst += size;
            break;
            
         case IPT_INT16:
         case IPT_UINT16:
         case IPT_UNICODE16:
            /* Save as possible variable data size */
#if IS_BIGENDIAN
            data2.b[0] = *pInfo->pSrc++;
            data2.b[1] = *pInfo->pSrc++;
#else
            data2.b[1] = *pInfo->pSrc++;
            data2.b[0] = *pInfo->pSrc++;
#endif
            variableSize = data2.d;

            /* 2 byte data, align destination address to multiple of 2 */
            iptAlign(&pInfo->pDst, ALIGNOF(UINT16));
            pInfo->pDst += 2 * size;
            break;
            
         case IPT_INT32:
         case IPT_UINT32:
         case IPT_REAL32:
            /* 4 byte data, align destination address to multiple of 4 */
            pInfo->pSrc += 4 * size;
            iptAlign(&pInfo->pDst, ALIGNOF(UINT32));
            pInfo->pDst += 4 * size;
            break;

         case IPT_INT64:
         case IPT_UINT64:
         case IPT_TIMEDATE48:
            /* 8 byte data, align destination address to multiple of 4 */
            pInfo->pSrc += 8 * size;
            iptAlign(&pInfo->pDst, ALIGNOF(UINT64));
            pInfo->pDst += 8 * size;
            break;

         case IPT_STRING:
            /* 1 byte data, 1 byte alignment */
            pInfo->pSrc += strlen((char *)(pInfo->pSrc + 1));   /* Skip over source string, including zero termination */
            pInfo->pDst += size;
            break;

         default:
            IPTVosPrint1(IPT_WARN,
                         "calcDatasetSize ERROR Wrong data type=%d\n",
                         pFormat->id);
            ret = IPT_MARSHALL_UNKNOWN_DATASETID;
            break;
         }
      }

      pFormat++;  /* Get next formatting line */
      nLines--;
   }

   pInfo->level--;
   return ret;
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       iptAlign
ABSTRACT:   Aligns a pointer to a specified size 1, 2, 4, 8, 16... for a process variable
            Sizes greater than 4 are aligned to size 4.            
RETURNS:    -
*/
void iptAlign(
   BYTE **p,
   unsigned int size)
{
   UINT32 mask;
   
   if (size > ALIGNOF(UINT64ST))
      size = ALIGNOF(UINT64ST);

   mask = size - 1;
   *p = (BYTE *) (((UINT32) *p + mask) & ~mask);
}

/*******************************************************************************
NAME:       iptAlignStruct
ABSTRACT:   Aligns a pointer to a specified size 1, 2, 4, 8, 16... for a structure
            For ARM: always align structs to size 4
            For other: align to specified size but never greater than 4
RETURNS:    -
*/
void iptAlignStruct(
   BYTE **p,
   unsigned int size)
{
   UINT32 mask;
   
#ifdef HMI400
   size = 4;
#else
   if (size > ALIGNOF(UINT64ST))
      size = ALIGNOF(UINT64ST);
#endif

   mask = size - 1;
   *p = (BYTE *) (((UINT32) *p + mask) & ~mask);
}

/*******************************************************************************
NAME:       iptAlignZero
ABSTRACT:   Aligns a pointer to a specified size 1, 2, 4, 8, 16... for process variable
            and zeroes bytes that are skipped.
            Sizes greater than 4 are aligned to size 4.            
RETURNS:    -
*/
void iptAlignZero(
   BYTE **p,
   unsigned int size)
{
   UINT32 mask;
   BYTE *p2;
   
   if (size > ALIGNOF(UINT64ST))
      size = ALIGNOF(UINT64ST);

   mask = size - 1;
   p2 = (BYTE *) (((UINT32) *p + mask) & ~mask);
   while (*p != p2)
   {
      *(*p)++ = 0;
   }
}

/*******************************************************************************
NAME:       iptAlignStructZero
ABSTRACT:   Aligns a pointer to a specified size 1, 2, 4, 8, 16... for structure 
            and zeroes bytes that are skipped.
            For ARM: always align structs to size 4
            For other: align to specified size but never greater than 4
RETURNS:    -
*/
void iptAlignStructZero(
   BYTE **p,
   unsigned int size)
{
   UINT32 mask;
   BYTE *p2;
   
#ifdef HMI400
   size = 4;
#else
   if (size > ALIGNOF(UINT64ST))
      size = ALIGNOF(UINT64ST);
#endif

   mask = size - 1;
   p2 = (BYTE *) (((UINT32) *p + mask) & ~mask);
   while (*p != p2)
   {
      *(*p)++ = 0;
   }
}

/*******************************************************************************
NAME:       iptMarshall
ABSTRACT:   Marshalls PD or MD data and packs into a send buffer.
RETURNS:    0 if OK, !=0 if error
*/
int iptMarshall(
   UINT32 comId,     /* Comid for this data */
   const BYTE *pSrc, /* Pointer to source data */
   BYTE *pDst,       /* OUT: Pointer to destination buffer */
   UINT32 *pDstSize) /* IN: size of destination buffer*/
                     /* OUT: no of bytes written */ 
{
   int ret;
   UINT32 datasetId;

   ret = iptConfigGetDatasetId(comId, &datasetId);
   if (ret == (int)IPT_OK)
   {
      ret = iptMarshallDSInt(0, datasetId, pSrc, pDst, pDstSize);
   }
   
   return ret;
}

/*******************************************************************************
NAME:       iptMarshallDS
ABSTRACT:   Marshalls a dataset.
RETURNS:    0 if OK, !=0 if error
*/
int iptMarshallDS(
   UINT32 datasetId ,/* DatasetId for this data, used to get formatting info */
   const BYTE *pSrc, /* Pointer to source data */
   BYTE *pDst,       /* OUT: Pointer to destination buffer */
   UINT32 *pDstSize) /* IN: size of destination buffer*/
                     /* OUT: no of bytes written */ 
{
	return iptMarshallDSInt(0, datasetId, pSrc, pDst, pDstSize);
}

/*******************************************************************************
NAME:       iptMarshallDSInt
ABSTRACT:   Marshalls a dataset (Conditional marshalling).
RETURNS:    0 if OK, !=0 if error
*/
int iptMarshallDSInt(
   UINT8 conditional,	/* If != 0, honor disableMarshalling flags */ 
   UINT32 datasetId,	/* DatasetId for this data, used to get formatting info */
   const BYTE *pSrc, 	/* Pointer to source data */
   BYTE *pDst,       	/* OUT: Pointer to destination buffer */
   UINT32 *pDstSize) 	/* IN: size of destination buffer	*/
                     	/* OUT: no of bytes written */ 
{
   int ret;
   MARSHALL_INFO info;
   IPT_CFG_DATASET_INT *pDataset;

   if (datasetId == 0)
   {
      return (int)IPT_NOT_FOUND;
   }

   /* Get pointer to dataset for this datasetId */
   pDataset = iptConfigGetDataset(datasetId);
   if (pDataset == NULL)
   {
      return (int)IPT_MARSHALL_UNKNOWN_DATASETID;
	}

   if (conditional != 0 &&
   	(IPTGLOBAL(disableMarshalling) || pDataset->disableMarshalling))
   {
      /* Do not perform marshalling, just copy from source to destination. Use destination size */
      memcpy(pDst, pSrc, *pDstSize);
      ret = 0;
   }
   else
   {
      /* Set up all info needed, then start marshalling */
      info.level = 0;
      info.pSrc = pSrc;
      info.pDst = pDst;
      info.pDstEnd = pDst + *pDstSize;
      memset(pDst, 0, *pDstSize);

      ret = marshall(pDataset->nLines, pDataset->alignment, pDataset->pFormat, &info);

      *pDstSize = info.pDst - pDst; /* Return no of bytes written in destination buffer */
   }
/*lint !e429 Custodial pointer OK*/
   return ret; /*lint !e429 Custodial pointer OK*/
}

/*******************************************************************************
NAME:       iptMarshallDSF
ABSTRACT:   Marshalls a dataset.
RETURNS:    0 if OK, !=0 if error
*/
int iptMarshallDSF(
   UINT32 nLines,
   UINT16 alignment,
   UINT8	 lDisableMarshalling,
   IPT_DATA_SET_FORMAT_INT *pFormat,
   const BYTE *pSrc, /* Pointer to source data */
   BYTE *pDst,       /* OUT: Pointer to destination buffer */
   UINT32 *pDstSize) /* IN: size of destination buffer*/
                     /* OUT: no of bytes written */ 
{
   int ret = 0;
   MARSHALL_INFO info;

   if (IPTGLOBAL(disableMarshalling) || lDisableMarshalling)
   {
      /* Do not perform marshalling, just copy from source to destination. Use destination size */
      memcpy(pDst, pSrc, *pDstSize);
   }
	else
	{

      /* Set up all info needed, then start marshalling */
      info.level = 0;
      info.pSrc = pSrc;
      info.pDst = pDst;
      info.pDstEnd = pDst + *pDstSize;
      memset(pDst, 0, *pDstSize);

      ret = marshall(nLines, alignment, pFormat, &info);

      *pDstSize = info.pDst - pDst; /* Return no of bytes written in destination buffer */
   }
   
  return ret;
}

/*******************************************************************************
NAME:       iptUnmarshall
ABSTRACT:   Unmarshalls PD data and unpacks into a receiving buffer.
RETURNS:    0 if OK, !=0 if error
*/
int iptUnmarshall(
   UINT32 comId,      /* Comid for this data, used to get formatting info */
   BYTE *pSrc,        /* Pointer to source data */
   BYTE *pDst,        /* OUT: Pointer to destination buffer */
   UINT32 *pDstSize)  /* IN: size of destination buffer */
                      /* OUT: no of bytes written */ 
{
   int ret;
   UINT32 datasetId;

   ret = iptConfigGetDatasetId(comId, &datasetId);
   if (ret == (int)IPT_OK)
   {
      ret = iptUnmarshallDSInt(0, datasetId, pSrc, pDst, pDstSize);
   }
   
   return ret;
}

/*******************************************************************************
NAME:       iptUnmarshallDS
ABSTRACT:   Unmarshalls a dataset uncondionally.
RETURNS:    0 if OK, !=0 if error
*/
int iptUnmarshallDS(
   UINT32 datasetId,  /* Dataset id for this data, used to get formatting info */
   BYTE *pSrc,        /* Pointer to source data */
   BYTE *pDst,        /* OUT: Pointer to destination buffer */
   UINT32 *pDstSize)  /* IN: size of destination buffer */
                      /* OUT: no of bytes written */ 
{
	return iptUnmarshallDSInt(1, datasetId, pSrc, pDst, pDstSize);
}

/*******************************************************************************
NAME:       iptUnmarshallDSInt
ABSTRACT:   Unmarshalls a dataset conditionally.
RETURNS:    0 if OK, !=0 if error
*/
int iptUnmarshallDSInt(
	UINT8 conditional,
   UINT32 datasetId,  /* Dataset id for this data, used to get formatting info */
   BYTE *pSrc,        /* Pointer to source data */
   BYTE *pDst,        /* OUT: Pointer to destination buffer */
   UINT32 *pDstSize)  /* IN: size of destination buffer */
                      /* OUT: no of bytes written */ 
{
   int ret;
   MARSHALL_INFO info;
   IPT_CFG_DATASET_INT *pDataset;

   if (datasetId == 0)
   {
      return (int)IPT_NOT_FOUND;
   }

   /* Get pointer to dataset for this datasetId */
   pDataset = iptConfigGetDataset(datasetId);
   if (pDataset == NULL)
   {
      return (int)IPT_MARSHALL_UNKNOWN_DATASETID;
	}

   if (conditional != 0 &&
   	(IPTGLOBAL(disableMarshalling) || pDataset->disableMarshalling))
   {
      /* Do not perform unmarshalling, just copy from source to destination. Use destination size */
      memcpy(pDst, pSrc, *pDstSize);
      ret = 0;
   }
   else
   {
      /* Set up all info needed, then start unmarshalling */
      info.level = 0;
      info.pSrc = pSrc;
      info.pDst = pDst;
      info.pDstEnd = pDst + *pDstSize;
      memset(pDst, 0, *pDstSize);

      ret = unmarshall(pDataset->nLines, pDataset->alignment, pDataset->pFormat, &info);

      *pDstSize = info.pDst - pDst; /* Return no of bytes written in destination buffer */
	}
   return ret;
}

/*******************************************************************************
NAME:       iptUnmarshallDSF
ABSTRACT:   Unmarshalls a dataset.
RETURNS:    0 if OK, !=0 if error
*/
int iptUnmarshallDSF(
   UINT32 nLines,
   UINT16 alignment,
   UINT8 lDisableMarshalling,
   IPT_DATA_SET_FORMAT_INT *pFormat,
   BYTE *pSrc,        /* Pointer to source data */
   BYTE *pDst,        /* OUT: Pointer to destination buffer */
   UINT32 *pDstSize)  /* IN: size of destination buffer */
{
   int ret = 0;
   MARSHALL_INFO info;

   if (IPTGLOBAL(disableMarshalling) || lDisableMarshalling)
   {
      /* Do not perform unmarshalling, just copy from source to destination. Use destination size */
      memcpy(pDst, pSrc, *pDstSize);
   }
	else
	{

      /* Set up all info needed, then start unmarshalling */
      info.level = 0;
      info.pSrc = pSrc;
      info.pDst = pDst;
      info.pDstEnd = pDst + *pDstSize;
      memset(pDst, 0, *pDstSize);

      ret = unmarshall(nLines, alignment, pFormat, &info);

      *pDstSize = info.pDst - pDst; /* Return no of bytes written in destination buffer */
	}   
   return ret;
}

/*******************************************************************************
NAME:       iptCalcGetDatasetSize
ABSTRACT:   Calculate size of a dataset. 
            For fixed size dataset this should already have been calculated. 
            For variable size datasets the size is calculated based on current data.
            Note! The source data is expected to be marshalled data, i.e. data from wire.
RETURNS:    0 = OK !=0 if error
*/
int iptCalcGetDatasetSize(
   UINT32 datasetId,  /* IN dataset ID */
   BYTE *pSrc,        /* IN Pointer to source data */
   UINT32 *pSize,     /* OUT Pointer to size */
   IPT_CFG_DATASET_INT **ppDataset)
{
   IPT_CFG_DATASET_INT *pDataset;
   int ret = IPT_OK;
   MARSHALL_INFO info;

   pDataset = iptConfigGetDataset(datasetId);
   if ((pDataset == NULL) || (pDataset->prepared == FALSE ))
   {
      *pSize = 0; 
      ret = (int)IPT_MARSHALL_UNKNOWN_DATASETID;
      
   }
   else if (!pDataset->varSize)
   {
      *pSize = (UINT32)pDataset->size;         /* Fixed size dataset, already calculated */
   }
   else
   {
      /* Caclulate size of dataset based on current data */
      /* Set up all info needed, then start calculating */
      info.level = 0;
      info.pSrc = pSrc;
      info.pDst = 0;

      ret = calcDatasetSize(pDataset->nLines, pDataset->alignment, pDataset->pFormat, &info);
      if (ret == (int)IPT_OK)
      {
         *pSize = (UINT32)info.pDst;
      }
      else
      {
         *pSize = (UINT32)info.pDst;
      }
   }

   *ppDataset = pDataset;

   return ret;
}

/*******************************************************************************
NAME:       iptCalcDatasetSize
ABSTRACT:   Calculate size of a dataset. 
            For fixed size dataset this should already have been calculated. 
            For variable size datasets the size is calculated based on current data.
            Note! The source data is expected to be marshalled data, i.e. data from wire.
RETURNS:    0 = OK !=0 if error
*/
int iptCalcDatasetSize(
   UINT32 datasetId,  /* IN dataset ID */
   BYTE *pSrc,        /* IN Pointer to source data */
   UINT32 *pSize)     /* OUT Pointer to size */
{
   IPT_CFG_DATASET_INT *pDataset;
   int ret = IPT_OK;
   MARSHALL_INFO info;
   UINT32 size = *pSize;

   pDataset = iptConfigGetDataset(datasetId);
   if ((pDataset == NULL) || (pDataset->prepared == FALSE ))
   {
      *pSize = 0; 
      ret = (int)IPT_MARSHALL_UNKNOWN_DATASETID;
      
   }
   else if (!pDataset->varSize)
   {
      *pSize = (UINT32)pDataset->size;         /* Fixed size dataset, already calculated */
   }
   else
   {
      /* Caclulate size of dataset based on current data */
      /* Set up all info needed, then start calculating */
      info.level = 0;
      info.pSrc = pSrc;
      info.pDst = 0;

      ret = calcDatasetSize(pDataset->nLines, pDataset->alignment, pDataset->pFormat, &info);
      
      *pSize = (UINT32)info.pDst;

      iptAlignStruct((BYTE **)(void *)pSize, pDataset->alignment);

      /* CR 7779, Check for ill-formated frames */
      if (size && (size != *pSize)) {
         IPTVosPrint3(IPT_WARN, "Size of calculateded datasetId=%d is %d, expected %d\n",datasetId, *pSize, size);
         return IPT_ILLEGAL_SIZE;
      }

   }
   
   return ret;
}

/*******************************************************************************
NAME:       iptCalcSendBufferSize
ABSTRACT:   Calculate size of a PD/MD send buffer so the added FCS will fit.
If size of source is not a multiple of 4 bytes then padding is added.
RETURNS:    Size of buffer in bytes
*/
UINT32 iptCalcSendBufferSize(
   UINT32 srcSize)   /* Size of source data in bytes */
{
   UINT32 calcSize;
   
   if (srcSize == 0)
   {
      calcSize = 4;
   }
   else
   {
      srcSize = ((srcSize + 3) / 4) * 4;     /* Align size */
      
      calcSize = srcSize + 4 * ((srcSize + (IPT_BLOCK_LENGTH - 4)) / IPT_BLOCK_LENGTH);
   }
   
   return calcSize;
}

/*******************************************************************************
NAME:       iptLoadSendDataFCS
ABSTRACT:   Loads PD or MD data into a send buffer and adds a FCS (Frame Check Sequence)
after every 256 bytes and after the data.
If size of source is not a multiple of 4 bytes then padding is added to the 
destination.
RETURNS:    0 if OK, !=0 if error
*/
int iptLoadSendData(
   const BYTE *pSrc,  /* Pointer to source data */
   UINT32 srcSize,    /* Size of source data, in bytes */
   BYTE *pDst,        /* OUT: Pointer to destination buffer */
   UINT32 *pDstSize)  /* IN: size of destination buffer, in bytes */
                      /* OUT: no of bytes written */ 
{
   UINT32 calcDstSize, blockCtr;
   UINT32 fcs;
   BYTE b;
   
   /* Check the size of the send buffer */
   calcDstSize = iptCalcSendBufferSize(srcSize);
   if (calcDstSize > *pDstSize)
      return (int)IPT_ILLEGAL_SIZE;
   
   /* Start copying and calculating FCS at the same time */
   fcs = INITFCS;
   blockCtr = IPT_BLOCK_LENGTH;
   
   while (srcSize-- > 0)
   {
      b = *pSrc++;
      fcs = (fcs >> 8) ^ fcstab[(fcs ^ b) & 0xff];
      *pDst++ = b;
      
      /* Add FCS after each 256 bytes */
      if ((--blockCtr == 0) && (srcSize != 0))
      {
         /* Time for a FCS */
         fcs ^= 0xffffffff;               /* complement */
         *pDst++ = (BYTE) (fcs & 0x00ff);     /* least significant byte first */
         *pDst++ = (BYTE) ((fcs >>= 8) & 0x00ff);
         *pDst++ = (BYTE) ((fcs >>= 8) & 0x00ff);
         *pDst++ = (BYTE) ((fcs >> 8) & 0x00ff);
         
         fcs = INITFCS;
         blockCtr = IPT_BLOCK_LENGTH;
      }
   }
   
   /* Pad with zeroes if needed */
   while (((UINT32) pDst & 0x3) != 0)
   {
      b = 0;
      fcs = (fcs >> 8) ^ fcstab[(fcs ^ b) & 0xff];
      *pDst++ = b;
   }
   
   /* Add FCS at end */
   fcs ^= 0xffffffff;               /* complement */
   *pDst++ = (BYTE) (fcs & 0x00ff);     /* least significant byte first */
   *pDst++ = (BYTE) ((fcs >>= 8) & 0x00ff);
   *pDst++ = (BYTE) ((fcs >>= 8) & 0x00ff);
   *pDst++ = (BYTE) ((fcs >> 8) & 0x00ff);
   
   *pDstSize = calcDstSize;    /* Return no of UINT32's written */
   return 0;
}

/*******************************************************************************
NAME:       iptLoadReceiveDataFCS
ABSTRACT:   Load received PD or MD data with FCS (Frame Check Sequence) after each 
256 byte into a buffer, checking and removing the FCS's.
Received buffer must be aligned
RETURNS:    0 if OK, !=0 if error
*/ 
int iptLoadReceiveDataFCS(
   BYTE *pSrc,        /* Pointer to source data */
   UINT32 srcSize,    /* Size of source data */
   BYTE *pDst,        /* OUT: Pointer to destination buffer */
   UINT32 *pDstSize)  /* IN: size of destination buffer */
                      /* OUT: no of bytes written */
{ 
   UINT32 calcDstSize, blockCtr; 
   UINT32 fcs; 
#ifdef FCS_CHECK
   int i;
#endif
   BYTE b;
    
   /* Check that we have at least 4 characters 
   in src and that the source size is a multiple of 4 bytes */
   if (srcSize < 4 || (srcSize & 0x3) != 0)
   { 
      *pDstSize = 0; 
      return (int)IPT_ILLEGAL_SIZE;
   } 
   /* Calculate size of a PD/MD receive buffer after added FCS are removed.
      If size of source is not a multiple of 4 bytes then padding is added */
   calcDstSize = srcSize - 4 - 4 * ((srcSize - 4) / (IPT_BLOCK_LENGTH + 4));
    
    
   /* Check the size of the destination buffer */
   if (calcDstSize > *pDstSize)
   { 
      *pDstSize = 0; 
      return (int)IPT_ILLEGAL_SIZE;
   } 
    
   /* Start copying and calculating FCS at the same time */
#ifdef FCS_CHECK
   fcs = INITFCS; 
#endif 
   blockCtr = IPT_BLOCK_LENGTH; 
   srcSize -= 4;  /* End when only last FCS left */
    
   while (srcSize > 0) 
   { 
      /* Check FCS after 256 bytes */
      if (blockCtr-- == 0)
      { 
#ifdef FCS_CHECK
         for (i = 0; i < 4; i++)
         { 
            fcs = (fcs >> 8) ^ fcstab[(fcs ^ *pSrc++) & 0xff];
         } 
          
         if (fcs != GOODFCS)
         { 
            *pDstSize = 0; 
            return (int)IPT_FCS_ERROR;
         } 
         fcs = INITFCS; 
#else 
         pSrc += 4; 
#endif 
         blockCtr = IPT_BLOCK_LENGTH; 
         srcSize -= 4; 
      } 
      else
      { 

         b = *pSrc++; 
#ifndef NO_FCS_CHECK 
         fcs = (fcs >> 8) ^ fcstab[(fcs ^ b) & 0xff];
#endif
         *pDst++ = b; 
         srcSize--; 
      } 
   } 

#ifdef FCS_CHECK 
   /* Check FCS at end */
   for (i = 0; i < 4; i++)
   { 
      fcs = (fcs >> 8) ^ fcstab[(fcs ^ *pSrc++) & 0xff];
   } 
    
   if (fcs != GOODFCS)
   { 
      *pDstSize = 0; 
      return (int)IPT_FCS_ERROR;
   } 
#endif   
   *pDstSize = calcDstSize;    /* Return no of bytes written */
   return 0;
} 

/*******************************************************************************
NAME:       iptAddDataFCS
ABSTRACT:   Adds FCS (Frame Check Sequence) after PD or MD data.
Note!       There must be 4 bytes reserved after the data for the FCS.
FCS is added at aligned adress, padded with zero.
RETURNS:    -
*/
void iptAddDataFCS(
   BYTE *pSrc,      /* Pointer to source data */
   UINT32 srcSize)   /* Size of source data */
{
   UINT32 fcs;
   BYTE b;
   
   /* Calculate FCS  */
   fcs = INITFCS;
   while (srcSize-- > 0)
   {
      fcs = (fcs >> 8) ^ fcstab[(fcs ^ *pSrc++) & 0xff];
   }
   
   /* Pad with zero to get aligned address */
   b = 0;
   while (((UINT32) pSrc & 0x3) != 0)
   {
      *pSrc++ = b;
      fcs = (fcs >> 8) ^ fcstab[(fcs ^ b) & 0xff];
   }
   
   /* Add FCS at end */
   fcs ^= 0xffffffff;               /* complement */
   *pSrc++ = (BYTE) (fcs & 0x00ff);     /* least significant byte first */
   *pSrc++ = (BYTE) ((fcs >>= 8) & 0x00ff);
   *pSrc++ = (BYTE) ((fcs >>= 8) & 0x00ff);
   *pSrc++ = (BYTE) ((fcs >> 8) & 0x00ff);
   
}

/*******************************************************************************
NAME:       iptCheckFCS
ABSTRACT:   Checks FCS (Frame Check Sequence) for a PD or MD data buffer.
RETURNS:    0 if OK, !=0 if not.
*/
/*lint -save -esym(429, pSrc) pDst is not custotory */
int iptCheckFCS(
   BYTE *pSrc,        /* Pointer to source data */
   UINT32 srcSize)    /* Size of source data */
{
   UINT32 fcs;
   
   /* Start calculating FCS  */
   fcs = INITFCS;
   
   while (srcSize-- > 0)
   {
      fcs = (fcs >> 8) ^ fcstab[(fcs ^ *pSrc++) & 0xff];
   }
   
   if (fcs != GOODFCS)
      return (int)IPT_FCS_ERROR;
   else
      return 0;
}
/*lint -restore*/

/*******************************************************************************
NAME:       iptCheckRecTopoCnt
ABSTRACT:   Check that the received topo counter is   equal to the own current
            topo counter value.
RETURNS:    IPT_OK if OK, IPT_ERROR if not.
*/
int iptCheckRecTopoCnt(
   UINT32 recTopoCnt
   #ifdef TARGET_SIMU
   ,
   UINT32 simuIpAddr
   #endif
   )
{
   UINT8 inaugState;
   UINT8 ownTopoCnt;    
   T_TDC_RESULT res;

   if (IPTGLOBAL(tdcsim.enableTDCSimulation))
   {
      return((int)IPT_OK);
   }
   else
   {
      ownTopoCnt = 0;
#ifdef TARGET_SIMU
      if ((res = tdcGetIptStatePiq (&inaugState, &ownTopoCnt, simuIpAddr)) == TDC_OK)
#else
      if ((res = tdcGetIptState (&inaugState, &ownTopoCnt)) == TDC_OK)
#endif
      {
         if (inaugState != TDC_IPT_INAUGSTATE_OK)
         {
            IPTVosPrint1(IPT_WARN,
                   "tdcGetIptState returned inaugState=%d\n",
                   inaugState);
            return((int)IPT_ERROR);
         }
         if (ownTopoCnt == recTopoCnt)
         {
            return((int)IPT_OK);
         }
         else
         {
            IPTVosPrint2(IPT_WARN,
                      "iptCheckRecTopoCnt wrong topo conter rec=%d cur=%d\n",
                      recTopoCnt,ownTopoCnt);
            return((int)IPT_ERROR);
         }
      }
      else
      {
         IPTVosPrint1(IPT_ERR,
                      "tdcGetIptState returned error code=%#x\n",
                      res);
         return((int)IPT_ERROR);
      }
   }
}

/*******************************************************************************
NAME:       iptGetUriHostPart
ABSTRACT:   Get URI from an IP address 
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT iptGetUriHostPart (
   T_IPT_IP_ADDR   ipAddr,       /* input  */
   T_IPT_URI       uri,          /* output */
   UINT8          *pTopoCnt)     /* Pointer to topocounter, set to 0 if ignore */      
{
   if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      return tdcSimGetUriHostPart (ipAddr, uri, pTopoCnt);
   else
      return tdcGetUriHostPart (ipAddr, uri, pTopoCnt);
}

/* Public non-TDC type version */
int IPTCom_getUriHostPart (
   UINT32 ipAddr,       /* input  */
   char *pUri,          /* output */
   UINT8 *pTopoCnt)     /* Pointer to topocounter, set to 0 if ignore */  
{
   return iptGetUriHostPart(ipAddr, pUri, pTopoCnt);
}

#ifdef TARGET_SIMU
/*******************************************************************************
NAME:       iptGetUriHostPartSim
ABSTRACT:   Get URI from an IP address 
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT iptGetUriHostPartSim (
   T_IPT_IP_ADDR   ipAddr,       /* input  */
   UINT32          simuIpAddr, 
   T_IPT_URI       uri,          /* output */
   UINT8          *pTopoCnt)     /* Pointer to topocounter, set to 0 if ignore */      
{
   if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      return tdcSimGetUriHostPart (ipAddr, uri, pTopoCnt);
   else
      return tdcGetUriHostPartPiq (ipAddr, uri, pTopoCnt, simuIpAddr);
}
#endif

/*******************************************************************************
NAME:       iptGetAddrByName
ABSTRACT:   Get IP address from an URI
Returns the string converted to UINT32.
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT iptGetAddrByName(
   const T_IPT_URI uri,       /* input  */
   T_IPT_IP_ADDR *pIpAddr,    /* output */
   UINT8         *pTopoCnt)   /* Pointer to topocounter, set to 0 if ignore */    
{
   const char *p;
   int ret;

   if (uri == 0)
   {
      return((int)IPT_ERROR);
   }

   /* Skip any 'instance.function@' part before host part */
   p = strrchr(uri, '@');
   if (p != NULL)
      uri = p + 1;

   if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      ret = tdcSimGetAddrByName (uri, pIpAddr, pTopoCnt);
   else
   {
      ret = tdcGetAddrByName (uri, pIpAddr, pTopoCnt);
   }

   if (ret != (int)IPT_OK)
   {
      if ((ret != TDC_NO_CONFIG_DATA) && (ret != TDC_MUST_FINISH_INIT))
      {
         IPTVosPrint2(IPT_ERR, "iptGetAddrByName: No IP address for URI = %s. TDC ERROR=%#x\n", uri, ret);
      }
      else
      {
         IPTVosPrint1(IPT_WARN, "iptGetAddrByName: No IP address for URI = %s. TDC not ready\n", uri);
      }
   }


   return ret;
}

/* Public non-TDC type version */
int IPTCom_getAddrByName(
   const char *pUri,          /* Pointer to URI string */
   UINT32 *pIpAddr,           /* output */
   UINT8 *pTopoCnt)           /* Pointer to topocounter, set to 0 if ignore */
{
   return iptGetAddrByName(pUri, pIpAddr, pTopoCnt);
}


#ifdef TARGET_SIMU
/*******************************************************************************
NAME:       iptGetAddrByNameSim
ABSTRACT:   Get IP address from an URI
Returns the string converted to UINT32.
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT iptGetAddrByNameSim(
   const T_IPT_URI uri,       /* input  */
   UINT32        simuIpAddr, 
   T_IPT_IP_ADDR *pIpAddr,    /* output */
   UINT8         *pTopoCnt)   /* Pointer to topocounter, set to 0 if ignore */    
{
   const char *p;
   int ret;

   if (uri == 0)
   {
      return((int)IPT_ERROR);
   }

   /* Skip any 'instance.function@' part before host part */
   p = strrchr(uri, '@');
   if (p != NULL)
      uri = p + 1;

   if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      ret = tdcSimGetAddrByName (uri, pIpAddr, pTopoCnt);
   else
   {
      ret = tdcGetAddrByNamePiq (uri, pIpAddr, pTopoCnt, simuIpAddr);
   }

   if (ret != (int)IPT_OK)
   {
      if ((ret != TDC_NO_CONFIG_DATA) && (ret != TDC_MUST_FINISH_INIT))
      {
         IPTVosPrint2(IPT_ERR, "iptGetAddrByNameSim: No IP address for URI = %s. TDC ERROR=%#x\n", uri, ret);
      }
      else
      {
         IPTVosPrint1(IPT_WARN, "iptGetAddrByNameSim: No IP address for URI = %s. TDC not ready\n", uri);
      }
   }


   return ret;
}
#endif
/*******************************************************************************
NAME:       iptGetAddrByNameExt
ABSTRACT:   Get IP address from an URI
Returns the string converted to UINT32.
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT iptGetAddrByNameExt(
   const T_IPT_URI uri,       /* input  */
   T_IPT_IP_ADDR *pIpAddr,    /* output */
   T_TDC_BOOL    *pFrg,       /* Pointer to FRG */
   UINT8         *pTopoCnt)   /* Pointer to topocounter, set to 0 if ignore */    
{
   const char *p;
   T_TDC_RESULT ret;

   if (uri == 0)
   {
      return((int)IPT_ERROR);
   }
   
   /* Skip any 'instance.function@' part before host part */
   p = strrchr(uri, '@');
   if (p != NULL)
      uri = p + 1;

   if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      ret = tdcSimGetAddrByNameExt (uri, pIpAddr, pFrg, pTopoCnt);
   else
   {
      ret = tdcGetAddrByNameExt (uri, pIpAddr, pFrg, pTopoCnt);
   }

   if (ret != (int)IPT_OK)
      IPTVosPrint2(IPT_ERR, "iptGetAddrByNameExt: No IP address for URI = %s. TDC ERROR=%#x\n", uri, ret);

   return ret;
}

/* Public non-TDC type version */
int IPTCom_getAddrByNameExt(
   const char *pUri,       /* Pointer to URI */
   UINT32 *pIpAddr,        /* Pointer to resulting IP address */
   int *pFrg,              /* Pointer to FRG */
   UINT8  *pTopoCnt)       /* Pointer to topocounter, set to 0 if ignore */    
{
   return iptGetAddrByNameExt(pUri, pIpAddr, pFrg, pTopoCnt);       
}


#ifdef TARGET_SIMU
/*******************************************************************************
NAME:       iptGetAddrByNameExtSim
ABSTRACT:   Get IP address from an URI
Returns the string converted to UINT32.
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT iptGetAddrByNameExtSim(
   const T_IPT_URI uri,       /* input  */
   UINT32        simuIpAddr, 
   T_IPT_IP_ADDR *pIpAddr,    /* output */
   T_TDC_BOOL    *pFrg,       /* Pointer to FRG */
   UINT8         *pTopoCnt)   /* Pointer to topocounter, set to 0 if ignore */    
{
   const char *p;
   T_TDC_RESULT ret;

   if (uri == 0)
   {
      return((int)IPT_ERROR);
   }
   
   /* Skip any 'instance.function@' part before host part */
   p = strrchr(uri, '@');
   if (p != NULL)
      uri = p + 1;

   if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      ret = tdcSimGetAddrByNameExt (uri, pIpAddr, pFrg, pTopoCnt);
   else
   {
      ret = tdcGetAddrByNameExtPiq(uri, pIpAddr, pFrg, pTopoCnt, simuIpAddr);
   }

   if (ret != (int)IPT_OK)
      IPTVosPrint2(IPT_ERR, "iptGetAddrByNameExtSim: No IP address for URI = %s. TDC ERROR=%#x\n", uri, ret);

   return ret;
}
#endif

/*******************************************************************************
NAME:       iptGetOwnIds
ABSTRACT:   Get own device, car and consist URI labels
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT iptGetOwnIds(
   T_IPT_LABEL devId, /* output */ 
   T_IPT_LABEL carId, /* output */
   T_IPT_LABEL cstId) /* output */
{
   if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      return tdcSimGetOwnIds (devId, carId, cstId);
   else
      return tdcGetOwnIds (devId, carId, cstId);
}

/* Public non-TDC type version */
int IPTCom_getOwnIds(
   char *pDevId,     /* output */ 
   char *pCarId,     /* output */
   char *pCstId)     /* output */
{
   return iptGetOwnIds(pDevId, pCarId, pCstId);
}

/*******************************************************************************
NAME:       iptGetTopoCnt
ABSTRACT:   Get topo counter out of IP address
RETURNS:    0 if OK, <>0 if not.
*/
int iptGetTopoCnt(
   UINT32 destIpAddr, /* Destination IP address */
   #ifdef TARGET_SIMU
   UINT32 simuIpAddr,
   #endif
   UINT32 *pTopoCnt)  /* Pointer to topo counter */
{
   int ret = (int)IPT_OK;
   UINT8 ownTopoCnt;    
   UINT8 inaugState;
   T_TDC_RESULT res;

   /* Own consist ? */
   if(isOwnConsistAddr(destIpAddr))
   {
      *pTopoCnt = 0;  
   }
   else
   {
      if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      {
         /* Topo counter value 1 is always used in simulation mode */
         *pTopoCnt = 1;  
      }
      else
      {
         ownTopoCnt = 0;

#ifdef TARGET_SIMU
         if ((res = tdcGetIptStatePiq (&inaugState, &ownTopoCnt, simuIpAddr)) == TDC_OK)
#else
         if ((res = tdcGetIptState (&inaugState, &ownTopoCnt)) == TDC_OK)
#endif
         {
            if (ownTopoCnt != 0)
            {
               *pTopoCnt = ownTopoCnt;   
            }
            else
            {
               IPTVosPrint1(IPT_WARN,
                      "Zero topo counter. ETB message can not be sent\n",
                      inaugState);
               ret = (int)IPT_TDC_NOT_READY;
            }
         }
         else
         {
            IPTVosPrint1(IPT_ERR,
                         "tdcGetIptState returned error code=%#x\n",
                         res);
               ret = (int)IPT_ERROR;
         }
      }
   }
   return(ret);
}

/*******************************************************************************
 NAME:       IPTCom_disableMarshalling
 ABSTRACT:   Disable marshalling in PDCom and MDCom.
 Marshalling is the adjustment to data so it complies with the standard
 on the wire, e.g. big/little-endianess.
 RETURNS:    -
 */
void IPTCom_disableMarshalling(
                               int disable)  /* Set 1 (=TRUE) to disable marshalling */
{
   IPTGLOBAL(disableMarshalling) = disable;
}

/*******************************************************************************
 NAME:       enableFrameSizeCheck
 ABSTRACT:   Enable frame size check in MDCom.
 Ensures, that too small frames are negated
 RETURNS:      -
 */
void IPTCom_enableFrameSizeCheck(
                               int enable)  /* Set 1 (=TRUE) to enable frame size check */
{
   IPTGLOBAL(enableFrameSizeCheck) = enable;
}

#ifdef LINUX_MULTIPROC
/*******************************************************************************
NAME:       IPTCom_MPattach
ABSTRACT:   Attaches to an IPTCom already started with IPTCom_prepareInit
            by another process in a Linux multi-processing system.
RETURNS:    0 if OK, !=0 if not
*/
int IPTCom_MPattach(void)
{
   pIptGlobal = (IPT_GLOBAL *)((void *) IPTVosAttachSharedMemory());
   if (pIptGlobal == NULL)
   {
      printf ("\nIPTCom - Ver. %d.%d.%d.%d Buildtime: %s, %s\n", 
             IPT_VERSION, IPT_RELEASE, IPT_UPDATE, IPT_EVOLUTION,__DATE__, __TIME__);

      IPTVosPrint0(IPT_ERR,"IPTCom_MPattach FAILED\n");
      return (int)IPT_ERROR;
   }
   
   printf ("\nIPTCom - Ver. %d.%d.%d.%d Buildtime: %s\n", 
          IPT_VERSION, IPT_RELEASE, IPT_UPDATE, IPT_EVOLUTION, IPTGLOBAL(buildTime));

#ifdef WIN32
   /* Create memory allocation semaphore */
   if (IPTVosCreateSem(&IPTGLOBAL(mem.sem), IPT_SEM_FULL) != (int)IPT_OK)
      return (int)IPT_SEM_ERR;  /* Could not create sempahore */

   if (IPTVosCreateSem(&IPTGLOBAL(configDB.sem), IPT_SEM_FULL) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "Could not create config semaphore\n");
      return (int)IPT_SEM_ERR;
   }

   if (IPTVosCreateSem(&IPTGLOBAL(md.remQueueSemId), IPT_SEM_FULL) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "ERROR creating remQueueSemId\n");
      return IPT_SEM_ERR;
   }

   /* Create a semaphore for the listeners list resource
    initial state = free */
   if (IPTVosCreateSem(&IPTGLOBAL(md.listenerSemId), IPT_SEM_FULL) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "ERROR creating  listenerSemId\n");
      return IPT_SEM_ERR;
   }

   /* Create a semaphore for the session layer list resource
    initial state = free */
   if (IPTVosCreateSem(&IPTGLOBAL(md.seSemId), IPT_SEM_FULL) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "ERROR creating session layer semaphore\n");
      return IPT_SEM_ERR;
   }

   /* Create a semaphore for the transport layer list resource
    initial state = free */
   if (IPTVosCreateSem(&IPTGLOBAL(md.trListSemId), IPT_SEM_FULL) != (int)IPT_OK)
   {
      /* Create a semaphore for the send sequence list resource
       initial state = free */
      IPTVosPrint0(IPT_ERR,
                   "ERROR creating tranport layer list resource semaphore\n");
      return IPT_SEM_ERR;
   }
   
   if (IPTVosCreateSem(&IPTGLOBAL(md.SendSeqSemId), IPT_SEM_FULL) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR,
                   "ERROR creating send sequence list resource semaphore\n");
      return IPT_SEM_ERR;
   }
   
   if (IPTVosCreateSem(&IPTGLOBAL(pd.sendSem), IPT_SEM_FULL) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR,
                   "ERROR creating pd send semaphore\n");
      return IPT_SEM_ERR;
   }
   
   if (IPTVosCreateSem(&IPTGLOBAL(pd.recSem), IPT_SEM_FULL) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR,
                   "ERROR creating pd receive semaphore\n");
      return IPT_SEM_ERR;
   }
   
   IPTVosSystemStartup();

#endif

   return 0;
}

/*******************************************************************************
NAME:       IPTCom_MPdetach
ABSTRACT:   Detaches a process that has been attached to an IPTCom 
            in a Linux multi-processing system.
RETURNS:    0 if OK, !=0 if not
*/
int IPTCom_MPdetach(void)
{
   
   return IPTVosDetachSharedMemory((char *) pIptGlobal);
}
#endif

/*******************************************************************************
NAME:       IPTCom_getErrorString
ABSTRACT:   Get error information string
RETURNS:    Error string
*/
const char *IPTCom_getErrorString(
   int errorNumber)
{
   unsigned int i;

   if ((errorNumber & ERR_IPTVCOM) != ERR_IPTVCOM)
   {
      if ((errorNumber & ERR_TDCCOM) == ERR_TDCCOM)
      {
         return "Error belonging to TDC and not to IPTCom.";
      }
      else
      {
         return "Error not belonging to IPTCom or TDC.";
      }
   }
   
   i = 0;
   while (i < sizeof(errorInfoTable)/sizeof(ERROR_INFO))
   {
      if (errorInfoTable[i].code == errorNumber)
      {
         return(errorInfoTable[i].pStr);
      }
      i++;
   }
   
   return("Unknown error");   
}

/*******************************************************************************
NAME:       IPTCom_getVersion
ABSTRACT:   Returns the SW version of the IPTCom.
RETURNS:    Software version as 32-bit integer (0xvvrruuee: version, release, update, evolution)
*/
UINT32 IPTCom_getVersion(void)
{
   return ((IPT_VERSION << 24) | (IPT_RELEASE << 16) | (IPT_UPDATE << 8) | IPT_EVOLUTION);
}

/*******************************************************************************
NAME:       IPTCom_getStatus
ABSTRACT:   Reads the IPTCom and TDC status.
RETURNS:    Returns the IPTCom status
*/
int IPTCom_getStatus(void)
{
   
   T_TDC_RESULT tdcRes;
   T_IPT_LABEL  devId;
   T_IPT_LABEL  carId;
   T_IPT_LABEL  cstId;

#ifdef LINUX_MULTIPROC
   if ((pIptGlobal != NULL) && (IPTGLOBAL(iptComInitiated)))
#else
   if (IPTGLOBAL(iptComInitiated))
#endif
   {
      
      if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      {
         return(IPTCOM_RUN);
      }
      else
      {
         /* Check TDC status */
         tdcRes = tdcGetOwnIds(devId, carId, cstId);
         if ((tdcRes == TDC_OK) || (tdcRes == TDC_NO_MATCHING_ENTRY))
         {
            return(IPTCOM_RUN);
         }
         else if (tdcRes == TDC_NO_CONFIG_DATA)
         {
            return(IPTCOM_TDC_NOT_CONFIGURED);
         }
         else
         {
            return(IPTCOM_NOT_INITIATED);
         }   
      }
   }
   else
   {
      return(IPTCOM_NOT_INITIATED);
   }
}

/*******************************************************************************
NAME:       IPTCom_getOwnIpAddr
ABSTRACT:   Get own IP address
RETURNS:    -
*/
UINT32 IPTCom_getOwnIpAddr(void)
{
   UINT32 i;
   UINT32 ipAddrCnt;
   UINT32 ipAddrs[20];
 
   /* Own IP address not set? */
   if (IPTGLOBAL(ownIpAddress) == 0)
   {
      /* Get own IP address
        First of all read own IP-Address. Allow any address except 127.0.0.1 
        and 0.0.0.0 to enable use of the emulator */
      ipAddrCnt = tdcGetOwnIpAddrs ("IPTCom", 20, ipAddrs);
      for (i = 0; i < ipAddrCnt; i++)
      {
         if (    ((ipAddrs[i] & 0xFFFFF000) == 0x0A000000)
              && ((ipAddrs[i] & 0x00000FFF) != 0x00000000)
              && ((ipAddrs[i] & 0x00000FFF) != 0x00000FFF)
            )
         {
            IPTGLOBAL(ownIpAddress) = ipAddrs[i];
            break;
         }
      }
   }
  
   return(IPTGLOBAL(ownIpAddress));
}

/*******************************************************************************
NAME:       iptStrcmp
ABSTRACT:   String compare with ignore case
RETURNS:    -
*/
int iptStrcmp(const char*  s1, const char*  s2)
{
   int ret = 0;

   if (s1 == NULL)
   {
      ret = -1;
   }
   else
   {
      if (s2 == NULL)
      {
         ret = 1;
      }
      else
      {
         while((ret == 0) && ((*s1 != 0) || (*s2 != 0)))
         {
            if (tolower (*s1) != tolower (*s2))
            {
               if (tolower (*s1) < tolower (*s2))
               {
                  ret = -1;
               }
               else
               {
                  ret = 1;
               }
            }
            else
            {
               s1++;
               s2++;
            }   
         }
      }
   }

   return(ret);
}

/*******************************************************************************
NAME:       iptStrcmp
ABSTRACT:   String compare with ignore case
RETURNS:    -
*/
int iptStrncmp(const char*  s1, const char*  s2, int len)
{
   
   int ret = 0;

   if (s1 == NULL)
   {
      ret = -1;
   }
   else
   {
      if (s2 == NULL)
      {
         ret = 1;
      }
      else
      {
         while((ret == 0) && ((*s1 != 0) || (*s2 != 0)) && (len > 0))
         {
            if (tolower (*s1) != tolower (*s2))
            {
               if (tolower (*s1) < tolower (*s2))
               {
                  ret = -1;
               }
               else
               {
                  ret = 1;
               }
            }
            else
            {
               s1++;
               s2++;
               len--;
            }   
         }
      }
   }

   return(ret);
}

