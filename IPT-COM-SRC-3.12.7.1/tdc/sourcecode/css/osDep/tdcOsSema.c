/*
 *  $Id: tdcOsSema.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 *  DESCRIPTION    Front-End for Semaphore and Mutex management
 *
 *  AUTHOR         Manfred Ritz
 *
 *  REMARKS        The switch VXWORKS has to be set
 *
 *  DEPENDENCIES
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

/* ---------------------------------------------------------------------------- */

#if !defined (VXWORKS)
   #error "This is the CSS Version of TDC_OS_SEMA, i.e. VXWORKS has to be specified"
#endif

/* ---------------------------------------------------------------------------- */

#include <vxWorks.h>
#include <stdio.h>
#include <semLib.h>

#define TDC_BASETYPES_H    /* Use original VxWorks Header File definitions */
#include "tdc.h"

/* ---------------------------------------------------------------------------- */

#define MUTEX_MAGIC_NO                 0x1234FEDC
#define SEMA_MAGIC_NO                  0x1234FEDB

#define SEMA_NAME_LEN                  16
typedef struct
{
   UINT32      magicNo;
   SEM_ID      semaId;
   char        name[SEMA_NAME_LEN];
} T_SEMAPHORE;

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_MUTEX_ID tdcCreateMutex (const char*            pModName,
                               const char*            pMutexName,
                               T_TDC_MUTEX_STATUS*    pStatus)
{
   T_SEMAPHORE*      pMutex = (T_SEMAPHORE*)tdcAllocMemChk (pModName, (UINT32) sizeof (T_SEMAPHORE));

   *pStatus = TDC_MUTEX_ERR;

   if (pMutex != NULL)
   {
      if ((pMutex->semaId = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE)) != NULL)
      {
         *pStatus        = TDC_MUTEX_OK;
         pMutex->magicNo = MUTEX_MAGIC_NO;
         (void) tdcStrNCpy (pMutex->name, pMutexName, SEMA_NAME_LEN);
      }
      else
      {
         tdcFreeMem (pMutex);
         pMutex = NULL;
      }
   }

   if ((*pStatus) != TDC_MUTEX_OK)
   {
      char     text[60];

      (void) tdcSNPrintf  (text, (UINT32) sizeof (text), "Failed to create mutex (%s)", pMutexName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return ((T_TDC_MUTEX_ID) pMutex);
}

/* ---------------------------------------------------------------------------- */

T_TDC_MUTEX_STATUS tdcMutexLock (const char*       pModName,
                                 T_TDC_MUTEX_ID    mutexId)
{
   T_TDC_MUTEX_STATUS      mutexStatus = TDC_MUTEX_ERR;
   const char*             pMutexName  = "Unknown";

   if (mutexId != NULL)
   {
      T_SEMAPHORE*      pMutex = (T_SEMAPHORE *) mutexId;

      if (pMutex->magicNo == MUTEX_MAGIC_NO)
      {
         pMutexName = pMutex->name;

         if (semTake (pMutex->semaId, WAIT_FOREVER) == OK)
         {
            mutexStatus = TDC_MUTEX_OK;
         }
      }
   }

   if (mutexStatus != TDC_MUTEX_OK)
   {
      char     text[60];

      (void) tdcSNPrintf  (text, (UINT32) sizeof (text), "Unable to lock mutex %s", pMutexName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (mutexStatus);
}

/* ---------------------------------------------------------------------------- */

/* T_TDC_MUTEX_STATUS tdcMutexTrylock (T_TDC_MUTEX_ID  mutex_id)           */
/* {                                                                                */
/*    T_TDC_SEMA_STATUS       sema_status;                                      */
/*    T_TDC_SEMA_ID           sema_id = (T_TDC_MUTEX_ID) mutex_id;           */
/*                                                                                  */
/*    sema_status = tdcWaitSema (sema_id, 0);       - Don't Wait                   */
/*                                                                                  */
/*    return ((sema_status == TDC_SEMA_OK) ? (TDC_MUTEX_OK) : (TDC_MUTEX_ERR));  */
/* }                                                                                */

/* ---------------------------------------------------------------------------- */

T_TDC_MUTEX_STATUS tdcMutexUnlock (const char*          pModName,
                                   T_TDC_MUTEX_ID       mutexId)
{
   T_TDC_MUTEX_STATUS      mutexStatus = TDC_MUTEX_ERR;
   const char*             pMutexName  = "Unknown";

   if (mutexId != NULL)
   {
      T_SEMAPHORE*      pMutex = (T_SEMAPHORE *) mutexId;

      if (pMutex->magicNo == MUTEX_MAGIC_NO)
      {
         pMutexName = pMutex->name;

         if (semGive (pMutex->semaId) == OK)
         {
            mutexStatus = TDC_MUTEX_OK;
         }
      }
   }

   if (mutexStatus != TDC_MUTEX_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (UINT32) sizeof (text), "Unable to unlock mutex (%s)", pMutexName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (mutexStatus);
}

/* ---------------------------------------------------------------------------- */

T_TDC_MUTEX_STATUS tdcMutexDelete (const char*        pModName,
                                   T_TDC_MUTEX_ID*    pMutexId)
{
   T_TDC_MUTEX_STATUS      mutexStatus = TDC_MUTEX_ERR;
   const char*             pMutexName  = "Unknown";

   if (pMutexId != NULL)
   {
      T_SEMAPHORE*      pMutex = (T_SEMAPHORE *) *pMutexId;

      if (pMutex != NULL)
      {
         if (pMutex->magicNo == MUTEX_MAGIC_NO)
         {
            pMutexName = pMutex->name;

            if (semDelete (pMutex->semaId) == OK)
            {
               mutexStatus = TDC_MUTEX_OK;
            }

            tdcFreeMem (pMutex);
         }
      }

      *pMutexId = NULL;
   }

   if (mutexStatus != TDC_MUTEX_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (UINT32) sizeof (text), "Unable to delete mutex (%s)", pMutexName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (mutexStatus);
}

/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */
/* ---------------------------------------------------------------------------- */

T_TDC_SEMA_ID tdcCreateSema (const char*          pModName,
                             const char*          pSemaName,
                             UINT16               initCnt,
                             T_TDC_BOOL           bShared,     /* shared between different processes ? */
                             T_TDC_SEMA_STATUS*   pStatus)
{
   T_SEMAPHORE*      pSema = NULL;

   *pStatus = TDC_SEMA_ERR;

   if (bShared)
   {
      DEBUG_WARN (pModName, "No support for shared semaphores");
   }
   else
   {
      if ((pSema = (T_SEMAPHORE*)tdcAllocMemChk (pModName, (UINT32) sizeof (T_SEMAPHORE))) != NULL)
      {
         if ((pSema->semaId = semCCreate (SEM_Q_FIFO, (int) initCnt)) != NULL)
         {
            *pStatus       = TDC_SEMA_OK;
            pSema->magicNo = SEMA_MAGIC_NO;
            (void) tdcStrNCpy(pSema->name, pSemaName, SEMA_NAME_LEN);
         }
         else
         {
            tdcFreeMem (pSema);
            pSema = NULL;
         }
      }
      else
      {
         DEBUG_ERROR1 (pModName, "Failed to alloc memory (%d)", sizeof (T_SEMAPHORE));
      }
   }

   if ((*pStatus) != TDC_SEMA_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (UINT32) sizeof (text), "Unable to create semaphore (%s)", pSemaName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return ((T_TDC_SEMA_ID) pSema);
}
         
/* ---------------------------------------------------------------------------- */

T_TDC_SEMA_STATUS tdcWaitSema (const char*      pModName,
                               T_TDC_SEMA_ID    semaId,
                               INT32            timeout)
{
   T_TDC_SEMA_STATUS    semaStatus = TDC_SEMA_ERR;
   const char*          pSemaName  = "Unknown";

   if (semaId != NULL)
   {
      T_SEMAPHORE*      pSema = (T_SEMAPHORE *) semaId;

      if (pSema->magicNo == SEMA_MAGIC_NO)
      {
         int         myTimeout;
         
         pSemaName  = pSema->name;
         semaStatus = TDC_SEMA_TIMEOUT;

         switch (timeout)
         {
            case 0:                          {myTimeout = NO_WAIT;                     break;}
            case TDC_SEMA_WAIT_FOREVER:      {myTimeout = WAIT_FOREVER;                break;}
            default:                         {myTimeout = msec2OsTicks (timeout);      break;}
         }
         
         if (semTake (pSema->semaId, myTimeout) == OK)
         {
            semaStatus = TDC_SEMA_OK;
         }
      }
   }

   if (semaStatus != TDC_SEMA_OK)
   {
      char     text[60];

      (void) tdcSNPrintf  (text, (UINT32) sizeof (text), "Unable to wait on semaphore (%s)", pSemaName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (semaStatus);
}

/* ---------------------------------------------------------------------------- */

T_TDC_SEMA_STATUS tdcSignalSema (const char*       pModName,
                                 T_TDC_SEMA_ID     semaId)
{
   T_TDC_SEMA_STATUS    semaStatus = TDC_SEMA_ERR;
   const char*          pSemaName  = "Unknown";

   if (semaId != NULL)
   {
      T_SEMAPHORE*      pSema = (T_SEMAPHORE *) semaId;

      if (pSema->magicNo == SEMA_MAGIC_NO)
      {
         pSemaName = pSema->name;

         if (semGive (pSema->semaId) == OK)
         {
            semaStatus = TDC_SEMA_OK;
         }
      }
   }

   if (semaStatus != TDC_SEMA_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (UINT32) sizeof (text), "Unable to signal semaphore (%s)", pSemaName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (semaStatus);
}

/* ---------------------------------------------------------------------------- */

T_TDC_SEMA_STATUS tdcSemaDelete  (const char*      pModName,
                                  T_TDC_SEMA_ID*   pSemaId)
{
   T_TDC_SEMA_STATUS    semaStatus = TDC_SEMA_ERR;
   const char*          pSemaName  = "Unknown";

   if (pSemaId != NULL)
   {
      T_SEMAPHORE*      pSema = (T_SEMAPHORE *) *pSemaId;

      if (pSema != NULL)
      {
         if (pSema->magicNo == SEMA_MAGIC_NO)
         {
            pSemaName = pSema->name;

            if (semDelete (pSema->semaId) == OK)
            {
               semaStatus = TDC_SEMA_OK;
            }

            tdcFreeMem (pSema);
         }
      }

      *pSemaId = NULL;
   }

   if (semaStatus != TDC_SEMA_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (UINT32) sizeof (text), "Unable to delete semaphore (%s)", pSemaName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (semaStatus);
}

