/*******************************************************************************
 *  COPYRIGHT      : (c) 2006-12 Bombardier Transportation
 *******************************************************************************
 *  PROJECT        : IPTrain
 *
 *  MODULE         : vos.c
 *
 *  ABSTRACT       : The combined code for Windows XP, LINUX and CSS for lowlevel
 *                   Ethernet communication
 *
 *******************************************************************************
 *  HISTORY     :
 *  
 * $Id: vos.c 36143 2015-03-24 08:19:39Z gweiss $
 *
 *  CR-9583 (Gerhard Weiß, 2015-03-24)
 *          Support of Win7 Service
 *
 *  CR-7241 (Gerhard Weiss, Bernd Loehr, 2014-02-10)
 *  CR-7240 (Gerhard Weiss, Bernd Loehr, 2014-02-10)
 *          Correction of UNICODE Macro for wsprinf
 *
 *  CR-9020 (Ruben Scheja, 2014-01-29)
 *          Changing permissions for msgget to 0666
 *
 *  CR-4093 (Gerhard Weiss, 2012-04-20)
 *          Correcting timing for periodic tasks on VXWORKS
 *
 *  CR-3326 (Bernd Loehr, 2012-02-10)
 *           Improvement for 3rd party use / Darwin port added.
 *
 *  CR-3477 (Bernd Loehr, 2012-02-09)
 * 			TUEV Assessment findings
 *
 *  CR-480 (Gerhard Weiss, 2011-09-07)
 * 			Windows Support for OSBuild
 *          (mark string as unicode in msgqueue creation)
 *
 *  CR-432 (Gerhard Weiss, 2010-11-24)
 *          Corrected position of UNUSED Parameter Macros
 *          Added more missing UNUSED Parameter Macros
 *          Removed unused variables
 *
 *  CR-685 (Gerhard Weiss, 2010-08-24)
 *          Add interlock mechanism during termination, ensuring no Memory is
 *          freed before all threads are terminated.
 *
 *  Internal (Bernd Loehr, 2010-08-13) 
 * 			Old obsolete CVS history removed
 *
 *
 ******************************************************************************/

/*******************************************************************************
* INCLUDES */
#if defined(WIN32)
/* Windows XP */
 #include "vos_socket.h"      /* OS independent socket definitions */
 #include <windows.h>         /* Thread specific defines */
 #include <process.h>         /* Thread specific defines */
 #include <conio.h>           /* Console io routines */
 #include <stdio.h>           /* sprintf et al. */
 #include <time.h>            /* time et al. */
 #include <errno.h>           /* errno */
 #include <mmsystem.h>
 #include <sddl.h>            /* Security descriptor defintion language */
 #include "iptcom.h"          /* Common type definitions for IPT */
 #define INT32_ALREADY_DEFINED
 #include "ipt.h"                    /* Common type definitions for IPT  */
 #undef INT32_ALREADY_DEFINED
 #include "iptcom_priv.h"   /* Common type definitions for IPT */

#elif defined(LINUX)
/* LINUX */
 #include "iptcom.h"           /* Common type definitions for IPT */
 #include "iptcom_priv.h"   /* Common type definitions for IPT */
 #include <sys/ipc.h>
 #include <sys/sem.h>
 #include <sys/shm.h>
 #include <sys/types.h>
 #include <time.h>
 #include <sys/time.h>
 #include <sys/msg.h>
 #include <sys/stat.h>
 #include <pthread.h>
 #include <unistd.h>
 #include <errno.h>
 #include <string.h>
 #include <limits.h>
 #include <sys/syscall.h>
 #include <unistd.h> 
 #include "ipt.h"           /* Common type definitions for IPT */

#elif defined(VXWORKS)
/* VXWorks */
 #include <stdio.h>
 #include <string.h>
 #include <taskLib.h>
 #include <sysLib.h>
 #include <ioLib.h>      /* I/O interface library (close,...) */
 #include <timers.h>
 #include <semLib.h>
 #include <msgQLib.h>
 #include <intLib.h>

 #include "iptcom.h"           /* Common type definitions for IPT */
 #include "ipt.h"           /* Common type definitions for IPT */
 #include "iptcom_priv.h"   /* Common type definitions for IPT */

#elif defined(__INTEGRITY)
/* INTEGRITY */
 #include <INTEGRITY.h>
 #include <stdio.h>
 #include <string.h>
 #include <errno.h>
 #include <sys/time.h>
 #include "iptcom.h"           /* Common type definitions for IPT */
 #include "ipt.h"           /* Common type definitions for IPT */
 #include "iptcom_priv.h"   /* Common type definitions for IPT */

#elif defined(DARWIN)
/* Mac OS X */
#include "iptcom.h"           /* Common type definitions for IPT */
#include "iptcom_priv.h"   /* Common type definitions for IPT */
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <sys/syscall.h>
#include <unistd.h> 
#include "ipt.h"           /* Common type definitions for IPT */

#else
#error "Includes for target architecture are missing"
#endif

#include "vos.h"
#include "vos_priv.h"



/*******************************************************************************
* DEFINES */

#define TARGET_RESOLUTION 1         // 1-millisecond target resolution

/*******************************************************************************
* TYPEDEFS */

typedef struct threadargs
{
   IPT_THREAD_ROUTINE TheFunction;
   void *pArguments;
} VOS_THREAD_ARGS;

typedef struct cyclicthreadargs
{
   void (*pFunc)(void);
   int Interval;
} CYCLIC_THREAD_ARGS;

#if defined(__INTEGRITY)
typedef struct MsgQueue
{
   int size;
   void *buffer;
} MsgQueue;
#endif

/*******************************************************************************
* GLOBALS */

/*******************************************************************************
* LOCALS */

#if defined(WIN32)
/* Update frequency for Windows performance timer */
static __int64 ticksSec = 1;        /* Ticks per second */

static  UINT  wTimerRes;

static HANDLE  semaHandles [SEMA_MAX_COUNT];
static HANDLE  queueHandles [QUEUE_MAX_COUNT];

#endif

static int threadSystemShutdown = 0;

/* added for CR-685 */
static int threadsRunning = 0;

#if defined(LINUX) || defined(DARWIN)
/* File path to a writable file that is used to generate a unique number.
   For identification of message queues and shared memory.
   If the file do not exist it will be created.
   The file name and path may be set in the IPTCom XML configuration file */

static char linuxFile[LINUX_FILE_SIZE] = "/tmp/msgq";
#endif

#if defined(WIN32)
const LPTSTR winFile = TEXT("Global\\IPTComShrMem");
const LPTSTR winSemaName = TEXT("Global\\IPTComSema%02d");
static HANDLE hMapObject = NULL;  /* handle to file mapping */

const LPTSTR winSecurityDescriptor =
      TEXT("D:")                    /* Discretionary ACL */
      TEXT("(D;OICI;GA;;;BG)")      /* deny access to built-in guests */
      TEXT("(D;OICI;GA;;;AN)")      /* deny access to anonymus logon */
      TEXT("(A;OICI;GWGRGX;;;AU)")  /* allow read, write, execute to authenticated  users */
      TEXT("(A;OICI;GA;;;BU)");     /* allow for all to builtin-users ("BA" = built-in administrators) */
                                    /* ("BU" = built-in users) */
/* Security descriptor for accesing service from user space and vice-versa */

#endif


#if defined(VXWORKS)
static UINT32 timeStampFreq;
#endif

/*******************************************************************************
* LOCAL FUNCTIONS */

#if defined(WIN32)
/*******************************************************************************
NAME     :  getSecurityDescriptor

ABSTRACT :  Initializes the security descriptor

RETURNS  : 
*/
static void getSecurityDescriptor(SECURITY_ATTRIBUTES* pSecurity )
{
   pSecurity->lpSecurityDescriptor = NULL;
   ZeroMemory(pSecurity, sizeof(SECURITY_ATTRIBUTES));
   pSecurity->nLength = sizeof(SECURITY_ATTRIBUTES);
   ConvertStringSecurityDescriptorToSecurityDescriptor(
            winSecurityDescriptor,
            SDDL_REVISION_1,
            &pSecurity->lpSecurityDescriptor,
            NULL);
}
#endif

/*******************************************************************************
NAME     :  cyclicThread

ABSTRACT :

RETURNS  :
*/
static void cyclicThread(void *pData)
{
   CYCLIC_THREAD_ARGS *pArgs;
   UINT32 nextTime;
   UINT32 currentTime;
   UINT32 delayTime;
   UINT32 cycleTime;

   /* Verify that pointer is valid */
   if (pData == NULL)
      return;

   /* Setup pointer */
   pArgs = (CYCLIC_THREAD_ARGS *)pData;
   cycleTime = pArgs->Interval;

    nextTime = IPTVosGetMilliSecTimer();

    while (!threadSystemShutdown)
        {
      /* Call the thread function */
      pArgs->pFunc ();

      /* Calculate time for next cycle */
      nextTime = nextTime + cycleTime;

      currentTime = IPTVosGetMilliSecTimer();
      if (currentTime < nextTime)
      {
         delayTime = nextTime - currentTime;
         if (delayTime > cycleTime)
         {
            /* The clock has been changed.
               Use the interval time as delay value */
            delayTime = cycleTime;
            nextTime = currentTime + cycleTime;
         }
      }
      else
      {
         /* The clock has been changed or the thread function takes longer time
            than the interval time.
            Use the interval time as delay value */
         delayTime = cycleTime;
         nextTime = currentTime + cycleTime;
      }

      IPTVosTaskDelay(delayTime);
   }
   
   IPTVosFree((BYTE *)pArgs);

   MON_PRINTF("cyclic thread exits\n");
   threadsRunning--;
}

/*******************************************************************************
NAME:       RoutineStarter
ABSTRACT:   Help function to IPTVosThreadSpawn
RETURNS:    -
*/
#if defined(__INTEGRITY)
static void RoutineStarter(
   void)
{
   VOS_THREAD_ARGS *pVTA;
   Error RetVal;

   if ((RetVal = GetTaskIdentification(CurrentTask(), (Address *)&pVTA)) != Success)
   {
      IPTVosPrint1(IPT_ERR, "RoutineStarter GetTaskIdentification failed %d", RetVal);
      return;
   }

   if (pVTA == NULL)
   {
      IPTVosPrint0(IPT_ERR, "RoutineStarter VOS_THREAD_ARGS = NULL" );
      return;
   }

   /* Call task */
   if(pVTA->TheFunction == NULL)
   {
      IPTVosPrint0(IPT_ERR, "RoutineStarter TheFunction = NULL" );
      return;
   }

   pVTA->TheFunction(pVTA->pArguments);

   IPTVosFree((BYTE *) pVTA);   
}
#else
static VOS_STARTER_TYPE RoutineStarter(
   void *pArguments)             /* Pointer to thread arguments */
{
   VOS_THREAD_ARGS *pVTA = (VOS_THREAD_ARGS *)pArguments;

   /* Call task */
   pVTA->TheFunction(pVTA->pArguments);

   IPTVosFree((BYTE *) pArguments);

   return 0;
}
#endif
/*******************************************************************************
* GLOBAL FUNCTIONS */


/*******************************************************************************
NAME:       IPTVosSystemStartup
ABSTRACT:   Start system
RETURNS:    IPT_OK if successful, errorcode otherwise
*/
#if defined(LINUX) || defined(DARWIN)
int IPTVosSystemStartup(char *path)
#else
int IPTVosSystemStartup()
#endif
{
#if defined(WIN32)
   WORD wVersionRequested;
   WSADATA wsaData;
   int err;
   TIMECAPS tc;

   /* Get ticks-per-second of the performance counter. */
   /*  Note the necessary typecast to a LARGE_INTEGER structure */
   if (!QueryPerformanceFrequency((LARGE_INTEGER*) &ticksSec))
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSystemStartup : error hardware doesn't support performance counter\n");
      return (int)IPT_ERROR;  /* error hardware doesn't support performance counter */
   }

   wVersionRequested = MAKEWORD( 2, 2 );

   err = WSAStartup( wVersionRequested, &wsaData );
   if ( err != 0 )
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSystemStartup : could not find a usable WinSock DLL\n");
      return (int)IPT_ERROR;
   }

   /* Confirm that the WinSock DLL supports 2.2.*/
   /* Note that if the DLL supports versions greater    */
   /* than 2.2 in addition to 2.2, it will still return */
   /* 2.2 in wVersion since that is the version we      */
   /* requested.                                        */

   if ( LOBYTE( wsaData.wVersion ) != 2 ||
         HIBYTE( wsaData.wVersion ) != 2 )
   {
      /* Tell the user that we could not find a usable */
      /* WinSock DLL.                                  */
      (void)WSACleanup( );
      IPTVosPrint0(IPT_ERR, "IPTVosSystemStartup : could not find a usable WinSock DLL\n");
      return (int)IPT_ERROR;
   }

   /* Get the systems minimum time resolution */
   if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSystemStartup : could not get minimum time resolution\n");
      return (int)IPT_ERROR;
   }

   wTimerRes = min(max(tc.wPeriodMin, TARGET_RESOLUTION), tc.wPeriodMax);

   if (timeBeginPeriod(wTimerRes) != TIMERR_NOERROR)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSystemStartup : could not set minimum time resolution\n");
      return (int)IPT_ERROR;
   }

#elif defined(LINUX) || defined(DARWIN)
   FILE *stream;

   if ((path != NULL) && (strlen(path) > 0))
   {
      strncpy(linuxFile,path,sizeof(linuxFile)-1);
   }

   /* Create msgq file */
   if( (stream  = fopen(linuxFile, "a")) == NULL )
   {
      IPTVosPrint1(IPT_ERR, "IPTVosSystemStartup : Could not create file %s\n",linuxFile);
      return((int)IPT_ERROR);
   }
   else
      fclose(stream);

   /* Setup curses */
/* initscr();
   cbreak();
   noecho();
   nonl();
   intrflush(stdscr, FALSE);
   keypad(stdscr, TRUE);
*/

#elif defined(VXWORKS)
   int prevIOCTL;
   int newIOCTL;

   prevIOCTL = ioctl((int)stdin, FIOGETOPTIONS, 0);
   newIOCTL = prevIOCTL & ~OPT_LINE;

   timeStampFreq = sysTimestampFreq();

   if (timeStampFreq == 0)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSystemStartup : timeStampFreq is 0. Division impossible\n");
      return((int)IPT_ERROR);
   }

#elif defined(__INTEGRITY)
   /* Nothing to do */
#endif
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       IPTVosSystemShutdown
ABSTRACT:   Shutdown system
RETURNS:    -
*/
void IPTVosSystemShutdown(void)
{
#if defined(WIN32)
   int errCode;

   if (WSACleanup() == SOCKET_ERROR)
   {
      errCode =  WSAGetLastError();
      IPTVosPrint2(IPT_WARN,"WSACleanup Windows error=%d %s\n", errCode, strerror(errCode));
   }

   /* Restore minimum timer resolution */
   if (timeEndPeriod(wTimerRes) != TIMERR_NOERROR)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSystemStartup : could not restore minimum time resolution\n");
   }
#elif defined(LINUX) || defined(DARWIN)

   /*  Shutdown curses */
   /*endwin();*/

#elif defined(VXWORKS)
   /* Nothing to do */
#elif defined(__INTEGRITY)
   /* Nothing to do */
#endif

}

/*******************************************************************************
NAME:       IPTVosTaskDelay
ABSTRACT:   Put the calling thread to sleep for specified no of millisecs
RETURNS:    -
*/
void IPTVosTaskDelay(
   int delayInMs)    /* Delay in millisecs */
{
#if defined(WIN32)
   Sleep(delayInMs);
#elif defined(LINUX) || defined(DARWIN)
   int ret;
   struct timespec delayTime;
   struct timespec remainTime;
   if (delayInMs >= 1000 )
   {
      delayTime.tv_sec = delayInMs/1000;
      delayTime.tv_nsec = (delayInMs%1000) * 1000000;
   }
   else
   {
      delayTime.tv_sec = 0;
      delayTime.tv_nsec = delayInMs * 1000000;
   }
   ret = nanosleep(&delayTime, &remainTime);
   while ((ret != 0) && (errno == EINTR))
   {
      /* The sleep was interrupted,
         Wait the remaining time */
      ret = nanosleep(&remainTime, &remainTime);
   }

#elif defined(VXWORKS)

    int noTicks;
    int clock_rate;           /* ticks per second */

    /* ms -> ticks */
    clock_rate = sysClkRateGet();
    noTicks = (clock_rate * delayInMs + 999) / 1000;

   taskDelay(noTicks);

#elif defined(__INTEGRITY)
   (void) usleep ((unsigned long) (delayInMs * 1000));
#endif
}

/*******************************************************************************
NAME     :  IPTVosRegisterCyclicThread
ABSTRACT :
RETURNS  :
*/
void IPTVosRegisterCyclicThread(
                        void (*pFunc)(void),                         /* Pointer to thread function */
                        char *pName,                                 /* Thread name */
                        int interval,                                /* Thread cycle time */
                        int policy,                                  /* Thread policy */
                        int priority,                                /* Priority */
                        int stackSize)                               /* Stack size */
{
   CYCLIC_THREAD_ARGS *pArg;
    
   threadsRunning++;
    
   pArg = (CYCLIC_THREAD_ARGS *)IPTVosMalloc(sizeof(CYCLIC_THREAD_ARGS));
   if (pArg != NULL)
   {
      pArg->pFunc = pFunc;
      pArg->Interval = interval;
      (void)IPTVosThreadSpawn(pName, policy, priority, stackSize, &cyclicThread, pArg);
   }
}

/*******************************************************************************
NAME:       IPTVosThreadSpawn
ABSTRACT:   Spawns a thread
RETURNS:    Thread ID
*/
VOS_THREAD_ID IPTVosThreadSpawn(
   char *pName,                                 /* Thread name */
   int Policy,                                  /* Thread policy */
   int Priority,                                /* Priority */
   int StackSize,                               /* Stack size */
   IPT_THREAD_ROUTINE TheFunction,              /* Pointer to thread main function */
   void *pArguments)                            /* Pointer to thread arguments */
{
   VOS_THREAD_ARGS *pVTrArgs;

#if defined(WIN32)
   HANDLE hThread;
   unsigned threadID;
   int errCode;
   
   IPT_UNUSED (StackSize)

#elif defined(LINUX) || defined(DARWIN)
   pthread_t hThread;
   pthread_attr_t ThreadAttrib;
   struct sched_param SchedParam;  /* scheduling priority */
   int RetCode;

   IPT_UNUSED (pName)
   IPT_UNUSED (Policy)

#elif defined(VXWORKS)
   int threadID;
   char errBuf[80];

   IPT_UNUSED (Policy)

#elif defined(__INTEGRITY)
   Task T;
   Error RetVal;
#endif

   IPT_UNUSED (pName)
   IPT_UNUSED (Policy)
   
   pVTrArgs = (VOS_THREAD_ARGS *)IPTVosMalloc(sizeof(VOS_THREAD_ARGS));
   if (pVTrArgs != NULL)
   {
      pVTrArgs->TheFunction = TheFunction;
      pVTrArgs->pArguments = pArguments;
   }

#if defined(WIN32)
   /* Create the second thread. */
   hThread = (HANDLE)_beginthreadex( NULL, 0, &RoutineStarter, pVTrArgs, 0, &threadID );
   if ((unsigned long)hThread == (unsigned long) -1)
   {
      errCode = GetLastError();
      IPTVosPrint2(IPT_ERR, "IPTVosThreadSpawn _beginthreadex failed Windows error=%d %s\n",errCode, strerror(errCode));
      IPTVosFree((const BYTE *)pVTrArgs);
      return((HANDLE)-1);
   }

   /* Set the scheduling priority of the thread */
   if(SetThreadPriority(hThread, Priority) == 0)
   {
      errCode = GetLastError();
      IPTVosPrint2(IPT_ERR, "IPTVosThreadSpawn SetThreadPriority Windows error=%d %s\n",errCode, strerror(errCode));
      IPTVosFree((const BYTE *)pVTrArgs);
      return((HANDLE)-1);
   }

   return hThread;

#elif defined(LINUX) || defined(DARWIN)
   /* Initialize thread attributes to default values */
   RetCode = pthread_attr_init(&ThreadAttrib);
   if (RetCode != 0)
   {
      IPTVosPrint1(IPT_ERR, "IPTVosThreadSpawn pthread_attr_init failed Linux return=%d\n",RetCode );
      IPTVosFree((BYTE *) pVTrArgs);
      return(0);
   }

   /* Set stack size */
   if (StackSize > PTHREAD_STACK_MIN)
   {
      RetCode = pthread_attr_setstacksize(&ThreadAttrib, StackSize);
   }
   else
   {
      RetCode = pthread_attr_setstacksize(&ThreadAttrib, PTHREAD_STACK_MIN);
   }
   if (RetCode != 0)
   {
      IPTVosPrint1(IPT_ERR, "IPTVosThreadSpawn pthread_attr_setstacksize failed Linux return=%d\n",RetCode );
      IPTVosFree((BYTE *) pVTrArgs);
      return(0);
   }

   /* Detached thread */
   RetCode = pthread_attr_setdetachstate(&ThreadAttrib, PTHREAD_CREATE_DETACHED);
   if (RetCode != 0)
   {
      IPTVosPrint1(IPT_ERR, "IPTVosThreadSpawn pthread_attr_setdetachstate failed Linux return=%d\n",RetCode );
      IPTVosFree((BYTE *) pVTrArgs);
      return(0);
   }

   /* Set the policy of the thread */
   RetCode = pthread_attr_setschedpolicy(&ThreadAttrib, Policy);
   if (RetCode != 0)
   {
      IPTVosPrint1(IPT_ERR, "IPTVosThreadSpawn pthread_attr_setschedpolicy failed Linux return=%d\n",RetCode );
      IPTVosFree((BYTE *) pVTrArgs);
      return(0);
   }

   /* Set the scheduling priority of the thread */
   SchedParam.sched_priority = Priority;
   RetCode = pthread_attr_setschedparam(&ThreadAttrib, &SchedParam);
   if (RetCode != 0)
   {
      IPTVosPrint1(IPT_ERR, "IPTVosThreadSpawn pthread_attr_setschedparam failed Linux return=%d\n",RetCode );
      IPTVosFree((BYTE *) pVTrArgs);
      return(0);
   }

   /* Set inheritsched attribute of the thread */
   RetCode = pthread_attr_setinheritsched(&ThreadAttrib, PTHREAD_EXPLICIT_SCHED);
   if (RetCode != 0)
   {
      IPTVosPrint1(IPT_ERR, "IPTVosThreadSpawn pthread_attr_setinheritsched failed Linux return=%d\n",RetCode );
      IPTVosFree((BYTE *) pVTrArgs);
      return(0);
   }

   /* Create the thread */
   RetCode = pthread_create(&hThread, &ThreadAttrib, &RoutineStarter, (void *)pVTrArgs);
   if (RetCode != 0)
   {
      IPTVosPrint1(IPT_ERR, "IPTVosThreadSpawn pthread_create failed Linux return=%d\n",RetCode );
      IPTVosFree((BYTE *) pVTrArgs);
      return(0);
   }
   /* Destroy thread attributes */
   RetCode = pthread_attr_destroy(&ThreadAttrib);
   if (RetCode != 0)
   {
      IPTVosPrint1(IPT_ERR, "IPTVosThreadSpawn pthread_attr_destroy Linux failed return=%d\n",RetCode );
      IPTVosFree((BYTE *) pVTrArgs);
      return(0);
   }

   return hThread;  /*lint !e429 */
   /* lint warning "Custodial pointer 'pVTrArgs' (line 953) has not been freed or returned" */
   /* becaused pVTrArgs is not returned. The memory is returned in RoutineStarter instead */

#elif defined(VXWORKS)
   threadID = taskSpawn(pName, Priority, VX_FP_TASK, StackSize, (FUNCPTR) &RoutineStarter,
   (int)pVTrArgs, 0, 0, 0, 0, 0, 0, 0, 0, 0);
   if (threadID == -1)
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "taskSpawn failed. VxWorks errno=%#x %s\n",errno, errBuf);
      IPTVosFree((BYTE *) pVTrArgs);
      return(0);
   }

   return threadID;

#elif defined(__INTEGRITY)
   if ((RetVal = CommonCreateTask(Priority, (Address)&RoutineStarter, StackSize, pName, &T)) != Success)
   {
      IPTVosPrint2(IPT_ERR, "IPTVosThreadSpawn CommonCreateTask failed INTEGRITY error=%d %s\n", RetVal, strerror(RetVal));
      IPTVosFree((BYTE *) pVTrArgs);
      return 0;
   }

   if ((RetVal = SetTaskIdentification(T, (Address)pVTrArgs)) != Success)
   {
      IPTVosPrint2(IPT_ERR, "IPTVosThreadSpawn SetTaskIdentification failed INTEGRITY error=%d %s\n", RetVal, strerror(RetVal));
      IPTVosFree((BYTE *) pVTrArgs);
      return 0;      
   }

   if ((RetVal = RunTask(T)) != Success)
   {
      IPTVosPrint2(IPT_ERR, "IPTVosThreadSpawn RunTask failed INTEGRITY error=%d %s\n", RetVal, strerror(RetVal));
      IPTVosFree((BYTE *) pVTrArgs);
      return 0;      
   }

   return (VOS_THREAD_ID)T;
#else
#error "Code for target architecture is missing"
#endif

}

/*******************************************************************************
 NAME     :  vos_thread_terminate
 ABSTRACT :
 RETURNS  :
 */
void IPTVosThreadTerminate(void)
{
    threadSystemShutdown = 1;
}

/*******************************************************************************
 NAME     :  vos_thread_terminate
 ABSTRACT :  waits for termination of threads
 RETURNS  :
 */
void IPTVosThreadDone(void)
{
    MON_PRINTF("Waiting for Thread termination\n");
    while (threadsRunning)
    {
        IPTVosTaskDelay(1000);
    }
    MON_PRINTF("Done\n");
}

/*******************************************************************************
NAME:       IPTVosCreateSem
ABSTRACT:   Create a sempahore
RETURNS:    IPT_OK if success, IPT_SEM_ERR otherwise
*/
int IPTVosCreateSem(
   IPT_SEM *hSem,                /* Pointer to semaphore handle */
   IPT_SEM_STATE InitialState)   /* The initial state of the sempahore */
{
#if defined(WIN32)
   BOOL IniState;
   int errCode;

#ifdef LINUX_MULTIPROC
   SECURITY_ATTRIBUTES security;
   
   int i;
   HRESULT hr;
   TCHAR semaName[80];
#endif

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosCreateSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }
   if (InitialState == IPT_SEM_FULL)
      IniState = TRUE;
   else
      IniState = FALSE;

#ifdef LINUX_MULTIPROC
   for (i = 0; i < SEMA_MAX_COUNT && semaHandles[i] != NULL; i++) {
   }
   
   if (i >= SEMA_MAX_COUNT) {
      IPTVosPrint0(IPT_ERR, "IPTVosCreateSem: no more semas\n");
      return IPT_SEM_ERR;
   }
   
   hr = wsprintf(semaName, winSemaName, i);

   getSecurityDescriptor(&security);
   
   semaHandles[i] = CreateEvent(&security, FALSE, IniState, semaName);

   LocalFree(security.lpSecurityDescriptor);

   (*hSem) = i;

   if (semaHandles[i] == NULL)
#else
   (*hSem) = CreateEvent((LPSECURITY_ATTRIBUTES)NULL, FALSE, IniState, (LPTSTR)NULL);
   if (*hSem == NULL)
#endif
   {
      errCode = GetLastError();
      IPTVosPrint2(IPT_ERR, "IPTVosCreateSem: CreateEvent Windows error=%d %s\n",errCode, strerror(errCode));
      return((int)IPT_SEM_ERR);
   }

   return (int)IPT_OK;

#elif defined(LINUX) || defined(DARWIN)
#ifdef LINUX_MULTIPROC      /* Process semaphores for multi process environment */
   int ret;
   union semun
   {
      struct semid_ds *buf;
      unsigned short int *array;
      struct seminfo *__buf;
   } arg;
   unsigned short val[1];
   char errBuf[80];

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosCreateSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }

   ret = semget(IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
   if (ret == -1)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosCreateSem: semget Linux errno=%d %s\n",errno, errBuf);
      *hSem = (IPT_SEM) NULL;
      return (int)IPT_SEM_ERR;
   }
   else
   {
      *hSem = ret;
   }

   /* Set semaphore state to full or empty */
   val[0] = (InitialState == IPT_SEM_FULL) ? 1 : 0;
   arg.array = val;
   ret = semctl(*hSem, 0, SETALL, arg);
   if (ret == -1)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosCreateSem: semctl Linux errno=%d %s\n",errno, errBuf);
      *hSem = (IPT_SEM) NULL;
      return (int)IPT_SEM_ERR;
   }
   return IPT_OK;

#else
   int RetCode;
   char errBuf[80];

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosCreateSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }

   RetCode = pthread_mutex_init(&(hSem->mutex), NULL);

   if(RetCode != 0)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosCreateSem: pthread_mutex_init Linux errno=%d %s\n",errno, errBuf);
      return (int)IPT_SEM_ERR;
   }

   RetCode = pthread_cond_init(&(hSem->condition), NULL);

   if(RetCode != 0)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosCreateSem: pthread_cond_init Linux errno=%d %s\n",errno, errBuf);
      (void)pthread_mutex_destroy( &(hSem->mutex) );
      return (int)IPT_SEM_ERR;
   }

   if (InitialState == IPT_SEM_FULL)
      hSem->semCount = 1;
   else
      hSem->semCount = 0;

   return (int)IPT_OK;
#endif
#elif defined(VXWORKS)
   char errBuf[80];

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosCreateSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }

   (*hSem) = semMCreate (SEM_Q_PRIORITY | SEM_INVERSION_SAFE);
   if (*hSem != 0)
   {
      if (InitialState !=IPT_SEM_FULL)
      {
         if (semTake (((SEM_ID)*hSem), IPT_NO_WAIT) != OK)
         {
            return (int)IPT_SEM_ERR;
         }
      }
      return (int)IPT_OK;
   }
   else
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPTVosCreateSem failed: semMCreate VxWorks errno=%#x %s\n",errno, errBuf);
      return (int)IPT_SEM_ERR;
   }
#elif defined(__INTEGRITY)
   Semaphore sem;
   Error ret;

   ret = CreateSemaphore((InitialState == IPT_SEM_FULL) ? 1 : 0, &sem);
   if (ret == Success)
   {
      *hSem = sem;
      return (int)IPT_OK;
   }
   else
   {
      IPTVosPrint2(IPT_ERR, "IPTVosCreateSem: CreateSemaphore INTEGRITY error=%d %s\n", ret, strerror(ret));
      return (int)IPT_SEM_ERR;
   }
#else
#error "Code for target architecture is missing"
#endif
}

/*******************************************************************************
NAME:       IPTVosDestroySem
ABSTRACT:   Closes a sempahore
RETURNS:    -
*/
void IPTVosDestroySem(
   IPT_SEM *hSem)               /* Pointer to semaphore handle */
{
#if defined(WIN32)
   int errCode;

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosDestroySem: Wrong parameter hSem=NULL\n");
      return;
   }

#ifdef LINUX_MULTIPROC
   if (CloseHandle(semaHandles[*hSem]) == 0)
#else
   if (CloseHandle(*hSem) == 0)
#endif
   {
      errCode = GetLastError();
      IPTVosPrint2(IPT_ERR, "IPTVosDestroySem: CloseHandle Windows error=%d %s\n",errCode, strerror(errCode));
      *hSem = 0;
   }

#elif defined(LINUX) || defined(DARWIN)
   int res;
#ifdef LINUX_MULTIPROC
   union semun
   {
      struct semid_ds *buf;
      unsigned short int *array;
      struct seminfo *__buf;
   } arg;
   char errBuf[80];

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosDestroySem: Wrong parameter hSem=NULL\n");
      return;
   }

   res = semctl(*hSem, 1, IPC_RMID, arg); /*lint !e530 Ignore warning, arg is not used */
   if (res < 0)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosDestroySem: semctl Linux errno=%d %s\n",errno, errBuf);
   }
#else
   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosDestroySem: Wrong parameter hSem=NULL\n");
      return;
   }

   res  = pthread_mutex_destroy( &(hSem->mutex) );
   if (res != 0)
   {
      IPTVosPrint1(IPT_ERR, "IPTVosDestroySem: pthread_mutex_destroy Linux return=%d\n",res);
   }
#endif
#elif defined(VXWORKS)
   char errBuf[80];

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosDestroySem: Wrong parameter hSem=NULL\n");
      return;
   }

   if(semDelete((SEM_ID)*hSem) == ERROR)
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPTVosDestroySem failed: semDelete VxWorks errno=%#x %s\n",errno, errBuf);
   }

#elif defined(__INTEGRITY)
   Error ret;
   
   ret = CloseSemaphore((Semaphore)*hSem);
   if(ret != Success)
   {
      IPTVosPrint2(IPT_ERR, "IPTVosDestroySem: CloseSemaphore INTEGRITY error=%d %s\n", ret, strerror(ret));
   }
#else
#error "Code for target architecture is missing"
#endif
}

/*******************************************************************************
NAME:       IPTVosGetSem
ABSTRACT:   Try to get (decrease) a semaphore
RETURNS:    IPT_OK on success, IPT_SEM_ERR otherwise
*/
int IPTVosGetSem(
   IPT_SEM *hSem,                /* Pointer to semaphore handle */
   int timeOutInMs)              /* Timeout time */
{
#if defined(WIN32)
   DWORD dwTimeout;
 
   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }
   if (timeOutInMs == IPT_WAIT_FOREVER)
      dwTimeout = INFINITE;
   else
      dwTimeout = timeOutInMs;

#ifdef LINUX_MULTIPROC
   switch(WaitForSingleObject(semaHandles[*hSem], dwTimeout))
#else
   switch(WaitForSingleObject(*hSem, dwTimeout))
#endif
   {
   case WAIT_OBJECT_0:
      return (int)IPT_OK;
   case WAIT_ABANDONED:
      return (int)IPT_SEM_ERR;
   case WAIT_TIMEOUT:
      return (int)IPT_SEM_ERR;
   }

   IPTVosPrint1(IPT_ERR, "IPTVosGetSem: System Error (%d)\n", GetLastError());
   
   return (int)IPT_SEM_ERR;

#elif defined(LINUX) || defined(DARWIN)
#ifdef LINUX_MULTIPROC
   struct sembuf op[1];
   int ret = 1;
   char errBuf[80];
    
   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }
 
   op[0].sem_num = 0;         /* Use first and only semaphore */
   op[0].sem_op = -1;         /* Decrement by 1 */
   op[0].sem_flg = SEM_UNDO;   /* Permit undoing */

   if (timeOutInMs == IPT_WAIT_FOREVER)
   {
      while (1)
      {
	
         /* Wait forever */
         ret = semop(*hSem, op, 1);
	 
	 
         if (ret == 0)
         { 
            /* OK */
            break;
         }
         else if (errno != EINTR)
         {
            (void)strerror_r(errno,errBuf,sizeof(errBuf));
            IPTVosPrint2(IPT_ERR, "IPTVosGetSem: semop Linux errno=%d %s\n",errno, errBuf);
            break;
         }
      }
   }
   else
   {
      /* Return immediately */
      if (timeOutInMs == IPT_NO_WAIT)
      {
         op[0].sem_flg |= IPC_NOWAIT;
         while (1)
         {
            ret = semop(*hSem, op, 1);
            if ((ret == 0) || (errno == EAGAIN ))
            {
               break;
            }
            else if (errno != EINTR)
            {
               (void)strerror_r(errno,errBuf,sizeof(errBuf));
               IPTVosPrint2(IPT_ERR, "IPTVosGetSem: semop Linux errno=%d %s\n",errno, errBuf);
               break;
            }
         }
      }
      else
      {
#if 0
         struct timespec timeOut;

         /* Wait specified time */
         timeOut.tv_sec = timeOutInMs / 1000;
         timeOut.tv_nsec = (long) 1000000 * (timeOutInMs % 1000);
         ret = semtimedop(*hSem, op, 1, &timeOut);
#else
         IPTVosPrint0(IPT_ERR, "IPTVosGetSem with timeout in ms not supported\n");
#endif
      }
   }

   return (ret == 0) ? (int)IPT_OK : (int)IPT_SEM_ERR;
#else
   struct timespec delay;
   int RetCode;
   int timeout;
   char errBuf[80];

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }

   /* Wait forever */
   if (timeOutInMs == IPT_WAIT_FOREVER)
   {
      RetCode = pthread_mutex_lock(&(hSem->mutex));

      if (RetCode != 0)
      {
         (void)strerror_r(RetCode,errBuf,sizeof(errBuf));
         IPTVosPrint2(IPT_ERR, "IPTVosGetSem: pthread_mutex_lock Linux return=%d %s\n",RetCode, errBuf);
         return (int)IPT_SEM_ERR;
      }
   }
   else
   {
      /* Return immediately */
      if (timeOutInMs == IPT_NO_WAIT)
      {
         RetCode = pthread_mutex_trylock(&(hSem->mutex));
         if (RetCode != 0)
         {
            if (RetCode != EBUSY)
            {
               (void)strerror_r(RetCode,errBuf,sizeof(errBuf));
               IPTVosPrint2(IPT_ERR, "IPTVosGetSem: pthread_mutex_trylock Linux return=%d %s\n",RetCode, errBuf);
            }
            return (int)IPT_SEM_ERR;
         }
      }
      else
      {
         /* Wait with timeout */
         timeout = 0;
         do
         {
             delay.tv_sec = 0;
             delay.tv_nsec = 1000000;  /* 1 milli sec */

             RetCode = pthread_mutex_trylock(&(hSem->mutex));
             if (RetCode == 0)
             {
                /* we now own the mutex  */
                break;
             }
             else
             {
                 /* Did somebody else own the semaphore */
                 if (RetCode == EPERM )
                 {
                     /* Wait a millisec */
                     (void)nanosleep(&delay, NULL);
                     timeout++ ;
                 }
                 else
                 {
                     (void)strerror_r(RetCode,errBuf,sizeof(errBuf));
                     IPTVosPrint2(IPT_ERR, "IPTVosGetSem: pthread_mutex_trylock Linux return=%d %s\n",RetCode, errBuf);
                     /* Other error  */
                     return (int)IPT_SEM_ERR;
                 }
             }
         } while (timeout < timeOutInMs);
      }
   }

   while (hSem->semCount <= 0)
   {
      RetCode = pthread_cond_wait(&(hSem->condition), &(hSem->mutex));
      if (RetCode && errno != EINTR )
      {
         (void)strerror_r(RetCode,errBuf,sizeof(errBuf));
         IPTVosPrint2(IPT_ERR, "IPTVosGetSem: pthread_cond_wait Linux return=%d %s\n",RetCode, errBuf);
         break;
      }
   }
   hSem->semCount--;

   RetCode = pthread_mutex_unlock(&(hSem->mutex));
   if (RetCode != 0)
   {
      (void)strerror_r(RetCode,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosGetSem: pthread_mutex_unlock Linux return=%d %s\n",RetCode, errBuf);
      return (int)IPT_SEM_ERR;
   }

   return (int)IPT_OK;
#endif
#elif defined(VXWORKS)

   int noTicks;
   int clock_rate;           /* ticks per second */

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }

   if (timeOutInMs == IPT_NO_WAIT)
   {
      noTicks = IPT_NO_WAIT;
   }
   else if (timeOutInMs == IPT_WAIT_FOREVER)
   {
      noTicks = IPT_WAIT_FOREVER;
   }
   else
   {
      /* ms -> ticks */
      clock_rate = sysClkRateGet();
      noTicks = (clock_rate * timeOutInMs + 999) / 1000;
   }

   if (semTake (((SEM_ID)*hSem), noTicks) == OK)
      return (int)IPT_OK;
   else
      return (int)IPT_SEM_ERR;

#elif defined(__INTEGRITY)

   int ret = (int)IPT_SEM_ERR;
   long long fraction;
   Error res;

   if (timeOutInMs == IPT_NO_WAIT)
   {
      res = TryToObtainSemaphore((Semaphore)*hSem);
      if (res == Success)
      {
         ret = (int)IPT_OK;
      }
      else if (res != ResourceNotAvailable)
      {
         IPTVosPrint2(IPT_ERR, "IPTVosGetSem:: TryToObtainSemaphore INTEGRITY error=%d %s\n", res, strerror(res));
      }
   }
   else if (timeOutInMs == IPT_WAIT_FOREVER)
   {
      res = WaitForSemaphore((Semaphore)*hSem);
      if (res == Success)
      {
         ret = (int)IPT_OK;
      }
      else
      {
         IPTVosPrint2(IPT_ERR, "IPTVosGetSem:: WaitForSemaphore INTEGRITY error=%d %s\n", res, strerror(res));
      }
   }
   else
   {
      Time T;

      T.Seconds = timeOutInMs / 1000;
      /* 0x10000000 / 1000 = 4294967.296 (0x418937) */
      fraction = ((long long)(timeOutInMs % 1000)) << 32;
      T.Fraction = (UINT4)(fraction / 1000);
      
      res = TimedWaitForSemaphore((Semaphore)*hSem, &T);
      if (res == Success)
      {
         ret = (int)IPT_OK;
      }
      else
      {
         IPTVosPrint2(IPT_ERR, "IPTVosGetSem:: TimedWaitForSemaphore INTEGRITY error=%d %s\n", res, strerror(res));
      }
   } 

   return ret;
#else
#error "Code for target architecture is missing"
#endif

}

/*******************************************************************************
NAME:       IPTVosPutSemR
ABSTRACT:   Put (increase) a semaphore
RETURNS:    IPT_OK on success, IPT_SEM_ERR otherwise
*/
int IPTVosPutSemR(
   IPT_SEM *hSem)               /* Pointer to semaphore handle */
{
#if defined(WIN32)
   int errCode;

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }

#ifdef LINUX_MULTIPROC
   if (SetEvent(semaHandles[*hSem]) == 0)
#else
   if (SetEvent(*hSem) == 0)
#endif
   {
      errCode = GetLastError();
      IPTVosPrint2(IPT_ERR, "IPTVosPutSem: SetEvent Windows error=%d %s\n",errCode, strerror(errCode));
      return((int)IPT_SEM_ERR);
   }
   return((int)IPT_OK);

#elif defined(LINUX) || defined(DARWIN)
   int RetCode;
#ifdef LINUX_MULTIPROC
   struct sembuf op[1];
   char errBuf[80];

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }

   op[0].sem_num = 0;         /* Use first and only semaphore */
   op[0].sem_op = 1;          /* Increment by 1 */
   op[0].sem_flg = SEM_UNDO;  /* Permit undoing */

   while (1)
   {
      RetCode = semop(*hSem, op, 1);
      if (RetCode == 0)
      {
         break;
      }
      else if (errno != EINTR)
      {
         (void)strerror_r(errno,errBuf,sizeof(errBuf));
         IPTVosPrint2(IPT_ERR, "IPTVosPutSem: semop Linux errno=%d %s\n",errno, errBuf);
         return((int)IPT_SEM_ERR);
      }
   }
   return((int)IPT_OK);

#else
   char errBuf[80];

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }

   RetCode = pthread_mutex_lock(&(hSem->mutex));

   if (RetCode != 0)
   {
      (void)strerror_r(RetCode,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosPutSem: pthread_mutex_lock Linux return=%d %s\n",RetCode, errBuf);
      return((int)IPT_SEM_ERR);
   }

   if (hSem->semCount < 1)
      hSem->semCount ++;

   RetCode = pthread_mutex_unlock(&(hSem->mutex));
   if (RetCode != 0)
   {
      (void)strerror_r(RetCode,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosPutSem: pthread_mutex_unlock Linux return=%d %s\n",RetCode, errBuf);
      return((int)IPT_SEM_ERR);
   }

   RetCode = pthread_cond_signal(&(hSem->condition));
   if (RetCode != 0)
   {
      (void)strerror_r(RetCode,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosPutSem: pthread_cond_signal Linux return=%d %s\n",RetCode, errBuf);
      return((int)IPT_SEM_ERR);
   }
   return((int)IPT_OK);
#endif
#elif defined(VXWORKS)
   char errBuf[80];

   if (hSem == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosPutSem: Wrong parameter hSem=NULL\n");
      return((int)IPT_SEM_ERR);
   }

   if (semGive ((SEM_ID)*hSem) == ERROR)
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPTVosPutSem failed: semGive VxWorks errno=%#x %s\n",errno, errBuf);
      return((int)IPT_SEM_ERR);
   }
   return((int)IPT_OK);
#elif defined(__INTEGRITY)
   Error res;

   res = ReleaseSemaphore ((Semaphore)*hSem);
   if (res != Success)
   {
      IPTVosPrint2(IPT_ERR, "IPTVosPutSem: ReleaseSemaphore INTEGRITY error=%d %s\n", res, strerror(res));
      return((int)IPT_SEM_ERR);
   }
   return((int)IPT_OK);
#else
#error "Code for target architecture is missing"
#endif

}

/*******************************************************************************
NAME:       IPTVosPutSem
ABSTRACT:   Put (increase) a semaphore
RETURNS:    -
*/
void IPTVosPutSem(
   IPT_SEM *hSem)               /* Pointer to semaphore handle */
{
   (void)IPTVosPutSemR(hSem);
}

/*******************************************************************************
NAME:       IPTVosCreateMsgQueue
ABSTRACT:   Create a message queue
RETURNS:    IPT_OK if success, IPT_ERROR otherwise
*/
int IPTVosCreateMsgQueue(
   IPT_QUEUE *pQueueID, /* Pointer to queue */
   int maxNoMsg,        /* Maximum no of messages size */
   int maxLength)       /* Maximum message size */
{
#if defined(WIN32)
   SECURITY_ATTRIBUTES security;
   
   IPT_QUEUE_TYPE *pQ;
   int errCode;

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosCreateMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   pQ = (IPT_QUEUE_TYPE *)IPTVosMalloc(sizeof(IPT_QUEUE_TYPE));

   if (pQ)
   {
      /* Create unique name */
      InterlockedIncrement(&IPTGLOBAL(vos.QueueNo));
      wsprintf(pQ->mailSlotName, TEXT("\\\\.\\mailslot\\iptvos%d"), IPTGLOBAL(vos.QueueNo));

      pQ->maxNoMsg = maxNoMsg;
      pQ->noOfItemsInQueue = 0;

      /* Create queue with attr: return immediately and handle can not be inherited */

      getSecurityDescriptor(&security );

      pQ->hMailSlot = CreateMailslot(pQ->mailSlotName, maxLength, 0, &security);

      LocalFree(security.lpSecurityDescriptor);

      if (pQ->hMailSlot == INVALID_HANDLE_VALUE)
      {
         errCode = GetLastError();
         IPTVosPrint3(IPT_ERR, "IPTVosCreateMsgQueue: CreateMailslot %s Windows error=%d %s\n", pQ->mailSlotName, errCode, strerror(errCode));
         IPTVosFree ((unsigned char *)pQ);
         *pQueueID = (IPT_QUEUE)NULL;
         InterlockedIncrement(&IPTGLOBAL(vos.queueCnt.queuCreateErrCnt));
         return (int)IPT_QUEUE_ERR;
      }

      pQ->ProcID = GetCurrentProcessId();

#ifndef LINUX_MULTIPROC
      pQ->hFile = CreateFile(pQ->mailSlotName, GENERIC_WRITE, FILE_SHARE_READ,
         (LPSECURITY_ATTRIBUTES) NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);

      if (pQ->hFile == INVALID_HANDLE_VALUE)
      {
         errCode = GetLastError();
         IPTVosPrint2(IPT_ERR, "IPTVosCreateMsgQueue: CreateFile Windows error=%d %s\n",errCode, strerror(errCode));

         if(CloseHandle(pQ->hMailSlot) == 0)
         {
            errCode = GetLastError();
            IPTVosPrint2(IPT_ERR, "IPTVosCreateMsgQueue: CloseHandle Windows error=%d %s\n",errCode, strerror(errCode));
         }

         IPTVosFree ((unsigned char *)pQ);
         *pQueueID = (IPT_QUEUE)NULL;
         InterlockedIncrement(&IPTGLOBAL(vos.queueCnt.queuCreateErrCnt));
         return (int)IPT_QUEUE_ERR;
      }
      else
#endif
      {
         *pQueueID = pQ;
         IPTGLOBAL(vos.queueCnt.queueAllocated)++;
         if (IPTGLOBAL(vos.queueCnt.queueAllocated) > IPTGLOBAL(vos.queueCnt.queueMax))
         {
            IPTGLOBAL(vos.queueCnt.queueMax) = IPTGLOBAL(vos.queueCnt.queueAllocated);
         }

         return (int)IPT_OK;
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "Queue couldn't be created. Not enough memory\n");
      *pQueueID = (IPT_QUEUE)NULL;
      IPTGLOBAL(vos.queueCnt.queuCreateErrCnt)++;
      return (int)IPT_MEM_ERROR;
   }

#elif defined(LINUX) || defined(DARWIN)

   int   i;
   int   qid;
   key_t keyval;
   FILE  *fp;
   int msgmax;
   int msgmnb;
   char errBuf[80];

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosCreateMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   /* Check parameters */
   fp = fopen("/proc/sys/kernel/msgmax","r");
   if (fp)
   {
      fscanf(fp,"%d",&msgmax);
      fclose(fp);
      if (maxLength > msgmax)
      {
         IPTVosPrint2(IPT_ERR, "A queue couldn't be created. Max msg size=%d > msgmax=%d\n", maxLength, msgmax);
         *pQueueID = 0;
         IPTGLOBAL(vos.queueCnt.queuCreateErrCnt)++;
         return((int)IPT_QUEUE_ERR);
      }
   }

   fp = fopen("/proc/sys/kernel/msgmnb","r");
   if (fp)
   {
      fscanf(fp,"%d",&msgmnb);
      fclose(fp);
      if (maxNoMsg*maxLength > msgmnb)
      {
         if (maxLength != 0)
         {
            IPTVosPrint2(IPT_ERR, "A queue couldn't be created. Max msg no=%d > System limit=%d\n", maxNoMsg, msgmnb/maxLength);
         }
         else
         {
            IPTVosPrint3(IPT_ERR, "A queue couldn't be created. Max msg no=%d, Msgnb=%d, Maxlength=%d\n", maxNoMsg, msgmnb, maxLength);
         }

         *pQueueID = 0;
         IPTGLOBAL(vos.queueCnt.queuCreateErrCnt)++;
         return((int)IPT_QUEUE_ERR);
      }
   }

   i = 0;

   /* Try to create a free queue */
   do
   {
      IPTGLOBAL(vos.QueueNo)++;

      /* The least significant bit must not be zero when ftok is called */
      if ((IPTGLOBAL(vos.QueueNo) & 0xff) == 0)
      {
         IPTGLOBAL(vos.QueueNo)++;
      }

      /* Create unique queue key using file id */
      keyval = ftok(linuxFile, IPTGLOBAL(vos.QueueNo));
      if (keyval == -1)
      {
         (void)strerror_r(errno,errBuf,sizeof(errBuf));
         IPTVosPrint2(IPT_ERR, "A queue couldn't be created. ftok Linux errno=%d %s\n", errno, errBuf);
         *pQueueID = 0;
         IPTGLOBAL(vos.queueCnt.queuCreateErrCnt)++;
         return((int)IPT_QUEUE_ERR);
      }

      qid = msgget( keyval, IPC_CREAT | IPC_EXCL | 0666 );
      /* Avoid qid == 0, some Linux versions returns zero */
      if (qid == 0)
      {
         /* Destroy the queue with qid = 0 */
         (void)IPTVosDestroyMsgQueue(&qid);
      }
      else if(qid != -1)
      {
         *pQueueID = qid;
         IPTGLOBAL(vos.queueCnt.queueAllocated)++;
         if (IPTGLOBAL(vos.queueCnt.queueAllocated) > IPTGLOBAL(vos.queueCnt.queueMax))
         {
            IPTGLOBAL(vos.queueCnt.queueMax) = IPTGLOBAL(vos.queueCnt.queueAllocated);
         }

         /* A queue is created */
         return((int)IPT_OK);
      }
      else
      {
         /* Error not equal to "A message queue for the keyval already exists" */
         if (errno != EEXIST)
         {
            (void)strerror_r(errno,errBuf,sizeof(errBuf));
            IPTVosPrint2(IPT_ERR, "A queue couldn't be created. msgget Linux errno=%d %s\n", errno, errBuf);
            *pQueueID = 0;
            IPTGLOBAL(vos.queueCnt.queuCreateErrCnt)++;
            return((int)IPT_QUEUE_ERR);
         }
      }

      /* Try next keyval */
      i++;
   }
   while(i<256); /* More possible keyval? */

   /* No free queue available */
   IPTVosPrint0(IPT_ERR, "A queue couldn't be created, no free queues \n");
   *pQueueID = 0;
   IPTGLOBAL(vos.queueCnt.queuCreateErrCnt)++;
   return((int)IPT_QUEUE_ERR);


#elif defined(VXWORKS)
   MSG_Q_ID qid;
   char errBuf[80];

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosCreateMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   qid = msgQCreate(maxNoMsg,maxLength,MSG_Q_FIFO);
   if (qid != 0)
   {
      *pQueueID = qid;
      IPTGLOBAL(vos.queueCnt.queueAllocated)++;
      if (IPTGLOBAL(vos.queueCnt.queueAllocated) > IPTGLOBAL(vos.queueCnt.queueMax))
      {
         IPTGLOBAL(vos.queueCnt.queueMax) = IPTGLOBAL(vos.queueCnt.queueAllocated);
      }
      return((int)IPT_OK);
   }
   else
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPTVosCreateMsgQueue failed: msgQCreate VxWorks errno=%#x %s\n",errno, errBuf);
      *pQueueID = 0;
      IPTGLOBAL(vos.queueCnt.queuCreateErrCnt)++;
      return((int)IPT_QUEUE_ERR);
   }
#elif defined(__INTEGRITY)
   MessageQueue mq;
   int i;
   MsgQueue dummy;
   Error res;

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosCreateMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   /* Add 4 byte for size member */
   maxLength += sizeof(dummy.size);

   if (sizeof(MsgQueue) > maxLength)
   {
      maxLength = sizeof(MsgQueue);
   }

   /* maxNoMsg and maxLength must be a power of two */
   maxNoMsg--;
   maxLength--;
   for (i = 1; i < 32; i <<= 1)
   {
      maxNoMsg = maxNoMsg | (maxNoMsg >> i);
      maxLength = maxLength | (maxLength >> i);
   }
   maxNoMsg++;
   maxLength++;

   res = CreateMessageQueue(maxNoMsg,maxLength,MESSAGE_QUEUE_LOCAL, &mq);
   if (res == Success)
   {
      *pQueueID = mq;
      IPTGLOBAL(vos.queueCnt.queueAllocated)++;
      if (IPTGLOBAL(vos.queueCnt.queueAllocated) > IPTGLOBAL(vos.queueCnt.queueMax))
      {
         IPTGLOBAL(vos.queueCnt.queueMax) = IPTGLOBAL(vos.queueCnt.queueAllocated);  
      }
      return((int)IPT_OK);    
   }
   else
   {
     IPTVosPrint2(IPT_ERR, "Queue couldn't be created. CreateMessageQueue INTEGRITY error=%d %s\n", res, strerror(res));
      *pQueueID = 0;
      IPTGLOBAL(vos.queueCnt.queuCreateErrCnt)++;
      return((int)IPT_QUEUE_ERR);
   }
#else
#error "Code for target architecture is missing"
#endif
}

/*******************************************************************************
NAME:       IPTVosDestroyMsgQueue
ABSTRACT:   Destroys a message queue
RETURNS:    (int)IPT_OK if success, IPT_ERROR otherwise
*/
int IPTVosDestroyMsgQueue(IPT_QUEUE *pQueueID)
{
#if defined(WIN32)
   int errCode;
   HANDLE SourceProcess = NULL;
   HANDLE duplicateHandle = NULL;

   if ((pQueueID == NULL) || (*pQueueID == 0))
   {
      IPTVosPrint0(IPT_ERR, "IPTVosDestroyMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

#ifndef LINUX_MULTIPROC
   if (CloseHandle((*pQueueID)->hFile) == 0)
   {
      errCode = GetLastError();
      IPTVosPrint2(IPT_ERR, "IPTVosDestroyMsgQueue: CloseHandle Windows error=%d %s\n",errCode, strerror(errCode));
   }
#endif

   if((*pQueueID)->ProcID != GetCurrentProcessId())
   {
       SourceProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, (*pQueueID)->ProcID);

       if(DuplicateHandle(SourceProcess,                // SourceProcessHandle
                          (*pQueueID)->hMailSlot,       // SourceHandle
                          GetCurrentProcess(),          // TargetProcessHandle
                          &duplicateHandle,             // TargetHandle
                          0,                            // DesiredAccess
                          FALSE,                        // InheritHandle
                          DUPLICATE_CLOSE_SOURCE) == 0) // Options
       {
          errCode = GetLastError();
          IPTVosPrint2(IPT_ERR, "IPTVosDestroyMsgQueue: DuplicateHandle Windows error=%d %s\n",errCode, strerror(errCode));
          return((int)IPT_QUEUE_ERR);
       }

       CloseHandle(duplicateHandle);
       CloseHandle(SourceProcess);
   }
   else
   {
       if(CloseHandle((*pQueueID)->hMailSlot) == 0)
       {
          errCode = GetLastError();
          IPTVosPrint2(IPT_ERR, "IPTVosDestroyMsgQueue: CloseHandle Windows error=%d %s\n",errCode, strerror(errCode));
          return((int)IPT_QUEUE_ERR);
       }
   }
   
   IPTVosFree ((unsigned char *)(*pQueueID));
   IPTGLOBAL(vos.queueCnt.queueAllocated)--;

   return (int)IPT_OK;

#elif defined(LINUX) || defined(DARWIN)
   char errBuf[80];

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosDestroyMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   if (msgctl(*pQueueID, IPC_RMID, 0) == 0)
   {
      IPTGLOBAL(vos.queueCnt.queueAllocated)--;
      return((int)IPT_OK);
   }
   else
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "Queue couldn't be deleted. msgctl Linux errno=%d %s\n",errno, errBuf);
      IPTGLOBAL(vos.queueCnt.queuDestroyErrCnt)++;
      return((int)IPT_QUEUE_ERR);
   }

#elif defined(VXWORKS)
   char errBuf[80];

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosDestroyMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   if (msgQDelete((MSG_Q_ID)*pQueueID) == 0)
   {
      IPTGLOBAL(vos.queueCnt.queueAllocated)--;
      return((int)IPT_OK);
   }
   else
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPTVosDestroyMsgQueue failed: msgQDelete VxWorks errno=%#x %s\n",errno, errBuf);
      IPTGLOBAL(vos.queueCnt.queuDestroyErrCnt)++;
      return((int)IPT_QUEUE_ERR);
   }
#elif defined(__INTEGRITY)
   Error res;

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosDestroyMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   res = CloseMessageQueue((MessageQueue)*pQueueID);
   if (res == Success)
   {
      IPTGLOBAL(vos.queueCnt.queueAllocated)--;
      return((int)IPT_OK);
   }
   else
   {
      IPTVosPrint2(IPT_ERR, "Queue couldn't be deleted. CloseMessageQueue INTEGRITY error=%d %s\n", res, strerror(res));
      IPTGLOBAL(vos.queueCnt.queuDestroyErrCnt)++;
      return((int)IPT_QUEUE_ERR);
   }
#else
#error "Code for target architecture is missing"
#endif
}

/*******************************************************************************
NAME:       IPTVosSendMsgQueue
ABSTRACT:   Send a message on a queue
RETURNS:    IPT_OK if success, IPT_QUEUE_ERR otherwise

NOTE in the LINUX case the message buffer to be send has to have space for the
     long int mtype that is needed to be set by this procedure.
*/
int IPTVosSendMsgQueue(
   IPT_QUEUE *pQueueID,    /* Pointer to queue */
   char *pMsg,              /* Message buffer */
   unsigned int size)      /* Size of message buffer */
{
#if defined(WIN32)
   DWORD bytesWritten;
   int errCode;
#ifdef LINUX_MULTIPROC
   HANDLE sHandle;
#endif

   SECURITY_ATTRIBUTES security;

   if ((pQueueID == NULL) || (*pQueueID == 0))
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSendMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   /* Any place left in the queue ? */
   if ((*pQueueID)->noOfItemsInQueue >= (*pQueueID)->maxNoMsg)
   {
      IPTGLOBAL(vos.queueCnt.queuWriteErrCnt)++;
      IPTVosPrint2(IPT_ERR, "IPTVosSendMsgQueue: Queue=%#x %s full\n",pQueueID, (*pQueueID)->mailSlotName);
      return (int)IPT_QUEUE_ERR;
   }

   getSecurityDescriptor(&security );
   
#ifdef LINUX_MULTIPROC
      sHandle = CreateFile((*pQueueID)->mailSlotName,
                        GENERIC_WRITE,
                        FILE_SHARE_READ|FILE_SHARE_WRITE,
                        &security,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL);

   if (sHandle == INVALID_HANDLE_VALUE)
   {
      errCode = GetLastError();
      IPTVosPrint3(IPT_ERR, "IPTVosSendMsgQueue: createFile Windows error=%d %s %s\n", errCode, strerror(errCode), (*pQueueID)->mailSlotName);
      IPTGLOBAL(vos.queueCnt.queuWriteErrCnt)++;
      LocalFree(security.lpSecurityDescriptor);
      return (int)IPT_QUEUE_ERR;
   }
   LocalFree(security.lpSecurityDescriptor);

#endif
   (*pQueueID)->noOfItemsInQueue++;

#ifdef LINUX_MULTIPROC
   if (!WriteFile(sHandle, pMsg, (DWORD) size, &bytesWritten, (LPOVERLAPPED) NULL))
#else
   if (!WriteFile((*pQueueID)->hFile, pMsg, (DWORD) size, &bytesWritten, (LPOVERLAPPED) NULL))
#endif
   {
      errCode = GetLastError();
      IPTVosPrint2(IPT_ERR, "IPTVosSendMsgQueue: WriteFile Windows error=%d %s\n", errCode, strerror(errCode));
      (*pQueueID)->noOfItemsInQueue--;
      IPTGLOBAL(vos.queueCnt.queuWriteErrCnt)++;
      return (int)IPT_QUEUE_ERR;
   }

#ifdef LINUX_MULTIPROC
   if (sHandle != NULL)
   {
      if (CloseHandle(sHandle) == FALSE)
      {
         errCode = GetLastError();

         IPTVosPrint3(IPT_ERR, "IPTVosSendMsgQueue: closing file for Queue %s with error %d %s\n",
                      (*pQueueID)->mailSlotName,
                      errCode,
                      strerror(errCode));
      }
   }
#endif

   return (int)IPT_OK;

#elif defined(LINUX) || defined(DARWIN)
   int res;
   struct msgbuf *pMsgBuf;
   char errBuf[80];

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSendMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   pMsgBuf = (struct msgbuf *)pMsg; /*lint !e826 pMsg actually points to struct msgbuf*/
   pMsgBuf->mtype = IPT_MSG_BUF_TYPE;
   while (1)
   {
      res = msgsnd(*pQueueID, pMsgBuf, size - sizeof(long), IPC_NOWAIT);
      if (res == 0)
      {
         return((int)IPT_OK);
      }
      else if (errno != EINTR)
      {
         (void)strerror_r(errno,errBuf,sizeof(errBuf));
         IPTVosPrint2(IPT_ERR, "IPTVosSendMsgQueue: msgsnd Linux errno=%d %s\n",errno, errBuf);
         IPTGLOBAL(vos.queueCnt.queuWriteErrCnt)++;
         return((int)IPT_QUEUE_ERR);
      }
   }

#elif defined(VXWORKS)
   int res;
   char errBuf[80];

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSendMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   res = msgQSend((MSG_Q_ID)*pQueueID, pMsg, size,NO_WAIT, MSG_PRI_NORMAL);
   if (res == OK)
   {
      return((int)IPT_OK);
   }
   else
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPTVosSendMsgQueue failed: msgQSend VxWorks errno=%#x %s\n",errno, errBuf);
      IPTGLOBAL(vos.queueCnt.queuWriteErrCnt)++;
      return((int)IPT_QUEUE_ERR);
   }
#elif defined(__INTEGRITY)
   Error res;
   MsgQueue *MQ;

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosSendMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   res = AllocateMessageQueueBuffer((MessageQueue)*pQueueID, (void**)&MQ);
   
   if (res == Success)
   {
      MQ->size = size;
      memcpy(&MQ->buffer, pMsg, size);
      res = SendOnMessageQueue((MessageQueue)*pQueueID, MQ);
      if (res == Success)
      {
         return((int)IPT_OK);    
      }
      else
      {
         IPTVosPrint2(IPT_ERR, "IPTVosSendMsgQueue: SendOnMessageQueue INTEGRITY error=%d %s\n", res, strerror(res));
         IPTGLOBAL(vos.queueCnt.queuWriteErrCnt)++;
         return((int)IPT_QUEUE_ERR);    
      }
   }
   else
   {
      IPTVosPrint2(IPT_ERR, "IPTVosSendMsgQueue: AllocateMessageQueueBuffer INTEGRITY error=%d %s\n", res, strerror(res));
      IPTGLOBAL(vos.queueCnt.queuWriteErrCnt)++;
      return((int)IPT_QUEUE_ERR);
   }
#else
#error "Code for target architecture is missing"
#endif
}

/*******************************************************************************
NAME:       IPTVosReceiveMsgQueue
ABSTRACT:   Receive a message from a queue
RETURNS:    number of bytes read if success, IPT_QUEUE_ERR otherwise

NOTE in the LINUX case the message buffer to be send has to have space for the
     long int mtype that is needed to be set by this procedure.
*/
int IPTVosReceiveMsgQueue(
      IPT_QUEUE *pQueueID,       /* Pointer to queue */
      char *pMsg,               /* Message buffer */
      unsigned int size,         /* Size of messsage buffer */
      int timeout)               /* Timeout */
{
#if defined(WIN32)
   DWORD sizeOfNextMsg;
   DWORD noOfMessages;
   DWORD noOfBytesRead;
   DWORD sizeOfDataToRead;
   DWORD sizeOfTimeout;
   DWORD mailSlotTimeout;
   DWORD errCode;

   if ((pQueueID == NULL) || (*pQueueID == 0))
   {
      IPTVosPrint0(IPT_ERR, "IPTVosReceiveMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   /* Get queue info */
   if (!GetMailslotInfo((*pQueueID)->hMailSlot, (unsigned long *)NULL,
       &sizeOfNextMsg, &noOfMessages, &sizeOfTimeout))
   {
      errCode = GetLastError();
      IPTVosPrint3(IPT_ERR, "IPTVosReceiveMsgQueue: GetMailslotInfo Windows error=%d %s for %s\n",errCode, strerror(errCode), (*pQueueID)->mailSlotName);
      IPTGLOBAL(vos.queueCnt.queuReadErrCnt)++;
      return (int)IPT_QUEUE_ERR;
   }

   if (timeout == IPT_WAIT_FOREVER)
   {
      mailSlotTimeout = MAILSLOT_WAIT_FOREVER;
   }
   else if (timeout == IPT_NO_WAIT)
   {
      mailSlotTimeout = 0;
   }
   else
   {
      IPTVosPrint1(IPT_ERR, "IPTVosReceiveMsgQueue wrong timeout value=%d\n",timeout);
      return (int)IPT_INVALID_PAR;
   }

   /* Timeout <> previous used timeout? */
   if (sizeOfTimeout  != mailSlotTimeout)
   {
      /* Yes. Set new timeout */
      if (!SetMailslotInfo((*pQueueID)->hMailSlot, mailSlotTimeout))
      {
         errCode = GetLastError();
         IPTVosPrint2(IPT_ERR, "IPTVosReceiveMsgQueue: SetMailslotInfo Windows error=%d %s\n",errCode, strerror(errCode));
         IPTGLOBAL(vos.queueCnt.queuReadErrCnt)++;
         return (int)IPT_QUEUE_ERR;
      }
   }


   /* Check if there is any messages and timeout == no wait */
   if ((sizeOfNextMsg == MAILSLOT_NO_MESSAGE) && (mailSlotTimeout == 0))
   {
      /* No messages */
      return(0);
   }
   /* Bounds check the supplied buffer */
   else if (sizeOfNextMsg > size)
   {
      sizeOfDataToRead = size;
   }
   else
   {
      sizeOfDataToRead = sizeOfNextMsg;
   }

#if 1
   /* Read data from mailslot */
   if (ReadFile((*pQueueID)->hMailSlot, pMsg, sizeOfDataToRead, &noOfBytesRead, (LPOVERLAPPED) NULL))
   {
      (*pQueueID)->noOfItemsInQueue--;

      if (noOfBytesRead == 0) {
            IPTVosPrint1(IPT_ERR, "IPTVosReceiveMsgQueue: Exit %s with no data", (*pQueueID)->mailSlotName);
      }
      
      return (noOfBytesRead);
   }
#else
   while (ReadFile((*pQueueID)->hMailSlot, pMsg, sizeOfDataToRead, &noOfBytesRead, (LPOVERLAPPED) NULL))
   {
      if (noOfBytesRead) {
         (*pQueueID)->noOfItemsInQueue--;
         IPTVosPrint2(IPT_ERR, "IPTVosReceiveMsgQueue: Exit %s with %d bytes\n", (*pQueueID)->mailSlotName, noOfBytesRead);
         
         return (noOfBytesRead);
      }
   }
#endif
   /* CR-685 During thread termination an errorous read result is not an error */
   if (threadSystemShutdown)
       return (int)IPT_QUEUE_ERR;

   errCode = GetLastError();
   IPTVosPrint2(IPT_ERR, "IPTVosReceiveMsgQueue: ReadFile Windows error=%d %s\n",errCode, strerror(errCode));
   IPTGLOBAL(vos.queueCnt.queuReadErrCnt)++;
   return (int)IPT_QUEUE_ERR;

#elif defined(LINUX) || defined(DARWIN)
   int res;
   int msgFlg;
   struct msgbuf *pMsgBuf;
   char errBuf[80];

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosReceiveMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   pMsgBuf = (struct msgbuf *)pMsg;  /*lint !e826 pMsg actually points to struct msgbuf*/

   if (timeout == IPT_WAIT_FOREVER)
   {
      msgFlg = 0;
   }
   else if (timeout == IPT_NO_WAIT)
   {
      msgFlg = IPC_NOWAIT;
   }
   else
   {
      IPTVosPrint1(IPT_ERR, "IPTVosReceiveMsgQueue wrong timeout value=%d\n",timeout);
      return (int)IPT_INVALID_PAR;
   }

   pMsgBuf->mtype = IPT_MSG_BUF_TYPE;
   while (1)
   {
      res = msgrcv(*pQueueID, (struct msgbuf *)pMsg, size - sizeof(long), pMsgBuf->mtype, msgFlg); /*lint !e826 pMsg actually points to struct msgbuf*/
      if (res > 0)
      {
         return(res + sizeof(long));
      }
      else
      {
         if (errno == ENOMSG)
         {
            return(0);
         }
         else if (errno != EINTR)
         {
            (void)strerror_r(errno,errBuf,sizeof(errBuf));
            IPTVosPrint2(IPT_ERR, "IPTVosReceiveMsgQueue: msgrcv Linux errno=%d %s\n",errno, errBuf);
            IPTGLOBAL(vos.queueCnt.queuReadErrCnt)++;
            return((int)IPT_QUEUE_ERR);
         }
      }
   }

#elif defined(VXWORKS)
   int res;
   char errBuf[80];

   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosReceiveMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   if ((timeout != IPT_WAIT_FOREVER) &&
       (timeout != IPT_NO_WAIT))
   {
      IPTVosPrint1(IPT_ERR, "IPTVosReceiveMsgQueue wrong timeout value=%d\n",timeout);
      return (int)IPT_INVALID_PAR;
   }

   res = msgQReceive((MSG_Q_ID)*pQueueID, pMsg, size, timeout);

   if (res >= 0)
   {
      return(res);
   }
   else
   {
      if ((errno == S_objLib_OBJ_UNAVAILABLE) || (errno == S_objLib_OBJ_TIMEOUT))
      {
         return(0);
      }
      else
      {
         (void)strerror_r(errno, errBuf);
         IPTVosPrint2(IPT_ERR, "IPTVosReceiveMsgQueue failed: msgQReceive VxWorks errno=%#x %s\n",errno, errBuf);
         IPTGLOBAL(vos.queueCnt.queuReadErrCnt)++;
         return((int)IPT_QUEUE_ERR);
      }
   }
#elif defined(__INTEGRITY)
   Error res;
   MsgQueue *MQ;
   int ret;
   
   if (pQueueID == NULL)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosReceiveMsgQueue: Wrong parameter pQueueID=NULL\n");
      return((int)IPT_QUEUE_ERR);
   }

   switch (timeout)
   {
      case IPT_WAIT_FOREVER:
         res = ReceiveOnMessageQueue((MessageQueue)*pQueueID, (void**)&MQ);
         if (res != Success)
         {
            IPTVosPrint2(IPT_ERR, "IPTVosReceiveMsgQueue: ReceiveOnMessageQueue INTEGRITY error=%d %s\n", res, strerror(res));
         }
         break;
      case IPT_NO_WAIT:
         res = TryReceiveOnMessageQueue((MessageQueue)*pQueueID, (void**)&MQ);
         if (res == ResourceNotAvailable)
         {
            return 0;
         }
         else if (res != Success)
         {
            IPTVosPrint2(IPT_ERR, "IPTVosReceiveMsgQueue: TryReceiveOnMessageQueue INTEGRITY error=%d %s\n", res, strerror(res));
         }
         break;
      default:
          IPTVosPrint1(IPT_ERR, "IPTVosReceiveMsgQueue wrong timeout value=%d\n",timeout);
          return (int)IPT_INVALID_PAR;
   }

   if (res == Success)
   {
      ret = MQ->size;
      if (ret > size)
      {
         ret = size;
      }
      memcpy(pMsg, &MQ->buffer, ret);
      res = FreeMessageQueueBuffer((MessageQueue)*pQueueID, (void**)&MQ);
      if (res != Success)
      {
         IPTVosPrint2(IPT_ERR, "IPTVosRceeiveMsgQueue: FreeMessageQueueBuffer INTEGRITY error=%d %s\n", res, strerror(res));
      }

      return ret;
   }
   else
   {
      IPTGLOBAL(vos.queueCnt.queuReadErrCnt)++;
      return((int)IPT_QUEUE_ERR);    
   }
#else
#error "Code for target architecture is missing"
#endif
}

/*******************************************************************************
*  NAME     : IPTVosQueueShow
*
*  ABSTRACT :  Displays the queue statistic
*
*  RETURNS  :
*/

void IPTVosQueueShow(void)
{
   MON_PRINTF("No of allocated queues = %d\n",IPTGLOBAL(vos.queueCnt.queueAllocated));
   MON_PRINTF("Maximum no of allocated queues = %d\n",IPTGLOBAL(vos.queueCnt.queueMax));
   MON_PRINTF("No of queue create errors = %d\n",IPTGLOBAL(vos.queueCnt.queuCreateErrCnt));
   MON_PRINTF("No of queue destroy errors = %d\n",IPTGLOBAL(vos.queueCnt.queuDestroyErrCnt));
   MON_PRINTF("No of queue write errors = %d\n",IPTGLOBAL(vos.queueCnt.queuWriteErrCnt));
   MON_PRINTF("No of queue read errors = %d\n",IPTGLOBAL(vos.queueCnt.queuReadErrCnt));
}

/*******************************************************************************
NAME:      IPVosDateTimeString
ABSTRACT:  Get date and time string
RETURNS:
*/
void IPVosDateTimeString(char *pString)
{
#if defined(WIN32)
   struct tm ltm;
   time_t curTime;

   curTime = time((time_t *)NULL);
   (void)localtime_v(&curTime, &ltm);  /* convert to local time */
   sprintf(pString,"%04d-%02d-%02d %02d:%02d:%02d",
           1900+(ltm.tm_year), 1+(ltm.tm_mon), ltm.tm_mday,
           ltm.tm_hour, ltm.tm_min, ltm.tm_sec);

#elif defined(LINUX) || defined(DARWIN)
   struct timeval tv;
   struct timezone tz;
   struct tm ltm;
   char errBuf[80];

   if (gettimeofday(&tv, &tz) == 0)
   {
      (void)localtime_r(&tv.tv_sec, &ltm);  /* convert to local time */
      sprintf(pString,"%04d-%02d-%02d %02d:%02d:%02d:%03d",
              1900+(ltm.tm_year), 1+(ltm.tm_mon), ltm.tm_mday,
              ltm.tm_hour, ltm.tm_min, ltm.tm_sec, (int)(tv.tv_usec/1000));
   }
   else
   {
      sprintf(pString," ");
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPVosDateTimeString: gettimeofday Linux errno=%d %s\n",errno, errBuf);
   }

#elif defined(VXWORKS)
   struct timespec curtime;
   UINT32 ticSubPart;
   UINT32 timestamp;
   int    lvl;
   struct tm ltm;
   char errBuf[80];

   lvl = intLock();

   if (clock_gettime(CLOCK_REALTIME, &curtime) == ERROR)
   {
      sprintf(pString," ");
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPVosDateTimeString failed: clock_gettime VxWorks errno=%#x %s\n",errno, errBuf);

      (void)intUnlock(lvl);
   }
   else
   {
      timestamp = sysTimestamp();           /* Use unlocked version */

      (void)intUnlock(lvl);

      /* Compute how long we are into the tic period in ns */
      ticSubPart = ((timestamp * 1000 + timeStampFreq/2000000)/(timeStampFreq/1000000));

      /* convert to local time */
      (void)localtime_r(&curtime.tv_sec, &ltm);

      sprintf(pString,"%04d-%02d-%02d %02d:%02d:%02d:%03d",
              1900+(ltm.tm_year), 1+(ltm.tm_mon), ltm.tm_mday,
              ltm.tm_hour, ltm.tm_min, ltm.tm_sec, (int)(curtime.tv_nsec + ticSubPart)/1000000);
   }
#elif defined(__INTEGRITY)
   struct timeval tv;
   struct tm ltm;

   if (gettimeofday(&tv, NULL) == 0)
   {
      (void)localtime_r(&tv.tv_sec, &ltm);  /* convert to local time */
      sprintf(pString,"%04d-%02d-%02d %02d:%02d:%02d:%03d",
              1900+(ltm.tm_year), 1+(ltm.tm_mon), ltm.tm_mday,
              ltm.tm_hour, ltm.tm_min, ltm.tm_sec, (int)(tv.tv_usec/1000));
   }
   else
   {
      sprintf(pString," ");
      IPTVosPrint2(IPT_ERR, "IPVosDateTimeString: gettimeofday INTEGRITY errno=%d %s\n",errno, strerror(errno));
   }
#elif defined(DARWIN)
    struct timeval tv;
    struct timezone tz;
    struct tm ltm;
    char errBuf[80];
    
    if (gettimeofday(&tv, &tz) == 0)
    {
        (void)localtime_r(&tv.tv_sec, &ltm);  /* convert to local time */
        sprintf(pString,"%04d-%02d-%02d %02d:%02d:%02d:%03d",
                1900+(ltm.tm_year), 1+(ltm.tm_mon), ltm.tm_mday,
                ltm.tm_hour, ltm.tm_min, ltm.tm_sec, (int)(tv.tv_usec/1000));
    }
    else
    {
        sprintf(pString," ");
        (void)strerror_r(errno,errBuf,sizeof(errBuf));
        IPTVosPrint2(IPT_ERR, "IPVosDateTimeString: gettimeofday Unix errno=%d %s\n",errno, errBuf);
    }
    
#else
#error "Code for target architecture is missing"
#endif
}

/*******************************************************************************
NAME:      IPVosGetSecTimer
ABSTRACT:  Get second counter value
RETURNS:   Current value of second counter
*/
unsigned long IPVosGetSecTimer(void)
{
#if defined(WIN32)
   __int64 perfCount;
   unsigned long RetVal;

   /* Initialize the counter. */
   if (QueryPerformanceCounter((LARGE_INTEGER*)&perfCount) == 0)
   {
      IPTVosPrint0(IPT_ERR, "IPVosGetSecTimer: high-resolution timer not supported\n");
      return 0;
   }

   if (ticksSec != 0)
   {
      RetVal = (unsigned long)((perfCount/ticksSec));
      return RetVal;
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPVosGetSecTimer: Ticks per second is 0. Division impossible\n");
      return 0;
   }

#elif defined(LINUX)

   struct timespec curtime;
   char errBuf[80];
   
   
#if defined(LINUX_ARM_OTN_PLATF)
   if (clock_gettime(CLOCK_REALTIME, &curtime) != 0) 
#else
   /* syscall is used as clock_gettime is not included in most libraries */
   if (syscall(__NR_clock_gettime,CLOCK_MONOTONIC, &curtime) != 0)
#endif
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPVosGetSecTimer failed: clock_gettime Linux errno=%#x %s\n",errno, errBuf);
      /* error */
      return 0;
   }

   return(curtime.tv_sec);

#elif defined(VXWORKS)
   struct timespec curtime;
   char errBuf[80];

   if (clock_gettime(CLOCK_MONOTONIC, &curtime) == ERROR)
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPVosGetSecTimer failed: clock_gettime VxWorks errno=%#x %s\n",errno, errBuf);
      /* error */
      return 0;
   }

   return(curtime.tv_sec);
#elif defined(__INTEGRITY)
   Time CurTime;
   Error res;

   res = GetClockTime(HighestResStandardClock, &CurTime);
   if (res == Success)
   {
      return CurTime.Seconds;
   }
   else
   {
      IPTVosPrint2(IPT_ERR, "IPVosGetSecTimer: GetClockTime INTEGRITY errno=%d %s\n",res, strerror(res));
      /* error */
      return 0;
   }
#elif defined(DARWIN)
    
    struct timeval curtime;
    char errBuf[80];
    
    if (gettimeofday(&curtime, NULL) != 0) 
    {
        (void)strerror_r(errno,errBuf,sizeof(errBuf));
        IPTVosPrint2(IPT_ERR, "IPVosGetSecTimer failed: gettimeofday Unix errno=%#x %s\n",errno, errBuf);
        /* error */
        return 0;
    }
    
    return(curtime.tv_sec);
    
#else
#error "Code for target architecture is missing"
#endif

}


/*******************************************************************************
NAME:      IPTVosGetMilliSecTimer
ABSTRACT:  Get millisecond counter value
RETURNS:   Current value of millisecond counter
*/
UINT32 IPTVosGetMilliSecTimer(void)
{
#if defined(WIN32)
   __int64 perfCount;
   UINT32 RetVal;

   /* Initialize the counter. */
   if (QueryPerformanceCounter((LARGE_INTEGER*)&perfCount) == 0)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetMilliSecTimer: high-resolution timer not supported\n");
      return 0;
   }

   if (ticksSec != 0)
   {
      RetVal = (UINT32)((perfCount/ticksSec)*1000 + ((perfCount%ticksSec)*1000)/ticksSec);
      return RetVal;
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetMilliSecTimer: Ticks per second is 0. Division impossible\n");
      return 0;
   }

#elif defined(LINUX)
   struct timespec curtime;
   char errBuf[80];
   
#if defined(LINUX_ARM_OTN_PLATF)
   if (clock_gettime(CLOCK_REALTIME, &curtime) != 0) 
#else
   /* syscall is used as clock_gettime is not included in most libraries */
   if (syscall(__NR_clock_gettime,CLOCK_MONOTONIC, &curtime) != 0)
#endif
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosGetMilliSecTimer failed: clock_gettime Linux errno=%#x %s\n",errno, errBuf);
      /* error */
      return 0;
   }
   return(curtime.tv_sec * 1000 + curtime.tv_nsec/1000000);

#elif defined(VXWORKS)
   struct timespec curtime;
   UINT32 ticSubPart;
   double timestamp;
   int    lvl;
   char errBuf[80];

   lvl = intLock();

   if (clock_gettime(CLOCK_MONOTONIC, &curtime) == ERROR)
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPTVosGetMilliSecTimer failed: clock_gettime VxWorks errno=%#x %s\n",errno, errBuf);
      /* error */
      (void)intUnlock(lvl);
      return 0;
   }

   timestamp = sysTimestamp();           /* Use unlocked version */

   (void)intUnlock(lvl);

   /* Compute how long we are into the tic period in ns */
   /* CR-3477 TUEV objected this algorithm:
    ticSubPart = ((timestamp * 1000 + timeStampFreq/2000000)/(timeStampFreq/1000000));
    */
   ticSubPart = (double) timestamp * 1000000000.5 / (double) timeStampFreq;
   
   return(curtime.tv_sec * 1000 + (curtime.tv_nsec + ticSubPart + 500000)/1000000);
#elif defined(__INTEGRITY)
   Time CurTime;
   long long usec;
   Error res;

   res = GetClockTime(HighestResStandardClock, &CurTime);
   if (res == Success)
   {
      usec = (long long)CurTime.Fraction * (long long)1000;
      return CurTime.Seconds * 1000 + (UINT32)(usec >> 32);
   }
   else
   {
      IPTVosPrint2(IPT_ERR, "IPTVosGetMilliSecTimer: GetClockTime INTEGRITY errno=%d %s\n",res, strerror(res));
      /* error */
      return 0;
   }
#elif defined(DARWIN)
    struct timeval curtime;
    char errBuf[80];
    
    if (gettimeofday(&curtime, NULL) != 0) 
    {
        (void)strerror_r(errno,errBuf,sizeof(errBuf));
        IPTVosPrint2(IPT_ERR, "IPTVosGetMilliSecTimer failed: gettimeofday Unix errno=%#x %s\n",errno, errBuf);
        /* error */
        return 0;
    }
    return(curtime.tv_sec * 1000 + curtime.tv_usec/1000);
    
#else
#error "Code for target architecture is missing"
#endif

}

/*******************************************************************************
NAME:      IPTVosGetMicroSecTimer
ABSTRACT:  Get microsecond counter value
RETURNS:   Current value of microsecond counter
*/
UINT32 IPTVosGetMicroSecTimer(void)
{
#if defined(WIN32)
   __int64 perfCount;
   UINT32 RetVal;

   /* Initialize the counter. */
   if (QueryPerformanceCounter((LARGE_INTEGER*)&perfCount) == 0)
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetMilliSecTimer: high-resolution timer not supported\n");
      return 0;
   }

   if (ticksSec != 0)
   {
      RetVal = (UINT32)((perfCount/ticksSec)*1000000 + ((perfCount%ticksSec)*1000000)/ticksSec);
      return RetVal;
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetMicroSecTimer: Ticks per second is 0. Division impossible\n");
      return 0;
   }

#elif defined(LINUX)
   struct timespec curtime;
   char errBuf[80];
   
#if defined(LINUX_ARM_OTN_PLATF)
   if (clock_gettime(CLOCK_REALTIME, &curtime) != 0) 
#else
   /* syscall is used as clock_gettime is not included in most libraries */
   if (syscall(__NR_clock_gettime,CLOCK_MONOTONIC, &curtime) != 0)
#endif
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR, "IPTVosGetMicroSecTimer failed: clock_gettime Linux errno=%#x %s\n",errno, errBuf);
      /* error */
      return 0;
   }
   return(curtime.tv_sec * 1000000 + curtime.tv_nsec/1000);


#elif defined(VXWORKS)
   struct timespec curtime;
   UINT32 ticSubPart;
   double timestamp;
   int    lvl;
   char errBuf[80];

   lvl = intLock();

   if(clock_gettime(CLOCK_MONOTONIC,&curtime) == ERROR)
   {
      (void)strerror_r(errno, errBuf);
      IPTVosPrint2(IPT_ERR, "IPTVosGetMicroSecTimer failed: clock_gettime VxWorks errno=%#x %s\n",errno, errBuf);
      /* error */
      return 0;
   }

   timestamp = sysTimestamp();           /* Use unlocked version */

   (void)intUnlock(lvl);

   /* Compute how long we are into the tic period in ns */
   /* CR-3477 TUEV objected this algorithm:
        ticSubPart = ((timestamp * 1000 + timeStampFreq/2000000)/(timeStampFreq/1000000));
    */
   ticSubPart = (double) timestamp * 1000000000.5 / (double) timeStampFreq;
    
   return(curtime.tv_sec * 1000000 + (curtime.tv_nsec + ticSubPart + 500)/1000);
#elif defined(__INTEGRITY)
   Time CurTime;
   long long usec;
   Error res;

   res = GetClockTime(HighestResStandardClock, &CurTime);
   if (res == Success)
   {
      usec = (long long)CurTime.Fraction * (long long)1000000;
      return CurTime.Seconds * 1000000 + (UINT32)(usec >> 32);
   }
   else
   {
      IPTVosPrint2(IPT_ERR, "IPTVosGetMicroSecTimer: GetClockTime INTEGRITY errno=%d %s\n",res, strerror(res));
      /* error */
      return 0;
   }
#elif defined(DARWIN)
    struct timeval curtime;
    char errBuf[80];
    
    if (gettimeofday(&curtime, NULL) != 0) 
    {
        (void)strerror_r(errno,errBuf,sizeof(errBuf));
        IPTVosPrint2(IPT_ERR, "IPTVosGetMicroSecTimer failed: gettimeofday Unix errno=%#x %s\n",errno, errBuf);
        /* error */
        return 0;
    }
    return(curtime.tv_sec * 1000000 + curtime.tv_usec);
    
#else
#error "Code for target architecture is missing"
#endif

}

#ifdef LINUX_MULTIPROC
#if defined(LINUX)
/*******************************************************************************
NAME:      IPTVosCreateSharedMemory
ABSTRACT:  Create an area of shared memory to be used for globals in
           Linux multi-process systems.
RETURNS:   Pointer to memory if OK, NULL if not.
*/
char *IPTVosCreateSharedMemory(
                              UINT32 size)   /* Size of shared memory area */
{
   int key, segId;
   IPT_SHARED_MEMORY_CB *pShm;
   char *pTop = (char *) 0x40000000;   /* Start address for Linux shared libraries */
   char errBuf[80];

   /* Create unique queue key using a file id */
   key = ftok(linuxFile, 1);
   if (key == -1)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR,"IPTVosCreateSharedMemory FAILED. ftok Linux errno=%d %s\n",errno, errBuf);
      return NULL;
   }

   /* Allocate a shared memory segment.  */
   segId = shmget(key, size,
      IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);

   if (segId ==  -1)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint3(IPT_ERR, "IPTVosCreateSharedMemory shmget FAILED size=%d Linux errno=%d %s\n",size, errno, errBuf);
      return NULL;
   }

   /* Attach to the shared memory segment, at address just below shared libraries. */
   pShm = (IPT_SHARED_MEMORY_CB *) shmat(segId, pTop - size, SHM_RND);
   if ((void *) pShm == (void *) -1)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint4(IPT_ERR, "IPTVosCreateSharedMemory shmat FAILED, segId=%d, size=%d, Linux errno=%d %s\n", segId, size, errno, errBuf);
      return NULL;
   }

   /* Clear memory */
   memset(pShm, 0, size);

   /* Store segment ID and address first in global area */
   pShm->segId = segId;
   pShm->shmAddr = (char *) pShm;

   /* printf("Creating shared memory, key: %x, segId: %d, pShm: %x\n", key, segId, pShm);*/

   return (char *) pShm;
}

/*******************************************************************************
NAME:      IPTVosDestroySharedMemory
ABSTRACT:  Destroy an area of shared memory used for globals in
           Linux multi-process systems.
RETURNS:   -
*/
void IPTVosDestroySharedMemory(
                           char *pSharedMem) /* Pointer to shared memory */
{
   int ret, segId;
   IPT_SHARED_MEMORY_CB *pShm;
   char errBuf[80];

   /* Get segment ID, should be found first in global area */
   pShm = (IPT_SHARED_MEMORY_CB *)((void *)pSharedMem);
   segId = pShm->segId;

   /* printf("Destroying shared memory, segId: %d, pShm: %x\n", segId, pShm);*/

   /* Detach the shared memory segment.  */
   ret = shmdt(pSharedMem);
   if (ret != 0)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR,"shmdt failed Linux errno=%d %s\n",errno, errBuf);
   }
   else
   {
      /* Deallocate the shared memory segment.  */
      ret = shmctl(segId, IPC_RMID, 0);
      if (ret != 0)
      {
         (void)strerror_r(errno,errBuf,sizeof(errBuf));
         IPTVosPrint2(IPT_ERR,"shmctl failed Linux errno=%d %s\n",errno, errBuf);
      }
   }

   /* printf("Destroying shared memory, segId: %d, p: %x\n", segId, pShm);*/

}

/*******************************************************************************
NAME:      IPTVosAttachSharedMemory
ABSTRACT:  Attach to an area of shared memory used for globals in
           Linux multi-process systems. The area must have been created before
           by IPTVosCreateSharedMemory.
RETURNS:   Pointer to memory if OK, NULL if not.
*/
char *IPTVosAttachSharedMemory(void)
{
   int key, segId, ret;
   IPT_SHARED_MEMORY_CB *pShm;
   void *pNewShm;
   char errBuf[80];

   /* Create unique queue key using a file id. */
   key = ftok(linuxFile, 1);
   if (key == -1)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR,"IPTVosAttachSharedMemory FAILED. ftok Linux errno=%d %s\n",errno, errBuf);
      return NULL;
   }
   /* Get segment id  */
   segId = shmget(key, 0, 0);
   if (segId == -1)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR,"IPTVosAttachSharedMemory FAILED. shmget Linux errno=%d %s\n",errno, errBuf);
      return NULL;
   }

   /* Attach to the shared memory segment.  */
   pShm = (IPT_SHARED_MEMORY_CB *) shmat(segId, 0, 0);
   if ((void *) pShm == (void *) -1)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR,"IPTVosAttachSharedMemory FAILED. shmat Linux errno=%d %s\n",errno, errBuf);
      return NULL;
   }

   /* Get the address at which the primary process has located the shared memory */
   pNewShm = pShm->shmAddr;

   /* printf("Attaching shared memory, key: %x, segId: %d, pShm: %x, pNewShm: %x\n", key, segId, pShm, pNewShm); */

   /* Detach the shared memory and then reattach at the right address */
   ret = shmdt((void *) pShm);
   if (ret != 0)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR,"IPTVosAttachSharedMemory FAILED. shmdt Linux errno=%d %s\n",errno, errBuf);
      return NULL;
   }

   pShm = (IPT_SHARED_MEMORY_CB *) shmat(segId, pNewShm, 0);
   if ((void *) pShm == (void *) -1)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR,"IPTVosAttachSharedMemory FAILED. shmat Linux errno=%d %s\n",errno, errBuf);
      return NULL;
   }

   /* printf("Attaching shared memory, key: %x, segId: %d, p: %x\n", key, segId, pShm); */

   return (char *) pShm;
}

/*******************************************************************************
NAME:      IPTVosDetachSharedMemory
ABSTRACT:  Detach an area of shared memory used for globals in
           Linux multi-process systems.
RETURNS:   0 if OK, !=0 if not
*/
int IPTVosDetachSharedMemory(
                           char *pSharedMem) /* Pointer to shared memory */
{
   int ret;
   char errBuf[80];

   /* printf("Detaching shared memory, p: %x\n", pSharedMem); */

   /* Detach the shared memory segment.  */
   ret = shmdt(pSharedMem);
   if (ret != 0)
   {
      (void)strerror_r(errno,errBuf,sizeof(errBuf));
      IPTVosPrint2(IPT_ERR,"IPTVosDetachSharedMemory FAILED. shmdt Linux errno=%d %s\n",errno, errBuf);
      return (int)IPT_ERROR;
   }

   return 0;
}
#endif

#if defined(WIN32)
/*******************************************************************************
 NAME:      IPTVosCreateSharedMemory
 ABSTRACT:  Create an area of shared memory to be used for globals in
 Linux multi-process systems.
 RETURNS:   Pointer to memory if OK, NULL if not.
 */
char *IPTVosCreateSharedMemory(UINT32 size)   /* Size of shared memory area */
{
   LPVOID lpvMem = NULL;      // pointer to shared memory
   SECURITY_ATTRIBUTES security;

   // Create a named file mapping object

   IPTVosPrint0(IPT_ERR, "IPTVosCreateSharedMemory\n");

   getSecurityDescriptor(&security);

   hMapObject = CreateFileMapping(
                                  INVALID_HANDLE_VALUE,        // use paging file
                                  &security,                   // default security attributes
                                  PAGE_READWRITE | SEC_COMMIT, // read/write access
                                  0,                           // size: high 32-bits
                                  size,                        // size: low 32-bits
                                  winFile);                    // name of map object

   if (hMapObject == NULL)
   {
      IPTVosPrint1(IPT_ERR, "Could not create file mapping object (%d).\n", GetLastError());
      LocalFree(security.lpSecurityDescriptor);
      return NULL;
   }
   LocalFree(security.lpSecurityDescriptor);

   // Get a pointer to the file-mapped shared memory
   
   lpvMem = MapViewOfFileEx(
                          hMapObject,              // object to map view of
                          FILE_MAP_ALL_ACCESS,     // read/write access
                          0,                       // high offset:  map from
                          0,                       // low offset:   beginning
                          0,                       // default: map entire file
                          (LPVOID) 0x08000000);    // start address; should be far enough

   if (lpvMem == NULL) {
      IPTVosPrint1(IPT_ERR, "Could not map file object (%d).\n", GetLastError());
      return NULL;
   }
   
   // Initialize memory
   memset(lpvMem, '\0', size);
   
   return (char *) lpvMem;
}

/*******************************************************************************
 NAME:      IPTVosDestroySharedMemory
 ABSTRACT:  Destroy an area of shared memory used for globals in
 Linux multi-process systems.
 RETURNS:   -
 */
void IPTVosDestroySharedMemory(
                               char *pSharedMem) /* Pointer to shared memory */
{
   BOOL fIgnore;
   
   IPTVosPrint1(IPT_ERR, "Unmap shared memory at 0x%x\n", pSharedMem);

   /* Unmap shared memory from the process's address space */
   fIgnore = UnmapViewOfFile((LPVOID)pSharedMem);
   
   /* Close the process's handle to the file-mapping object */
   fIgnore = CloseHandle(hMapObject);

   hMapObject = NULL;
}

/*******************************************************************************
 NAME:      IPTVosAttachSharedMemory
 ABSTRACT:  Attach to an area of shared memory used for globals in
 Linux multi-process systems. The area must have been created before
 by IPTVosCreateSharedMemory.
 RETURNS:   Pointer to memory if OK, NULL if not.
 */
char *IPTVosAttachSharedMemory(void)
{
   LPVOID lpvMem = NULL;      // pointer to shared memory

   // Create a named file mapping object
   IPTVosPrint0(IPT_ERR, "IPTVosAttachSharedMemory\n");

   hMapObject = OpenFileMapping(
                                  FILE_MAP_ALL_ACCESS,   // read/write access
                                  FALSE,                 // no inherit handle
                                  winFile);              // name of map object
           
   if (hMapObject == NULL)
   {
      IPTVosPrint1(IPT_ERR, "Could not open map file object (%d).\n",
                   GetLastError());
      return NULL;
   }
   
   lpvMem = MapViewOfFileEx(
                            hMapObject,            // object to map view of
                            FILE_MAP_ALL_ACCESS,   // PAGE_READWRITE,   // read/write access
                            0,                     // high offset:  map from
                            0,                     // low offset:   beginning
                            0,                     // default: map entire file
                            (LPVOID) 0x08000000);
   
   if (lpvMem == NULL)
   {
      IPTVosPrint1(IPT_ERR, "Could not open map file object (%d).\n",
                                GetLastError());
      return NULL;
   }

   return (char *) lpvMem;
}

/*******************************************************************************
 NAME:      IPTVosDetachSharedMemory
 ABSTRACT:  Detach an area of shared memory used for globals in
 Linux multi-process systems.
 RETURNS:   0 if OK, !=0 if not
 */
int IPTVosDetachSharedMemory(char *pSharedMem) /* Pointer to shared memory */
{
   BOOL fIgnore;
   
   IPTVosPrint1(IPT_ERR, "IPTVosDetachSharedMemory at 0x%x\n", pSharedMem);
   
   /* Unmap shared memory from the process's address space */
   fIgnore = UnmapViewOfFile((LPVOID)pSharedMem);
   
   /* Close the process's handle to the file-mapping object */
   fIgnore = CloseHandle(hMapObject);
   
   hMapObject = NULL;
      
   return 0;
}
#endif

#endif
