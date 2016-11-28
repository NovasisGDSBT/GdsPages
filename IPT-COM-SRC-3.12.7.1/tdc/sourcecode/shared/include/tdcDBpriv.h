/*                                                                            */
/*  $Id: tdcDBpriv.h 11653 2010-08-23 08:17:51Z bloehr $                       */
/*                                                                            */
/*  DESCRIPTION    Definitions, Prototypes ... for the Database of TDC        */
/*                 private routines / data ...                                */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS                                                                   */
/*                                                                            */
/*  DEPENDENCIES   Basic Datatypes have to be defined in advance if LINUX     */
/*                 is chosen.                                                 */
/*                                                                            */
/*  MODIFICATIONS                       									  */
/*   																		  */
/*                                                                            */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002-2010.             */
/*                                                                            */

/* ---------------------------------------------------------------------------- */

#if !defined (_TDCDBPRIV_H)
   #define _TDCDBPRIV_H

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

typedef struct
{
   UINT8                   cstCarNo;         /* device's carNo in cst */
   UINT16                  hostId;           /* (device number) */
   UINT16                  lblIdx;           /* device label index  */
} T_DB_DEVICE;

typedef struct
{
   UINT32                  cnt;              /* number of devices in List */
   UINT32                  addIdx;           /* index for insertion of next device */
   T_DB_DEVICE             dev[1];
} T_DB_DEV_LIST;
#define DB_DEVICE_SIZE(cnt)                  ((UINT32) (cnt * sizeof (T_DB_DEVICE)))
#define DB_DEV_LIST_DEV_OFFSET()             ARRAY_OFFSET_IN_STRUC (T_DB_DEV_LIST, dev)
#define DB_DEV_LIST_SIZE(cnt)                (DB_DEV_LIST_DEV_OFFSET () + DB_DEVICE_SIZE (cnt))

/* ---------------------------------------------------------------------------- */

typedef struct
{
   UINT8                   cstCarNo;         /* multicast group's carNo in cst (0 == cst-group) */
   UINT16                  lblIdx;
   UINT16                  no;
} T_DB_MC_GRP;

typedef struct
{
   UINT32                  cnt;              /* Number of multicast groups in List */
   UINT32                  addIdx;           /* index for insertion of next multicast group */
   T_DB_MC_GRP             grp[1];
} T_DB_MC_GRP_LIST;
#define DB_MC_GRP_SIZE(cnt)                  ((UINT32) (cnt * sizeof (T_DB_MC_GRP)))
#define DB_MC_GRP_LIST_GRP_OFFSET()          ARRAY_OFFSET_IN_STRUC (T_DB_MC_GRP_LIST, grp)
#define DB_MC_GRP_LIST_SIZE(cnt)             (DB_MC_GRP_LIST_GRP_OFFSET () + DB_MC_GRP_SIZE (cnt))

/* ---------------------------------------------------------------------------- */

typedef struct
{
   T_IPT_LABEL             carId;            /* car label */
   T_IPT_LABEL             carNoLbl;         /* car no as label (e.g. 'car01') */
   UINT32                  carTypeLblIdx;    /* car type  */
   T_UIC_IDENT             uicIdent;
   UINT8                   cstCarNo;         /* consist car no */
   UINT8                   trnOrient;        /* Orientation of car relative to IP-Train */
                                             /* 0 --> opposite, else --> same or.       */
   UINT8                   cstOrient;        /* Orientation of car relative to Consist's ref. dir */
                                             /* 0 --> opposite, else --> same or.                 */
} T_DB_CAR;

typedef struct
{
   UINT16                  cnt;              /* number of cars in List */
   UINT16                  addIdx;           /* index for insertion of next car */
   T_DB_CAR                car[1];
} T_DB_CAR_LIST;
#define DB_CAR_SIZE(cnt)                     ((UINT32) (cnt * sizeof (T_DB_CAR)))
#define DB_CAR_LIST_CAR_OFFSET()             ARRAY_OFFSET_IN_STRUC (T_DB_CAR_LIST, car)
#define DB_CAR_LIST_SIZE(cnt)                (DB_CAR_LIST_CAR_OFFSET () + DB_CAR_SIZE (cnt))

/* ---------------------------------------------------------------------------- */

typedef struct  S_CONSIST
{
               T_IPT_LABEL             cstId;            /* Consist label */
               T_IPT_LABEL             cstNoLbl;         /* Train Consist number as label (e.g. 'cst01') */
               T_DB_MC_GRP_LIST*       pCstMcLst;        /* List of all cst multicast groups in this consist */
/*@shared@*/   T_DB_MC_GRP_LIST*       pCarMcLst;        /* List of all car multicast groups in this consist */
/*@shared@*/   T_DB_CAR_LIST*          pCarLst;          /* List of all cars in this consist */
/*@shared@*/   T_DB_DEV_LIST*          pDevLst;          /* List of all devices in this consist */
/*@shared@*/   struct S_CONSIST*       pNext;            /* Used for linked list */
               UINT16                  trnCstNo;         /* Train Consist number */
               UINT8                   bIsLocal;         /* local or remote consist ? */
                                                         /* 0 --> remote consist, 1 --> local consist */
               UINT8                   orient;           /* orientation of consist in train */
                                                         /* 0 -->opposite, 1 --> IP-Train ref direction */
} T_DB_CONSIST;

/* ---------------------------------------------------------------------------- */

typedef struct
{
   UINT32                  lblCnt;
   T_IPT_LABEL*            pLbl;
} T_DB_LABEL_LIST;

typedef struct
{
   UINT32                  grpCnt;
   T_DB_MC_GRP_DATA*       pGrp;
} T_DB_MCGRP_LIST;


typedef struct
{
                           UINT8                inaugState;
                           UINT8                topoCnt;
                           T_DB_LABEL_LIST      carTypeLst;
                           T_DB_LABEL_LIST      devLblLst;
                           T_DB_LABEL_LIST      grpLblLst;
                           T_DB_MCGRP_LIST      mcLst;            /* Train level MC Groups   */
   /*@null@*/ /*@shared@*/ UINT8*               pLblAndGrpBuf;    /* buffer for Labels and Groups */
   /*@null@*/ /*@shared@*/ const T_DB_CAR*      pLocCar;          /* pointer to local Car    */
   /*@null@*/ /*@shared@*/ const T_DB_DEVICE*   pLocDev;          /* pointer to local Device */
   /*@null@*/ /*@shared@*/ T_DB_CONSIST*        pCstRoot;         /* linked Consist list     */
} T_DB_IPT_TRAIN;

/* ---------------------------------------------------------------------------- */

typedef struct
{
   T_TDC_UIC_GLOB_DATA     global;
   T_TDC_UIC_CAR_DATA*     pCarData;
} T_DB_UIC_TRAIN;

/* ---------------------------------------------------------------------------- */

typedef enum
{
   CMS_ANYCAR_IDX             = 0,
   IPTDIRSERVER_ANYCAR_IDX,
   ANYCAR_INDEX_TAB_SIZE
} T_ANYCAR_INDEX;

typedef struct
{
   UINT16               hostId;
   T_IPT_LABEL          devId;
} T_ANYCAR_TAB;

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

extern T_DB_IPT_TRAIN         dbCurIptTrn;
extern T_DB_IPT_TRAIN         dbShadowIptTrn;

extern T_DB_UIC_TRAIN         dbCurUicTrn;
extern T_DB_UIC_TRAIN         dbShadowUicTrn;

extern T_DB_DYNAMIC_DATA      dbDynData;

/* ---------------------------------------------------------------------------- */

extern T_ANYCAR_TAB           dbAnyCarTab[ANYCAR_INDEX_TAB_SIZE];

/* ---------------------------------------------------------------------------- */

extern void                delIptTrain       (T_DB_IPT_TRAIN*        pTrain,
                                              T_TDC_BOOL             bInclLocal);    /* Include local consist ? */
extern void                delUicTrain       (T_DB_UIC_TRAIN*        pTrain,
                                              T_TDC_BOOL             bInclLocal);    /* Include local consist ? */

/*@null@*/ /*@shared@*/
extern const T_DB_CONSIST* findCstByCstNo    (UINT8                     trnCstNo);
/*@null@*/
extern const T_DB_CAR*     findCarByCarNo    (const T_DB_CONSIST*       pCst,
                                              UINT8                     cstCarNo);
/*@null@*/
extern const T_DB_MC_GRP*  findGrpByLbl      (const T_DB_MC_GRP_LIST*   pMcLst,
                                              const T_IPT_LABEL         grpLabel,
                                              const UINT8*              pCstCarNo);
/*@null@*/
extern const T_DB_MC_GRP*  findGrpByGrpNo    (const T_DB_MC_GRP_LIST*   pMcLst,
                                                    UINT16              grpNo,
                                              const UINT8*              pCstCarNo);

/*@null@*/ /*@shared@*/
extern const T_DB_CONSIST* findCstByCstLbl   (const T_IPT_LABEL         cst);
/*@null@*/ /*@shared@*/
extern const T_DB_CAR*     findCarByCarLbl   (const T_DB_CONSIST*    pCst,
                                              const T_IPT_LABEL      car);
/*@null@*/ /*@shared@*/
extern const T_DB_CAR*     findCarByLbls     (/*@out@*/ const T_DB_CONSIST**   ppCst,
                                              const T_IPT_LABEL      cstLabel,
                                              const T_IPT_LABEL      carLabel);
/*@null@*//*@shared@*/
extern const T_DB_DEVICE*  findDevByLDevLbl  (/*@null@*/ const T_DB_CONSIST*    pCst,
                                              const T_DB_CAR*        pCar,
                                              const T_IPT_LABEL      dev);
extern  T_TDC_BOOL         findIptUicIdent   (const T_UIC_IDENT      uicIdent,
                                              T_IPT_LABEL            cstLabel,
                                              T_IPT_LABEL            carLabel);

/* ---------------------------------------------------------------------------- */

extern void verboseIptTrn                    (const T_DB_IPT_TRAIN*  pIptTrn);

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif


#endif



