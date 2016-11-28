/*******************************************************************************
*  COPYRIGHT   : (C) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT     :  IPTrain
*
*  MODULE      :  pdcom.cpp
*
*  ABSTRACT    :  Public C++ methods for PD communication classes:
*                 - PDComAPI
*                 - PDCom
*
********************************************************************************
*  HISTORY     :
*	
* $Id: pdcom_cls.cpp 11620 2010-08-16 13:06:37Z bloehr $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*
*******************************************************************************/

/*******************************************************************************
*  INCLUDES */
#include "iptcom.h"	/* Common type definitions for IPT */
#ifdef IPTCOM_CM_COMPONENT
#include "tcms_co.h"
#endif

/*******************************************************************************
*  DEFINES */

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME     :  PDComAPI::sub

ABSTRACT :  Subscribe for comid data

RETURNS  :  Handle to be used for subsequent calls.
            Exception is raised if error.
*/
PDComAPI* PDComAPI::sub(UINT32 schedGrp, UINT32 comId, UINT32 filterId, char *src, UINT32 destId, char *dest)
{
   PDComAPI *api = new PDComAPI();

   api->handle = PDComAPI_sub(schedGrp, comId, filterId, src, destId, dest);

   if (api->handle == (PD_HANDLE) NULL)
   {
		throw PDComAPIException((int)IPT_ERROR, __FILE__, __LINE__ );
   }
   
   return api;
}

/*******************************************************************************
NAME     :  PDComAPI::subscribe

ABSTRACT :  Subscribe for comid data

RETURNS  :  Handle to be used for subsequent calls.
            Exception is raised if error.
*/
PDComAPI* PDComAPI::subscribe(UINT32 schedGrp, UINT32 comId, char *src)
{
   PDComAPI *api = new PDComAPI();

   api->handle = PDComAPI_subscribe(schedGrp, comId, src);

   if (api->handle == (PD_HANDLE) NULL)
   {
		throw PDComAPIException((int)IPT_ERROR, __FILE__, __LINE__ );
   }
   
   return api;
}

/*******************************************************************************
NAME     :  PDComAPI::subscribe

ABSTRACT :  Subscribe for comid data with a source filter

RETURNS  :  Handle to be used for subsequent calls.
            Exception is raised if error.
*/
PDComAPI* PDComAPI::subscribeWfilter(UINT32 schedGrp, UINT32 comId, UINT32 filterId)
{
   PDComAPI *api = new PDComAPI();

   api->handle = PDComAPI_subscribeWfilter(schedGrp, comId, filterId);

   if (api->handle == (PD_HANDLE) NULL)
   {
		throw PDComAPIException((int)IPT_ERROR, __FILE__, __LINE__ );
   }
   
   return api;
}

/*******************************************************************************
NAME     :  PDComAPI::renewSub

ABSTRACT :  Get data from dataset in schedGrpBuffer into application buffer

RETURNS  :  0 if OK, !=0 if error
            Raises exception in case of error.
*/
int PDComAPI::renewSub(void)
{
	int res;
	
	res = PDComAPI_renewSub(this->handle);

	if isException(res)
	{
		throw PDComAPIException(res, __FILE__, __LINE__ );
	}

	return(res);
}

/*******************************************************************************
NAME     :  PDComAPI::pub

ABSTRACT :  Register publisher of comid

RETURNS  :  -
            Raises exception in case of error.
*/
PDComAPI *PDComAPI::pub(UINT32 schedGrp, UINT32 comId, UINT32 destId, char *dest)
{
   PDComAPI *api = new PDComAPI();
   api->handle = PDComAPI_pub(schedGrp, comId, destId, dest);

   if (api->handle == (PD_HANDLE) NULL)
   {
		throw PDComAPIException((int)IPT_ERROR, __FILE__, __LINE__ );
   }

   return api;
}

/*******************************************************************************
NAME     :  PDComAPI::publish

ABSTRACT :  Register publisher of comid

RETURNS  :  -
            Raises exception in case of error.
*/
PDComAPI *PDComAPI::publish(UINT32 schedGrp, UINT32 comId, char *dest)
{
   PDComAPI *api = new PDComAPI();
   api->handle = PDComAPI_publish(schedGrp, comId, dest);

   if (api->handle == (PD_HANDLE) NULL)
   {
		throw PDComAPIException((int)IPT_ERROR, __FILE__, __LINE__ );
   }

   return api;
}

/*******************************************************************************
NAME     :  PDComAPI::publish

ABSTRACT :  Register publisher of comid defered sending until application
            has called put and source methods 

RETURNS  :  -
            Raises exception in case of error.
*/
PDComAPI *PDComAPI::publishDef(UINT32 schedGrp, UINT32 comId, char *dest)
{
   PDComAPI *api = new PDComAPI();
   api->handle = PDComAPI_pub(schedGrp, comId, 0, dest);

   if (api->handle == (PD_HANDLE) NULL)
   {
		throw PDComAPIException((int)IPT_ERROR, __FILE__, __LINE__ );
   }

   return api;
}

/*******************************************************************************
NAME     :  PDComAPI::renewpub

ABSTRACT :  Get data from dataset in schedGrpBuffer into application buffer

RETURNS  :  0 if OK, !=0 if error
            Raises exception in case of error.
*/
int PDComAPI::renewPub(void)
{
	int res;
	
	res = PDComAPI_renewPub(this->handle);

	if isException(res)
	{
		throw PDComAPIException(res, __FILE__, __LINE__ );
	}

	return(res);
}

/*******************************************************************************
NAME     :  PDComAPI::get

ABSTRACT :  Get data from dataset in schedGrpBuffer into application buffer

RETURNS  :  0 if OK, !=0 if error
            Raises exception in case of error.
*/
int PDComAPI::get(DataSet *pBuffer, UINT16 bufferSize)
{
	int res;
	
	res = PDComAPI_get(this->handle, pBuffer, bufferSize);

	if isException(res)
	{
		throw PDComAPIException(res, __FILE__, __LINE__ );
	}

	return(res);
}

/*******************************************************************************
NAME     :  PDComAPI::get

ABSTRACT :  Get valid status and data from dataset in schedGrpBuffer into
            application buffer 

RETURNS  :  0 if OK, !=0 if error
            Raises exception in case of error.
*/
int PDComAPI::get(DataSet *pBuffer, UINT16 bufferSize, int *pInValid)
{
	int res;
	
	res = PDComAPI_getWStatus(this->handle, pBuffer, bufferSize, pInValid);

	if isException(res)
	{
		throw PDComAPIException(res, __FILE__, __LINE__ );
	}

	return(res);
}

/*******************************************************************************
NAME     :  PDComAPI::put

ABSTRACT :  Put data from application into dataset in schedGrpBuffer

RETURNS  :  -
            Raises exception in case of error.
*/
void PDComAPI::put(DataSet *pBuffer)
{
   PDComAPI_put(this->handle, pBuffer);
}

/*******************************************************************************
NAME     :  PDComAPI::getStatistics

ABSTRACT :  Get statistics for communication

RETURNS  :  0 if OK, !=0 if error
            Raises exception in case of error.
*/
int PDComAPI::getStatistics(BYTE *pBuffer, UINT16 *pBufferSize)
{
	int res;
	
	res = PDComAPI_getStatistics(this->handle, pBuffer, pBufferSize);

	if isException(res)
	{
		throw PDComAPIException(res, __FILE__, __LINE__ );
	}

	return(res);
}

/*******************************************************************************
NAME     :  PDComApi::unsubscribe

ABSTRACT :  Stop subscribing for comid data

RETURNS  :  -
*/
void PDComAPI::unsubscribe(void)
{
   PDComAPI_unsubscribe(&this->handle);
   delete(this);
}

/*******************************************************************************
NAME     :  PDComAPI::unpublish

ABSTRACT :  Unregister publisher of comid.
            Deletes the reference object. No use here after!

RETURNS  :  -
*/
void PDComAPI::unpublish(void)
{
   PDComAPI_unpublish(&this->handle);
   delete(this);
}
