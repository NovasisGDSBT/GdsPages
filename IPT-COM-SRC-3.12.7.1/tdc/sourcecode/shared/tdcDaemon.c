/*
 *  $Id: tdcDaemon.c 11839 2011-10-07 07:31:38Z bloehr $
 *
 *  DESCRIPTION    Daemon for IP-Train Directory Client (TDC)
 *                 tdcDaemon handles startup and Interprocess Communication
 *                 as well
 *
 *  AUTHOR         M.Ritz         PPC/EBT
 *
 *  REMARKS
 *
 *  DEPENDENCIES
 *
 *  MODIFICATIONS (log starts 2010-08-11)
 *
 *  CR-2621 (Gerhard Weiß, 2011-09-22)
 *       Correct unicast IP calculation from consist number (Max was 15, is 61)
 *
 *  CR-1038 (Bernd Loehr, 2011-09-06)
 *       Ommit error return for maxdev = 0
 *
 *  CR-685 (Gerhard Weiss, 2010-11-25)
 *       Corrected release of timer
 *
 *  CR-432 (Gerhard Weiss, 2010-10-07)
 *  Removed warnings provoked by -W (for GCC)
 *
 *  CR-695 (Gerhard Weiss, 2010-05-11)
 *  Some routines are not save if called in initialization phase.
 *  Those have been corrected. Touched routines:
 *  - tdcDestroy
 *  - tdcGetTrnBackboneType
 *  - tdcGetAddrByName
 *  - tdcGetUriHostParam
 *
 *  CR-382 (Bernd Loehr, 2010-08-13)
 *  Additional return code for getIptState to reflect local or remote
 *  consist info
 *
 *  All rights reserved. Reproduction, modification, use or disclosure
 *  to third parties without express authority is forbidden.
 *  Copyright Bombardier Transportation GmbH, Germany, 2002-2010.
 *
 */

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

#include <stdio.h>

#include "tdc.h"
#include "tdcInit.h"
#include "tdcIptCom.h"
#include "tdcResolve.h"
#include "tdcConfig.h"
#include "tdcDB.h"
#include "tdcThread.h"

/* -------------------------------------------------------------------------- */

static T_IPT_IP_ADDR             lIpAddr    = 0;
static T_IPT_IP_ADDR             simuIpAddr = 0;

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static UINT8         getTopoCnt            (void);

static T_TDC_RESULT  checkIptTopoCnt       (T_TDC_RESULT             tdcResult,
                                            UINT8                    curTopoCnt,          // current  TopoCnt
                                            UINT8*                   pPropTopoCnt);       // proposed TopoCnt

static T_TDC_BOOL    uriLabelizer          (const T_IPT_URI          uri,
                                            /*@out@*/ T_URI_LABELS*  pUriLbls);

static T_TDC_BOOL    getDirectIpAddress    (const T_IPT_URI          uri,
                                            T_IPT_IP_ADDR*           pIpAddr,
                                            UINT8*                   pTopoCnt);
static T_TDC_RESULT  getUnicastAddr        (const T_URI_LABELS*      pUriLbls,
                                            T_IPT_IP_ADDR*           pIpAddr,
                                            UINT8*                   pTopoCnt);
static T_TDC_RESULT  getMulticastcAddr     (const T_URI_LABELS*      pUriLbls,
                                            T_IPT_IP_ADDR*           pIpAddr,
                                            UINT8*                   pTopoCnt);
static T_TDC_RESULT  getUnicastUri         (UINT8                    cstNo,
                                            UINT16                   hostId,
                                            T_IPT_URI                uri,
                                            UINT8*                   pTopoCnt);
static T_TDC_RESULT  getMulticastUri       (UINT8                    cstNo,
                                            UINT16                   grpNo,
                                            T_IPT_URI                uri,
                                            UINT8*                   pTopoCnt);

static T_IPT_IP_ADDR bldUnicastAddr        (UINT16                   lCstNo,
                                            UINT16                   cstNo,
                                            UINT16                   hostId,
                                            UINT8                    topoCnt);
static T_IPT_IP_ADDR bldTrnMulticastAddr   (UINT16                   grpNo);
static T_IPT_IP_ADDR bldCstMulticastAddr   (UINT16                   grpNo,
                                            UINT8                    cstNo,
                                            UINT8                    lCstNo,
                                            UINT8                    topoCnt);

static void          verboseUriLabels      (const char*              pPrefix,
                                            const T_URI_LABELS*      pUriLabels);

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

static void verboseUriLabels (const char*          pPrefix,
                              const T_URI_LABELS*  pUriLabels)
{
   char              text[200];

   (void) tdcSNPrintf (text, (unsigned) sizeof (text),
                       "%s: dev(%s), car(%s), cst(%s), train(%s)",
                       pPrefix, pUriLabels->dev, pUriLabels->car, pUriLabels->cst, pUriLabels->trn);
   text[sizeof (text) - 1] = '\0';

   DEBUG_INFO (MOD_MAIN, text);
}

/* -------------------------------------------------------------------------- */

static UINT8 getTopoCnt (void)
{
   T_DB_TRAIN_STATE     trainState;

   (void) dbGetCurTrainState (&trainState);

   return (trainState.iptTopoCnt);
}

/* -------------------------------------------------------------------------- */

static T_IPT_IP_ADDR bldUnicastAddr (UINT16     lCstNo,        /* local  Consist */
                                     UINT16     cstNo,         /* actual Consist */
                                     UINT16     hostId,
                                     UINT8      topoCnt)
{
   UINT8       unitNo;

   if (cstNo != lCstNo)
   {
      topoCnt = (UINT8) (topoCnt & 0x3F);
      unitNo  = (UINT8) (cstNo & 0x3F);
   }
   else
   {
      topoCnt = (UINT8) 0;
      unitNo  = (UINT8) 0;
   }
                                                          /*@ -shiftimplementation */
   return (UC_ADDR (10, topoCnt, unitNo, hostId));        /*@ =shiftimplementation */
}

/* -------------------------------------------------------------------------- */

static T_IPT_IP_ADDR bldTrnMulticastAddr (UINT16         grpNo)
{
   UINT8       topoCnt = (UINT8) 0;             /* Don't care for Train wide MC Groups */
   UINT8       unitNo  = (UINT8) 0x3F;          /* all Consists */

   return (MC_ADDR (239, topoCnt, unitNo, grpNo));
}

/* -------------------------------------------------------------------------- */

static T_IPT_IP_ADDR bldCstMulticastAddr (UINT16         grpNo,
                                          UINT8          cstNo,
                                          UINT8          lCstNo,
                                          UINT8          topoCnt)
{
   UINT8       unitNo;

   unitNo  = (cstNo == lCstNo) ? (UINT8) 0 : cstNo;
   topoCnt = (cstNo == lCstNo) ? (UINT8) 0 : topoCnt;

   return (MC_ADDR (239, topoCnt, unitNo, grpNo));
}

/* -------------------------------------------------------------------------- */

static T_TDC_BOOL uriLabelizer (const T_IPT_URI    uri,
                                T_URI_LABELS*      pUriLbls)
{
   T_IPT_URI      tempUri;
   char*          pDev   = tempUri;
   char*          pCar   = NULL;
   char*          pCst   = NULL;
   char*          pTrn   = NULL;
   char*          pTemp ;
   UINT32         devLen;
   UINT32         carLen;
   UINT32         cstLen;
   UINT32         trnLen;

   (void) tdcStrNCpy (tempUri, uri, IPT_URI_SIZE);

   if ((pTemp = tdcStrStr (tempUri, "@")) != NULL)
   {
      pDev = pTemp + 1;
   }

   if ((pTemp = tdcStrStr (pDev, ".")) != NULL)
   {
      *pTemp = '\0';
      pCar   = pTemp + 1;

      if ((pTemp = tdcStrStr (pCar, ".")) != NULL)
      {
         *pTemp = '\0';
         pCst   = pTemp + 1;

         if ((pTemp = tdcStrStr (pCst, ".")) != NULL)
         {
            *pTemp = '\0';
            pTrn   = pTemp + 1;
         }
      }
   }

   devLen = tdcStrLen (pDev);
   (void) tdcStrNCpy (pUriLbls->dev, pDev, IPT_MAX_LABEL_LEN);

   if (pCar == NULL)
   {
      carLen =  tdcStrLen (localCar);
      (void) tdcStrNCpy (pUriLbls->car, localCar, IPT_MAX_LABEL_LEN);
   }
   else
   {
      carLen = tdcStrLen (pCar);
      (void) tdcStrNCpy (pUriLbls->car, pCar, IPT_MAX_LABEL_LEN);
   }

   if (pCst == NULL)
   {
      cstLen = tdcStrLen (localCst);
      (void) tdcStrNCpy (pUriLbls->cst, localCst, IPT_MAX_LABEL_LEN);
   }
   else
   {
      cstLen = tdcStrLen (pCst);
      (void) tdcStrNCpy (pUriLbls->cst, pCst, IPT_MAX_LABEL_LEN);
   }

   if (pTrn == NULL)
   {
      trnLen = tdcStrLen (localTrn);
      (void) tdcStrNCpy (pUriLbls->trn, localTrn, IPT_MAX_LABEL_LEN);
   }
   else
   {
      trnLen = tdcStrLen (pTrn);
      (void) tdcStrNCpy (pUriLbls->trn, pTrn, IPT_MAX_LABEL_LEN);
   }

   return (    ((devLen > 0)   &&   (devLen < IPT_MAX_LABEL_LEN))
            && ((carLen > 0)   &&   (carLen < IPT_MAX_LABEL_LEN))
            && ((cstLen > 0)   &&   (cstLen < IPT_MAX_LABEL_LEN))
            && ((trnLen > 0)   &&   (trnLen < IPT_MAX_LABEL_LEN))
          );
}

/* -------------------------------------------------------------------------- */

static T_TDC_RESULT getUnicastUri (UINT8           cstNo,
                                   UINT16          hostId,
                                   T_IPT_URI       uri,
                                   UINT8*          pTopoCnt)
{
   T_URI_LABELS      uriLbls;
   T_TDC_RESULT      tdcResult = TDC_ERROR;

   switch (dbIptGetUnicastLabels (cstNo, hostId, &uriLbls, pTopoCnt))
   {
      case DB_OK:
      {
         tdcResult = TDC_OK;
         (void) tdcSNPrintf (uri, (unsigned) sizeof (T_IPT_URI), 
                             "%s.%s.%s.lTrain", uriLbls.dev, uriLbls.car, uriLbls.cst);
         uri[sizeof (T_IPT_URI) - 1] = '\0';
         break;
      }
      case DB_NO_CONFIG:            {tdcResult = TDC_NO_CONFIG_DATA;       break;}
      case DB_NO_MATCHING_ENTRY:    {tdcResult = TDC_NO_MATCHING_ENTRY;    break;}
      case DB_ERROR:
      default:                      {tdcResult = TDC_ERROR;                break;}

   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

static T_TDC_RESULT getMulticastUri (UINT8            cstNo,
                                     UINT16           grpNo,
                                     T_IPT_URI        uri, 
                                     UINT8*           pTopoCnt)
{
   T_URI_LABELS      uriLbls;
   T_TDC_RESULT      tdcResult = TDC_ERROR;

   switch (dbIptGetMulticastLabels (cstNo, grpNo, &uriLbls, pTopoCnt))
   {
      case DB_OK:
      {
         tdcResult = TDC_OK;
         (void) tdcSNPrintf (uri, (unsigned) sizeof (T_IPT_URI), 
                             "%s.%s.%s.lTrain", uriLbls.dev, uriLbls.car, uriLbls.cst);
         uri[sizeof (T_IPT_URI) - 1] = '\0';
         break;
      }
      case DB_NO_CONFIG:            {tdcResult = TDC_NO_CONFIG_DATA;       break;}
      case DB_NO_MATCHING_ENTRY:    {tdcResult = TDC_NO_MATCHING_ENTRY;    break;}
      case DB_ERROR:
      default:                      {tdcResult = TDC_ERROR;                break;}

   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

static T_TDC_RESULT getUnicastAddr (const T_URI_LABELS*     pUriLbls,
                                          T_IPT_IP_ADDR*    pIpAddr,
                                          UINT8*            pTopoCnt)
{
   T_TDC_RESULT      tdcResult = TDC_UNKNOWN_URI;

   *pIpAddr = (T_IPT_IP_ADDR) 0;

   if (    (tdcStrICmp (pUriLbls->car, allCars) != 0)     
        && (tdcStrICmp (pUriLbls->cst, allCsts) != 0)
      )
   {
      UINT8             lCstNo;
      UINT8             cstNo;
      UINT16            hostId;
      UINT8             tbType;
      T_IPT_IP_ADDR     gatewayAddr;
      T_DB_RESULT       dbResult;

      if (tdcStrICmp (pUriLbls->car, anyCar) == 0)
      {
         dbResult = dbGetAnyCarHostId (pUriLbls, &lCstNo, &cstNo, &hostId, pTopoCnt, &tbType, &gatewayAddr);
      }
      else
      {
         dbResult = dbIptGetHostId (pUriLbls, &lCstNo, &cstNo, &hostId, pTopoCnt, &tbType, &gatewayAddr);
      }

      switch (dbResult)
      {
         case DB_OK:
         {          
            tdcResult = TDC_OK;
            *pIpAddr  = bldUnicastAddr ((UINT16) lCstNo, (UINT16) cstNo, hostId, *pTopoCnt);

            if (lCstNo == cstNo)          
            {
               if (*pIpAddr == lIpAddr)      // local host ???
               {
                  *pIpAddr = UC_ADDR (127, 0, 0, 1);
               }
            }
            else     // i.e. NOT local consist
            {
               if (tbType == TDC_IPT_TBTYPE_WTB)
               {
                  *pIpAddr = gatewayAddr;
               }
            }

            DEBUG_INFO4 (MOD_MAIN, "getUnicastAddr (lCst=%d, cstNo=%d, hostId=%d - ipAddr=x%08x",
                                   lCstNo, cstNo, hostId, *pIpAddr);
            break;
         }
         case DB_NO_MATCHING_ENTRY:
         {  
            tdcResult = TDC_UNKNOWN_URI;       

            if (tbType == TDC_IPT_TBTYPE_WTB)
            {
               if (!dbIsCstLCst(pUriLbls))
               {
                  *pIpAddr  = gatewayAddr;
                  tdcResult = TDC_OK;
               }
            }
            break;
         }
         case DB_NO_CONFIG:            {tdcResult = TDC_NO_CONFIG_DATA;    break;}
         case DB_ERROR:
         default:                      {tdcResult = TDC_ERROR;             break;}
      }
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

static T_TDC_RESULT getMulticastcAddr (const T_URI_LABELS*     pUriLbls,
                                             T_IPT_IP_ADDR*    pIpAddr,
                                             UINT8*            pTopoCnt)
{
   T_TDC_RESULT      tdcResult = TDC_UNKNOWN_URI;

   if (tdcStrICmp (pUriLbls->cst, allCsts) == 0) 
   {
      // assert allCars, train wide groups require to be in all Cars as well!

      if (tdcStrICmp (pUriLbls->car, allCars) == 0)  
      {
         UINT16               grpNo;
         UINT8                tbType;
         T_IPT_IP_ADDR        gatewayAddr;

         switch (dbGetTrnGrpNo (pUriLbls, &grpNo, pTopoCnt, &tbType, &gatewayAddr))
         {
            case DB_OK: 
            {
               if (tbType == TDC_IPT_TBTYPE_WTB)
               {
                  *pIpAddr = gatewayAddr;
               }
               else
               {
                  *pIpAddr = bldTrnMulticastAddr (grpNo);
               }
               tdcResult = TDC_OK;
               break;
            }
            case DB_NO_MATCHING_ENTRY:    
            {
               tdcResult = TDC_UNKNOWN_URI;

               if (tbType == TDC_IPT_TBTYPE_WTB)
               {
                  *pIpAddr  = gatewayAddr;
                  tdcResult = TDC_OK;
               }
               break;
            }
            case DB_NO_CONFIG:            {tdcResult = TDC_NO_CONFIG_DATA;    break;}
            case DB_ERROR:
            default:                      {tdcResult = TDC_ERROR;             break;}
         }
      }
   }
   else                                                        // consist wide group
   {
      T_DB_RESULT          dbResult;
      UINT16               grpNo;
      UINT8                cstNo;
      UINT8                lCstNo;
      UINT8                tbType;
      T_IPT_IP_ADDR        gatewayAddr;

      if (tdcStrICmp (pUriLbls->car, allCars) == 0)            // consist wide group ?
      {
         dbResult = dbGetCstGrpNo (pUriLbls, &grpNo, &cstNo, &lCstNo, pTopoCnt, &tbType, &gatewayAddr);
      }
      else
      {
         if (tdcStrICmp (pUriLbls->car, anyCar) == 0)             // in any unspecified car ?
         {
            dbResult = dbGetAnyCarGrpNo (pUriLbls, &grpNo, &cstNo, &lCstNo, pTopoCnt, &tbType, &gatewayAddr);
         }
         else
         {
            dbResult = dbGetCarGrpNo (pUriLbls, &grpNo, &cstNo, &lCstNo, pTopoCnt, &tbType, &gatewayAddr);
         }
      }

      switch (dbResult)
      {
         case DB_OK: 
         {
            *pIpAddr  = bldCstMulticastAddr (grpNo, cstNo, lCstNo, *pTopoCnt);

            if (cstNo != lCstNo)
            {
               if (tbType == TDC_IPT_TBTYPE_WTB)
               {
                  *pIpAddr = gatewayAddr;
               }
            }
            tdcResult = TDC_OK;
            break;
         }
         case DB_NO_MATCHING_ENTRY: 
         {
            tdcResult = TDC_UNKNOWN_URI;       

            if (tbType == TDC_IPT_TBTYPE_WTB)
            {
               if (!dbIsCstLCst (pUriLbls))
               {
                  *pIpAddr  = gatewayAddr;
                  tdcResult = TDC_OK;
               }
            }
            break;
         }
         case DB_NO_CONFIG:            {tdcResult = TDC_NO_CONFIG_DATA;    break;}
         case DB_ERROR:
         default:                      {tdcResult = TDC_ERROR;             break;}
      }
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

void setSimuIpAddr (T_IPT_IP_ADDR   ipAddr)
{
   simuIpAddr = ipAddr;
}

/* -------------------------------------------------------------------------- */

T_TDC_BOOL tdcDetermineOwnDevice (const char*   pModName)
{
   T_TDC_BOOL     bOk = FALSE;
   UINT32         i;
   UINT32         ipAddrCnt;
   UINT32         ipAddrs[40];

   /* First of all read own IP-Address. The own IP-Address is assigned by DHCP and          */
   /* must be something like 10.0.x.y - with x < 16  (and xy not all zero or all 1)         */
   /* There is no way to determine the own device, as long as DHCP process is not finished! */

   DEBUG_INFO (MOD_MAIN, "\nTry to determine own Device");

   ipAddrCnt = tdcGetOwnIpAddrs (pModName, TAB_SIZE (ipAddrs), ipAddrs);

   for (i = 0; i < ipAddrCnt; i++)
   {
      if (    ((ipAddrs[i] & 0xFFFFF000) == 0x0A000000)
           && ((ipAddrs[i] & 0x00000FFF) != 0x00000000)
           && ((ipAddrs[i] & 0x00000FFF) != 0x00000FFF)
         )
      {
         // Check if interface is ready for Multicast
         // It seems that sometimes the interface itself is ready, but the route
         // isn't finally setup. That's why we wait until MC support is granted.

         if (    (!tdcGetCheckMCSupport ())
              || (tdcAssertMCsupport (pModName, ipAddrs[i]))
            )
         {
            DEBUG_INFO4 (MOD_MAIN, "Using local IP-Addr: %3d.%3d.%3d.%3d",
                                   (ipAddrs[i] >> 24) & 0xFF, 
                                   (ipAddrs[i] >> 16) & 0xFF,
                                   (ipAddrs[i] >> 8)  & 0xFF, 
                                   ipAddrs[i]         & 0xFF);

            lIpAddr = ipAddrs[i];
            bOk     = TRUE;
            break;
         }
      }
   }

   if (simuIpAddr != 0)
   {
      // e.g. SIMU_IPADDR  = UC_ADDR (10, 0, 0, 0x283)

      char     text[100];

      lIpAddr = simuIpAddr;
      bOk     = TRUE;

      (void) tdcSNPrintf (text, sizeof (text), "Cheating own IP-Addr to 'x%08x' for Module Test", lIpAddr);
      text [sizeof (text) - 1] = 0;

      DEBUG_INFO (pModName, text);
   }

   return (bOk);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetTrnBackboneType (UINT8*	         pTbType,
                                    T_IPT_IP_ADDR*    pGatewayIpAddr)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pTbType != NULL)
   {
      T_IPT_IP_ADDR     gatewayIpAddr = (T_IPT_IP_ADDR) 0;

      *pTbType        = (UINT8) 0;                       // satisfy lint

      tdcResult = TDC_MUST_FINISH_INIT;
      
      if (bBaseInitDone)
      {
         tdcResult = TDC_ERROR;
         
         if (dbGetIptDirGateway (pTbType, &gatewayIpAddr))
         {
            tdcResult = TDC_OK;
            
            if (pGatewayIpAddr != NULL)
            {
               *pGatewayIpAddr = gatewayIpAddr;
            }
         }
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetTrnBackboneType: Null - pointer for 'pTbType'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetIptState (UINT8*       pInaugState, /* output */
                             UINT8*       pTopoCnt)    /* output */
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pInaugState != NULL)
   {
      *pInaugState = (UINT8) 0;

      if (pTopoCnt != NULL)
      {
         *pTopoCnt = (UINT8) 0;
         tdcResult = TDC_MUST_FINISH_INIT;

         if (bBaseInitDone)
         {
            T_DB_TRAIN_STATE        trainState;

            switch (dbGetCurTrainState (&trainState))
            {
               case DB_OK:
               {
                  UINT8    inaugState;

                  switch (trainState.iptInaugState)
                  {
                     case 2:        {inaugState = TDC_IPT_INAUGSTATE_OK;         break;}
                     case 1:        {inaugState = TDC_IPT_INAUGSTATE_INVALID;    break;}
                     case 0:
                     default:       {inaugState = TDC_IPT_INAUGSTATE_FAULT;      break;}
                  }

   				  /* 	CR-382:
                    		If the standalone flag is set and we used the local
                  		    cstSta file, return the appropriate code
                  */
                  if (tdcGetStandaloneSupport() == TRUE &&
                  	  tdcGetIPTDirServerEmul() == TRUE)
                  {
    	              	*pInaugState = TDC_IPT_INAUGSTATE_LOCAL;
                        *pTopoCnt    = 0;
                  }
                  else
                  {
                    	*pInaugState = inaugState;
                  		*pTopoCnt    = trainState.iptTopoCnt;
                  }

                  tdcResult    = TDC_OK;

                  break;
               }

               case DB_ERROR:
               case DB_NO_CONFIG:
               case DB_NO_MATCHING_ENTRY:
               default:                      {tdcResult = TDC_ERROR;                break;}
            }
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcGetIptState: Null - pointer for 'pTopoCnt'");
      }
   }
   else
   {
      if (pTopoCnt != NULL)
      {
         *pTopoCnt = (UINT8) 0;
      }
      DEBUG_WARN (MOD_MAIN, "tdcGetIptState: Null - pointer for 'pInaugState'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetOwnIds (T_IPT_LABEL         devId,         /* output */     /* who am I? */
                           T_IPT_LABEL         carId,         /* output */
                           T_IPT_LABEL         cstId)         /* output */
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (devId != NULL)
   {
      devId[0] = '\0';

      if (carId != NULL)
      {
         carId[0] = '\0';

         if (cstId != NULL)
         {
            T_URI_LABELS      uriLbls;
            UINT8             curTopoCnt = (UINT8) 0;

            cstId[0]  = '\0';
            tdcResult = TDC_MUST_FINISH_INIT;

            if (bBaseInitDone)
            {
               switch (dbIptGetOwnIds (&uriLbls, &curTopoCnt))
               {
                  case DB_OK:          
                  {
                     (void) tdcStrNCpy (devId, uriLbls.dev, IPT_LABEL_SIZE);
                     (void) tdcStrNCpy (carId, uriLbls.car, IPT_LABEL_SIZE);
                     (void) tdcStrNCpy (cstId, uriLbls.cst, IPT_LABEL_SIZE);

                     tdcResult = TDC_OK;                   
                     break;
                  }
                  case DB_NO_CONFIG:            {tdcResult = TDC_NO_CONFIG_DATA;       break;}
                  case DB_NO_MATCHING_ENTRY:    {tdcResult = TDC_NO_MATCHING_ENTRY;    break;}
                  case DB_ERROR:
                  default:                      {tdcResult = TDC_ERROR;                break;}
               }
            }
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcGetOwnIds: Null - pointer for 'cstId'");
         }
      }
      else
      {
         if (cstId != NULL)
         {
            cstId[0] = '\0';
         }
         DEBUG_WARN (MOD_MAIN, "tdcGetOwnIds: Null - pointer for 'carId'");
      }
   }
   else
   {
      if (carId != NULL)
      {
         carId[0] = '\0';
      }
      if (cstId != NULL)
      {
         cstId[0] = '\0';
      }

      DEBUG_WARN (MOD_MAIN, "tdcGetOwnIds: Null - pointer for 'devId'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_BOOL tdcDottedQuad2Number (const char*       pDottedQuad,
                              T_IPT_IP_ADDR*    pIpAddr)
{
   UINT32            byte3;
   UINT32            byte2;
   UINT32            byte1;
   UINT32            byte0;
   char              dummyString[2];

   /* The dummy string with specifier %1s searches for a non-whitespace char
    * after the last number. If it is found, the result of scanf will be 5
    * instead of 4, indicating an errorous format of the ip-number.
    */
   if (tdcSScanf (pDottedQuad, "%u.%u.%u.%u%1s",
                  &byte3, &byte2, &byte1, &byte0, dummyString) == 4)
   {
      if (    (byte3 < 256)
           && (byte2 < 256)
           && (byte1 < 256)
           && (byte0 < 256)
         )
      {
         *pIpAddr  =   (byte3 << 24)
                     + (byte2 << 16)
                     + (byte1 << 8)
                     +  byte0;

         return (TRUE);
      }
   }

   return (FALSE);
}

/* -------------------------------------------------------------------------- */

static T_TDC_BOOL getDirectIpAddress (const T_IPT_URI          uri, 
                                            T_IPT_IP_ADDR*     pIpAddr,
                                            UINT8*             pTopoCnt)
{
   if (tdcDottedQuad2Number (uri, pIpAddr))
   {
      *pTopoCnt = getTopoCnt ();

      return (TRUE);
   }

   return (FALSE);
}

/* -------------------------------------------------------------------------- */

static T_TDC_RESULT checkIptTopoCnt (T_TDC_RESULT        tdcResult,                 
                                     UINT8               curTopoCnt,       // current  TopoCnt
                                     UINT8*              pPropTopoCnt)     // proposed TopoCnt)
{
   if (tdcResult == TDC_OK)
   {
      if (curTopoCnt != (*pPropTopoCnt))
      {
         if ((*pPropTopoCnt) > ((UINT8) 0))
         {
            tdcResult = TDC_WRONG_TOPOCOUNT;
         }
      }
   }

   *pPropTopoCnt = curTopoCnt;

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

static T_TDC_RESULT checkUicTopoCnt (T_TDC_RESULT   tdcResult, 
                                     UINT8*         pTopoCnt)
{
   T_DB_TRAIN_STATE     trainState;
   T_DB_RESULT          dbResult   = dbGetCurTrainState (&trainState);
   UINT8                curTopoCnt = (dbResult == DB_OK) ? (trainState.uicTopoCnt) : (*pTopoCnt);

   if (curTopoCnt != (*pTopoCnt))
   {
      if ((*pTopoCnt) > ((UINT8) 0))
      {
         if (tdcResult == TDC_OK)
         {
            tdcResult = TDC_WRONG_TOPOCOUNT;
         }
      }

      *pTopoCnt = curTopoCnt;
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

static T_TDC_RESULT check2TopoCnt (T_TDC_RESULT     tdcResult, 
                                   UINT8*           pIptTopoCnt,
                                   UINT8*           pUicTopoCnt)
{
   T_DB_TRAIN_STATE     trainState;
   T_DB_RESULT          dbResult      = dbGetCurTrainState (&trainState);
   UINT8                curIptTopoCnt = (dbResult == DB_OK) ? (trainState.iptTopoCnt) : (*pIptTopoCnt);
   UINT8                curUicTopoCnt = (dbResult == DB_OK) ? (trainState.uicTopoCnt) : (*pUicTopoCnt);

   if (    (    (curIptTopoCnt  != (*pIptTopoCnt))
             && ((*pIptTopoCnt) >  ((UINT8) 0))
           )
        || (    (curUicTopoCnt != (*pUicTopoCnt))
             && ((*pUicTopoCnt) > ((UINT8) 0))
           )
      )
   {
      if (tdcResult == TDC_OK)
      {
         tdcResult = TDC_WRONG_TOPOCOUNT;
      }
   }

   *pIptTopoCnt = curIptTopoCnt;
   *pUicTopoCnt = curUicTopoCnt;

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_IPT_IP_ADDR tdcGetLocalIpAddr (void)
{
   return (lIpAddr);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetAddrByName (const T_IPT_URI     uri, 
                               T_IPT_IP_ADDR*      pIpAddr,
                               UINT8*              pTopoCnt)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (uri != NULL)
   {
      if (pIpAddr != NULL)
      {
         *pIpAddr = 0;

         if (pTopoCnt != NULL)
         {
            UINT8       curTopoCnt = (UINT8) 0;

            tdcResult = TDC_MUST_FINISH_INIT;
            
            if (bBaseInitDone)
            {
               if (getDirectIpAddress (uri, pIpAddr, &curTopoCnt))
               {
                  tdcResult = TDC_OK;
               }
               else
               {
                  T_URI_LABELS      uriLbls;
                  
                  tdcResult = TDC_UNKNOWN_URI;
                  *pIpAddr  = 0;
                  
                  if (uriLabelizer (uri, &uriLbls))
                  {
                     verboseUriLabels ("uriLabelizer", &uriLbls);
                     
                     tdcResult = TDC_MUST_FINISH_INIT;
                     
                     if (bBaseInitDone)
                     {
                        if (tdcStrICmp (uriLbls.trn, localTrn) == 0)                   /* must be local Train */
                        {
                           if (    (tdcStrNICmp (uriLbls.dev, "grp", GROUP_PREF_LEN) == 0)   /* Multicast Group ? */
                               || (tdcStrNICmp (uriLbls.dev, "frg", GROUP_PREF_LEN) == 0)
                               )
                           {
                              tdcResult = getMulticastcAddr (&uriLbls, pIpAddr, &curTopoCnt);
                           }
                           else                                                        /* unicast */
                           {
                              tdcResult = getUnicastAddr (&uriLbls, pIpAddr, &curTopoCnt);
                           }
                        }
                        else
                        {
                           tdcResult = TDC_UNKNOWN_URI;
                           DEBUG_INFO (MOD_MAIN, "'lTrain' is the only valid train label in URis");
                        }
                     }
                  }
               }
               
               tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
            }
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcGetAddrByName: Null - pointer for 'pTopoCnt'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcGetAddrByName: Null - pointer for 'pIpAddr'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetAddrByName: Null - pointer for 'uri'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetAddrByNameExt (const T_IPT_URI      uri, 
                                  T_IPT_IP_ADDR*       pIpAddr,
                                  T_TDC_BOOL*          pIsFRG,
                                  UINT8*               pTopoCnt)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pIsFRG != NULL)
   {
      tdcResult = tdcGetAddrByName (uri, pIpAddr, pTopoCnt);

      *pIsFRG = FALSE;

      if (tdcResult == TDC_OK)
      {
         if (isIpAddrMC (*pIpAddr))
         {
            T_URI_LABELS      uriLbls;

            if (uriLabelizer (uri, &uriLbls))
            {
               if (tdcStrNICmp (uriLbls.dev, "frg", GROUP_PREF_LEN) == 0)
               {
                  *pIsFRG = TRUE;
               }
            }
         }
      }
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetUriHostPart (T_IPT_IP_ADDR      ipAddr, 
                                T_IPT_URI          uri,
                                UINT8*             pTopoCnt)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (uri != NULL)
   {
      uri[0] = '\0';

      if (pTopoCnt != NULL)
      {
         UINT8    protType   = (UINT8) ipAddrGetProtType (ipAddr);
         UINT8    curTopoCnt = (UINT8) 0;

         tdcResult = TDC_MUST_FINISH_INIT;
         
         if (bBaseInitDone)
         {
            tdcResult = TDC_UNKNOWN_IPADDR;

            switch (protType)
            {
               case (10):        // Unicast Address
               {
                  UINT8    cstNo  = (UINT8)  ucAddrGetUnitNo (ipAddr);
                  UINT16   hostId = (UINT16) ucAddrGetDevNo  (ipAddr);
                  
                  if (cstNo == (UINT8) 0)   /* translate to local consist number */
                  {
                     UINT8       lCstNo;
                     UINT8       rCstNo;
                     
                     if (dbIptGetCstNo (localCst, &lCstNo, &rCstNo, &curTopoCnt) == DB_OK)
                     {
                        if (lCstNo != rCstNo)
                        {
                           DEBUG_WARN (MOD_MD, "Inconsistent Database detected");
                        }
                        cstNo = lCstNo;
                     }
                     else
                     {
                        DEBUG_WARN (MOD_MD, "Unable to find local consist");
                        break;
                     }
                  }
                  
                  tdcResult = getUnicastUri (cstNo, hostId, uri, &curTopoCnt);
                  
                  break;
               }
               case (127):          // only localhost available
               {
                  if (ipAddr == UC_ADDR (127, 0, 0, 1))     /* local Host */
                  {
                     T_URI_LABELS      uriLbls;
                     
                     switch (dbIptGetOwnIds (&uriLbls, &curTopoCnt))
                     {
                        case DB_OK:
                        {
                           (void) tdcSNPrintf (uri, (unsigned) sizeof (T_IPT_URI),
                                               "%s.%s.%s.lTrain", uriLbls.dev, uriLbls.car, uriLbls.cst);
                           uri[sizeof (T_IPT_URI) - 1] = '\0';
                           tdcResult = TDC_OK;
                           break;
                        }
                        case DB_NO_CONFIG:               {tdcResult = TDC_NO_CONFIG_DATA;    break;}
                        case DB_ERROR:
                        case DB_NO_MATCHING_ENTRY:
                        default:                         {tdcResult = TDC_ERROR;             break;}
                     }
                  }
                  
                  break;
               }
               case (239):          // Multicast Address
               {
                  UINT8    cstNo = (UINT8)  mcAddrGetUnitNo (ipAddr);
                  UINT16   grpNo = (UINT16) mcAddrGetGrpNo  (ipAddr);
                  
                  tdcResult = getMulticastUri (cstNo, grpNo, uri, &curTopoCnt);
                  break;
               }
               default:
               {
                  break;
               }
            }
            
            tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcGetUriHostPart: Null - pointer for 'pTopoCnt'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetUriHostPart: Null - pointer for 'uri'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcLabel2CarId (T_IPT_LABEL         carId,   
                             UINT8*              pTopoCnt,    
                             const T_IPT_LABEL   cstLabel,
                             const T_IPT_LABEL   carLabel)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (carId != NULL)
   {
      carId[0] = '\0';

      if (carLabel != NULL)
      {
         if (pTopoCnt != NULL)
         {
            UINT8       curTopoCnt = (UINT8) 0;

            tdcResult = TDC_MUST_FINISH_INIT;

            if (bBaseInitDone)
            {
               UINT8       cstNo;
               UINT8       carNo;

               switch (dbIptGetCstNoCarNo (cstLabel, carLabel, &cstNo, &carNo))
               {
                  case DB_OK:
                  {
                     if (dbIptGetCarId (cstNo, carNo, carId, &curTopoCnt) == DB_OK)
                     {
                        tdcResult = TDC_OK;
                     }
                     else
                     {
                        tdcResult = TDC_ERROR;
                     }

                     break;
                  }
                  case DB_NO_CONFIG:               {tdcResult = TDC_NO_CONFIG_DATA;    break;}
                  case DB_NO_MATCHING_ENTRY:       {tdcResult = TDC_UNKNOWN_URI;       break;}
                  case DB_ERROR:
                  default:                         {tdcResult = TDC_ERROR;             break;}
               }
            }

            tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcLabel2CarId: Null - pointer for 'pTopoCnt'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcLabel2CarId: Null - pointer for 'carLabel'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcLabel2CarId: Null - pointer for 'carId'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcAddr2CarId  (T_IPT_LABEL         carId, 
                             UINT8*              pTopoCnt,    
                             T_IPT_IP_ADDR       ipAddr)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (carId != NULL)
   {
      carId[0]  = '\0';

      if (pTopoCnt != NULL)
      {
         UINT8       curTopoCnt = (UINT8) 0;

         tdcResult = TDC_MUST_FINISH_INIT;

         if (bBaseInitDone)
         {
            T_IPT_URI      uri;

            tdcResult = tdcGetUriHostPart (ipAddr, uri, &curTopoCnt);

            if (tdcResult == TDC_OK)
            {
               T_URI_LABELS      uriLabels;

               if (uriLabelizer (uri, &uriLabels))
               {
                  (void) tdcStrNCpy (carId, uriLabels.car, IPT_LABEL_SIZE);
                  tdcResult = TDC_OK;
               }
               else
               {
                  tdcResult = TDC_ERROR;
               }
            }
         }

         tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcAddr2CarId: Null - pointer for 'pTopoCnt'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcAddr2CarId: Null - pointer for 'carId'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcLabel2CstId (T_IPT_LABEL         cstId,  
                             UINT8*              pTopoCnt,    
                             const T_IPT_LABEL   carLabel)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (cstId != NULL)
   {
      cstId[0] = '\0';

      if (carLabel != NULL)
      {
         if (pTopoCnt != NULL)
         {
            UINT8       curTopoCnt = (UINT8) 0;

            tdcResult = TDC_MUST_FINISH_INIT;

            if (bBaseInitDone)
            {
               switch (dbIptLabel2CstId (carLabel, cstId, &curTopoCnt))
               {
                  case DB_OK:                {tdcResult = TDC_OK;                break;}
                  case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
                  case DB_NO_MATCHING_ENTRY:
                  case DB_ERROR:
                  default:                   {tdcResult = TDC_ERROR;             break;}
               }
            }

            tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcLabel2CstId: Null - pointer for 'pTopoCnt'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcLabel2CstId: Null - pointer for 'carLabel'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcLabel2CstId: Null - pointer for 'cstId'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcAddr2CstId  (T_IPT_LABEL         cstId,     
                             UINT8*              pTopoCnt,    
                             T_IPT_IP_ADDR       ipAddr)    
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (cstId != NULL)
   {
      cstId[0]  = '\0';

      if (pTopoCnt != NULL)
      {
         UINT8       curTopoCnt = (UINT8) 0;

         tdcResult = TDC_MUST_FINISH_INIT;

         if (bBaseInitDone)
         {
            T_IPT_URI      uri;

            tdcResult = tdcGetUriHostPart (ipAddr, uri, &curTopoCnt);

            if (tdcResult == TDC_OK)
            {
               T_URI_LABELS      uriLabels;

               if (uriLabelizer (uri, &uriLabels))
               {
                  (void) tdcStrNCpy (cstId, uriLabels.cst, IPT_LABEL_SIZE);
                  tdcResult = TDC_OK;
               }
               else
               {
                  tdcResult = TDC_ERROR;
               }
            }
         }

         tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcAddr2CstId: Null - pointer for 'pTopoCnt'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcAddr2CstId: Null - pointer for 'cstId'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcCstNo2CstId (T_IPT_LABEL         cstId,     
                             UINT8*              pTopoCnt,     
                             UINT8               trnCstNo)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (cstId != NULL)
   {
      cstId[0]  = '\0';

      if (pTopoCnt != NULL)
      {
         UINT8       curTopoCnt = (UINT8) 0;

         tdcResult = TDC_MUST_FINISH_INIT;

         if (bBaseInitDone)
         {
            //tdcResult = TDC_UNSUPPORTED_REQU; removed-raises warning in GH-CC; gweiss

            switch (dbIptCstNo2CstId (trnCstNo, cstId, &curTopoCnt))
            {
               case DB_OK:                {tdcResult = TDC_OK;                   break;}
               case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;       break;}
               case DB_NO_MATCHING_ENTRY:
               case DB_ERROR:
               default:                   {tdcResult = TDC_ERROR;                break;}
            }
         }

         tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcCstNo2CstId: Null - pointer for 'pTopoCnt'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcCstNo2CstId: Null - pointer for 'cstId'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcLabel2TrnCstNo (UINT8*              pTrnCstNo, 
                                UINT8*              pTopoCnt,    
                                const T_IPT_LABEL   carLabel) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pTrnCstNo != NULL)
   {
      *pTrnCstNo = (UINT8) 0;

      if (carLabel != NULL)
      {
         if (pTopoCnt != NULL)
         {
            UINT8       curTopoCnt = (UINT8) 0;

            tdcResult = TDC_MUST_FINISH_INIT;

            if (bBaseInitDone)
            {
               T_IPT_LABEL         cstId;

               switch (dbIptLabel2CstId (carLabel, cstId, &curTopoCnt))
               {
                  case DB_OK:
                  {
                     UINT8       lCstNo;

                     if (dbIptGetCstNo (cstId, &lCstNo, pTrnCstNo, &curTopoCnt) == DB_OK)
                     {
                        tdcResult = TDC_OK;
                     }
                     else
                     {
                        tdcResult = TDC_ERROR;
                     }
                     break;
                  }
                  case DB_NO_CONFIG:
                  {
                     tdcResult = TDC_NO_CONFIG_DATA;
                     break;
                  }
                  case DB_NO_MATCHING_ENTRY:
                  case DB_ERROR:
                  default:
                  {
                     tdcResult = TDC_ERROR;
                     break;
                  }
               }
            }

            tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcLabel2TrnCstNo: Null - pointer for 'pTopoCnt'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcLabel2TrnCstNo: Null - pointer for 'carLabel'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcLabel2TrnCstNo: Null - pointer for 'pTrnCstNo'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcAddr2TrnCstNo  (UINT8*              pTrnCstNo,
                                UINT8*              pTopoCnt,    
                                T_IPT_IP_ADDR       ipAddr)  
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pTrnCstNo != NULL)
   {
      *pTrnCstNo = (UINT8) 0;

      if (pTopoCnt != NULL)
      {
         UINT8       curTopoCnt = (UINT8) 0;

         tdcResult  = TDC_MUST_FINISH_INIT;

         if (bBaseInitDone)
         {
            T_IPT_URI      uri;

            tdcResult = TDC_UNKNOWN_IPADDR;

            if (tdcGetUriHostPart (ipAddr, uri, &curTopoCnt) == TDC_OK)
            {
               T_URI_LABELS      uriLabels;

               if (uriLabelizer (uri, &uriLabels))
               {
                  UINT8       lCstNo;

                  switch (dbIptGetCstNo (uriLabels.cst, &lCstNo, pTrnCstNo, &curTopoCnt))
                  {
                     case DB_OK:                {tdcResult = TDC_OK;                break;}
                     case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
                     case DB_NO_MATCHING_ENTRY:
                     case DB_ERROR:
                     default:                   {tdcResult = TDC_ERROR;             break;}
                  }
               }
            }
         }

         tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcAddr2TrnCstNo: Null - pointer for 'pTopoCnt'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcAddr2TrnCstNo: Null - pointer for 'pTrnCstNo'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetTrnCstCnt (UINT8*      pCstCnt,
                              UINT8*      pTopoCnt)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pCstCnt != NULL)
   {
      *pCstCnt = (UINT8) 0;

      if (pTopoCnt != NULL)
      {
         UINT8       curTopoCnt = (UINT8) 0;

         tdcResult = TDC_MUST_FINISH_INIT;

         if (bBaseInitDone)
         {
            switch (dbIptGetCstCnt (pCstCnt, &curTopoCnt))
            {
               case DB_OK:                {tdcResult = TDC_OK;                break;}
               case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
               case DB_NO_MATCHING_ENTRY:
               case DB_ERROR:
               default:                   {tdcResult = TDC_ERROR;             break;}
            }
         }

         tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcGetTrnCstCnt: Null - pointer for 'pTopoCnt'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetTrnCstCnt: Null - pointer for 'pCstCnt'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetCstCarCnt (UINT8*               pCarCnt,
                              UINT8*               pTopoCnt,
                              const T_IPT_LABEL    cstId)  
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pCarCnt != NULL)
   {
      *pCarCnt = (UINT8) 0;

      if (cstId != NULL)
      {
         if (pTopoCnt != NULL)
         {
            UINT8       curTopoCnt = (UINT8) 0;

            tdcResult = TDC_MUST_FINISH_INIT;

            if (bBaseInitDone)
            {
               switch (dbIptGetCstCarCnt (cstId, pCarCnt, &curTopoCnt))
               {
                  case DB_OK:                {tdcResult = TDC_OK;                break;}
                  case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
                  case DB_NO_MATCHING_ENTRY:
                  case DB_ERROR:
                  default:                   {tdcResult = TDC_ERROR;             break;}
               }
            }

            tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcGetCstCarCnt: Null - pointer for 'pTopoCnt'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcGetCstCarCnt: Null - pointer for 'cstId'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetCstCarCnt: Null - pointer for 'pCarCnt'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetCarDevCnt (UINT16*              pDevCnt,   
                              UINT8*               pTopoCnt,
                              const T_IPT_LABEL    cstLabel,  
                              const T_IPT_LABEL    carLabel)  
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pDevCnt != NULL)
   {
      *pDevCnt = (UINT16) 0;

      if (carLabel != NULL)
      {
         if (pTopoCnt != NULL)
         {
            UINT8       curTopoCnt = (UINT8) 0;

            tdcResult = TDC_MUST_FINISH_INIT;

            if (bBaseInitDone)
            {
               switch (dbIptGetCarDevCnt (cstLabel, carLabel, pDevCnt, &curTopoCnt))
               {
                  case DB_OK:                {tdcResult = TDC_OK;                break;}
                  case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
                  case DB_NO_MATCHING_ENTRY:
                  case DB_ERROR:
                  default:                   {tdcResult = TDC_ERROR;             break;}
               }
            }

            tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcGetCarDevCnt: Null - pointer for 'pTopoCnt'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcGetCarDevCnt: Null - pointer for 'carLabel'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetCarDevCnt: Null - pointer for 'pDevCnt'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetCarInfo (T_TDC_CAR_DATA*        pCarData, 
                            UINT8*                 pTopoCnt,
                            UINT16                 maxDev,   
                            const T_IPT_LABEL      cstLabel, 
                            const T_IPT_LABEL      carLabel) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pCarData != NULL)
   {
      pCarData->devCnt = (UINT16) 0;

      if (carLabel != NULL)
      {
         if (pTopoCnt != NULL)
         {
            UINT8       curTopoCnt = (UINT8) 0;

            tdcResult = TDC_MUST_FINISH_INIT;

            if (bBaseInitDone)
            {
               switch (dbIptGetCarInfo (pCarData, maxDev, cstLabel, carLabel, &curTopoCnt))
               {
                  case DB_OK:                   {tdcResult = TDC_OK;                      break;}
                  case DB_ERROR:                {tdcResult = (pCarData->devCnt == 0)
                                                             ? (TDC_ERROR)
                                                             : (TDC_NOT_ENOUGH_MEMORY);   break;}
                  case DB_NO_CONFIG:            {tdcResult = TDC_NO_CONFIG_DATA;          break;}
                  case DB_NO_MATCHING_ENTRY:    {tdcResult = TDC_UNKNOWN_URI;             break;}
                  default:                      {tdcResult = TDC_ERROR;                   break;}
               }
            }

            tdcResult = checkIptTopoCnt (tdcResult, curTopoCnt, pTopoCnt);
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcGetCarInfo: Null - pointer for 'pTopoCnt'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcGetCarInfo: Null - pointer for 'carLabel'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetCarInfo: Null - pointer for 'pCarData'");
   }

	/* CR-1038: Ommit error return if there was no interest in car data	*/ 
   if ((TDC_NOT_ENOUGH_MEMORY == tdcResult || TDC_ERROR == tdcResult) &&
         0 == maxDev)
   {
      tdcResult = TDC_OK;
   }
   return (tdcResult);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetUicState (UINT8*       pInaugState, 
                             UINT8*       pTopoCnt)  
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pInaugState != NULL)
   {
      *pInaugState = (UINT8) 0;

      if (pTopoCnt != NULL)
      {
         *pTopoCnt = (UINT8) 0;

         tdcResult = TDC_MUST_FINISH_INIT;

         if (bBaseInitDone)
         {
            T_DB_TRAIN_STATE        trainState;

            switch (dbGetCurTrainState (&trainState))
            {
               case DB_OK:
               {
                  *pInaugState = trainState.uicInaugState;
                  *pTopoCnt    = trainState.uicTopoCnt;
                  tdcResult    = TDC_OK;
                  break;
               }
               case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
               case DB_NO_MATCHING_ENTRY:
               case DB_ERROR:
               default:                   {tdcResult = TDC_ERROR;             break;}
            }
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcGetUicState: Null - pointer for 'pTopoCnt'");
      }
   }
   else
   {
      if (pTopoCnt != NULL)
      {
         *pTopoCnt = (UINT8) 0;
      }
      DEBUG_WARN (MOD_MAIN, "tdcGetUicState: Null - pointer for 'pInaugState'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetUicGlobalData (T_TDC_UIC_GLOB_DATA*   pGlobData,
                                  UINT8*                 pTopoCnt)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pGlobData != NULL)
   {
      pGlobData->trnCarCnt = (UINT8) 0;

      if (pTopoCnt != NULL)
      {
         tdcResult = TDC_MUST_FINISH_INIT;

         if (bBaseInitDone)
         {
            switch (dbUicGetGlobalData (pGlobData))
            {
               case DB_OK:                {tdcResult = TDC_OK;                break;}
               case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
               case DB_NO_MATCHING_ENTRY:
               case DB_ERROR:
               default:                   {tdcResult = TDC_ERROR;             break;}
            }
         }
         
         tdcResult = checkUicTopoCnt (tdcResult, pTopoCnt);
      }
      else
      {
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetUicGlobalData: Null - pointer for 'pGlobData'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcGetUicCarData  (T_TDC_UIC_CAR_DATA*   pCarData,
                                UINT8*                pTopoCnt, 
                                UINT8                 carSeqNo)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pCarData != NULL)
   {
      pCarData->contrCarCnt = (UINT8) 0;

      if (pTopoCnt != NULL)
      {
         tdcResult = TDC_MUST_FINISH_INIT;

         if (bBaseInitDone)
         {
            switch (dbUicGetCarData (pCarData, carSeqNo))
            {
               case DB_OK:                   {tdcResult = TDC_OK;                   break;}
               case DB_NO_CONFIG:            {tdcResult = TDC_NO_CONFIG_DATA;       break;}
               case DB_NO_MATCHING_ENTRY:    {tdcResult = TDC_NO_MATCHING_ENTRY;    break;}
               case DB_ERROR:
               default:                      {tdcResult = TDC_ERROR;                break;}
            }
         }

         tdcResult = checkUicTopoCnt (tdcResult, pTopoCnt);
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcGetUicCarData: Null - pointer for 'pTopoCnt'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcGetUicCarData: Null - pointer for 'pCarData'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcLabel2UicCarSeqNo (UINT8*              pCarSeqNo,
                                   UINT8*              pIptTopoCnt,
                                   UINT8*              pUicTopoCnt, 
                                   const T_IPT_LABEL   cstLabel, 
                                   const T_IPT_LABEL   carLabel) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pCarSeqNo != NULL)
   {
      *pCarSeqNo = (UINT8) 0;

      if (cstLabel != NULL)
      {
         if (carLabel != NULL)
         {
            if (pIptTopoCnt != NULL)
            {
               if (pUicTopoCnt != NULL)
               {
                  tdcResult = TDC_MUST_FINISH_INIT;

                  if (bBaseInitDone)
                  {
                     switch (dbGetUicCarSeqNo (pCarSeqNo, cstLabel, carLabel))
                     {
                        case DB_OK:                {tdcResult = TDC_OK;                break;}
                        case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
                        case DB_NO_MATCHING_ENTRY:
                        case DB_ERROR:
                        default:                   {tdcResult = TDC_ERROR;             break;}
                     }
                  }

                  tdcResult = check2TopoCnt (tdcResult, pIptTopoCnt, pUicTopoCnt);
               }
               else
               {
                  DEBUG_WARN (MOD_MAIN, "tdcLabel2UicCarSeqNo: Null - pointer for 'pUicTopoCnt'");
               }
            }
            else
            {
               DEBUG_WARN (MOD_MAIN, "tdcLabel2UicCarSeqNo: Null - pointer for 'pIptTopoCnt'");
            }
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcLabel2UicCarSeqNo: Null - pointer for 'carLabel'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcLabel2UicCarSeqNo: Null - pointer for 'cstLabel'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcLabel2UicCarSeqNo: Null - pointer for 'pCarSeqNo'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcAddr2UicCarSeqNo  (UINT8*              pCarSeqNo,
                                   UINT8*              pIptTopoCnt,
                                   UINT8*              pUicTopoCnt, 
                                   T_IPT_IP_ADDR       ipAddr)   
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pCarSeqNo != NULL)
   {
      *pCarSeqNo = (UINT8) 0;

      if (pIptTopoCnt != NULL)
      {
         if (pUicTopoCnt != NULL)
         {
            tdcResult  = TDC_MUST_FINISH_INIT;

            if (bBaseInitDone)
            {
               T_IPT_URI      uri;

               tdcResult = TDC_UNKNOWN_IPADDR;

               if (tdcGetUriHostPart (ipAddr, uri, pIptTopoCnt) == TDC_OK)
               {
                  T_URI_LABELS      uriLabels;

                  if (uriLabelizer (uri, &uriLabels))
                  {
                     switch (dbGetUicCarSeqNo (pCarSeqNo, uriLabels.cst, uriLabels.car))
                     {
                        case DB_OK:                {tdcResult = TDC_OK;                break;}
                        case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
                        case DB_NO_MATCHING_ENTRY:
                        case DB_ERROR:
                        default:                   {tdcResult = TDC_ERROR;             break;}
                     }
                  }
               }
            }

            tdcResult = check2TopoCnt (tdcResult, pIptTopoCnt, pUicTopoCnt);
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcAddr2UicCarSeqNo: Null - pointer for 'pUicTopoCnt'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcAddr2UicCarSeqNo: Null - pointer for 'pIptTopoCnt'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_MAIN, "tdcAddr2UicCarSeqNo: Null - pointer for 'pCarSeqNo'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */

T_TDC_RESULT tdcUicCarSeqNo2Ids (T_IPT_LABEL         cstId, 
                                 T_IPT_LABEL         carId, 
                                 UINT8*              pIptTopoCnt,
                                 UINT8*              pUicTopoCnt, 
                                 UINT8               carSeqNo) 
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (cstId != NULL)
   {
      cstId[0] = '\0';

      if (carId != NULL)
      {
         carId[0] = '\0';

         if (pIptTopoCnt != NULL)
         {
            if (pUicTopoCnt != NULL)
            {
               tdcResult   = TDC_MUST_FINISH_INIT;

               if (bBaseInitDone)
               {
                  switch (dbUicCarSeqNo2Ids (cstId, carId, carSeqNo))
                  {
                     case DB_OK:                {tdcResult = TDC_OK;                break;}
                     case DB_NO_CONFIG:         {tdcResult = TDC_NO_CONFIG_DATA;    break;}
                     case DB_NO_MATCHING_ENTRY:
                     case DB_ERROR:
                     default:                   {tdcResult = TDC_ERROR;             break;}
                  }
               }

               tdcResult = check2TopoCnt (tdcResult, pIptTopoCnt, pUicTopoCnt);
            }
            else
            {
               DEBUG_WARN (MOD_MAIN, "tdcUicCarSeqNo2Ids: Null - pointer for 'pUicTopoCnt'");
            }
         }
         else
         {
            DEBUG_WARN (MOD_MAIN, "tdcUicCarSeqNo2Ids: Null - pointer for 'pIptTopoCnt'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_MAIN, "tdcUicCarSeqNo2Ids: Null - pointer for 'carId'");
      }
   }
   else
   {
      if (carId != NULL)
      {
         carId[0] = '\0';
      }
      DEBUG_WARN (MOD_MAIN, "tdcUicCarSeqNo2Ids: Null - pointer for 'cstId'");
   }

   return (tdcResult);
}

/* -------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
      
/*@null@*/ static void* tdcMain (void* pArgV); 

static void* tdcMain (void* pArgV)
{
   (void) tdcPrintf ("\nIP-Train Directory Client - Buildtime: %s, %s\n", __DATE__, __TIME__);

   tdcStartupFw (pArgV);

   /* Initialization should be finished by now, switch to run-time Debug Level */
   /* MRi 2007/06/15, moved to message data reception task. Called after first reception */
   /* valid Configuration data */
   /* tdcSetDebugLevel (tdcDbgLevelRun, NULL, NULL);*/

   // Don't terminate the main thread as long as tdc is running!

   DEBUG_INFO (MOD_MAIN, "Waiting for startupSemaId");

   if (tdcWaitSema (MOD_MAIN, startupSemaId, TDC_SEMA_WAIT_FOREVER) != TDC_SEMA_OK)
   {
      //DEBUG_WARN (MOD_MAIN, "Error waiting on startupSemaId");
   }
                  
   (void) tdcSemaDelete (MOD_MAIN, &startupSemaId);  

   return (NULL);
}

/* ---------------------------------------------------------------------------- */

static /*@null@*/ void* tdcMainStart (/*@in@*/ void*      pArgV)
{
   (void) tdcPrintf  ("TDC:     %s Ver. %d.%d.%d.%d started successfully\n", 
                      APP_NAME, TDC_VERSION, TDC_RELEASE, TDC_UPDATE, TDC_EVOLUTION);
   (void) tdcMain    (pArgV);
          DEBUG_INFO (MOD_MAIN, "Terminated");
   return (NULL);
}
/* ---------------------------------------------------------------------------- */

static const T_TDC_BOOL          always = TRUE;

static const T_THREAD_FRAME     threadIdMain =
{
   &always,
   APP_NAME,
   TASKNAME_MAIN,
   TASK_TYPE,
   DEFAULT_PRIORITY,
   DEFAULT_STACK_SIZE,
   NULL,
   tdcMainStart,
   T_MAIN_INDEX
};

/* ---------------------------------------------------------------------------- */

int tdcPrepareInit (int startupMode, int RAMaddr)
{
   (void) tdcPrintf  ("TDC:     tdcPrepareInit() called\n");

   TDC_UNUSED(startupMode)
   TDC_UNUSED(RAMaddr)

   tdcThreadIdTab[threadIdMain.threadIdx] = startupTdcSingleThread (&threadIdMain);

   if (tdcThreadIdTab[threadIdMain.threadIdx] != NULL)
   {
      return (0);
   }

   return (-1);
}

/* ---------------------------------------------------------------------------- */

static T_TDC_BOOL tdcTerminateMain ()
{
   T_TDC_BOOL        bOk = TRUE;

   if (!dbDeleteDB ())
   {
      bOk = FALSE;
   }

   if (!tdcRemoveSignalHandler ())
   {
      bOk = FALSE;
   }

   if (!tdcDeleteMutexes ())
   {
      bOk = FALSE;
   }

   if (!tdcDeleteSemaphores ())
   {
      bOk = FALSE;
   }

   if (!tdcRemoveITimer (cycleTimeIPTDirPD))
   {
       bOk = FALSE;
   }
    
   return (bOk);
}

/* ---------------------------------------------------------------------------- */

/* The function tdcTerminate shall later on prepare a shut down */
T_TDC_RESULT tdcTerminate (void)
{
	/* Do nothing */
	return TDC_OK;
}

/* ---------------------------------------------------------------------------- */

T_TDC_RESULT  tdcDestroy     (void)
{
   T_TDC_BOOL        bOk = TRUE;

   DEBUG_WARN (MOD_MAIN, "TDC - Termination requested");

   bTerminate = TRUE;
   
   /* This prevents access of internal data structures during termination */
   bBaseInitDone = FALSE;           

   tdcTerminateMD (MOD_MAIN);          // MD Thread terminates if there's a problem with MD queue
   // tdcTerminatePD (MOD_MAIN);       // gracefull termination of Processdata Thread is anyway within PD_CYCLE, 
                                       // simply by watching 'bTerminate' variable

   /* Wait until all threads terminated completely */

   if (!tdcTerminateIpcServ ())      /* IpcServ is responsible for killing all its childs */
   {
      bOk = FALSE;
   }

   if (!tdcTerminateTMsgData ())
   {
      bOk = FALSE;
   }

   if (!tdcTerminateTCyclic ())
   {
      bOk = FALSE;
   }

   if (!tdcTerminateMain ())
   {
      bOk = FALSE;
   }

   return (bOk);
}

/* ---------------------------------------------------------------------------- */

typedef struct
{
                  T_TDC_RESULT            tdcResult;
   /*@shared@*/   const char*             pErrInfo;
} T_TDC_ERROR_INFO;

static const char*               pErrInfoUnknown = "Unknown Errorcode specified";
static const T_TDC_ERROR_INFO    errInfoTab[]    =
{
   {TDC_OK,                   "NO error occured"},
   {TDC_UNSUPPORTED_REQU,     "Request is not supported by TDC"},
   {TDC_UNKNOWN_URI,          "An unknown or invalid URI was specified"},
   {TDC_UNKNOWN_IPADDR,       "An unknown or invalid IP-Address was specified"},
   {TDC_NO_CONFIG_DATA,       "Wait until TDC has got configuration data from IPTDir-Server"},
   {TDC_NULL_POINTER_ERROR,   "A null pointer was detected for a mandatory API parameter"},
   {TDC_NOT_ENOUGH_MEMORY,    "There is not enough memory to store the data"},
   {TDC_NO_MATCHING_ENTRY,    "TDC didn't find a matching entry"},
   {TDC_MUST_FINISH_INIT,     "TDC could not yet finish basic initialization"},
   {TDC_INVALID_LABEL,        "An unknown or invalid Label was specified"},
   {TDC_WRONG_TOPOCOUNT,      "The specified topo counter no longer matches the configuration"},
   {TDC_ERROR,                "A general Error occured"},
};


const char* tdcGetErrorString (int     errCode)
{
   const char*       pErrInfo  = pErrInfoUnknown;
   T_TDC_RESULT      tdcResult = (T_TDC_RESULT) errCode;
   int               i;

   for (i = 0; i < TAB_SIZE (errInfoTab); i++)
   {
      if (tdcResult == errInfoTab[i].tdcResult)
      {
         pErrInfo = errInfoTab[i].pErrInfo;
         break;
      }
   }

   return (pErrInfo);
}

/* ---------------------------------------------------------------------------- */

UINT32 tdcGetVersion     (void)
{
   UINT32   myVersion = (   (TDC_VERSION   << 24)
                          | (TDC_RELEASE   << 16)
                          | (TDC_UPDATE    << 8)
                          | (TDC_EVOLUTION << 0)
                        );

   return (myVersion);
}







