//
// $Id: tdcOsSema.c 11619 2010-07-22 10:04:00Z bloehr $
//
// DESCRIPTION    Front-End for Semaphore and Mutex management
//
// AUTHOR         M.Ritz         PPC/EBT
//
// REMARKS        The switch INTEGRITY has to be set
//
// DEPENDENCIES
//
// All rights reserved. Reproduction, modification, use or disclosure
// to third parties without express authority is forbidden.
// Copyright Bombardier Transportation GmbH, Germany, 2002.
//


// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------


#if !defined (__INTEGRITY)
   #error "This is the INTEGRITY Version of tdcOsSema, "
          "i.e. INTEGRITY has to be specified"
#endif


// ----------------------------------------------------------------------------

#include <INTEGRITY.h>
#include <errno.h>

#include "tdc.h"

// ----------------------------------------------------------------------------

#define MUTEX_MAGIC_NO                 0x1234FEDC
#define SEMA_MAGIC_NO                  0x1234FEDB

#define SEMA_NAME_LEN                  16
#define MUTEX_NAME_LEN                 16

typedef struct
{
   UINT32            magicNo;
   LocalMutex        mutexId;
   char              name[MUTEX_NAME_LEN];
} T_MUTEX;

typedef struct
{
   UINT32            magicNo;
   Semaphore         semaId;
   char              name[SEMA_NAME_LEN];
} T_SEMAPHORE;

// ----------------------------------------------------------------------------

/*@null@*/
T_TDC_MUTEX_ID tdcCreateMutex (const char*            pModName,
                               const char*            pMutexName,
                               T_TDC_MUTEX_STATUS*    pStatus)
{
   T_MUTEX*          pMutex = tdcAllocMemChk (pModName, (UINT32) sizeof (T_MUTEX));

   if (pStatus == NULL)
   {
      DEBUG_WARN (pModName, "tdcCreateMutex: Wrong parameter status (NULL)");
      return NULL;
   }

   *pStatus = TDC_MUTEX_ERR;

   if (pMutexName == NULL)
   {
      DEBUG_WARN (pModName, "tdcCreateMutex: Wrong parameter mutex name (NULL)");
      return NULL; 
   }

   if (pMutex != NULL)
   {
      if (CreateLocalMutex(&pMutex->mutexId) == Success)
      {
         *pStatus        = TDC_MUTEX_OK;
         pMutex->magicNo = MUTEX_MAGIC_NO;
         (void) tdcStrNCpy (pMutex->name, pMutexName, MUTEX_NAME_LEN);
      }
      else
      {
         DEBUG_WARN (pModName, "Can not initialize Mutex");
         tdcFreeMem (pMutex);
         pMutex = NULL;
      }
   }

   if ((*pStatus) != TDC_MUTEX_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Failed to create mutex (%s)", pMutexName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return ((T_TDC_MUTEX_ID) pMutex);
}

// ----------------------------------------------------------------------------

T_TDC_MUTEX_STATUS tdcMutexLock (const char*       pModName,
                                 T_TDC_MUTEX_ID    mutexId)
{
   T_TDC_MUTEX_STATUS      mutexStatus = TDC_MUTEX_ERR;
   const char*             pMutexName  = "Unknown";

   if (mutexId != NULL)
   {
      T_MUTEX*       pMutex = (T_MUTEX *) mutexId;

      if (pMutex->magicNo == MUTEX_MAGIC_NO)
      {
         pMutexName = pMutex->name;

         if (WaitForLocalMutex(pMutex->mutexId) == Success)
         {
            mutexStatus = TDC_MUTEX_OK;
         }
      }
   }

   if (mutexStatus != TDC_MUTEX_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "[sizeof (text), Unable to lock mutex (%s)", pMutexName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (mutexStatus);
}

// ----------------------------------------------------------------------------

//T_TDC_MUTEX_STATUS tdcMutexTrylock (T_TDC_MUTEX_ID  mutexId)
//{
//   return ((pthread_mutex_trylock (mutexId) == 0) ? (TDC_MUTEX_OK) : (TDC_MUTEX_ERR));
//}

// ----------------------------------------------------------------------------

T_TDC_MUTEX_STATUS tdcMutexUnlock (const char*          pModName,
                                   T_TDC_MUTEX_ID       mutexId)
{
   T_TDC_MUTEX_STATUS      mutexStatus = TDC_MUTEX_ERR;
   const char*             pMutexName  = "Unknown";

   if (mutexId != NULL)
   {
      T_MUTEX*       pMutex = (T_MUTEX *) mutexId;

      if (pMutex->magicNo == MUTEX_MAGIC_NO)
      {
         pMutexName = pMutex->name;

         if (ReleaseLocalMutex(pMutex->mutexId) == Success)
         {
            mutexStatus = TDC_MUTEX_OK;
         }
      }
   }

   if (mutexStatus != TDC_MUTEX_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Unable to unlock mutex (%s)", pMutexName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (mutexStatus);
}

// ----------------------------------------------------------------------------

T_TDC_MUTEX_STATUS tdcMutexDelete (const char*        pModName,
                                   T_TDC_MUTEX_ID*    pMutexId)
{
   T_TDC_MUTEX_STATUS      mutexStatus = TDC_MUTEX_ERR;
   const char*             pMutexName  = "Unknown";

   if (pMutexId != NULL)
   {
      T_MUTEX*       pMutex = (T_MUTEX *) *pMutexId;

      if (pMutex != NULL)
      {
         if (pMutex->magicNo == MUTEX_MAGIC_NO)
         {
            pMutexName = pMutex->name;

            if (TryToObtainLocalMutex(pMutex->mutexId) == Success)
            {
               if (CloseLocalMutex(pMutex->mutexId) == Success)
            {
               mutexStatus = TDC_MUTEX_OK;
            }
            }

            tdcFreeMem (pMutex);
         }
      }

      *pMutexId = NULL;
   }

   if (mutexStatus != TDC_MUTEX_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Unable to delete mutex (%s)", pMutexName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (mutexStatus);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

T_TDC_SEMA_ID tdcCreateSema (const char*           pModName,
                             const char*           pSemaName,
                             UINT16                initCnt,
                             T_TDC_BOOL            bShared,  // shared between different processes ?
                             T_TDC_SEMA_STATUS*    pStatus)
{
   T_SEMAPHORE*      pSema = NULL;

   if (pStatus == NULL)
   {
      DEBUG_WARN (pModName, "tdcCreateSema: Wrong parameter status (NULL)");
      return NULL;
   }

   *pStatus = TDC_SEMA_ERR;

   if (pSemaName == NULL)
   {
      DEBUG_WARN (pModName, "tdcCreateSema: Wrong parameter sema name (NULL)");
      return NULL;
   }

   if (bShared)
   {
      DEBUG_WARN (pModName, "No support for shared semaphores");
   }
   else
   {
      if ((pSema = tdcAllocMemChk (pModName, (UINT32) sizeof (T_SEMAPHORE))) != NULL)
      {
         if (CreateSemaphore((Value)initCnt, &pSema->semaId) == Success)
         {
            *pStatus       = TDC_SEMA_OK;
            pSema->magicNo = SEMA_MAGIC_NO;
            (void) tdcStrNCpy(pSema->name, pSemaName, SEMA_NAME_LEN);
         }
         else
         {
            tdcFreeMem (pSema);
            pSema  = NULL;
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

      (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Unable to create semaphore (%s)", pSemaName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return ((T_TDC_SEMA_ID *) pSema);
}

// ----------------------------------------------------------------------------

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
         pSemaName  = pSema->name;
         semaStatus = TDC_SEMA_OK;

         switch (timeout)
         {
            case 0:
            {
               Error ret;

               if ((ret = TryToObtainSemaphore(pSema->semaId)) != Success)
               {
                  semaStatus = (ret == ResourceNotAvailable) ? (TDC_SEMA_EAGAIN) : (TDC_SEMA_ERR);
               }
               break;
            }
            case TDC_SEMA_WAIT_FOREVER:
            {
               Error ret;

               if ((ret = WaitForSemaphore(pSema->semaId)) != Success)
               {
                     DEBUG_WARN1 (MOD_MAIN, "Error (%d) waiting on semaphore", ret);
                     semaStatus = TDC_SEMA_ERR;
               }
               break;
            }
            default:
            {
               DEBUG_WARN (MOD_MAIN, "Wait for semaphore with timeout not supported");
               break;
            }
         }
      }
   }

   if (semaStatus != TDC_SEMA_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Unable to wait on semaphore (%s)", pSemaName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (semaStatus);
}

// ----------------------------------------------------------------------------

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

         if (ReleaseSemaphore(pSema->semaId) == Success)
         {
            semaStatus = TDC_SEMA_OK;
         }
      }
   }

   if (semaStatus != TDC_SEMA_OK)
   {
      char     text[60];

      (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Unable to signal semaphore (%s)", pSemaName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (semaStatus);
}

// ----------------------------------------------------------------------------

T_TDC_SEMA_STATUS tdcSemaDelete  (const char*         pModName,
                                  T_TDC_SEMA_ID*      pSemaId)
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

            if (CloseSemaphore(pSema->semaId) == Success)
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

      (void) tdcSNPrintf (text, (unsigned int) sizeof (text), "Unable to delete semaphore (%s)", pSemaName);
      text[sizeof (text) - 1] = '\0';
      DEBUG_ERROR (pModName, text);
   }

   return (semaStatus);
}

// ----------------------------------------------------------------------------






