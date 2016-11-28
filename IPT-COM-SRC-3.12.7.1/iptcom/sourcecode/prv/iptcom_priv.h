/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2012 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     : IPTrain
 *
 *  MODULE      : iptcom_priv.h
 *
 *  ABSTRACT    : Private header file for iptcom
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 *  $Id: iptcom_priv.h 35885 2015-03-10 10:56:39Z gweiss $
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *           Findings from TUEV-Assessment
 *
 *  CR-3326 (Bernd Loehr, 2012-02-10)
 *           Improvement for 3rd party use / Darwin port added.
 *
 *  CR-1356 (Bernd Loehr, 2010-10-08)
 * 			Additional function iptConfigAddDatasetExt to define dataset
 *			dependent un/marshalling. Parameters for iptMarshallDSF ff changed.
 *
 *  CR-382 (Bernd Loehr, 2010-08-18) 
 * 			Avoid duplicate INT64 definitions
 *
 *  Internal (Bernd Loehr, 2010-08-13) 
 * 			Old obsolete CVS history removed
 *
 ******************************************************************************/

#ifndef IPTCOM_PRIV_H
#define IPTCOM_PRIV_H

/*******************************************************************************
*  INCLUDES */
#include "tdcSyl.h"        /* TDC System level definitions */
#include "tdcApi.h"        /* TDC interface routines */
#include "vos.h"   
#include "vos_priv.h"   
#include "mdcom.h"
#include "mdcom_priv.h"
#include "pdcom.h"
#include "netdriver.h"
#include "pdcom_priv.h"

#ifdef __cplusplus
extern "C" {
#endif
   
/*******************************************************************************
*  DEFINES */
#ifdef LINUX_MULTIPROC
#define IPTGLOBAL(x)  pIptGlobal->x
#else
#define IPTGLOBAL(x)  iptGlobal.x
#endif

/* Default communication parameters */
#define PD_REC_TIMEOUT_DEF 0    /* PD receive time out */
#define PD_REC_VAL_BEH_DEF 0    /* PD validity behavior */
#define PD_CYCL_TIME_DEF   100  /* PD send cycle time */

#define IPT_MARSHALL_VAR_SIZE +1  /* Return value if a dataset is of variable
                                     size */

#define IPT_TAB_ADDITEMS      1  /* No of items to add to table if more 
                                        space is needed */


/* Sizes of PD and MD header items used on the communication link */
#define TIMESTAMP_SIZE        4
#define PROT_VER_SIZE         4
#define TOPO_COUNT_SIZE       4
#define COMID_SIZE            4
#define TYPE_SIZE             2
#define DATA_LENGTH_SIZE      2
#define USER_STATUS_SIZE      2
#define HEAD_LENGTH_SIZE      2
#define SRC_URI_LENGTH_SIZE   1  
#define DEST_URI_LENGTH_SIZE  1
#define INDEX_SIZE            2
#define SEQ_NO_SIZE           2
#define MSG_LENGTH_SIZE       2
#define SESSION_ID_SIZE       4
#define RESPONSE_TIMEOUT_SIZE 4
#define DEST_IP_SIZE          4
#define FCS_SIZE              4U
   
#define MIN_MD_HEADER_SIZE (TIMESTAMP_SIZE + \
   PROT_VER_SIZE + \
   TOPO_COUNT_SIZE + \
   COMID_SIZE + \
   TYPE_SIZE + \
   DATA_LENGTH_SIZE + \
   USER_STATUS_SIZE + \
   HEAD_LENGTH_SIZE + \
   SRC_URI_LENGTH_SIZE + \
   DEST_URI_LENGTH_SIZE + \
   INDEX_SIZE + \
   SEQ_NO_SIZE + \
   MSG_LENGTH_SIZE + \
   SESSION_ID_SIZE + \
   RESPONSE_TIMEOUT_SIZE + \
   DEST_IP_SIZE + \
   FCS_SIZE)
   
   /* Header index common for MD and PD */
#define TIMESTAMP_OFF       0
#define PROT_VER_OFF        (TIMESTAMP_OFF + TIMESTAMP_SIZE)
#define TOPO_COUNT_OFF      (PROT_VER_OFF + PROT_VER_SIZE)
#define COMID_OFF           (TOPO_COUNT_OFF + TOPO_COUNT_SIZE)
#define TYPE_OFF            (COMID_OFF + COMID_SIZE)
#define DATA_LENGTH_OFF     (TYPE_OFF + TYPE_SIZE)
#define USER_STATUS_OFF     (DATA_LENGTH_OFF + DATA_LENGTH_SIZE)
#define HEAD_LENGTH_OFF     (USER_STATUS_OFF + USER_STATUS_SIZE)
   
   /* Header index for MD */
#define SRC_URI_LENGTH_OFF  (HEAD_LENGTH_OFF + HEAD_LENGTH_SIZE)
#define DEST_URI_LENGTH_OFF (SRC_URI_LENGTH_OFF + SRC_URI_LENGTH_SIZE)
#define INDEX_OFF           (DEST_URI_LENGTH_OFF + DEST_URI_LENGTH_SIZE)
#define SEQ_NO_OFF          (INDEX_OFF + INDEX_SIZE)
#define MSG_LENGTH_OFF      (SEQ_NO_OFF + SEQ_NO_SIZE)
#define SESSION_ID_OFF      (MSG_LENGTH_OFF + MSG_LENGTH_SIZE)
#define SRC_URI_OFF         (SESSION_ID_OFF + SESSION_ID_SIZE)
   
   /* Header index for PD */
#define RESERVED2_OFF       (HEAD_LENGTH_OFF + HEAD_LENGTH_SIZE)
#define HEAD_FCS_OFF        (RESERVED2_OFF + RESERVED2_SIZE)

/* Length of block in PD/MD before new FCS, in bytes */
#define IPT_BLOCK_LENGTH  256 

   /* Default communication parameters for PD */
#define PD_DEF_QOS   5
#define PD_DEF_TTL   64

   /* Default communication parameters for MD */
#define MD_DEF_QOS   3
#define MD_DEF_TTL   64

/*   Multicast IP addresses 239.0.0.0 to 239.255.255.255 */
#define MULTICAST_ADDR  ((UINT32)239*0x1000000)
#define MULTICAST_MASK  ((UINT32)255*0x1000000)

#define isMulticastIpAddr(ipAddr) ((ipAddr & MULTICAST_MASK) == MULTICAST_ADDR)

/* Check if a IP address is intra or inter consist */
#define isOwnConsistAddr(ipAddr) (ipAddrGetUnitNo(ipAddr) == 0)

#ifndef IPT_MAX_LABEL_LEN  
   #define IPT_MAX_LABEL_LEN  16
#endif

/*************** Task/performance default defines ******************************/
#define PD_SND_CYCLE  100                          /* PD send task cylce time */
#define MD_SND_CYCLE  100                          /* MD send task cylce time */
#define IPTCOM_SND_CYCLE  100                      /* IPTCOM send task cylce time */

#if defined(WIN32)
/* Win32 */

#define PD_SND_POLICY 0                            /* Not used */
#define PD_SND_PRIO   THREAD_PRIORITY_NORMAL       /* PD send task priority */
#define PD_SND_STACK  10000                        /* PD send task stack size */

#define PD_REC_POLICY 0                            /* Not used */
#define PD_REC_PRIO   THREAD_PRIORITY_ABOVE_NORMAL /* PD receive task priority */
#define PD_REC_STACK  10000                        /* PD receive task stack size */

#define MD_SND_POLICY 0                            /* Not used */
#define MD_SND_PRIO   THREAD_PRIORITY_NORMAL       /* MD send task priority */
#define MD_SND_STACK  10000                        /* MD send task stack size */

#define MD_REC_POLICY 0                            /* Not used */
#define MD_REC_PRIO   THREAD_PRIORITY_ABOVE_NORMAL /* MD receive task priority */
#define MD_REC_STACK  10000                        /* MD receive task stack size */

#define SNMP_REC_POLICY 0                            /* Not used */
#define SNMP_REC_PRIO   THREAD_PRIORITY_ABOVE_NORMAL /* PD receive task priority */
#define SNMP_REC_STACK  10000                        /* PD receive task stack size */

#define IPTCOM_SND_POLICY 0                        /* Not used */
#define IPTCOM_SND_PRIO   THREAD_PRIORITY_NORMAL   /* IPTCOM send task priority */
#define IPTCOM_SND_STACK  10000                    /* IPTCOM send task stack size */

#ifdef LINUX_MULTIPROC
#define NET_CTRL_POLICY 0                             /* Net control task attribute */
#define NET_CTRL_PRIO   THREAD_PRIORITY_ABOVE_NORMAL  /* Net control task priority */
#define NET_CTRL_STACK  10000                         /* Net control task stack size */
#endif

#elif defined(LINUX)
/* Linux */

/* Thread attributes SCHED_OTHER (for regular non-realtime scheduling) */
/* SCHED_RR (realtime round-robin policy), SCHED_FIFO (realtime FIFO policy) */

#define PD_SND_POLICY SCHED_RR                     /* PD send task attribute */
#define PD_SND_PRIO   15                           /* PD send task priority */
#define PD_SND_STACK  10000                        /* PD send task stack size */

#define PD_REC_POLICY SCHED_RR                     /* PD receive task attribute */
#define PD_REC_PRIO   20                           /* PD receive task priority */
#define PD_REC_STACK  10000                        /* PD receive task stack size */

#define MD_SND_POLICY SCHED_RR                     /* MD send task attribute */
#define MD_SND_PRIO   10                           /* MD send task priority */
#define MD_SND_STACK  10000                        /* MD send task stack size */

#define MD_REC_POLICY SCHED_RR                     /* MD receive task attribute */
#define MD_REC_PRIO   19                           /* MD receive task priority */
#define MD_REC_STACK  10000                        /* MD receive task stack size */

#define SNMP_REC_POLICY SCHED_RR                   /* PD receive task attribute */
#define SNMP_REC_PRIO   18                         /* PD receive task priority */
#define SNMP_REC_STACK  10000                      /* PD receive task stack size */

#define IPTCOM_SND_POLICY SCHED_RR                 /* IPTCOM send task attribute */
#define IPTCOM_SND_PRIO   15                       /* IPTCOM send task priority */
#define IPTCOM_SND_STACK  10000                    /* IPTCOM send task stack size */

#ifdef LINUX_MULTIPROC
#define NET_CTRL_POLICY SCHED_RR                   /* Net control task attribute */
#define NET_CTRL_PRIO   20                         /* Net control task priority */
#define NET_CTRL_STACK  10000                      /* Net control task stack size */
#endif

#elif defined(DARWIN)
    /* BSD  */
    
    /* Thread attributes SCHED_OTHER (for regular non-realtime scheduling) */
    /* SCHED_RR (realtime round-robin policy), SCHED_FIFO (realtime FIFO policy) */
    
#define PD_SND_POLICY SCHED_OTHER                     /* PD send task attribute */
#define PD_SND_PRIO   15                           /* PD send task priority */
#define PD_SND_STACK  8 * 1024                        /* PD send task stack size */
    
#define PD_REC_POLICY SCHED_OTHER                     /* PD receive task attribute */
#define PD_REC_PRIO   20                           /* PD receive task priority */
#define PD_REC_STACK  8 * 1024                        /* PD receive task stack size */
    
#define MD_SND_POLICY SCHED_OTHER                     /* MD send task attribute */
#define MD_SND_PRIO   10                           /* MD send task priority */
#define MD_SND_STACK  8 * 1024                        /* MD send task stack size */
    
#define MD_REC_POLICY SCHED_OTHER                     /* MD receive task attribute */
#define MD_REC_PRIO   19                           /* MD receive task priority */
#define MD_REC_STACK  8 * 1024                        /* MD receive task stack size */
    
#define SNMP_REC_POLICY SCHED_OTHER                   /* PD receive task attribute */
#define SNMP_REC_PRIO   18                         /* PD receive task priority */
#define SNMP_REC_STACK  8 * 1024                      /* PD receive task stack size */
    
#define IPTCOM_SND_POLICY SCHED_OTHER                 /* IPTCOM send task attribute */
#define IPTCOM_SND_PRIO   15                       /* IPTCOM send task priority */
#define IPTCOM_SND_STACK  8 * 1024                    /* IPTCOM send task stack size */
    
#ifdef LINUX_MULTIPROC
#define NET_CTRL_POLICY SCHED_OTHER                   /* Net control task attribute */
#define NET_CTRL_PRIO   20                         /* Net control task priority */
#define NET_CTRL_STACK  8 * 1024                      /* Net control task stack size */
#endif
    
#elif defined(VXWORKS)
/* VxWorks */

#define PD_SND_POLICY 0                            /* Not used */
#define PD_SND_PRIO   62                           /* PD send task priority */
#define PD_SND_STACK  10000                        /* PD send task stack size */

#define PD_REC_POLICY 0                            /* Not used */
#define PD_REC_PRIO   61                           /* PD receive task priority */
#define PD_REC_STACK  10000                        /* PD receive task stack size */

#define MD_SND_POLICY 0                            /* Not used */
#define MD_SND_PRIO   64                           /* MD send task priority */
#define MD_SND_STACK  10000                        /* MD send task stack size */

#define MD_REC_POLICY 0                            /* Not used */
#define MD_REC_PRIO   63                           /* MD receive task priority */
#define MD_REC_STACK  10000                        /* MD receive task stack size */

#define SNMP_REC_POLICY 0                          /* Not used */
#define SNMP_REC_PRIO   65                         /* MD receive task priority */
#define SNMP_REC_STACK  10000                      /* MD receive task stack size */

#define IPTCOM_SND_POLICY 0                        /* Not used */
#define IPTCOM_SND_PRIO   64                       /* IPTCOM send task priority */
#define IPTCOM_SND_STACK  10000                    /* IPTCOM send task stack size */

#elif defined(__INTEGRITY)
/* INTEGRITY */

#define PD_SND_POLICY 0                            /* Not used */
#define PD_SND_PRIO   193                          /* PD send task priority */
#define PD_SND_STACK  10000                        /* PD send task stack size */

#define PD_REC_POLICY 0                            /* Not used */
#define PD_REC_PRIO   194                          /* PD receive task priority */
#define PD_REC_STACK  10000                        /* PD receive task stack size */

#define MD_SND_POLICY 0                            /* Not used */
#define MD_SND_PRIO   191                          /* MD send task priority */
#define MD_SND_STACK  10000                        /* MD send task stack size */

#define MD_REC_POLICY 0                            /* Not used */
#define MD_REC_PRIO   192                          /* MD receive task priority */
#define MD_REC_STACK  10000                        /* MD receive task stack size */

#define SNMP_REC_POLICY 0                          /* Not used */
#define SNMP_REC_PRIO   190                        /* MD receive task priority */
#define SNMP_REC_STACK  10000                      /* MD receive task stack size */

#define IPTCOM_SND_POLICY 0                        /* Not used */
#define IPTCOM_SND_PRIO   191                      /* IPTCOM send task priority */
#define IPTCOM_SND_STACK  10000                    /* IPTCOM send task stack size */

#endif

/*******************************************************************************
*  TYPEDEFS */

   /* Definition of 64 bit data */
#ifndef INT64
    #ifndef O_CSS
       #if defined (VXWORKS)
          typedef signed    long long   T_IPT_INT64;
          typedef unsigned  long long   T_IPT_UINT64; 
       #else
          #if defined (LINUX) || defined(DARWIN)
             typedef signed    long long   T_IPT_INT64;
             typedef unsigned  long long   T_IPT_UINT64; 
          #else
             #if defined (WIN32)
                typedef signed    __int64  T_IPT_INT64;
                typedef unsigned  __int64  T_IPT_UINT64;
             #else    
                #if defined (__INTEGRITY)
                   typedef signed    long long  T_IPT_INT64;
                   typedef unsigned  long long  T_IPT_UINT64; 
                #else
                   #error "Either INTEGRITY, VXWORKS, LINUX or WIN32 has to be specified"
                #endif
             #endif
          #endif
       #endif
    #endif
#endif

#if !defined (INT64)
   #define INT64              T_IPT_INT64
#endif

#if !defined (UINT64)
   #define UINT64             T_IPT_UINT64
#endif

/* Alignment (used for Marshalling) */

#if defined(WIN32)
#define ALIGNOF(type) __alignof(type)
#define ALIGN(size) __declspec(align(size))
#else
#define ALIGNOF(type) __alignof__(type)
#define ALIGN(size) __attribute__((aligned(size)))
#endif

/* Special UINT64 type for detecting alignment in structs */
typedef struct { UINT64 U; } UINT64ST;

   
   /* TDC simulator */
#define TDCSIM_MAX_NO_OF_HOSTS 1000
#define TDCSIM_MAX_LINE_LEN    500
#define TDCSIM_SIZEOF_URI_BUF  20000

typedef struct tdcsimiptouriitem
{
   UINT32    IPaddr;
   const char      *pURI;
   T_TDC_BOOL is_Frg;       

} TDCSIM_IP_TO_URI_ITEM;

typedef struct tdcsimuritoipitem
{
   char *pURI;
} TDCSIM_URI_TO_IP_ITEM;

typedef struct
{
   int enableTDCSimulation;      /* <> 0 if TDC should be simulated */
   char hostFilePath[IPT_SIZE_OF_FILEPATH+1];
   int tdcSimNoOfHosts;   /* No of hosts */
   char *uriBufCurPos;
   
   /* Cross reference between IP and URI */
   TDCSIM_IP_TO_URI_ITEM tdcSimIPToURI[TDCSIM_MAX_NO_OF_HOSTS];
   TDCSIM_IP_TO_URI_ITEM tdcSimURIToIP[TDCSIM_MAX_NO_OF_HOSTS];
   char URIBuffer[TDCSIM_SIZEOF_URI_BUF];
} TDCSIM_CONTROL;

/* Table functions */
typedef struct
{
   UINT32 key;          /* Key for this table item */
   /* Here comes the data of the item */
} IPT_TAB_ITEM_HDR;

typedef struct 
{
   UINT16 initialized;  /* True if table is initialized */
   UINT16 nItems;       /* No of items in table */
   UINT16 maxItems;     /* Current max no of items in table */
   UINT16 itemSize;     /* Size of each item in table, in bytes */
   IPT_TAB_ITEM_HDR *pTable; /* Pointer to table items, dynamically allocated */
   UINT32 tableSize;    /* Current table size */
} IPT_TAB_HDR;

/* Table functions */
typedef struct
{
   UINT32 key1;          /* Key for this table item */
   UINT32 key2;          /* Key for this table item */
   /* Here comes the data of the item */
} IPT_TAB2_ITEM_HDR;

typedef struct 
{
   UINT16 initialized;  /* True if table is initialized */
   UINT16 nItems;       /* No of items in table */
   UINT16 maxItems;     /* Current max no of items in table */
   UINT16 itemSize;     /* Size of each item in table, in bytes */
   IPT_TAB2_ITEM_HDR *pTable; /* Pointer to table items, dynamically allocated */
   UINT32 tableSize;    /* Current table size */
} IPT_TAB2_HDR;

typedef struct
{
   char   labelName[IPT_MAX_HOST_URI_LEN+1];
} URI_LABEL_TAB_ITEM;

typedef struct 
{
   UINT16 initialized;      /* True if table is initialized */
   UINT16 nItems;           /* No of items in table */
   UINT16 maxItems;         /* Current max no of items in table */
   UINT16 itemSize;     /* Size of each item in table, in bytes */
   URI_LABEL_TAB_ITEM *pTable; /* Pointer to table items, dynamically allocated */
   UINT32 tableSize;        /* Current table size */
} URI_LABEL_TAB_HDR;

typedef struct
{
   char   labelName1[IPT_MAX_LABEL_LEN+1];
   char   labelName2[IPT_MAX_LABEL_LEN+1];
} URI_LABEL_TAB2_ITEM;

typedef struct 
{
   UINT16 initialized;      /* True if table is initialized */
   UINT16 nItems;           /* No of items in table */
   UINT16 maxItems;         /* Current max no of items in table */
   UINT16 itemSize;     /* Size of each item in table, in bytes */
   URI_LABEL_TAB2_ITEM *pTable; /* Pointer to table items, dynamically allocated */
   UINT32 tableSize;        /* Current table size */
} URI_LABEL_TAB2_HDR;

/* Table functions */
typedef struct
{
   UINT32 key1;          /* Key for this table item */
   char   labelName[IPT_MAX_HOST_URI_LEN+1];
   /* Here comes the data of the item */
} IPT_TAB_URI_ITEM;

typedef struct 
{
   UINT16 initialized;  /* True if table is initialized */
   UINT16 nItems;       /* No of items in table */
   UINT16 maxItems;     /* Current max no of items in table */
   UINT16 itemSize;     /* Size of each item in table, in bytes */
   IPT_TAB_URI_ITEM *pTable; /* Pointer to table items, dynamically allocated */
   UINT32 tableSize;    /* Current table size */
} IPT_TAB_URI_HDR;

/* Table functions */
typedef struct
{
   UINT32 key1;          /* Key for this table item */
   UINT32 key2;          /* Key for this table item */
   char   labelName[IPT_MAX_HOST_URI_LEN+1];
   /* Here comes the data of the item */
} IPT_TAB2_URI_ITEM;

typedef struct 
{
   UINT16 initialized;  /* True if table is initialized */
   UINT16 nItems;       /* No of items in table */
   UINT16 maxItems;     /* Current max no of items in table */
   UINT16 itemSize;     /* Size of each item in table, in bytes */
   IPT_TAB2_URI_ITEM *pTable; /* Pointer to table items, dynamically allocated */
   UINT32 tableSize;    /* Current table size */
} IPT_TAB2_URI_HDR;

/* Declaration of configuration database tables. Start with uninitialized tables */
typedef struct
{
   IPT_SEM sem;                 /* Semaphore for config DB protection */
   IPT_TAB_HDR exchgParTable;
   IPT_TAB_HDR pdSrcFilterParTable;
   IPT_TAB_HDR destIdParTable;
   IPT_TAB_HDR datasetTable;
   IPT_TAB_HDR comParTable;
   IPT_TAB_HDR fileTable;     /* Table with config files */
   int finish_addr_resolv;    /* Flag used to indicate that address 
                                 resolving shall be done when TDC is ready */
#if defined(IF_WAIT_ENABLE)
   int finish_socket_creation;/* Flag used to indicate that MD sockets has to
                                 created when the ethernet interface is initiated */
#endif
} CONFIG_DB;

typedef struct 
{
#ifdef TARGET_SIMU
   IPT_TAB2_HDR pdJoinedMcAddrTable; /* Table with joined multicast groups for PD*/
   IPT_TAB2_HDR mdJoinedMcAddrTable; /* Table with joined multicast groups for MD*/
#else
   IPT_TAB_HDR pdJoinedMcAddrTable; /* Table with joined multicast groups for PD*/
   IPT_TAB_HDR mdJoinedMcAddrTable; /* Table with joined multicast groups for MD*/
#endif
   IPT_QUEUE netCtrlQueueId;
} NET_CONTROL;

/* Structure that always should start the shared memory globals */
typedef struct 
{
   int segId;              /* Segment ID, returned from Linux */
   char *shmAddr;          /* Shared memory address, returned from Linux */
} IPT_SHARED_MEMORY_CB;

typedef struct 
{
   int pdRecPriority;      /* Priority for PD receive thread/task */
   int mdRecPriority;      /* Priority for MD receive thread/task */
   int snmpRecPriority;    /* Priority for SNMP receive thread/task */
   int pdProcCycle;        /* Cycle time for PD send thread/task */
   int pdProcPriority;     /* Priority for PD send thread/task */
   int mdProcCycle;        /* Cycle time for MD send thread/task */
   int mdProcPriority;     /* Priority for MD send thread/task */
   int iptComProcCycle;    /* Cycle time for IPTCom send thread/task */
   int iptComProcPriority; /* Priority for IPTCom send thread/task */
#ifdef LINUX_MULTIPROC
   int netCtrlPriority;    /* Priority for Net control thread/task */
#endif
} THREAD_CONTROL;

typedef struct
{
   UINT32 filterId;    /* Source filter ID*/
   char *pSourceURI;   /* Pointer to source filter URI string */
} IPT_CONFIG_SRC_FILTER_PAR;

typedef struct
{
   UINT32 comId;           /* Com ID */
   IPT_TAB_HDR *pFiltTab;  /* Pointer to source filter table */
} IPT_CONFIG_COMID_SRC_FILTER_PAR;
   
typedef struct
{
   UINT32 destId;    /* Destination ID*/
   char *pDestURI;   /* Pointer to destination URI string */
} IPT_CONFIG_DEST_ID_PAR;

typedef struct
{
   UINT32 comId;             /* Com ID */
   IPT_TAB_HDR *pDestIdTab;  /* Pointer to destination ID table */
} IPT_CONFIG_COMID_DEST_ID_PAR;
   
/* Statistic control block*/
typedef struct
{
   IPT_SEM sem;                 /* Semaphore for statistic OID table protection */
   IPT_TAB_HDR oidTable;
} STAT_CONTROL;


typedef struct
{
   char   hostName[IPT_MAX_HOST_URI_LEN+1]; /* Destination URI */
   UINT32 destIpAddr;                       /* Destination IP address */
} HOST_URI_ITEM;

typedef struct
{
   const void   *pRedFuncRef;      /* Redundant function ref */
} FRG_REF_ITEM;

typedef struct listnerQueue
{
   struct listnerQueue *pNext;
   struct listnerQueue *pPrev;
   MD_QUEUE        listenerQueueId;  /* Listeners queue Id */
   const void      *pCallerRef;      /* Listeners caller reference */
   UINT32          lastRecMsgNo;     /* Last message no put on the queue */
   UINT32          noOfListener;     /* No of listeners instances for this item */
} LISTENER_QUEUE;

typedef struct listnerFunc
{
   struct listnerFunc *pNext;
   struct listnerFunc *pPrev;
   IPT_REC_FUNCPTR func;             /* Listeners call-back function */
   const void      *pCallerRef;      /* Listeners caller reference */
   UINT32          lastRecMsgNo;     /* Last message no put on the queue */
   UINT32          noOfListener;     /* No of listeners instances for this item */
} LISTENER_FUNC;

typedef struct queueList
{
   struct queueList *pNext;
   LISTENER_QUEUE   *pQueue;
   UINT32           destIpAddr;       /* Destination IP address */
   UINT32           comId;            /* Communication ID */
   UINT32           destId;           /* Destination ID */
   char             *pDestUri;        /* Pointer to destination URI */
} QUEUE_LIST;

typedef struct queueFrgList
{
   struct queueFrgList *pNext;
   const void          *pRedFuncRef;     /* Redundant function ref */
   LISTENER_QUEUE      *pQueue;
   UINT32              destIpAddr;       /* Destination IP address */
   UINT32              comId;            /* Communication ID */
   UINT32              destId;           /* Destination ID */
   char                *pDestUri;        /* Pointer to destination URI */
} QUEUE_FRG_LIST;

typedef struct funcList
{
   struct funcList *pNext;
   LISTENER_FUNC   *pFunc;
   UINT32          destIpAddr;       /* Destination IP address */
   UINT32          comId;            /* Communication ID */
   UINT32          destId;           /* Destination ID */
   char            *pDestUri;        /* Pointer to destination URI */
} FUNC_LIST;

typedef struct funcFrgList
{
   struct funcFrgList *pNext;
   const void         *pRedFuncRef;      /* Redundant function ref */
   LISTENER_FUNC      *pFunc;
   UINT32             destIpAddr;       /* Destination IP address */
   UINT32             comId;            /* Communication ID */
   UINT32             destId;           /* Destination ID */
   char               *pDestUri;        /* Pointer to destination URI */
} FUNC_FRG_LIST;


typedef struct
{
   QUEUE_LIST     *pQueueList;
   QUEUE_FRG_LIST *pQueueFrgList;
   FUNC_LIST      *pFuncList;
   FUNC_FRG_LIST  *pFuncFrgList;
   int             counted;          /* flag set when a received msg is added to mdInPackets */
   UINT32          mdInPackets;      /* No of received packets for nrmal listeners*/
   int             frgCounted;       /* flag set when a received msg is added to mdFrgInPackets */
   UINT32          mdFrgInPackets;   /* No of received packets for FRG listeners*/
} LISTENER_LISTS;

typedef struct
{
   IPT_TAB_HDR comidListTableHdr;          /* Table header for table with ComId listeners (COMID_ITEM) */
   URI_LABEL_TAB2_HDR instXFuncNListTableHdr;         /* Table header for table with ComId listeners (INSTX_FUNCN_ITEM) */
   URI_LABEL_TAB_HDR aInstFuncNListTableHdr;          /* Table header for table with ComId listeners (AINST_FUNCN_ITEM) */
   URI_LABEL_TAB_HDR instXaFuncListTableHdr;          /* Table header for table with ComId listeners (INSTX_AFUNC_ITEM) */
   LISTENER_LISTS aInstAfunc;           /* Table with listeners of all instances of all functions */
} LISTERNER_TABLES;

#ifdef TARGET_SIMU            
typedef struct
{
   UINT32 simuIpAddr;       /* Destination ID */
   char   simUri[IPT_MAX_HOST_URI_LEN+1]; /* Simulated device host URI */
   LISTERNER_TABLES listTables;
} SIMU_DEV_ITEM;
#endif

typedef struct
{
   UINT32         keyComId;         /* ComId */
   LISTENER_LISTS lists;
} COMID_ITEM;

typedef struct
{
   char   instName[IPT_MAX_LABEL_LEN+1]; /* URI instance name */
   char   funcName[IPT_MAX_LABEL_LEN+1]; /* URI funtion name */
   LISTENER_LISTS lists;
} INSTX_FUNCN_ITEM;

typedef struct
{
   char   funcName[IPT_MAX_LABEL_LEN+1]; /* URI funtion name */
   LISTENER_LISTS lists;
} AINST_FUNCN_ITEM;

typedef struct
{
   char   instName[IPT_MAX_LABEL_LEN+1]; /* URI funtion name */
   LISTENER_LISTS lists;
} INSTX_AFUNC_ITEM;

typedef struct 
{
   int mdComInitiated;                     /* Flag set when MDCom is initiated */
   IPT_QUEUE mdSendQueueId;                /* Queue for queue with messages to be send */
   IPT_SEM listenerSemId;                  /* Semaphore for linked lists of registered listeners */
   LISTENER_QUEUE *pFirstListenerQueue;
   LISTENER_FUNC  *pFirstListenerFunc;
   IPT_TAB_HDR  frgTableHdr;               /* Table header for table with registred functions references for FRG listeners (FRG_ITEM) */
   IPT_TAB_HDR  queueTableHdr;             /* Table header for table with created queues (MD_QUEUE_ITEM) */
#ifdef TARGET_SIMU            
   IPT_TAB_URI_HDR simuDevListTableHdr;          /* Table header for table with ComId listeners (SIMU_DEV_ITEM) for
                                                      listeners of all instances of all functions */
#endif
   LISTERNER_TABLES listTables;            /* Tables with listeners */
   REM_QUEUE_ITEM *pFirstRemQueue;         /* Pointer to first queue to be removed */
   IPT_SEM remQueueSemId;                  /* Semaphore for list of queues to be deleted */
   
   int seInitiated;                        /* Flag set when session layer is initiated */
   SESSION_INSTANCE *pFirstSeInstance;     /* Pointer to first element in the linked list of active session instances */
   SESSION_INSTANCE *pLastSeInstance;      /* Pointer to last  element in the linked list of active session instances */
   int sessionId;                          /* Current value of session ID used in MD message header */
   IPT_SEM seSemId;                        /* Semaphore for linked list of active session instances */
   
   int trInitiated;                        /* Flag set when transport layer is initiated */
   TRANSPORT_INSTANCE *pFirstTrInstance;   /* Pointer to first element in the linked list of active transport instances */
   TRANSPORT_INSTANCE *pLastTrInstance;    /* Pointer to last  element in the linked list of active transport instances */
   IPT_SEM trListSemId;                    /* Semaphore for linked list of active transport instance */
   REC_SEQ_CNT *pFirstRecSeqCnt;           /* Pointer to first element in the linked list of received sequence number for unicast messages */
   REC_FRG_SEQ_CNT *pFirstFrgRecSeqCnt;    /* Pointer to first element in the linked list of received sequence number for FRG multicast messages */
   SEND_SEQ_CNT *pFirstSendSeqCnt;         /* Pointer to first element in the linked list of destination IP addresses used for send sequence number */
   IPT_SEM SendSeqSemId;                   /* Semaphore for linked list of IP address used for the send sequence counters lists */
   UINT8 lastValidRecTopoCnt;              /* Last valid value of topo counter used for the receive sequence counters lists */
   UINT8 lastSendTopoCnt;                  /* Last value of topo counter used for the send sequence counters lists */
   int finish_addr_resolv;                 /* Flag used to indicate that address resolving shall be done when TDC is ready */
   UINT32 defAckTimeOut;                   /* Default acknowledge time-out time */
   UINT32 defResponseTimeOut;              /* Default response time-out time */
   UINT16 maxStoredSeqNo;                  /* Maximum of stored received sequence number per IP address */
   MD_STATISTIC mdCnt;                     /* Statistic counters */
} MD_CONTROL;

typedef struct 
{
   struct pdSendNetCB *pFirstSendNetCB; /* Pointer to first send CB */
   struct pdRecNetCB *pFirstRecNetCB;   /* Pointer to first receive CB */
   PUBGRPTAB pubSchedGrpTab;            /* Publish schedule group table */
   SUBGRPTAB subSchedGrpTab;            /* Publish schedule group table */
   RECTAB recTab;                       /* Receive look up table */
   struct pdNotResPubCB *pFirstNotResPubCB;   /* Pointer to first not resolved
                                                 publish CB */
   struct pdNotResSubCB *pFirstNotResSubCB;   /* Pointer to first not resolved
                                                 subscribe CB */
   int netCBchanged;                    /* Flag indicating that net CB has been changed */ 
   IPT_TAB_HDR  sendTableHdr;           /* Table header for table with PD send cycle items  (PD_CYCLE_ITEM) */
   UINT32 leaderDev;                    /* Leader/follower status of the whole device */
   IPT_TAB_HDR  redIdTableHdr;          /* Table header for table with PD redundant item items  (PD_RED_ID_ITEM) */
   IPT_SEM sendSem;                     /* Semaphore to protect PD send tables */
   IPT_SEM recSem;                      /* Semaphore to protect PD receive tables */
   int finishSendAddrResolv;            /* Flag used to indicate that address
                                           resolving shall be done when TDC is
                                           ready */
   int finishRecAddrResolv;             /* Flag used to indicate that address
                                           resolving shall be done when TDC is
                                           ready */
   UINT32 defInvalidBehaviour;          /* Default behaviour when invalid,
                                           PD_INVALID_* */
   UINT32 defTimeout;                   /* Default Timeout before considering
                                           invalid, in ms */
   UINT32 defCycle;                     /* Default cycle time when to distribute
                                           in ms */
   PD_STATISTIC pdCnt;                  /* Statistic counters */
} PD_CONTROL;

/* Global data area. All globals and statics must be put here! */   
typedef struct
{
   IPT_SHARED_MEMORY_CB shmCB;   /* Shared memory info block in Linux, must be first! */
   int iptComInitiated;          /* Bolean set when IPTCom is initated */
   char buildTime[30];           /* Build date and time */
   MD_CONTROL md;                /* MD control block */
   PD_CONTROL pd;                /* PD control block */
   MEM_CONTROL mem;              /* Memory control block */
   VOS_CONTROL vos;              /* VOS control block */
   NET_CONTROL net;              /* NET control block */
   TDCSIM_CONTROL tdcsim;        /* TDC simulator */
   CONFIG_DB configDB;           /* Configuration database */
   STAT_CONTROL stat;            /* Statistic control block */
   THREAD_CONTROL task;          /* Thread/task control block */
   int disableMarshalling;       /* Set by public method IPTCom_disableMarshalling */
   int enableFrameSizeCheck;     /* Set by public method IPTCom_enableFrameSizeCheck */
   int systemShutdown;           /* Terminate application if this variable is set to 1 */
#if defined(IF_WAIT_ENABLE)
   int ifWaitRec;                /* Flag set if communication has to wait for the ethernet interface to be ready */
   int ifWaitSend;               /* Flag set if communication has to wait for the ethernet interface to be ready */
   int ifRecReadyPD;             /* Flag set when the ethernet interface for receiving PD is ready */
   int ifRecReadyMD;             /* Flag set when the ethernet interface for receiving MD is ready */
#endif
   UINT32 ownIpAddress;          /* Own IP address */
   UINT32 iptcomStarttime;       /* Start time for IPTCom */
   UINT32 topoCnt;               /* Local copy of topo counter. Updated by the TDC */
} IPT_GLOBAL;

/*******************************************************************************
* GLOBALS */

#ifdef LINUX_MULTIPROC
extern IPT_GLOBAL *pIptGlobal;
#else
extern IPT_GLOBAL iptGlobal;
#endif
extern int iptEnableTDCSimulation;      /* <> 0 if TDC should be simulated */

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

void IPTCom_send(void);

/* FCS methods */
UINT32 iptCalcSendBufferSize(UINT32 srcSize);
UINT32 iptCalcReceiveBufferSize(UINT32 srcSize);
int iptLoadSendData(const BYTE *pSrc, UINT32 srcSize, BYTE *pDst, UINT32 *pDstSize);
int iptLoadReceiveDataFCS(BYTE *pSrc, UINT32 srcSize, BYTE *pDst, UINT32 *pDstSize);
void iptAddDataFCS(BYTE *pSrc, UINT32 srcSize);
int iptCheckFCS(BYTE *pSrc, UINT32 srcSize);
int iptCalcGetDatasetSize(UINT32 datasetId, BYTE *pSrc, UINT32 *pSize, IPT_CFG_DATASET_INT **ppDataset);

int iptPrepareDataset(UINT32 datasetId);
int iptPrepareAllDataset(void);

void iptAlign(BYTE **p, unsigned int size);
void iptAlignStruct(BYTE **p, unsigned int size);
void iptAlignZero(BYTE **p, unsigned int size);
void iptAlignStructZero(BYTE **p, unsigned int size);

/*lint -save -esym(429, pDst) pDst is not custotory */
int iptMarshallDSInt(UINT8 condition, UINT32 datasetId, const unsigned char *pSrc, unsigned char *pDst, UINT32 *pDstSize);
int iptMarshallDSF(UINT32 nLines, UINT16 alignment, UINT8 disableMarshalling, IPT_DATA_SET_FORMAT_INT *pFormat, const BYTE *pSrc, BYTE *pDst, UINT32 *pDstSize);
int iptUnmarshallDSInt(UINT8 condition, UINT32 datasetId, unsigned char *pSrc, unsigned char *pDst, UINT32 *pDstSize);
int iptUnmarshallDSF(UINT32 nLines, UINT16 alignment, UINT8 disableMarshalling, IPT_DATA_SET_FORMAT_INT *pFormat, BYTE *pSrc, BYTE *pDst,  UINT32 *pDstSize);
/*lint -restore*/
   
/* Table functions */
int iptTabInit(IPT_TAB_HDR *pTab, UINT16 itemSize);
int iptTab2Init(IPT_TAB2_HDR *pTab, UINT16 itemSize);
int iptUriLabelTabInit(URI_LABEL_TAB_HDR *pTab, UINT16 itemSize);
int iptUriLabelTab2Init(URI_LABEL_TAB2_HDR *pTab, UINT16 itemSize);
int iptTabUriInit(IPT_TAB_URI_HDR *pTab, UINT16 itemSize);
int iptTab2UriInit(IPT_TAB2_URI_HDR *pTab, UINT16 itemSize);

void iptTabTerminate(IPT_TAB_HDR *pTab);
void iptTab2Terminate(IPT_TAB2_HDR *pTab);
void iptUriLabelTabTerminate(URI_LABEL_TAB_HDR *pTab);
void iptUriLabelTab2Terminate(URI_LABEL_TAB2_HDR *pTab);
void iptTabUriTerminate(IPT_TAB_URI_HDR *pTab);
void iptTab2UriTerminate(IPT_TAB2_URI_HDR *pTab);

int iptTabExpand(IPT_TAB_HDR *pTab);

int iptTabAdd(IPT_TAB_HDR *pTab, const IPT_TAB_ITEM_HDR *pItem);
int iptTab2Add(IPT_TAB2_HDR *pTab,const IPT_TAB2_ITEM_HDR *pNewItem);
int iptUriLabelTabAdd(URI_LABEL_TAB_HDR *pTab, const URI_LABEL_TAB_ITEM *pNewItem);
int iptUriLabelTab2Add(URI_LABEL_TAB2_HDR *pTab, const URI_LABEL_TAB2_ITEM *pNewItem);
int iptTabUriAdd(IPT_TAB_URI_HDR *pTab,const IPT_TAB_URI_ITEM *pNewItem);
int iptTab2UriAdd(IPT_TAB2_URI_HDR *pTab,const IPT_TAB2_URI_ITEM *pNewItem);

int iptTabDelete(IPT_TAB_HDR *pTab, UINT32 key);
int iptTab2Delete(IPT_TAB2_HDR *pTab, UINT32 key1, UINT32 key2);
int iptUriLabelTabDelete(URI_LABEL_TAB_HDR *pTab,char *pKey);
int iptUriLabelTab2Delete(URI_LABEL_TAB2_HDR *pTab,char *pKey1, char *pKey2);
int iptTabUriDelete(IPT_TAB_URI_HDR *pTab, UINT32 key1, const char *pKey2);
int iptTab2UriDelete(IPT_TAB2_URI_HDR *pTab, UINT32 key1, UINT32 key2, const char *pKey3);

IPT_TAB_ITEM_HDR *iptTabFind(const IPT_TAB_HDR *pTab, UINT32 key);
IPT_TAB2_ITEM_HDR *iptTab2Find(const IPT_TAB2_HDR *pTab, UINT32 key1, UINT32 key2);
URI_LABEL_TAB_ITEM *iptUriLabelTabFind(const URI_LABEL_TAB_HDR *pTab, const char *pKey);
URI_LABEL_TAB2_ITEM *iptUriLabelTab2Find(const URI_LABEL_TAB2_HDR *pTab, const char *pKey1, const char *pKey2);
IPT_TAB_URI_ITEM *iptTabUriFind(const IPT_TAB_URI_HDR *pTab, UINT32 key1, const char  *pKey2);
IPT_TAB2_URI_ITEM *iptTab2UriFind(const IPT_TAB2_URI_HDR *pTab, UINT32 key1,UINT32 key2, const char  *pKey3);

IPT_TAB_ITEM_HDR *iptTabFindNext(const IPT_TAB_HDR *pTab, UINT32 key);
IPT_TAB2_ITEM_HDR *iptTab2FindNext(const IPT_TAB2_HDR *pTab, UINT32 key1, UINT32 key2);
URI_LABEL_TAB_ITEM *iptUriLabelTabFindNext(const URI_LABEL_TAB_HDR *pTab,const char *pKey);
URI_LABEL_TAB2_ITEM *iptUriLabelTab2FindNext(const URI_LABEL_TAB2_HDR *pTab,const char *pKey1,const char *pKey2);
IPT_TAB_URI_ITEM *iptTabUriFindNext(const IPT_TAB_URI_HDR *pTab, UINT32 key1, const char  *pKey2);
IPT_TAB2_URI_ITEM *iptTab2UriFindNext(const IPT_TAB2_URI_HDR *pTab, UINT32 key1, UINT32 key2, const char  *pKey3);

/* Configuration functions */
int iptConfigInit(void);
void iptConfigDestroy(void);
#if defined(LINUX) || defined(DARWIN)
   int iptConfigParseXMLInit(const char *path, UINT32 *pMemSize, UINT32 *pIfWait, char *pLinuxFile);
#else
   int iptConfigParseXMLInit(const char *path, UINT32 *pMemSize, UINT32 *pIfWait);
#endif
int iptConfigParseXML(const char *path);
int iptConfigParseOwnUriXML(const char *path);
int iptAddConfig(const char *pXMLPath);
int iptConfigParseXML4DevConfig(const char *path);
int iptConfigParseXML4Dbg(const char *path);
int iptConfigGetExchgPar(UINT32 comId, IPT_CONFIG_EXCHG_PAR_EXT *pExchgPar);
int iptConfigGetPdSrcFiltPar(UINT32 comId, UINT32 filterId, const char **ppSourceUri);
int iptConfigGetDestIdPar(UINT32 comId, UINT32 destId, const char **ppDestUri);
int iptConfigGetComPar(UINT32 comParId, IPT_CONFIG_COM_PAR_EXT *pComPar);
IPT_CFG_DATASET_INT *iptConfigGetDataset(UINT32 datasetId);
void iptFinishConfig(void);

/* Topo counter functions */
#ifdef TARGET_SIMU
int iptCheckRecTopoCnt (UINT32 recTopoCnt, UINT32 simuIpAddr);
#else
int iptCheckRecTopoCnt (UINT32 recTopoCnt);
#endif

/* Statistics functions */
int iptStatGet(IPT_STAT_DATA *pStatData);
int iptStatGetNext(IPT_STAT_DATA *pStatData);
int iptStatSet(IPT_STAT_DATA *pStatData);

/* TDC simulation functions */
int tdcSimInit (char *filePath);
void tdcSimDestroy (void);
T_TDC_RESULT tdcSimGetAddrByName(const T_IPT_URI uri, T_IPT_IP_ADDR *pIpAddr, UINT8 *pTopoCnt);
T_TDC_RESULT tdcSimGetAddrByNameExt(const T_IPT_URI uri, T_IPT_IP_ADDR *pIpAddr, T_TDC_BOOL *pFrg, UINT8 *pTopoCnt);
T_TDC_RESULT tdcSimGetUriHostPart (T_IPT_IP_ADDR ipAddr, T_IPT_URI uri, UINT8 *pTopoCnt);
T_TDC_RESULT tdcSimGetOwnIds(T_IPT_LABEL devId, T_IPT_LABEL carId, T_IPT_LABEL cstId);

T_TDC_RESULT iptGetAddrByName(const T_IPT_URI uri, T_IPT_IP_ADDR *pIpAddr, UINT8 *pTopoCnt);
T_TDC_RESULT iptGetAddrByNameExt(const T_IPT_URI uri, T_IPT_IP_ADDR *pIpAddr, T_TDC_BOOL *pFrg, UINT8 *pTopoCnt);
T_TDC_RESULT iptGetUriHostPart (T_IPT_IP_ADDR ipAddr, T_IPT_URI uri, UINT8 *pTopoCnt);
#ifdef TARGET_SIMU
T_TDC_RESULT iptGetAddrByNameSim(const T_IPT_URI uri, UINT32 simuIpAddr, T_IPT_IP_ADDR *pIpAddr, UINT8 *pTopoCnt);
T_TDC_RESULT iptGetAddrByNameExtSim(const T_IPT_URI uri, UINT32 simuIpAddr, T_IPT_IP_ADDR *pIpAddr, T_TDC_BOOL *pFrg, UINT8 *pTopoCnt);
T_TDC_RESULT iptGetUriHostPartSim (T_IPT_IP_ADDR ipAddr, UINT32 simuIpAddr, T_IPT_URI uri, UINT8 *pTopoCnt);
#endif
T_TDC_RESULT iptGetOwnIds(T_IPT_LABEL devId, T_IPT_LABEL carId, T_IPT_LABEL cstId);
#ifdef TARGET_SIMU
int iptGetTopoCnt(UINT32 destIpAddr, UINT32 simuIpAddr, UINT32 *pTopoCnt);
#else
int iptGetTopoCnt(UINT32 destIpAddr, UINT32 *pTopoCnt);
#endif


void iptConfigShowComid(void);
void iptConfigShowDataset(void);
void iptConfigShowComPar(void);

const char* datatypeInt2String(INT32 type);

int iptStatInit(void);
#ifdef TARGET_SIMU
void iptSnmpInMessage(UINT32 srcIPaddr, UINT16 srcPort, UINT32 destIPaddr, UINT8 *pMsg, UINT32 msgLen);
#else
void iptSnmpInMessage(UINT32 srcIPaddr, UINT16 srcPort, UINT8 *pMsg, UINT32 msgLen);
#endif

int iptStrcmp(const char*  s1, const char*  s2);
int iptStrncmp(const char*  s1, const char*  s2, int len);

#ifdef TARGET_SIMU
int IPTCom_SimSendUDP(BYTE *pMsg, int msgLen, struct sockaddr *srcAddr, struct sockaddr *destAddr);
int IPTCom_SimProcessUDPData(BYTE *pRecBuf, unsigned short port, UINT32 *srcAddr, UINT16 *srcPort, UINT32 *destAddr, UINT32 *sizeUDP);
void IPTCom_SimJoinMulticastgroup(UINT32 simDevAddr, UINT32 multicastAddr, UINT8 pd, UINT8 md);
void IPTCom_SimLeaveMulticastgroup(UINT32 simDevAddr, UINT32 multicastAddr, UINT8 pd, UINT8 md);
#endif

#ifdef __cplusplus
}
#endif

#endif
