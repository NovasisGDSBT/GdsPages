/*
 * $Id: tdcInit.c 11861 2012-04-19 11:30:50Z gweiss $
 *
 * DESCRIPTION    Startup routines for TDC
 *
 * AUTHOR         M.Ritz         PPC/EBT
 *
 * REMARKS
 *
 * DEPENDENCIES   Either the switch VXWORKS, LINUX or WIN32 has to be set
 *
 * MODIFICATIONS:
 *
 * CR-3477 (Gerhard Weiss, 2012-04-18)
 *          Adding corrections for TÜV-Assessment
 *
 * CR-685 (Gerhard Weiss, 2010-11-25)
 *          Corrected Semaphore deletion
 *
 * CR-382 (Bernd Loehr, 2010-08-24)
 * 			Initialization for standalone mode (prepStandalone)
 *
 * All rights reserved. Reproduction, modification, use or disclosure
 * to third parties without express authority is forbidden.
 * Copyright Bombardier Transportation GmbH, Germany, 2002-2010.
 */

/* ---------------------------------------------------------------------------- */

#include "tdc.h"
#include "tdcThread.h"
#include "tdcInit.h"
#include "tdcIptCom.h"
#include "tdcConfig.h"
#include "tdcDB.h"
#include "tdcXML2bin.h"

/* ----------------------- function prototypes --------------------------------- */

static void  createCommonMutexes             (/*@in@*/ void*      pArgV);
static void  createCommonSemaphores          (/*@in@*/ void*      pArgV);
static void  determineOwnDev                 (/*@in@*/ void*      pArgV);
static void  prepStandalone                  (/*@in@*/ void*      pArgV);
static void  finishBaseInitialization        (/*@in@*/ void*      pArgV);

static void  createCommonITimer              (/*@in@*/ void*      pArgV);
static void  initializeIptCom                (/*@in@*/ void*      pArgV);

static void  signalAlarm                     (int                 sigNo);
static void  startupThreads                  (void);
static /*@null@*/ void* cyclicThreadStart    (/*@in@*/ void*      pArgV);
static /*@null@*/ void* ipcServThreadStart   (/*@in@*/ void*      pArgV);
static /*@null@*/ void* msgDataThreadStart   (/*@in@*/ void*      pArgV);

/* ----------------------- type definitions ------------------------------------ */

typedef struct
{
    const char        *pInitText;
    void              (*pInitFct) (/*@in@*/ void*       pArgV);
} T_INIT_FW_TAB;

/* ----------------------- constant definitions -------------------------------- */

static const T_INIT_FW_TAB   initFwTab[] =
      {
         {"reading configuration",                          readConfiguration},
         {"creating mutexes",                               createCommonMutexes},
         {"creating semaphores",                            createCommonSemaphores},
         /*{"creating queues",                                createCommonQueues},*/

         {"initializing database",                          dbInitializeDB},
         {"determining own Device",                         determineOwnDev},
         {"checking standalone mode",                       prepStandalone},
         {"creating itimer",                                createCommonITimer},
         {"finishing base initialization",                  finishBaseInitialization},
         {"initializing IPTCom Interface",                  initializeIptCom},
         {"initializing cyclic task",                       tdcTCyclicInit},
         {"initializing InterProcessComm",                  tdcTIpcServInit},
         /*{"adding monitor hooks",                            add_monitor_hooks}, */
      };


/* ---------------------------------------------------------------------------- */

static const T_TDC_BOOL          always = TRUE;

static const T_THREAD_FRAME     threadIdTab[] =
{
   {
      &always,
      APP_NAME,
      TASKNAME_CYCLIC,
      TASK_TYPE,
      DEFAULT_PRIORITY,
      DEFAULT_STACK_SIZE,
      0,
      cyclicThreadStart,
      T_CYCLIC_INDEX
   },
   {
      &always,
      APP_NAME,
      TASKNAME_MSGDATA,
      TASK_TYPE,
      DEFAULT_PRIORITY,
      DEFAULT_STACK_SIZE,
      0,
      msgDataThreadStart,
      T_MSGDATA_INDEX
   },
   {
      &always,
      APP_NAME,
      TASKNAME_IPCSERV,
      TASK_TYPE,
      DEFAULT_PRIORITY,
      DEFAULT_STACK_SIZE,
      0,
      ipcServThreadStart,
      T_IPCSERV_INDEX
   }
};


/* ------------------------- variable definitions -----------------------------   */
                                                                                         
T_TDC_SEMA_ID_TAB       semaIdTab[2]  = {{NULL, "startupSema"},   {NULL, "timerSema"}};
T_TDC_MUTEX_ID_TAB      mutexIdTab[2] = {{NULL, "databaseMutex"}, {NULL, "taskIdMutex"}};  
T_TDC_BOOL              bTerminate    = FALSE;
T_TDC_BOOL              bBaseInitDone = FALSE;
                                                                                         
T_THREAD_ID             tdcThreadIdTab[TDC_STD_THREAD_NO] = {NULL, NULL, NULL, NULL};    

/* ---------------------------------------------------------------------------- */

static void createCommonSemaphores (void*       pArgV)
{
   int                  i;
   UINT16               initCnt    = 0;
   T_TDC_BOOL           bShared    = FALSE;

   TDC_UNUSED (pArgV)

   for (i = 0; i < TAB_SIZE (semaIdTab); i++)
   {
      T_TDC_SEMA_STATUS    semaStatus;

      semaIdTab[i].semaId = tdcCreateSema (MOD_MAIN,
                                           semaIdTab[i].pName,
                                           initCnt, bShared, &semaStatus);
   }
}

/* ---------------------------------------------------------------------------- */

static void createCommonMutexes (void*       pArgV)
{
   int                     i;

   TDC_UNUSED (pArgV)

   for (i = 0; i < TAB_SIZE (mutexIdTab); i++)
   {
      T_TDC_MUTEX_STATUS      mutexStatus;

      mutexIdTab[i].mutexId = tdcCreateMutex (MOD_MAIN, mutexIdTab[i].pName, &mutexStatus);
   }

}

/* ---------------------------------------------------------------------------- */

static void finishBaseInitialization (void*       pArgV)
{
   TDC_UNUSED (pArgV)

   bBaseInitDone = TRUE;
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcDeleteMutexes (void)
{
   T_TDC_BOOL        bOK = TRUE;
   int               i;

   for (i = 0; i < TAB_SIZE (mutexIdTab); i++)
   {
      if (tdcMutexDelete (MOD_MAIN, &mutexIdTab[i].mutexId) != TDC_MUTEX_OK)
      {
         bOK = FALSE;
      }
   }

   return (bOK);
}

/* ---------------------------------------------------------------------------- */

T_TDC_BOOL tdcDeleteSemaphores (void)
{
   T_TDC_BOOL        bOK = TRUE;
   int               i;

   for (i = 1; i < TAB_SIZE (semaIdTab); i++)
   {
      if (tdcSignalSema (MOD_MAIN, semaIdTab[i].semaId) != TDC_SEMA_OK)
      {
         bOK = FALSE;
      }
      
      tdcSleep (0);    // ensure task-switch to prevent still blocked semas
       
      if (tdcSemaDelete (MOD_MAIN, &semaIdTab[i].semaId) != TDC_SEMA_OK)
      {
         bOK = FALSE;
      }
   }

   // The last one has to be the 'STARTUP_SEMA' and must be deleted by the tdcMain itself

   if (tdcSignalSema (MOD_MAIN, semaIdTab[0].semaId) != TDC_SEMA_OK)
   {
      bOK = FALSE;
   }
   
   tdcSleep (0);    // ensure task-switch to prevent still blocked semas

   return (bOK);
}

/* ---------------------------------------------------------------------------- */

static void determineOwnDev (void*       pArgV)
{
   static UINT32        cnt = 0;
   static T_TDC_BOOL    bOk = FALSE;

   /* First of all read own IP-Address. The own IP-Address is assigned by DHCP and  */
   /* must be something like 10.0.x.y - with x < 16  (and xy not all zero or all 1) */
   /* There is no need to do anything, as long as DHCP process is not finished!     */
   /* This function is blocking until a valid device is found                       */

   TDC_UNUSED (pArgV)
    
   /* gweiss 2010-09-11 move this initialziation to here since sockets are used */
   /* during DetermineOwnDevice */
   tdcInitTcpIpWinsock (MOD_MAIN);
    
   for (cnt = 0; !bOk; cnt++)
   {
      bOk = tdcDetermineOwnDevice (MOD_MD);

      if (!bOk)
      {
         tdcSleep (250);          /* wait 1/4 second */

         if ((cnt % 240) == 0)    /* Debug Info from time to time */
         {
            DEBUG_WARN1 (MOD_MD, "Unable to determine own device - No DHCP (%d seconds)", cnt / 4);
         }
      }
   }
}

/* -- CR-382 ---------------------------------------------------------------- */

static void prepStandalone(void*       pArgV)
{
   T_TDC_BOOL	bOk = FALSE;

   TDC_UNUSED (pArgV)

   /* 	CR-382:
   		If the standalone flag is set, read the consist configuration file and
        generate the packets for IPTDir'less times.
   	*/
   if (tdcGetStandaloneSupport() == TRUE)
   {
       /* Check for existence of cstSta file:	*/
       if (tdcFSize ("tdcInit", tdcGetCstStaFilename()) == 0)
       {
       	    DEBUG_WARN (MOD_MAIN, "...standalone support disabled (no cstSta found)");
            tdcSetStandaloneSupport("FALSE");
       }
	   else
       {
           /* read cstSta file and update packet binaries	*/
           bOk = tdcXML2Packets(tdcGetLocalIpAddr(),
                                tdcGetCstStaFilename(),
                                tdcGetComId100Filename(),
                                tdcGetComId101Filename(),
                                tdcGetComId102Filename());
           if (bOk)
           {
               DEBUG_INFO (MOD_MAIN, "...standalone support enabled ");
           }
           else
           {
               DEBUG_WARN (MOD_MAIN, "...standalone support failed ");
           }
       }
   }
   else
   {
	   DEBUG_INFO (MOD_MAIN, "...standalone support disabled ");
   }
}

/* ---------------------------------------------------------------------------- */
 
static void signalAlarm (int sigNo)
{
   if (timerSemaId != NULL)
   {
      if (tdcSignalSema (MOD_ITIMER, timerSemaId) == TDC_SEMA_OK)
      {
         /*DEBUG_INFO (MOD_MAIN, "Signaled Semaphore"); */
      }
   }
   else
   {
      sigNo = sigNo;
   }
}

/* ---------------------------------------------------------------------------- */

static void createCommonITimer (void*       pArgV)
{
   TDC_UNUSED (pArgV)

   if (tdcInitITimer (signalAlarm, cycleTimeIPTDirPD) == 0)
   {
      DEBUG_ERROR (MOD_MAIN, "Could Not successfully install Interval-Timer");
   }
   else
   {
      /* DEBUG_INFO (MOD_MAIN, "Interval-Timer successfully installed"); */
   }
}

/* ---------------------------------------------------------------------------- */

static void initializeIptCom (void*       pArgV)
{
   TDC_UNUSED (pArgV)

   tdcInitIptCom (MOD_MAIN);
}

/* ---------------------------------------------------------------------------- */

static void tdcFwInit (void*  pArgV)
{
   UINT32         i;

   TDC_UNUSED (pArgV)

   DEBUG_INFO (MOD_MAIN, "tMain  started successfully");

   for (i = 0; i < TAB_SIZE (initFwTab); i++)
   {
      if (!isDisaster ())
      {
         (void) tdcPrintf ("TDC:     %s", initFwTab[i].pInitText);
         initFwTab[i].pInitFct (pArgV);
         (void) tdcPrintf (" ... complete\n");
      }
      else
      {
         DEBUG_INFO (MOD_MAIN, "cannot Initialize");
      }
   }
}

/* ----- cyclicThreadStart() --------------------------------------------------
* Abstract    : Start up the TDC Cyclic (Intervall-Timer) Thread
* Parameter(s): pArgv - Not used within this function
* Return value: NULL  - Not used within this function and hence constant
* Remarks     :
* History     : 01-07-25     Ritz           TTC/EDE           modified
* ---------------------------------------------------------------------------- */
static /*@null@*/ void* cyclicThreadStart (/*@in@*/ void*      pArgV)
{
   (void) tdcPrintf  (STARTUP_TASK_FORMAT, TASKNAME_CYCLIC);
   tdcTCyclic (pArgV);

   // Cyclic Task is about to terminate
   if (tdcMutexLock (MOD_MD, taskIdMutexId) == TDC_MUTEX_OK)
   {
      tdcFreeMem (taskIdCyclic);
      taskIdCyclic = NULL;

      (void) tdcMutexUnlock (MOD_MD, taskIdMutexId);
   }

   DEBUG_INFO (MOD_MAIN, TASKNAME_CYCLIC " ... terminated");
   return     (NULL);
}

/* ----- msgDataThreadStart() --------------------------------------------------
* Abstract    : Start up the TDC Message Data Handling thread
* Parameter(s): pArgv - Not used within this function
* Return value: NULL  - Not used within this function and hence constant
* Remarks     :
* History     : 01-07-25     Ritz           TTC/EDE           modified
* ---------------------------------------------------------------------------- */
static /*@null@*/ void* msgDataThreadStart (void*       pArgV)
{
   (void) tdcPrintf (STARTUP_TASK_FORMAT, TASKNAME_MSGDATA);
   tdcTMsgData (pArgV);

   // MSGDATA Task is about to terminate
   if (tdcMutexLock (MOD_MD, taskIdMutexId) == TDC_MUTEX_OK)
   {
      tdcFreeMem (taskIdMsgData);
      taskIdMsgData = NULL;

      (void) tdcMutexUnlock (MOD_MD, taskIdMutexId);
   }

   DEBUG_INFO  (MOD_MAIN, TASKNAME_MSGDATA " ... terminated");
   return      (NULL);
}

/* ----- ipcServThreadStart() --------------------------------------------------
* Abstract    : Start up the TDC Interprocesscommunication Service thread
* Parameter(s): pArgv - Not used within this function
* Return value: NULL  - Not used within this function and hence constant
* Remarks     :
* History     : 01-07-25     Ritz           TTC/EDE           modified
* ---------------------------------------------------------------------------- */
static /*@null@*/ void* ipcServThreadStart (void*       pArgV)
{
   (void) tdcPrintf (STARTUP_TASK_FORMAT, TASKNAME_IPCSERV);
   tdcTIpcServ (pArgV);

   // IPCSERV Task is about to terminate
   if (tdcMutexLock (MOD_MD, taskIdMutexId) == TDC_MUTEX_OK)
   {
      tdcFreeMem (taskIdIpcServ);
      taskIdIpcServ = NULL;

      (void) tdcMutexUnlock (MOD_MD, taskIdMutexId);
   }

   DEBUG_INFO  (MOD_MAIN, TASKNAME_IPCSERV " ... terminated");
   return      (NULL);
}

/* ----- startupThreads() ----------------------------------------------------
* Abstract    : The TDC-Daemon consists of several threads.
*               tdcStartupThreads starts up every thread one by another.
* Parameter(s): ---
* Return value: ---
* Remarks     : For Debug purposes, it is possible to install some Signal-
*               Handlers. This must be done manually within this module.
* History     : 01-07-25     Ritz           TTC/EDE           modified
* ---------------------------------------------------------------------------- */
static void startupThreads (void)
{
   int      i;

   /* Start up every thread except the service (daemon) itself */

   DEBUG_INFO (MOD_MAIN, "Starting up all threads");

   for (i = 0; i < TAB_SIZE (threadIdTab); i++)
   {
      if (!bTerminate)
      {
         if (tdcMutexLock (MOD_MD, taskIdMutexId) == TDC_MUTEX_OK)
         {
            tdcThreadIdTab[threadIdTab[i].threadIdx] = startupTdcSingleThread ((const T_THREAD_FRAME*) &threadIdTab[i]);

            if (tdcThreadIdTab[threadIdTab[i].threadIdx] == NULL)
            {
               bTerminate = TRUE;      // terminate gracefully
            }

            (void) tdcMutexUnlock (MOD_MD, taskIdMutexId);
         }
         else
         {
            bTerminate = TRUE;         // terminate gracefully
         }
      }
   }

   if (!bTerminate)      /* Install some Signal Handlers for debug purposes, if neccessary */
   {
      tdcInstallSignalHandler ();
   }
}



/* ---------------------------------------------------------------------------- */

void tdcStartupFw (void*       pArgV)
{
   if (!ASSERT_BASE_TYPES ()) /*lint !e506 constant expression of*/
   {
      DEBUG_ERROR (MOD_MAIN, "Invalid Base-Types detected");
   }

   tdcFwInit      (pArgV);
   startupThreads ();

   // Wait a bit, in order to be sure that every task started up
   tdcSleep (500);
}

