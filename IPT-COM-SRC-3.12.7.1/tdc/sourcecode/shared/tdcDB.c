/*
* $Id: tdcDB.c 11184 2009-05-14 13:28:06Z mritz $
*
* DESCRIPTION    Database for IP-Train Directory Client (TDC)
*                internal routines
*
* AUTHOR         M.Ritz      PPC/EBT
*
* REMARKS
*
* DEPENDENCIES   Either the switch LINUX or WIN32 has to be set
*
*
* All rights reserved. Reproduction, modification, use or disclosure
* to third parties without express authority is forbidden.
* Copyright Bombardier Transportation GmbH, Germany, 2002.
*/


/* ----------------------------------------------------------------------------
* ----------------------------------------------------------------------------
* ---------------------------------------------------------------------------- */

#include "tdc.h"
#include "tdcInit.h"
#include "tdcConfig.h"
#include "tdcDB.h"
#include "tdcDBpriv.h"

/* ---------------------------------------------------------------------------- */

T_DB_IPT_TRAIN             dbCurIptTrn;
T_DB_IPT_TRAIN             dbShadowIptTrn;

T_DB_UIC_TRAIN             dbCurUicTrn;
T_DB_UIC_TRAIN             dbShadowUicTrn;

T_DB_DYNAMIC_DATA          dbDynData;

/* ---------------------------------------------------------------------------- */

T_ANYCAR_TAB               dbAnyCarTab[ANYCAR_INDEX_TAB_SIZE];

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL          assertLCst        (/*@null@*/ const T_DB_CONSIST*    pCst);
static T_TDC_BOOL          assertLCar        (/*@null@*/ const T_DB_CONSIST*    pCst,
                                              /*@null@*/ const T_DB_CAR*        pCar);
/*@null@*/ /*@shared@*/
static T_DB_CONSIST*       delConsist        (T_DB_CONSIST*                      pCst);

static void                verboseCst        (const T_DB_IPT_TRAIN*              pIptTrn,
                                              const T_DB_CONSIST*                pCst,
                                              UINT8                              cstNo);

static void                verboseCarList    (const T_DB_IPT_TRAIN*              pIptTrn,
                                              const T_DB_CONSIST*                pCst,
                                              UINT8                              cstNo);
static void                verboseMcGrp      (const T_DB_IPT_TRAIN*              pIptTrn,
                                              const T_DB_MC_GRP_LIST*            pMc,
                                              UINT8                              cstCarNo);
static void                verboseDevList    (const T_DB_IPT_TRAIN*              pIptTrn,
                                              const T_DB_DEV_LIST*               pDevList,
                                              UINT8                              cstCarNo);

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL assertLCst (const T_DB_CONSIST*     pCst)
{
   if (pCst != NULL)
   {
      if (pCst == dbCurIptTrn.pCstRoot)
      {
         return ((T_TDC_BOOL) pCst->bIsLocal);
      }
   }

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL assertLCar (const T_DB_CONSIST*  pCst,
                              const T_DB_CAR*      pCar)
{
   if (assertLCst (pCst))
   {
      if (dbCurIptTrn.pLocCar != NULL)
      {
         if (pCar == dbCurIptTrn.pLocCar)
         {
            return (TRUE);
         }
      }
   }

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

static T_DB_CONSIST* delConsist (T_DB_CONSIST*   pCst)
{
   T_DB_CONSIST*      pNext = pCst->pNext;

   tdcFreeMem (pCst);

   return (pNext);
}

/* ---------------------------------------------------------------------------- */

static void verboseCst (const T_DB_IPT_TRAIN*   pIptTrn,
                        const T_DB_CONSIST*     pCst, 
                        UINT8                   cstNo)
{
   char     textBuf[100];

   (void) tdcSNPrintf (textBuf, (unsigned) sizeof (textBuf),
                       "cst[%d] - cstId=%s, cstNo=%s", (int) cstNo, pCst->cstId, pCst->cstNoLbl);
   textBuf[sizeof (textBuf) - 1] = '\0';
   DEBUG_INFO (MOD_DB, textBuf);

   (void) tdcSNPrintf (textBuf, (unsigned) sizeof (textBuf),
                       "cst[%d] - trainCstNo=%d, bLocal=%d, orient=%d",
                       (int) cstNo, (int) pCst->trnCstNo, (int) pCst->bIsLocal, (int) pCst->orient);
   textBuf[sizeof (textBuf) - 1] = '\0';
   DEBUG_INFO (MOD_DB, textBuf);

   verboseMcGrp   (pIptTrn, pCst->pCstMcLst, (UINT8) 0);
   verboseCarList (pIptTrn, pCst, cstNo);
}

/* ---------------------------------------------------------------------------- */

static void verboseCarList (const T_DB_IPT_TRAIN*     pIptTrn,
                            const T_DB_CONSIST*       pCst, 
                            UINT8                     cstNo)         
{
   UINT32                  carNo;
   const T_DB_CAR_LIST*    pCarLst = pCst->pCarLst;

   DEBUG_INFO3 (MOD_DB, "cst[%2d] - carCnt=%d, addCarIdx=%d", cstNo, pCarLst->cnt, pCarLst->addIdx);

   for (carNo = 0; carNo < pCarLst->cnt; carNo++)
   {
      char              textBuf[120];
      const T_DB_CAR*   pCar = &pCarLst->car[carNo];

      (void) tdcSNPrintf (textBuf, (unsigned) sizeof (textBuf),
                          "car[%d] - carId=%s, carNo=%s, carType=%s, uicId=x%02x%02x%02x%02x%02x",
                          carNo + 1,
                          pCar->carId,
                          pCar->carNoLbl,
                          pIptTrn->carTypeLst.pLbl[pCar->carTypeLblIdx],
                          (int) pCar->uicIdent[0],
                          (int) pCar->uicIdent[1],
                          (int) pCar->uicIdent[2],
                          (int) pCar->uicIdent[3],
                          (int) pCar->uicIdent[4]);
      textBuf[sizeof (textBuf) - 1] = '\0';

      DEBUG_INFO (MOD_DB, textBuf);

      (void) tdcSNPrintf (textBuf, (unsigned) sizeof (textBuf),
                          "car[%d] - cstCarNo=%d, trnOrient=%d, cstOrient=%d",
                          carNo + 1, (int) pCar->cstCarNo, (int) pCar->trnOrient, (int) pCar->cstOrient);
      textBuf[sizeof (textBuf) - 1] = '\0';

      DEBUG_INFO (MOD_DB, textBuf);

      verboseMcGrp   (pIptTrn, pCst->pCarMcLst, pCar->cstCarNo);
      verboseDevList (pIptTrn, pCst->pDevLst,   pCar->cstCarNo);
   }
}

/* ---------------------------------------------------------------------------- */

static void verboseMcGrp (const T_DB_IPT_TRAIN*       pIptTrn,
                          const T_DB_MC_GRP_LIST*     pMc,
                          UINT8                       cstCarNo)
{
   const char*    pGrpType = (cstCarNo == (UINT8) 0) ? "cstMcGrp" : "carMcGrp";
   UINT32         grpNo;

   for (grpNo = 0; grpNo < pMc->cnt; grpNo++)
   {
      if (pMc->grp[grpNo].cstCarNo == cstCarNo)
      {
         char           textBuf[100];

         (void) tdcSNPrintf (textBuf, (unsigned) sizeof (textBuf),
                             "%s[%d] - grpLabel(%s), grpNo(%d)",
                             pGrpType, grpNo, 
                             pIptTrn->grpLblLst.pLbl[pMc->grp[grpNo].lblIdx],
                             pMc->grp[grpNo].no);
         textBuf[sizeof (textBuf) - 1] = '\0';
         DEBUG_INFO (MOD_DB, textBuf);
      }
   }
}

/* ---------------------------------------------------------------------------- */

static void verboseDevList (const T_DB_IPT_TRAIN*     pIptTrn,
                            const T_DB_DEV_LIST*      pDevList, 
                            UINT8                     cstCarNo)
{
   char                    textBuf[100];
   UINT32                  devNo;

   (void) tdcSNPrintf (textBuf, (UINT32) sizeof (textBuf),
                       "cst[%d] - devCnt=%2d, addDevIdx=%d", 
                       (int) cstCarNo, (int) pDevList->cnt, (int) pDevList->addIdx);
   textBuf[sizeof (textBuf) - 1] = '\0';
   DEBUG_INFO  (MOD_DB, textBuf);

   for (devNo = 0; devNo < pDevList->cnt; devNo++)
   {
      if (pDevList->dev[devNo].cstCarNo == cstCarNo)
      {
         (void) tdcSNPrintf (textBuf, (unsigned) sizeof (textBuf),
                             "dev[%2d] - cstCarNo=%d, hostId=%4d, devId=%s",
                             devNo,
                             (int) pDevList->dev[devNo].cstCarNo,
                             pDevList->dev[devNo].hostId,
                             pIptTrn->devLblLst.pLbl[pDevList->dev[devNo].lblIdx]);
         textBuf[sizeof (textBuf) - 1] = '\0';
         DEBUG_INFO (MOD_DB, textBuf);
      }
   }
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

const T_DB_CONSIST* findCstByCstLbl (const T_IPT_LABEL   cst)
{
   const T_DB_CONSIST*      pCst;

   /* find consist in List */

   if (tdcStrICmp (cst, localCst) == 0)
   {
      if (assertLCst (dbCurIptTrn.pCstRoot))
      {
         return (dbCurIptTrn.pCstRoot);
      }

      return (NULL);
   }

   for (pCst = dbCurIptTrn.pCstRoot; pCst != NULL; pCst = pCst->pNext)
   {
      if (    (tdcStrICmp (pCst->cstId,    cst) == 0)
           || (tdcStrICmp (pCst->cstNoLbl, cst) == 0)
         )
      {
         return (pCst);
      }
   }

   return (NULL);
}

/* ---------------------------------------------------------------------------- */

const T_DB_CAR* findCarByCarLbl (const T_DB_CONSIST*   pCst, const T_IPT_LABEL   car)
{
   UINT32            carNo;
   const T_DB_CAR*   pCar;

   if (tdcStrICmp (car, localCar) == 0)
   {
      if (assertLCst (pCst))
      {
         return (dbCurIptTrn.pLocCar);
      }

      return (NULL);
   }

   for (carNo = 0, pCar = pCst->pCarLst->car; carNo < pCst->pCarLst->cnt; carNo++, pCar++)
   {
      if (    (tdcStrICmp (pCar->carId,    car) == 0)
           || (tdcStrICmp (pCar->carNoLbl, car) == 0)
         )
      {
         return (pCar);
      }
   }

   return (NULL);
}

/* ---------------------------------------------------------------------------- */

const T_DB_CAR* findCarByLbls (const T_DB_CONSIST**      ppCst,
                               const T_IPT_LABEL         cstLbl,
                               const T_IPT_LABEL         carLbl)
{
   const T_DB_CAR*         pCar = NULL;
   const T_DB_CONSIST*     pCst = NULL;

   if (    (cstLbl    == NULL)
        || (cstLbl[0] == '\0')
      )
   {
      for (pCst = dbCurIptTrn.pCstRoot; pCst != NULL; pCst = pCst->pNext)
      {
         if ((pCar = findCarByCarLbl (pCst, carLbl)) != NULL)
         {
            break;
         }
      }
   }
   else
   {
      if ((pCst = findCstByCstLbl (cstLbl)) != NULL)
      {
         pCar = findCarByCarLbl (pCst, carLbl);
      }
   }

   *ppCst = pCst;
   return (pCar);
}

/* ---------------------------------------------------------------------------- */

const T_DB_DEVICE* findDevByLDevLbl (const T_DB_CONSIST*    pCst,
                                     const T_DB_CAR*        pCar,
                                     const T_IPT_LABEL      dev)
{
   if (tdcStrICmp (dev, localDev) == 0)
   {
      if (assertLCar (pCst, pCar))
      {                              
         return (dbCurIptTrn.pLocDev);
      }

      return (NULL);
   }

   if (pCst != NULL)
   {
      UINT32            devNo;

      for (devNo = 0; devNo < pCst->pDevLst->cnt; devNo++)
      {
         if (tdcStrICmp (dbCurIptTrn.devLblLst.pLbl[pCst->pDevLst->dev[devNo].lblIdx], dev) == 0)
         {
            if (pCst->pDevLst->dev[devNo].cstCarNo == pCar->cstCarNo)
            {
               return (&pCst->pDevLst->dev[devNo]);
            }
         }
      }
   }

   return (NULL);
}

/* ---------------------------------------------------------------------------- */

const T_DB_MC_GRP* findGrpByLbl (const T_DB_MC_GRP_LIST*    pMcLst,
                                 const T_IPT_LABEL          grpLabel,
                                 const UINT8*               pCstCarNo)
{
   UINT32      i;

   for (i = 0; i < pMcLst->cnt; i++)
   {
      if (tdcStrICmp (grpLabel, dbCurIptTrn.grpLblLst.pLbl[pMcLst->grp[i].lblIdx]) == 0)
      {
         if (pCstCarNo == NULL)
         {
            return (&pMcLst->grp[i]);
         }
         else
         {
            if (pMcLst->grp[i].cstCarNo == (*pCstCarNo))
            {
               return (&pMcLst->grp[i]);
            }
         }
      }
   }

   return (NULL);
}

/* ---------------------------------------------------------------------------- */

const T_DB_MC_GRP* findGrpByGrpNo (const T_DB_MC_GRP_LIST*  pMcLst,
                                         UINT16             grpNo,
                                   const UINT8*             pCstCarNo)
{
   UINT32      i;

   for (i = 0; i < pMcLst->cnt; i++)
   {
      if (pMcLst->grp[i].no == grpNo)
      {
         if (pCstCarNo == NULL)
         {
            return (&pMcLst->grp[i]);
         }
         else
         {
            if (pMcLst->grp[i].cstCarNo == (*pCstCarNo))
            {
               return (&pMcLst->grp[i]);
            }
         }
      }
   }

   return (NULL);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL findIptUicIdent(const T_UIC_IDENT     uicIdent,
                                 T_IPT_LABEL     cstLbl,
                                 T_IPT_LABEL     carLbl)
{
   const T_DB_CONSIST*     pIptCst;

   for (pIptCst = dbCurIptTrn.pCstRoot; pIptCst != NULL; pIptCst = pIptCst->pNext)
   {
       UINT32              carNo;
       const T_DB_CAR*     pIptCar = pIptCst->pCarLst->car;

       for (carNo = 0; carNo < pIptCst->pCarLst->cnt; carNo++, pIptCar++)
       {
          if (tdcMemCmp (pIptCar->uicIdent, uicIdent, IPT_UIC_IDENTIFIER_CNT) == 0)
          {
             (void) tdcStrNCpy (carLbl, pIptCar->carId, IPT_LABEL_SIZE);
            (void)  tdcStrNCpy (cstLbl, pIptCst->cstId, IPT_LABEL_SIZE);
             return (TRUE);
          }
       }
   }

   return (FALSE);
}

/* -------------------------------------------------------------------------- */

void dbInitializeDB (void*      pArgV)
{
   TDC_UNUSED (pArgV)

   tdcMemClear (&dbDynData,      (UINT32) sizeof (T_DB_DYNAMIC_DATA));
   tdcMemClear (&dbCurIptTrn,    (UINT32) sizeof (T_DB_IPT_TRAIN));
   tdcMemClear (&dbShadowIptTrn, (UINT32) sizeof (T_DB_IPT_TRAIN));
   tdcMemClear (&dbCurUicTrn,    (UINT32) sizeof (T_DB_UIC_TRAIN));
   tdcMemClear (&dbShadowUicTrn, (UINT32) sizeof (T_DB_UIC_TRAIN));

   tdcMemClear (dbAnyCarTab, (UINT32) sizeof (dbAnyCarTab));
   dbAnyCarTab[CMS_ANYCAR_IDX].hostId = (UINT16) 1;        // 10.0.0.1)

   dbDynData.tbType = TDC_IPT_TBTYPE_ETB;                   // Will be overwritten by Process data

   (void) tdcStrNCpy (dbAnyCarTab[CMS_ANYCAR_IDX].devId,          cmsAnyCar,          IPT_LABEL_SIZE);
   (void) tdcStrNCpy (dbAnyCarTab[IPTDIRSERVER_ANYCAR_IDX].devId, iptDirServerAnyCar, IPT_LABEL_SIZE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbGetIptDirGateway (UINT8*           pTbType,
                               T_IPT_IP_ADDR*   pGatewayAddr)
{
   T_TDC_BOOL     bOK = FALSE;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      *pTbType      = dbDynData.tbType;
      *pGatewayAddr = dbDynData.gatewayAddr;
      bOK           = TRUE;

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (bOK);
}

/* ---------------------------------------------------------------------------- */

void dbSetIptDirGateway (UINT8            tbType,
                         T_IPT_IP_ADDR    gatewayAddr)
{
   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbDynData.tbType      = tbType;
      dbDynData.gatewayAddr = gatewayAddr;

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }
}

/* ---------------------------------------------------------------------------- */

void dbSetIptDirServerHostId (UINT16            hostId)
{
   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbAnyCarTab[IPTDIRSERVER_ANYCAR_IDX].hostId = hostId;

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbDeleteDB (void)
{
   T_TDC_BOOL     bOk          = FALSE;
   const char*    pSuccessText = "Termination failed";

   DEBUG_WARN (MOD_DB, "Termination requested");

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      delUicTrain (&dbCurUicTrn,    TRUE);
      delUicTrain (&dbShadowUicTrn, TRUE);
      delIptTrain (&dbCurIptTrn,    TRUE);
      delIptTrain (&dbShadowIptTrn, TRUE);

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);

      bOk          = TRUE;
      pSuccessText = "Termination finished";
   }

   DEBUG_WARN (MOD_DB, pSuccessText);

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

void delIptTrain (T_DB_IPT_TRAIN*  pTrn,
                  T_TDC_BOOL       bInclLocal)          /* Include local consist ? */
{
   if (pTrn->pCstRoot != NULL)      /* At least one element */
   {
      T_DB_CONSIST*     pDelCst;

      /* preserve local consist for the moment */

      for (pDelCst = pTrn->pCstRoot->pNext; pDelCst != NULL; pDelCst = delConsist (pDelCst))
      {
      }

      if (bInclLocal)
      {
         (void) delConsist (pTrn->pCstRoot);
                tdcFreeMem (pTrn->pLblAndGrpBuf);
         (void) tdcMemSet  (pTrn, 0, (UINT32) sizeof (T_DB_IPT_TRAIN));
      }
      else
      {                                                              /*@ -nullderef */
         pTrn->pCstRoot->pNext    = NULL;
         pTrn->pCstRoot->trnCstNo = 1;

         (void) tdcStrCpy (pTrn->pCstRoot->cstNoLbl, "cst01");       /*@ =nullderef */
      }
   }
}

/* ---------------------------------------------------------------------------- */

void delUicTrain (T_DB_UIC_TRAIN*  pTrain,
                  T_TDC_BOOL       bInclLocal)    /* Include local consist ? */
{
   if (!bInclLocal)
   {
   }

   tdcFreeMem  (pTrain->pCarData);
   tdcMemClear (pTrain, (UINT32) sizeof (T_DB_UIC_TRAIN));
}

/* ---------------------------------------------------------------------------- */

const T_DB_CONSIST* findCstByCstNo (UINT8       trnCstNo)
{
   if (trnCstNo == (UINT8) 0)         /* 'lCst' */
   {
      if (assertLCst (dbCurIptTrn.pCstRoot))
      {                                   
         return (dbCurIptTrn.pCstRoot);   
      }
   }
   else
   {
      const T_DB_CONSIST*     pCst = NULL;

      for (pCst = dbCurIptTrn.pCstRoot; pCst != NULL; pCst = pCst->pNext)
      {
         if (pCst->trnCstNo == (UINT16) trnCstNo)
         {                         
            return (pCst);         
         }
      }
   }

   return (NULL);
}

/* ---------------------------------------------------------------------------- */

const T_DB_CAR* findCarByCarNo (const T_DB_CONSIST*    pCst,
                                UINT8                  cstCarNo)
{
   UINT32      i;

   if (cstCarNo == (UINT8) 0)         /* 'lCar' */
   {
      if (!assertLCst (pCst))
      {                                   
         return (NULL);   
      }

      return (dbCurIptTrn.pLocCar);
   }

   for (i = 0; i < pCst->pCarLst->cnt; i++)
   {
      if (pCst->pCarLst->car[i].cstCarNo == cstCarNo)
      {
         return (&pCst->pCarLst->car[i]);
      }
   }

   return (NULL);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

void verboseIptTrn (const T_DB_IPT_TRAIN*       pIptTrn)
{
   char                 textBuf[100];
   UINT32               cstNo;
   UINT32               trnMcGrpNo;
   const T_DB_CONSIST*  pCst;

   DEBUG_INFO (MOD_DB, "Local IPT-Train:");

   (void) tdcSNPrintf (textBuf, (unsigned) sizeof (textBuf),
                       "inaugState=%d, topCnt=%d, lCarId=%s, lDevId=%s",
                       (int) pIptTrn->inaugState, (int) pIptTrn->topoCnt,
                       (pIptTrn->pLocCar != NULL) ? pIptTrn->pLocCar->carId : " ? ",
                       (pIptTrn->pLocDev != NULL) ? pIptTrn->devLblLst.pLbl[pIptTrn->pLocDev->lblIdx] : " ? ");
   textBuf[sizeof (textBuf) - 1] = '\0';

   DEBUG_INFO (MOD_DB, textBuf);

   for (trnMcGrpNo = 0; trnMcGrpNo < pIptTrn->mcLst.grpCnt; trnMcGrpNo++)
   {
      (void) tdcSNPrintf (textBuf, (unsigned) sizeof (textBuf),
                          "trnMcGrp[%d] - grpLabel(%s), grpNo(%d)",
                          trnMcGrpNo, 
                          pIptTrn->grpLblLst.pLbl[pIptTrn->mcLst.pGrp[trnMcGrpNo].lblIdx],
                          pIptTrn->mcLst.pGrp[trnMcGrpNo].no);
      textBuf[sizeof (textBuf) - 1] = '\0';
      DEBUG_INFO (MOD_DB, textBuf);
   }

   for (pCst = pIptTrn->pCstRoot, cstNo = (UINT32) 1; pCst != NULL; pCst = pCst->pNext, cstNo++)
   {
      verboseCst (pIptTrn, pCst, (UINT8) cstNo);
   }
}

