/*                                                                            */
/*  $Id: tdcApi.cpp,v 1.7 2008-07-15 07:35:36 mritz Exp $                     */
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
#include "tdcApiPiq.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


IPTDirClientPiq	tdcPiq;


//-----------------------------------------------------------------------------

const char* IPTDirClientPiq::tdcGetErrorStringPiq (int               errCode,  
                                                   T_IPT_IP_ADDR     baseIpAddr) 
{
   return (::tdcGetErrorStringPiq (errCode, baseIpAddr));
}

//-----------------------------------------------------------------------------

UINT32 IPTDirClientPiq::tdcGetVersionPiq (T_IPT_IP_ADDR       baseIpAddr) 
{
   return (::tdcGetVersionPiq (baseIpAddr));
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetTrnBackboneTypePiq (UINT8*	               pTbType,           /* output */
                                               T_IPT_IP_ADDR*        pGatewayIpAddr,    /* output */
                                               T_IPT_IP_ADDR         baseIpAddr)    
{
   T_TDC_RESULT   tdcResult = ::tdcGetTrnBackboneTypePiq (pTbType, pGatewayIpAddr, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetIptStatePiq (UINT8*              pInaugState, /* output */
                                        UINT8*              pTopoCnt,    /* output */
                                        T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetIptStatePiq (pInaugState, pTopoCnt, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetOwnIdsPiq (T_IPT_LABEL         devId,     /* who am I? */
                                      T_IPT_LABEL         carId,     
                                      T_IPT_LABEL         cstId,  
                                      T_IPT_IP_ADDR       baseIpAddr)     
{
   T_TDC_RESULT   tdcResult = ::tdcGetOwnIdsPiq (devId, carId, cstId, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}
 
//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetAddrByNamePiq (const T_IPT_URI     uri,     
                                          T_IPT_IP_ADDR*      pIpAddr,
                                          UINT8*              pTopoCnt,  
                                          T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetAddrByNamePiq (uri, pIpAddr, pTopoCnt, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetAddrByNameExtPiq (const T_IPT_URI     uri,     
                                             T_IPT_IP_ADDR*      pIpAddr,
                                             T_TDC_BOOL*         pIsFRG,
                                             UINT8*              pTopoCnt,  
                                             T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetAddrByNameExtPiq (uri, pIpAddr, pIsFRG, pTopoCnt, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetUriHostPartPiq (T_IPT_IP_ADDR       ipAddr,   
                                           T_IPT_URI           uri,
                                           UINT8*              pTopoCnt,  
                                           T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetUriHostPartPiq (ipAddr, uri, pTopoCnt,  baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcLabel2CarIdPiq (T_IPT_LABEL         carId,
                                        UINT8*              pTopoCnt,   
                                        const T_IPT_LABEL   cstLabel,
                                        const T_IPT_LABEL   carLabel,  
                                        T_IPT_IP_ADDR       baseIpAddr)
{
   T_TDC_RESULT   tdcResult = ::tdcLabel2CarIdPiq (carId, pTopoCnt, cstLabel, carLabel, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcAddr2CarIdPiq (T_IPT_LABEL         carId,  
                                       UINT8*              pTopoCnt,   
                                       T_IPT_IP_ADDR       ipAddr,  
                                       T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcAddr2CarIdPiq (carId, pTopoCnt, ipAddr, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcLabel2CstIdPiq (T_IPT_LABEL         cstId,    
                                        UINT8*              pTopoCnt,   
                                        const T_IPT_LABEL   carLabel,  
                                        T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcLabel2CstIdPiq (cstId, pTopoCnt, carLabel, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcAddr2CstIdPiq (T_IPT_LABEL         cstId,   
                                       UINT8*              pTopoCnt,   
                                       T_IPT_IP_ADDR       ipAddr,  
                                       T_IPT_IP_ADDR       baseIpAddr)  
{
   T_TDC_RESULT   tdcResult = ::tdcAddr2CstIdPiq (cstId, pTopoCnt, ipAddr, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcCstNo2CstIdPiq (T_IPT_LABEL         cstId,     
                                        UINT8*              pTopoCnt,     
                                        UINT8               trnCstNo,  
                                        T_IPT_IP_ADDR       baseIpAddr)
{
   T_TDC_RESULT   tdcResult = ::tdcCstNo2CstIdPiq (cstId, pTopoCnt, trnCstNo, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcLabel2TrnCstNoPiq (UINT8*              pTrnCstNo,
                                           UINT8*              pTopoCnt,   
                                           const T_IPT_LABEL   carLabel,  
                                           T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcLabel2TrnCstNoPiq (pTrnCstNo, pTopoCnt, carLabel, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcAddr2TrnCstNoPiq (UINT8*              pTrnCstNo, 
                                          UINT8*              pTopoCnt,   
                                          T_IPT_IP_ADDR       ipAddr,  
                                          T_IPT_IP_ADDR       baseIpAddr)    
{
   T_TDC_RESULT   tdcResult = ::tdcAddr2TrnCstNoPiq (pTrnCstNo, pTopoCnt, ipAddr, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetTrnCstCntPiq (UINT8*          pCstCnt, 
                                         UINT8*          pTopoCnt,  
                                         T_IPT_IP_ADDR   baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetTrnCstCntPiq (pCstCnt, pTopoCnt, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetCstCarCntPiq (UINT8*              pCarCnt,  
                                         UINT8*              pTopoCnt,   
                                         const T_IPT_LABEL   cstLabel,  
                                         T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetCstCarCntPiq (pCarCnt, pTopoCnt, cstLabel, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetCarDevCntPiq (UINT16*             pDevCnt,  
                                         UINT8*              pTopoCnt,   
                                         const T_IPT_LABEL   cstLabel, 
                                         const T_IPT_LABEL   carLabel,  
                                         T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetCarDevCntPiq (pDevCnt, pTopoCnt, cstLabel, carLabel, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetCarInfoPiq (T_TDC_CAR_DATA*     pCarData,
                                       UINT8*              pTopoCnt,   
                                       UINT16              maxDev,  
                                       const T_IPT_LABEL   cstLabel,
                                       const T_IPT_LABEL   carLabel,  
                                       T_IPT_IP_ADDR       baseIpAddr)
{
   T_TDC_RESULT   tdcResult = ::tdcGetCarInfoPiq (pCarData, pTopoCnt, maxDev, cstLabel, carLabel, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetUicStatePiq (UINT8*              pInaugState,
                                        UINT8*              pTopoCnt,  
                                        T_IPT_IP_ADDR       baseIpAddr)   
{
   T_TDC_RESULT   tdcResult = ::tdcGetUicStatePiq (pInaugState, pTopoCnt, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetUicGlobalDataPiq (T_TDC_UIC_GLOB_DATA*    pGlobData, 
                                             UINT8*                  pTopoCnt,  
                                             T_IPT_IP_ADDR           baseIpAddr)
{
   T_TDC_RESULT   tdcResult = ::tdcGetUicGlobalDataPiq (pGlobData, pTopoCnt, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcGetUicCarDataPiq (T_TDC_UIC_CAR_DATA*     pCarData, 
                                          UINT8*                  pTopoCnt,   
                                          UINT8                   carSeqNo,  
                                          T_IPT_IP_ADDR           baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcGetUicCarDataPiq (pCarData, pTopoCnt, carSeqNo, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcLabel2UicCarSeqNoPiq (UINT8*              pCarSeqNo,
                                              UINT8*              pIptTopoCnt,   
                                              UINT8*              pUicTopoCnt,   
                                              const T_IPT_LABEL   cstLabel, 
                                              const T_IPT_LABEL   carLabel,  
                                              T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcLabel2UicCarSeqNoPiq (pCarSeqNo, pIptTopoCnt, pUicTopoCnt, cstLabel, carLabel, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcAddr2UicCarSeqNoPiq (UINT8*            pCarSeqNo, 
                                             UINT8*            pIptTopoCnt,   
                                             UINT8*            pUicTopoCnt,   
                                             T_IPT_IP_ADDR     ipAddr,  
                                             T_IPT_IP_ADDR     baseIpAddr)    
{
   T_TDC_RESULT   tdcResult = ::tdcAddr2UicCarSeqNoPiq (pCarSeqNo, pIptTopoCnt, pUicTopoCnt, ipAddr, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------

int IPTDirClientPiq::tdcUicCarSeqNo2IdsPiq (T_IPT_LABEL         cstLabel, 
                                            T_IPT_LABEL         carLabel, 
                                            UINT8*              pIptTopoCnt,   
                                            UINT8*              pUicTopoCnt,   
                                            UINT8               carSeqNo,  
                                            T_IPT_IP_ADDR       baseIpAddr) 
{
   T_TDC_RESULT   tdcResult = ::tdcUicCarSeqNo2IdsPiq (cstLabel, carLabel, pIptTopoCnt, pUicTopoCnt, carSeqNo, baseIpAddr);

   if (tdcResult != TDC_OK)
   {
      // check if exception shall be thrown 
   }

   return (tdcResult);
}

//-----------------------------------------------------------------------------


