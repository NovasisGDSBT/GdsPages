/*                                                                           */
/* $Id: tdcTMsgData.c 11696 2010-09-17 10:03:34Z gweiss $                    */
/*                                                                           */
/* DESCRIPTION                                                               */
/*                                                                           */
/* AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                           */
/* REMARKS                                                                   */
/*                                                                           */
/* DEPENDENCIES   Either the switch VXWORKS, LINUX or WIN32 has to be set    */
/*                                                                           */
/* All rights reserved. Reproduction, modification, use or disclosure        */
/* to third parties without express authority is forbidden.                  */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.                  */
/*                                                                           */

/* ---------------------------------------------------------------------------- */

#include "tdc.h"
#include "tdcInit.h"
#include "tdcDB.h"
#include "tdcConfig.h"
#include "tdcIptCom.h"
#include "tdcThread.h"
#include "tdcMsgData.h"

/* ---------------------------------------------------------------------------- */

#define POINTER_DISTANCE(x, y)         ((((UINT32) (x))  >  ((UINT32) (y)))      \
                                          ? (((UINT32) (x))  -  ((UINT32) (x)))  \
                                          : (((UINT32) (y))  -  ((UINT32) (x)))  \
                                       )

/* ---------------------------------------------------------------------------- */

typedef struct
{
   UINT32                        msgLen;
   UINT32                        nextStop;
   UINT32                        chkLen;
   UINT8*                        pIptMd;
   T_IPT_CAR_TYPE_LIST*          pCarTypeLst;
   T_IPT_DEV_LABEL_LIST*         pDevLblLst;
   T_IPT_GRP_LABEL_LIST*         pGrpLblLst;
   T_IPT_MC_GRP_LIST*            pMcLst;
   T_IPT_RES_QW_LIST*            pResQwLst;
   T_IPT_CONSIST_LIST*           pCstLst;
} T_TRAIN_INFO;

typedef struct
{
   T_IPT_CONSIST_DATA_SET*       pCst;
   T_IPT_MC_GRP_LIST*            pMcLst;
   T_IPT_RES_QW_LIST*            pResQwLst;
   T_IPT_CAR_LIST*               pCarLst;
} T_CST_INFO;

typedef struct
{
   T_IPT_CAR_DATA_SET*           pCar;
   T_IPT_MC_GRP_LIST*            pMclst;
   T_IPT_RES_QW_LIST*            pResQwLst;
   T_IPT_DEV_LIST*               pDevLst;
} T_CAR_INFO;

/* ---------------------------------------------------------------------------- */

static void       iptDirRequest     (UINT16                             reqType);

static void       consumeIptMsgDataV2 (T_IPTDIR_IPT_MD*                 pIptMsg,
                                       UINT32                           msgLen);
static void       consumeUicMsgDataV2 (T_IPTDIR_UIC_MD*                 pIptMsg,
                                       UINT32                           msgLen);

static T_TDC_BOOL checkMsgLen         (const T_TRAIN_INFO*              pTrnInfo,
                                       const char*                      pText);
static T_TDC_BOOL buildIptTrain       (T_TRAIN_INFO*                    pTrnInfo);
static T_TDC_BOOL addIptCst           (T_TRAIN_INFO*                    pTrnInfo,
                                       const T_CST_INFO*                pCstInfo);
static T_TDC_BOOL getCstVarCnt        (T_TRAIN_INFO*                    pTrnInfo,
                                       const T_CST_INFO*                pCstInfo,
                                       T_DB_IPT_CST_DATA_SET*           pCst);

static void       labelN2H            (T_IPT_LABEL                      iptLabel);

static void       carTypesN2H         (T_IPT_CAR_TYPE_LIST*             pCarTypeLst);
static void       devLabelsN2H        (T_IPT_DEV_LABEL_LIST*            pDevLblLst);
static void       grpLabelsN2H        (T_IPT_GRP_LABEL_LIST*            pGrpLblLst);
static void       mcGrpsN2H           (T_IPT_MC_GRP_LIST*               pMcLst);
static void       resQwN2H            (T_IPT_RES_QW_LIST*               pResQwLst);
static T_TDC_BOOL devDataN2H          (T_TRAIN_INFO*                    pTrnInfo);

static void       cstDataN2H          (T_IPT_CONSIST_DATA_SET*          pCst,
                                       UINT32                           cstNo);
static void       carDataN2H          (T_IPT_CAR_DATA_SET*              pCar,
                                       UINT32                           carNo);


static void       verboseCarTypes     (const T_IPT_CAR_TYPE_LIST*       pCarTypeLst);
static void       verboseDevLabels    (const T_IPT_DEV_LABEL_LIST*      pDevLblLst);
static void       verboseGrpLabels    (const T_IPT_GRP_LABEL_LIST*      pGrpLblLst);
static void       verboseMcGrps       (const T_IPT_MC_GRP_LIST*         pMclst);
static void       verboseResQw        (const T_IPT_RES_QW_LIST*         pResQwLst);

static T_TDC_BOOL getTrainInfo        (T_TRAIN_INFO*                    pTrnInfo);
static T_TDC_BOOL getCstInfo          (T_TRAIN_INFO*                    pTrnInfo,
                                       T_CST_INFO*                      pCstInfo,
                                       UINT32                           cstNo);

static void       getUicGlobData      (/*@out@*/ T_TDC_UIC_GLOB_DATA*   pGlobData,
                                       T_IPTDIR_UIC_MD*                 pIptMsg);
static void       getUicCarData       (T_TDC_UIC_CAR_DATA*              pCarData,
                                       T_IPTDIR_UIC_MD*                 pIptMsg,
                                       UINT32                           carNo);

/* ---------------------------------------------------------------------------- */
/*@ -compdestroy */
static void iptDirRequest (UINT16      reqType)
{
   T_WRITE_MD              writeMd;
   T_IPT_IPTDIR_REQ_MD     iptReq;

   iptReq.protVer      = tdcH2N32 (MAKE_PROT_VERSION (2, 0, 0, 0));
   iptReq.requType     = tdcH2N16 (reqType);
   iptReq.reserved0    = tdcH2N16 (0);

   writeMd.msgDataType = SEND_IPT_MSG_DATA;
   writeMd.msgLen      = (UINT32) sizeof (iptReq);
   writeMd.pMsgData    = &iptReq;

   (void) tdcStrNCpy (writeMd.destUri, iptDirServerAnyCar, IPT_LABEL_SIZE);
   (void) tdcStrNCat (writeMd.destUri, ".",                IPT_URI_SIZE);
   (void) tdcStrNCat (writeMd.destUri, anyCar,             IPT_URI_SIZE);

   if (!tdcWriteMsgData (MOD_MD, &writeMd))
   {
      DEBUG_WARN1 (MOD_MD, "Failed to call tdcRequIptMsgData(%d)", iptReq.requType);
   }
   else
   {
      DEBUG_INFO (MOD_MD, "Successfully requested MsgData (from IPTDir-Server)");
   }
} /*@ =compdestroy */

/* ---------------------------------------------------------------------------- */

static void labelN2H (T_IPT_LABEL   iptLabel)
{
   UINT32      i;

   for (i = 0; (i < IPT_LABEL_SIZE)   &&   (iptLabel[i] != '\0'); i++)
   {
      iptLabel[i] = MD_N2H8 (iptLabel[i]);
   }
}

/* ---------------------------------------------------------------------------- */

static void carTypesN2H (T_IPT_CAR_TYPE_LIST*     pCarTypeLst)
{
   UINT32            i;

   for (i = 0; i < pCarTypeLst->lblCnt; i++)
   {
      labelN2H (pCarTypeLst->lbl[i]);
   }

   verboseCarTypes (pCarTypeLst);
}

/* ---------------------------------------------------------------------------- */

static void devLabelsN2H (T_IPT_DEV_LABEL_LIST*     pDevLblLst)
{
   UINT32            i;

   for (i = 0; i < pDevLblLst->lblCnt; i++)
   {
      labelN2H (pDevLblLst->lbl[i]);
   }

   verboseDevLabels (pDevLblLst);
}

/* ---------------------------------------------------------------------------- */

static void grpLabelsN2H (T_IPT_GRP_LABEL_LIST*  pGrpLblLst)
{
   UINT32            i;

   for (i = 0; i < pGrpLblLst->lblCnt; i++)
   {
      labelN2H (pGrpLblLst->lbl[i]);
   }

   verboseGrpLabels (pGrpLblLst);
}

/* ---------------------------------------------------------------------------- */

static void mcGrpsN2H (T_IPT_MC_GRP_LIST*  pMcLst)
{
   UINT32            i;

   for (i = 0; i < pMcLst->grpCnt; i++)
   {
      pMcLst->grp[i].lblIdx = MD_N2H16 (pMcLst->grp[i].lblIdx);
      pMcLst->grp[i].no     = MD_N2H16 (pMcLst->grp[i].no);
   }

   verboseMcGrps (pMcLst);
}

/* ---------------------------------------------------------------------------- */

static void resQwN2H (T_IPT_RES_QW_LIST*     pResQwLst)
{
   UINT32            i;

   for (i = 0; i < pResQwLst->resQwCnt; i++)
   {
      pResQwLst->resQw[i] = MD_N2H32 (pResQwLst->resQw[i]);  
   }

   verboseResQw (pResQwLst);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL checkMsgLen (const T_TRAIN_INFO*       pTrnInfo,
                               const char*               pText)
{
   T_TDC_BOOL        bOK = TRUE;

   if (pTrnInfo->chkLen > pTrnInfo->msgLen)
   {
      char     textBuf[100];

      (void) tdcSNPrintf (textBuf, (UINT32) sizeof (textBuf), 
                          "%s: Message Size mismatch - req. (%d), recv (%d)", 
                          pText, (int) pTrnInfo->chkLen, (int) pTrnInfo->msgLen);
      textBuf[sizeof (textBuf) - 1] = '\0';

      DEBUG_WARN (MOD_MD, textBuf);
      bOK = FALSE;
   }

   return (bOK);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL devDataN2H (T_TRAIN_INFO*        pTrnInfo)
{
   UINT32            devNo;
   T_IPT_DEV_LIST*   pDevLst = (T_IPT_DEV_LIST *) ((void *) &pTrnInfo->pIptMd[pTrnInfo->nextStop]);

   pTrnInfo->nextStop = pTrnInfo->chkLen;                                     // pDevData

   DEBUG_INFO1 (MOD_MD, "IPT-MD: devCnt=%d", pDevLst->devCnt);

   for (devNo = 0; devNo < pDevLst->devCnt; devNo++)
   {
      T_IPT_DEV_DATA_SET*     pDev = (T_IPT_DEV_DATA_SET *) ((void *) &pTrnInfo->pIptMd[pTrnInfo->nextStop]);

      pTrnInfo->nextStop += DEV_DATA_SET_RESQW_LIST_OFFSET ();            // pResQwDataSet
      pTrnInfo->chkLen    = pTrnInfo->nextStop + RES_QW_LIST_SIZE (0);    // pResQwData

      if (!checkMsgLen (pTrnInfo, "devDataN2H-1"))
      {
         return (FALSE);
      }

      pDev->lblIdx            = MD_N2H16 (pDev->lblIdx);  
      pDev->no                = MD_N2H16 (pDev->no);  
      pDev->resQwLst.resQwCnt = MD_N2H32 (pDev->resQwLst.resQwCnt);

      pTrnInfo->nextStop += RES_QW_LIST_SIZE (pDev->resQwLst.resQwCnt);             // pDevData
      pTrnInfo->chkLen    = pTrnInfo->nextStop;                                     // pDevData

      DEBUG_INFO3 (MOD_MD, "IPT-MD: dev[%d] - lblIdx=%d, no=%d", devNo, pDev->lblIdx, pDev->no);

      if (!checkMsgLen (pTrnInfo, "devDataN2H-2"))
      {
         return (FALSE);
      }

      resQwN2H (&pDev->resQwLst);
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

static void cstDataN2H (T_IPT_CONSIST_DATA_SET*    pCst,
                        UINT32                     cstNo)
{
   char     textBuf[100];

   labelN2H (pCst->cstLbl);

   pCst->trnCstNo = MD_N2H8 (pCst->trnCstNo);
   pCst->bIsLocal = MD_N2H8 (pCst->bIsLocal);
   pCst->orient   = MD_N2H8 (pCst->orient);

   (void) tdcSNPrintf (textBuf, (UINT32) sizeof (textBuf), "IPT-MD: cstNo[%d] - cstId = %s", cstNo, pCst->cstLbl);
   textBuf[sizeof (textBuf) - 1] = '\0';

   DEBUG_INFO  (MOD_MD, textBuf);
   DEBUG_INFO3 (MOD_MD, "IPT-MD: trnCstNo=%d, bIsLocal=%d, orient=%d", pCst->trnCstNo, pCst->bIsLocal, pCst->orient);
}

/* ---------------------------------------------------------------------------- */

static void carDataN2H (T_IPT_CAR_DATA_SET*     pCar,
                        UINT32                  carNo)
{
   char     textBuf[100];
   int      i;

   labelN2H (pCar->carLbl);

   pCar->cstCarNo      = MD_N2H8  (pCar->cstCarNo);
   pCar->TCrrrrrr      = MD_N2H8  (pCar->TCrrrrrr);
   pCar->carTypeLblIdx = MD_N2H16 (pCar->carTypeLblIdx);

   for (i = 0; i < IPT_UIC_IDENTIFIER_CNT; i++)
   {
      pCar->uicID[i] = MD_N2H8 (pCar->uicID[i]);
   }

   (void) tdcSNPrintf (textBuf, (UINT32) sizeof (textBuf), "IPT-MD: car[%d] - carId = %s", carNo, pCar->carLbl);
   textBuf[sizeof (textBuf) - 1] = '\0';

   DEBUG_INFO  (MOD_MD, textBuf);
   DEBUG_INFO3 (MOD_MD, "IPT-MD: cstCarNo=%d, TCrrrrrr=x%x, carTypeLblIdx=%d",
                        pCar->cstCarNo, pCar->TCrrrrrr, pCar->carTypeLblIdx);
   (void) tdcSNPrintf (textBuf, (UINT32) sizeof (textBuf), 
                       "IPT-MD: UIC-Id=x%02x%02x%02x%02x%02x",
                       pCar->uicID[0], pCar->uicID[1], pCar->uicID[2],
                       pCar->uicID[3], pCar->uicID[4]);

   DEBUG_INFO  (MOD_MD, textBuf);
}

/* ---------------------------------------------------------------------------- */

static void verboseCarTypes (const T_IPT_CAR_TYPE_LIST*     pCarTypeLst)
{
   UINT32            i;

   DEBUG_INFO1 (MOD_MD, "IPT-MD: carTypeCnt=%d", pCarTypeLst->lblCnt);

   for (i = 0; i < pCarTypeLst->lblCnt; i++)
   {
      char     text[100];

      (void) tdcSNPrintf (text, (UINT32) sizeof (text), "IPT-MD: carType[%2d]=%s", i, pCarTypeLst->lbl[i]);
      text[sizeof (text) - 1] = '\0';

      DEBUG_INFO (MOD_MD, text);
   }
}

/* ---------------------------------------------------------------------------- */

static void verboseDevLabels (const T_IPT_DEV_LABEL_LIST*      pDevLblLst)
{
   UINT32            i;

   DEBUG_INFO1 (MOD_MD, "IPT-MD: devLabelCnt=%d", pDevLblLst->lblCnt);

   for (i = 0; i < pDevLblLst->lblCnt; i++)
   {
      char     text[100];

      (void) tdcSNPrintf (text, (UINT32) sizeof (text), "IPT-MD: devLabel[%2d]=%s", i, pDevLblLst->lbl[i]);
      text[sizeof (text) - 1] = '\0';

      DEBUG_INFO (MOD_MD, text);
   }
}

/* ---------------------------------------------------------------------------- */

static void verboseGrpLabels (const T_IPT_GRP_LABEL_LIST*  pGrpLblLst)
{
   UINT32            i;

   DEBUG_INFO1 (MOD_MD, "IPT-MD: grpLabelCnt=%d", pGrpLblLst->lblCnt);

   for (i = 0; i < pGrpLblLst->lblCnt; i++)
   {
      char     text[100];

      (void) tdcSNPrintf (text, (UINT32) sizeof (text), "IPT-MD: grpLabel[%2d]=%s", i, pGrpLblLst->lbl[i]);
      text[sizeof (text) - 1] = '\0';

      DEBUG_INFO (MOD_MD, text);
   }
}

/* ---------------------------------------------------------------------------- */

static void verboseMcGrps (const T_IPT_MC_GRP_LIST*  pMcLst)
{
   UINT32            i;

   DEBUG_INFO1 (MOD_MD, "IPT-MD: mcGrpsCnt=%d", pMcLst->grpCnt);

   for (i = 0; i < pMcLst->grpCnt; i++)
   {
      DEBUG_INFO3 (MOD_MD, "IPT-MD: grpLblIdx[%d]=%d, grpNo=%d", i, pMcLst->grp[i].lblIdx, pMcLst->grp[i].no);
   }
}

/* ---------------------------------------------------------------------------- */

static void verboseResQw (const T_IPT_RES_QW_LIST*     pResQwLst)
{
   UINT32            i;

   DEBUG_INFO1 (MOD_MD, "IPT-MD: resQwCnt=%d", pResQwLst->resQwCnt);

   for (i = 0; i < pResQwLst->resQwCnt; i++)
   {
      DEBUG_INFO2 (MOD_MD, "IPT-MD: resQw[%d]=x%08x", i, pResQwLst->resQw[i]);
   }
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL getTrainInfo (T_TRAIN_INFO*      pTrnInfo)
{
   pTrnInfo->pCarTypeLst         = (T_IPT_CAR_TYPE_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pTrnInfo->pCarTypeLst->lblCnt = MD_N2H32 (pTrnInfo->pCarTypeLst->lblCnt);
   pTrnInfo->nextStop           += CAR_TYPE_LIST_SIZE (pTrnInfo->pCarTypeLst->lblCnt);             // pDevLblLst
   pTrnInfo->chkLen              = pTrnInfo->nextStop + DEV_LABEL_LIST_SIZE (0);                   // pDevLblDataSet

   if (!checkMsgLen (pTrnInfo, "getTrainInfo-1"))
   {
      return (FALSE);
   }

   carTypesN2H (pTrnInfo->pCarTypeLst);

   pTrnInfo->pDevLblLst         = (T_IPT_DEV_LABEL_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pTrnInfo->pDevLblLst->lblCnt = MD_N2H32 (pTrnInfo->pDevLblLst->lblCnt);
   pTrnInfo->nextStop          += DEV_LABEL_LIST_SIZE (pTrnInfo->pDevLblLst->lblCnt);              // pGrpLblLst 
   pTrnInfo->chkLen            = pTrnInfo->nextStop + GRP_LABEL_LIST_SIZE (0);                     // pGrpLblDataSet    

   if (!checkMsgLen (pTrnInfo, "getTrainInfo-2"))
   {
      return (FALSE);
   }

   devLabelsN2H (pTrnInfo->pDevLblLst);

   pTrnInfo->pGrpLblLst         = (T_IPT_GRP_LABEL_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pTrnInfo->pGrpLblLst->lblCnt = MD_N2H32 (pTrnInfo->pGrpLblLst->lblCnt);
   pTrnInfo->nextStop          += GRP_LABEL_LIST_SIZE (pTrnInfo->pGrpLblLst->lblCnt);              // pMcGrpLst
   pTrnInfo->chkLen            = pTrnInfo->nextStop + MC_GRP_LIST_SIZE (0);                        // pMcGrpDataSet    

   if (!checkMsgLen (pTrnInfo, "getTrainInfo-3"))
   {
      return (FALSE);
   }

   grpLabelsN2H (pTrnInfo->pGrpLblLst);

   pTrnInfo->pMcLst         = (T_IPT_MC_GRP_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pTrnInfo->pMcLst->grpCnt = MD_N2H32 (pTrnInfo->pMcLst->grpCnt);
   pTrnInfo->nextStop      += MC_GRP_LIST_SIZE (pTrnInfo->pMcLst->grpCnt);                         // pResQwLst
   pTrnInfo->chkLen         = pTrnInfo->nextStop + RES_QW_LIST_SIZE (0);                           // pResQwDataSet    

   if (!checkMsgLen (pTrnInfo, "getTrainInfo-4"))
   {
      return (FALSE);
   }

   mcGrpsN2H (pTrnInfo->pMcLst);

   pTrnInfo->pResQwLst           = (T_IPT_RES_QW_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pTrnInfo->pResQwLst->resQwCnt = MD_N2H32 (pTrnInfo->pResQwLst->resQwCnt);
   pTrnInfo->nextStop           += RES_QW_LIST_SIZE (pTrnInfo->pResQwLst->resQwCnt);               // pCstLst
   pTrnInfo->chkLen              = pTrnInfo->nextStop + CONSIST_LIST_SIZE (0);                     // pCstDataSet    

   if (!checkMsgLen (pTrnInfo, "getTrainInfo-5"))
   {
      return (FALSE);
   }

   resQwN2H (pTrnInfo->pResQwLst);

   pTrnInfo->pCstLst         = (T_IPT_CONSIST_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pTrnInfo->pCstLst->cstCnt = MD_N2H32 (pTrnInfo->pCstLst->cstCnt);
   pTrnInfo->nextStop        = pTrnInfo->chkLen;                                                   // pCstDataSet

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL getCstInfo (T_TRAIN_INFO*     pTrnInfo,
                              T_CST_INFO*       pCstInfo,
                              UINT32            cstNo)
{
   pCstInfo->pCst      = (T_IPT_CONSIST_DATA_SET *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pTrnInfo->nextStop += CONSIST_DATA_SET_MCLIST_OFFSET ();                                     // pMcGrpLst 
   pTrnInfo->chkLen    = pTrnInfo->nextStop + MC_GRP_LIST_SIZE (0);                             // pMcGrpDataSet    

   if (!checkMsgLen (pTrnInfo, "getCstInfo-1"))
   {
      return (FALSE);
   }

   cstDataN2H (pCstInfo->pCst, cstNo);

   pCstInfo->pMcLst         = (T_IPT_MC_GRP_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pCstInfo->pMcLst->grpCnt = MD_N2H32 (pCstInfo->pMcLst->grpCnt);
   pTrnInfo->nextStop      += MC_GRP_LIST_SIZE (pCstInfo->pMcLst->grpCnt);                      // pResQwLst 
   pTrnInfo->chkLen         = pTrnInfo->nextStop + RES_QW_LIST_SIZE (0);                        // pResQwDataSet    

   if (!checkMsgLen (pTrnInfo, "getCstInfo-2"))
   {
      return (FALSE);
   }

   mcGrpsN2H (pCstInfo->pMcLst);

   pCstInfo->pResQwLst           = (T_IPT_RES_QW_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pCstInfo->pResQwLst->resQwCnt = MD_N2H32 (pCstInfo->pResQwLst->resQwCnt);
   pTrnInfo->nextStop           += RES_QW_LIST_SIZE (pCstInfo->pResQwLst->resQwCnt);            // pCarLst
   pTrnInfo->chkLen              = pTrnInfo->nextStop + CAR_LIST_CAR_OFFSET ();                 // pCarDatSeta

   if (!checkMsgLen (pTrnInfo, "getCstInfo-3"))
   {
      return (FALSE);
   }

   resQwN2H (pCstInfo->pResQwLst);

   pCstInfo->pCarLst         = (T_IPT_CAR_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
   pCstInfo->pCarLst->carCnt = MD_N2H32 (pCstInfo->pCarLst->carCnt);
   pTrnInfo->nextStop        = pTrnInfo->chkLen;                                               // pCarDataSet 

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL getCstVarCnt (T_TRAIN_INFO*               pTrnInfo,
                                const T_CST_INFO*           pCstInfo,
                                T_DB_IPT_CST_DATA_SET*      pCst)
{
   UINT32                  carNo;

   pCst->carCnt      = pCstInfo->pCarLst->carCnt;
   pCst->carMcGrpCnt = 0;
   pCst->devCnt      = 0;

   for (carNo = 0; carNo < pCst->carCnt; carNo++)
   {
      T_IPT_CAR_DATA_SET*        pCar       = (T_IPT_CAR_DATA_SET *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
      T_IPT_MC_GRP_LIST*         pMcLst;
      T_IPT_RES_QW_LIST*         pResQwLst;
      T_IPT_DEV_LIST*            pDevLst;

      pTrnInfo->nextStop += CAR_DATA_SET_MCLIST_OFFSET ();                             // pMcGrpLst 
      pTrnInfo->chkLen    = pTrnInfo->nextStop + MC_GRP_LIST_SIZE (0);                 // pMcGrpDataSet    

      if (!checkMsgLen (pTrnInfo, "getCstVarCnt-1"))
      {
         return (FALSE);
      }

      carDataN2H (pCar, carNo);

      pMcLst              = (T_IPT_MC_GRP_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
      pMcLst->grpCnt      = MD_N2H32 (pMcLst->grpCnt);
      pTrnInfo->nextStop += MC_GRP_LIST_SIZE (pMcLst->grpCnt);                         // pResQwLst
      pTrnInfo->chkLen    = pTrnInfo->nextStop + RES_QW_LIST_SIZE (0);                 // pResQwDataSet   

      if (!checkMsgLen (pTrnInfo, "getCstVarCnt-2"))
      {
         return (FALSE);
      }

      mcGrpsN2H (pMcLst);

      pResQwLst           = (T_IPT_RES_QW_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
      pResQwLst->resQwCnt = MD_N2H32 (pResQwLst->resQwCnt);
      pTrnInfo->nextStop += RES_QW_LIST_SIZE (pResQwLst->resQwCnt);                       // pDevLst
      pTrnInfo->chkLen    = pTrnInfo->nextStop + DEV_LIST_DEV_DATA_OFFSET ();             // pDevDataSet

      if (!checkMsgLen (pTrnInfo, "getCstVarCnt-3"))
      {
         return (FALSE);
      }

      resQwN2H (pResQwLst);

      pDevLst         = (T_IPT_DEV_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
      pDevLst->devCnt = MD_N2H32 (pDevLst->devCnt);

      if (!devDataN2H (pTrnInfo))
      {
         return (FALSE);
      }

      pCst->carMcGrpCnt += pMcLst->grpCnt;
      pCst->devCnt      += pDevLst->devCnt;
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL addIptCst (T_TRAIN_INFO*         pTrnInfo,
                             const T_CST_INFO*     pCstInfo)
{
   T_DB_IPT_CST_DATA_SET      cst;
   UINT32                     nextStop = pTrnInfo->nextStop;      // save value for second pass

   if (!getCstVarCnt (pTrnInfo, pCstInfo, &cst))
   {
      return (FALSE);   /* Terminate buildTrain */
   }

   pTrnInfo->nextStop = nextStop;      // restore value for second pass  --> pCarData

   (void) tdcStrNCpy  (cst.cstId, pCstInfo->pCst->cstLbl, IPT_LABEL_SIZE);

   cst.pMcLst   = pCstInfo->pMcLst;
   cst.trnCstNo = pCstInfo->pCst->trnCstNo;
   cst.bIsLocal = pCstInfo->pCst->bIsLocal;
   cst.orient   = pCstInfo->pCst->orient;

   if (!dbIptAddConsist (&cst))
   {
      return (FALSE);   /* Terminate buildTrain */
   }
   else
   {
      UINT32      carNo;

      for (carNo = 0; carNo < cst.carCnt; carNo++)
      {
         T_IPT_CAR_DATA_SET*     pCar       = (T_IPT_CAR_DATA_SET *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
         T_IPT_RES_QW_LIST*      pResQwLst;
         T_IPT_DEV_LIST*         pDevLst;
         T_DB_IPT_DEV_DATA*      pDbDevs;

         pTrnInfo->nextStop += CAR_DATA_SET_SIZE (pCar->mcLst.grpCnt);                                // pResQwDataSet  
         pResQwLst          = (T_IPT_RES_QW_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
         pTrnInfo->nextStop += RES_QW_LIST_SIZE (pResQwLst->resQwCnt);                                // pDevDataSet
         pDevLst             = (T_IPT_DEV_LIST *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));
         pTrnInfo->nextStop += DEV_LIST_DEV_DATA_OFFSET ();                                           // pDevData
         pDbDevs             = (T_DB_IPT_DEV_DATA *) tdcAllocMem (DB_IPT_DEV_DATA_SIZE (pDevLst->devCnt));

         if (pDbDevs != NULL)
         {
            UINT32                     devNo;
            T_DB_IPT_CAR_DATA_SET      car;

            (void) tdcStrNCpy (car.cstId,    pCstInfo->pCst->cstLbl, IPT_LABEL_SIZE);
            (void) tdcStrNCpy (car.carId,    pCar->carLbl,           IPT_LABEL_SIZE);
            (void) tdcMemCpy  (car.uicIdent, pCar->uicID,            (UINT32) IPT_UIC_IDENTIFIER_CNT);

            car.pMcLst        = &pCar->mcLst;
            car.carTypeLblIdx = pCar->carTypeLblIdx;
            car.cstCarNo      = pCar->cstCarNo;
            car.trnOrient     = IPT_GET_CARDATA_TRN_ORIENT (pCar->TCrrrrrr);
            car.cstOrient     = IPT_GET_CARDATA_CST_ORIENT (pCar->TCrrrrrr);
            pDbDevs->devCnt   = pDevLst->devCnt;

            for (devNo = 0; devNo < pDevLst->devCnt; devNo++)
            {
               const T_IPT_DEV_DATA_SET*  pDev = (const T_IPT_DEV_DATA_SET *) ((void *) &(pTrnInfo->pIptMd[pTrnInfo->nextStop]));

               pDbDevs->dev[devNo].lblIdx = pDev->lblIdx;
               pDbDevs->dev[devNo].no     = pDev->no;
               pTrnInfo->nextStop        += DEV_DATA_SET_SIZE (pDev->resQwLst.resQwCnt);
            }

            car.pDevs = pDbDevs;

            if (!dbIptAddCar (&car))
            {
               DEBUG_WARN (MOD_MD, "Error adding car to new IptTrain");       /*@ -compdestroy */
               tdcFreeMem (pDbDevs);
               return (FALSE);                                                /*@ =compdestroy */
            }

            tdcFreeMem (pDbDevs);
         }
         else
         {
            return (FALSE);
         }
      }          
   }             

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL buildIptTrain (T_TRAIN_INFO*     pTrnInfo)
{
   T_DB_IPT_TRN_DATA       trn;
   T_IPTDIR_IPT_MD*        pIptMd = (T_IPTDIR_IPT_MD *) ((void *) pTrnInfo->pIptMd);

   trn.inaugState  = pIptMd->inaugState;
   trn.topoCnt     = pIptMd->topoCnt;
   trn.pCarTypeLst = pTrnInfo->pCarTypeLst;
   trn.pDevLblLst  = pTrnInfo->pDevLblLst;
   trn.pGrpLblLst  = pTrnInfo->pGrpLblLst;
   trn.pMcLst      = pTrnInfo->pMcLst;

   if (dbIptAddTrain (&trn))
   {
      UINT32      cstNo;

      DEBUG_INFO1 (MOD_MD, "IPT-MD: cstCnt=%d", pTrnInfo->pCstLst->cstCnt);

      for (cstNo = 0; cstNo < pTrnInfo->pCstLst->cstCnt; cstNo++)
      {
         T_CST_INFO     cstInfo;

         if (!getCstInfo (pTrnInfo, &cstInfo, cstNo))
         {
            return (FALSE);
         }
         if (!addIptCst (pTrnInfo, &cstInfo))
         {
            return (FALSE);
         }
      }

      if (pTrnInfo->chkLen == pTrnInfo->msgLen)
      {
         UINT16      hostId = (UINT16) ucAddrGetDevNo (tdcGetLocalIpAddr ());

         if (dbIptSetLocal (hostId))
         {
            return (dbIptCheckTrain ());
         }
         else
         {
            DEBUG_WARN (MOD_MD, "buildIptTrain: failed to call dbIptSetLocal");
         }
      }
      else
      {
         DEBUG_WARN2 (MOD_MD, "buildIptTrain: Telegram Size exceeds msgSize - %d - %d", pTrnInfo->chkLen, pTrnInfo->msgLen);
      }
   }

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

static void consumeIptMsgDataV2 (T_IPTDIR_IPT_MD*     pIptMd,
                                 UINT32               msgLen)
{
   static T_TDC_BOOL    bFirstTime = TRUE;

   if (msgLen > IPT_MD_CAR_TYPE_DATA_SET_OFFSET ())
   {
      pIptMd->inaugState = MD_N2H8 (pIptMd->inaugState);
      pIptMd->topoCnt    = MD_N2H8 (pIptMd->topoCnt);

      if (dbIptNewTrain (pIptMd->inaugState, pIptMd->topoCnt))     /*@ -compdestroy */
      {
         T_TRAIN_INFO      trnInfo;

         DEBUG_INFO2 (MOD_MD, "IPT-MD: iptInaugState=%d, iptTopoCnt=%d", pIptMd->inaugState, pIptMd->topoCnt);

         trnInfo.pIptMd   = (UINT8 *) pIptMd;
         trnInfo.msgLen   = msgLen;
         trnInfo.nextStop = IPT_MD_CAR_TYPE_LIST_OFFSET ();
         trnInfo.chkLen   = IPT_MD_CAR_TYPE_DATA_SET_OFFSET ();

         if (getTrainInfo (&trnInfo))
         {
            if (buildIptTrain (&trnInfo))
            {
               DEBUG_INFO (MOD_MD, "IPT-MD: Activating new train");

               (void) dbIptActivateNewTrain ();
               dbIptVerboseTrain     ();
               if (bFirstTime)
               {
                  tdcSetDebugLevel (tdcDbgLevelRun, NULL, NULL);
                  bFirstTime = FALSE;
               }
            }
            else
            {
               dbIptNoNewTrain ();
            }
         }
         else
         {
            DEBUG_WARN1 (MOD_MD, "Error parsing IPTDir IPT-MD message - msgLen (%d)", msgLen);
         }
      }    /*@ =compdestroy */
      else
      {
         DEBUG_INFO (MOD_MD, "Discarding IPTDir IPT-MD Telegram - already stored or old");
      }
   }
   else
   {
      DEBUG_WARN1 (MOD_MD, "Error parsing IPTDir IPT-MD message - msgLen (%d)", msgLen);
   }
}

/* ---------------------------------------------------------------------------- */

static void getUicGlobData (T_TDC_UIC_GLOB_DATA*     pGlobData,
                            T_IPTDIR_UIC_MD*         pIptMsg)
{
   int            i;

   pIptMsg->optPresent = MD_N2H32 (pIptMsg->optPresent);
   pIptMsg->OACrrrrr   = MD_N2H8  (pIptMsg->OACrrrrr);

   for (i = 0; i < IPT_UIC_CONF_POS_CNT; i++)
   {
      pGlobData->confPos[i] = MD_N2H8 (pIptMsg->confirmedPos[i]);
   }

   pGlobData->confPosAvail   = UIC_GET_TRNDATA_OPT_CONFPOS     (pIptMsg->optPresent);
   pGlobData->operatAvail    = UIC_GET_TRNDATA_OPT_OPERATOWNER (pIptMsg->optPresent);
   pGlobData->natApplAvail   = UIC_GET_TRNDATA_OPT_NATAPPL     (pIptMsg->optPresent);
   pGlobData->cstPropAvail   = UIC_GET_TRNDATA_OPT_CSTPROP     (pIptMsg->optPresent);
   pGlobData->carPropAvail   = UIC_GET_TRNDATA_OPT_CARPROP     (pIptMsg->optPresent);
   pGlobData->seatResNoAvail = UIC_GET_TRNDATA_OPT_SEATRES     (pIptMsg->optPresent);
   pGlobData->inaugFrameVer  = MD_N2H8 (pIptMsg->inaugFrameVer);
   pGlobData->rDataVer       = MD_N2H8 (pIptMsg->rdataVer);
   pGlobData->inaugState     = MD_N2H8 (pIptMsg->uicInaugState);
   pGlobData->topoCnt        = MD_N2H8 (pIptMsg->uicTopoCnt);
   pGlobData->orient         = UIC_GET_TRNDATA_ORIENT       (pIptMsg->OACrrrrr);
   pGlobData->notAllConf     = UIC_GET_TRNDATA_NOTALLCONF   (pIptMsg->OACrrrrr);
   pGlobData->confCancelled  = UIC_GET_TRNDATA_CONFCANCELED (pIptMsg->OACrrrrr);
   pGlobData->trnCarCnt      = MD_N2H32 (pIptMsg->uicCarDataSet.trnCarCnt);
}

/* ---------------------------------------------------------------------------- */

static void verboseBuf (const char*     pModname,
                        const char*     pFormat,
                        const UINT8*    pBuf,
                        int             bufLen)
{
   int      i;
   const int      maxItemPerLine = 8;
   char     line[80]       = "";
   char     text[50]       = "";

   for (i = 0; i < bufLen; i++)
   {
      int         j = i % maxItemPerLine;

      (void) tdcSNPrintf (&text[3 * j], (UINT32) (sizeof (text) - (3 * j)), " %02x", (unsigned int) pBuf[i]);

      if (j == (maxItemPerLine - 1))    // print Line
      {                                                              /*@ -formatconst */
         (void) tdcSNPrintf (line, (UINT32) sizeof (line), pFormat, text);  /*@ =formatconst */
         DEBUG_INFO  (pModname, line);
         text[0] = '\0';
      }
   }

   if ((i % maxItemPerLine) != 0)       // flush line
   {                                                                 /*@ -formatconst */
      (void) tdcSNPrintf (line, (UINT32) sizeof (line), pFormat, text);     /*@ =formatconst */
      DEBUG_INFO  (pModname, line);
   }
}

/* ---------------------------------------------------------------------------- */

static void verboseUicGlobData (const T_TDC_UIC_GLOB_DATA*    pGlobData)
{
   UINT32         optPresent = UIC_SET_TRNDATA_OPT      (pGlobData->confPosAvail,
                                                         pGlobData->operatAvail,
                                                         pGlobData->natApplAvail,
                                                         pGlobData->cstPropAvail,
                                                         pGlobData->carPropAvail,
                                                         pGlobData->seatResNoAvail);
   UINT8          OACrrrrr   = UIC_SET_TRNDATA_OACRRRRR (pGlobData->orient, 
                                                         pGlobData->notAllConf, 
                                                         pGlobData->confCancelled);

   DEBUG_INFO3 (MOD_MD, "UIC-MD: optPres=x%08x, inaugFrameVer=%d, rdataVer=%d", 
                        optPresent, pGlobData->inaugFrameVer, pGlobData->rDataVer);
   DEBUG_INFO3 (MOD_MD, "UIC-MD: inaugState=%d, topoCnt=%d, OACrrrrr=x%02x", 
                        pGlobData->inaugState, pGlobData->topoCnt, OACrrrrr);
   verboseBuf  (MOD_MD, "UIC-MD: confPos=%s",   pGlobData->confPos, IPT_UIC_CONF_POS_CNT);
   DEBUG_INFO1 (MOD_MD, "UIC-MD: trnCarCnt=%d", pGlobData->trnCarCnt);
}

/* ---------------------------------------------------------------------------- */

static void getUicCarData (T_TDC_UIC_CAR_DATA*     pCarData,
                           T_IPTDIR_UIC_MD*        pIptMsg,
                           UINT32                  carNo)
{
   T_IPT_UIC_CAR_DATA_SET*       pCar = &pIptMsg->uicCarDataSet.car[carNo];
   int                           j;

   pCar->TCLRrrrr = MD_N2H8 (pCar->TCLRrrrr);

   for (j = 0; j < IPT_MAX_UIC_CST_NO; j++)
   {
      pCarData->cstProp[j] = MD_N2H8 (pCar->uicCstProp[j]);
   }
   for (j = 0; j < IPT_UIC_CAR_PROPERTY_CNT; j++)
   {
      pCarData->carProp[j] = MD_N2H8 (pCar->uicCarProp[j]);
   }
   for (j = 0; j < IPT_UIC_IDENTIFIER_CNT; j++)
   {
      pCarData->uicIdent[j] = MD_N2H8 (pCar->uicIdent[j]);
   }

   pCarData->cstSeqNo      = MD_N2H8  (pCar->uicCstNo);
   pCarData->carSeqNo      = MD_N2H8  (pCar->uicCarSeqNo);
   pCarData->seatResNo     = MD_N2H16 (pCar->seatResNo);
   pCarData->contrCarCnt   = MD_N2H8  (pCar->contrCarCnt);
   pCarData->operat        = MD_N2H8  (pCar->operat);
   pCarData->owner         = MD_N2H8  (pCar->owner);
   pCarData->natAppl       = MD_N2H8  (pCar->natAppl);
   pCarData->natVer        = MD_N2H8  (pCar->natVer);
   pCarData->trnOrient     = UIC_GET_CARDATA_TRN_ORIENT (pCar->TCLRrrrr);
   pCarData->cstOrient     = UIC_GET_CARDATA_CST_ORIENT (pCar->TCLRrrrr);
   pCarData->isLeading     = UIC_GET_CARDATA_LEADING    (pCar->TCLRrrrr);
   pCarData->isLeadRequ    = UIC_GET_CARDATA_LEADREQU   (pCar->TCLRrrrr);
   pCarData->trnSwInCarCnt = MD_N2H8 (pCar->trnSwInCarCnt);
}

/* ---------------------------------------------------------------------------- */

static void verboseUicCarData (const T_TDC_UIC_CAR_DATA*     pCarData, UINT32    carNo)
{
   UINT8          TCLRrrrr = UIC_SET_CARDATA_TCLRRRRR (pCarData->trnOrient, 
                                                       pCarData->cstOrient, 
                                                       pCarData->isLeading, 
                                                       pCarData->isLeadRequ);

   DEBUG_INFO1 (MOD_MD, "UIC-MD: car%02d", carNo + 1);
   DEBUG_INFO3 (MOD_MD, "UIC-MD: uicCstNo=%d, contrCarCnt=%d, uicCarSeqNo=%d",
                        pCarData->cstSeqNo, pCarData->contrCarCnt, pCarData->carSeqNo);
   DEBUG_INFO4 (MOD_MD, "UIC-MD: operat=%d, owner=%d, natAppl=%d, natVer=%d",
                        pCarData->operat, pCarData->owner, pCarData->natAppl, pCarData->natVer);
   DEBUG_INFO3 (MOD_MD, "UIC-MD: seatResNo=%d, TCLRrrrr=x%02x, trnSwInCarCnt=%d",
                        pCarData->seatResNo, TCLRrrrr, pCarData->trnSwInCarCnt);

   verboseBuf (MOD_MD, "UIC-MD: uicCstProp=%s", pCarData->cstProp,  IPT_MAX_UIC_CST_NO);
   verboseBuf (MOD_MD, "UIC-MD: uicIdent=%s",   pCarData->uicIdent, IPT_UIC_IDENTIFIER_CNT);
   verboseBuf (MOD_MD, "UIC-MD: uicCarProp=%s", pCarData->carProp,  IPT_UIC_CAR_PROPERTY_CNT);
}

/* ---------------------------------------------------------------------------- */

static void consumeUicMsgDataV2 (T_IPTDIR_UIC_MD*     pIptMsg,
                                 UINT32               msgLen)
{
   T_TDC_UIC_GLOB_DATA     globData;

   if (msgLen < UIC_MD_SIZE (0))
   {
      DEBUG_WARN1 (MOD_MD, "Error parsing IPTDir UIC-MD message - msgLen (%d)", msgLen);
      return;
   }

   getUicGlobData (&globData, pIptMsg);

   if (msgLen != UIC_MD_SIZE (globData.trnCarCnt))
   {
      DEBUG_WARN2 (MOD_MD, "Error parsing IPTDir UIC-MD message - msgLen(%d) - carCnt(%d)",
                            msgLen, (int) globData.trnCarCnt);
      return;
   }

   /* Telegram - Size is ok ! */

   if (dbUicNewTrain (globData.inaugState, globData.topoCnt))
   {
      verboseUicGlobData (&globData);

      if (dbUicAddGlobData (&globData))
      {
         int      carNo;

         for (carNo = 0; carNo < (int) globData.trnCarCnt; carNo++)
         {
            T_TDC_UIC_CAR_DATA            carData;

            getUicCarData     (&carData, pIptMsg, carNo);
            verboseUicCarData (&carData, carNo);

            if (!dbUicAddCarData (&carData, (UINT8) carNo))
            {
               DEBUG_WARN (MOD_MD, "Failed to call dbUicAddCarData");
            }
         }

         (void) dbUicActivateNewTrain ();
      }
   }
   else
   {
      DEBUG_INFO (MOD_MD, "Discarding IPTDir UIC-MD Telegram - already stored or old");
   }
}

/* ---------------------------------------------------------------------------- */

void tdcRequIptMsgData (void)
{
   iptDirRequest (IPT_IPTDIR_REQ_SEND_IPT);
}

/* ---------------------------------------------------------------------------- */

void tdcRequUicMsgData (void)
{
   iptDirRequest (IPT_IPTDIR_REQ_SEND_UIC);
}

/* ---------------------------------------------------------------------------- */

void tdcTMsgDataInit (void*       pArgV)
{
   pArgV = pArgV;
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcTerminateTMsgData (void)
{
   int      i;

   DEBUG_WARN (MOD_MAIN, "Termination requested");

   for (i = 0; i < 10; i++, tdcSleep (100))
   {
      if (tdcMutexLock (MOD_MAIN, taskIdMutexId) == TDC_MUTEX_OK)
      {
         if (tdcThreadIdTab[T_MSGDATA_INDEX] == NULL)
         {
            (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
            DEBUG_WARN     (MOD_MAIN, "Termination finished");
            return (TRUE);
         }

         (void) tdcMutexUnlock (MOD_MAIN, taskIdMutexId);
      }
   }

   DEBUG_WARN (MOD_MAIN, "Termination failed");

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

void tdcTMsgData (void*    pArgV)
{
   TDC_UNUSED(pArgV)

   DEBUG_INFO (MOD_MD, CONVERSION_INFO);

   for (;;)
   {
      T_READ_MD      readMd;

      if (bTerminate)
      {
         DEBUG_INFO (MOD_MD, "Aborting tdcTMsgData() due to Terminate-Request");
         break;
      }

      if (tdcReadMsgData (MOD_MD, &readMd))
      {
         if (readMd.msgLen >= ((UINT32) sizeof (UINT32)))
         {
            switch (readMd.msgDataType)
            {
               case RECV_IPT_MSG_DATA:
               {
                  T_IPTDIR_IPT_MD*     pMsgData = (T_IPTDIR_IPT_MD *) ((void *) readMd.pMsgData);
                  UINT32               protVer  = tdcN2H32 (pMsgData->protVer);

                  switch (PROT_GET_VERSION (protVer))
                  {
                     case 2:
                     {
                        consumeIptMsgDataV2 (pMsgData, readMd.msgLen);
                        break;
                     }
                     default:
                     {
                        DEBUG_WARN1 (MOD_MD, "received IPTDir-IPT MD telegram with invalid Ver (x%08x)", protVer);
                        break;
                     }
                  }

                  break;
               }
               case RECV_UIC_MSG_DATA:
               {
                  T_IPTDIR_UIC_MD*  pMsgData = (T_IPTDIR_UIC_MD *) ((void *) readMd.pMsgData);
                  UINT32            protVer  = tdcN2H32 (pMsgData->protVer);

                  switch (PROT_GET_VERSION (protVer))
                  {
                     case 2:
                     {
                        consumeUicMsgDataV2 (pMsgData, readMd.msgLen);
                        break;
                     }
                     default:
                     {
                        DEBUG_WARN1 (MOD_MD, "received IPTDir-UIC MD telegram with invalid Ver (x%08x)", protVer);
                        break;
                     }
                  }

                  break;
               }
               default:
               {
                  DEBUG_WARN1 (MOD_MD, "Error reading message Data - Unknown Message Data Type (%d)", readMd.msgDataType);
                  break;
               }
            }
         }
         else
         {
            DEBUG_WARN1 (MOD_MD, "Error reading message Data - Message too short (%d)", readMd.msgLen);
         }

         (void) tdcFreeMDBuf (readMd.pMsgData);
      }
      else
      {
         DEBUG_WARN (MOD_MD, "Error reading message Data");
         tdcSleep (250);
      }           /*@ -compdestroy */
   }              /*@ =compdestroy */

   tdcSleep       (250);       /* Wait for 250 msecs - if tdcDestroy was called, listener and queue will safely be removed then*/
   tdcTerminateMD (MOD_MD);
}

/* ---------------------------------------------------------------------------- */






