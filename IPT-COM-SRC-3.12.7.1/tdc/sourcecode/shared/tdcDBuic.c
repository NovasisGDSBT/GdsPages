/*                                                                            */
/*  $Id: tdcDBuic.c 11648 2010-08-20 15:33:06Z bloehr $                      */
/*                                                                            */
/*  DESCRIPTION    Database for IP-Train Directory Client (TDC)			  	  */
/*					UIC data handling										  */
/*                                                                            */
/*  AUTHOR         M.Ritz         PPC/EBT                                     */
/*                                                                            */
/*  REMARKS                                                                   */
/*                                                                            */
/*  DEPENDENCIES   Either the switch LINUX or WIN32 has to be set	          */
/*                                                                            */
/*  MODIFICATIONS (log starts 2010-08-11)									  */
/*   																		  */
/*                                                                            */
/*  All rights reserved. Reproduction, modification, use or disclosure        */
/*  to third parties without express authority is forbidden.                  */
/*  Copyright Bombardier Transportation GmbH, Germany, 2002-2010.             */
/*                                                                            */


/* ----------------------------------------------------------------------------
* ----------------------------------------------------------------------------
* ---------------------------------------------------------------------------- */

#include "tdc.h"
#include "tdcInit.h"
//#include "tdcConfig.h"
#include "tdcDB.h"
#include "tdcDBpriv.h"

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void dbUicNewTopoCnt (UINT8      topoCnt)
{
   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      // Save inauguration State before cleaning the UIC train

      UINT8    inaugState = dbCurUicTrn.global.inaugState;

      delUicTrain (&dbCurUicTrn, FALSE);

      // restore inauguration State
      dbCurUicTrn.global.inaugState    = inaugState;
      dbCurUicTrn.global.topoCnt       = (UINT8) 0;
      dbDynData.expTrnState.uicTopoCnt = topoCnt;

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }
}

/* ---------------------------------------------------------------------------- */

void dbUicNewInaugState (UINT8      inaugState,
                         UINT8      topoCnt)
{
   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      switch (inaugState)
      {
         case 0:
         {
            delUicTrain (&dbCurUicTrn, FALSE);
            break;
         }
         case 1:
         {
            /* Train is now ready for communication, awaiting train configuration data */
            break;
         }
         case 2:
         {
            delUicTrain (&dbCurUicTrn, TRUE);
            break;
         }
         default:
         {
            DEBUG_WARN1 (MOD_DB, "Invalid uicInaugState w(%d) received!", inaugState);
            break;
         }
      }

      dbCurUicTrn.global.inaugState = inaugState;
      dbCurUicTrn.global.topoCnt    = (UINT8) 0;

      dbDynData.expTrnState.uicInaugState = inaugState;
      dbDynData.expTrnState.uicTopoCnt    = topoCnt;

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbUicNewTrain (UINT8    inaugState,
                          UINT8    topoCnt)
{
   T_TDC_BOOL     bOk = FALSE;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (    (dbCurUicTrn.global.inaugState != inaugState)
           || (dbCurUicTrn.global.topoCnt    != topoCnt)
         )
      {
         /* New configuration data available - check against expected state */

         if (   (dbDynData.expTrnState.uicInaugState == inaugState)
             && (dbDynData.expTrnState.uicTopoCnt    == topoCnt)
            )
         {
            delUicTrain (&dbShadowUicTrn, TRUE);

            dbShadowUicTrn.global.inaugState = inaugState;
            dbShadowUicTrn.global.topoCnt    = topoCnt;

            bOk = TRUE;
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbUicAddGlobData (const T_TDC_UIC_GLOB_DATA*     pGlobData)
{
   T_TDC_BOOL     bOk = FALSE;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (pGlobData->topoCnt == dbDynData.expTrnState.uicTopoCnt)
      {
         UINT32                  trnCarCnt   = pGlobData->trnCarCnt;
         UINT32                  carDataSize = (UINT32) sizeof (T_TDC_UIC_CAR_DATA);
         T_TDC_UIC_CAR_DATA*     pCarData    = (T_TDC_UIC_CAR_DATA *) tdcAllocMemChk (MOD_MD, trnCarCnt * carDataSize);

         if (pCarData != NULL)
         {
            delUicTrain (&dbShadowUicTrn, TRUE);

            dbShadowUicTrn.global   = *pGlobData;
            dbShadowUicTrn.pCarData = pCarData;

            bOk = TRUE;
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbUicAddCarData (const T_TDC_UIC_CAR_DATA*    pCarData,
                            UINT8                        carNo)
{
   T_TDC_BOOL     bOk = FALSE;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (dbShadowUicTrn.pCarData != NULL)
      {
         if (carNo < dbShadowUicTrn.global.trnCarCnt)
         {
            dbShadowUicTrn.pCarData[(int) carNo] = *pCarData;
            bOk = TRUE;
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL dbUicActivateNewTrain  (void)
{
   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      T_DB_UIC_TRAIN     tempTrain = dbCurUicTrn;

      dbCurUicTrn = dbShadowUicTrn;
      tdcMemClear(&dbShadowUicTrn, (UINT32) sizeof (dbShadowUicTrn));

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);

      delUicTrain (&tempTrain, TRUE);
   }

   return (TRUE);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbUicGetGlobalData (T_TDC_UIC_GLOB_DATA*   pGlobData)
{
   T_DB_RESULT     bResult = DB_ERROR;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (dbCurUicTrn.pCarData != NULL)
      {
         *pGlobData = dbCurUicTrn.global;
         bResult    = DB_OK;
      }
      else
      {
         bResult = DB_NO_CONFIG;
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (bResult);
}

/* ---------------------------------------------------------------------------- */

T_DB_RESULT dbUicGetCarData (T_TDC_UIC_CAR_DATA*   pCarData,
                             UINT8                 carSeqNo)
{
   T_DB_RESULT     dbResult = DB_ERROR;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      if (dbCurUicTrn.pCarData != NULL)
      {
         int   carNo;

         dbResult = DB_NO_MATCHING_ENTRY;

         for (carNo = 0; carNo < (int) dbCurUicTrn.global.trnCarCnt; carNo++)
         {
            if (dbCurUicTrn.pCarData[carNo].carSeqNo == carSeqNo)
            {
               *pCarData = dbCurUicTrn.pCarData[carNo];
               dbResult  = DB_OK;
               break;
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

T_DB_RESULT dbGetUicCarSeqNo (UINT8*               pCarSeqNo,
                              const T_IPT_LABEL    cstLabel,
                              const T_IPT_LABEL    carLabel)
{
   T_DB_RESULT     dbResult = DB_ERROR;

   *pCarSeqNo = (UINT8) 0;

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (    (dbCurUicTrn.pCarData != NULL)
           && (dbCurIptTrn.pCstRoot != NULL)
         )
      {
         const T_DB_CONSIST*     pIptCst = NULL;
         const T_DB_CAR*         pIptCar = findCarByLbls (&pIptCst, cstLabel, carLabel);

         dbResult = DB_NO_MATCHING_ENTRY;

         if (pIptCar != NULL)
         {
            int      carNo;

            for (carNo = 0; carNo < (int) dbCurUicTrn.global.trnCarCnt; carNo++)
            {
               const T_TDC_UIC_CAR_DATA*     pUicCar = &dbCurUicTrn.pCarData[carNo];

               if (tdcMemCmp (pIptCar->uicIdent, pUicCar->uicIdent, IPT_UIC_IDENTIFIER_CNT) == 0)
               {
                  *pCarSeqNo = pUicCar->carSeqNo;
                  dbResult   = DB_OK;
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

T_DB_RESULT dbUicCarSeqNo2Ids (T_IPT_LABEL           cstLabel,
                               T_IPT_LABEL           carLabel,
                               UINT8                 carSeqNo)
{
   T_DB_RESULT     dbResult = DB_ERROR;

   cstLabel[0] = '\0';
   carLabel[0] = '\0';

   if (tdcMutexLock (MOD_MD, dbMutexId) == TDC_MUTEX_OK)
   {
      dbResult = DB_NO_CONFIG;

      if (    (dbCurUicTrn.pCarData != NULL)
           && (dbCurIptTrn.pCstRoot != NULL)
         )
      {
         int      carNo;

         for (carNo = 0; carNo < (int) dbCurUicTrn.global.trnCarCnt; carNo++)
         {
            if (dbCurUicTrn.pCarData[carNo].carSeqNo == carSeqNo)
            {
               if (findIptUicIdent(dbCurUicTrn.pCarData[carNo].uicIdent, cstLabel, carLabel))
               {
                  dbResult = DB_OK;
               }
               
               break;
            }
         }
      }

      (void) tdcMutexUnlock (MOD_MD, dbMutexId);
   }

   return (dbResult);
}

