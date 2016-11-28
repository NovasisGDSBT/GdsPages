/*
 *  $Id: tdcPiq.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    Application Interface for the IP-Train Directory Client
 *                 (TDC) for position independant queries
 *
 *  AUTHOR         M.Ritz
 *
 *  REMARKS        While the TDC-API defined in tdcApi.h is position-depending
 *                 i.e. depending on the device localhost, the API defined
 *                 here may be called to deliver results like it is called
 *                 from a device with baseIpAddr
 *
 *  DEPENDENCIES   Either the switch VXWORKS, INTEGRITY, LINUX or WIN32 has to be set
 *
 *  MODIFICATIONS (log starts 2010-08-11)
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *          Adding corrections for TÜV-Assessment
 *
 *  All rights reserved. Reproduction, modification, use or disclosure
 *  to third parties without express authority is forbidden.
 *  Copyright Bombardier Transportation GmbH, Germany, 2002-2012.
 *
 */

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

#include <stdio.h>

#include "tdc.h"
#include "tdcApiPiq.h"

// -----------------------------------------------------------------------------

#define MOD_PIQ                     "TDC_PIQ"

// -----------------------------------------------------------------------------

typedef struct
{
   T_IPT_LABEL             dev;
   T_IPT_LABEL             car;
   T_IPT_LABEL             cst;
   T_IPT_LABEL             trn;
} T_URI_LABELS;

typedef struct
{
   T_TDC_BOOL              bOK;
   T_IPT_IP_ADDR           ipAddr;
   UINT8                   topoCnt;
   T_IPT_LABEL             dev;
   T_IPT_LABEL             car;
   T_IPT_LABEL             cst;
} T_LOCAL_DEV_INFO;

typedef struct
{
   UINT8*            pTopoCnt;
   T_IPT_IP_ADDR     baseIpAddr;
   const char*       pUri;
   T_IPT_URI         adaptedUri;
} T_ADAPT_URI;

typedef struct
{
   T_TDC_BOOL        bDevLabel;
   T_TDC_BOOL        bCarLabel;
   T_TDC_BOOL        bCstLabel;
   UINT8*            pTopoCnt;
   T_IPT_IP_ADDR     baseIpAddr;
   const char*       pDevLabel;
   const char*       pCarLabel;
   const char*       pCstLabel;
   const char*       pAdaptedDevLabel;
   const char*       pAdaptedCarLabel;
   const char*       pAdaptedCstLabel;
   T_IPT_LABEL       devLabel;
   T_IPT_LABEL       carLabel;
   T_IPT_LABEL       cstLabel;
} T_ADAPT_LABELS;

// -----------------------------------------------------------------------------

static  T_TDC_MUTEX_ID                    tdcPiqMutexId = NULL;
#define MUST_CREATE_MUTEX(mutexId)        (mutexId == NULL)

// ----------------------------------------------------------------------------

static const T_IPT_LABEL                  lDev        = "lDev";
static const T_IPT_LABEL                  lCar        = "lCar";
static const T_IPT_LABEL                  lCst        = "lCst";
static const T_IPT_LABEL                  lTrn        = "lTrn";
static const T_IPT_IP_ADDR                localHostIP = UC_ADDR (127, 0, 0, 1);

static T_LOCAL_DEV_INFO                   locDevInfo  = {FALSE, UC_ADDR (0, 0, 0, 0), 0, "", "", ""};

// ----------------------------------------------------------------------------

static T_TDC_BOOL    lockTdcPiqMutex      (void);
static T_TDC_BOOL    unlockTdcPiqMutex    (void);

static T_TDC_BOOL    uriLabelizer         (const T_IPT_URI        uri,
                                 /*@out@*/ T_URI_LABELS*          pUriLbls);
static T_TDC_RESULT  getBaseUriLbls       (T_IPT_IP_ADDR          baseIpAddr,
                                           UINT8*                 pTopoCnt,
                                           T_URI_LABELS*          pBaseUriLbls);
static T_TDC_RESULT  getAdaptedUri        (T_ADAPT_URI*           pAdaptUri);
static T_TDC_RESULT  getAdaptedLabels     (T_ADAPT_LABELS*        pAdaptLbls);
static T_IPT_IP_ADDR getAdaptedIpAddr     (T_IPT_IP_ADDR          ipAddr,
                                           T_IPT_IP_ADDR          baseIpAddr);

static T_TDC_RESULT  adaptLocalHost       (T_IPT_IP_ADDR*         pIpAddr);
static T_TDC_RESULT  getLocDevInfo        (void);

// ----------------------------------------------------------------------------

static T_TDC_BOOL lockTdcPiqMutex (void)
{
   if (MUST_CREATE_MUTEX (tdcPiqMutexId))
   {
      T_TDC_MUTEX_STATUS      mutexStatus;

      tdcPiqMutexId = tdcCreateMutex (MOD_PIQ, "tdcPiqLib", &mutexStatus);

      if (MUST_CREATE_MUTEX (tdcPiqMutexId))
      {
         return (FALSE);
      }
   }

   return (tdcMutexLock (MOD_PIQ, tdcPiqMutexId) == TDC_MUTEX_OK);
}

// ----------------------------------------------------------------------------

static T_TDC_BOOL unlockTdcPiqMutex (void)
{
   return (tdcMutexUnlock (MOD_PIQ, tdcPiqMutexId) == TDC_MUTEX_OK);
}

// -----------------------------------------------------------------------------

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
      carLen = tdcStrLen (lCar);
      (void) tdcStrNCpy (pUriLbls->car, lCar, IPT_MAX_LABEL_LEN);
   }
   else
   {
      carLen = tdcStrLen (pCar);
      (void) tdcStrNCpy (pUriLbls->car, pCar, IPT_MAX_LABEL_LEN);
   }

   if (pCst == NULL)
   {
      cstLen = tdcStrLen (lCst);
      (void) tdcStrNCpy (pUriLbls->cst, lCst, IPT_MAX_LABEL_LEN);
   }
   else
   {
      cstLen = tdcStrLen (pCst);
      (void) tdcStrNCpy (pUriLbls->cst, pCst, IPT_MAX_LABEL_LEN);
   }

   if (pTrn == NULL)
   {
      trnLen = tdcStrLen (lTrn);
      (void) tdcStrNCpy (pUriLbls->trn, lTrn, IPT_MAX_LABEL_LEN);
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

// ----------------------------------------------------------------------------

static T_TDC_RESULT getBaseUriLbls (T_IPT_IP_ADDR     baseIpAddr,
                                    UINT8*            pIptTopoCnt,
                                    T_URI_LABELS*     pBaseUriLbls)
{
   T_IPT_URI      iptUri    = "";
   T_TDC_RESULT   tdcResult = tdcGetUriHostPart (baseIpAddr, iptUri, pIptTopoCnt);

   if (tdcResult == TDC_OK)
   {
      if (!uriLabelizer (iptUri, pBaseUriLbls))
      {
         tdcResult = TDC_ERROR;
         DEBUG_ERROR (MOD_PIQ, "tdcGetUriHostPart returned strange ERROR");
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

static T_TDC_RESULT getAdaptedUri (T_ADAPT_URI*           pAdaptUri)
{

   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (pAdaptUri->pUri != NULL)
   {
      T_URI_LABELS      uriLbls;

      tdcResult = TDC_UNKNOWN_URI;

      if (uriLabelizer (pAdaptUri->pUri, &uriLbls))
      {  // Check if one of the following laiases is used: lDev, lCar and lCst
         // in case, replace them by absolute names according baseIpAddr

         if (    (tdcStrICmp (uriLbls.dev, lDev) == 0)
              || (tdcStrICmp (uriLbls.car, lCar) == 0)
              || (tdcStrICmp (uriLbls.cst, lCst) == 0)
            )
         {
            T_URI_LABELS      baseUriLbls;

            if ((tdcResult = getBaseUriLbls (pAdaptUri->baseIpAddr, pAdaptUri->pTopoCnt, &baseUriLbls)) == TDC_OK)
            {
               if (tdcStrICmp (uriLbls.dev, lDev) == 0)
               {
                  (void) tdcStrNCpy (uriLbls.dev, baseUriLbls.dev, IPT_LABEL_SIZE);
               }

               if (tdcStrICmp (uriLbls.car, lCar) == 0)
               {
                  (void) tdcStrNCpy (uriLbls.car, baseUriLbls.car, IPT_LABEL_SIZE);
               }

               if (tdcStrICmp (uriLbls.cst, lCst) == 0)
               {
                  (void) tdcStrNCpy (uriLbls.cst, baseUriLbls.cst, IPT_LABEL_SIZE);
               }

               (void) tdcSNPrintf (pAdaptUri->adaptedUri, IPT_URI_SIZE, "%s.%s.%s", 
                                   uriLbls.dev, uriLbls.car, uriLbls.cst);
            }
         }
         else
         {
            tdcResult = TDC_OK;

            (void) tdcStrNCpy (pAdaptUri->adaptedUri, pAdaptUri->pUri, IPT_URI_SIZE);
         }
      }
   }
   else
   {
      DEBUG_WARN (MOD_PIQ, "Null - pointer for 'uri'");
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

static T_TDC_RESULT getAdaptedLabels (T_ADAPT_LABELS*      pAdaptLbls)
{
   T_TDC_RESULT      tdcResult = TDC_OK;

   pAdaptLbls->bDevLabel        =    pAdaptLbls->bDevLabel
                                  && (pAdaptLbls->pDevLabel != NULL)
                                  && (tdcStrICmp (pAdaptLbls->pDevLabel, lDev) == 0);
   pAdaptLbls->bCarLabel        =    pAdaptLbls->bCarLabel
                                  && (pAdaptLbls->pCarLabel != NULL)
                                  && (tdcStrICmp (pAdaptLbls->pCarLabel, lCar) == 0);
   pAdaptLbls->bCstLabel        =    pAdaptLbls->bCstLabel
                                  && (pAdaptLbls->pCstLabel != NULL)
                                  && (tdcStrICmp (pAdaptLbls->pCstLabel, lCst) == 0);
   pAdaptLbls->pAdaptedDevLabel = pAdaptLbls->pDevLabel;
   pAdaptLbls->pAdaptedCarLabel = pAdaptLbls->pCarLabel;
   pAdaptLbls->pAdaptedCstLabel = pAdaptLbls->pCstLabel;

   if (    pAdaptLbls->bDevLabel
        || pAdaptLbls->bCarLabel
        || pAdaptLbls->bCstLabel
      )
   {
      T_URI_LABELS      baseUriLbls;

      if ((tdcResult = getBaseUriLbls (pAdaptLbls->baseIpAddr, pAdaptLbls->pTopoCnt, &baseUriLbls)) == TDC_OK)
      {
         if (pAdaptLbls->bDevLabel)
         {
            (void) tdcStrNCpy (pAdaptLbls->devLabel, baseUriLbls.dev, IPT_LABEL_SIZE);
            pAdaptLbls->pAdaptedDevLabel = pAdaptLbls->devLabel;
         }

         if (pAdaptLbls->bCarLabel)
         {
            (void) tdcStrNCpy (pAdaptLbls->carLabel, baseUriLbls.car, IPT_LABEL_SIZE);
            pAdaptLbls->pAdaptedCarLabel = pAdaptLbls->carLabel;
         }

         if (pAdaptLbls->bCstLabel)
         {
            (void) tdcStrNCpy (pAdaptLbls->cstLabel, baseUriLbls.cst, IPT_LABEL_SIZE);
            pAdaptLbls->pAdaptedCstLabel = pAdaptLbls->cstLabel;
         }
      }
   }

   return (tdcResult);
}

// ----------------------------------------------------------------------------

static T_IPT_IP_ADDR getAdaptedIpAddr (T_IPT_IP_ADDR      ipAddr,
                                       T_IPT_IP_ADDR      baseIpAddr)
{
   return ((ipAddr == localHostIP) ? (baseIpAddr) : (ipAddr));
}

// ----- adaptLocalHost() ------------------------------------------------------
// Abstract    : No position independent query shall return 127.0.0.1 but always
//               the full IP Address as seen on the Bus
// Parameter(s): pIpAddr  - A pointer to any valid IP Address
// Return value: TDC_OK if everything is ok.
// Remarks     : TDCD is running on any one of the devices on the Consist-Ring.
//               If TDCD returns 127.0.0.1, it refres to the device it is 
//               running on.
// History     : 08-10-13     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
static T_TDC_RESULT adaptLocalHost (T_IPT_IP_ADDR*   pIpAddr)
{
   T_TDC_RESULT   tdcResult = TDC_OK;
   T_IPT_IP_ADDR  ipAddr    = *pIpAddr;

   if (ipAddr == localHostIP)
   {
      tdcResult = TDC_ERROR;

      if (lockTdcPiqMutex ())
      {
         tdcResult = getLocDevInfo ();

         if (tdcResult == TDC_OK)
         {
            ipAddr = locDevInfo.ipAddr;
         }

         unlockTdcPiqMutex ();/*lint !e534 ignoring return is ok here*/
      }
   }

   *pIpAddr = ipAddr;

   return (tdcResult);
}

// -----------------------------------------------------------------------------

#define CAR_DATA_SIZE(cnt)          ((UINT32) (sizeof (T_TDC_CAR_DATA)  + ((cnt - 1) * sizeof (T_TDC_DEV_DATA))))

static T_TDC_RESULT getLocDevInfo (void)
{
   UINT8          topoCnt    = 0;
   UINT8          inaugState = 0;
   T_TDC_RESULT   tdcResult  = tdcGetIptState (&inaugState, &topoCnt);

   if (tdcResult == TDC_OK)
   {
      locDevInfo.bOK =    (locDevInfo.bOK)  
                       && (locDevInfo.topoCnt == topoCnt);

      if (!locDevInfo.bOK)
      {  // determine local device Info incl. IP-Address

         if ((tdcResult = tdcGetOwnIds (locDevInfo.dev, locDevInfo.car, locDevInfo.cst)) == TDC_OK)
         {
            UINT16               maxDev   = 200;
            T_TDC_CAR_DATA*      pCarData = (T_TDC_CAR_DATA *) tdcAllocMem (CAR_DATA_SIZE (maxDev));

            if (pCarData != NULL)
            {
               if ((tdcResult = tdcGetCarInfo (pCarData, &topoCnt, maxDev, lCst, lCar)) == TDC_OK)
               {
                  UINT16      devNo;

                  tdcResult = TDC_ERROR;

                  for (devNo = 0; devNo < pCarData->devCnt; devNo++)
                  {
                     if (tdcStrICmp (pCarData->devData[devNo].devId, locDevInfo.dev) == 0)
                     {
                        // devId must be unique on local car --> local Host found 
   
                        locDevInfo.ipAddr  = UC_ADDR (10, 0, 0, pCarData->devData[devNo].hostId);
                        locDevInfo.topoCnt = topoCnt;
                        locDevInfo.bOK     = TRUE;
                        tdcResult          = TDC_OK;
                        break;
                     }
                  }
               }

               tdcFreeMem (pCarData);
            }
            else
            {
               tdcResult = TDC_NOT_ENOUGH_MEMORY;
               DEBUG_ERROR (MOD_PIQ, "Out of dynamic Memory");
            }
         }
      }
   }
   else
   {
      locDevInfo.bOK = FALSE;
   }

   return (tdcResult);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------

// ----- tdcGetErrorStringPiq() ------------------------------------------------
// Abstract    : Position independent version of function tdcGetErrorString
// Parameter(s): s. tdcGetErrorString
// Return value: s. tdcGetErrorString
// Remarks     : tdcGetErrorString is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-10     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
const char* tdcGetErrorStringPiq (int              errCode, 
                                  T_IPT_IP_ADDR    baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcGetErrorString (errCode));
}

// ----- tdcGetVersionPiq() ----------------------------------------------------
// Abstract    : Position independent version of function tdcGetVersion
// Parameter(s): s. tdcGetVersion
// Return value: s. tdcGetVersion
// Remarks     : tdcGetVersion is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-10     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
UINT32 tdcGetVersionPiq (T_IPT_IP_ADDR    baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcGetVersion ());
}

// ----- tdcGetTrnBackboneTypePiq() --------------------------------------------
// Abstract    : Position independent version of function tdcGetTrnBackboneType
// Parameter(s): s. tdcGetTrnBackboneType
// Return value: s. tdcGetTrnBackboneType
// Remarks     : tdcGetTrnBackboneType is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-10     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetTrnBackboneTypePiq ( UINT8*	            pTbType,
                                        T_IPT_IP_ADDR*      pGatewayIpAddr,
                                        T_IPT_IP_ADDR       baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcGetTrnBackboneType (pTbType, pGatewayIpAddr));
}

// ----- tdcGetIptStatePiq() ---------------------------------------------------
// Abstract    : Position independent version of function tdcGetIptState
// Parameter(s): s. tdcGetIptState
// Return value: s. tdcGetIptState
// Remarks     : tdcGetIptState is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-10     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetIptStatePiq (UINT8*             pInaugState, 
                                UINT8*             pTopoCnt,
                                T_IPT_IP_ADDR      baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcGetIptState (pInaugState, pTopoCnt));
}

// ----- tdcGetOwnIdsPiq() -----------------------------------------------------
// Abstract    : Position independent version of function tdcGetOwnIds
// Parameter(s): s. tdcGetOwnIds
// Return value: s. tdcGetOwnIds
// Remarks     : 
// History     : 08-10-10     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetOwnIdsPiq (T_IPT_LABEL       devId,
                              T_IPT_LABEL       carId,       
                              T_IPT_LABEL       cstId,
                              T_IPT_IP_ADDR     baseIpAddr)
{
   T_TDC_RESULT      tdcResult = TDC_NULL_POINTER_ERROR;

   if (devId != NULL)
   {
      if (carId != NULL)
      {
         if (cstId != NULL)
         {
            UINT8          iptTopoCnt = (UINT8) 0;
            T_URI_LABELS   baseUriLbls;

            if ((tdcResult = getBaseUriLbls (baseIpAddr, &iptTopoCnt, &baseUriLbls)) == TDC_OK)
            {
               (void) tdcStrNCpy (devId, baseUriLbls.dev, IPT_MAX_LABEL_LEN);
               (void) tdcStrNCpy (carId, baseUriLbls.car, IPT_MAX_LABEL_LEN);
               (void) tdcStrNCpy (cstId, baseUriLbls.cst, IPT_MAX_LABEL_LEN);
            }
         }
         else
         {
            DEBUG_WARN (MOD_PIQ, "tdcGetOwnIdsPiq: Null - pointer for 'cstId'");
         }
      }
      else
      {
         DEBUG_WARN (MOD_PIQ, "tdcGetOwnIdsPiq: Null - pointer for 'carId'");
      }
   }
   else
   {
      DEBUG_WARN (MOD_PIQ, "tdcGetOwnIdsPiq: Null - pointer for 'devId'");
   }

   if (tdcResult != TDC_OK)
   {
      if (devId != NULL)
      {
         devId[0] = '\0';
      }

      if (carId != NULL)
      {
         carId[0] = '\0';
      }

      if (cstId != NULL)
      {
         cstId[0] = '\0';
      }
   }

   return (tdcResult);
}

// ----- tdcGetAddrByNamePiq() -------------------------------------------------
// Abstract    : Position independent version of function tdcGetAddrByNamePiq
// Parameter(s): s. tdcGetAddrByNamePiq
// Return value: s. tdcGetAddrByNamePiq
// Remarks     : 
// History     : 08-10-10     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetAddrByNamePiq (const T_IPT_URI       uri,   
                                  T_IPT_IP_ADDR*        pIpAddr,
                                  UINT8*                pTopoCnt,
                                  T_IPT_IP_ADDR         baseIpAddr)
{
   T_ADAPT_URI       adaptUri;
   T_TDC_RESULT      tdcResult;

   tdcMemClear (&adaptUri, (UINT32) sizeof (T_ADAPT_URI));

   adaptUri.baseIpAddr = baseIpAddr;
   adaptUri.pTopoCnt   = pTopoCnt;
   adaptUri.pUri       = uri;

   if ((tdcResult = getAdaptedUri (&adaptUri)) == TDC_OK)
   {
      if ((tdcResult = tdcGetAddrByName (adaptUri.adaptedUri, pIpAddr, pTopoCnt)) == TDC_OK)
      {
         // Ensure that address is NOT localhost "127.0.0.1"

         tdcResult = adaptLocalHost (pIpAddr);
         if (tdcResult != TDC_OK)
         {
            tdcPrintf ("\nadaptLocalHost failed!\n");
         }
      }
      else
      {
         tdcPrintf ("\ntdcGetAddrByName failed!\n");
      }
   }
   else
   {
      tdcPrintf ("\ngetAdaptedUri failed!\n");
   }

   return (tdcResult);
}

// ----- tdcGetAddrByNameExtPiq() ----------------------------------------------
// Abstract    : Position independent version of function tdcGetAddrByNameExtPiq
// Parameter(s): s. tdcGetAddrByNameExtPiq
// Return value: s. tdcGetAddrByNameExtPiq
// Remarks     : 
// History     : 08-10-13     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetAddrByNameExtPiq (const T_IPT_URI     uri, 
                                     T_IPT_IP_ADDR*      pIpAddr,
                                     T_TDC_BOOL*         pIsFRG,
                                     UINT8*              pTopoCnt,
                                     T_IPT_IP_ADDR       baseIpAddr)
{
   T_ADAPT_URI       adaptUri;
   T_TDC_RESULT      tdcResult;

   tdcMemClear (&adaptUri, (UINT32) sizeof (T_ADAPT_URI));

   adaptUri.baseIpAddr = baseIpAddr;
   adaptUri.pTopoCnt   = pTopoCnt;
   adaptUri.pUri       = uri;

   if ((tdcResult = getAdaptedUri (&adaptUri)) == TDC_OK)
   {
      if ((tdcResult = tdcGetAddrByNameExt (adaptUri.adaptedUri, pIpAddr, pIsFRG, pTopoCnt)) == TDC_OK)
      {
         // Ensure that address is NOT localhost "127.0.0.1"

         tdcResult = adaptLocalHost (pIpAddr);
      }
   }

   return (tdcResult);
}

// ----- tdcGetUriHostPartPiq() ------------------------------------------------
// Abstract    : Position independent version of function tdcGetUriHostPartPiq
// Parameter(s): s. tdcGetUriHostPartPiq
// Return value: s. tdcGetUriHostPartPiq
// Remarks     : 
// History     : 08-10-13     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetUriHostPartPiq (T_IPT_IP_ADDR      ipAddr,
                                   T_IPT_URI          uri,
                                   UINT8*             pTopoCnt,
                                   T_IPT_IP_ADDR      baseIpAddr)
{
   T_IPT_IP_ADDR     queryIpAddr = getAdaptedIpAddr (ipAddr, baseIpAddr);
                         
   return (tdcGetUriHostPart (queryIpAddr, uri, pTopoCnt));
}

// ----- tdcLabel2CarIdPiq() ---------------------------------------------------
// Abstract    : Position independent version of function tdcLabel2CarIdPiq
// Parameter(s): s. tdcLabel2CarIdPiq
// Return value: s. tdcLabel2CarIdPiq
// Remarks     : 
// History     : 08-10-13     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcLabel2CarIdPiq (T_IPT_LABEL           carId,
                                UINT8*                pTopoCnt,    
                                const T_IPT_LABEL     cstLabel, 
                                const T_IPT_LABEL     carLabel,
                                T_IPT_IP_ADDR         baseIpAddr)
{
   T_ADAPT_LABELS    adaptLbls;
   T_TDC_RESULT      tdcResult;

   tdcMemClear (&adaptLbls, (UINT32) sizeof (adaptLbls));

   adaptLbls.bCarLabel  = TRUE;
   adaptLbls.bCstLabel  = TRUE;
   adaptLbls.pTopoCnt   = pTopoCnt;
   adaptLbls.baseIpAddr = baseIpAddr;
   adaptLbls.pCstLabel  = cstLabel;
   adaptLbls.pCarLabel  = carLabel;

   if ((tdcResult = getAdaptedLabels (&adaptLbls)) == TDC_OK)
   {
      tdcResult = tdcLabel2CarId (carId, pTopoCnt, adaptLbls.pAdaptedCstLabel, adaptLbls.pAdaptedCarLabel);
   }

   return (tdcResult);
}

// ----- tdcAddr2CarIdPiq() ----------------------------------------------------
// Abstract    : Position independent version of function tdcAddr2CarIdPiq
// Parameter(s): s. tdcAddr2CarIdPiq
// Return value: s. tdcAddr2CarIdPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcAddr2CarIdPiq (T_IPT_LABEL         carId,
                               UINT8*              pTopoCnt,    
                               T_IPT_IP_ADDR       ipAddr,
                               T_IPT_IP_ADDR       baseIpAddr)
{
   T_IPT_IP_ADDR     queryIpAddr = getAdaptedIpAddr (ipAddr, baseIpAddr);

   return (tdcAddr2CarId (carId, pTopoCnt, queryIpAddr));
}

// ----- tdcLabel2CstIdPiq() ---------------------------------------------------
// Abstract    : Position independent version of function tdcLabel2CstIdPiq
// Parameter(s): s. tdcLabel2CstIdPiq
// Return value: s. tdcLabel2CstIdPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcLabel2CstIdPiq (T_IPT_LABEL           cstId,
                                UINT8*                pTopoCnt,     
                                const T_IPT_LABEL     carLabel,
                                T_IPT_IP_ADDR         baseIpAddr)
{
   T_ADAPT_LABELS    adaptLbls;
   T_TDC_RESULT      tdcResult;

   tdcMemClear (&adaptLbls, (UINT32) sizeof (adaptLbls));

   adaptLbls.bCarLabel  = TRUE;
   adaptLbls.pTopoCnt   = pTopoCnt;
   adaptLbls.baseIpAddr = baseIpAddr;
   adaptLbls.pCarLabel  = carLabel;

   if ((tdcResult = getAdaptedLabels (&adaptLbls)) == TDC_OK)
   {
      tdcResult = tdcLabel2CstId (cstId, pTopoCnt, adaptLbls.pAdaptedCarLabel);
   }

   return (tdcResult);
}

// ----- tdcAddr2CstIdPiq() ----------------------------------------------------
// Abstract    : Position independent version of function tdcAddr2CstIdPiq
// Parameter(s): s. tdcAddr2CstIdPiq
// Return value: s. tdcAddr2CstIdPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcAddr2CstIdPiq (T_IPT_LABEL         cstId,     
                               UINT8*              pTopoCnt,     
                               T_IPT_IP_ADDR       ipAddr,
                               T_IPT_IP_ADDR       baseIpAddr)
{
   T_IPT_IP_ADDR     queryIpAddr = getAdaptedIpAddr (ipAddr, baseIpAddr);

   return (tdcAddr2CstId (cstId, pTopoCnt, queryIpAddr));
}

// ----- tdcCstNo2CstIdPiq() ----------------------------------------------------
// Abstract    : Position independent version of function tdcCstNo2CstIdPiq
// Parameter(s): s. tdcCstNo2CstIdPiq
// Return value: s. tdcCstNo2CstIdPiq
// Remarks     : tdcCstNo2CstIdPiq is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcCstNo2CstIdPiq (T_IPT_LABEL        cstId,     
                                UINT8*             pTopoCnt,     
                                UINT8              trnCstNo,
                                T_IPT_IP_ADDR      baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcCstNo2CstId (cstId, pTopoCnt, trnCstNo));
}

// ----- tdcLabel2TrnCstNoPiq() ------------------------------------------------
// Abstract    : Position independent version of function tdcLabel2TrnCstNoPiq
// Parameter(s): s. tdcLabel2TrnCstNoPiq
// Return value: s. tdcLabel2TrnCstNoPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcLabel2TrnCstNoPiq (UINT8*                pTrnCstNo,
                                   UINT8*                pTopoCnt, 
                                   const T_IPT_LABEL     carLabel,
                                   T_IPT_IP_ADDR         baseIpAddr)
{
   T_ADAPT_LABELS    adaptLbls;
   T_TDC_RESULT      tdcResult;

   tdcMemClear (&adaptLbls, (UINT32) sizeof (adaptLbls));

   adaptLbls.bCarLabel  = TRUE;
   adaptLbls.pTopoCnt   = pTopoCnt;
   adaptLbls.baseIpAddr = baseIpAddr;
   adaptLbls.pCarLabel  = carLabel;

   if ((tdcResult = getAdaptedLabels (&adaptLbls)) == TDC_OK)
   {
      tdcResult = tdcLabel2TrnCstNo (pTrnCstNo, pTopoCnt, adaptLbls.pAdaptedCarLabel);
   }

   return (tdcResult);
}

// ----- tdcAddr2TrnCstNoPiq() ------------------------------------------------
// Abstract    : Position independent version of function tdcAddr2TrnCstNoPiq
// Parameter(s): s. tdcAddr2TrnCstNoPiq
// Return value: s. tdcAddr2TrnCstNoPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcAddr2TrnCstNoPiq (UINT8*              pTrnCstNo,
                                  UINT8*              pTopoCnt, 
                                  T_IPT_IP_ADDR       ipAddr,
                                  T_IPT_IP_ADDR       baseIpAddr)
{
   T_IPT_IP_ADDR     queryIpAddr = getAdaptedIpAddr (ipAddr, baseIpAddr);

   return (tdcAddr2TrnCstNo (pTrnCstNo, pTopoCnt, queryIpAddr));
}

// ----- tdcGetTrnCstCntPiq() --------------------------------------------------
// Abstract    : Position independent version of function tdcGetTrnCstCntPiq
// Parameter(s): s. tdcGetTrnCstCntPiq
// Return value: s. tdcGetTrnCstCntPiq
// Remarks     : tdcGetTrnCstCntPiq is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetTrnCstCntPiq (UINT8*            pCstCnt,
                                 UINT8*            pTopoCnt,
                                 T_IPT_IP_ADDR     baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcGetTrnCstCnt (pCstCnt, pTopoCnt));
}

// ----- tdcAddr2TrnCstNoPiq() ------------------------------------------------
// Abstract    : Position independent version of function tdcAddr2TrnCstNoPiq
// Parameter(s): s. tdcAddr2TrnCstNoPiq
// Return value: s. tdcAddr2TrnCstNoPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetCstCarCntPiq (UINT8*               pCarCnt,
                                 UINT8*               pTopoCnt,  
                                 const T_IPT_LABEL    cstLabel,
                                 T_IPT_IP_ADDR        baseIpAddr)
{
   T_ADAPT_LABELS    adaptLbls;
   T_TDC_RESULT      tdcResult;

   tdcMemClear (&adaptLbls, (UINT32) sizeof (adaptLbls));

   adaptLbls.bCstLabel  = TRUE;
   adaptLbls.pTopoCnt   = pTopoCnt;
   adaptLbls.baseIpAddr = baseIpAddr;
   adaptLbls.pCstLabel  = cstLabel;

   if ((tdcResult = getAdaptedLabels (&adaptLbls)) == TDC_OK)
   {
      tdcResult = tdcGetCstCarCnt (pCarCnt, pTopoCnt, adaptLbls.pAdaptedCstLabel);
   }

   return (tdcResult);
}

// ----- tdcGetCarDevCntPiq() --------------------------------------------------
// Abstract    : Position independent version of function tdcGetCarDevCntPiq
// Parameter(s): s. tdcGetCarDevCntPiq
// Return value: s. tdcGetCarDevCntPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetCarDevCntPiq (UINT16*              pDevCnt,
                                 UINT8*               pTopoCnt,  
                                 const T_IPT_LABEL    cstLabel, 
                                 const T_IPT_LABEL    carLabel,
                                 T_IPT_IP_ADDR        baseIpAddr)
{
   T_ADAPT_LABELS    adaptLbls;
   T_TDC_RESULT      tdcResult;

   tdcMemClear (&adaptLbls, (UINT32) sizeof (adaptLbls));

   adaptLbls.bCarLabel  = TRUE;
   adaptLbls.bCstLabel  = TRUE;
   adaptLbls.pTopoCnt   = pTopoCnt;
   adaptLbls.baseIpAddr = baseIpAddr;
   adaptLbls.pCstLabel  = cstLabel;
   adaptLbls.pCarLabel  = carLabel;

   if ((tdcResult = getAdaptedLabels (&adaptLbls)) == TDC_OK)
   {
      tdcResult = tdcGetCarDevCnt (pDevCnt, pTopoCnt, adaptLbls.pAdaptedCstLabel, adaptLbls.pAdaptedCarLabel);
   }

   return (tdcResult);
}

// ----- tdcGetCarInfoPiq() ----------------------------------------------------
// Abstract    : Position independent version of function tdcGetCarInfoPiq
// Parameter(s): s. tdcGetCarInfoPiq
// Return value: s. tdcGetCarInfoPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetCarInfoPiq (T_TDC_CAR_DATA*        pCarData,
                               UINT8*                 pTopoCnt,  
                               UINT16                 maxDev,    
                               const T_IPT_LABEL      cstLabel,  
                               const T_IPT_LABEL      carLabel,
                               T_IPT_IP_ADDR          baseIpAddr)
{
   T_ADAPT_LABELS    adaptLbls;
   T_TDC_RESULT      tdcResult;

   tdcMemClear (&adaptLbls, (UINT32) sizeof (adaptLbls));

   adaptLbls.bCarLabel  = TRUE;
   adaptLbls.bCstLabel  = TRUE;
   adaptLbls.pTopoCnt   = pTopoCnt;
   adaptLbls.baseIpAddr = baseIpAddr;
   adaptLbls.pCstLabel  = cstLabel;
   adaptLbls.pCarLabel  = carLabel;

   if ((tdcResult = getAdaptedLabels (&adaptLbls)) == TDC_OK)
   {
      tdcResult = tdcGetCarInfo (pCarData, pTopoCnt,  maxDev, adaptLbls.pAdaptedCstLabel, adaptLbls.pAdaptedCarLabel);
   }

   return (tdcResult);
}

// ----- tdcGetUicStatePiq() ---------------------------------------------------
// Abstract    : Position independent version of function tdcGetUicStatePiq
// Parameter(s): s. tdcGetUicStatePiq
// Return value: s. tdcGetUicStatePiq
// Remarks     : tdcGetTrnCstCntPiq is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetUicStatePiq (UINT8*             pInaugState,
                                UINT8*             pTopoCnt,
                                T_IPT_IP_ADDR      baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcGetUicState (pInaugState, pTopoCnt));
}

// ----- tdcGetUicGlobalDataPiq() ----------------------------------------------
// Abstract    : Position independent version of function tdcGetUicGlobalDataPiq
// Parameter(s): s. tdcGetUicGlobalDataPiq
// Return value: s. tdcGetUicGlobalDataPiq
// Remarks     : tdcGetUicGlobalDataPiq is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetUicGlobalDataPiq (T_TDC_UIC_GLOB_DATA*   pGlobData,
                                     UINT8*                 pTopoCnt,
                                     T_IPT_IP_ADDR          baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcGetUicGlobalData (pGlobData, pTopoCnt));
}

// ----- tdcGetUicCarDataPiq() -------------------------------------------------
// Abstract    : Position independent version of function tdcGetUicCarDataPiq
// Parameter(s): s. tdcGetUicCarDataPiq
// Return value: s. tdcGetUicCarDataPiq
// Remarks     : tdcGetUicCarDataPiq is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcGetUicCarDataPiq (T_TDC_UIC_CAR_DATA*    pCarData,
                                  UINT8*                 pTopoCnt,  
                                  UINT8                  carSeqNo,
                                  T_IPT_IP_ADDR          baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcGetUicCarData (pCarData, pTopoCnt, carSeqNo));
}

// ----- tdcLabel2UicCarSeqNoPiq() -------------------------------------------------
// Abstract    : Position independent version of function tdcLabel2UicCarSeqNoPiq
// Parameter(s): s. tdcLabel2UicCarSeqNoPiq
// Return value: s. tdcLabel2UicCarSeqNoPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcLabel2UicCarSeqNoPiq (UINT8*                pCarSeqNo,
                                      UINT8*                pIptTopoCnt,
                                      UINT8*                pUicTopoCnt, 
                                      const T_IPT_LABEL     cstLabel,  
                                      const T_IPT_LABEL     carLabel,
                                      T_IPT_IP_ADDR         baseIpAddr)
{           
   T_ADAPT_LABELS    adaptLbls;
   T_TDC_RESULT      tdcResult;

   tdcMemClear (&adaptLbls, (UINT32) sizeof (adaptLbls));

   adaptLbls.bCarLabel  = TRUE;
   adaptLbls.bCstLabel  = TRUE;
   adaptLbls.pTopoCnt   = pIptTopoCnt;
   adaptLbls.baseIpAddr = baseIpAddr;
   adaptLbls.pCstLabel  = cstLabel;
   adaptLbls.pCarLabel  = carLabel;

   if ((tdcResult = getAdaptedLabels (&adaptLbls)) == TDC_OK)
   {
      tdcResult = tdcLabel2UicCarSeqNo (pCarSeqNo, pIptTopoCnt, pUicTopoCnt, 
                                        adaptLbls.pAdaptedCstLabel, adaptLbls.pAdaptedCarLabel);
   }

   return (tdcResult);
}

// ----- tdcAddr2UicCarSeqNoPiq() ----------------------------------------------
// Abstract    : Position independent version of function tdcAddr2UicCarSeqNoPiq
// Parameter(s): s. tdcAddr2UicCarSeqNoPiq
// Return value: s. tdcAddr2UicCarSeqNoPiq
// Remarks     : 
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcAddr2UicCarSeqNoPiq (UINT8*              pCarSeqNo, 
                                     UINT8*              pIptTopoCnt,
                                     UINT8*              pUicTopoCnt, 
                                     T_IPT_IP_ADDR       ipAddr,
                                     T_IPT_IP_ADDR       baseIpAddr)
{
   T_IPT_IP_ADDR     queryIpAddr = getAdaptedIpAddr (ipAddr, baseIpAddr);

   return (tdcAddr2UicCarSeqNo (pCarSeqNo, pIptTopoCnt, pUicTopoCnt, queryIpAddr));
}

// ----- tdcUicCarSeqNo2IdsPiq() -----------------------------------------------
// Abstract    : Position independent version of function tdcUicCarSeqNo2IdsPiq
// Parameter(s): s. tdcUicCarSeqNo2IdsPiq
// Return value: s. tdcUicCarSeqNo2IdsPiq
// Remarks     : tdcUicCarSeqNo2IdsPiq is already position-independent, Parameter
//               baseIpAddr is not used
// History     : 08-10-14     M.Ritz (PGR/3HEVT6)              created
// -----------------------------------------------------------------------------
EXP_DLL_DECL 
T_TDC_RESULT tdcUicCarSeqNo2IdsPiq (T_IPT_LABEL          cstId,  
                                    T_IPT_LABEL          carId,  
                                    UINT8*               pIptTopoCnt,
                                    UINT8*               pUicTopoCnt, 
                                    UINT8                carSeqNo,
                                    T_IPT_IP_ADDR        baseIpAddr)
{
   TDC_UNUSED(baseIpAddr)

   return (tdcUicCarSeqNo2Ids (cstId, carId, pIptTopoCnt, pUicTopoCnt, carSeqNo));
}

