/*                                                                            */
/*  $Id: tdcApi.h 36621 2015-04-23 13:47:21Z rscheja $                      */
/*                                                                            */
/*  DESCRIPTION    Application Interface for the IP-Train Directory Client    */
/*                 (TDC)                                                      */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS        Either the switch VXWORKS, LINUX or WIN32 has to be set    */
/*                                                                            */
/*  DEPENDENCIES   Basic Datatypes have to be defined in advance if LINUX     */
/*                 or WIN32 is chosen.                                        */
/*                                                                            */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002.                  */
/*                                                                            */

/* ----------------------------------------------------------------------------  */
/* ----------------------------------------------------------------------------  */
/* ----------------------------------------------------------------------------  */

#if !defined (_TDC_API_H)
   #define _TDC_API_H

/* ------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++*/
   extern "C" {
#endif

/* -------------------------------------------------------------------------- */

#include "tdcSyl.h"

/* -------------------------------------------------------------------------- */

/* Unicast address definition */
#define UC_ADDR(protType, topoCnt, unitNo, hostId)                                   \
                                                   (   (((unsigned) (protType & 0xFF))  << 24)     \
                                                     + (((unsigned) (topoCnt  & 0x3F))  << 18)     \
                                                     + (((unsigned) (unitNo   & 0x3F))  << 12)     \
                                                     +  ((unsigned) (hostId   & 0xFFF))            \
                                                   )
/* Multicast address definition */
#define MC_ADDR(protType, topoCnt, unitNo, grpNo)                                                  \
                                                   (   (((unsigned) (protType & 0xFF))  << 24)     \
                                                     + (((unsigned) (topoCnt  & 0x3F))  << 18)     \
                                                     + (((unsigned) (unitNo   & 0x3F))  << 12)     \
                                                     +  ((unsigned) (grpNo    & 0xFFF))            \
                                                   )   
                                                       
#define isIpAddrUC(ipAddr)                         (ipAddrGetProtType (ipAddr) == 10)
#define isIpAddrMC(ipAddr)                         (ipAddrGetProtType (ipAddr) == 239)

#define ipAddrGetProtType(ipAddr)                  ((ipAddr >> 24) & 0xFF)
#define ipAddrGetTopoCnt(ipAddr)                   ((ipAddr >> 18) & 0x3F)
#define ipAddrGetUnitNo(ipAddr)                    ((ipAddr >> 12) & 0x3F)       /* != 0, if addr outside local consist */
#define ipAddrGetHostId(ipAddr)                    ((ipAddr >> 0)  & 0xFFF)  

#define ucAddrGetHostId(ipAddr)                    ((ipAddr >> 0)  & 0xFFF)
#define ucAddrGetDevNo(ipAddr)                     ucAddrGetHostId (ipAddr)
#define ucAddrGetUnitNo(ipAddr)                    ipAddrGetUnitNo  (ipAddr)
#define ucAddrGetTopoCnt(ipAddr)                   ipAddrGetTopoCnt (ipAddr)

#define mcAddrGetGrpNo(ipAddr)                     ((ipAddr >> 0)  & 0xFFF)
#define mcAddrGetUnitNo(ipAddr)                    ipAddrGetUnitNo  (ipAddr)
#define mcAddrGetTopoCnt(ipAddr)                   ipAddrGetTopoCnt (ipAddr)

/* ------------------------------------------------------------------------- */

typedef int                               T_TDC_BOOL;

/* --------------------   TDC Result   -------------------- */

typedef INT32                             T_TDC_RESULT;

/* ------------------------------------------------------------------------- */

#define IPT_ERR_TYPE_TDC                  ((UINT32) 2)     /* ERR */
#define IPT_INST_TDC                      ((UINT32) 1)
#define IPT_FUNC_TDC                      ((UINT32) 4)
#define IPT_GLOBFLAG_TDC                  ((UINT32) 1)

#define MK_RESULT_CODE(type, inst, func, glob, sub)               \
                                     (    ((type)   << 30)        \
                                       |  ((inst)   << 24)        \
                                       |  ((func)   << 16)        \
                                       |  ((glob)   << 15)        \
                                       |  (sub)                   \
                                     )

#define MK_TDC_RESULT_CODE(subCode)       MK_RESULT_CODE (IPT_ERR_TYPE_TDC,   \
                                                          IPT_INST_TDC,       \
                                                          IPT_FUNC_TDC,       \
                                                          IPT_GLOBFLAG_TDC,   \
                                                          subCode)

#define TDC_OK                            ((T_TDC_RESULT) 0)
#define TDC_UNSUPPORTED_REQU              ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x0001))
#define TDC_UNKNOWN_URI                   ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x0002))
#define TDC_UNKNOWN_IPADDR                ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x0003))
#define TDC_NO_CONFIG_DATA                ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x0004))
#define TDC_NULL_POINTER_ERROR            ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x0005))
#define TDC_NOT_ENOUGH_MEMORY             ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x0006))
#define TDC_NO_MATCHING_ENTRY             ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x0007))
#define TDC_MUST_FINISH_INIT              ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x0008))
#define TDC_INVALID_LABEL                 ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x0009))
#define TDC_WRONG_TOPOCOUNT               ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x000A))

#define TDC_ERROR                         ((T_TDC_RESULT) MK_TDC_RESULT_CODE (0x03E8))       /* 0x03E8 == 1000 */

/* ---------------------------------------------------------------------------- */

#define ENV_TDC_SERVICE_IP_ADDR           "TDC_SERVICE_IP_ADDR"

/* ---------------------------------------------------------------------------- */
/* -----    TDC System API    ------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

extern int           tdcPrepareInit    (int startupMode, int RAMaddr);
extern T_TDC_RESULT  tdcTerminate      (void);
extern T_TDC_RESULT  tdcDestroy        (void);
extern EXP_DLL_DECL const char*   tdcGetErrorString (int     errCode);
extern UINT32        tdcGetVersion     (void);

/* ---------------------------------------------------------------------------- */
/* -----   TDC User API    ---------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

#define TDC_IPT_TBTYPE_ETB                      ((UINT8) 0)
#define TDC_IPT_TBTYPE_WTB                      ((UINT8) 1)
#define TDC_IPT_TBTYPE_UNKNOWN                  MAX_UINT8

#define TDC_IPT_INAUGSTATE_FAULT                ((UINT8) 0)
#define TDC_IPT_INAUGSTATE_INVALID              ((UINT8) 1)
#define TDC_IPT_INAUGSTATE_OK                   ((UINT8) 2)
#define TDC_IPT_INAUGSTATE_LOCAL                ((UINT8) 3)

extern T_TDC_RESULT EXP_DLL_DECL tdcGetTrnBackboneType	(/*@out@*/ UINT8*	            pTbType,
                                           /*@out@*/ T_IPT_IP_ADDR*     pGatewayIpAddr);
extern T_TDC_RESULT EXP_DLL_DECL tdcGetIptState     ( /*@out@*/ UINT8*               pInaugState, 
                                         /*@out@*/ UINT8*               pTopoCnt);   
extern T_TDC_RESULT EXP_DLL_DECL  tdcGetOwnIds       ( /*@out@*/ T_IPT_LABEL          devId,       /* who am I? */
                                         /*@out@*/ T_IPT_LABEL          carId,       
                                         /*@out@*/ T_IPT_LABEL          cstId);      

/* ---------------------------------------------------------------------------- */

extern T_TDC_RESULT EXP_DLL_DECL tdcGetAddrByName   ( /*@in@*/  const T_IPT_URI      uri,   
                                         /*@out@*/ T_IPT_IP_ADDR*       pIpAddr,
                                /*@in@*/ /*@out@*/ UINT8*               pTopoCnt);

extern T_TDC_RESULT EXP_DLL_DECL tdcGetAddrByNameExt (/*@in@*/  const T_IPT_URI      uri, 
                                         /*@out@*/ T_IPT_IP_ADDR*       pIpAddr,
                                         /*@out@*/ T_TDC_BOOL*          pIsFRG,
                                /*@in@*/ /*@out@*/ UINT8*               pTopoCnt);

extern T_TDC_RESULT EXP_DLL_DECL tdcGetUriHostPart  (           T_IPT_IP_ADDR        ipAddr,
                                         /*@out@*/ T_IPT_URI            uri,
                                /*@in@*/ /*@out@*/ UINT8*               pTopoCnt);  

/* ---------------------------------------------------------------------------- */

extern T_TDC_RESULT EXP_DLL_DECL tdcLabel2CarId     ( /*@out@*/ T_IPT_LABEL          carId,
                                /*@in@*/ /*@out@*/ UINT8*               pTopoCnt,    
                                         /*@in@*/  const T_IPT_LABEL    cstLabel, 
                                         /*@in@*/  const T_IPT_LABEL    carLabel);
extern T_TDC_RESULT EXP_DLL_DECL tdcAddr2CarId      ( /*@out@*/ T_IPT_LABEL          carId,
                               /*@in@*/  /*@out@*/ UINT8*               pTopoCnt,    
                                                   T_IPT_IP_ADDR        ipAddr);  

/* ---------------------------------------------------------------------------- */

extern T_TDC_RESULT EXP_DLL_DECL tdcLabel2CstId     ( /*@out@*/ T_IPT_LABEL          cstId,
                               /*@in@*/  /*@out@*/ UINT8*               pTopoCnt,     
                                         /*@in@*/  const T_IPT_LABEL    carLabel); 
extern T_TDC_RESULT EXP_DLL_DECL tdcAddr2CstId      ( /*@out@*/ T_IPT_LABEL          cstId,     
                               /*@in@*/  /*@out@*/ UINT8*               pTopoCnt,     
                                                   T_IPT_IP_ADDR        ipAddr);   
extern T_TDC_RESULT EXP_DLL_DECL tdcCstNo2CstId     ( /*@out@*/ T_IPT_LABEL          cstId,     
                               /*@in@*/  /*@out@*/ UINT8*               pTopoCnt,     
                                                   UINT8                trnCstNo);   

/* ---------------------------------------------------------------------------- */

extern T_TDC_RESULT EXP_DLL_DECL tdcLabel2TrnCstNo  ( /*@out@*/ UINT8*               pTrnCstNo,
                               /*@in@*/  /*@out@*/ UINT8*               pTopoCnt, 
                                         /*@in@*/  const T_IPT_LABEL    carLabel); 
extern T_TDC_RESULT EXP_DLL_DECL tdcAddr2TrnCstNo   ( /*@out@*/ UINT8*               pTrnCstNo,
                               /*@in@*/  /*@out@*/ UINT8*               pTopoCnt, 
                                                   T_IPT_IP_ADDR        ipAddr);   

/* ---------------------------------------------------------------------------- */

extern T_TDC_RESULT EXP_DLL_DECL tdcGetTrnCstCnt    ( /*@out@*/ UINT8*               pCstCnt,
                               /*@in@*/  /*@out@*/ UINT8*               pTopoCnt); 

extern T_TDC_RESULT EXP_DLL_DECL tdcGetCstCarCnt    ( /*@out@*/ UINT8*               pCarCnt,
                               /*@in@*/  /*@out@*/ UINT8*               pTopoCnt,  
                                         /*@in@*/  const T_IPT_LABEL    cstLabel);
                                         
extern T_TDC_RESULT EXP_DLL_DECL tdcGetCarDevCnt    ( /*@out@*/ UINT16*              pDevCnt,
                               /*@in@*/  /*@out@*/ UINT8*               pTopoCnt,  
                                         /*@in@*/  const T_IPT_LABEL    cstLabel, 
                                         /*@in@*/  const T_IPT_LABEL    carLabel);

/* ---------------------------------------------------------------------------- */

typedef struct
{
   UINT16            hostId;        /* corresponds to host in IP-Addr */
   T_IPT_LABEL       devId;         /* device identifier (Label) */
} T_TDC_DEV_DATA;

typedef struct
{
   T_IPT_LABEL       carId;         /* the car identifier */
   T_IPT_LABEL       carType;       /* the car type */
   T_UIC_IDENT       uicIdent;      /* UIC identification number */
   UINT8             cstCarNo;      /* sequence number of car in consist */
   UINT8             trnOrient;     /* opposite(0) or same(1) orientation rel. to train */
   UINT8             cstOrient;     /* opposite(0) or same(1) orientation rel. to consist */
   UINT16            devCnt;        /* number of devices in the car */
   T_TDC_DEV_DATA    devData[1];    /* device data list. The list size '1' is just a proxy */
                                    /* definition for the real size (devCnt) in order to */
                                    /* satisfy C-Language */
} T_TDC_CAR_DATA;
#define TDC_DEV_DATA_SIZE(cnt)               ((UINT32) (cnt * sizeof (T_TDC_DEV_DATA)))
#define TDC_CAR_DATA_DEV_OFFSET()            ARRAY_OFFSET_IN_STRUC (T_TDC_CAR_DATA, devData)
#define TDC_CAR_DATA_SIZE(cnt)               (TDC_CAR_DATA_DEV_OFFSET () + TDC_DEV_DATA_SIZE (cnt))   


extern T_TDC_RESULT EXP_DLL_DECL tdcGetCarInfo      ( /*@out@*/ T_TDC_CAR_DATA*     pCarData,
                               /*@in@*/  /*@out@*/ UINT8*              pTopoCnt,  
                                                   UINT16              maxDev,    
                                         /*@in@*/  const T_IPT_LABEL   cstLabel,  
                                         /*@in@*/  const T_IPT_LABEL   carLabel); 

/* ---------------------------------------------------------------------------- */

#define TDC_UIC_INAUGSTATE_ACTUAL               ((UINT8) 0)
#define TDC_UIC_INAUGSTATE_CONFIRMED            ((UINT8) 1)
#define TDC_UIC_INAUGSTATE_INVALID              ((UINT8) 2)

extern T_TDC_RESULT EXP_DLL_DECL tdcGetUicState     ( /*@out@*/ UINT8*              pInaugState,
                               /*@in@*/  /*@out@*/ UINT8*              pTopoCnt); 

/* ---------------------------------------------------------------------------- */

typedef struct
{
   UINT32            trnCarCnt;                       /* Total number of UIC cars */
   UINT8             confPos[IPT_UIC_CONF_POS_CNT];   /* confirmed position of unreachable cars */
   UINT8             confPosAvail;                    /* 0 if conf. Position is not available */
   UINT8             operatAvail;                     /* 0 if operator/owner is not available */
   UINT8             natApplAvail;                    /* 0 if national Application/Version is not available */
   UINT8             cstPropAvail;                    /* 0 if UIC consist properties are not available */
   UINT8             carPropAvail;                    /* 0 if UIC car properties are not available */
   UINT8             seatResNoAvail;                  /* 0 if if reservation number is not available */
   UINT8             inaugFrameVer;                   /* inauguration frame version, s. Leaflet 556 Ann. C.3 */
   UINT8             rDataVer;                        /* supported R-Telegram Version, s. Leaflet 556 Ann. A */
   UINT8             inaugState;                      /* UIC inaugaration state */
   UINT8             topoCnt;                         /* UIC (i.e. TCN) topography counter */
   UINT8             orient;                          /* 0 if UIC reference orientation is opposite to IPT */
   UINT8             notAllConf;                      /* 0 if feature is not available */
   UINT8             confCancelled;                   /* 0 if feature is not available */
} T_TDC_UIC_GLOB_DATA;

extern T_TDC_RESULT EXP_DLL_DECL tdcGetUicGlobalData   ( /*@out@*/ T_TDC_UIC_GLOB_DATA*    pGlobData,
                                   /*@in@*/ /*@out@*/ UINT8*                  pTopoCnt);

/* ---------------------------------------------------------------------------- */

typedef struct
{
   UINT8             cstProp[IPT_MAX_UIC_CST_NO];        /* consist properties*/
   UINT8             carProp[IPT_UIC_CAR_PROPERTY_CNT];  /* car properties */
   UINT8             uicIdent[IPT_UIC_IDENTIFIER_CNT];   /* UIC identification number */
   UINT8             cstSeqNo;                           /* consist sequence number in UIC Train */
   UINT8             carSeqNo;                           /* car sequence number in UIC ref direction */
   UINT16            seatResNo;                          /* car number for seat reservation */
   INT8              contrCarCnt;                        /* total number of controlled cars in consist */
   UINT8             operat;                             /* consist operator type (s. UIC 556) */
   UINT8             owner;                              /* consist owner type (s. UIC 556) */
   UINT8             natAppl;                            /* national application type (s. UIC 556) */
   UINT8             natVer;                             /* national application version (s. UIC 556) */
   UINT8             trnOrient;                          /* 0 if car orientation is opposite to Train */
   UINT8             cstOrient;                          /* 0 if car orientation is opposite to Consist */
   UINT8             isLeading;                          /* 0 if car is not leading */
   UINT8             isLeadRequ;                         /* 0 if no leading request */
   UINT8             trnSwInCarCnt;                      /* total number of train switches in car */
} T_TDC_UIC_CAR_DATA;

extern T_TDC_RESULT EXP_DLL_DECL tdcGetUicCarData   ( /*@out@*/ T_TDC_UIC_CAR_DATA*   pCarData,
                               /*@in@*/  /*@out@*/ UINT8*                pTopoCnt,  
                                                   UINT8                 carSeqNo); 

/* ---------------------------------------------------------------------------- */

extern T_TDC_RESULT EXP_DLL_DECL tdcLabel2UicCarSeqNo (/*@out@*/ UINT8*              pCarSeqNo,
                               /*@in@*/   /*@out@*/ UINT8*              pIptTopoCnt,
                               /*@in@*/   /*@out@*/ UINT8*              pUicTopoCnt, 
                                          /*@in@*/  const T_IPT_LABEL   cstLabel,  
                                          /*@in@*/  const T_IPT_LABEL   carLabel); 
extern T_TDC_RESULT EXP_DLL_DECL tdcAddr2UicCarSeqNo  (/*@out@*/ UINT8*              pCarSeqNo, 
                               /*@in@*/   /*@out@*/ UINT8*              pIptTopoCnt,
                               /*@in@*/   /*@out@*/ UINT8*              pUicTopoCnt, 
                                                    T_IPT_IP_ADDR       ipAddr);   

/* ---------------------------------------------------------------------------- */

extern T_TDC_RESULT EXP_DLL_DECL tdcUicCarSeqNo2Ids ( /*@out@*/ T_IPT_LABEL          cstId,  
                                         /*@out@*/ T_IPT_LABEL          carId,  
                               /*@in@*/  /*@out@*/ UINT8*               pIptTopoCnt,
                               /*@in@*/  /*@out@*/ UINT8*               pUicTopoCnt, 
                                                   UINT8                carSeqNo); 

/* ---------------------------------------------------------------------------- */

/* callable from Library only (when using IPC) */
/* use tdcSetDebugLevel directly when in same addresspace! */
extern T_TDC_RESULT _tdcSetDebugLevel  (/*@in@*/  const char*    pPar0, 
                                        /*@in@*/  const char*    pPar1, 
                                        /*@in@*/  const char*    pPar2);

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

class   EXP_DLL_DECL    IPTDirClient
{
public:

   static int tdcGetTrnBackboneType	(UINT8*	            pTbType,
                                     T_IPT_IP_ADDR*      pGatewayIpAddr);

   static int tdcGetIptState        (UINT8*              pInaugState, 
                                     UINT8*              pTopoCnt);   
   static int tdcGetOwnIds          (T_IPT_LABEL         devId,            /* who am I? */
                                     T_IPT_LABEL         carId,       
                                     T_IPT_LABEL         cstId);      
   /* ---------------------------------------------------------------------------- */

   static int tdcGetAddrByName      (const T_IPT_URI     uri,        
                                     T_IPT_IP_ADDR*      pIpAddr,
                                     UINT8*              pTopoCnt);   
   static int tdcGetAddrByNameExt   (const T_IPT_URI     uri, 
                                     T_IPT_IP_ADDR*      pIpAddr,
                                     T_TDC_BOOL*         pIsFRG,
                                     UINT8*              pTopoCnt);
   static int tdcGetUriHostPart     (T_IPT_IP_ADDR       ipAddr,     
                                     T_IPT_URI           uri,
                                     UINT8*              pTopoCnt);       

   /* ---------------------------------------------------------------------------- */

   static int tdcLabel2CarId        (T_IPT_LABEL         carId,
                                     UINT8*              pTopoCnt,      
                                     const T_IPT_LABEL   cstLabel,   
                                     const T_IPT_LABEL   carLabel);  
   static int tdcAddr2CarId         (T_IPT_LABEL         carId,      
                                     UINT8*              pTopoCnt,      
                                     T_IPT_IP_ADDR       ipAddr);    

   /* ---------------------------------------------------------------------------- */

   static int tdcLabel2CstId        (T_IPT_LABEL         cstId,      
                                     UINT8*              pTopoCnt,      
                                     const T_IPT_LABEL   carLabel);  
   static int tdcAddr2CstId         (T_IPT_LABEL         cstId,      
                                     UINT8*              pTopoCnt,      
                                     T_IPT_IP_ADDR       ipAddr);    
   static int tdcCstNo2CstId        (T_IPT_LABEL         cstId,     
                                     UINT8*              pTopoCnt,     
                                     UINT8               trnCstNo);   

   /* ---------------------------------------------------------------------------- */

   static int tdcLabel2TrnCstNo     (UINT8*              pTrnCstNo,  
                                     UINT8*              pTopoCnt,      
                                     const T_IPT_LABEL   carLabel);  
   static int tdcAddr2TrnCstNo      (UINT8*              pTrnCstNo,  
                                     UINT8*              pTopoCnt,      
                                     T_IPT_IP_ADDR       ipAddr);    

   /* ---------------------------------------------------------------------------- */

   static int tdcGetTrnCstCnt       (UINT8*              pCstCnt,
                                     UINT8*              pTopoCnt);    

   static int tdcGetCstCarCnt       (UINT8*              pCarCnt,
                                     UINT8*              pTopoCnt,     
                                     const T_IPT_LABEL   cstLabel);   

   static int tdcGetCarDevCnt       (UINT16*             pDevCnt,
                                     UINT8*              pTopoCnt,     
                                     const T_IPT_LABEL   cstLabel,    
                                     const T_IPT_LABEL   carLabel);   

   /* ---------------------------------------------------------------------------- */

   static int tdcGetCarInfo         (T_TDC_CAR_DATA*     pCarData,
                                     UINT8*              pTopoCnt,    
                                     UINT16              maxDev,      
                                     const T_IPT_LABEL   cstLabel,    
                                     const T_IPT_LABEL   carLabel);   

   /* ---------------------------------------------------------------------------- */

   static int tdcGetUicState        (UINT8*              pInaugState, 
                                     UINT8*              pTopoCnt);   

   /* ---------------------------------------------------------------------------- */

   static int tdcGetUicGlobalData   (T_TDC_UIC_GLOB_DATA*   pGlobData,
                                     UINT8*                 pTopoCnt); 

   /* ---------------------------------------------------------------------------- */

   static int tdcGetUicCarData      (T_TDC_UIC_CAR_DATA*   pCarData,
                                     UINT8*                pTopoCnt,   
                                     UINT8                 carSeqNo);  

   /* ---------------------------------------------------------------------------- */

   static int tdcLabel2UicCarSeqNo  (UINT8*              pCarSeqNo,  
                                     UINT8*              pIptTopoCnt,
                                     UINT8*              pUicTopoCnt, 
                                     const T_IPT_LABEL   cstLabel,   
                                     const T_IPT_LABEL   carLabel);  
   static int tdcAddr2UicCarSeqNo   (UINT8*              pCarSeqNo,  
                                     UINT8*              pIptTopoCnt,
                                     UINT8*              pUicTopoCnt, 
                                     T_IPT_IP_ADDR       ipAddr);    

   /* ---------------------------------------------------------------------------- */

   static int tdcUicCarSeqNo2Ids    (T_IPT_LABEL         cstLabel,   
                                     T_IPT_LABEL         carLabel,   
                                     UINT8*              pIptTopoCnt,
                                     UINT8*              pUicTopoCnt, 
                                     UINT8               carSeqNo);  

   /* ---------------------------------------------------------------------------- */
   /* 2008/07/11, MRi - added missing prototype for C++ Wrapper API */

   static const char*   tdcGetErrorString (int     errCode);
   static UINT32        tdcGetVersion     (void);

};

extern IPTDirClient				tdc;

#endif


#endif



