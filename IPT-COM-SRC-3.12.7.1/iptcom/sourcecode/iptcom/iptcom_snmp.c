/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2010 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     :  IPTrain
 *
 *  MODULE      :  iptcom_snmp.c
 *
 *  ABSTRACT    :  Functions for the SNMP agent interface 
 *                 for retrieval of statistics and diagnostics.
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 * $Id: iptcom_snmp.c 11859 2012-04-18 16:01:04Z gweiss $
 *
 *  CR-3477 (Bernd Löhr, 2012-02-18)
 * 			TÜV Assessment findings
 *
 *  CR-432 (Gerhard Weiss, 2010-11-24)
 *          Added more missing UNUSED Parameter Macros
 *
 *  Internal (Bernd Loehr, 2010-08-16) 
 * 			Old obsolete CVS history removed
 *
 *
 ******************************************************************************/

/*******************************************************************************
*  INCLUDES */

#include <string.h>

#include "iptcom.h"        /* Common type definitions for IPT */
#include "vos.h"           /* OS independent system calls */
#include "netdriver.h"
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"


/*******************************************************************************
*  DEFINES
*/
/* Fixed start part of an OID for IPTCom */
#define BER_MAX_bufLen        500   /* Max length of an response message */
#define BER_MAX_NVAR          20    /* Max no of variables in each message */

#define BER_CLASS             0xc0
#define BER_PC                0x20
#define BER_NUMBER            0x1f

#define BER_UNIVERSAL         0
#define BER_APPLICATION       (1 << 6)
#define BER_CONTEXT_SPECIFIC  (2 << 6)
#define BER_PRIVATE           (3 << 6)

#define BER_PRIMITIVE         0
#define BER_CONSTRUCTED       (1 << 5)

/* Primitive SNMP application types */
#define BER_INTEGER           (BER_UNIVERSAL | BER_PRIMITIVE | 0x02)
#define BER_OCTET_STRING      (BER_UNIVERSAL | BER_PRIMITIVE | 0x04)
#define BER_NULL              (BER_UNIVERSAL | BER_PRIMITIVE | 0x05)
#define BER_OID               (BER_UNIVERSAL | BER_PRIMITIVE | 0x06)
#define BER_IPADDRESS         (BER_APPLICATION | BER_PRIMITIVE | 0x00)
#define BER_COUNTER           (BER_APPLICATION | BER_PRIMITIVE | 0x01)
#define BER_GUAGE             (BER_APPLICATION | BER_PRIMITIVE | 0x02)
#define BER_TIMETICKS         (BER_APPLICATION | BER_PRIMITIVE | 0x03)
#define BER_SEQUENCE          (BER_UNIVERSAL | BER_CONSTRUCTED | 0x10)

/* Context-specific types within an SNMP Message */
#define BER_GET_REQUEST       (BER_CONTEXT_SPECIFIC | BER_CONSTRUCTED | 0x0)
#define BER_GET_NEXT_REQUEST  (BER_CONTEXT_SPECIFIC | BER_CONSTRUCTED | 0x1)
#define BER_GET_RESPONSE      (BER_CONTEXT_SPECIFIC | BER_CONSTRUCTED | 0x2)
#define BER_SET_REQUEST       (BER_CONTEXT_SPECIFIC | BER_CONSTRUCTED | 0x3)

/* Status codes */
#define BER_STATUS_NO_ERROR      0
#define BER_STATUS_TOO_BIG       1
#define BER_STATUS_NO_SUCH_NAME  2
#define BER_STATUS_BAD_VALUE     3
#define BER_STATUS_READ_ONLY     4
#define BER_STATUS_GEN_ERR       5

/*******************************************************************************
*  TYPEDEFS
*/

typedef struct
{
   UINT32 type;   /* BER type */
   UINT32 length; /* Length of value */
   union
   {
      INT32 integer;
      char octetString[IPT_STAT_STRING_LEN + 1];
      INT32 oid[IPT_STAT_OID_LEN];    /* Stops with a stopper */
      UINT32 ipAddress;
      UINT32 counter;
      UINT32 timeTicks;
   } value;
} BER_VALUE;

typedef struct
{
   UINT8 *pBuf;      /* Pointer to next token in buffer */
   UINT32 bufLen;    /* Length of remaining part of buffer */
   UINT8 *pBegin;    /* Pointer to beginning of buffer */
   UINT8 *pEnd;      /* Pointer to end of buffer(first byte after the real buffer) */
} BER_MSG_INFO;


/*******************************************************************************
*  GLOBAL DATA
*/


/*******************************************************************************
*  LOCAL DATA
*/


/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/
static int berEncodeUint32(BER_MSG_INFO *pInfo, UINT32 type, UINT32 value);


/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/* Decode functions */

/*******************************************************************************
NAME:       berDecodeLength
ABSTRACT:   Decodes a BER tag length. 
            If <128 it is the short form and it is the length itself.
            Otherwise it is the long form and bit0-7 indicates
            how many bytes that will follow that contains the length.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeLength(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 *pLength)      /* Pointer to length variable */
{
   UINT32 i, octets = 0, len;

   if (pInfo->bufLen < 1)
      return BER_STATUS_BAD_VALUE;

   len = pInfo->pBuf[0];
   
   if (len > 127)
   {
      octets = len & 0x3f;

      if (octets < 1 || octets > 4 || (octets + 1) > pInfo->bufLen)
         return BER_STATUS_BAD_VALUE;

      for (i = 0, len = 0; i < octets; i++)
      {
         len = (len << 8) + pInfo->pBuf[i + 1]; 
      }
   }
      
   *pLength = len;
   pInfo->pBuf += octets + 1;    /* Move msg pointer to after length part */
   pInfo->bufLen -= octets + 1;

   return 0;
}

/*******************************************************************************
NAME:       berDecodeNull
ABSTRACT:   Decodes a BER NULL. 
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeNull(
              BER_MSG_INFO *pInfo)  /* Pointer to message info structure */
{
   int ret = 0;

   if (pInfo->bufLen < 2 ||
       pInfo->pBuf[0] != BER_NULL ||
       pInfo->pBuf[1] != 0)
   {
      ret = BER_STATUS_BAD_VALUE;
   }
   else
   {
      pInfo->pBuf += 2;
      pInfo->bufLen -= 2;
   }

   return ret;
}

/*******************************************************************************
NAME:       berDecodeConstructed
ABSTRACT:   Decodes a BER NULL.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeConstructed(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT8 *pPduType,      /* Pointer to PDU type variable, BER_GET_REQUEST.. */
              UINT32 *pLength)      /* Pointer to length variable */
{
   int ret = 0;

   if (pInfo->bufLen < 2 ||
      !(pInfo->pBuf[0] == BER_GET_REQUEST ||
        pInfo->pBuf[0] == BER_GET_NEXT_REQUEST ||
        pInfo->pBuf[0] == BER_SET_REQUEST))
   {
      ret = BER_STATUS_BAD_VALUE;
   }
   else
   {
      *pPduType = pInfo->pBuf[0];
      pInfo->pBuf++;
      pInfo->bufLen--;
      ret = berDecodeLength(pInfo, pLength);
   }

   return ret;
}

/*******************************************************************************
NAME:       berDecodeSequence
ABSTRACT:   Decodes a BER Sequence.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeSequence(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 *pLength)      /* Pointer to length variable */

{
   int ret = 0;

   if (pInfo->bufLen < 2 || pInfo->pBuf[0] != BER_SEQUENCE)
      ret = BER_STATUS_BAD_VALUE;
   else
   {
      pInfo->pBuf++;
      pInfo->bufLen--;
      ret = berDecodeLength(pInfo, pLength);
   }

   return ret;
}

/*******************************************************************************
NAME:       berDecodeInteger
ABSTRACT:   Decodes a BER integer. The length byte tells how long it is. 
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeInteger(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              INT32 *pVal)          /* Pointer to integer */
{
   int ret = 0;
   UINT32 i, octets;
   INT32 value = 0;

   if (pInfo->bufLen < 2 ||
       pInfo->pBuf[0] != BER_INTEGER ||
       pInfo->pBuf[1] < 1 ||
       pInfo->pBuf[1] > 4 ||
       pInfo->pBuf[1] > pInfo->bufLen)
   {
      ret = BER_STATUS_BAD_VALUE;
   }
   else
   {
      octets = pInfo->pBuf[1];

      /* If first byte is negative the final integer is negative */
      if ((pInfo->pBuf[2] & 0x80) != 0)
         value = -1;    /* =0xffffffff */

      /* Get the rest of the bytes */
      for (i = 0; i < octets; i++)
         value = (value << 8) + pInfo->pBuf[i + 2];

      *pVal = value;
      pInfo->pBuf += octets + 2;
      pInfo->bufLen -= octets + 2;
   }

   return ret;
}

/*******************************************************************************
NAME:       berDecodeOctetString
ABSTRACT:   Decodes a BER Octet string. 
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeOctetString(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              char *pStr)           /* Pointer to string */
{
   int ret;
   UINT32 len;

   if (pInfo->bufLen < 1 ||
       pInfo->pBuf[0] != BER_OCTET_STRING)
   {
      ret = BER_STATUS_BAD_VALUE;
   }
   else
   {
      pInfo->pBuf++;
      pInfo->bufLen--;

      ret = berDecodeLength(pInfo, &len);

      if (ret != 0 || len >= IPT_STAT_STRING_LEN)
         ret = BER_STATUS_TOO_BIG;
      else
      {
         memcpy(pStr, pInfo->pBuf, len);
         pStr[len] = 0;    /* Null terminate string */

         pInfo->pBuf += len;
         pInfo->bufLen -= len;
      }
   }
   
   return ret;
}

/*******************************************************************************
NAME:       berDecodeOidPart
ABSTRACT:   Decodes a BER Object identifier part. 
            One or more octets, bit0-6 = value, bit7 = more
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeOidPart(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 *pValue)       /* Pointer to OID part */
{
   UINT32 value = 0;
   UINT8 c;

   while (pInfo->bufLen > 0)
   {
      c = *pInfo->pBuf++;   /* Get next octet */
      pInfo->bufLen--;

      value = (value << 7) + (c & 0x7f);  /* Take bit0-6 as value */
      
      if ((c & 0x80) == 0)               /* Bit7 indicates if more octets */
      {
         *pValue = value;                 /* Part ready */
         return 0;
      }
   }

   return BER_STATUS_BAD_VALUE;           /* We ran out of message before IOD part was ready */
}

/*******************************************************************************
NAME:       berDecodeOid
ABSTRACT:   Decodes a BER Object identifier. The parts of the OID is put into an array of integers. 
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeOid(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 oid[])         /* Pointer to OID array of integers */
{
   UINT32 n = 0, len, value = 0;
   int ret;

   if (pInfo->bufLen < 1 ||
       pInfo->pBuf[0] != BER_OID)
   {
      return BER_STATUS_BAD_VALUE;
   }

   pInfo->pBuf++;
   pInfo->bufLen--;
   
   if ((ret = berDecodeLength(pInfo, &len)) != 0)
      return ret;
   
   len = pInfo->bufLen - len; /* Set to target bufLen when the complete OID is handled */

   /* First byte should be 0x2b, meaning .1.3 */
   if (*pInfo->pBuf != 0x2b)
      return BER_STATUS_BAD_VALUE;
   else
   {
      oid[n++] = 1;
      oid[n++] = 3;
      pInfo->pBuf++;
      pInfo->bufLen--;
   }

   while (pInfo->bufLen > len) 
   {
      if (n >= (IPT_STAT_OID_LEN - 1))
         return BER_STATUS_TOO_BIG;

      if ((ret = berDecodeOidPart(pInfo, &value)) != 0)
         return ret;

      oid[n++] = value;
   }

   oid[n] = IPT_STAT_OID_STOPPER;     /* Add stopper last */

   return 0;
}

/*******************************************************************************
NAME:       berDecodeIpAddress
ABSTRACT:   Decodes an IP address
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeIpAddress(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 *pVal)         /* Pointer to 4 byte IP address = UINT32 */
{
   int ret = 0;
   UINT32 i, value = 0;
   
   if (pInfo->bufLen < 6 || 
       pInfo->pBuf[0] != BER_IPADDRESS ||
       pInfo->pBuf[1] != 4)
   {
      ret = BER_STATUS_BAD_VALUE;
   }
   else
   {
      for (i = 0; i < 4; i++)
         value = (value << 8) + pInfo->pBuf[i + 2];

      *pVal = value;
      
      pInfo->pBuf += 6;
      pInfo->bufLen -= 6;
   }

   return ret;
}

/*******************************************************************************
NAME:       berDecodeUint32
ABSTRACT:   Decodes a BER uint32. The length byte tells how long it is. 
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeUint32(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 type,          /* Expected type of part */
              UINT32 *pVal)         /* Pointer to counter = unsigned 32 bit integer */
{
   int ret = 0;
   UINT32 i, octets;
   UINT32 value = 0;

   if (pInfo->bufLen < 2 ||
       pInfo->pBuf[0] != type ||
       pInfo->pBuf[1] < 1 ||
       pInfo->pBuf[1] > 4 ||
       pInfo->pBuf[1] > pInfo->bufLen)
   {
      ret = BER_STATUS_BAD_VALUE;
   }
   else
   {
      /* Get the rest of the bytes */
      octets = pInfo->pBuf[1];
      for (i = 0; i < octets; i++)
         value = (value << 8) + pInfo->pBuf[i + 2];

      *pVal = value;
      pInfo->pBuf += octets + 2;
      pInfo->bufLen -= octets + 2;
   }

   return ret;
}

/*******************************************************************************
NAME:       berDecodeData
ABSTRACT:   Decodes a BER data type
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berDecodeData(
                  BER_MSG_INFO *pInfo,     /* Pointer to message info structure */
                  IPT_STAT_DATA *pValue)   /* Pointer to data structure */
{
   int ret = 0;

   if (pInfo->bufLen < 1)
   {
      ret = BER_STATUS_BAD_VALUE;
   }
   else
   {
      switch (pInfo->pBuf[0])
      {
         case BER_INTEGER:
            pValue->type = IPT_STAT_TYPE_INTEGER;
            ret = berDecodeInteger(pInfo, &pValue->value.integer);
            break;

         case BER_OCTET_STRING:
            pValue->type = IPT_STAT_TYPE_OCTET_STRING;
            ret = berDecodeOctetString(pInfo, pValue->value.octetString);
            break;

         case BER_OID:
            pValue->type = IPT_STAT_TYPE_OID;
            ret = berDecodeOid(pInfo, pValue->oid);
            break;

         case BER_IPADDRESS:
            pValue->type = IPT_STAT_TYPE_IPADDRESS;
            ret = berDecodeIpAddress(pInfo, &pValue->value.ipAddress);
            break;

         case  BER_COUNTER:
            pValue->type = IPT_STAT_TYPE_COUNTER;
            ret = berDecodeUint32(pInfo, BER_COUNTER, &pValue->value.counter);
            break;

         case BER_TIMETICKS:
            pValue->type = IPT_STAT_TYPE_TIMETICKS;
            ret = berDecodeUint32(pInfo, BER_TIMETICKS, &pValue->value.timeTicks);
            break;

         default:
            ret = BER_STATUS_BAD_VALUE;
      }
   }

   return ret;
}

/*******************************************************************************
NAME:       berDecodeHeader
ABSTRACT:   Decode an incoming SNMP message header 
RETURNS:    -
*/
static int berDecodeHeader(
    BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
    UINT8 *pPduType,      /* Pointer to PDU type variable, BER_GET_REQUEST.. */
    INT32 *pRequestId)    /* Pointer to request Id variable */
{
    int i, ret = 0;
    BER_VALUE val;

    for (i = 1; i <= 7 && ret == 0; i++)
    {
        /*  CR-3477: Unnecessary double check removed    */

        switch(i)
        {
            case 1:
                /* Part 1: SEQUENCE */
                ret = berDecodeSequence(pInfo, &val.value.counter);
                break;

            case 2:
                /* Part 2: version (INTEGER, 1 byte, =0) */
                ret = berDecodeInteger(pInfo, &val.value.integer);
                if (ret == 0 && val.value.integer != 0)
                    ret = BER_STATUS_BAD_VALUE;
                break;

            case 3:
                /* Part 3: community (OCTET STRING, 6 bytes, "public") */
                ret = berDecodeOctetString(pInfo, val.value.octetString);
                if (ret == 0 && strcmp(val.value.octetString, "public") != 0)
                    ret = BER_STATUS_BAD_VALUE;
                break;

            case 4:
                /* Part 4: PDU type */
                ret = berDecodeConstructed(pInfo, pPduType, &val.value.counter);
                break;

            case 5:
                /* Part 5: request id (INTEGER, 4 bytes) */
                ret = berDecodeInteger(pInfo, pRequestId);
                break;

            case 6:
            case 7:
                /* Part 6: error status (INTEGER, 1 byte, 0 = noError) */
                ret = berDecodeInteger(pInfo, &val.value.integer);
                if (ret == 0 && val.value.integer != 0)
                    ret = BER_STATUS_BAD_VALUE;
                break;

            default:
                ret = BER_STATUS_BAD_VALUE;
                break;
        }
    }

    if (ret != 0)
        IPTVosPrint1(IPT_ERR, "SNMP Illegal syntax in received message header, part %d\n", i);

    return ret;
}

/* Encode functions */

/*******************************************************************************
NAME:       berEncodeLength
ABSTRACT:   Encodes a BER tag length. 
            If <128 it is the short form and it is the length itself.
            Otherwise it is the long form and bit0-7 indicates
            how many bytes that will follow that contains the length.
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeLength(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 length)        /* Length */
{
   int ret = 0;

   if (pInfo->bufLen < 1)
      ret = BER_STATUS_BAD_VALUE;
   else
   {
      if (length < 128)
      {
         pInfo->pBuf--;
         pInfo->bufLen--;
         *pInfo->pBuf = (UINT8) length;
      }
      else
      {
         /* Store length as if it was a Counter, including leading type */
         ret = berEncodeUint32(pInfo, 0, length);

         /* Back pointer and set length to long form */
         pInfo->pBuf++;
         pInfo->bufLen++;
         pInfo->pBuf[0] |= 0x80;
      }
   }
      
   return ret;
}

/*******************************************************************************
NAME:       berEncodeNull
ABSTRACT:   Encodes a BER NULL. 
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeNull(
              BER_MSG_INFO *pInfo)  /* Pointer to message info structure */
{
   int ret = 0;

   if (pInfo->bufLen < 2)
      ret = BER_STATUS_BAD_VALUE;
   else
   {
      pInfo->pBuf -= 2;
      pInfo->bufLen -= 2;
      pInfo->pBuf[0] = BER_NULL;
      pInfo->pBuf[1] = 0;
   }

   return ret;
}

/*******************************************************************************
NAME:       berEncodeSequence
ABSTRACT:   Encodes a BER SEQUENCE
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeSequence(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 length)        /* Length */
{
   int ret;
   
   if ((ret = berEncodeLength(pInfo, length)) == 0)
   {
      if (pInfo->bufLen < 2)
         ret = BER_STATUS_TOO_BIG;
      else
      {
         pInfo->bufLen--;
         pInfo->pBuf--;
         pInfo->pBuf[0] = BER_SEQUENCE;
      }
   }

   return ret;
}

/*******************************************************************************
NAME:       berEncodeInteger
ABSTRACT:   Encodes a BER integer including leading type and length bytes. 
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeInteger(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              INT32 value)          /* Value */
{
   int ret = 0;
   UINT32 i, octets, mask = 0xff000000;
   
   /* Skip leading bytes if possible */
   octets = 4;
   while (octets > 1)
   {
      if ((value & mask) == mask || (value & mask) == 0)
      {
         /* First byte and bit7 of next byte are all 1 or 0 */
         octets--;
         value = value << 8;
      }
      else
         break;
   }

   if ((octets + 2) > pInfo->bufLen)
      ret = BER_STATUS_TOO_BIG;
   else
   {
      pInfo->pBuf -= octets + 2;
      pInfo->bufLen -= octets + 2;
      pInfo->pBuf[0] = BER_INTEGER;
      pInfo->pBuf[1] = octets;

      for (i = 0; i < octets; i++)
      {
         pInfo->pBuf[i + 2] = (value & 0xff000000) >> 24;
         value <<= 8;
      }
   }

   return ret;
}

/*******************************************************************************
NAME:       berEncodeOctetString
ABSTRACT:   Encodes a BER Octet string including leading type and length bytes. 
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeOctetString(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              const char *pStr)     /* Pointer to zero-terminated string */
{
   int ret = 0;
   UINT32 len;

   len = strlen(pStr);
   
   if (len > IPT_STAT_STRING_LEN || len > pInfo->bufLen)
      ret = BER_STATUS_TOO_BIG;
   else
   {
      pInfo->pBuf -= len;
      pInfo->bufLen -= len;
      memcpy(pInfo->pBuf, pStr, len);

      if ((ret = berEncodeLength(pInfo, len)) == 0 && pInfo->bufLen > 1)
      {
         pInfo->pBuf--;
         pInfo->bufLen--;
         pInfo->pBuf[0] = BER_OCTET_STRING;
      }
   }

   return ret;
}

/*******************************************************************************
NAME:       berEncodeOidPart
ABSTRACT:   Encodes a BER Object identifier part. 
            One or more octets, bit0-6 = value, bit7 = more
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeOidPart(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 value)         /* OID part */
{
   UINT8 tmp, more = 0;

   do
   {
      if (pInfo->bufLen < 1)
         return BER_STATUS_TOO_BIG;
      
      pInfo->pBuf--;
      pInfo->bufLen--;
      
      tmp = (value & 0x7f) | more;  /* Take bit0-6 from value and set bit7 for more */
      pInfo->pBuf[0] = tmp;

      value = value >> 7;
      more = (value != 0)? 0x80 : 0;
      
   } while (more);

   return 0;
}

/*******************************************************************************
NAME:       berEncodeOid
ABSTRACT:   Encodes a BER Object identifier. The parts of the OID is taken from an array of integers. 
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeOid(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              const UINT32 oid[])   /* Pointer to OID array of integers */
{
   int ret;
   UINT32 i, len, n = 0;
   UINT8 *pSave;

   /* Get no of parts in the OID */
   while (oid[n] != IPT_STAT_OID_STOPPER)
      n++;

   /* Check that the OID starts with at least "1.3" */
   if (n < 2 || oid[0] != 1 || oid[1] != 3)
      return BER_STATUS_BAD_VALUE;

   /* Add the OID parts backwards */
   pSave = pInfo->pBuf;             /* Save pointer before loading */
   for (i = n - 1; i > 1; i--)
   {
      if ((ret = berEncodeOidPart(pInfo, oid[i])) != 0)
         return ret;
   }

   /* Set first byte to 0x2b, meaning .1.3 */
   pInfo->pBuf--;
   pInfo->bufLen--;
   pInfo->pBuf[0] = 0x2b;

   /* Add length of total OID in front of the OID parts */
   len = pSave - pInfo->pBuf;
   
   if ((ret = berEncodeLength(pInfo, len)) == 0 && pInfo->bufLen > 1)
   {
      pInfo->pBuf--;
      pInfo->bufLen--;
      pInfo->pBuf[0] = BER_OID;
   }

   return ret;
}

/*******************************************************************************
NAME:       berEncodeIpAddress
ABSTRACT:   Encodes an IP address. Format UINT32, always with 4 octets.
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeIpAddress(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 value)         /* Pointer to 4 byte IP address = UINT32 */
{
   UINT32 i;

   if (pInfo->bufLen < 6)
      return BER_STATUS_TOO_BIG;

   pInfo->pBuf -= 6;
   pInfo->bufLen -= 6;

   pInfo->pBuf[0] = BER_IPADDRESS;
   pInfo->pBuf[1] = 4;
   
   for (i = 0; i < 4; i++)
   {
      pInfo->pBuf[i + 2] = (value & 0xff000000) >> 24;
      value <<= 8;
   }


   return 0;
}

/*******************************************************************************
NAME:       berEncodeUint32
ABSTRACT:   Encodes a BER Uint32 including leading type and length bytes. 
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeUint32(
              BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
              UINT32 type,          /* BER_COUNTER, _GUAGE, _TIMETICKS */
              UINT32 value)         /* Value */
{
   int ret = 0;
   UINT32 i, octets; 
   UINT32 mask = 0xff800000;
   
   /* Skip leading bytes if possible */
   octets = 4;
   while (octets > 1)
   {
      if ((value & mask) == 0)
      {
         /* First byte and bit7 of next byte are all 0 */
         octets--;
         value = value << 8;
      }
      else
         break;
   }

   if ((octets + 2) > pInfo->bufLen)
      ret = BER_STATUS_TOO_BIG;
   else
   {
      pInfo->pBuf -= octets + 2;
      pInfo->bufLen -= octets + 2;
      pInfo->pBuf[0] = type;
      pInfo->pBuf[1] = octets;

      for (i = 0; i < octets; i++)
      {
         pInfo->pBuf[i + 2] = (value & 0xff000000) >> 24;
         value <<= 8;
      }
   }

   return ret;
}

/*******************************************************************************
NAME:       berEncodeConstructed
ABSTRACT:   Encodes a BER Constructed type, e.g. PDU. 
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeConstructed(
                    BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
                    UINT32 type,          /* BER_GET_RESPONSE... */
                    UINT32 length)        /* Length */
{
   int ret = 0;

   ret = berEncodeLength(pInfo, length);
   if (ret == 0)
   {
      pInfo->pBuf--;
      pInfo->bufLen--;
      pInfo->pBuf[0] = type;
   }

   return ret;
}

/*******************************************************************************
NAME:       berEncodeData
ABSTRACT:   Encodes a BER data type
            Note! Buffer filled backwards.
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeData(
                  BER_MSG_INFO *pInfo,          /* Pointer to message info structure */
                  const IPT_STAT_DATA *pValue)  /* Pointer to data structure */
{
   int ret = 0;

   switch (pValue->type)
   {
   case IPT_STAT_TYPE_INTEGER:
      ret = berEncodeInteger(pInfo, pValue->value.integer);
      break;

   case IPT_STAT_TYPE_OCTET_STRING:
      ret = berEncodeOctetString(pInfo, pValue->value.octetString);
      break;

   case IPT_STAT_TYPE_OID:
      ret = berEncodeOid(pInfo, pValue->oid);
      break;

   case IPT_STAT_TYPE_IPADDRESS:
      ret = berEncodeIpAddress(pInfo, pValue->value.ipAddress);
      break;

   case  IPT_STAT_TYPE_COUNTER:
      ret = berEncodeUint32(pInfo, BER_COUNTER, pValue->value.counter);
      break;

   case IPT_STAT_TYPE_TIMETICKS:
      ret = berEncodeUint32(pInfo, BER_TIMETICKS, pValue->value.timeTicks);
      break;

   default:
      break;
   }

   return ret;
}

/*******************************************************************************
NAME:       berEncodeHeader
ABSTRACT:   Decode an incoming SNMP message header 
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int berEncodeHeader(
                    BER_MSG_INFO *pInfo,  /* Pointer to message info structure */
                    UINT8 pduType,        /* PDU type variable */
                    INT32 requestId,      /* Request Id variable */
                    UINT32 errorStatus,   /* Status for task */       
                    UINT32 errorIndex)    /* Index to variable that failed, 0 if none */
{
   int i, ret = 0;

   /* Fill header backwards */
   for (i = 7; i > 0 && ret == 0; i--)
   {
      switch(i)
      {
      case 1:
         /* Part 1: SEQUENCE */
         ret = berEncodeSequence(pInfo, pInfo->pEnd - pInfo->pBuf);
         break;
         
      case 2:
         /* Part 2: version (INTEGER, 1 byte, =0) */
         ret = berEncodeInteger(pInfo, 0);
         break;
         
      case 3:
         /* Part 3: community (OCTET STRING, 6 bytes, "public") */
         ret = berEncodeOctetString(pInfo, "public");
         break;
         
      case 4:
         /* Part 4: PDU type */
         ret = berEncodeConstructed(pInfo, pduType, pInfo->pEnd - pInfo->pBuf);
         break;
         
      case 5:
         /* Part 5: request id (INTEGER, 4 bytes) */
         ret = berEncodeInteger(pInfo, requestId);
         break;
         
      case 6:
         /* Part 6: error status (INTEGER, 1 byte, 0 = noError) */
         ret = berEncodeInteger(pInfo, (int) errorStatus);
         break;
         
      case 7:
         /* Part 7: error index (INTEGER, 1 byte) */
         ret = berEncodeInteger(pInfo, (int) errorIndex);
         break;
         
      default:
         ret = BER_STATUS_BAD_VALUE;
         break;
      }
   }

   if (ret != 0)
      IPTVosPrint1(IPT_ERR, "SNMP Problem encoding response header, part %d\n", i + 1);

   return ret;
}

/*******************************************************************************
NAME:       stripOid
ABSTRACT:   Checks and strips leading part of an OID. 
            (OID created by other functions that guarantees correct ending.)
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int stripOid(
             UINT32 *pOid)    /* Pointer to OID array, ending with stopper */
{
   UINT32 oidStart[] = IPT_START_OID;
   UINT32 n, startLen;

   startLen = sizeof(oidStart) / sizeof(UINT32);

   /* Count number of OID parts */
   n = 0;
   while (pOid[n] != IPT_STAT_OID_STOPPER)
      n++;

   if (n < startLen || memcmp(oidStart, pOid, startLen * sizeof(UINT32)) != 0)
      return BER_STATUS_NO_SUCH_NAME;

   memmove(pOid, &pOid[startLen], ((n - startLen) + 1) * sizeof(UINT32));

   return 0;
}

/*******************************************************************************
NAME:       unstripOid
ABSTRACT:   Resets leading part of an OID 
RETURNS:    
*/
static void unstripOid(
             UINT32 *pOid)    /* Pointer to OID array, ending with stopper */
{
   UINT32 oidStart[] = IPT_START_OID;
   UINT32 n, startLen;

   startLen = sizeof(oidStart) / sizeof(UINT32);

   /* Count number of OID parts */
   n = 0;
   while (pOid[n] != IPT_STAT_OID_STOPPER) 
      n++;

   memmove(&pOid[startLen], pOid, (n + 1) * sizeof(UINT32));
   memmove(pOid, oidStart, startLen * sizeof(UINT32));
}

/*******************************************************************************
NAME:       snmpGetRequest
ABSTRACT:   Decode an incoming SNMP Get/GetNext request message and perform requested task
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int snmpGetRequest(
                   UINT8 pduType,            /* PDU type */ 
                   BER_MSG_INFO *pReqInfo,   /* Pointer to request message info structure */
                   BER_MSG_INFO *pRespInfo,  /* Pointer to response message info structure */
                   UINT32 *pErrorIndex       /* If error: index to which variable, 1.. */
#ifdef TARGET_SIMU
                 , UINT32 simDevIp
#endif
               )
{
   int i, ret = 0, nVar = 0;
   UINT32 length;
   /* GW 2012-04-16, CR-3477. Initalize Vars */
   IPT_STAT_DATA *pStatData[BER_MAX_NVAR] = {0};
   int retCode[BER_MAX_NVAR] = {0};
   UINT8 *pEnd;

#ifdef TARGET_SIMU
   IPT_UNUSED (pduType)
   IPT_UNUSED (simDevIp)
#endif

   *pErrorIndex = 0;
   
   /* Parse variable list */
   if ((ret = berDecodeSequence(pReqInfo, &length)) == 0)
   {
      while (ret == 0 && pReqInfo->pBuf < pReqInfo->pEnd && nVar < BER_MAX_NVAR)
      {
         /* Variable */
         if ((ret = berDecodeSequence(pReqInfo, &length)) == 0)
         {
            /* Allocate a data access structure for this variable */
            pStatData[nVar] = (IPT_STAT_DATA *) IPTVosMalloc(sizeof(IPT_STAT_DATA));
            if (pStatData[nVar] == NULL)
               return BER_STATUS_TOO_BIG;
            
            /* Get variable OID */
            if ((ret = berDecodeOid(pReqInfo, pStatData[nVar]->oid)) == 0)
            {
               /* NULL */
               if ((ret = berDecodeNull(pReqInfo)) == 0)
               {
                  /* Get statistic data (after stripping leading fixed part of OID */
                  if ((ret = stripOid(pStatData[nVar]->oid)) == 0)
                  {
#ifdef TARGET_SIMU
                     retCode[nVar] = IPT_NOT_FOUND;
#else
                     if (pduType == BER_GET_REQUEST)
                        retCode[nVar] = iptStatGet(pStatData[nVar]);
                     else
                        retCode[nVar]= iptStatGetNext(pStatData[nVar]);
#endif                     

                     if (retCode[nVar] != 0)
                     {
                        *pErrorIndex = (UINT32) (nVar + 1);
                        IPTVosPrint1(IPT_ERR, "SNMP Could not get statistic data 0x%x\n", ret);
                        ret = BER_STATUS_NO_SUCH_NAME;
                     }
                  }
               }
            }
            
            nVar++;
         }
      }

      /* If there were problems, free all allocated memory and interrupt */
      if (ret != 0)
      {
         if (*pErrorIndex == 0)
            *pErrorIndex = (UINT32) nVar;    /* Mark as error */

         for (i = 0; i < nVar; i++)
            if (pStatData[i] != NULL)
               (void) IPTVosFree((unsigned char *) pStatData[i]);
      }
      else
      {
         /* Create a complete response backwards with all variables */
         pEnd = pRespInfo->pEnd;
         for (i = nVar - 1; i >= 0; i--)
         {
         
            /* Fill response message with variable data (or NULL) and OID */
            if (retCode[i] == 0)
               (void) berEncodeData(pRespInfo, pStatData[i]);
            else
               (void) berEncodeNull(pRespInfo);
         
            unstripOid(pStatData[i]->oid);  /* Reset leading part of OID */
            (void) berEncodeOid(pRespInfo, pStatData[i]->oid); 
            (void) berEncodeSequence(pRespInfo, (UINT32) (pEnd - pRespInfo->pBuf));
            pEnd = pRespInfo->pBuf;
            (void) IPTVosFree((BYTE *) pStatData[i]);
         }
      
         (void) berEncodeSequence(pRespInfo, pRespInfo->pEnd - pRespInfo->pBuf);
      }
   }

   if (*pErrorIndex != 0)
      return BER_STATUS_NO_SUCH_NAME;
   else
      return 0;
}

/*******************************************************************************
NAME:       snmpSetRequest
ABSTRACT:   Decode an incoming SNMP Set request message and perform requested task
RETURNS:    0 if OK, BER_STATUS_* if not
*/
static int snmpSetRequest(
                   BER_MSG_INFO *pReqInfo,   /* Pointer to request message info structure */
                   BER_MSG_INFO *pRespInfo,  /* Pointer to response message info structure */
                   UINT32 *pErrorIndex)      /* If error: index to which variable, 1.. */
{
   int ret = 0;
   UINT32 length;
   IPT_STAT_DATA statData;

   *pErrorIndex = 0;
   
   /* Variable list */
   if ((ret = berDecodeSequence(pReqInfo, &length)) == 0)
   {
      /* Variable */
      if ((ret = berDecodeSequence(pReqInfo, &length)) == 0)
      {
         /* OID */
         if ((ret = berDecodeOid(pReqInfo, statData.oid)) == 0)
         {
            /* Data */
            if ((ret = berDecodeData(pReqInfo, &statData)) == 0)
            {
               /* Set statistic data (after stripping leading fixed part of OID */
               if ((ret = stripOid(statData.oid)) == 0)
               {
#ifdef TARGET_SIMU
                  ret = IPT_NOT_FOUND;
#else
                  ret = iptStatSet(&statData);
#endif
                  if (ret != 0)
                  {
                     *pErrorIndex = 1;
                     IPTVosPrint1(IPT_ERR, "SNMP Could not set statistic data 0x%x\n", ret);
                  }
               }
            }
            
            /* Fill response message with NULL and OID */
            (void) berEncodeNull(pRespInfo);
            
            unstripOid(statData.oid);  /* Reset leading part of OID */
            (void) berEncodeOid(pRespInfo, statData.oid); 
         }
     
         (void) berEncodeSequence(pRespInfo, pRespInfo->pEnd - pRespInfo->pBuf);
      }
      
      (void) berEncodeSequence(pRespInfo, pRespInfo->pEnd - pRespInfo->pBuf);
   }

   if (*pErrorIndex != 0)
      return BER_STATUS_NO_SUCH_NAME;
   else
      return 0;
}

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       iptSnmpInMessage
ABSTRACT:   Decode an incoming SNMP message and perform correct actions.
RETURNS:    -
*/
void iptSnmpInMessage(
                    UINT32 srcIPaddr,  /* Source IP address */
                    UINT16 srcPort,    /* Source port no */
#ifdef TARGET_SIMU
                    UINT32 destIPaddr, /* Destination IP address */
#endif
                    UINT8 *pBuf,       /* Pointer to message */
                    UINT32 bufLen)     /* Length of message */
{
   int ret = 0;
   UINT32 destAddr;
   UINT16 destPort;
   UINT8 pduType = 0;
   INT32 requestId;
   UINT32 errorIndex = 0;
   BER_MSG_INFO reqInfo, respInfo;
   UINT8 respBuf[BER_MAX_bufLen];    /* Buffer for response message */

   /* Set up request msg info */
   reqInfo.pBuf = pBuf;
   reqInfo.bufLen = bufLen;
   reqInfo.pBegin = pBuf;
   reqInfo.pEnd = pBuf + bufLen;

   /* Set up response msg info. Buffer is filled backwards */
   respInfo.pBuf = &respBuf[sizeof(respBuf)];   
   respInfo.bufLen = sizeof(respBuf);
   respInfo.pBegin = respBuf;
   respInfo.pEnd = respInfo.pBuf;
   memset(respBuf, 0, sizeof(respBuf));
   destAddr = FROMWIRE32(srcIPaddr);
   destPort = FROMWIRE16(srcPort);

   /* Decode message parts, check that right things comes in right order */
   if ((ret = berDecodeHeader(&reqInfo, &pduType, &requestId)) != 0)
   {
      /* Return error response */
      (void) berEncodeSequence(&respInfo, 0);
      (void) berEncodeHeader(&respInfo, BER_GET_RESPONSE, requestId, (UINT32) ret, 0);
   }
   else
   {
      /* Perform task given in the request */
      if (pduType == BER_GET_REQUEST || pduType == BER_GET_NEXT_REQUEST)
#ifdef TARGET_SIMU
         ret = snmpGetRequest(pduType, &reqInfo, &respInfo, &errorIndex, destIPaddr);
#else
         ret = snmpGetRequest(pduType, &reqInfo, &respInfo, &errorIndex);
#endif
      else if (pduType == BER_SET_REQUEST)
         ret = snmpSetRequest(&reqInfo, &respInfo, &errorIndex);
   }

   /* Add header */
   if ((ret = berEncodeHeader(&respInfo, BER_GET_RESPONSE, requestId, (UINT32) ret, errorIndex)) == 0)
   {
      /* Send response message */
#ifdef TARGET_SIMU
      (void) IPTDriveSNMPSend(TOWIRE32(destAddr), destPort, destIPaddr, respInfo.pBuf, (int) (sizeof(respBuf) - respInfo.bufLen));
#else
      (void) IPTDriveSNMPSend(TOWIRE32(destAddr), (UINT16) TOWIRE16(destPort), respInfo.pBuf, (int) (sizeof(respBuf) - respInfo.bufLen));
#endif
   }
}

