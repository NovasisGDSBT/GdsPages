/*                                                                            */
/*  $Id: tdcDB.h 11648 2010-08-20 15:33:06Z bloehr $                           */
/*                                                                            */
/*  DESCRIPTION    Definitions, Prototypes ... for the Database of TDC        */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS                                                                   */
/*                                                                            */
/*  DEPENDENCIES   Basic Datatypes have to be defined in advance if LINUX     */
/*                 is chosen.                                                 */
/*                                                                            */
/*  MODIFICATIONS (log starts 2010-08-11):									  */
/*   																		  */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002-2010.             */
/*                                                                            */

/* ---------------------------------------------------------------------------- */

#if !defined (_TDCDB_H)
   #define _TDCDB_H

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

typedef enum
{
   DB_OK = 0,
   DB_ERROR,
   DB_NO_CONFIG,
   DB_NO_MATCHING_ENTRY
} T_DB_RESULT;

/* ---------------------------------------------------------------------------- */

typedef struct
{
   T_IPT_LABEL             dev;
   T_IPT_LABEL             car;
   T_IPT_LABEL             cst;
   T_IPT_LABEL             trn;
} T_URI_LABELS;

/* -------------------------------------------------------------------------- */

typedef struct
{
   UINT16                  lblIdx;
   UINT16                  hostId;
} T_DB_DEV_DATASET;

typedef struct
{
   UINT16                  lblCnt;
   UINT16                  devCnt;
   T_IPT_LABEL*            lblTab;
   T_DB_DEV_DATASET*       pDev;
} T_DB_DEV_DATA;

typedef struct
{
   UINT8                   iptInaugState;    /* IPT Inauguration state  */
   UINT8                   iptTopoCnt;       /* IPT Topology counter    */
   UINT8                   uicInaugState;    /* UIC Inauguration state  */
   UINT8                   uicTopoCnt;       /* UIC Topology counter    */
   UINT8                   dynState;         /* dynamic data state      */
   UINT8                   dynCnt;           /* dynamic counter         */
} T_DB_TRAIN_STATE;

typedef struct
{
   T_DB_TRAIN_STATE        expTrnState;      /* expected next train state */
                                             /* (inicated by IPTDir PD)   */
   T_IPT_IP_ADDR           gatewayAddr;
   UINT8                   tbType;
} T_DB_DYNAMIC_DATA;

/* ---------------------------------------------------------------------------- */

typedef struct
{
   UINT16                              lblIdx;
   UINT16                              no;
} T_DB_MC_GRP_DATA;

typedef struct
{
   const T_IPT_CAR_TYPE_LIST*          pCarTypeLst;
   const T_IPT_DEV_LABEL_LIST*         pDevLblLst;
   const T_IPT_GRP_LABEL_LIST*         pGrpLblLst;
   const T_IPT_MC_GRP_LIST*            pMcLst;
   UINT8                               inaugState;
   UINT8                               topoCnt;
} T_DB_IPT_TRN_DATA;

typedef struct
{
   T_IPT_LABEL                         cstId;
   const T_IPT_MC_GRP_LIST*            pMcLst;
   UINT32                              carCnt;
   UINT32                              devCnt;
   UINT32                              carMcGrpCnt;
   UINT8                               trnCstNo;
   UINT8                               bIsLocal;
   UINT8                               orient;
} T_DB_IPT_CST_DATA_SET;


typedef struct
{
   UINT16                              lblIdx;
   UINT16                              no;
} T_DB_IPT_DEV_DATA_SET;

typedef struct
{
   UINT32                              devCnt;
   T_DB_IPT_DEV_DATA_SET               dev[1];
} T_DB_IPT_DEV_DATA;
#define DB_IPT_DEV_DATA_SET_SIZE(cnt)     ((UINT32) (cnt * sizeof (T_DB_IPT_DEV_DATA_SET)))
#define DB_IPT_DEV_DATA_OFFSET()          ARRAY_OFFSET_IN_STRUC (T_DB_IPT_DEV_DATA, dev)
#define DB_IPT_DEV_DATA_SIZE(cnt)         (DB_IPT_DEV_DATA_OFFSET () + DB_IPT_DEV_DATA_SET_SIZE (cnt))

typedef struct
{
   T_IPT_LABEL                         cstId;
   T_IPT_LABEL                         carId;
   UINT32                              carTypeLblIdx;
   const T_IPT_MC_GRP_LIST*            pMcLst;
   const T_DB_IPT_DEV_DATA*            pDevs;
   T_UIC_IDENT                         uicIdent;
   UINT8                               cstCarNo;
   UINT8                               trnOrient;
   UINT8                               cstOrient;
} T_DB_IPT_CAR_DATA_SET;

/* ---------------------------------------------------------------------------- */

extern void        dbInitializeDB         (/*@in@*/ void*                  pArgV);
extern T_TDC_BOOL  dbDeleteDB             (void);
extern T_DB_RESULT dbGetCurTrainState     (/*@out@*/ T_DB_TRAIN_STATE*      pTrnState);

extern UINT8       dbIptGetTopoCnt        (void);
extern void        dbIptNewTopoCnt        (UINT8                  topoCnt);
extern void        dbIptNewInaugState     (UINT8                  inaugState,
                                           UINT8                  topoCnt);

/* ---------------------------------------------------------------------------- */

extern T_TDC_BOOL  dbIptNewTrain          (UINT8                  inaugState,
                                           UINT8                  topoCnt);
extern void        dbIptNoNewTrain        (void);
extern T_TDC_BOOL  dbIptSetLocal          (UINT16                 hostId);

extern T_TDC_BOOL  dbIptAddTrain          (const T_DB_IPT_TRN_DATA*        pTrn);
extern T_TDC_BOOL  dbIptAddConsist        (const T_DB_IPT_CST_DATA_SET*    pCst);
extern T_TDC_BOOL  dbIptAddCar            (const T_DB_IPT_CAR_DATA_SET*    pCar);
extern T_TDC_BOOL  dbIptCheckTrain        (void);
extern T_TDC_BOOL  dbIptActivateNewTrain  (void);

/* ---------------------------------------------------------------------------- */
/*@ -exportlocal */
extern void        dbUicNewTopoCnt        (UINT8                        topoCnt);
/*@ =exportlocal */
extern void        dbUicNewInaugState     (UINT8                        inaugState,
                                           UINT8                        topoCnt);

extern T_TDC_BOOL  dbUicNewTrain          (UINT8                        inaugState,
                                           UINT8                        topoCnt);
extern T_TDC_BOOL  dbUicAddGlobData       (const T_TDC_UIC_GLOB_DATA*   pGlobData);
extern T_TDC_BOOL  dbUicAddCarData        (const T_TDC_UIC_CAR_DATA*    pCarData,
                                           UINT8                        carNo);
extern T_TDC_BOOL  dbUicActivateNewTrain  (void);

/* ---------------------------------------------------------------------------- */

extern void        dbIptVerboseTrain      (void);
extern void        dbUicVerboseTrain      (void);

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

extern T_DB_RESULT dbIptGetOwnIds         (/*@out@*/ T_URI_LABELS*      pUriLabels,
                                           /*@out@*/ UINT8*             pTopoCnt);

extern T_DB_RESULT dbIptGetCstCnt         (/*@out@*/ UINT8*             pCstCnt,
                                           /*@out@*/ UINT8*             pTopoCnt);
extern T_DB_RESULT dbIptGetCstCarCnt      (const T_IPT_LABEL            cstLabel,
                                           /*@out@*/ UINT8*             pCarCnt,
                                           /*@out@*/ UINT8*             pTopoCnt);
extern T_DB_RESULT dbIptGetCarDevCnt      (const T_IPT_LABEL            cstLabel,
                                           const T_IPT_LABEL            carLabel,
                                           /*@out@*/ UINT16*            pDevCnt,
                                           /*@out@*/ UINT8*             pTopoCnt);

extern T_DB_RESULT dbIptLabel2CstId       (const T_IPT_LABEL            carLabel,
                                           /*@out@*/ T_IPT_LABEL        cstId,
                                           /*@out@*/ UINT8*             pTopoCnt);
extern T_DB_RESULT dbIptCstNo2CstId       (UINT8                        trnCstNo,
                                           /*@out@*/ T_IPT_LABEL        cstId,
                                           /*@out@*/ UINT8*             pTopoCnt);

extern T_DB_RESULT dbIptGetCstNo          (const T_IPT_LABEL            cstLabel,
                                           /*@out@*/ UINT8*             pLCstNo,
                                           /*@out@*/ UINT8*             pCstNo,
                                           /*@out@*/ UINT8*             pTopoCnt);
extern T_DB_RESULT dbIptGetCstNoCarNo     (const T_IPT_LABEL            cstLabel,
                                           const T_IPT_LABEL            carLabel,
                                           /*@out@*/ UINT8*             pCstNo,
                                           /*@out@*/ UINT8*             pCarNo);
extern T_DB_RESULT dbIptGetCarNo          (const T_IPT_LABEL            cstLabel,
                                           const T_IPT_LABEL            carLabel,
                                           /*@out@*/ UINT8*             pCarNo);

/*@ -exportlocal */
extern T_DB_RESULT dbIptGetCar            (const T_IPT_LABEL            cstLabel,
                                           UINT16                       hostId,
                                           /*@out@*/ T_IPT_LABEL        devId,
                                           /*@out@*/ T_IPT_LABEL        carId,
                                           /*@out@*/ T_IPT_LABEL        cstId,
                                           /*@out@*/ UINT8*             pCarNo);
/*@ =exportlocal */

extern T_DB_RESULT dbIptGetHostId         (const T_URI_LABELS*          pUriLabels,
                                           /*@out@*/ UINT8*             pLCstNo,
                                           /*@out@*/ UINT8*             pCstNo,
                                           /*@out@*/ UINT16*            pHostId,
                                           /*@out@*/ UINT8*             pTopoCnt,
                                           /*@out@*/ UINT8*             pTbType,
                                           /*@out@*/ T_IPT_IP_ADDR*     pGatewayAddr);
extern T_DB_RESULT dbIptGetUnicastLabels  (UINT8                        cstNo,
                                           UINT16                       hostId,
                                           /*@out@*/ T_URI_LABELS*      pUriLabels,
                                           /*@out@*/ UINT8*             pTopoCnt);
extern T_DB_RESULT dbIptGetMulticastLabels(UINT8                        cstNo,
                                           UINT16                       hostId,
                                           /*@out@*/ T_URI_LABELS*      pUriLabels,
                                           /*@out@*/ UINT8*             pTopoCnt);

extern T_DB_RESULT dbIptGetCarId          (UINT8                        cstNo,
                                           UINT8                        carNo,
                                           /*@out@*/ T_IPT_LABEL        carId,
                                           /*@out@*/ UINT8*             pTopoCnt);

extern T_DB_RESULT dbIptGetCarInfo        (/*@out@*/ T_TDC_CAR_DATA*    pCarData,
                                           UINT16                       maxDev,
                                           const T_IPT_LABEL            cstLabel,
                                           const T_IPT_LABEL            carLabel,
                                           /*@out@*/ UINT8*             pTopoCnt);

extern T_DB_RESULT dbGetTrnGrpNo          (const T_URI_LABELS*          pUriLabels,
                                           /*@out@*/ UINT16*            pGrpNo,
                                           /*@out@*/ UINT8*             pTopoCnt,
                                           /*@out@*/ UINT8*             pTbType,
                                           /*@out@*/ T_IPT_IP_ADDR*     pGatewayAddr);

extern T_DB_RESULT dbGetCstGrpNo          (const T_URI_LABELS*          pUriLabels,
                                           /*@out@*/ UINT16*            pGrpNo,
                                           /*@out@*/ UINT8*             pCstNo,
                                           /*@out@*/ UINT8*             pLCstNo,
                                           /*@out@*/ UINT8*             pTopoCnt,
                                           /*@out@*/ UINT8*             pTbType,
                                           /*@out@*/ T_IPT_IP_ADDR*     pGatewayAddr);

extern T_DB_RESULT dbGetAnyCarGrpNo       (const T_URI_LABELS*          pUriLabels,
                                           /*@out@*/ UINT16*            pGrpNo,
                                           /*@out@*/ UINT8*             pCstNo,
                                           /*@out@*/ UINT8*             pLCstNo,
                                           /*@out@*/ UINT8*             pTopoCnt,
                                           /*@out@*/ UINT8*             pTbType,
                                           /*@out@*/ T_IPT_IP_ADDR*     pGatewayAddr);

extern T_DB_RESULT dbGetCarGrpNo          (const T_URI_LABELS*          pUriLabels,
                                           /*@out@*/ UINT16*            pGrpNo,
                                           /*@out@*/ UINT8*             pCstNo,
                                           /*@out@*/ UINT8*             pLCstNo,
                                           /*@out@*/ UINT8*             pTopoCnt,
                                           /*@out@*/ UINT8*             pTbType,
                                           /*@out@*/ T_IPT_IP_ADDR*     pGatewayAddr);

/* ---------------------------------------------------------------------------- */

extern T_DB_RESULT dbUicGetGlobalData     (/*@out@*/ T_TDC_UIC_GLOB_DATA*  pGlobData);
extern T_DB_RESULT dbUicGetCarData        (/*@out@*/ T_TDC_UIC_CAR_DATA*   pCarData,
                                                     UINT8                 carSeqNo);

/* ---------------------------------------------------------------------------- */

extern T_DB_RESULT dbGetUicCarSeqNo       (/*@out@*/ UINT8*                pCarSeqNo,
                                           const T_IPT_LABEL               cstLabel,
                                           const T_IPT_LABEL               carLabel);

extern T_DB_RESULT dbUicCarSeqNo2Ids      (/*@out@*/ T_IPT_LABEL           cstLabel,
                                           /*@out@*/ T_IPT_LABEL           carLabel,
                                           UINT8                           carSeqNo);

/* ---------------------------------------------------------------------------- */

extern void        dbSetIptDirServerHostId   (UINT16                       hostId);
extern T_TDC_BOOL  dbGetIptDirGateway        (UINT8*                       pTbType,
                                              T_IPT_IP_ADDR*               pGatewayAddr);
extern void        dbSetIptDirGateway        (UINT8                        tbType,
                                              T_IPT_IP_ADDR                gatewayAddr);
extern T_DB_RESULT dbGetAnyCarHostId         (const T_URI_LABELS*          pUriLabels,
                                              /*@out@*/ UINT8*             pLCstNo,
                                              /*@out@*/ UINT8*             pCstNo,
                                              /*@out@*/ UINT16*            pHostId,
                                              /*@out@*/ UINT8*             pTopoCnt,
                                              /*@out@*/ UINT8*             pTbType,
                                              /*@out@*/ T_IPT_IP_ADDR*     pGatewayAddr);

extern T_TDC_BOOL  dbIsCstLCst               (const T_URI_LABELS*          pUriLabels);

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif


#endif



