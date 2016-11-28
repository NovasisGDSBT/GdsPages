//
// $Id: tdcOsSema.c 11744 2010-11-29 15:47:20Z bloehr $
//
// DESCRIPTION    Front-End for Semaphore and Mutex management
//
// AUTHOR         M.Ritz         PPC/EBT
//
// REMARKS        The switch LINUX has to be set
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


#if !defined (LINUX)
   #error "This is the Linux Version of tdcOsSema, i.e. LINUX has to be specified"
#endif


// ----------------------------------------------------------------------------

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "tdc.h"

// ----------------------------------------------------------------------------

#define MUTEX_MAGIC_NO                 0x1234FEDC
#define SEMA_MAGIC_NO                  0x1234FEDB

#define SEMA_NAME_LEN                  16
#define MUTEX_NAME_LEN                 16

typedef struct
{
   UINT32            magicNo;
   pthread_mutex_t   mutexId;
   char              name[MUTEX_NAME_LEN];
} T_MUTEX;

typedef struct
{
   UINT32            magicNo;
   sem_t             semaId;
   char              name[SEMA_NAME_LEN];
} T_SEMAPHORE;

// ----------------------------------------------------------------------------

/*@null@*/
T_TDC_MUTEX_ID tdcCreateMutex (const char*            pModName,
                               const char*            pMutexName,
                               T_TDC_MUTEX_STATUS*    pStatus)
{
   T_MUTEX*          pMutex = tdcAllocMemChk (pModName, (UINT32) sizeof (T_MUTEX));

   *pStatus = TDC_MUTEX_ERR;

   if (pMutex != NULL)
   {
      if (pthread_mutex_init (&pMutex->mutexId, NULL) == 0)
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

         if (pthread_mutex_lock (&pMutex->mutexId) == 0)
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

         if (pthread_mutex_unlock (&pMutex->mutexId) == 0)
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

            if (pthread_mutex_destroy (&pMutex->mutexId) == 0)
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

   *pStatus = TDC_SEMA_ERR;

   if (bShared)
   {
      DEBUG_WARN (pModName, "No support for shared semaphores");
   }
   else
   {
      if ((pSema = tdcAllocMemChk (pModName, (UINT32) sizeof (T_SEMAPHORE))) != NULL)
      {
         if (sem_init (&pSema->semaId, (int) bShared, (unsigned int) initCnt) != -1)
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
               if (sem_trywait (&pSema->semaId) == -1)
               {
                  semaStatus = (errno == EAGAIN) ? (TDC_SEMA_EAGAIN) : (TDC_SEMA_ERR);
               }
               break;
            }
            case TDC_SEMA_WAIT_FOREVER:
            {
               for (; sem_wait (&pSema->semaId) == -1;)
               {
                  if (errno != EINTR)
                  {
                     DEBUG_WARN1 (MOD_MAIN, "Error waiting on semaphore (%d)", errno);
                     semaStatus = TDC_SEMA_ERR;
                     break;
                  }
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

         if (sem_post (&pSema->semaId) != -1)
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

            if (sem_destroy (&pSema->semaId) != -1)
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






