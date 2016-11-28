/*
 *  $Id: tdcDBipt.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    Database for IP-Train Directory Client (TDC) IPT data handling
 *
 *  AUTHOR         Manfred Ritz
 *
 *  REMARKS
 *
 *  DEPENDENCIES   Either the switch VXWORKS, INTEGRITY, LINUX or WIN32 has to be set
 *
 *  MODIFICATIONS (log starts 2010-08-11)
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *          Adding corrections for TÜV-Assessment
 *
 *  CR-590  (Gerhard Weiss, 2010-08-26)
 *          Removed a problem if grpAll.aCar.lCst is resolved.
 *          Current topocnt wasn't retrieved, so resolving this URI resulted
 *          in TDC_WRONG_TOPOCOUNT if the requested topocount isn't 0
 *
 *  All rights reserved. Reproduction, modification, use or disclosure
 *  to third parties without express authority is forbidden.
 *  Copyright Bombardier Transportation GmbH, Germany, 2002-2012.
 *
 */


/* ----------------------------------------------------------------------------
* ----------------------------------------------------------------------------
* ---------------------------------------------------------------------------- */

#include "tdc.h"
#include "tdcInit.h"
#include "tdcConfig.h"
#include "tdcDB.h"
#include "tdcDBpriv.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static T_DB_RESULT getTrnMcLbls (UINT16              grpNo,
                                 T_URI_LABELS*       pUriLabels,
                                 UINT8*              pTopoCnt);
static T_DB_RESULT getCstMcLbls (UINT8               trnCstNo,
                                 UINT16              grpNo,
                                 T_URI_LABELS*       pUriLabels,
                                 UINT8*              pTopoCnt);

/* -------------------------------------------------------------------------- */


UINT8 dbIptGetTopoCnt (void)
{
   return (dbCurIptTrn.topoCnt);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbGetCurTrainState (T_DB_TRAIN_STATE*      pTrnState)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      pTrnState->iptInaugState = dbCurIptTrn.inaugState;
      pTrnState->iptTopoCnt    = dbCurIptTrn.topoCnt;
      pTrnState->uicInaugState = dbCurUicTrn.global.inaugState;
      pTrnState->uicTopoCnt    = dbCurUicTrn.global.topoCnt;
      pTrnState->dynState      = (UINT8) 0;
      pTrnState->dynCnt        = (UINT8) 0;

      dbResult = DB_OK;

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }
   else
   {
      (void) tdcMemSet (pTrnState, 0, sizeof (T_DB_TRAIN_STATE));
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

void dbIptNewTopoCnt (UINT8      topoCnt)
{
   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      delIptTrain (&dbCurIptTrn, FALSE);

      dbCurIptTrn.topoCnt              = (UINT8) 0;
      dbDynData.expTrnState.iptTopoCnt = topoCnt;

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }
}

/* ---------------------------------------------------------------------------- */

void dbIptNewInaugState (UINT8      inaugState,
                         UINT8      topoCnt)
{
   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      switch (inaugState)
      {
         case 0:
         {
            delIptTrain (&dbCurIptTrn, TRUE);
            dbCurIptTrn.inaugState = (UINT8) 0;
            dbCurIptTrn.topoCnt    = (UINT8) 0;

            break;
         }
         case 1:
         {
            delIptTrain (&dbCurIptTrn, FALSE);
            dbCurIptTrn.inaugState = (UINT8) 1;
            dbCurIptTrn.topoCnt    = (UINT8) 0;

            break;
         }
         case 2:
         {
            /* Train is now ready for communication, await train configuration data */

            dbCurIptTrn.inaugState = (UINT8) 0;
            dbCurIptTrn.topoCnt    = (UINT8) 0;

            break;
         }
         default:
         {
            DEBUG_WARN1 (MOD_DB, "Invalid iptInaugState w(%d) received!", inaugState);
            break;
         }
      }

      dbDynData.expTrnState.iptInaugState = inaugState;
      dbDynData.expTrnState.iptTopoCnt    = topoCnt;

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbIptNewTrain (UINT8        inaugState,
                          UINT8        topoCnt)
{
   T_TDC_BOOL     bOK = FALSE;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (    (dbCurIptTrn.inaugState != inaugState)
           || (dbCurIptTrn.topoCnt    != topoCnt)
         )
      {
         /* New configuration data available - check against expected state */

         bOK  = (    (dbDynData.expTrnState.iptInaugState == inaugState)
                  && (dbDynData.expTrnState.iptTopoCnt    == topoCnt)
                );
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (bOK);
}

/* ---------------------------------------------------------------------------- */

void dbIptNoNewTrain (void)      /* Clean up memory (in case of wrong MD telegram - ComID 101) */
{
   delIptTrain (&dbShadowIptTrn, TRUE);      
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbIptAddTrain (const T_DB_IPT_TRN_DATA*   pTrn)
{
   UINT32   i;
   UINT32   carTypeLblCnt = pTrn->pCarTypeLst->lblCnt;
   UINT32   devLblCnt     = pTrn->pDevLblLst->lblCnt;
   UINT32   grpLblCnt     = pTrn->pGrpLblLst->lblCnt + 1;        // append (predefined) grpAll 
   UINT32   mcGrpCnt      = pTrn->pMcLst->grpCnt     + 1;        // append (predefined) grpAll
   UINT32   carTypeSize   = ALIGN_4 (carTypeLblCnt * sizeof (T_IPT_LABEL));
   UINT32   devLblSize    = ALIGN_4 (devLblCnt     * sizeof (T_IPT_LABEL));
   UINT32   grpLblSize    = ALIGN_4 (grpLblCnt     * sizeof (T_IPT_LABEL));
   UINT32   mcGrpSize     = ALIGN_4 (mcGrpCnt      * sizeof (T_DB_MC_GRP_DATA));

   delIptTrain (&dbShadowIptTrn, TRUE);

   dbShadowIptTrn.pLblAndGrpBuf = (UINT8 *) tdcAllocMemChk (MOD_MD, carTypeSize + devLblSize + grpLblSize + mcGrpSize);

   if (dbShadowIptTrn.pLblAndGrpBuf == NULL)
   {
      DEBUG_WARN (MOD_DB, "Failed to alloc memory for global train data");
      return (FALSE);
   }

   dbShadowIptTrn.inaugState = pTrn->inaugState;
   dbShadowIptTrn.topoCnt    = pTrn->topoCnt;

   dbShadowIptTrn.carTypeLst.lblCnt = carTypeLblCnt; 
   dbShadowIptTrn.devLblLst.lblCnt  = devLblCnt;
   dbShadowIptTrn.grpLblLst.lblCnt  = grpLblCnt;
   dbShadowIptTrn.mcLst.grpCnt      = mcGrpCnt;

   dbShadowIptTrn.carTypeLst.pLbl = (T_IPT_LABEL *)      ((void*) dbShadowIptTrn.pLblAndGrpBuf);
   dbShadowIptTrn.devLblLst.pLbl  = (T_IPT_LABEL *)      ((void*) (&dbShadowIptTrn.pLblAndGrpBuf[carTypeSize]));
   dbShadowIptTrn.grpLblLst.pLbl  = (T_IPT_LABEL *)      ((void*) (&dbShadowIptTrn.pLblAndGrpBuf[carTypeSize + devLblSize]));
   dbShadowIptTrn.mcLst.pGrp      = (T_DB_MC_GRP_DATA *) ((void*) (&dbShadowIptTrn.pLblAndGrpBuf[carTypeSize + devLblSize + grpLblSize]));

   for (i = 0; i < pTrn->pCarTypeLst->lblCnt; i++)
   {
      (void) tdcStrNCpy (dbShadowIptTrn.carTypeLst.pLbl[i], pTrn->pCarTypeLst->lbl[i], IPT_LABEL_SIZE);
   }

   for (i = 0; i < pTrn->pDevLblLst->lblCnt; i++)
   {
      (void) tdcStrNCpy (dbShadowIptTrn.devLblLst.pLbl[i], pTrn->pDevLblLst->lbl[i], IPT_LABEL_SIZE);
   }

   for (i = 0; i < pTrn->pGrpLblLst->lblCnt; i++)
   {
      (void) tdcStrNCpy (dbShadowIptTrn.grpLblLst.pLbl[i], pTrn->pGrpLblLst->lbl[i], IPT_LABEL_SIZE);
   }

   // add predefined label groupAll
   (void) tdcStrNCpy (dbShadowIptTrn.grpLblLst.pLbl[grpLblCnt - 1], groupAll, IPT_LABEL_SIZE);

   for (i = 0; i < pTrn->pMcLst->grpCnt; i++)
   {
      if (pTrn->pMcLst->grp[i].lblIdx >= dbShadowIptTrn.grpLblLst.lblCnt)
      {
         DEBUG_WARN2 (MOD_DB, "TrainMcGrpIdx out of range idx = %d, max = %d", 
                              pTrn->pMcLst->grp[i].lblIdx, dbShadowIptTrn.grpLblLst.lblCnt);
         return (FALSE);
      }

      dbShadowIptTrn.mcLst.pGrp[i].lblIdx = pTrn->pMcLst->grp[i].lblIdx;
      dbShadowIptTrn.mcLst.pGrp[i].no     = pTrn->pMcLst->grp[i].no;
   }

   dbShadowIptTrn.mcLst.pGrp[pTrn->pMcLst->grpCnt].lblIdx = (UINT16) (mcGrpCnt - 1);
   dbShadowIptTrn.mcLst.pGrp[pTrn->pMcLst->grpCnt].no     = 0;

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbIptAddConsist (const T_DB_IPT_CST_DATA_SET*     pCst)
{
   UINT32         cstMcLstCnt  = pCst->pMcLst->grpCnt + 1;                 // append (predefined) grpAll
   UINT32         carLstSize   = ALIGN_4 (DB_CAR_LIST_SIZE    (pCst->carCnt));
   UINT32         devLstSize   = ALIGN_4 (DB_DEV_LIST_SIZE    (pCst->devCnt));
   UINT32         carMcLstSize = ALIGN_4 (DB_MC_GRP_LIST_SIZE (pCst->carMcGrpCnt));
   UINT32         cstMcLstSize = ALIGN_4 (DB_MC_GRP_LIST_SIZE (cstMcLstCnt));
   UINT32         cstBufSize   = sizeof (T_DB_CONSIST) + carLstSize + devLstSize + carMcLstSize + cstMcLstSize;
   T_DB_CONSIST*  pNewCst      = (T_DB_CONSIST *) tdcAllocMemChk (MOD_MD, cstBufSize);

   if (pNewCst != NULL)
   {                             
      UINT8*                  pByte    = (UINT8 *) (pNewCst + 1);
      UINT32                  i;
      T_DB_MC_GRP_LIST*       pMcLst;           

      pNewCst->pCstMcLst      = (T_DB_MC_GRP_LIST *) ((void*) pByte);
      pNewCst->pCarMcLst      = (T_DB_MC_GRP_LIST *) ((void*) &pByte[cstMcLstSize]);
      pNewCst->pCarLst        = (T_DB_CAR_LIST    *) ((void*) &pByte[cstMcLstSize + carMcLstSize]);
      pNewCst->pDevLst        = (T_DB_DEV_LIST    *) ((void*) &pByte[cstMcLstSize + carMcLstSize + carLstSize]);
      pNewCst->pCstMcLst->cnt = pCst->pMcLst->grpCnt + 1;            // append (predefined) grpAll
      pNewCst->pCarMcLst->cnt = pCst->carMcGrpCnt;
      pNewCst->pCarLst->cnt   = (UINT16) pCst->carCnt;
      pNewCst->pDevLst->cnt   = pCst->devCnt;
      pNewCst->trnCstNo       = (UINT16) pCst->trnCstNo;
      pNewCst->bIsLocal       = pCst->bIsLocal;
      pNewCst->orient         = pCst->orient;

      (void) tdcStrNCpy  (pNewCst->cstId, pCst->cstId, IPT_LABEL_SIZE);
      (void) tdcSNPrintf (pNewCst->cstNoLbl, IPT_LABEL_SIZE, "cst%02d", (int) pCst->trnCstNo);
      pNewCst->cstNoLbl[sizeof (T_IPT_LABEL) - 1] = '\0';

      pMcLst = pNewCst->pCstMcLst;  

      for (i = 0; i < pCst->pMcLst->grpCnt; i++)              
      {
         if (pCst->pMcLst->grp[i].lblIdx >= dbShadowIptTrn.grpLblLst.lblCnt)
         {
            DEBUG_WARN2 (MOD_DB, "CstMcGrpIdx out of range idx = %d, max = %d", 
                                 pCst->pMcLst->grp[i].lblIdx, pCst->pMcLst->grpCnt);
            tdcFreeMem (pNewCst);
            return     (FALSE);
         }

         pMcLst->grp[pMcLst->addIdx].lblIdx   = pCst->pMcLst->grp[i].lblIdx;
         pMcLst->grp[pMcLst->addIdx].no       = pCst->pMcLst->grp[i].no;
         pMcLst->grp[pMcLst->addIdx].cstCarNo = (UINT8) 0;
         pMcLst->addIdx++;
      }

      // append (predefined) grpAll
      pMcLst->grp[pMcLst->addIdx].lblIdx   = (UINT16) (dbShadowIptTrn.mcLst.grpCnt - 1);
      pMcLst->grp[pMcLst->addIdx].no       = (UINT16) 0;
      pMcLst->grp[pMcLst->addIdx].cstCarNo = (UINT8)  0;
      pMcLst->addIdx++;

      if (dbShadowIptTrn.pCstRoot == NULL)
      {
         dbShadowIptTrn.pCstRoot = pNewCst;
      }
      else
      {
         if (dbShadowIptTrn.pCstRoot->bIsLocal != (UINT8) 0)
         {
            pNewCst->pNext                 = dbShadowIptTrn.pCstRoot->pNext;
            dbShadowIptTrn.pCstRoot->pNext = pNewCst;
         }
         else
         {
            pNewCst->pNext          = dbShadowIptTrn.pCstRoot;
            dbShadowIptTrn.pCstRoot = pNewCst;
         }
      }

      return (TRUE);
   }
                                     
   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL addIptCar (const T_DB_CONSIST*               pCst,
                             const T_DB_IPT_CAR_DATA_SET*      pCarData)
{
   if (pCst->pCarLst->addIdx < pCst->pCarLst->cnt)
   {
      if (pCarData->carTypeLblIdx < dbShadowIptTrn.carTypeLst.lblCnt)
      {
         T_DB_CAR*     pCar = &pCst->pCarLst->car[pCst->pCarLst->addIdx];

         pCar->carTypeLblIdx = pCarData->carTypeLblIdx;
         pCar->cstCarNo      = pCarData->cstCarNo;
         pCar->trnOrient     = pCarData->trnOrient;
         pCar->cstOrient     = pCarData->cstOrient;

         (void) tdcMemCpy   (pCar->uicIdent, pCarData->uicIdent, IPT_UIC_IDENTIFIER_CNT);
         (void) tdcStrNCpy  (pCar->carId,    pCarData->carId,    IPT_LABEL_SIZE);
         (void) tdcSNPrintf (pCar->carNoLbl, IPT_LABEL_SIZE,     "car%02d",  (int) pCarData->cstCarNo);
         pCar->carNoLbl[sizeof (T_IPT_LABEL) - 1] = '\0';

         pCst->pCarLst->addIdx++;

         return (TRUE);
      }
      else
      {
         DEBUG_WARN2 (MOD_DB, "iptCarTypeLblIdx out of range idx = %d, max = %d", 
                              pCarData->carTypeLblIdx, dbShadowIptTrn.carTypeLst.lblCnt);
      }
   }
   else
   {
      DEBUG_WARN2 (MOD_DB, "iptCarIdx out of range idx = %d, max = %d", 
                           pCst->pCarLst->addIdx, pCst->pCarLst->cnt);
   }

   return (FALSE);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL addIptCarMcGrps (const T_DB_CONSIST*               pCst,
                                   const T_DB_IPT_CAR_DATA_SET*      pCar)
{
   UINT32               i;
   T_DB_MC_GRP_LIST*    pMcLst = pCst->pCarMcLst;

   for (i = 0; i < pCar->pMcLst->grpCnt; i++)
   {
      if (pCst->pCarMcLst->addIdx < pCst->pCarMcLst->cnt)
      {
         if (pCar->pMcLst->grp[i].lblIdx < dbShadowIptTrn.grpLblLst.lblCnt)
         {
            pMcLst->grp[pMcLst->addIdx].cstCarNo = pCar->cstCarNo;
            pMcLst->grp[pMcLst->addIdx].lblIdx   = pCar->pMcLst->grp[i].lblIdx;
            pMcLst->grp[pMcLst->addIdx].no       = pCar->pMcLst->grp[i].no;
            pMcLst->addIdx++;
         }
         else
         {
            DEBUG_WARN2 (MOD_DB, "mcGrpLblIdx out of range idx = %d, max = %d", 
                                 pCar->pMcLst->grp[i].lblIdx, dbShadowIptTrn.grpLblLst.lblCnt);

            return (FALSE);
         }
      }
      else
      {
         DEBUG_WARN2 (MOD_DB, "mcGrpCnt out of range idx = %d, max = %d", 
                              pCst->pCarMcLst->addIdx, pCst->pCarMcLst->cnt);

         return (FALSE);
      }
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL addIptDevs (const T_DB_CONSIST*              pCst,
                              const T_DB_IPT_CAR_DATA_SET*     pCar)
{
   UINT32            devNo;
   T_DB_DEV_LIST*    pDevs = pCst->pDevLst;

   for (devNo = 0; devNo < pCar->pDevs->devCnt; devNo++)
   {
      if (pDevs->addIdx < pDevs->cnt)
      {
         if (pCar->pDevs->dev[devNo].lblIdx < dbShadowIptTrn.devLblLst.lblCnt)
         {
            pDevs->dev[pDevs->addIdx].cstCarNo = pCar->cstCarNo;
            pDevs->dev[pDevs->addIdx].lblIdx   = pCar->pDevs->dev[devNo].lblIdx;
            pDevs->dev[pDevs->addIdx].hostId   = pCar->pDevs->dev[devNo].no;
            pDevs->addIdx++;
         }
         else
         {
            DEBUG_WARN2 (MOD_DB, "devLblIdx out of range idx = %d, max = %d", 
                                 pCar->pDevs->dev[devNo].lblIdx, dbShadowIptTrn.devLblLst.lblCnt);

            return (FALSE);
         }
      }
      else
      {
         DEBUG_WARN2 (MOD_DB, "devLabelCnt out of range idx = %d, max = %d", 
                              pCst->pDevLst->addIdx, pCst->pDevLst->cnt);

         return (FALSE);
      }
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbIptAddCar (const T_DB_IPT_CAR_DATA_SET*     pCar)
{
   const T_DB_CONSIST*     pCst;
   T_TDC_BOOL              bOK = FALSE;

   for (pCst = dbShadowIptTrn.pCstRoot; pCst != NULL; pCst = pCst->pNext)
   {
      if (tdcStrCmp (pCst->cstId, pCar->cstId) == 0)    /* corresponding Consist found */
      {
         bOK =    addIptCar       (pCst, pCar)
               && addIptCarMcGrps (pCst, pCar)
               && addIptDevs      (pCst, pCar);
      }
   }

   return (bOK);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbIptCheckTrain (void)
{
   /* Do some plausability checks */

   T_DB_CONSIST*     pCst = dbShadowIptTrn.pCstRoot;

   /* Assert one and only one local consist */

   if (pCst != NULL)
   {
      if (pCst->bIsLocal != (UINT8) 0)
      {
         for (pCst = dbShadowIptTrn.pCstRoot->pNext; pCst != NULL; pCst = pCst->pNext)
         {
            if (pCst->bIsLocal != (UINT8) 0)
            {
               DEBUG_WARN (MOD_DB, "Invalid Train Configuration - More than one local Consist!");
               return (FALSE);
            }
         }

      }
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbIptActivateNewTrain (void)
{
   T_TDC_BOOL     bOk = FALSE;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      T_DB_IPT_TRAIN     tempTrain = dbCurIptTrn;

      dbCurIptTrn = dbShadowIptTrn;
      tdcMemClear (&dbShadowIptTrn, (UINT32) sizeof (dbShadowIptTrn));

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);

      delIptTrain (&tempTrain, TRUE);

      bOk = TRUE;
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

void dbIptVerboseTrain (void)
{
   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
       verboseIptTrn (&dbCurIptTrn);

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetOwnIds (T_URI_LABELS*    pUriLabels,
                            UINT8*           pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   pUriLabels->cst[0] = '\0';
   pUriLabels->car[0] = '\0';
   pUriLabels->dev[0] = '\0';
   pUriLabels->trn[0] = '\0';
   *pTopoCnt          = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         dbResult  = DB_NO_MATCHING_ENTRY;
         *pTopoCnt = dbCurIptTrn.topoCnt;

         if (    (dbCurIptTrn.pLocCar != NULL)
              && (dbCurIptTrn.pLocDev != NULL)
            )
         {
            (void) tdcStrNCpy (pUriLabels->cst, dbCurIptTrn.pCstRoot->cstId,                             IPT_LABEL_SIZE);
            (void) tdcStrNCpy (pUriLabels->car, dbCurIptTrn.pLocCar->carId,                              IPT_LABEL_SIZE);
            (void) tdcStrNCpy (pUriLabels->dev, dbCurIptTrn.devLblLst.pLbl[dbCurIptTrn.pLocDev->lblIdx], IPT_LABEL_SIZE);

            dbResult  = DB_OK;
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

static UINT32 findHostInAnyCarTab (const T_IPT_LABEL     dev,
                                   UINT16*               pHostId)
{
   UINT32      i;
   UINT32      foundCnt = 0;

   for (i = 0; (i < (int) ANYCAR_INDEX_TAB_SIZE)   &&  (foundCnt < 2); i++)
   {
      if (tdcStrNICmp (dbAnyCarTab[i].devId, dev, sizeof (T_IPT_LABEL)) == 0)
      {
         *pHostId = dbAnyCarTab[i].hostId;
         foundCnt++;
      }
   }

   return (foundCnt);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbGetAnyCarHostId (const T_URI_LABELS*    pUriLabels,
                                     UINT8*           pLCstNo,
                                     UINT8*           pCstNo,
                                     UINT16*          pHostId,
                                     UINT8*           pTopoCnt,
                                     UINT8*           pTbType,
                                     T_IPT_IP_ADDR*   pGatewayAddr)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pLCstNo      = (UINT8)  0;
   *pCstNo       = (UINT8)  0;
   *pHostId      = (UINT16) 0;
   *pTbType      = (UINT8)  0;
   *pGatewayAddr = (T_IPT_IP_ADDR) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      *pTbType      = dbDynData.tbType;
      *pGatewayAddr = dbDynData.gatewayAddr;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = findCstByCstLbl (pUriLabels->cst);

         *pTopoCnt = dbCurIptTrn.topoCnt;
         dbResult  = DB_NO_MATCHING_ENTRY;

         if (pCst != NULL)
         {
            UINT32      foundCnt = 0;
            UINT32      i;

            *pLCstNo = (UINT8) dbCurIptTrn.pCstRoot->trnCstNo;
            *pCstNo  = (UINT8) pCst->trnCstNo;

            // look for ipAddr in anyCar table (local consist only)

            if (pCst->trnCstNo == dbCurIptTrn.pCstRoot->trnCstNo) 
            {
               for (i = 0; (i < (int) ANYCAR_INDEX_TAB_SIZE)   &&  (foundCnt < 2); i++)
               {
                  foundCnt = findHostInAnyCarTab (pUriLabels->dev, pHostId);
               }
            }

            // look for devId in the consist

            for (i = 0; (i < pCst->pDevLst->cnt)  &&  (foundCnt < 2); i++)
            {
               if (tdcStrICmp (dbCurIptTrn.devLblLst.pLbl[pCst->pDevLst->dev[i].lblIdx], pUriLabels->dev) == 0)
               {
                  *pHostId = pCst->pDevLst->dev[i].hostId;
                  foundCnt++;
               }
            }

            if (foundCnt == 1)         // unique devId found ?
            {
               dbResult = DB_OK;
            }
         }
      }
      else // NO_CONFIG_DATA, check for predefined URIs
      {
         dbResult = DB_NO_CONFIG;

         // assert lCst

         if (tdcStrICmp (localCst, pUriLabels->cst) == 0)
         {
            if (findHostInAnyCarTab (pUriLabels->dev, pHostId) == 1)
            {
               dbResult = DB_OK;
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetCstCnt (UINT8*     pCstCnt,
                            UINT8*     pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;
   UINT16         cstCnt   = (UINT16) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*      pCst;

         for (pCst = dbCurIptTrn.pCstRoot; pCst != NULL; pCst = pCst->pNext)
         {
            cstCnt++;
         }

         *pTopoCnt = dbCurIptTrn.topoCnt;
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);

      dbResult = (cstCnt == ((UINT16) 0)) ? (DB_NO_CONFIG) : (DB_OK);
   }

   *pCstCnt = (UINT8) cstCnt;

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetCstCarCnt (const T_IPT_LABEL      cstLbl,
                                     UINT8*           pCarCnt,
                                     UINT8*           pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pCarCnt = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*      pCst = findCstByCstLbl (cstLbl);

         *pTopoCnt = dbCurIptTrn.topoCnt;
         dbResult  = DB_NO_MATCHING_ENTRY;

         if (pCst != NULL)
         {
            *pCarCnt = (UINT8) pCst->pCarLst->cnt;
            dbResult = DB_OK;
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetCarDevCnt (const T_IPT_LABEL      cstLbl,
                               const T_IPT_LABEL      carLbl,
                               UINT16*                pDevCnt,
                               UINT8*                 pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;
   UINT16         devCnt   = 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = NULL;
         const T_DB_CAR*         pCar = findCarByLbls (&pCst, cstLbl, carLbl);

         *pTopoCnt = dbCurIptTrn.topoCnt;
         dbResult  = DB_NO_MATCHING_ENTRY;

         if (    (pCar != NULL)
              && (pCst != NULL)
            )
         {
            UINT32         i;

            dbResult = DB_OK;
                                                           
            for (i = 0; i < pCst->pDevLst->cnt; i++)
            {
               if (pCst->pDevLst->dev[i].cstCarNo == pCar->cstCarNo)
               {
                  devCnt++;
               }
            }                                              
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   *pDevCnt = devCnt;

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptLabel2CstId (const T_IPT_LABEL    carLbl,
                                    T_IPT_LABEL    cstId,
                                    UINT8*         pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   cstId[0]  = '\0';
   *pTopoCnt = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*      pCst;

         dbResult  = DB_NO_MATCHING_ENTRY;
         *pTopoCnt = dbCurIptTrn.topoCnt;

         for (pCst = dbCurIptTrn.pCstRoot; pCst != NULL; pCst = pCst->pNext)
         {
            if (findCarByCarLbl (pCst, carLbl) != NULL)
            {
               (void) tdcStrNCpy (cstId, pCst->cstId, IPT_LABEL_SIZE);
               dbResult  = DB_OK;
               break;
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptCstNo2CstId (UINT8             trnCstNo,
                              T_IPT_LABEL       cstId,
                              UINT8*            pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   cstId[0]  = '\0';
   *pTopoCnt = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*      pCst;

         dbResult  = DB_NO_MATCHING_ENTRY;
         *pTopoCnt = dbCurIptTrn.topoCnt;

         for (pCst = dbCurIptTrn.pCstRoot; pCst != NULL; pCst = pCst->pNext)
         {
            if (pCst->trnCstNo == trnCstNo)
            {
               (void) tdcStrNCpy (cstId, pCst->cstId, IPT_LABEL_SIZE);
               dbResult  = DB_OK;
               break;
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetCstNo (const T_IPT_LABEL    cstLbl,
                           UINT8*               pLCstNo,
                           UINT8*               pCstNo,
                           UINT8*               pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pLCstNo  = (UINT8) 0;
   *pCstNo   = (UINT8) 0;
   *pTopoCnt = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*      pCst = findCstByCstLbl (cstLbl);

         *pTopoCnt = dbCurIptTrn.topoCnt;
         dbResult  = DB_NO_MATCHING_ENTRY;

         if (pCst != NULL)
         {
            *pCstNo  = (UINT8) pCst->trnCstNo;
            *pLCstNo = (UINT8) dbCurIptTrn.pCstRoot->trnCstNo;
            dbResult = DB_OK;
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetCstNoCarNo (const T_IPT_LABEL     cstLbl,
                                const T_IPT_LABEL     carLbl,
                                UINT8*                pCstNo,
                                UINT8*                pCarNo)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pCstNo = (UINT8) 0;
   *pCarNo = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = NULL;
         const T_DB_CAR*         pCar = findCarByLbls (&pCst, cstLbl, carLbl);

         if (    (pCar != NULL)
              && (pCst != NULL)
            )
         {                                        
            *pCstNo  = (UINT8) pCst->trnCstNo;
            *pCarNo  = pCar->cstCarNo;     
            dbResult = DB_OK;
         }
         else
         {
            dbResult = DB_NO_MATCHING_ENTRY;
         }
      }
      else
      {
         dbResult = DB_NO_CONFIG;
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetCarNo (const T_IPT_LABEL       cstLbl,
                           const T_IPT_LABEL       carLbl,
                           UINT8*                  pCarNo)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pCarNo = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = NULL;
         const T_DB_CAR*         pCar = findCarByLbls (&pCst, cstLbl, carLbl);

         if (pCar != NULL)
         {
            *pCarNo  = pCar->cstCarNo;
            dbResult = DB_OK;
         }
         else
         {
            dbResult = DB_NO_MATCHING_ENTRY;
         }
      }
      else
      {
         dbResult = DB_NO_CONFIG;
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetCar (const T_IPT_LABEL   cstLbl,
                         UINT16              hostId,
                         T_IPT_LABEL         devId,
                         T_IPT_LABEL         carId,
                         T_IPT_LABEL         cstId,
                         UINT8*              pCarNo)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   devId[0] = '\0';
   carId[0] = '\0';
   cstId[0] = '\0';
   *pCarNo  = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*      pCst = findCstByCstLbl (cstLbl);

         dbResult = DB_NO_MATCHING_ENTRY;

         if (pCst != NULL)
         {
            UINT32            i;
            T_DB_DEV_LIST*    pDevLst = pCst->pDevLst;

            for (i = 0; i < pDevLst->cnt; i++)
            {
               if (pDevLst->dev[i].hostId == hostId)
               {
                  /* device found */

                  UINT32            j;
                  T_DB_CAR_LIST*    pCarLst = pCst->pCarLst;

                  for (j = 0; j < pCarLst->cnt; j++)
                  {
                     if (pCarLst->car[j].cstCarNo == pDevLst->dev[i].cstCarNo)
                     {
                        /* car found */

                        dbResult = DB_OK;
                        *pCarNo  = pDevLst->dev[i].cstCarNo;
                        (void) tdcStrNCpy (devId, dbCurIptTrn.devLblLst.pLbl[pDevLst->dev[i].lblIdx], IPT_LABEL_SIZE);
                        (void) tdcStrNCpy (carId, pCarLst->car[j].carId,                              IPT_LABEL_SIZE);
                        (void) tdcStrNCpy (cstId, pCst->cstId,                                        IPT_LABEL_SIZE);
                        break;
                     }
                  }

                  if (dbResult != DB_OK)
                  {
                     DEBUG_WARN (MOD_DB, "Inconsistent DB, no matching car for device");
                     dbResult = DB_ERROR;
                  }

                  break;
               }
            }
         }
      }
      else
      {
         dbResult = DB_NO_CONFIG;
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbIptSetLocal (UINT16     hostId)
{
   T_TDC_BOOL     bOk = FALSE;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      const T_DB_CONSIST*      pCst = dbShadowIptTrn.pCstRoot;

      if (    (pCst           != NULL)
           && (pCst->bIsLocal != (UINT8) 0)
         )
      {
         UINT32            i;
         T_DB_DEV_LIST*    pDevLst = pCst->pDevLst;

         for (i = 0; i < pDevLst->cnt; i++)          /* searching device */
         {
            if (pDevLst->dev[i].hostId == hostId)
            {
               /* device found */

               UINT32            j;
               T_DB_CAR_LIST*    pCarLst = pCst->pCarLst;

               for (j = 0; j < pCarLst->cnt; j++)       /* searching car */
               {
                  if (pCarLst->car[j].cstCarNo == pDevLst->dev[i].cstCarNo)
                  {
                     /* car found */

                     bOk                  = TRUE;;
                     dbShadowIptTrn.pLocCar = &pCarLst->car[j];
                     dbShadowIptTrn.pLocDev = &pDevLst->dev[i];

                     break;
                  }
               }

               break;
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetHostId (const T_URI_LABELS*    pUriLbls,
                            UINT8*                 pLCstNo,
                            UINT8*                 pCstNo,
                            UINT16*                pHostId,
                            UINT8*                 pTopoCnt,
                            UINT8*                 pTbType,
                            T_IPT_IP_ADDR*         pGatewayAddr)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pLCstNo      = (UINT8)         0;
   *pCstNo       = (UINT8)         0;
   *pHostId      = (UINT16)        0;
   *pTbType      = (UINT8)         0;
   *pGatewayAddr = (T_IPT_IP_ADDR) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      *pTbType      = dbDynData.tbType;
      *pGatewayAddr = dbDynData.gatewayAddr;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = NULL;
         const T_DB_CAR*         pCar = findCarByLbls (&pCst, pUriLbls->cst, pUriLbls->car);

         dbResult  = DB_NO_MATCHING_ENTRY;
         *pTopoCnt = dbCurIptTrn.topoCnt;

         if (pCar != NULL)
         {
            const T_DB_DEVICE*    pDev = findDevByLDevLbl (pCst, pCar, pUriLbls->dev);

            if (pDev != NULL)
            {
               *pLCstNo = (UINT8) dbCurIptTrn.pCstRoot->trnCstNo;
               *pCstNo  = (UINT8) pCst->trnCstNo;
               *pHostId = pDev->hostId;
               dbResult = DB_OK;
            }
         }
      }
      else
      {
         dbResult = DB_NO_CONFIG;
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetUnicastLabels (UINT8              cstNo,
                                   UINT16             hostId,
                                   T_URI_LABELS*      pUriLbls,
                                   UINT8*             pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   (void) tdcSNPrintf (pUriLbls->cst, IPT_LABEL_SIZE, "cst%02d", (int) cstNo);
   pUriLbls->cst[sizeof (T_IPT_LABEL) - 1] = '\0';

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = findCstByCstLbl (pUriLbls->cst);

         dbResult  = DB_NO_MATCHING_ENTRY;
         *pTopoCnt = dbCurIptTrn.topoCnt;

         if (pCst != NULL)
         {
            UINT32      i;

            for (i = 0; i < pCst->pDevLst->cnt; i++)
            {
               if (pCst->pDevLst->dev[i].hostId == hostId)
               {
                  const T_DB_CAR*      pCar;

                  (void) tdcSNPrintf (pUriLbls->car, IPT_LABEL_SIZE,
                                      "car%02d", (int) pCst->pDevLst->dev[i].cstCarNo);
                  pUriLbls->car[sizeof (T_IPT_LABEL) - 1] = '\0';

                  pCar = findCarByCarLbl (pCst, pUriLbls->car);

                  if (pCar != NULL)
                  {
                     (void) tdcStrNCpy (pUriLbls->cst, pCst->cstId,                                              IPT_LABEL_SIZE);
                     (void) tdcStrNCpy (pUriLbls->car, pCar->carId,                                              IPT_LABEL_SIZE);
                     (void) tdcStrNCpy (pUriLbls->dev, dbCurIptTrn.devLblLst.pLbl[pCst->pDevLst->dev[i].lblIdx], IPT_LABEL_SIZE);
                     dbResult  = DB_OK;
                  }
                  else
                  {
                     DEBUG_WARN (MOD_DB, "Inconsistent DB, no matching car for device");
                     dbResult = DB_ERROR;
                  }

                  break;
               }
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

static T_DB_RESULT getTrnMcLbls (UINT16              grpNo,
                                 T_URI_LABELS*       pUriLbls,
                                 UINT8*              pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;
   
   if (grpNo == (UINT16) 0)      // check for grpAll.aCar.aCst
   {
      (void) tdcStrNCpy (pUriLbls->dev, groupAll, IPT_LABEL_SIZE); 
      (void) tdcStrNCpy (pUriLbls->car, allCars,  IPT_LABEL_SIZE);
      (void) tdcStrNCpy (pUriLbls->cst, allCsts,  IPT_LABEL_SIZE);
      (void) tdcStrNCpy (pUriLbls->trn, localTrn, IPT_LABEL_SIZE);
      dbResult = DB_OK;
   }
   else
   {
      if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
      {
         dbResult = DB_NO_CONFIG;
   
         if (dbCurIptTrn.pCstRoot != NULL)
         {
            UINT32   i;
   
            dbResult  = DB_NO_MATCHING_ENTRY;
            *pTopoCnt = dbCurIptTrn.topoCnt;
   
            for (i = 0; i < dbCurIptTrn.mcLst.grpCnt; i++)
            {
               if (dbCurIptTrn.mcLst.pGrp[i].no == grpNo)
               {
                  const char*    pGrpLbl = dbCurIptTrn.grpLblLst.pLbl[dbCurIptTrn.mcLst.pGrp[i].lblIdx];
   
                 (void) tdcStrNCpy (pUriLbls->dev, pGrpLbl,  IPT_LABEL_SIZE); 
                 (void) tdcStrNCpy (pUriLbls->car, allCars,  IPT_LABEL_SIZE);
                 (void) tdcStrNCpy (pUriLbls->cst, allCsts,  IPT_LABEL_SIZE);
                 (void) tdcStrNCpy (pUriLbls->trn, localTrn, IPT_LABEL_SIZE);
                 dbResult = DB_OK;
                 break;
               }
            }
         }
   
         (void) tdcMutexUnlock (MOD_MD, dbMutexId);
      }
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

static T_DB_RESULT getCstMcLbls (UINT8               trnCstNo,
                                 UINT16              grpNo,
                                 T_URI_LABELS*       pUriLbls,
                                 UINT8*              pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = findCstByCstNo (trnCstNo);

         dbResult  = DB_NO_MATCHING_ENTRY;
         *pTopoCnt = dbCurIptTrn.topoCnt;

         if (pCst != NULL)                            // look for cst groups
         {
            const T_DB_MC_GRP*   pGrp   = findGrpByGrpNo (pCst->pCstMcLst, grpNo, NULL);
            const char*          pCarId = allCars;

            if (pGrp == NULL)       // look for car groups, and count matches
            {
               UINT32         carNo;
               UINT32         foundCnt = 0;

	       //pCarId = anyCar; removed-raises warning in GH CC; gweiss

               for (carNo = 0; (carNo < pCst->pCarLst->cnt)  &&  (foundCnt < 2); carNo++)
               {
                  const T_DB_MC_GRP*      pFoundGrp;

                  if ((pFoundGrp = findGrpByGrpNo (pCst->pCarMcLst, grpNo, &pCst->pCarLst->car[carNo].cstCarNo)) != NULL)
                  {
                     pGrp = pFoundGrp;
                     foundCnt++;
                  }
               }

               pCarId = NULL;

               if (foundCnt == 1)
               {
                  if (pGrp != NULL)       // already asserted by foundCnt==1, but for PC-Lint...
                  {
                     const T_DB_CAR*      pCar = findCarByCarNo (pCst, pGrp->cstCarNo);
   
                     pCarId = (pCar != NULL)   ?   (pCar->carId)   :   (NULL);
                  }
               }
            }

            if (    (pGrp   != NULL)
                 && (pCarId != NULL)
               )
            {
               (void) tdcStrNCpy (pUriLbls->dev, dbCurIptTrn.grpLblLst.pLbl[pGrp->lblIdx], IPT_LABEL_SIZE); 
               (void) tdcStrNCpy (pUriLbls->car, pCarId,                                   IPT_LABEL_SIZE);
               (void) tdcStrNCpy (pUriLbls->cst, pCst->cstId,                              IPT_LABEL_SIZE);
               (void) tdcStrNCpy (pUriLbls->trn, localTrn,                                 IPT_LABEL_SIZE);
               dbResult  = DB_OK;
            }
         }
      }
      else           // check for grpAll.aCar.lCst
      {
         if (    (trnCstNo == (UINT8)  0)
              && (grpNo    == (UINT16) 0)
            )
         {
            (void) tdcStrNCpy (pUriLbls->dev, groupAll, IPT_LABEL_SIZE); 
            (void) tdcStrNCpy (pUriLbls->car, allCars,  IPT_LABEL_SIZE);
            (void) tdcStrNCpy (pUriLbls->cst, localCst, IPT_LABEL_SIZE);
            (void) tdcStrNCpy (pUriLbls->trn, localTrn, IPT_LABEL_SIZE);
            dbResult  = DB_OK;
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetMulticastLabels (UINT8               cstNo,
                                     UINT16              grpNo,
                                     T_URI_LABELS*       pUriLbls,
                                     UINT8*              pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   pUriLbls->dev[0] = '\0';
   pUriLbls->car[0] = '\0';
   pUriLbls->cst[0] = '\0';
   pUriLbls->trn[0] = '\0';
   *pTopoCnt        = (UINT8) 0;

   switch (cstNo)
   {
      case 0x3F:                 // aCst --> train Group
      {
         dbResult = getTrnMcLbls (grpNo, pUriLbls, pTopoCnt);
         break;
      }
      default:                   // consist or car group
      {
         dbResult = getCstMcLbls (cstNo, grpNo, pUriLbls, pTopoCnt);
         break;
      }
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbIptGetCarId (UINT8             cstNo,
                           UINT8             carNo,
                           T_IPT_LABEL       carId,
                           UINT8*            pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   carId[0]  = '\0';
   *pTopoCnt = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = findCstByCstNo (cstNo);

         dbResult = DB_NO_MATCHING_ENTRY;

         if (pCst != NULL)
         {
            if (    (carNo >  (UINT8) 0)
                 && (carNo <= (UINT8) pCst->pCarLst->cnt)
               )
            {
               (void) tdcStrCpy (carId, pCst->pCarLst->car[(int) carNo - 1].carId);
               *pTopoCnt = dbCurIptTrn.topoCnt;
               dbResult  = DB_OK;
            }
         }
      }
      else
      {
         dbResult = DB_NO_CONFIG;
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

/*lint -save -esym(429, pCarData) pCarData is not custotory */
T_DB_RESULT dbIptGetCarInfo (T_TDC_CAR_DATA*       pCarData,
                             UINT16                maxDev,
                             const T_IPT_LABEL     cstLbl,
                             const T_IPT_LABEL     carLbl,
                             UINT8*                pTopoCnt)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   pCarData->carId[0]   = '\0';
   pCarData->carType[0] = '\0';
   pCarData->cstCarNo   = (UINT8)  0;
   pCarData->trnOrient  = (UINT8)  0;
   pCarData->cstOrient  = (UINT8)  0;
   pCarData->devCnt     = (UINT16) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = NULL;
         const T_DB_CAR*         pCar = findCarByLbls (&pCst, cstLbl, carLbl);

         *pTopoCnt = dbCurIptTrn.topoCnt;
         dbResult  = DB_NO_MATCHING_ENTRY;

         if (pCar != NULL)
         {
            UINT32      i;

            dbResult = DB_OK;

            (void) tdcStrNCpy (pCarData->carId,    pCar->carId,    IPT_LABEL_SIZE);
            (void) tdcStrNCpy (pCarData->carType,  
                               dbCurIptTrn.carTypeLst.pLbl[pCar->carTypeLblIdx],  
                               IPT_LABEL_SIZE);
            (void) tdcMemCpy  (pCarData->uicIdent, pCar->uicIdent, IPT_UIC_IDENTIFIER_CNT);

            pCarData->cstCarNo  = pCar->cstCarNo;
            pCarData->trnOrient = pCar->trnOrient;
            pCarData->cstOrient = pCar->cstOrient;

            for (i = 0; i < pCst->pDevLst->cnt; i++)
            {
               if (pCst->pDevLst->dev[i].cstCarNo == pCar->cstCarNo)
               {
                  if (pCarData->devCnt < maxDev)
                  {
                     pCarData->devData[pCarData->devCnt].hostId = pCst->pDevLst->dev[i].hostId;
                                                                                       /*@ -usedef */
                     (void) tdcStrNCpy (pCarData->devData[pCarData->devCnt].devId,
                                        dbCurIptTrn.devLblLst.pLbl[pCst->pDevLst->dev[i].lblIdx],
                                        IPT_LABEL_SIZE);                /*@ =usedef */
                     pCarData->devCnt++;
                     //dbResult = DB_OK;
                  }
                  else
                  {
                     dbResult = DB_ERROR;
                     break;
                  }
               }
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}
/*lint -restore*/

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbGetTrnGrpNo (const T_URI_LABELS*     pUriLbls,
                           UINT16*                 pGrpNo,
                           UINT8*                  pTopoCnt,
                           UINT8*                  pTbType,
                           T_IPT_IP_ADDR*          pGatewayAddr)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pTopoCnt     = (UINT8) 0;
   *pTbType      = (UINT8) 0;
   *pGatewayAddr = (T_IPT_IP_ADDR) dbDynData.gatewayAddr;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult      = DB_NO_CONFIG;
      *pTbType      = dbDynData.tbType;
      *pGatewayAddr = dbDynData.gatewayAddr;

      // First test for the predefined multicast group:  "grpAll.aCar.aCst"

      if (    (tdcStrNICmp (pUriLbls->dev, groupAll, IPT_LABEL_SIZE) == 0)
           && (tdcStrNICmp (pUriLbls->car, allCars,  IPT_LABEL_SIZE) == 0)
         )
      {
         *pGrpNo  = (UINT16) 0;
         dbResult = DB_OK;
      }

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         *pTopoCnt = dbCurIptTrn.topoCnt;

         if (dbResult != DB_OK)
         {
            UINT32         i;
            
            dbResult  = DB_NO_MATCHING_ENTRY;

            for (i = 0; i < dbCurIptTrn.mcLst.grpCnt; i++)
            {
               if (tdcStrICmp (pUriLbls->dev, dbCurIptTrn.grpLblLst.pLbl[dbCurIptTrn.mcLst.pGrp[i].lblIdx]) == 0)
               {
                  *pGrpNo  = dbCurIptTrn.mcLst.pGrp[i].no;
                  dbResult = DB_OK;
                  break;
               }
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbGetCstGrpNo (const T_URI_LABELS*     pUriLbls,
                           UINT16*                 pGrpNo,
                           UINT8*                  pCstNo,
                           UINT8*                  pLCstNo,
                           UINT8*                  pTopoCnt,
                           UINT8*                  pTbType,
                           T_IPT_IP_ADDR*          pGatewayAddr)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pTopoCnt     = (UINT8) 0;
   *pTbType      = (UINT8) 0;
   *pGatewayAddr = (T_IPT_IP_ADDR) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult      = DB_NO_CONFIG;
      *pTbType      = dbDynData.tbType;
      *pGatewayAddr = dbDynData.gatewayAddr;

      // First test for the predefined multicast group:  "grpAll.aCar.lCst"

      if (    (tdcStrNICmp (pUriLbls->dev, groupAll, IPT_LABEL_SIZE) == 0)
           && (tdcStrNICmp (pUriLbls->cst, localCst, IPT_LABEL_SIZE) == 0)
         )
      {
         *pGrpNo  = (UINT16) 0;
         *pCstNo  = (UINT8)  1;
         *pLCstNo = (UINT8)  1;
        /* CR-590 Allow for requests with non-zero topocnt also */
         *pTopoCnt = dbCurIptTrn.topoCnt;
          
         dbResult = DB_OK;
      }
      else
      {
         if (dbCurIptTrn.pCstRoot != NULL)
         {
            const T_DB_CONSIST*     pCst;

            *pTopoCnt = dbCurIptTrn.topoCnt;
            pCst      = findCstByCstLbl (pUriLbls->cst);
            dbResult  = DB_NO_MATCHING_ENTRY;

            if (pCst != NULL)
            {
               const T_DB_MC_GRP*      pGrp = findGrpByLbl (pCst->pCstMcLst, pUriLbls->dev, NULL);

               if (pGrp != NULL)
               {
                  *pGrpNo  = pGrp->no;
                  *pCstNo  = (UINT8) pCst->trnCstNo;
                  *pLCstNo = (UINT8) dbCurIptTrn.pCstRoot->trnCstNo;
                  dbResult = DB_OK;
               }
               else     // Try alterantives to "lCst": "grpAll.aCar.lCst" 
               {
                  if (    (tdcStrNICmp (pUriLbls->dev, groupAll, IPT_LABEL_SIZE) == 0)
                       && (pCst->trnCstNo == dbCurIptTrn.pCstRoot->trnCstNo)
                     )
                  {
                     *pGrpNo  = (UINT16) 0;
                     *pCstNo  = (UINT8)  pCst->trnCstNo;
                     *pLCstNo = (UINT8)  pCst->trnCstNo;
                     dbResult = DB_OK;
                  }
               }
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbGetAnyCarGrpNo (const T_URI_LABELS*     pUriLbls,
                              UINT16*                 pGrpNo,
                              UINT8*                  pCstNo,
                              UINT8*                  pLCstNo,
                              UINT8*                  pTopoCnt,
                              UINT8*                  pTbType,
                              T_IPT_IP_ADDR*          pGatewayAddr)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pTopoCnt     = (UINT8) 0;
   *pTbType      = (UINT8) 0;
   *pGatewayAddr = (T_IPT_IP_ADDR) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult      = DB_NO_CONFIG;
      *pTbType      = dbDynData.tbType;
      *pGatewayAddr = dbDynData.gatewayAddr;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = findCstByCstLbl (pUriLbls->cst);

         dbResult  = DB_NO_MATCHING_ENTRY;
         *pTopoCnt = dbCurIptTrn.topoCnt;

         if (pCst != NULL)
         {
            const T_DB_MC_GRP*      pGrp = findGrpByLbl (pCst->pCarMcLst, pUriLbls->dev, NULL);

            if (pGrp != NULL)
            {
               *pGrpNo  = pGrp->no;
               *pCstNo  = (UINT8) pCst->trnCstNo;
               *pLCstNo = (UINT8) dbCurIptTrn.pCstRoot->trnCstNo;
               dbResult = DB_OK;
            }
         }

      }
      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbGetCarGrpNo (const T_URI_LABELS*     pUriLbls,
                           UINT16*                 pGrpNo,
                           UINT8*                  pCstNo,
                           UINT8*                  pLCstNo,
                           UINT8*                  pTopoCnt,
                           UINT8*                  pTbType,
                           T_IPT_IP_ADDR*          pGatewayAddr)
{
   T_DB_RESULT    dbResult = DB_ERROR;

   *pTopoCnt     = (UINT8) 0;
   *pTbType      = (UINT8) 0;
   *pGatewayAddr = (T_IPT_IP_ADDR) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult      = DB_NO_CONFIG;
      *pTbType      = dbDynData.tbType;
      *pGatewayAddr = dbDynData.gatewayAddr;

      if (dbCurIptTrn.pCstRoot != NULL)
      {
         const T_DB_CONSIST*     pCst = NULL;
         const T_DB_CAR*         pCar = findCarByLbls (&pCst, pUriLbls->cst, pUriLbls->car);

         dbResult  = DB_NO_MATCHING_ENTRY;
         *pTopoCnt = dbCurIptTrn.topoCnt;

         if (pCar != NULL)
         {
            const T_DB_MC_GRP*      pGrp = findGrpByLbl (pCst->pCarMcLst, pUriLbls->dev, &pCar->cstCarNo);

            if (pGrp != NULL)
            {
               *pGrpNo  = pGrp->no;
               *pCstNo  = (UINT8) pCst->trnCstNo;
               *pLCstNo = (UINT8) dbCurIptTrn.pCstRoot->trnCstNo;
               dbResult = DB_OK;
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbIsCstLCst (const T_URI_LABELS*     pUriLbls)
{
   T_TDC_BOOL        bLocal = FALSE;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      const T_DB_CONSIST*     pCst = NULL;
      const T_DB_CAR*         pCar = findCarByLbls (&pCst, pUriLbls->cst, pUriLbls->car);
   
      if (pCst!= NULL)
      {
         if (pCst->bIsLocal != (UINT8) 0)
         {
            bLocal = TRUE;
         }
         else
         {
            pCar = pCar;
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (bLocal);
}



