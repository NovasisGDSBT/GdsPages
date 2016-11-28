/*******************************************************************************
 *  COPYRIGHT   :  (C) 2006-2012 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     :  IPTrain
 *
 *  MODULE      :  pdcom.c
 *
 *  ABSTRACT    :  Public C methods for PD communication classes:
 *                 - PDCom
 *                 - PDComAPI
 *                 - PDComAPI
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 * $Id: pdcom.c 11859 2012-04-18 16:01:04Z gweiss $
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *           Findings from TUEV-Assessment
 *
 *  CR-2959 (Gerhard Weiss, Bernd Loehr, 2011-05-26)
 *          Fixed sending of unmarshalled PDs,
 *          Added log entry for unsent comIds
 *
 *  CR-1356 (Bernd Loehr, 2010-10-08)
 * 			Additional function iptConfigAddDatasetExt to define dataset
 *          dependent un/marshalling. Parameters for iptMarshallDSF changed.
 *
 *  Internal (Bernd Loehr, 2010-08-16) 
 * 			Old obsolete CVS history removed
 *
 *  CR-695 (Gerhard Weiss, 2010-06-02)
 *  			Corrected memory leaks found during release
 * 
 *
 ******************************************************************************/


/*******************************************************************************
*  INCLUDES */
#if defined(VXWORKS)
#include <vxWorks.h>
#endif

#include <string.h>
#include "iptcom.h"	/* Common type definitions for IPT */
#include "vos.h"		/* OS independent system calls */
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"
#include "netdriver.h"

/*******************************************************************************
*  DEFINES
*/

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  GLOBALS
*/

/*******************************************************************************
*  LOCALS
*/

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
* PDComAPI class methods
*******************************************************************************/

/*******************************************************************************
NAME:       PDComAPI_sub
ABSTRACT:   Subscribe for comid data
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_sub(
   UINT32 schedGrp,     /* schedule group */
   UINT32 comId,        /* comId */
   UINT32 filterId,    /* Source URI filter ID */
   const char *pSource, /* Pointer to string with source URI. 
                           Will override information in the configuration 
                           database. 
                           Set NULL if not used. */
   UINT32 destId,         /* Destination URI Id */
   const char *pDest)     /* Pointer to string with destination URI. 
                             Will override information in the configuration database. 
                             Set NULL if not used. */

{
   PD_SUB_CB *pCB = NULL;
   
   if (schedGrp == 0)
   {
      IPTVosPrint1(IPT_ERR,
                   "Subscription failed for ComId=%d. Illegal input parameters. schedGrp=0\n",
                   comId);
      return (PD_HANDLE) NULL;
   }
   else if (comId == 0)
   {
      IPTVosPrint0(IPT_ERR, "Subscription failed. Illegal input parameters. comId=0\n");
      return (PD_HANDLE) NULL;
   }
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER) == IPT_OK)
   {   
#ifdef TARGET_SIMU
   	pCB = pdSubComidCB_get(schedGrp, comId, filterId, pSource, destId, pDest, NULL);
#else
	   pCB = pdSubComidCB_get(schedGrp, comId, filterId, pSource, destId, pDest);
#endif

      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_sub: IPTVosPutSem(recSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_sub: IPTVosGetSem(recSem) ERROR\n");
   }
   
   return (PD_HANDLE) pCB;
}

/*******************************************************************************
NAME:       PDComAPI_subscribe
ABSTRACT:   Subscribe for comid data
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_subscribe(
   UINT32 schedGrp,     /* schedule group */
   UINT32 comId,        /* comId */
   const char *pSource) /* Pointer to string with source URI. 
                           Will override information in the configuration 
                           database. 
                           Set NULL if not used. */
{
   return PDComAPI_sub(schedGrp, comId, 0, pSource, 0, NULL);
}

/*******************************************************************************
NAME:       PDComAPI_subscribeWfilter
ABSTRACT:   Subscribe for comid data
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_subscribeWfilter(
   UINT32 schedGrp,    /* schedule group */
   UINT32 comId,       /* comId */
   UINT32 filterId)    /* Source URI filter ID */
{
   return PDComAPI_sub(schedGrp, comId, filterId, NULL, 0, NULL);
}

/*******************************************************************************
NAME:       PDComAPI_renewSub
ABSTRACT:   Renew IP addresses that has been changed after an inauguration
RETURNS  :  0 if OK, !=0 if error.
*/
int PDComAPI_renewSub(
   PD_HANDLE handle)   /* Handle returned at subscribe */
{
   int ret;
   PD_SUB_CB *pSubCB;    /* pointer to control block */
   
   if (handle == (PD_HANDLE) NULL)
   {
      IPTVosPrint0(IPT_ERR,"Renewing subscription failed. Illegal handle (NULL)\n");
      return (int)IPT_INVALID_PAR;
   }
   
   /* Set pointer (=handle) to the comid control block */
   pSubCB = (PD_SUB_CB *) handle;

   ret = IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {   
      ret = pdSub_renew(pSubCB);

      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_renewSub: IPTVosPutSem(recSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_renewSub: IPTVosGetSem(recSem) ERROR\n");
   }

   return(ret);
}

/*******************************************************************************
NAME:       PDComAPI_pub
ABSTRACT:   Register publisher of comid with defered sending until application
            has called put and source methods
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_pub(
   UINT32 schedGrp,   /* schedule group */
   UINT32 comId,      /* comId */
   UINT32 destId,     /* destination URI Id */
   const char *pDest) /* Pointer to string with destination URI. 
                         Will override information in the configuration database. 
                         Set NULL if not used. */
{
   PD_PUB_CB *pPubCB = NULL;
   
   if (comId == 0)
   {
      IPTVosPrint0(IPT_ERR, "Publishing failed. Illegal input parameters. ComId=0\n");
      return (PD_HANDLE) NULL;
   }
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
#ifdef TARGET_SIMU
      pPubCB = pdPubComidCB_get(schedGrp, comId, destId, pDest, TRUE, NULL);
#else
      pPubCB = pdPubComidCB_get(schedGrp, comId, destId, pDest, TRUE);
#endif
      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_pub: IPTVosPutSem(sendSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_pub: IPTVosGetSem(sendSem) ERROR\n");
   }
   
   return (PD_HANDLE) pPubCB;
}

/*******************************************************************************
NAME:       PDComAPI_publish
ABSTRACT:   Register publisher of comid
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_publish(
   UINT32 schedGrp,   /* schedule group */
   UINT32 comId,      /* comId */
   const char *pDest) /* Pointer to string with destination URI. 
                         Will override information in the configuration database. 
                         Set NULL if not used. */
{
   PD_PUB_CB *pPubCB;
   
   pPubCB = (PD_PUB_CB *)PDComAPI_pub(schedGrp, comId, 0, pDest); 
   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      if (pPubCB)
      {
         /* Indicate that data has been updated onces, i.e. sending of data will
            be started immediately.
            This is done here of compability reason */
         pPubCB->pdCB.updatedOnce = TRUE;

         if (pPubCB->pSendNetCB)
         {
            /* Set to true to indicate that the buffer has been updated at least
             onces, i.e. sending of data will be started immediately.
            This is done here of compability reason  */
            pPubCB->pSendNetCB->updatedOnceNs = TRUE;
         }
      }
   
      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_publish: IPTVosPutSem(sendSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_publish: IPTVosGetSem(sendSem) ERROR\n");
   }
   
   return (PD_HANDLE) pPubCB;
}

/*******************************************************************************
NAME:       PDComAPI_publishDef
ABSTRACT:   Register publisher of comid with defered sending until application
            has called put and source methods
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_publishDef(
   UINT32 schedGrp,   /* schedule group */
   UINT32 comId,      /* comId */
   const char *pDest) /* Pointer to string with destination URI. 
                         Will override information in the configuration database. 
                         Set NULL if not used. */
{
  return PDComAPI_pub(schedGrp, comId, 0, pDest); 
}

/*******************************************************************************
NAME:       PDComAPI_renewPub
ABSTRACT:   Renew IP addresses that has been changed after an inauguration
RETURNS  :  0 if OK, !=0 if error.
*/
int PDComAPI_renewPub(
   PD_HANDLE handle)   /* Handle returned at subscribe */
{
   int ret;

   PD_PUB_CB *pPubCB;    /* pointer to control block */
   
   if (handle == (PD_HANDLE) NULL)
   {
      IPTVosPrint0(IPT_ERR,"Renewing publish failed. Illegal handle (NULL)\n");
      return (int)IPT_INVALID_PAR;
   }
   
   /* Set pointer (=handle) to the comid control block */
   pPubCB = (PD_PUB_CB *) handle;

   ret = IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {   
      ret = pdPub_renew(pPubCB);

      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_renewPub: IPTVosPutSem(sendSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_renewPub: IPTVosGetSem(sendSem) ERROR\n");
   }

   return(ret);
}

/*******************************************************************************
NAME     :  PDComAPI_get
ABSTRACT :  Get data from dataset in schedGrpBuffer into application buffer
RETURNS  :  0 if OK, !=0 if error.
*/
int PDComAPI_get(
   PD_HANDLE handle,   /* Handle returned at subscribe */
   BYTE *pBuffer,      /* Pointer to application buffer */
   UINT16 bufferSize)  /* Size of application buffer */
{
   PD_SUB_CB *pSubCB;    /* pointer to control block */
   
   if (handle == (PD_HANDLE) NULL)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_get failed. Illegal handle (NULL)\n");
      return (int)IPT_NOT_FOUND;
   }
   
   if (pBuffer == NULL)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_get failed. Illegal buffer (NULL)\n");
      return (int)IPT_NOT_FOUND;
   }

   /* Set pointer (=handle) to the comid control block */
   pSubCB = (PD_SUB_CB *) handle;
   
   if (pSubCB->pdCB.pDataBuffer != NULL)
   {
      if (bufferSize < pSubCB->pdCB.size)
      {
         if (pSubCB->pRecNetCB[0])
         {
            /* application buffer too small */
            IPTVosPrint2(IPT_ERR,
                         "PDComAPI_get failed. Application buffer to small, must be %d for ComId=%d\n", 
                         pSubCB->pdCB.size, pSubCB->pRecNetCB[0]->comId);
         }
         else
         {
            /* application buffer too small */
            IPTVosPrint1(IPT_ERR,
                         "PDComAPI_get failed. Application buffer to small, must be %d\n", 
                         pSubCB->pdCB.size);
         }
         return (int)IPT_ILLEGAL_SIZE;
      }
   
      /* Copy comid schedGrpBuffer to the application buffer
         If not any message has been received the pDataBuffer contains onle 
         zeroes */
      memcpy(pBuffer, pSubCB->pdCB.pDataBuffer, pSubCB->pdCB.size);  
   }
   else
   {
      memset(pBuffer, 0, bufferSize);
   }
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME     :  PDComAPI_getWStatus
ABSTRACT :  Get data from dataset in schedGrpBuffer into application buffer
            Get status, i.e. if data is set to invalid or not.
            IPTCom set data to invalid if a time-out value is defined and the
            data is older than the time-out value. 
RETURNS  :  0 if OK, !=0 if error.
*/
int PDComAPI_getWStatus(
   PD_HANDLE handle,   /* Handle returned at subscribe */
   BYTE *pBuffer,      /* Pointer to application buffer */
   UINT16 bufferSize,  /* Size of application buffer */
   int  *pInValid)     /* Flag inticating if data has been received within 
                          time-out time */
{
   PD_SUB_CB *pSubCB;    /* pointer to control block */
   
   if (handle == (PD_HANDLE) NULL)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_getWStatus failed. Illegal handle (NULL)\n");
      return (int)IPT_INVALID_PAR;
   }
   
   if (pBuffer == NULL)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_getWStatus failed. Illegal buffer (NULL)\n");
      return (int)IPT_INVALID_PAR;
   }

   /* Set pointer (=handle) to the comid control block */
   pSubCB = (PD_SUB_CB *) handle;
   
   if (pSubCB->pdCB.pDataBuffer != NULL)
   {
      if (bufferSize < pSubCB->pdCB.size)
      {
         /* application buffer too small */
         IPTVosPrint2(IPT_ERR,
                      "PDComAPI_getWStatus failed. Application buffer to small, must be %d for ComId=%d\n", 
                      pSubCB->pdCB.size, pSubCB->pdCB.comId);
         return (int)IPT_ILLEGAL_SIZE;
      }
   
      /* Copy comid schedGrpBuffer to the application buffer
         If not any message has been received the pDataBuffer contains onle 
         zeroes */
      memcpy(pBuffer, pSubCB->pdCB.pDataBuffer, pSubCB->pdCB.size);  
   }
   else
   {
      memset(pBuffer, 0, bufferSize);
   }
   
   /* Any data ever been received? */
   if (pSubCB->pdCB.updatedOnce)
   {
      if (pSubCB->pdCB.invalid)
      {
         *pInValid = IPT_INVALID_OLD;
      }
      else
      {
         *pInValid = IPT_VALID;
      }
   }
   else
   {
      *pInValid = IPT_INVALID_NOT_RECEIVED;
   }
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME     :  PDComAPI_put
ABSTRACT :  Put data from application into dataset in schedGrpBuffer
RETURNS  :  -
*/
void PDComAPI_put(
   PD_HANDLE handle,    /* Handle returned at publish */
   const BYTE *pBuffer) /* Pointer to buffer */
{
   PD_PUB_CB *pPubCB;    /* pointer to control block */
   
   if (handle == (PD_HANDLE) 0)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_put failed. Illegal handle (NULL)\n");
      return;
   }
   
   if (pBuffer == NULL)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_put failed. Illegal buffer (NULL)\n");
      return;
   }

   /* Set pointer (=handle) to the comid control block */
   pPubCB = (PD_PUB_CB *) handle;
   
   if (pPubCB->pdCB.pDataBuffer != NULL)
   {
      /* Copy the application buffer to schedGrpBuffer */
      memcpy(pPubCB->pdCB.pDataBuffer, pBuffer, pPubCB->pdCB.size);
   
      /* Indicate new data */
      pPubCB->pubCBchanged = TRUE;

      /* Indicate that data has been updated onces
         Used by PDComAPI_get and PDComAPI_getWStatus when called with a 
         publish handle*/
      pPubCB->pdCB.updatedOnce = TRUE;
   }
}

/*******************************************************************************
NAME     :  PDComAPI_getStatistics
ABSTRACT :  Get statistics for communication
RETURNS  :  0 if OK, !=0 if error
*/
int PDComAPI_getStatistics(
   PD_HANDLE Handle,     /* Handle returned at subscribe */
   BYTE *buffer,         /* Buffer for statistics */
   UINT16 *pBufferSize)  /* Size of buffer */
{
   IPT_UNUSED (Handle)
   IPT_UNUSED (buffer)
   IPT_UNUSED (pBufferSize)

   /* Get statistics now implemented with IPTCom_stat* methods */
   return (int)IPT_OK;
}

/*******************************************************************************
NAME     :  PDComAPI_unsubscribe
ABSTRACT :  Stop subscribing for comid data
RETURNS  :  -
*/
void PDComAPI_unsubscribe(
   PD_HANDLE *pHandle)  /* Pointer to handle returned at subscribe */
{
   PD_SUB_CB *pSubCB;

   if (pHandle == NULL)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_unsubscribe failed. Illegal phandle=NULL\n");
      return;
   }
   if (*pHandle == (PD_HANDLE) NULL)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_unsubscribe failed. Illegal handle (NULL)\n");
      return;
   }
   pSubCB = (PD_SUB_CB *) (*pHandle);

   if (IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      /* Unregister subscriber, cleanup CB if possible */
      pdComidSubCB_cleanup(pSubCB);
   
      /* Set applications handle to NULL to stop it from using it again */
      *pHandle = (PD_HANDLE) NULL;
   
      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_unsubscribe: IPTVosPutSem(recSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_unsubscribe: IPTVosGetSem(recSem) ERROR\n");
   }
}

/*******************************************************************************
NAME     :  PDComAPI_unpublish
ABSTRACT :  Unregister publisher of comid
Deletes the handle. No use here after!
RETURNS  :  -
*/
void PDComAPI_unpublish(
   PD_HANDLE *pHandle)  /* Pointer to handle returned at publish */
{
   PD_PUB_CB *pPubCB;
   
   if (pHandle == NULL)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_unpublish failed. Illegal phandle=NULL\n");
      return;
   }
   if (*pHandle == (PD_HANDLE) NULL)
   {
      IPTVosPrint0(IPT_ERR,"PDComAPI_unpublish failed. Illegal handle (NULL)\n");
      return;
   }
   
   pPubCB = (PD_PUB_CB *) (*pHandle);

   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
   {   
      /* Unregister publisher, cleanup CB if possible */
      pdComidPubCB_cleanup(pPubCB);
      
      /* Set applications handle to NULL to stop it from using it again */
      *pHandle = (PD_HANDLE) NULL;
      
      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_unpublish: IPTVosPutSem(sendSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_unpublish: IPTVosGetSem(sendSem) ERROR\n");
   }
}

/*******************************************************************************
NAME     :  PDComAPI_setRedundant
ABSTRACT :  Set all redundant ID's to be a redundant leader or follower. All  
            comid's configured as redundant will be sent if leader parameter is
           TRUE and not sent if leader parameter is FALSE.
RETURNS  : -
*/
void PDComAPI_setRedundant(
   UINT32 leader) /* If set to TRUE this device is a leader, if FALSE it is 
                     follower */
{
   UINT32 i;
   UINT32 mode;

   PD_SEND_NET_CB *pSendNetCB;    /* Pointer to netbuffer control block to send */
   PD_RED_ID_ITEM *pRedIdItem;

   if (leader)
   {
      mode = TRUE;   
   }
   else
   {
      mode = FALSE;   
   }
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      /* Walk through all comid's to see if there is any publisher due for sending */
      pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB); 
      while (pSendNetCB != NULL)
      {
         if (pSendNetCB->redFuncId != 0)
         {
            pSendNetCB->leader = mode;   
         }

         pSendNetCB = pSendNetCB->pNext;    /* Go to next */
      }
 
      pRedIdItem = (PD_RED_ID_ITEM *)(void *)IPTGLOBAL(pd.redIdTableHdr.pTable);
      for (i=0; i<IPTGLOBAL(pd.redIdTableHdr.nItems); i++)
      {
         pRedIdItem[i].leader = mode; 
      }
      
      IPTGLOBAL(pd.leaderDev) = mode;

      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_setRedundant: IPTVosPutSem(sendSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_setRedundant: IPTVosGetSem(sendSem) ERROR\n");
   }
}

/*******************************************************************************
NAME    :  PDComAPI_setRedundantId
ABSTRACT:  Set redundant ID to be a redundant leader or follower.  
           All comid's configured with the the redundant ID equal to the
           parameter redId will be set to leader or follower according to value
           of the parameter leader. 
           The data will be sent if it is leader, but not sent if it is follower.
RETURNS : -
*/
void PDComAPI_setRedundantId(
   UINT32 leader,    /* If set to TRUE redundant function is a leader, if FALSE   
                        it is a follower */
   UINT32 redId)     /* Reduntant ID */
{
   int res;
   UINT32 mode;
   PD_RED_ID_ITEM redIdItem;
   PD_RED_ID_ITEM *pRedIdItem;
   PD_SEND_NET_CB *pSendNetCB;    /* Pointer to netbuffer control block to send */
  
   if (leader)
   {
      mode = TRUE;   
   }
   else
   {
      mode = FALSE;   
   }
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      pRedIdItem = (PD_RED_ID_ITEM *)(void*)iptTabFind(&IPTGLOBAL(pd.redIdTableHdr),
                                                       redId);/*lint !e826  Ignore casting warning */
      if (pRedIdItem == NULL)
      {
         redIdItem.redId = redId;
         redIdItem.leader = mode;   

         /* Add the redundancy function reference to the queue listeners table */
         res = iptTabAdd(&IPTGLOBAL(pd.redIdTableHdr), (IPT_TAB_ITEM_HDR *)((void *)&redIdItem));
         if (res != IPT_OK)
         {
            IPTVosPrint2(IPT_ERR,
               "PDComAPI_setRedundantId: Failed to add redId=%d to table. Error=%#x\n",
               redId,res);
         }
      }
      else
      {
         pRedIdItem->leader = mode;   
      }

      /* Walk through all comid's to see if there is any publisher due for sending */
      pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB); 
      while (pSendNetCB != NULL)
      {
         if (pSendNetCB->redFuncId == redId)
         {
            pSendNetCB->leader = mode;   
         }

         pSendNetCB = pSendNetCB->pNext;    /* Go to next */
      }

      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_setRedundantId: IPTVosPutSem(sendSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_setRedundantId: IPTVosGetSem(sendSem) ERROR\n");
   }
}

/*******************************************************************************
*  NAME     : PDComAPI_sink
*  ABSTRACT : Copy all data from netbuffer to schedGrpBuffer for all comid's that 
*             are subscribed or published by applications in this schedule group.
*  RETURNS  : -
*/
void PDComAPI_sink(
   UINT32 schedGrp)  /* Current schedule group */
{
#define MAXVAL    0xffffffff
   int i;
   int schedIx;
   int newest = 0;
   UINT8 invalid;
   UINT8 changed;
   UINT8 first;
   PD_SUB_CB *pSubCB;
   PD_REC_NET_CB  *pRecNetCB;
   UINT32 newestTimeRec = 0;
   UINT32 now, delta;
   SUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_sink: IPTVosGetSem(recSem) ERROR\n");
      return;
   }
   
   if (!pdSubGrpTabFind(schedGrp, &schedIx))
   {
      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_sink: IPTVosPutSem(recSem) ERROR\n");
      }
      return;     /* Nothing registered for this schedGrp */
   }

   /* Copy all comid netbuffer to schedGrpBuffer for which there are at 
      least one subscriber or one publisher in this schedule group */
   pSubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
   while (pSubCB != NULL)
   {
      first = TRUE;
      invalid = TRUE;
      changed = FALSE;
      for (i = 0; i < pSubCB->noOfNetCB ; i++)
      {
         pRecNetCB = pSubCB->pRecNetCB[i];

         /* Ever received ? */
         if ((pRecNetCB ) && (pRecNetCB->updatedOnceNr))
         {
            if (!pRecNetCB->invalid )
            {
               /* Check validity if any receive timeout is specified */
               if (pSubCB->timeout != 0)
               {
                  /* Check if it has been too long time since we last received a
                     dataset */
                  now = IPTVosGetMilliSecTimer();
                  /* Takes care of the wrap around */
                  delta = (pRecNetCB->timeRec + pSubCB->timeout) - now -1;

                  if (delta >= pSubCB->timeout) 
                  {
                     /* Too long time, tag this netbuffer as invalid */
                     pRecNetCB->invalid = TRUE;
                  }
                  else
                  {
                     invalid = FALSE;
                  }
               }
               else
               {
                  invalid = FALSE;
               }

               if (!invalid)
               {
                  if(first)
                  {
                     newestTimeRec = pRecNetCB->timeRec;
                     newest = i;
                     first = FALSE;   
                  }
                  else
                  {
                     /* Select the newest message */
                     delta = pRecNetCB->timeRec - newestTimeRec;
                     if (delta < MAXVAL/2)
                     {
                        newestTimeRec = pRecNetCB->timeRec;
                        newest = i;
                     } 
                  }

                  changed = TRUE;
               }
            }
         }
      }

      /* New data has been received? */
      if ((changed) && (pSubCB->pNetbuffer[newest]))
      {
         memcpy(pSubCB->pdCB.pDataBuffer, pSubCB->pNetbuffer[newest], pSubCB->pdCB.size);

         pSubCB->pdCB.invalid = FALSE;

         /* Set to true to indicate at least one message has been received */
         pSubCB->pdCB.updatedOnce = TRUE;
      }
      else
      {
         /* timed out? */
         if (invalid)
         {
            pSubCB->pdCB.invalid = TRUE;

            /* Set to zero if invalid? */
            if ( pSubCB->invalidBehaviour == 0)
               memset(pSubCB->pdCB.pDataBuffer, 0, pSubCB->pdCB.size);
         }
      }
      
      pSubCB = pSubCB->pNext;
   }
   
   if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_sink: IPTVosPutSem(recSem) ERROR\n");
   }
}

/*******************************************************************************
NAME     : PDComAPI_source
ABSTRACT : Copy all data from schedGrpBuffer to netbuffer for all comid's that 
are published, and have been changed, by applications in this schedule group
RETURNS  : -
*/
void PDComAPI_source(
   UINT32 schedGrp)  /* Current schedule group */
{
   int schedIx;
   int ret;
   BYTE temp[PD_DATASET_MAXSIZE];
   UINT32 datasetSize;
   UINT32 datasetSizeFCS;
   PD_PUB_CB *pPubCB;
   PUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.pubSchedGrpTab);
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_source: IPTVosGetSem(sendSem) ERROR\n");
      return;
   }

   if (!pdPubGrpTabFind(schedGrp, &schedIx))
   {
      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_source: IPTVosPutSem(sendSem) ERROR\n");
      }
      return;     /* Nothing registered for this schedGrp */
   }
   
   /* Copy all comid schedGrpBuffer to netbuffer for which there are at 
      least one publisher in this schedule group, and where the content has changed */
   pPubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
   while (pPubCB != NULL)
   {
      if ((pPubCB->pubCBchanged) && (pPubCB->pSendNetCB) && (pPubCB->pdCB.pDataBuffer))
      {
         datasetSize = pPubCB->pdCB.size;   /* Was: PD_DATASET_MAXSIZE, wrong in any case!   */
         ret = iptMarshallDSF(pPubCB->nLines, pPubCB->alignment, pPubCB->disableMarshalling,
                              pPubCB->pDatasetFormat,
                              pPubCB->pdCB.pDataBuffer, temp,
                              &datasetSize);
         if ((ret == (int)IPT_OK) && (pPubCB->pNetDatabuffer))
         {
            /* Load dataset */
            datasetSizeFCS = pPubCB->netDatabufferSize;
            ret = iptLoadSendData(temp, datasetSize, pPubCB->pNetDatabuffer, 
                                  &datasetSizeFCS);
            if ((ret == (int)IPT_OK) && (pPubCB->pPdHeader))
            {
               pPubCB->pPdHeader->datasetLength = TOWIRE16(datasetSize);

               pPubCB->pSendNetCB->size = sizeof(PD_HEADER) + datasetSizeFCS;
               
               /* Set to true to indicate that the buffer has been updated at least onces */
               pPubCB->pSendNetCB->updatedOnceNs = TRUE;

               /* Set to false to avoid unnecessary copying */
               pPubCB->pubCBchanged = FALSE;
            }
            else
            {
               IPTVosPrint1(IPT_ERR, "PDComAPI_source (1) failed for ComId=%d.\n",
                     pPubCB->pdCB.comId);
            }
         }
         else
         {
            IPTVosPrint1(IPT_ERR, "PDComAPI_source (2) failed for ComId=%d.\n",
                     pPubCB->pdCB.comId);
         }
      }
      
      pPubCB = pPubCB->pNext;
   }
   
   if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_source: IPTVosPutSem(sendSem) ERROR\n");
   }
}


/*******************************************************************************
NAME     :  PDCom_prepareInit
ABSTRACT :  Initilizes PD communication.
The pdControl data is set up to have one empty link list for the 
netbuffer, but no other schedule groups registered.
RETURNS  :  0 if OK, !=0 if not
*/
int PDCom_prepareInit(void)
{
   int res;
   IPTGLOBAL(pd.pFirstSendNetCB) = NULL;       
   IPTGLOBAL(pd.pFirstRecNetCB) = NULL;       
   IPTGLOBAL(pd.pFirstNotResPubCB) = NULL;       
   IPTGLOBAL(pd.pFirstNotResSubCB) = NULL;       
   
   if ((res = pdRecTabInit()) != IPT_OK)
      return res;

   if ((res = pdGrpTabInit()) != IPT_OK)
      return res;

   if ((res = IPTVosCreateSem(&IPTGLOBAL(pd.sendSem), IPT_SEM_FULL)) != IPT_OK)
      return res;
   
   if ((res = IPTVosCreateSem(&IPTGLOBAL(pd.recSem), IPT_SEM_FULL)) != IPT_OK)
      return res;
   
   if ((res = iptTabInit(&IPTGLOBAL(pd.sendTableHdr), sizeof(PD_CYCLE_ITEM))) != (int)IPT_OK)
      return res;

   IPTGLOBAL(pd.leaderDev) = FALSE;
   if ((res = iptTabInit(&IPTGLOBAL(pd.redIdTableHdr), sizeof(PD_RED_ID_ITEM))) != (int)IPT_OK)
      return res;

   return (int)IPT_OK;
}

/*******************************************************************************
NAME     :  PDCom_process
ABSTRACT :  Process method for pdCom component.
RETURNS  :  
*/
void PDCom_process(void)
{
   /* Do nothing */
}

/*******************************************************************************
NAME     :  PDCom_destroy
ABSTRACT :  Termination method for pdCom component. 
All comid control blocks are deleted, including any memory allocated.
RETURNS  :  -
*/
void PDCom_destroy(void)
{
   int i;
   PD_PUB_CB *pPubCB, *pPubNext;
   PD_SUB_CB *pSubCB, *pSubNext;
   PD_SEND_NET_CB *pSendNetCB, *pNextSendNetCB;
   PD_REC_NET_CB *pRecNetCB, *pNextRecNetCB;
   PD_NOT_RESOLVED_SUB_CB *pNotResolvedSubCB;
   PD_NOT_RESOLVED_SUB_CB *pNextNotResolvedSubCB;
   PD_NOT_RESOLVED_PUB_CB *pNotResolvedPubCB;
   PD_NOT_RESOLVED_PUB_CB *pNextNotResolvedPubCB;

   if (IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "PDCom_destroy: IPTVosGetSem(recSem) ERROR\n");
      return;
   }

   /* free all used comid control blocks */
   for (i = 0; i < IPTGLOBAL(pd.subSchedGrpTab.nItems); i++)
   {
      pSubCB = IPTGLOBAL(pd.subSchedGrpTab.pTable)[i].pFirstCB;
      
      while (pSubCB != NULL)
      {
         pSubNext = pSubCB->pNext;
         if (pSubCB->pDestUri != NULL )
         {
            IPTVosFree((BYTE *) pSubCB->pDestUri);
         } 
         if (pSubCB->pSourceUri != NULL )
         {
            IPTVosFree((BYTE *) pSubCB->pSourceUri);
         } 
      #ifdef TARGET_SIMU
         if (pSubCB->pSimUri != NULL )
         {
            IPTVosFree((BYTE *) pSubCB->pSimUri);
         } 
      #endif               
   
         if (pSubCB->pdCB.pDataBuffer != NULL)
         {
            IPTVosFree((BYTE *) pSubCB->pdCB.pDataBuffer);
         }
         IPTVosFree((BYTE *) pSubCB);
         pSubCB = pSubNext;
      }
   }
   
   pNotResolvedSubCB = IPTGLOBAL(pd.pFirstNotResSubCB); 
   while (pNotResolvedSubCB != NULL)
   {
      pNextNotResolvedSubCB = pNotResolvedSubCB->pNext;
      if (pNotResolvedSubCB->pSubCB != NULL)
      {
         if (pNotResolvedSubCB->pSubCB->pDestUri != NULL )
         {
            IPTVosFree((BYTE *) pNotResolvedSubCB->pSubCB->pDestUri);
         } 
         if (pNotResolvedSubCB->pSubCB->pSourceUri != NULL )
         {
            IPTVosFree((BYTE *) pNotResolvedSubCB->pSubCB->pSourceUri);
         } 
      #ifdef TARGET_SIMU
         if (pNotResolvedSubCB->pSubCB->pSimUri != NULL )
         {
            IPTVosFree((BYTE *) pNotResolvedSubCB->pSubCB->pSimUri);
         } 
      #endif               
         
         if (pNotResolvedSubCB->pSubCB->pdCB.pDataBuffer != NULL)
         {
            IPTVosFree((BYTE *) pNotResolvedSubCB->pSubCB->pdCB.pDataBuffer);
         }
         IPTVosFree((BYTE *) pNotResolvedSubCB->pSubCB);
      }
      IPTVosFree((BYTE *) pNotResolvedSubCB);          
      pNotResolvedSubCB = pNextNotResolvedSubCB;
   }
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) != IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "PDCom_destroy: IPTVosGetSem(sendSem) ERROR\n");
      return;
   }

   for (i = 0; i < IPTGLOBAL(pd.pubSchedGrpTab.nItems); i++)
   {
      pPubCB = IPTGLOBAL(pd.pubSchedGrpTab.pTable)[i].pFirstCB;
      
      while (pPubCB != NULL)
      {
         pPubNext = pPubCB->pNext;
         
         /* buffer  for overide destination URI? */
         if (pPubCB->pDestUri != NULL )
         {
            IPTVosFree((BYTE *) pPubCB->pDestUri);
         } 
#ifdef TARGET_SIMU               
         if (pPubCB->pSimUri != NULL )
         {
            IPTVosFree((BYTE *) pPubCB->pSimUri);
         } 
#endif               
         if (pPubCB->pdCB.pDataBuffer != NULL)
         {
         IPTVosFree((BYTE *) pPubCB->pdCB.pDataBuffer);
         }
         IPTVosFree((BYTE *) pPubCB);
         
         pPubCB = pPubNext;
      }
   }
   
   pNotResolvedPubCB = IPTGLOBAL(pd.pFirstNotResPubCB); 
   while (pNotResolvedPubCB != NULL)
   {
      pNextNotResolvedPubCB = pNotResolvedPubCB->pNext;
      
      if (pNotResolvedPubCB->pPubCB != NULL)
      {
         /* buffer  for overide destination URI? */
         if (pNotResolvedPubCB->pPubCB->pDestUri != NULL )
         {
            IPTVosFree((BYTE *) pNotResolvedPubCB->pPubCB->pDestUri);
         } 
#ifdef TARGET_SIMU               
         if (pNotResolvedPubCB->pPubCB->pSimUri != NULL )
         {
            IPTVosFree((BYTE *) pNotResolvedPubCB->pPubCB->pSimUri);
         } 
#endif               
         if (pNotResolvedPubCB->pPubCB->pdCB.pDataBuffer != NULL)
         {
            IPTVosFree((BYTE *) pNotResolvedPubCB->pPubCB->pdCB.pDataBuffer);
         }
         IPTVosFree((BYTE *) pNotResolvedPubCB->pPubCB);
      }
      IPTVosFree((BYTE *) pNotResolvedPubCB);          
      
      pNotResolvedPubCB = pNextNotResolvedPubCB;
   }
   
   pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB);
   while (pSendNetCB != NULL)
   {
      pNextSendNetCB = pSendNetCB->pNext;

      if (pSendNetCB->pSendBuffer)
      {
      IPTVosFree((BYTE *) pSendNetCB->pSendBuffer);
      }
      IPTVosFree((BYTE *) pSendNetCB);

      pSendNetCB = pNextSendNetCB;
   }
   
   pRecNetCB = IPTGLOBAL(pd.pFirstRecNetCB);
   while (pRecNetCB != NULL)
   {
      pNextRecNetCB = pRecNetCB->pNext;
      pdRecNetCB_destroy(pRecNetCB);
      pRecNetCB = pNextRecNetCB;
   }
   
   pdRecTabTerminate();
   pdGrpTabTerminate();

   IPTVosDestroySem(&IPTGLOBAL(pd.sendSem));
   IPTVosDestroySem(&IPTGLOBAL(pd.recSem));
}



/*******************************************************************************
NAME:     PDCom_clearStatistic
ABSTRACT: Clear diagnostic counters.
RETURNS:  -
*/
int PDCom_clearStatistic(void)
{
   int i, j;
   PD_SEND_NET_CB *pSendNetCB;    /* Pointer to netbuffer control block */

   IPTGLOBAL(pd.pdCnt.pdStatisticsStarttime) = IPTVosGetMilliSecTimer(); 
   IPTGLOBAL(pd.pdCnt.pdInPackets) = 0;
   IPTGLOBAL(pd.pdCnt.pdInFCSErrors) = 0;
   IPTGLOBAL(pd.pdCnt.pdInProtocolErrors) = 0;
   IPTGLOBAL(pd.pdCnt.pdInTopoErrors) = 0;
   IPTGLOBAL(pd.pdCnt.pdInNoSubscriber) = 0;
   IPTGLOBAL(pd.pdCnt.pdOutPackets) = 0;

   if (IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      if (IPTGLOBAL(pd.recTab.pTable))
      {
         for (i = 0; i < IPTGLOBAL(pd.recTab.nItems); i++)
         {
            if ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pRecNetCB)
            {
               ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pRecNetCB)->pdInPackets = 0;
            }
            if (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable)
            {
               for (j=0; j<IPTGLOBAL(pd.recTab.pTable)[i].nItems; j++)
               {
                  if ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].pFiltRecNetCB)
                  {
                     ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].pFiltRecNetCB)->pdInPackets = 0;
                  }
               }
            }
         }
      }
 
      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDCom_clearStatistic: IPTVosPutSem(recSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDCom_clearStatistic: IPTVosGetSem(recSem) ERROR\n");
   }

   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB); 
      while (pSendNetCB != NULL)
      {
         pSendNetCB->pdOutPackets = 0; 

         pSendNetCB = pSendNetCB->pNext;    /* Go to next */
      }

      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDCom_clearStatistic: IPTVosPutSem(sendSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDCom_clearStatistic: IPTVosGetSem(sendSem) ERROR\n");
   }

   return((int)IPT_OK);
}

/*******************************************************************************
NAME:     PDCom_showStatistic
ABSTRACT: Print diagnostic counters.
RETURNS:  -
*/
void PDCom_showStatistic(void)
{
   int i,j;
   PD_SEND_NET_CB *pSendNetCB;    /* Pointer to netbuffer control block */

   MON_PRINTF("PD statistic\n");
   MON_PRINTF("Time in seconds since last clearing of statistics = %d\n",
          IPTVosGetMilliSecTimer() - IPTGLOBAL(pd.pdCnt.pdStatisticsStarttime)); 
   MON_PRINTF("No of received PD packets = %d\n",
          IPTGLOBAL(pd.pdCnt.pdInPackets));
   MON_PRINTF("No of received PD packets with FCS errors = %d\n",
          IPTGLOBAL(pd.pdCnt.pdInFCSErrors));
   MON_PRINTF("No of received PD packets with wrong protocol = %d\n",
          IPTGLOBAL(pd.pdCnt.pdInProtocolErrors));
   MON_PRINTF("No of received PD packets with wrong topocounter = %d\n",
          IPTGLOBAL(pd.pdCnt.pdInTopoErrors));
   MON_PRINTF("No of received PD packets without subscriber = %d\n",
          IPTGLOBAL(pd.pdCnt.pdInNoSubscriber));
   MON_PRINTF("No of transmitted PD packets = %d\n",
          IPTGLOBAL(pd.pdCnt.pdOutPackets));

   MON_PRINTF("PD subscriber:\n");

   if (IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      for (i = 0; i < IPTGLOBAL(pd.recTab.nItems); i++)
      {
         if (IPTGLOBAL(pd.recTab.pTable))
         {
            if (IPTGLOBAL(pd.recTab.pTable)[i].pRecNetCB)
            {
               MON_PRINTF(" Comid=%d No source filter Received=%d Invalid=%d\n",
                      IPTGLOBAL(pd.recTab.pTable)[i].comId,
                      ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pRecNetCB)->pdInPackets,
                      ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pRecNetCB)->invalid); 
            }
            for (j=0; j<IPTGLOBAL(pd.recTab.pTable)[i].nItems; j++)
            {
               if ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].pFiltRecNetCB)
               {
                  MON_PRINTF(" Comid=%d Source filter=%d.%d.%d.%d Received=%d Invalid=%d\n",
                          IPTGLOBAL(pd.recTab.pTable)[i].comId,
                          (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr & 0xff000000) >> 24,
                          (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr & 0xff0000) >> 16,
                          (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr & 0xff00) >> 8,
                          IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr & 0xff,
                          ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].pFiltRecNetCB)->pdInPackets,
                          ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].pFiltRecNetCB)->invalid); 
               }
            }
         }
      }

      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDCom_showStatistic: IPTVosPutSem(recSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDCom_showStatistic: IPTVosGetSem(recSem) ERROR\n");
   }

   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      MON_PRINTF("PD publisher:\n");
   
      pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB); 
      while (pSendNetCB != NULL)
      {
         MON_PRINTF(" Comid=%d Dest IP=%d.%d.%d.%d Transmitted=%d\n",
                pSendNetCB->comId,
                (pSendNetCB->destIp & 0xff000000) >> 24,
                (pSendNetCB->destIp & 0xff0000) >> 16,
                (pSendNetCB->destIp & 0xff00) >> 8,
                pSendNetCB->destIp & 0xff,
                pSendNetCB->pdOutPackets); 

         pSendNetCB = pSendNetCB->pNext;    /* Go to next */
      }

      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDCom_showStatistic: IPTVosPutSem(sendSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDCom_showStatistic: IPTVosGetSem(sendSem) ERROR\n");
   }
}

#ifdef TARGET_SIMU
/*******************************************************************************
NAME:       PDComAPI_subSim
ABSTRACT:   Subscribe for comid data (simulated)
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_subSim(
   UINT32 schedGrp,     /* schedule group */
   UINT32 comId,        /* comId */
   UINT32 filterId,    /* Source URI filter ID */
   const char *pSource, /* Pointer to string with source URI. 
                           Will override information in the configuration 
                           database. 
                           Set NULL if not used. */
   UINT32 destId,         /* Destination URI Id */
   const char *pDest,     /* Pointer to string with destination URI. 
                             Will override information in the configuration database. 
                             Set NULL if not used. */
   const char *pSimUri)   /* Destination address */

{
   PD_SUB_CB *pCB = NULL;
   
   if (schedGrp == 0)
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_subSim: Illegal input parameters. schedGrp=0\n");
      return (PD_HANDLE) NULL;
   }
   else if (comId == 0)
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_subSim: Illegal input parameters. comId=0\n");
      return (PD_HANDLE) NULL;
   }
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER) == IPT_OK)
   {   
   	pCB = pdSubComidCB_get(schedGrp, comId, filterId, pSource, destId, pDest, pSimUri);

      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_subSim: IPTVosPutSem(recSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_subSim: IPTVosGetSem(recSem) ERROR\n");
   }
   
   return (PD_HANDLE) pCB;
}

/*******************************************************************************
NAME:       PDComAPI_subscribeSim
ABSTRACT:   Subscribe for comid data (simulated) 
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_subscribeSim(
   UINT32 schedGrp,     /* schedule group */
   UINT32 comId,        /* comId */
   const char *pSource, /* Pointer to string with source URI. 
                           Will override information in the configuration 
                           database. 
                           Set NULL if not used. */
   const char *pSimUri)   /* Destination address */
{
   return PDComAPI_subSim(schedGrp, comId, 0, pSource, 0, NULL, pSimUri);
}

/*******************************************************************************
NAME:       PDComAPI_subscribeWfilterSim
ABSTRACT:   Subscribe for comid data
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_subscribeWfilterSim(
   UINT32 schedGrp,    /* schedule group */
   UINT32 comId,       /* comId */
   UINT32 filterId,    /* Source URI filter ID */
   const char *pSimUri) /* Source address of this publisher (simulated) */
{
   return PDComAPI_subSim(schedGrp, comId, filterId, NULL, 0, NULL, pSimUri);
}

/*******************************************************************************
NAME:       PDComAPI_pubSim
ABSTRACT:   Register publisher of comid with defered sending until application
            has called put and source methods
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_pubSim(
   UINT32 schedGrp,   /* schedule group */
   UINT32 comId,      /* comId */
   UINT32 destId,     /* destination URI Id */
   const char *pDest, /* Pointer to string with destination URI. 
                         Will override information in the configuration database. 
                         Set NULL if not used. */
   const char *pSimUri) /* Source address of this publisher (simulated) */
{
   PD_PUB_CB *pPubCB = NULL;
   
   if (comId == 0)
   {
       IPTVosPrint0(IPT_ERR, "PDComAPI_pubSim: Illegal input parameters.\n");
      return (PD_HANDLE) NULL;
   }
   
   if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      pPubCB = pdPubComidCB_get(schedGrp, comId, destId, pDest, TRUE, pSimUri);
      
      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_pubSim: IPTVosPutSem(sendSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PDComAPI_pubSim: IPTVosGetSem(sendSem) ERROR\n");
   }
   
   return (PD_HANDLE) pPubCB;
}

/*******************************************************************************
NAME:       PDComAPI_publishSim
ABSTRACT:   Register simulated publisher of comid 
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_publishSim(
   UINT32 schedGrp,     /* schedule group */
   UINT32 comId,        /* comId */
   const char *pDest,   /* Pointer to string with destination URI. 
                           Will override information in the configuration database. 
                           Set NULL if not used. */
   const char *pSimUri) /* Source address of this publisher (simulated) */
{
   PD_PUB_CB *pPubCB;
	
   pPubCB = (PD_PUB_CB *)PDComAPI_pubSim(schedGrp, comId, 0, pDest, pSimUri);

   if (pPubCB)
   {
      if (IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER) == IPT_OK)
      {
         /* Indicate that data has been updated onces, i.e. sending of data will
            be started immediately.
            This is done here of compability reason */
         pPubCB->pdCB.updatedOnce = TRUE;
      
         if (pPubCB->pSendNetCB)
         {
         /* Set to true to indicate that the buffer has been updated at least
         onces, i.e. sending of data will be started immediately.
            This is done here of compability reason  */
            pPubCB->pSendNetCB->updatedOnceNs = TRUE;
         }
         
         if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "PDComAPI_publishSim: IPTVosPutSem(sendSem) ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "PDComAPI_publishSim: IPTVosGetSem(sendSem) ERROR\n");
      }
   }

   return (PD_HANDLE) pPubCB;
}

/*******************************************************************************
NAME:       PDComAPI_publishDefSim
ABSTRACT:   Register publisher of comid with defered sending until application
            has called put and source methods
RETURNS:    Handle to be used for subsequent calls, NULL if error
*/
PD_HANDLE PDComAPI_publishDefSim(
   UINT32 schedGrp,   /* schedule group */
   UINT32 comId,      /* comId */
   const char *pDest, /* Pointer to string with destination URI. 
                         Will override information in the configuration database. 
                         Set NULL if not used. */
   const char *pSimUri) /* Source address of this publisher (simulated) */
{
  return PDComAPI_pubSim(schedGrp, comId, 0, pDest, pSimUri);
}

#endif
