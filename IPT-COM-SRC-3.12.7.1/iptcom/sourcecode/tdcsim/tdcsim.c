/*******************************************************************************
*  COPYRIGHT   : (C) 2006-2010 Bombardier Transportation
********************************************************************************
*  PROJECT     : IPTrain
*
*  MODULE      : tdcsim.c
*
*  ABSTRACT    : TDC simulator
*
********************************************************************************
*  HISTORY     :
*	
* $Id: tdcsim.c 11646 2010-08-20 15:25:59Z bloehr $
*
*  Internal (Bernd Loehr, 2010-08-13) 
* 			Old obsolete CVS history removed
*
*
*
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(VXWORKS)
#elif defined(__INTEGRITY)
#else
#include <memory.h>
#endif

#include "ipt.h"	         /* Common type definitions for IPT  */
#include "iptDef.h"        /* IPT definitions */
#include "iptcom.h"
#include "iptcom_priv.h"

/*******************************************************************************
*  INCLUDES */

/*******************************************************************************
*  DEFINES
*/

/*******************************************************************************
*  GLOBALS */

/*******************************************************************************
* TYPEDEFS */


/*******************************************************************************
NAME:       
ABSTRACT:   
RETURNS:    
*/
int tdcSimCompareIPAddr( const void *arg1, const void *arg2 )
{
   TDCSIM_IP_TO_URI_ITEM *p1;
   TDCSIM_IP_TO_URI_ITEM *p2;

   p1 = (TDCSIM_IP_TO_URI_ITEM *)arg1;
   p2 = (TDCSIM_IP_TO_URI_ITEM *)arg2;

   if (p1->IPaddr < p2->IPaddr)
      return -1;

   if (p1->IPaddr > p2->IPaddr)
      return 1;

   return 0;  
}

/*******************************************************************************
NAME:       
ABSTRACT:   
RETURNS:    
*/
int tdcSimCompareURI( const void *arg1, const void *arg2 )
{
   return iptStrcmp(((TDCSIM_IP_TO_URI_ITEM *)arg1)->pURI, ((TDCSIM_IP_TO_URI_ITEM *)arg2)->pURI);
}

/*******************************************************************************
NAME:       tdcPrepareInit
ABSTRACT:   Initialize tdc component
RETURNS:    -
*/
int tdcSimInit (char *filePath)
{
   FILE *stream;
   UINT32 b1;
   UINT32 b2; 
   UINT32 b3; 
   UINT32 b4;
   char line[TDCSIM_MAX_LINE_LEN];
   char URI[TDCSIM_MAX_LINE_LEN];
   char frg[TDCSIM_MAX_LINE_LEN];
   int urilen;
   int noOfItems;
   
   /* Clear cross ref array */
   memset(IPTGLOBAL(tdcsim.tdcSimIPToURI), 0, TDCSIM_MAX_NO_OF_HOSTS * sizeof (TDCSIM_IP_TO_URI_ITEM));
   memset(IPTGLOBAL(tdcsim.tdcSimURIToIP), 0, TDCSIM_MAX_NO_OF_HOSTS * sizeof (TDCSIM_IP_TO_URI_ITEM));
   IPTGLOBAL(tdcsim.tdcSimNoOfHosts) = 0;
   IPTGLOBAL(tdcsim.URIBuffer)[0] = 0;
   IPTGLOBAL(tdcsim.uriBufCurPos) = IPTGLOBAL(tdcsim.URIBuffer);
   
   /* Try to open tdchosts */
   if( (stream = fopen( filePath, "r" )) != NULL )
   {
      /* While not end of file */
      while (!feof(stream))/*lint !e611 Lint for VxWorks gets lost in macro defintions */   
	   {      
         /* Get a line from the file */
         if( fgets( line, TDCSIM_MAX_LINE_LEN, stream ) != NULL)
         {     
            frg[0] = 0;
            /* Try to get IP and URI */
            noOfItems = sscanf(line, "%d.%d.%d.%d %s %s", &b1, &b2, &b3, &b4, URI, frg);
            if ((noOfItems == 5) || (noOfItems == 6))
            {
               /* Success */               
               URI[TDCSIM_MAX_LINE_LEN - 1] = 0;
               urilen = strlen(URI);
               frg[TDCSIM_MAX_LINE_LEN - 1] = 0;
               
               /* Add IP address to cross reference arrays */
               IPTGLOBAL(tdcsim.tdcSimIPToURI)[IPTGLOBAL(tdcsim.tdcSimNoOfHosts)].IPaddr = ((b1 << 24) + (b2 << 16) + (b3 << 8) + b4);
               IPTGLOBAL(tdcsim.tdcSimURIToIP)[IPTGLOBAL(tdcsim.tdcSimNoOfHosts)].IPaddr = IPTGLOBAL(tdcsim.tdcSimIPToURI)[IPTGLOBAL(tdcsim.tdcSimNoOfHosts)].IPaddr;
               
               /* Copy URI to symboltable */
               memcpy(IPTGLOBAL(tdcsim.uriBufCurPos), URI, urilen);
               IPTGLOBAL(tdcsim.uriBufCurPos)[urilen] = 0;
               
               /* Add reference to URI in crossref table */
               IPTGLOBAL(tdcsim.tdcSimIPToURI)[IPTGLOBAL(tdcsim.tdcSimNoOfHosts)].pURI = IPTGLOBAL(tdcsim.uriBufCurPos);
               IPTGLOBAL(tdcsim.tdcSimURIToIP)[IPTGLOBAL(tdcsim.tdcSimNoOfHosts)].pURI = IPTGLOBAL(tdcsim.uriBufCurPos);
               
               if ((noOfItems == 6) && (iptStrcmp("FRG",frg) == 0))
               {
                  /* Set the address as a  FRG address */
                  IPTGLOBAL(tdcsim.tdcSimIPToURI)[IPTGLOBAL(tdcsim.tdcSimNoOfHosts)].is_Frg = 1;
                  IPTGLOBAL(tdcsim.tdcSimURIToIP)[IPTGLOBAL(tdcsim.tdcSimNoOfHosts)].is_Frg = 1;
               }
               else
               {
                  /* Set the address as a non FRG address */
                  IPTGLOBAL(tdcsim.tdcSimIPToURI)[IPTGLOBAL(tdcsim.tdcSimNoOfHosts)].is_Frg = 0;
                  IPTGLOBAL(tdcsim.tdcSimURIToIP)[IPTGLOBAL(tdcsim.tdcSimNoOfHosts)].is_Frg = 0;
               }
               
               IPTGLOBAL(tdcsim.uriBufCurPos) += urilen + 1;

               IPTGLOBAL(tdcsim.tdcSimNoOfHosts)++;
               if (IPTGLOBAL(tdcsim.tdcSimNoOfHosts) >= TDCSIM_MAX_NO_OF_HOSTS)
                  break;

               /* Enough room left in symboltable ? */
               if ((IPTGLOBAL(tdcsim.uriBufCurPos) - IPTGLOBAL(tdcsim.URIBuffer)) > (TDCSIM_SIZEOF_URI_BUF - TDCSIM_MAX_LINE_LEN))
                  break;
            }
         }
      }

      /* Sort IP to URI cross ref. array */
      qsort(IPTGLOBAL(tdcsim.tdcSimIPToURI), IPTGLOBAL(tdcsim.tdcSimNoOfHosts), sizeof(TDCSIM_IP_TO_URI_ITEM), tdcSimCompareIPAddr);
      
      /* Sort URI to IP cross ref. array */
      qsort(IPTGLOBAL(tdcsim.tdcSimURIToIP), IPTGLOBAL(tdcsim.tdcSimNoOfHosts), sizeof(TDCSIM_IP_TO_URI_ITEM), tdcSimCompareURI);

      fclose( stream );
   }
   else
   {
      IPTVosPrint1(IPT_ERR,"Open TDC emulate file %s failed\n",filePath);
   }
   
   return TDC_OK;
}

/*******************************************************************************
NAME:       tdcSimGetAddrByName
ABSTRACT:   Get IP address from an URI
Returns the string converted to UINT32.
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT tdcSimGetAddrByName(
                              const T_IPT_URI uri,       /* input  */
                              T_IPT_IP_ADDR   *pIpAddr,    /* output */
                              UINT8           *pTopoCnt)                              
{
   TDCSIM_IP_TO_URI_ITEM searchItem;
   TDCSIM_IP_TO_URI_ITEM *pFoundItem;
   const char *pUriHost;
   int byte3, byte2, byte1, byte0;

   if ((uri == NULL) || (pIpAddr == NULL) || (pTopoCnt == NULL))
   {
      return TDC_NULL_POINTER_ERROR;
   }

   *pIpAddr = 0;

   /* Check the topo counter value if the value in the call != 0.
      In simulation mode current value is always 1 */
   if ((*pTopoCnt != 0) && (*pTopoCnt != 1))
   {
      return(TDC_WRONG_TOPOCOUNT);
   }
   *pTopoCnt = 1;

   pUriHost = strchr(uri,'@');   /*lint !e605 We trust strchr not change uri */
   if (pUriHost)
   {
      pUriHost += 1;
   }
   else
   {
      pUriHost = (char *)uri;
   }

   /* If already an IP address a.b.c.d then just return it */
   if (sscanf(pUriHost, "%d.%d.%d.%d", &byte3, &byte2, &byte1, &byte0) == 4)
   {
      if (((byte3 >= 0) && (byte3 < 256)) &&
          ((byte2 >= 0) && (byte2 < 256)) &&
          ((byte1 >= 0) && (byte1 < 256)) &&
          ((byte0 >= 0) && (byte0 < 256)))
      {
         *pIpAddr = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + (byte0);
         return TDC_OK;
      }
   }

   /* Setup search key */
   searchItem.IPaddr = 0;
   searchItem.pURI = pUriHost;
   
   /* Try to get item from cross ref array */
   pFoundItem = bsearch(
      &searchItem, 
      IPTGLOBAL(tdcsim.tdcSimURIToIP), 
      IPTGLOBAL(tdcsim.tdcSimNoOfHosts), 
      sizeof(TDCSIM_IP_TO_URI_ITEM), 
      tdcSimCompareURI);/*lint !e64 Assignment is OK */
   
   /* Success ? */
   if (pFoundItem != NULL)
   {
      *pIpAddr = pFoundItem->IPaddr;
      return TDC_OK;
   }
   else
      return TDC_ERROR;
}

/*******************************************************************************
NAME:       tdcSimGetAddrByNameExt
ABSTRACT:   Get IP address from an URI
Returns the string converted to UINT32.
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT tdcSimGetAddrByNameExt(
                              const T_IPT_URI uri,       /* input  */
                              T_IPT_IP_ADDR   *pIpAddr,    /* output */
                              T_TDC_BOOL      *pFrg,       
                              UINT8           *pTopoCnt)                              
{
   TDCSIM_IP_TO_URI_ITEM searchItem;
   TDCSIM_IP_TO_URI_ITEM *pFoundItem;
   const char *pUriHost;
   int byte3, byte2, byte1, byte0;

   if ((uri == NULL) || (pIpAddr == NULL) || (pTopoCnt == NULL) || (pFrg == NULL))
   {
      return TDC_NULL_POINTER_ERROR;
   }
   
   *pIpAddr = 0;

   /* Check the topo counter value if the value in the call != 0.
      In simulation mode current value is always 1 */
   if ((*pTopoCnt != 0) && (*pTopoCnt != 1))
   {
      return(TDC_WRONG_TOPOCOUNT);
   }
   *pTopoCnt = 1;

   pUriHost = strchr(uri,'@');   /*lint !e605 We trust strchr not change uri */
   if (pUriHost)
   {
      pUriHost += 1;
   }
   else
   {
      pUriHost = (char *)uri;
   }

   /* If already an IP address a.b.c.d then just return it */
   if (sscanf(pUriHost, "%d.%d.%d.%d", &byte3, &byte2, &byte1, &byte0) == 4)
   {
      if (((byte3 >= 0) && (byte3 < 256)) &&
          ((byte2 >= 0) && (byte2 < 256)) &&
          ((byte1 >= 0) && (byte1 < 256)) &&
          ((byte0 >= 0) && (byte0 < 256)))
      {
         *pIpAddr = (byte3 << 24) + (byte2 << 16) + (byte1 << 8) + (byte0);
         *pFrg = 0;
         return TDC_OK;
      }
   }

   /* Setup search key */
   searchItem.IPaddr = 0;
   searchItem.pURI = pUriHost;
   
   /* Try to get item from cross ref array */
   pFoundItem = bsearch(
      &searchItem, 
      IPTGLOBAL(tdcsim.tdcSimURIToIP), 
      IPTGLOBAL(tdcsim.tdcSimNoOfHosts), 
      sizeof(TDCSIM_IP_TO_URI_ITEM), 
      tdcSimCompareURI);/*lint !e64 Assignment is OK */
   
   /* Success ? */
   if (pFoundItem != NULL)
   {
      *pIpAddr = pFoundItem->IPaddr;
      *pFrg = pFoundItem->is_Frg;
      return TDC_OK;
   }
   else
      return TDC_ERROR;
}

/*******************************************************************************
NAME:       tdcSimGetUriHostPart
ABSTRACT:   Get URI from an IP address 
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT tdcSimGetUriHostPart (
                                T_IPT_IP_ADDR   ipAddr,  /* input  */
                                T_IPT_URI       uri,     /* output */
                                UINT8          *pTopoCnt)       
{
   TDCSIM_IP_TO_URI_ITEM searchItem;
   TDCSIM_IP_TO_URI_ITEM *pFoundItem;
   
   if ((uri == NULL)  || (pTopoCnt == NULL))
   {
      return TDC_NULL_POINTER_ERROR;
   }
   /* Check the topo counter value if the value in the call != 0.
      In simulation mode current value is always 1 */
   if ((*pTopoCnt != 0) && (*pTopoCnt != 1))
   {
      return(TDC_WRONG_TOPOCOUNT);
   }
   *pTopoCnt = 1;


   /* Setup search key */
   searchItem.IPaddr = ipAddr;   

   /* Try to get item from cross ref array */
   pFoundItem = bsearch(&searchItem, IPTGLOBAL(tdcsim.tdcSimIPToURI), 
      IPTGLOBAL(tdcsim.tdcSimNoOfHosts), sizeof(TDCSIM_IP_TO_URI_ITEM), tdcSimCompareIPAddr);/*lint !e64 Assignment is OK */

   /* Success ? */
   if (pFoundItem != NULL)
   {
      if (strlen(pFoundItem->pURI) < IPT_MAX_URI_LEN)
      {
         strcpy(uri, pFoundItem->pURI);
         return TDC_OK;
      }
      else
      {
         return TDC_ERROR;
      }
   }
   else
      return TDC_ERROR;
}


/*******************************************************************************
NAME:       tdcSimDestroy
ABSTRACT:   Clean up memory and other resources
RETURNS:    -
*/
void tdcSimDestroy ()
{
   /* Clear cross ref array */
   memset(IPTGLOBAL(tdcsim.tdcSimIPToURI), 0, TDCSIM_MAX_NO_OF_HOSTS * sizeof (TDCSIM_IP_TO_URI_ITEM));
   memset(IPTGLOBAL(tdcsim.tdcSimURIToIP), 0, TDCSIM_MAX_NO_OF_HOSTS * sizeof (TDCSIM_IP_TO_URI_ITEM));
   IPTGLOBAL(tdcsim.tdcSimNoOfHosts) = 0;
   IPTGLOBAL(tdcsim.URIBuffer)[0] = 0;
   IPTGLOBAL(tdcsim.uriBufCurPos) = IPTGLOBAL(tdcsim.URIBuffer);

   /* Reset flag */
   IPTGLOBAL(tdcsim.enableTDCSimulation) = 0;
}

/*******************************************************************************
NAME:       iptSimGetOwnIds
ABSTRACT:   Get own device, car and consist URI labels
RETURNS:    0 if OK, <>0 if not.
*/
T_TDC_RESULT tdcSimGetOwnIds(
                          T_IPT_LABEL devId, /* output */ 
                          T_IPT_LABEL carId, /* output */
                          T_IPT_LABEL cstId) /* output */

{
  T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

/* TODO get own from own ip address and return as TDC */  
   if (devId != NULL)
   {
      devId[0] = '\0';

      if (carId != NULL)
      {
         carId[0] = '\0';

         if (cstId != NULL)
         {
            (void) strcpy (cstId, "ownCst");
            (void) strcpy (carId, "ownCar");
            (void) strcpy (devId, "ownDev");
            tdcResult = TDC_OK;
         }
      }
      else
      {
         if (cstId != NULL)
         {
            cstId[0] = '\0';
         }
      }
   }
   else
   {
      if (carId != NULL)
      {
         carId[0] = '\0';
      }
      if (cstId != NULL)
      {
         cstId[0] = '\0';
      }

   }
   
   return(tdcResult);
}
