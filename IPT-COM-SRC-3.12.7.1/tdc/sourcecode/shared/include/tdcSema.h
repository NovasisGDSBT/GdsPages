/*                                                                           */
/* $Id: tdcSema.h 11018 2008-10-15 15:13:56Z tgkamp $                      */
/*                                                                           */
/* DESCRIPTION    Front-End for Semaphore and Mutex management               */
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

#if !defined (_TDC_SEMA_H)
   #define _TDC_SEMA_H


/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

typedef void*                    T_TDC_MUTEX_ID;
typedef void*                    T_TDC_SEMA_ID;

/* ---------------------------------------------------------------------------- */

#define TDC_SEMA_WAIT_FOREVER      (-1)

#define TDC_SEMA_OK                ((T_TDC_SEMA_STATUS) 0)
#define TDC_SEMA_TIMEOUT           ((T_TDC_SEMA_STATUS) 1)
#define TDC_SEMA_EEXIST            ((T_TDC_SEMA_STATUS) 2)
#define TDC_SEMA_EIDRM             ((T_TDC_SEMA_STATUS) 3)
#define TDC_SEMA_ENOENT            ((T_TDC_SEMA_STATUS) 4)
#define TDC_SEMA_ENOMEM            ((T_TDC_SEMA_STATUS) 5)
#define TDC_SEMA_NOSPC             ((T_TDC_SEMA_STATUS) 6)
#define TDC_SEMA_EAGAIN            ((T_TDC_SEMA_STATUS) 7)
#define TDC_SEMA_EACCESS           ((T_TDC_SEMA_STATUS) 8)
#define TDC_SEMA_ERR               ((T_TDC_SEMA_STATUS) 1000)

typedef INT16                       T_TDC_SEMA_STATUS;


/* ---------------------------------------------------------------------------- */

#define TDC_MUTEX_OK               ((T_TDC_MUTEX_STATUS) 0)
#define TDC_MUTEX_ERR              ((T_TDC_MUTEX_STATUS) 1000)

typedef INT16                       T_TDC_MUTEX_STATUS;

/* ---------------------------------------------------------------------------- */

extern /*@null@*/ T_TDC_MUTEX_ID   tdcCreateMutex (            const char*          pModName,
                                                               const char*          pMutexName,
                                                    /*@out@*/  T_TDC_MUTEX_STATUS*  pStatus);
extern T_TDC_MUTEX_STATUS          tdcMutexLock   (            const char*          pModName,
                                         /*@null@*/ /*@in@*/   T_TDC_MUTEX_ID       mutexId);
extern T_TDC_MUTEX_STATUS          tdcMutexUnlock (            const char*          pModName,
                                         /*@null@*/ /*@in@*/   T_TDC_MUTEX_ID       mutexId);
extern T_TDC_MUTEX_STATUS          tdcMutexDelete (            const char*          pModName,
                                         /*@null@*/            T_TDC_MUTEX_ID*      pMutexId);
/*extern T_TDC_MUTEX_STATUS tdcMutexTrylock (T_TDC_MUTEX_ID       mutexId);*/


/* ---------------------------------------------------------------------------- */

extern /*@null@*/ T_TDC_SEMA_ID    tdcCreateSema  (            const char*          pModName,
                                                               const char*          pSemaName,
                                                               UINT16               initCnt,
                                                               T_TDC_BOOL           isShared,  /* shared between different processes ? */
                                                    /*@out@*/  T_TDC_SEMA_STATUS*   pStatus);
extern T_TDC_SEMA_STATUS           tdcWaitSema    (            const char*          pModName,
                                         /*@null@*/ /*@in@*/   T_TDC_SEMA_ID        semaId,
                                                               INT32                timeout);
extern T_TDC_SEMA_STATUS           tdcSignalSema  (            const char*          pModName,
                                         /*@null@*/ /*@in@*/   T_TDC_SEMA_ID        semaId);
extern T_TDC_SEMA_STATUS           tdcSemaDelete  (            const char*          pModName,
                                         /*@null@*/            T_TDC_SEMA_ID*       pSemaId);

/* ---------------------------------------------------------------------------- */

#ifdef __cplusplus                            /* to be compatible with C++ */
   }
#endif

/* ---------------------------------------------------------------------------- */

#endif


