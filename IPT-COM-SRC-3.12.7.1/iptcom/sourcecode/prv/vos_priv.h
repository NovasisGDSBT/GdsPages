/*******************************************************************************
 *  COPYRIGHT      : (c) 2006 Bombardier Transportation
 ********************************************************************************
 *  PROJECT        : IPTrain
 *
 *  MODULE         : vos_priv.h
 *
 *  ABSTRACT       : Virtual operating system (VOS) definitions
 *
 ********************************************************************************
 *  HISTORY     :
 *
 * $Id: vos_priv.h 35896 2015-03-10 13:53:59Z gweiss $
 *
 *  CR-9781 (Gerhard Weiss, 2015-03-10)
 *          Correction for GCC Compiler Error
 *
 *  CR-7241 (Gerhard Weiss, Bernd Loehr, 2014-02-10)
 *  CR-7240 (Gerhard Weiss, Bernd Loehr, 2014-02-10)
 *          Correction of UNICODE Macro for wsprinf
 *
 *  CR-3326 (Bernd Loehr, 2012-02-10)
 *           Improvement for 3rd party use / Darwin port added.
 *
 *  Internal (Bernd Loehr, 2010-08-16)
 * 			Old obsolete CVS history removed
 *
 *
 *
 *******************************************************************************/
#ifndef VOS_PRIV_H
#define VOS_PRIV_H

/*******************************************************************************
* INCLUDES */
#ifndef O_CSS
#include <stdio.h>      
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* DEFINES */
#define MEM_MAX_NBLOCKSIZES 100    /* Maximum number of different sizes of memory allocation blocks */
#define MEM_NBLOCKSIZES 13    /* Default Number of different sizes of memory allocation blocks */
#define MEM_MAX_PREALLOCATE   10    /* Max blocks to pre-allocate */
   /* Default Sizes of memory blocks */
#define MEM_BLOCKSIZES  {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072}
   /* Default. Pre-allocation of free memory blocks. To avoid problem with too many small blocks and no large. 
   Specify how many of each block size that should be pre-allocated (and freed!) */
#define MEM_PREALLOCATE {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 4}
	
#define IPT_MSG_BUF_TYPE 1

/* Windows XP */
#if defined(WIN32)
#define VOS_STARTER_TYPE unsigned __stdcall
#define SEMA_MAX_COUNT  100
#define QUEUE_MAX_COUNT 1000

/* LINUX */
#elif defined(LINUX) || defined(DARWIN)

/* VXWORKS */
#elif defined(VXWORKS)
#endif

#define LINUX_FILE_SIZE 256

#ifdef WIN32
  /* reentrant localtime wrapper called localtime_v */
  #ifndef localtime_v
    #if _MSC_VER < 1400
      /* not available in ms vs 7.0 and earlier */
      #define localtime_v(todArg, resArg) \
        { struct tm* pArg = localtime(todArg); \
          memcpy(resArg, pArg, sizeof(struct tm)); }
    #else
      /* use secure localtime_s available in ms vs 8.0 and later */
      #define localtime_v(todArg, resultArg) localtime_s(resultArg, todArg)
    #endif /* MSC_VER < 1400 */
  #endif /* localtime_v */
#endif

#if defined(VXWORKS)
/* CSS API definitions */
 #ifndef O_CSS
   typedef enum ENUM_IP_STATUS 
   { 
       IP_RUNNING, 
       IP_LOOPBACK, 
       IP_ERROR,
       IP_NO_STATUS, 
       IP_NO_NETWORK 
   } IP_STATUS_T;
   extern IP_STATUS_T ip_status_get(
       const char* if_name,
       int         timeout);
   extern int mon_broadcast_printf(const char *fmt, ... );
 #endif
#define MON_PRINTF mon_broadcast_printf
#else
#define MON_PRINTF printf
#endif

/*******************************************************************************
* TYPEDEFS */
/* Windows XP */
#if defined(WIN32)

/* LINUX */
#elif defined(LINUX) || defined(DARWIN)

typedef void *VOS_STARTER_TYPE;	

/* VXWORKS */
#elif defined(VXWORKS)
typedef int VOS_STARTER_TYPE;

/* INTEGRITY */
#elif defined(__INTEGRITY)
typedef int VOS_STARTER_TYPE;

#endif

struct memBlock
{
   UINT32 size;                  /* Size of the data part of the block */
   struct memBlock *pNext;       /* Pointer to next block in linked list */
                                 /* Data area follows here */                        
};
typedef struct memBlock MEM_BLOCK;

typedef struct 
{
   UINT32 freeSize;              /* Size of free memory */
   UINT32 minFreeSize;           /* Size of free memory */
   UINT32 allocCnt;              /* No of allocated memory blocks */
   UINT32 allocErrCnt;           /* No of allocated memory errors */
   UINT32 freeErrCnt;            /* No of free memory errors */
   UINT32 blockCnt[MEM_MAX_NBLOCKSIZES];   /* D:o per block size */

} MEM_STATISTIC;

typedef struct 
{
   IPT_SEM sem;                  /* Memory allocation semaphore */
   BYTE *pArea;                  /* Pointer to start of memory area */
   BYTE *pFreeArea;              /* Pointer to start of free part of memory area */
   UINT32 memSize;               /* Size of memory area */
   UINT32 allocSize;             /* Size of allocated area */   
   UINT32 noOfBlocks;            /* No of blocks */   

   /* Free block header array, one entry for each possible free block size */
   struct                       
   {
      UINT32 size;               /* Block size */
      MEM_BLOCK *pFirst;         /* Pointer to first free block */
   } freeBlock[MEM_MAX_NBLOCKSIZES]; 
   MEM_STATISTIC memCnt;       /* Statistic counters */
} MEM_CONTROL;

typedef struct 
{
   UINT32 queueAllocated;        /* No of allocated queues */
   UINT32 queueMax;              /* Maximum number of allocated queues */
   UINT32 queuCreateErrCnt;      /* No of queue create errors */
   UINT32 queuDestroyErrCnt;     /* No of queue destroy errors */
   UINT32 queuWriteErrCnt;       /* No of queue write errors */
   UINT32 queuReadErrCnt;        /* No of queue read errors */
} VOS_STATISTIC;

typedef struct 
{
   int QueueNo;                  /* Numeric identifier of queue */
   VOS_STATISTIC queueCnt;       /* Statistic counters */
} VOS_CONTROL;


/*******************************************************************************
* GLOBALS */

DLL_DECL extern UINT16 iptDebugMask;

/*******************************************************************************
* LOCALS */

/*******************************************************************************
* LOCAL FUNCTIONS */

/*******************************************************************************
* GLOBAL FUNCTIONS */

/* System */
#if defined(LINUX) || defined(DARWIN)
int IPTVosSystemStartup(char *path);
#else
int IPTVosSystemStartup(void);
#endif
void IPTVosSystemShutdown(void);

/* Memory */
int IPTVosMemInit(void);
int IPTVosMemDestroy(void);

/* Semaphore */
int IPTVosPutSemR(IPT_SEM *hSem);

/* Queue statistic */
void IPTVosQueueShow(void);

/* Shared memory for globals */
char *IPTVosCreateSharedMemory(UINT32 size);
void IPTVosDestroySharedMemory(char *pSharedMem);
char *IPTVosAttachSharedMemory(void);
int IPTVosDetachSharedMemory(char *pSharedMem);

void IPVosDateTimeString(char *pString);

BYTE *IPTVosMallocBuf(UINT32 size, UINT32 *pbufSize);
const char * IPTVosGetDebugFileName(void);
UINT16 IPTVosGetDebugFileIndex();

#ifdef __cplusplus
}
#endif

#endif /* VOS_PRIV_H */
