/*                                                                     */
/* $Id: tdcInit.h 11512 2010-04-08 09:35:16Z gweiss $                */
/*                                                                     */
/* DESCRIPTION    Startup routines for TDC                             */
/*                                                                     */
/* AUTHOR         M.Ritz         PPC/EBTS                              */
/*                                                                     */
/* REMARKS                                                             */
/*                                                                     */
/* DEPENDENCIES                                                        */
/*                                                                     */
/*                                                                     */
/* All rights reserved. Reproduction, modification, use or disclosure  */
/* to third parties without express authority is forbidden.            */
/* Copyright Bombardier Transportation GmbH, Germany, 2002.            */
/*                                                                     */

/* ---------------------------------------------------------------------------- */

#if !defined (TDC_INIT_H)
   #define TDC_INIT_H

/* ---------------------------------------------------------------------------- */

#if defined (__cplusplus)
extern "C" {
#endif

/* ---------------------------------------------------------------------------- */

#define STARTUP_TASK_FORMAT         "TDC:     %-11sstarted successfully\n"

/* ---------------------------------------------------------------------------- */

#define APP_NAME                       "IPTDir-Client"
#define TASKNAME_MAIN                  "tTdcMain"
#define TASKNAME_CYCLIC                "tTdcCyc"
#define TASKNAME_IPCSERV               "tTdcIpcS"              /* IPC Service */
#define TASKNAME_IPCT                  "tTdcIpcT"              /* IPC Thread */
#define TASKNAME_MSGDATA               "tTdcMD"

/* ---------------------------------------------------------------------------- */
#define DEFAULT_PRIORITY_REF_VALUE        (UINT8) 75

#if defined(__INTEGRITY)
#define DEFAULT_PRIORITY                  (UINT8) (255 - DEFAULT_PRIORITY_REF_VALUE)
#else
#define DEFAULT_PRIORITY                  DEFAULT_PRIORITY_REF_VALUE
#endif
#define DEFAULT_STACK_SIZE                0x2000

/* ---------------------------------------------------------------------------- */

#define T_MAIN_INDEX                      0
#define T_CYCLIC_INDEX                    1
#define T_MSGDATA_INDEX                   2
#define T_IPCSERV_INDEX                   3
#define TDC_STD_THREAD_NO                 4

#define taskIdMain                        tdcThreadIdTab[T_MAIN_INDEX]
#define taskIdCyclic                      tdcThreadIdTab[T_CYCLIC_INDEX]
#define taskIdMsgData                     tdcThreadIdTab[T_MSGDATA_INDEX]
#define taskIdIpcServ                     tdcThreadIdTab[T_IPCSERV_INDEX]

/* ---------------------------------------------------------------------------- */

typedef struct
{
   /*@null@*/ T_TDC_SEMA_ID           semaId;
              const char*             pName;
} T_TDC_SEMA_ID_TAB;

typedef struct
{
   /*@null@*/ T_TDC_MUTEX_ID          mutexId;
              const char*             pName;
} T_TDC_MUTEX_ID_TAB;

#define STARTUP_SEMA_INDEX                0
#define TIMER_SEMA_INDEX                  1
#define DB_MUTEX_INDEX                    0
#define TASKID_MUTEX_INDEX                1

#define startupSemaId                     semaIdTab[STARTUP_SEMA_INDEX].semaId
#define timerSemaId                       semaIdTab[TIMER_SEMA_INDEX].semaId
#define dbMutexId                         mutexIdTab[DB_MUTEX_INDEX].mutexId
#define taskIdMutexId                     mutexIdTab[TASKID_MUTEX_INDEX].mutexId

extern T_TDC_SEMA_ID_TAB      semaIdTab[];         
extern T_TDC_MUTEX_ID_TAB     mutexIdTab[];

extern T_TDC_BOOL             bTerminate;
extern T_TDC_BOOL             bBaseInitDone;


extern void        tdcStartupFw             (/*@in@*/ void*       pArgV);

extern void        tdcTCyclicInit           (/*@in@*/ void*       pArgV);
extern void        tdcTIpcServInit          (/*@in@*/ void*       pArgV);

extern T_TDC_BOOL  tdcDeleteMutexes         (void);
extern T_TDC_BOOL  tdcDeleteSemaphores      (void);

/* ---------------------------------------------------------------------------- */

#if defined (__cplusplus)
}
#endif

#endif
