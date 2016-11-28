/*                                                                            */
/*  $Id: tdcApiPiq.h 11655 2010-08-23 15:58:44Z bloehr $                   */
/*                                                                            */
/*  DESCRIPTION    Application Interface for the IP-Train Directory Client    */
/*                 (TDC) for position independant queries                     */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS        Either the switch VXWORKS, LINUX or WIN32 has to be set    */
/*                 While the TDC-API defined in tdcApi.h is position-depending*/
/*                 i.e. depending on the device localhost, the API defined    */
/*                 here may be called to deliver results like it is called    */
/*                 from a device with baseIpAddr                              */
/*                                                                            */
/*  DEPENDENCIES   Basic Datatypes have to be defined in advance if LINUX     */
/*                 or WIN32 is chosen.                                        */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002.                  */
/*                                                                            */

/* ----------------------------------------------------------------------------  */
/* ----------------------------------------------------------------------------  */
/* ----------------------------------------------------------------------------  */

#if !defined (_TDC_API_PIQ_H)
   #define _TDC_API_PIQ_H

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++*/
   extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern const char*   tdcGetErrorStringPiq (/*@in@*/  int                errCode, 
                                           /*@in@*/  T_IPT_IP_ADDR      baseIpAddr);
EXP_DLL_DECL 
extern UINT32        tdcGetVersionPiq     (/*@in@*/  T_IPT_IP_ADDR      baseIpAddr);

/* ---------------------------------------------------------------------------- */
/* -----   TDC User API    ---------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetTrnBackboneTypePiq	(/*@out@*/ UINT8*	               pTbType,
                                              /*@out@*/ T_IPT_IP_ADDR*        pGatewayIpAddr,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);
EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetIptStatePiq        (/*@out@*/ UINT8*                pInaugState, 
                                              /*@out@*/ UINT8*                pTopoCnt,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);   
EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetOwnIdsPiq          (/*@out@*/ T_IPT_LABEL           devId,       /* who am I? */
                                              /*@out@*/ T_IPT_LABEL           carId,       
                                              /*@out@*/ T_IPT_LABEL           cstId,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);      

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetAddrByNamePiq      (/*@in@*/  const T_IPT_URI       uri,   
                                             /*@out@*/  T_IPT_IP_ADDR*        pIpAddr,
                                    /*@in@*/ /*@out@*/  UINT8*                pTopoCnt,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetAddrByNameExtPiq   (/*@in@*/  const T_IPT_URI       uri, 
                                              /*@out@*/ T_IPT_IP_ADDR*        pIpAddr,
                                              /*@out@*/ T_TDC_BOOL*           pIsFRG,
                                    /*@in@*/ /*@out@*/  UINT8*                pTopoCnt,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetUriHostPartPiq     (          T_IPT_IP_ADDR         ipAddr,
                                              /*@out@*/ T_IPT_URI             uri,
                                     /*@in@*/ /*@out@*/ UINT8*                pTopoCnt,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);  

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcLabel2CarIdPiq        (/*@out@*/ T_IPT_LABEL           carId,
                                     /*@in@*/ /*@out@*/ UINT8*                pTopoCnt,    
                                              /*@in@*/  const T_IPT_LABEL     cstLabel, 
                                              /*@in@*/  const T_IPT_LABEL     carLabel,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);
EXP_DLL_DECL 
extern T_TDC_RESULT tdcAddr2CarIdPiq         (/*@out@*/ T_IPT_LABEL           carId,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,    
                                              /*@in@*/  T_IPT_IP_ADDR         ipAddr,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);  

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcLabel2CstIdPiq        (/*@out@*/ T_IPT_LABEL           cstId,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,     
                                              /*@in@*/  const T_IPT_LABEL     carLabel,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr); 
EXP_DLL_DECL 
extern T_TDC_RESULT tdcAddr2CstIdPiq         (/*@out@*/ T_IPT_LABEL           cstId,     
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,     
                                              /*@in@*/  T_IPT_IP_ADDR         ipAddr,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);   
EXP_DLL_DECL 
extern T_TDC_RESULT tdcCstNo2CstIdPiq        (/*@out@*/ T_IPT_LABEL           cstId,     
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,     
                                              /*@in@*/  UINT8                 trnCstNo,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);   

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcLabel2TrnCstNoPiq     (/*@out@*/ UINT8*                pTrnCstNo,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt, 
                                              /*@in@*/  const T_IPT_LABEL     carLabel,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr); 
EXP_DLL_DECL 
extern T_TDC_RESULT tdcAddr2TrnCstNoPiq      (/*@out@*/ UINT8*                pTrnCstNo,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt, 
                                              /*@in@*/  T_IPT_IP_ADDR         ipAddr,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);   

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetTrnCstCntPiq       (/*@out@*/ UINT8*                pCstCnt,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr); 

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetCstCarCntPiq       (/*@out@*/ UINT8*                pCarCnt,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,  
                                              /*@in@*/  const T_IPT_LABEL     cstLabel,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);
                                         
EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetCarDevCntPiq       (/*@out@*/ UINT16*               pDevCnt,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,  
                                              /*@in@*/  const T_IPT_LABEL     cstLabel, 
                                              /*@in@*/  const T_IPT_LABEL     carLabel,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetCarInfoPiq         (/*@out@*/ T_TDC_CAR_DATA*       pCarData,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,  
                                              /*@in@*/  UINT16                maxDev,    
                                              /*@in@*/  const T_IPT_LABEL     cstLabel,  
                                              /*@in@*/  const T_IPT_LABEL     carLabel,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr); 

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetUicStatePiq        (/*@out@*/ UINT8*                pInaugState,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr); 

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetUicGlobalDataPiq   (/*@out@*/ T_TDC_UIC_GLOB_DATA*  pGlobData,
                                     /*@in@*/ /*@out@*/ UINT8*                pTopoCnt,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcGetUicCarDataPiq      (/*@out@*/ T_TDC_UIC_CAR_DATA*   pCarData,
                                    /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,  
                                              /*@in@*/     UINT8              carSeqNo,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr); 

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcLabel2UicCarSeqNoPiq  (/*@out@*/ UINT8*                pCarSeqNo,
                                   /*@in@*/   /*@out@*/ UINT8*                pIptTopoCnt,
                                   /*@in@*/   /*@out@*/ UINT8*                pUicTopoCnt, 
                                              /*@in@*/  const T_IPT_LABEL     cstLabel,  
                                              /*@in@*/  const T_IPT_LABEL     carLabel,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr); 
EXP_DLL_DECL 
extern T_TDC_RESULT tdcAddr2UicCarSeqNoPiq   (/*@out@*/ UINT8*                pCarSeqNo, 
                                   /*@in@*/   /*@out@*/ UINT8*                pIptTopoCnt,
                                   /*@in@*/   /*@out@*/ UINT8*                pUicTopoCnt, 
                                              /*@in@*/  T_IPT_IP_ADDR         ipAddr,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr);   

/* ---------------------------------------------------------------------------- */

EXP_DLL_DECL 
extern T_TDC_RESULT tdcUicCarSeqNo2IdsPiq    (/*@out@*/ T_IPT_LABEL           cstId,  
                                              /*@out@*/ T_IPT_LABEL           carId,  
                                    /*@in@*/  /*@out@*/ UINT8*                pIptTopoCnt,
                                    /*@in@*/  /*@out@*/ UINT8*                pUicTopoCnt, 
                                              /*@in@*/  UINT8                 carSeqNo,
                                              /*@in@*/  T_IPT_IP_ADDR         baseIpAddr); 

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif


#ifdef __cplusplus
  
 /********************************************************************************
  *  CLASSES (C++)
  *******************************************************************************/

class   EXP_DLL_DECL    IPTDirClientPiq
{
public:

   static int tdcGetTrnBackboneTypePiq	(UINT8*	            pTbType,
                                        T_IPT_IP_ADDR*      pGatewayIpAddr,
                                        T_IPT_IP_ADDR       baseIpAddr);

   static int tdcGetIptStatePiq        (UINT8*              pInaugState, 
                                        UINT8*              pTopoCnt,
                                        T_IPT_IP_ADDR       baseIpAddr);   
   static int tdcGetOwnIdsPiq          (T_IPT_LABEL         devId,            /* who am I? */
                                        T_IPT_LABEL         carId,       
                                        T_IPT_LABEL         cstId,
                                        T_IPT_IP_ADDR       baseIpAddr);

   /* ---------------------------------------------------------------------------- */

   static int tdcGetAddrByNamePiq      (const T_IPT_URI     uri,        
                                        T_IPT_IP_ADDR*      pIpAddr,
                                        UINT8*              pTopoCnt,
                                        T_IPT_IP_ADDR       baseIpAddr);   
   static int tdcGetAddrByNameExtPiq   (const T_IPT_URI     uri, 
                                        T_IPT_IP_ADDR*      pIpAddr,
                                        T_TDC_BOOL*         pIsFRG,
                                        UINT8*              pTopoCnt,
                                        T_IPT_IP_ADDR       baseIpAddr);
   static int tdcGetUriHostPartPiq     (T_IPT_IP_ADDR       ipAddr,     
                                        T_IPT_URI           uri,
                                        UINT8*              pTopoCnt,
                                        T_IPT_IP_ADDR       baseIpAddr);       

   /* ---------------------------------------------------------------------------- */

   static int tdcLabel2CarIdPiq        (T_IPT_LABEL         carId,
                                        UINT8*              pTopoCnt,      
                                        const T_IPT_LABEL   cstLabel,   
                                        const T_IPT_LABEL   carLabel,
                                        T_IPT_IP_ADDR       baseIpAddr);  
   static int tdcAddr2CarIdPiq         (T_IPT_LABEL         carId,      
                                        UINT8*              pTopoCnt,      
                                        T_IPT_IP_ADDR       ipAddr,
                                        T_IPT_IP_ADDR       baseIpAddr);    

   /* ---------------------------------------------------------------------------- */

   static int tdcLabel2CstIdPiq        (T_IPT_LABEL         cstId,      
                                        UINT8*              pTopoCnt,      
                                        const T_IPT_LABEL   carLabel,
                                        T_IPT_IP_ADDR       baseIpAddr);  
   static int tdcAddr2CstIdPiq         (T_IPT_LABEL         cstId,      
                                        UINT8*              pTopoCnt,      
                                        T_IPT_IP_ADDR       ipAddr,
                                        T_IPT_IP_ADDR       baseIpAddr);    
   static int tdcCstNo2CstIdPiq        (T_IPT_LABEL         cstId,     
                                        UINT8*              pTopoCnt,     
                                        UINT8               trnCstNo,
                                        T_IPT_IP_ADDR       baseIpAddr);   

   /* ---------------------------------------------------------------------------- */

   static int tdcLabel2TrnCstNoPiq     (UINT8*              pTrnCstNo,  
                                        UINT8*              pTopoCnt,      
                                        const T_IPT_LABEL   carLabel,
                                        T_IPT_IP_ADDR       baseIpAddr);  
   static int tdcAddr2TrnCstNoPiq      (UINT8*              pTrnCstNo,  
                                        UINT8*              pTopoCnt,      
                                        T_IPT_IP_ADDR       ipAddr,
                                        T_IPT_IP_ADDR       baseIpAddr);    

   /* ---------------------------------------------------------------------------- */

   static int tdcGetTrnCstCntPiq       (UINT8*              pCstCnt,
                                        UINT8*              pTopoCnt,
                                        T_IPT_IP_ADDR       baseIpAddr);    

   static int tdcGetCstCarCntPiq       (UINT8*              pCarCnt,
                                        UINT8*              pTopoCnt,     
                                        const T_IPT_LABEL   cstLabel,
                                        T_IPT_IP_ADDR       baseIpAddr);   

   static int tdcGetCarDevCntPiq       (UINT16*             pDevCnt,
                                        UINT8*              pTopoCnt,     
                                        const T_IPT_LABEL   cstLabel,    
                                        const T_IPT_LABEL   carLabel,
                                        T_IPT_IP_ADDR       baseIpAddr);   

   /* ---------------------------------------------------------------------------- */

   static int tdcGetCarInfoPiq         (T_TDC_CAR_DATA*     pCarData,
                                        UINT8*              pTopoCnt,    
                                        UINT16              maxDev,      
                                        const T_IPT_LABEL   cstLabel,    
                                        const T_IPT_LABEL   carLabel,
                                        T_IPT_IP_ADDR       baseIpAddr);   

   /* ---------------------------------------------------------------------------- */

   static int tdcGetUicStatePiq        (UINT8*              pInaugState, 
                                        UINT8*              pTopoCnt,
                                        T_IPT_IP_ADDR       baseIpAddr);   

   /* ---------------------------------------------------------------------------- */

   static int tdcGetUicGlobalDataPiq   (T_TDC_UIC_GLOB_DATA*   pGlobData,
                                        UINT8*                 pTopoCnt,
                                        T_IPT_IP_ADDR          baseIpAddr); 

   /* ---------------------------------------------------------------------------- */

   static int tdcGetUicCarDataPiq      (T_TDC_UIC_CAR_DATA*    pCarData,
                                        UINT8*                 pTopoCnt,   
                                        UINT8                  carSeqNo,
                                        T_IPT_IP_ADDR          baseIpAddr);  

   /* ---------------------------------------------------------------------------- */

   static int tdcLabel2UicCarSeqNoPiq  (UINT8*              pCarSeqNo,  
                                        UINT8*              pIptTopoCnt,
                                        UINT8*              pUicTopoCnt, 
                                        const T_IPT_LABEL   cstLabel,   
                                        const T_IPT_LABEL   carLabel,
                                        T_IPT_IP_ADDR       baseIpAddr);  
   static int tdcAddr2UicCarSeqNoPiq   (UINT8*              pCarSeqNo,  
                                        UINT8*              pIptTopoCnt,
                                        UINT8*              pUicTopoCnt, 
                                        T_IPT_IP_ADDR       ipAddr,
                                        T_IPT_IP_ADDR       baseIpAddr);    

   /* ---------------------------------------------------------------------------- */

   static int tdcUicCarSeqNo2IdsPiq    (T_IPT_LABEL         cstLabel,   
                                        T_IPT_LABEL         carLabel,   
                                        UINT8*              pIptTopoCnt,
                                        UINT8*              pUicTopoCnt, 
                                        UINT8               carSeqNo,
                                        T_IPT_IP_ADDR       baseIpAddr);  

   /* ---------------------------------------------------------------------------- */
   /* 2008/07/11, MRi - added missing prototype for C++ Wrapper API */

   static const char*   tdcGetErrorStringPiq (int                 errCode,
                                              T_IPT_IP_ADDR       baseIpAddr);
   static UINT32        tdcGetVersionPiq     (T_IPT_IP_ADDR       baseIpAddr);

};

extern IPTDirClientPiq				tdcPiq;

#endif



#endif



