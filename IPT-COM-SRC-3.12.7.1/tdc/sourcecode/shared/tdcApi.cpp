/*                                                                            */
/*  $Id: tdcApi.cpp 11648 2010-08-20 15:33:06Z bloehr $                     */
/*                                                                            */
/*  DESCRIPTION    TDC API wrapper for C++ (use by CM)						  */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS                                                                   */
/*                                                                            */
/*  DEPENDENCIES   Either the switch LINUX or WIN32 has to be set	          */
/*                                                                            */
/*  MODIFICATIONS (log starts 2010-08-11):									  */
/*   																		  */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002-2010.             */
/*                                                                            */


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

#include "tdc.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


IPTDirClient	tdc;


//-----------------------------------------------------------------------------

const char* IPTDirClient::tdcGetErrorString (int     errCode) 
{
   return (::tdcGetErrorString (errCode));
}

//-----------------------------------------------------------------------------

UINT32 IPTDirClient::tdcGetVersion (void) 
{
   return (::tdcGetVersion ());
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetTrnBackboneType (UINT8*	               pTbType,           /* output */
                                         T_IPT_IP_ADDR*        pGatewayIpAddr)    /* output */
{
   T_TDC_RESULT   tdcResult = ::tdcGetTrnBackboneType (pTbType, pGatewayIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetIptState (UINT8*        pInaugState, /* output */
                                  UINT8*        pTopoCnt)    /* output */
{
   T_TDC_RESULT   tdcResult = ::tdcGetIptState (pInaugState, pTopoCnt);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetOwnIds (T_IPT_LABEL         devId,     /* who am I? */
                                T_IPT_LABEL         carId,     
                                T_IPT_LABEL         cstId)     
{
   T_TDC_RESULT   tdcResult = ::tdcGetOwnIds (devId, carId, cstId);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}
 
//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetAddrByName (const T_IPT_URI     uri,     
                                     T_IPT_IP_ADDR*      pIpAddr,
                                     UINT8*              pTopoCnt) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetAddrByName (uri, pIpAddr, pTopoCnt);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetAddrByNameExt (const T_IPT_URI     uri,     
                                       T_IPT_IP_ADDR*      pIpAddr,
                                       T_TDC_BOOL*         pIsFRG,
                                       UINT8*              pTopoCnt) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetAddrByNameExt (uri, pIpAddr, pIsFRG, pTopoCnt);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetUriHostPart (T_IPT_IP_ADDR       ipAddr,   
                                      T_IPT_URI           uri,
                                      UINT8*              pTopoCnt) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetUriHostPart (ipAddr, uri, pTopoCnt);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcLabel2CarId (T_IPT_LABEL         carId,
                                   UINT8*              pTopoCnt,   
                                   const T_IPT_LABEL   cstLabel,
                                   const T_IPT_LABEL   carLabel)
{
   T_TDC_RESULT   tdcResult = ::tdcLabel2CarId (carId, pTopoCnt, cstLabel, carLabel);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcAddr2CarId (T_IPT_LABEL         carId,  
                                  UINT8*              pTopoCnt,   
                                  T_IPT_IP_ADDR       ipAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcAddr2CarId (carId, pTopoCnt, ipAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcLabel2CstId (T_IPT_LABEL         cstId,    
                                   UINT8*              pTopoCnt,   
                                   const T_IPT_LABEL   carLabel) 
{
   T_TDC_RESULT   tdcResult = ::tdcLabel2CstId (cstId, pTopoCnt, carLabel);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcAddr2CstId (T_IPT_LABEL         cstId,   
                                 UINT8*              pTopoCnt,   
                                 T_IPT_IP_ADDR       ipAddr)  
{
   T_TDC_RESULT   tdcResult = ::tdcAddr2CstId (cstId, pTopoCnt, ipAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcCstNo2CstId (T_IPT_LABEL         cstId,     
                                  UINT8*              pTopoCnt,     
                                  UINT8               trnCstNo)
{
   T_TDC_RESULT   tdcResult = ::tdcCstNo2CstId (cstId, pTopoCnt, trnCstNo);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcLabel2TrnCstNo (UINT8*              pTrnCstNo,
                                      UINT8*              pTopoCnt,   
                                      const T_IPT_LABEL   carLabel) 
{
   T_TDC_RESULT   tdcResult = ::tdcLabel2TrnCstNo (pTrnCstNo, pTopoCnt, carLabel);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcAddr2TrnCstNo (UINT8*              pTrnCstNo, 
                                     UINT8*              pTopoCnt,   
                                     T_IPT_IP_ADDR       ipAddr)    
{
   T_TDC_RESULT   tdcResult = ::tdcAddr2TrnCstNo (pTrnCstNo, pTopoCnt, ipAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetTrnCstCnt (UINT8*         pCstCnt, 
                                    UINT8*         pTopoCnt) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetTrnCstCnt (pCstCnt, pTopoCnt);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetCstCarCnt (UINT8*              pCarCnt,  
                                    UINT8*              pTopoCnt,   
                                    const T_IPT_LABEL   cstLabel) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetCstCarCnt (pCarCnt, pTopoCnt, cstLabel);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetCarDevCnt (UINT16*             pDevCnt,  
                                    UINT8*              pTopoCnt,   
                                    const T_IPT_LABEL   cstLabel, 
                                    const T_IPT_LABEL   carLabel) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetCarDevCnt (pDevCnt, pTopoCnt, cstLabel, carLabel);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetCarInfo (T_TDC_CAR_DATA*     pCarData,
                                  UINT8*              pTopoCnt,   
                                  UINT16              maxDev,  
                                  const T_IPT_LABEL   cstLabel,
                                  const T_IPT_LABEL   carLabel)
{
   T_TDC_RESULT   tdcResult = ::tdcGetCarInfo (pCarData, pTopoCnt, maxDev, cstLabel, carLabel);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetUicState (UINT8*              pInaugState,
                                   UINT8*              pTopoCnt)   
{
   T_TDC_RESULT   tdcResult = ::tdcGetUicState (pInaugState, pTopoCnt);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetUicGlobalData (T_TDC_UIC_GLOB_DATA*   pGlobData, 
                                        UINT8*                 pTopoCnt)
{
   T_TDC_RESULT   tdcResult = ::tdcGetUicGlobalData (pGlobData, pTopoCnt);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcGetUicCarData (T_TDC_UIC_CAR_DATA*    pCarData, 
                                     UINT8*                 pTopoCnt,   
                                     UINT8                  carSeqNo) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetUicCarData (pCarData, pTopoCnt, carSeqNo);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcLabel2UicCarSeqNo (UINT8*              pCarSeqNo,
                                         UINT8*              pIptTopoCnt,   
                                         UINT8*              pUicTopoCnt,   
                                         const T_IPT_LABEL   cstLabel, 
                                         const T_IPT_LABEL   carLabel) 
{
   T_TDC_RESULT   tdcResult = ::tdcLabel2UicCarSeqNo (pCarSeqNo, pIptTopoCnt, pUicTopoCnt, cstLabel, carLabel);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcAddr2UicCarSeqNo (UINT8*              pCarSeqNo, 
                                         UINT8*             pIptTopoCnt,   
                                         UINT8*             pUicTopoCnt,   
                                        T_IPT_IP_ADDR       ipAddr)    
{
   T_TDC_RESULT   tdcResult = ::tdcAddr2UicCarSeqNo (pCarSeqNo, pIptTopoCnt, pUicTopoCnt, ipAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClient::tdcUicCarSeqNo2Ids (T_IPT_LABEL         cstLabel, 
                                       T_IPT_LABEL         carLabel, 
                                       UINT8*              pIptTopoCnt,   
                                       UINT8*              pUicTopoCnt,   
                                       UINT8               carSeqNo) 
{
   T_TDC_RESULT   tdcResult = ::tdcUicCarSeqNo2Ids (cstLabel, carLabel, pIptTopoCnt, pUicTopoCnt, carSeqNo);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------


