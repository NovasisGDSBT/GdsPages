/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2010 Bombardier Transportation
 *******************************************************************************
 *  PROJECT     : IPTrain
 *
 *  MODULE      : iptcom_statistic.c
 *
 *  ABSTRACT    : Method for IPTCom statistics
 *
 *******************************************************************************
 *  HISTORY     :
 *	
 *  $Id: iptcom_statistic.c 35883 2015-03-10 10:50:54Z gweiss $
 *
 *  CR-4092 (Gerhard Weiss, 2015-02-05)
 *          Corrected Source of Counter in getUriListenerDetailed
 *
 *  CR-3477 (Bernd Löhr, 2012-02-18)
 * 			TÜV Assessment findings, missing default statement
 *
 *  CR-432 (Gerhard Weiss, 2010-11-24)
 *          Added more missing UNUSED Parameter Macros
 *
 *  CR-432 (Bernd Loehr, 2010-08-13) 
 * 			Compiler warnings (sprintf)
 *
 *  Internal (Bernd Loehr, 2010-08-13) 
 * 			Old obsolete CVS history removed
 *
 *
 ******************************************************************************/


/*******************************************************************************
*  INCLUDES */
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "iptcom.h"        /* Common type definitions for IPT */
#include "vos.h"           /* OS independent system calls */
#include "netdriver.h"
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"

/*******************************************************************************
*  DEFINES
*/

/* Table types */
#define IPTCOM_OID_TABLE   1
#define IPTCOM_OID_DATA    2

/* Action*/
#define IPTCOM_STAT_GET     1
#define IPTCOM_STAT_GETNEXT 2
#define IPTCOM_STAT_SET     3

/*******************************************************************************
*  TYPEDEFS
*/

typedef struct
{
   int size;
   IPT_OID_DATA *pStatStruct;
} IPT_OID_DATA_HDR;

typedef struct
{
   UINT32 oidDig;
   char name[MAX_OID_NAME];
   union
   {
      IPT_TAB_HDR  nextOidTable;
      IPT_OID_DATA_HDR oidHdr;
   }tabOrData;
   int tableType;
} STAT_TABLE_STRUCT;

/*******************************************************************************
*  GLOBAL DATA
*/

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/
static int getVersion(IPT_STAT_DATA *pStatData);
static int getUpTime(IPT_STAT_DATA *pStatData);
static int getDevName(IPT_STAT_DATA *pStatData);
static int getIpAddr(IPT_STAT_DATA *pStatData);
static int getTask(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getPdStatTime(IPT_STAT_DATA *pStatData);
static int getMdStatTime(IPT_STAT_DATA *pStatData);
static int getSubscriber(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getSubscriberGrp(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getPublisher(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getPublisherGrp(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getPdJoinedMcAddr(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getPdRedundant(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getComidListener(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getComidFrgListener(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getComidListenerDetailed(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getUriListener(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getUriFrgListener(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getUriListenerDetailed(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getMdJoinedMcAddr(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getMdRedundant(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getMemBlock(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getqueueErr(IPT_STAT_DATA *pStatData);
static int getMdDefaultMaxSeqNo(IPT_STAT_DATA *pStatData);
static int getPdDefaultValBeh(IPT_STAT_DATA *pStatData);
static int getExchPar(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getSourceFilter(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getDestId(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getDatasetData(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getAllDatasetData(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getComPar(IPT_STAT_DATA *pStatData, UINT32 ind, UINT32 oidDig);
static int getSetDebugLevel(IPT_STAT_DATA *pStatData, int set);
static int getSetDebugInfo(IPT_STAT_DATA *pStatData, int set);
static int getSetDebugFileName(IPT_STAT_DATA *pStatData, int set);
static int getSetDebugFileSize(IPT_STAT_DATA *pStatData, int set);
static int clearStatistic(IPT_STAT_DATA *pStatData);
static int pdClearStatistic(IPT_STAT_DATA *pStatData);
static int mdClearStatistic(IPT_STAT_DATA *pStatData);
#ifdef LINUX_MULTIPROC
static int iptStatOwnProcessInit(void);
static int iptStatAddSubAgent(UINT32 len, IPT_OID_DEF oidDef[], IPT_OID_DATA *pStatStruct,  UINT32 size);
#endif
           
/*******************************************************************************
*  LOCAL DATA
*/

#ifdef LINUX_MULTIPROC
static int initiated = 0;
static IPT_TAB_HDR oidTable;
#endif
static UINT32 dataSetHandle = 0;    /* Handle for dataset to get data items for */

static IPT_TAB_HDR *pOidStartTable = NULL;

static UINT32 devServices = 3;

/*lint -save -e611 -e64  Ignore warning Suspicious cast */
static IPT_OID_DATA taskItem[] =
{{1, "Index_Taskdev",     0, {(void *)getTask}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2, "Name_Taskdev",      0, {(void *)getTask}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {3, "Priority_Taskdev",  0, {(void *)getTask}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {4, "CycleTime_Taskdev", 0, {(void *)getTask}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};


static IPT_OID_DATA taskTable[] =
{{1, "Task_dev", sizeof(taskItem)/sizeof(IPT_OID_DATA), {(void *)taskItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};


static IPT_OID_DATA dev_stat[] =
{{1, "IPTComVer_dev",       0, {(void *)getVersion},     IPTCOM_STAT_FUNC_NOKEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {2, "UpTime_dev",          0, {(void *)getUpTime},      IPTCOM_STAT_FUNC_NOKEY_R, IPT_STAT_TYPE_TIMETICKS},
 {3, "OwnURI_dev",          0, {(void *)getDevName},     IPTCOM_STAT_FUNC_NOKEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {4, "OwnIpAddress_dev",    0, {(void *)getIpAddr},      IPTCOM_STAT_FUNC_NOKEY_R, IPT_STAT_TYPE_IPADDRESS},
 {5, "Services_dev",        0, {(void *)&devServices},   IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER}, /*lint !e826 Type cast OK */
 {6, "ClearStatistics_dev", 0, {(void *)clearStatistic}, IPTCOM_STAT_FUNC_NOKEY_W, IPT_STAT_TYPE_INTEGER},
 {7, "TaskTable_dev", sizeof(taskTable)/sizeof(IPT_OID_DATA), {(void *)taskTable}, IPTCOM_STAT_DATA_STRUCT,   IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA pdSubItem[] =
{{1, "ComId_pdSub",           0, {(void *)getSubscriber}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {2, "FilterIpAddress_pdSub", 0, {(void *)getSubscriber}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_IPADDRESS},
 {3, "InPackets_pdSub",       0, {(void *)getSubscriber}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA pdSubTable[] =
{{1, "Subscription_pd", sizeof(pdSubItem)/sizeof(IPT_OID_DATA), {(void *)pdSubItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA pdPubItem[] =
{{1, "ComId_pdPub",         0, {(void *)getPublisher}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {2, "DestIpAddress_pdPub", 0, {(void *)getPublisher}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_IPADDRESS},
 {3, "OutPackets_pdPub",    0, {(void *)getPublisher}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA pdPubTable[] =
{{1, "Publishing_pd", sizeof(pdPubItem)/sizeof(IPT_OID_DATA), {(void *)pdPubItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

IPT_OID_DATA pdMulticastJoinItem[] =
{{1, "MulticastIpAddr_pd",     0, {(void *)getPdJoinedMcAddr}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_IPADDRESS}
};

static IPT_OID_DATA pdMulticastJoinTable[] =
{{1, "MulticastJoin_pd", sizeof(pdMulticastJoinItem)/sizeof(IPT_OID_DATA), {(void *)pdMulticastJoinItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA pdSubGrpItem[] =
{{1, "GrpNo_subGrp",           0, {(void *)getSubscriberGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {2, "Index_subGrp",           0, {(void *)getSubscriberGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {3, "ComId_subGrp",           0, {(void *)getSubscriberGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {4, "FilterIp_subGrp",        0, {(void *)getSubscriberGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {5, "Status_subGrp",          0, {(void *)getSubscriberGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {6, "TimOut_subGrp",          0, {(void *)getSubscriberGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {7, "ValBeh_subGrp",          0, {(void *)getSubscriberGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {8, "JoinedIpAddress_subGrp", 0, {(void *)getSubscriberGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};

static IPT_OID_DATA pdSubGrpTable[] =
{{1, "SubGrp_pd", sizeof(pdSubGrpItem)/sizeof(IPT_OID_DATA), {(void *)pdSubGrpItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA pdPubGrpItem[] =
{{1, "GrpNo_pubGrp",          0, {(void *)getPublisherGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {2, "Index_pubGrp",          0, {(void *)getPublisherGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {3, "ComId_pubGrp",          0, {(void *)getPublisherGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {4, "DestIpAddress_pubGrp",  0, {(void *)getPublisherGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {5, "CycleTime_pubGrp",      0, {(void *)getPublisherGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {6, "RedundantId_pubGrp",    0, {(void *)getPublisherGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {7, "RedundantState_pubGrp", 0, {(void *)getPublisherGrp}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};

static IPT_OID_DATA pdPubGrpTable[] =
{{1, "PubGrp_pd", sizeof(pdPubGrpItem)/sizeof(IPT_OID_DATA), {(void *)pdPubGrpItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA pdRedIdItem[] =
{{1, "IdNo_redId",           0, {(void *)getPdRedundant}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {2, "RedundantState_redId", 0, {(void *)getPdRedundant}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};

static IPT_OID_DATA pdRedundantIdTable[] =
{{1, "RedId_pd", sizeof(pdRedIdItem)/sizeof(IPT_OID_DATA), {(void *)pdRedIdItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA pd_stat[] =
{{1, "InPackets_pd",             0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {2, "FCSErrors_pd",             0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {3, "ProtocolErrors_pd",        0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {4, "TopoErrors_pd",            0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {5, "InPacketsNoSubscriber_pd", 0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {6, "OutPackets_pd",            0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {7, "StatisticsTime_pd",        0, {(void *)getPdStatTime},    IPTCOM_STAT_FUNC_NOKEY_R, IPT_STAT_TYPE_TIMETICKS},
 {8, "ClearStatistics_pd",       0, {(void *)pdClearStatistic}, IPTCOM_STAT_FUNC_NOKEY_W, IPT_STAT_TYPE_INTEGER},
 {9, "SubscriptionTable_pd",    sizeof(pdSubTable)/sizeof(IPT_OID_DATA),           {(void *)pdSubTable},           IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {10,"PublishingTable_pd",      sizeof(pdPubTable)/sizeof(IPT_OID_DATA),           {(void *)pdPubTable},           IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {11,"MulticastJoinTable_pd",   sizeof(pdMulticastJoinTable)/sizeof(IPT_OID_DATA), {(void *)pdMulticastJoinTable}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {12,"SubscriptionGrpTable_pd", sizeof(pdSubGrpTable)/sizeof(IPT_OID_DATA),        {(void *)pdSubGrpTable},        IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {13,"PublishGrpTable_pd",        sizeof(pdPubGrpTable)/sizeof(IPT_OID_DATA),        {(void *)pdPubGrpTable},        IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {14,"RedundantIdTable_pd",   sizeof(pdRedundantIdTable)/sizeof(IPT_OID_DATA),   {(void *)pdRedundantIdTable},   IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA mdComidListenerItem[] =
{{1, "ComId_clmd",     0, {(void *)getComidListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2, "InPackets_clmd", 0, {(void *)getComidListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA mdComidListenerTable[] =
{{1, "ComIdListener_md", sizeof(mdComidListenerItem)/sizeof(IPT_OID_DATA), {(void *)mdComidListenerItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA mdComidFrgListenerItem[] =
{{1, "ComId_cflmd",     0, {(void *)getComidFrgListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2, "InPackets_cflmd", 0, {(void *)getComidFrgListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA mdComidFrgListenerTable[] =
{{1, "ComIdFrgListener_md", sizeof(mdComidFrgListenerItem)/sizeof(IPT_OID_DATA), {(void *)mdComidFrgListenerItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

IPT_OID_DATA mdUriListenerItem[] =
{{1, "Index_ulmd",     0, {(void *)getUriListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2, "Uri_ulmd",       0, {(void *)getUriListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {3, "InPackets_ulmd", 0, {(void *)getUriListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA mdUriListenerTable[] =
{{1, "UriListener_md", sizeof(mdUriListenerItem)/sizeof(IPT_OID_DATA), {(void *)mdUriListenerItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

IPT_OID_DATA mdUriFrgListenerItem[] =
{{1, "Index_uflmd",     0, {(void *)getUriFrgListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2, "Uri_uflmd",       0, {(void *)getUriFrgListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {3, "InPackets_uflmd", 0, {(void *)getUriFrgListener}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA mdUriFrgListenerTable[] =
{{1, "UriFrgListener_md", sizeof(mdUriFrgListenerItem)/sizeof(IPT_OID_DATA), {(void *)mdUriFrgListenerItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

IPT_OID_DATA mdMulticastJoinItem[] =
{{1, "MulticastIpAddr_md",     0, {(void *)getMdJoinedMcAddr}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_IPADDRESS}
};

static IPT_OID_DATA mdMulticastJoinTable[] =
{{1, "MulticastJoin_md", sizeof(mdMulticastJoinItem)/sizeof(IPT_OID_DATA), {(void *)mdMulticastJoinItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

IPT_OID_DATA mdComIdListenerDetailedItem[] =
{{1, "ComId_cldmd",           0, {(void *)getComidListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {2, "Index_cldmd",           0, {(void *)getComidListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {3, "ListenerType_cldmd",    0, {(void *)getComidListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {4, "QueueORFunc_cldmd",     0, {(void *)getComidListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {5, "QueueName_cldmd",       0, {(void *)getComidListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {6, "CallerRef_cldmd",       0, {(void *)getComidListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {7, "FrgRef_cldmd",          0, {(void *)getComidListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {8, "RedundantState_cldmd",  0, {(void *)getComidListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {9, "JoinedIpAddress_cldmd", 0, {(void *)getComidListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};

static IPT_OID_DATA mdComIdListenerDetailedTable[] =
{{1, "ComIdListenerDetailed_md", sizeof(mdComIdListenerDetailedItem)/sizeof(IPT_OID_DATA), {(void *)mdComIdListenerDetailedItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

IPT_OID_DATA mdUriListenerDetailedItem[] =
{{1,  "UriIndex_uldmd",        0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {2,  "ListenerIndex_uldmd",   0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {3,  "Uri_uldmd",             0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {4,  "ListenerType_uldmd",    0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {5,  "QueueORFunc_uldmd",     0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {6,  "QueueName_uldmd",       0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {7,  "CallerRef_uldmd",       0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {8,  "FrgRef_uldmd",          0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {9,  "RedundantState_uldmd",  0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {10, "JoinedIpAdrress_uldmd", 0, {(void *)getUriListenerDetailed}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};

static IPT_OID_DATA mdUriListenerDetailedTable[] =
{{1, "UriListenerDetailed_md", sizeof(mdUriListenerDetailedItem)/sizeof(IPT_OID_DATA), {(void *)mdUriListenerDetailedItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA mdFrgItem[] =
{{1, "FrgRef_frgmd",         0, {(void *)getMdRedundant}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {2, "RedundantState_frgmd", 0, {(void *)getMdRedundant}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};

static IPT_OID_DATA mdFrgRefTable[] =
{{1, "RedFrg_md", sizeof(mdFrgItem)/sizeof(IPT_OID_DATA), {(void *)mdFrgItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA md_stat[] =
{{1, "InPackets_md",            0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {2, "FreeSize_mem",            0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {3, "ProtocolErrors_md",       0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {4, "TopoErrors_md",           0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {5, "InPacketsNoListeners_md", 0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {6, "NotReturnedBuffers_md",   0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {7, "OutPackets_md",           0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {8, "Retransmissions_md",      0, {NULL},                     IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {9, "StatisticsTime_md",       0, {(void *)getMdStatTime},    IPTCOM_STAT_FUNC_NOKEY_R, IPT_STAT_TYPE_TIMETICKS},
 {10,"ClearStatistics_md",      0, {(void *)mdClearStatistic}, IPTCOM_STAT_FUNC_NOKEY_W, IPT_STAT_TYPE_INTEGER},
 {11,"ComIdListenerTable_md",         sizeof(mdComidListenerTable)/sizeof(IPT_OID_DATA),         {(void *)mdComidListenerTable},         IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {12,"ComIdFrgListenerTable_md",      sizeof(mdComidFrgListenerTable)/sizeof(IPT_OID_DATA),      {(void *)mdComidFrgListenerTable},      IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {13,"UriListenerTable_md",           sizeof(mdUriListenerTable)/sizeof(IPT_OID_DATA),           {(void *)mdUriListenerTable},           IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {14,"UriFrgListenerTable_md",        sizeof(mdUriFrgListenerTable)/sizeof(IPT_OID_DATA),        {(void *)mdUriFrgListenerTable},        IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {15,"MulticastJoinTable_md",         sizeof(mdMulticastJoinTable)/sizeof(IPT_OID_DATA),         {(void *)mdMulticastJoinTable},         IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {16,"ComIdListenerDetailedTable_md", sizeof(mdComIdListenerDetailedTable)/sizeof(IPT_OID_DATA), {(void *)mdComIdListenerDetailedTable}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {17,"UriListenerDetailedTable_md",   sizeof(mdUriListenerDetailedTable)/sizeof(IPT_OID_DATA),   {(void *)mdUriListenerDetailedTable},   IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {18,"RedFrgTable_md",                sizeof(mdFrgRefTable)/sizeof(IPT_OID_DATA),                {(void *)mdFrgRefTable},                IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA mdBlockItem[] =
{{1, "BlockSize_mem",      0, {(void *)getMemBlock}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2, "AllocatedBlock_mem", 0, {(void *)getMemBlock}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {3, "UsedBlock_mem",      0, {(void *)getMemBlock}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA memBlockTable[] =
{{1, "Block_mem", sizeof(mdBlockItem)/sizeof(IPT_OID_DATA), {(void *)mdBlockItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA mem_stat[] =
{{1, "TotalSize_mem",        0, {NULL}, IPTCOM_STAT_DATA_PTR_R, IPT_STAT_TYPE_COUNTER},
 {2, "FreeSize_mem",         0, {NULL}, IPTCOM_STAT_DATA_PTR_R, IPT_STAT_TYPE_COUNTER},
 {3, "MinFreeSize_mem",      0, {NULL}, IPTCOM_STAT_DATA_PTR_R, IPT_STAT_TYPE_COUNTER},
 {4, "AllocatedBlocks_mem",  0, {NULL}, IPTCOM_STAT_DATA_PTR_R, IPT_STAT_TYPE_COUNTER},
 {5, "AllocationErrors_mem", 0, {NULL}, IPTCOM_STAT_DATA_PTR_R, IPT_STAT_TYPE_COUNTER},
 {6, "FreeErrors_mem",       0, {NULL}, IPTCOM_STAT_DATA_PTR_R, IPT_STAT_TYPE_COUNTER},
 {7, "BlockTable_mem", sizeof(memBlockTable)/sizeof(IPT_OID_DATA), {(void *)memBlockTable}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA queue_stat[] =
{{1, "Allocated_queue",    0, {NULL},                IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {2, "MaxAllocated_queue", 0, {NULL},                IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {3, "Failures_queue",     0, {(void *)getqueueErr}, IPTCOM_STAT_FUNC_NOKEY_R, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA exchgParItem[] =
{{1,  "ComId_exp",                0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2,  "DataSetId_exp",            0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {3,  "ComParId_exp",             0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {4,  "PdSendDestUri_exp",        0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {5,  "PdSendCycleTime_exp",      0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {6,  "PdSendRedundantId_exp",    0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {7,  "PdRecSourceFilterUri_exp", 0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {8,  "PdRecTimeOut_exp",         0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {9,  "PdRecValidBehavior_exp",   0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {10, "MdSendDestUri_exp",        0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {11, "MdSendSourceUri_exp",      0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {12, "MdSendAckTimeOut_exp",     0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {13, "MdSendRespTimeOut_exp",    0, {(void *)getExchPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};     

static IPT_OID_DATA exchgParTable[] =
{{1, "exchgPar", sizeof(exchgParItem)/sizeof(IPT_OID_DATA), {(void *)exchgParItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA cfgSourceFilterItem[] =
{{1,  "ComId_sf",     0, {(void *)getSourceFilter}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2,  "FilterId_sf",  0, {(void *)getSourceFilter}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {3,  "SourceUri_sf", 0, {(void *)getSourceFilter}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};     

static IPT_OID_DATA cfgSourceFilterTable[] =
{{1, "sourceFilter", sizeof(cfgSourceFilterItem)/sizeof(IPT_OID_DATA), {(void *)cfgSourceFilterItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA dataSetVariable[] =
{{1, "Index_ds",      0, {(void *)getDatasetData}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {2, "Type_ds",       0, {(void *)getDatasetData}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {3, "ArraySize_ds",  0, {(void *)getDatasetData}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA dataSetTable[] =
{{1, "dataSetVariable", sizeof(dataSetVariable)/sizeof(IPT_OID_DATA), {(void *)dataSetVariable}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA allDataSetVariable[] =
{{1, "DataSetId_dsa", 0, {(void *)getAllDatasetData}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {2, "Index_dsa", 0, {(void *)getAllDatasetData}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {3, "Type_dsa",  0, {(void *)getAllDatasetData}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {4, "ArraySize_dsa",  0, {(void *)getAllDatasetData}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};

static IPT_OID_DATA allDataSetTable[] =
{{1, "allDataSetVariable", sizeof(allDataSetVariable)/sizeof(IPT_OID_DATA), {(void *)allDataSetVariable}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA dataSet[] =
{{1, "DataSetHandle", 0, {(void *)&dataSetHandle},IPTCOM_STAT_DATA_PTR_RW, IPT_STAT_TYPE_COUNTER}, /*lint !e826 Type cast OK */
 {2, "dataSetTable",    sizeof(dataSetTable)/sizeof(IPT_OID_DATA),   {(void *)dataSetTable},    IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {3, "allDataSetTable", sizeof(allDataSetTable)/sizeof(IPT_OID_DATA),{(void *)allDataSetTable}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA comParItem[] =
{{1,  "ComParId_Cp", 0, {(void *)getComPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2,  "QOS_Cp",      0, {(void *)getComPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {3,  "TTL_Cp",      0, {(void *)getComPar}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER}
};     

static IPT_OID_DATA comParTable[] =
{{1, "comPar", sizeof(comParItem)/sizeof(IPT_OID_DATA), {(void *)comParItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA cfgDestIdItem[] =
{{1,  "ComId_di",          0, {(void *)getDestId}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},  
 {2,  "DestinationId_ds",  0, {(void *)getDestId}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_COUNTER},
 {3,  "DestinationUri_ds", 0, {(void *)getDestId}, IPTCOM_STAT_FUNC_KEY_R, IPT_STAT_TYPE_OCTET_STRING}
};     

static IPT_OID_DATA cfgDestIdTable[] =
{{1, "destId", sizeof(cfgDestIdItem)/sizeof(IPT_OID_DATA), {(void *)cfgDestIdItem}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA config_stat[] =
{{1, "PdDefaultTimeout_cfg",         0, {NULL},                         IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {2, "PdDefaultValidBehavior_cfg",   0, {(void *)getPdDefaultValBeh},   IPTCOM_STAT_FUNC_NOKEY_R, IPT_STAT_TYPE_OCTET_STRING},
 {3, "PdDefaultCycleTime_cfg",       0, {NULL},                         IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {4, "MdDefaultAckTimeOut_cfg",      0, {NULL},                         IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {5, "MdDefaultResponseTimeOut_cfg", 0, {NULL},                         IPTCOM_STAT_DATA_PTR_R,   IPT_STAT_TYPE_COUNTER},
 {6, "MdMaxNoOfStoredSeqNo_cfg",     0, {(void *)getMdDefaultMaxSeqNo}, IPTCOM_STAT_FUNC_NOKEY_R, IPT_STAT_TYPE_COUNTER},
 {7, "exchgParTable",     sizeof(exchgParTable)/sizeof(IPT_OID_DATA),        {(void *)exchgParTable},         IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {8, "sourceFilterTable", sizeof(cfgSourceFilterTable)/sizeof(IPT_OID_DATA), {(void *)cfgSourceFilterTable}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {9, "dataSet",           sizeof(dataSet)/sizeof(IPT_OID_DATA),              {(void *)dataSet}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {10,"comParTable",       sizeof(comParTable)/sizeof(IPT_OID_DATA),          {(void *)comParTable}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {11, "destIdTable",      sizeof(cfgDestIdTable)/sizeof(IPT_OID_DATA),       {(void *)cfgDestIdTable}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};

static IPT_OID_DATA debug_stat[] =
{{1, "Mask_debug",        0, {(void *)getSetDebugLevel},    IPTCOM_STAT_FUNC_NOKEY_RW, IPT_STAT_TYPE_OCTET_STRING},
 {2, "Info_debug",        0, {(void *)getSetDebugInfo},     IPTCOM_STAT_FUNC_NOKEY_RW, IPT_STAT_TYPE_OCTET_STRING},
 {3, "FileName_debug",    0, {(void *)getSetDebugFileName}, IPTCOM_STAT_FUNC_NOKEY_RW, IPT_STAT_TYPE_OCTET_STRING},
 {4, "FileSize_debug",    0, {(void *)getSetDebugFileSize}, IPTCOM_STAT_FUNC_NOKEY_RW, IPT_STAT_TYPE_COUNTER}
};

static IPT_OID_DATA iptcom_stat[] = 
{{1, "dev",    sizeof(dev_stat)/sizeof(IPT_OID_DATA),    {(void *)dev_stat},    IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {2, "pd",     sizeof(pd_stat)/sizeof(IPT_OID_DATA),     {(void *)pd_stat},     IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {3, "md",     sizeof(md_stat)/sizeof(IPT_OID_DATA),     {(void *)md_stat},     IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {4, "mem",    sizeof(mem_stat)/sizeof(IPT_OID_DATA),    {(void *)mem_stat},    IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {5, "queue",  sizeof(queue_stat)/sizeof(IPT_OID_DATA),  {(void *)queue_stat},  IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {6, "config", sizeof(config_stat)/sizeof(IPT_OID_DATA), {(void *)config_stat}, IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL},
 {7, "debug",  sizeof(debug_stat)/sizeof(IPT_OID_DATA), {(void *)debug_stat},  IPTCOM_STAT_DATA_STRUCT, IPT_STAT_TYPE_NULL}
};
/*lint -restore */ 

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
NAME:       getVersion
ABSTRACT:   Get time in seconds since start-up.
RETURNS:    0 if OK, != if not
*/
static int getVersion(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   sprintf(pStatData->value.octetString,
           "IPTCom %d.%d.%d.%d %s", 
            IPT_VERSION, IPT_RELEASE, IPT_UPDATE, IPT_EVOLUTION,
            IPTGLOBAL(buildTime));

   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:       getUpTime
ABSTRACT:   Get time in seconds since start-up.
RETURNS:    0 if OK, != if not
*/
static int getUpTime(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   pStatData->value.timeTicks = (IPTVosGetMilliSecTimer() - IPTGLOBAL(iptcomStarttime))/10;
   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:       getDevName
ABSTRACT:   Get own URI.
RETURNS:    0 if OK, != if not
*/
static int getDevName(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   UINT8 topoCnt = 0;
   T_TDC_RESULT tdcRes;
   
   tdcRes = iptGetUriHostPart (0x7f000001, pStatData->value.octetString, &topoCnt);

   if (tdcRes != 0)
   {
      pStatData->value.octetString[0] = 0;
   }
   return(tdcRes);  
}

/*******************************************************************************
NAME:       getIpAddr
ABSTRACT:   Get own IP address.
RETURNS:    0 if OK, != if not
*/
static int getIpAddr(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   pStatData->value.ipAddress = IPTCom_getOwnIpAddr();
   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:       getTaskPriorityString
ABSTRACT:   Prepare task priority string.
RETURNS:    0 if OK, != if not
*/
static void getTaskPriorityString(
   int priority,
   char * outString)
{
#if defined(WIN32)
   switch (priority)
   {
      case THREAD_PRIORITY_NORMAL:
         sprintf(outString, "Normal");
         break;

      case THREAD_PRIORITY_ABOVE_NORMAL:
         sprintf(outString, "Above Normal");
         break;

      case THREAD_PRIORITY_BELOW_NORMAL:
         sprintf(outString, "Below Normal");
         break;

      case THREAD_PRIORITY_HIGHEST:
         sprintf(outString, "Highest");
         break;

      case THREAD_PRIORITY_LOWEST:
         sprintf(outString, "Lowest");
         break;

      case THREAD_PRIORITY_TIME_CRITICAL:
         sprintf(outString, "Time Critical");
         break;

      case THREAD_PRIORITY_IDLE:
         sprintf(outString, "Idle");
         break;

      default:
         sprintf(outString, "NOT VALID VALUE");
         break;
   }

#else
   sprintf(outString, "%d", priority);
#endif   
}

/*******************************************************************************
NAME:       getTask
ABSTRACT:   Get task statistic.
RETURNS:    0 if OK, != if not
*/
static int getTask(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   UINT32 devTaskIndex;
   int found = 0;


   devTaskIndex = pStatData->oid[ind + 1];
   /* Key in request? */
   if (devTaskIndex != IPT_STAT_OID_STOPPER)
   {
      if (IPTGLOBAL(iptComInitiated))
      {
         devTaskIndex++;
         switch (devTaskIndex)
         {
            case 1: /* PD receive */
               found = 1;
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = devTaskIndex;
                     break;

                  case 2: /* Task name */
                     sprintf(pStatData->value.octetString,
                             "PD receive");
                     break;

                  case 3: /* Task priority */
                     getTaskPriorityString(IPTGLOBAL(task.pdRecPriority),
                                           pStatData->value.octetString);
                     break;

                  case 4: /* Task cycle time */
                     sprintf(pStatData->value.octetString, " ");
                     break;

                  default:
                     found = 0;
                     break;
               }
               break;

            case 2: /* MD receive */
               found = 1;
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = devTaskIndex;
                     break;

                  case 2: /* Task name */
                     sprintf(pStatData->value.octetString,
                             "MD receive");
                     break;

                  case 3: /* Task priority */
                     getTaskPriorityString(IPTGLOBAL(task.mdRecPriority),
                                           pStatData->value.octetString);
                     break;

                  case 4: /* Task cycle time */
                     sprintf(pStatData->value.octetString, " ");
                     break;

                  default:
                     found = 0;
                     break;
               }
               break;

            case 3: /* SNMP receive */
               found = 1;
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = devTaskIndex;
                     break;

                  case 2: /* Task name */
                     sprintf(pStatData->value.octetString,
                             "SNMP receive");
                     break;

                  case 3: /* Task priority */
                     getTaskPriorityString(IPTGLOBAL(task.snmpRecPriority),
                                           pStatData->value.octetString);
                     break;

                  case 4: /* Task cycle time */
                     sprintf(pStatData->value.octetString, " ");
                     break;

                  default:
                     found = 0;
                     break;
               }
               break;

            case 4: 
               found = 1;
               if (IPTGLOBAL(task.iptComProcCycle) != 0)
               {
                  switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = devTaskIndex;
                        break;

                     case 2: /* Task name */
                        sprintf(pStatData->value.octetString,
                                "IPTCom send");
                        break;

                     case 3: /* Task priority */
                        getTaskPriorityString(IPTGLOBAL(task.iptComProcPriority),
                                              pStatData->value.octetString);
                        break;

                     case 4: /* Task cycle time */
                        sprintf(pStatData->value.octetString,
                                "%d", IPTGLOBAL(task.iptComProcCycle));
                        break;

                     default:
                        found = 0;
                        break;
                  }
               }
               else
               {
                  switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = devTaskIndex;
                        break;

                     case 2: /* Task name */
                        sprintf(pStatData->value.octetString,
                                "PD send");
                        break;

                     case 3: /* Task priority */
                        getTaskPriorityString(IPTGLOBAL(task.pdProcPriority),
                                              pStatData->value.octetString);
                        break;

                     case 4: /* Task cycle time */
                        sprintf(pStatData->value.octetString,
                                "%d", IPTGLOBAL(task.pdProcCycle));
                        break;

                     default:
                        found = 0;
                        break;
                  }
               }
               break;

            case 5: 
               found = 1;
               if (IPTGLOBAL(task.iptComProcCycle) != 0)
               {
#ifdef LINUX_MULTIPROC
                   switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = devTaskIndex;
                        break;

                     case 2: /* Task name */
                        sprintf(pStatData->value.octetString,
                                "Net Control");
                        break;

                     case 3: /* Task priority */
                        getTaskPriorityString(IPTGLOBAL(task.netCtrlPriority),
                                              pStatData->value.octetString);
                        break;

                     case 4: /* Task cycle time */
                        sprintf(pStatData->value.octetString, " ");
                        break;

                     default:
                        found = 0;
                        break;
                  }
#else
                  found = 0;
#endif                
               }
               else
               {
                  switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = devTaskIndex;
                        break;

                     case 2: /* Task name */
                        sprintf(pStatData->value.octetString,
                                "MD send");
                        break;

                     case 3: /* Task priority */
                        getTaskPriorityString(IPTGLOBAL(task.mdProcPriority),
                                              pStatData->value.octetString);
                        break;

                     case 4: /* Task cycle time */
                        sprintf(pStatData->value.octetString,
                                "%d", IPTGLOBAL(task.mdProcCycle));
                        break;

                     default:
                        found = 0;
                        break;
                  }
               }
               break;

#ifdef LINUX_MULTIPROC
            case 6: 
               if (IPTGLOBAL(task.iptComProcCycle) == 0)
               {
                  found = 1;
                   switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = devTaskIndex;
                        break;

                     case 2: /* Task name */
                        sprintf(pStatData->value.octetString,
                                "Net Control");
                        break;

                     case 3: /* Task priority */
                        getTaskPriorityString(IPTGLOBAL(task.netCtrlPriority),
                                              pStatData->value.octetString);
                        break;

                     case 4: /* Task cycle time */
                        sprintf(pStatData->value.octetString, " ");
                        break;

                     default:
                        found = 0;
                        break;
                  }
               }
               else
               {
                  found = 0;
               }
               break;
#endif                
               default:
                  found = 0;
                  break;

         }
 
         if (found)
         {
            pStatData->oid[ind + 1] = devTaskIndex;
            pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
         }
      }
   }
   else
   {
      if (IPTGLOBAL(iptComInitiated))
      {
         found = 1;
         pStatData->oid[ind + 1] = 1;
         pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
   
         switch (oidDig)
         {
            case 1:
               pStatData->value.counter = 1;
               break;

            case 2: /* Task name */
               sprintf(pStatData->value.octetString,
                       "PD receive");
               break;

            case 3: /* Task priority */
               getTaskPriorityString(IPTGLOBAL(task.pdRecPriority),
                                     pStatData->value.octetString);
               break;

            case 4: /* Task cycle time */
               sprintf(pStatData->value.octetString, " ");
               break;

            default:
               found = 0;
               break;
         }
      }
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}


/*******************************************************************************
NAME:       getPdStatTime
ABSTRACT:   Get time in seconds since start-up.
RETURNS:    0 if OK, != if not
*/
static int getPdStatTime(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   pStatData->value.timeTicks = (IPTVosGetMilliSecTimer() - IPTGLOBAL(pd.pdCnt.pdStatisticsStarttime))/10;
   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:       getMdStatTime
ABSTRACT:   Get time in seconds since start-up.
RETURNS:    0 if OK, != if not
*/
static int getMdStatTime(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   pStatData->value.timeTicks = (IPTVosGetMilliSecTimer() - IPTGLOBAL(md.mdCnt.mdStatisticsStarttime))/10;
   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:     getSubscriber
ABSTRACT: Get subscriber counters.
RETURNS:  -
*/
static int getSubscriber(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)             /* IN OID digit */
{
   int ret;
   int i;
   int j;
   int mul;
   int found = 0;
   UINT32 val;
   UINT32 comId;
   UINT32 sourceIP = 0;
   UINT32 foundComId = 0;
   UINT32 foundSourceIP = 0;
   UINT32 pdInPackets = 0;

   comId = pStatData->oid[ind + 1];
   /* Key 1 in request? */
   if (comId != IPT_STAT_OID_STOPPER)
   {
      /* Key 2 in request? */
      mul = 0x1000000;
      for (i = 0; i < 4; i++)
      {
         val = pStatData->oid[ind + 2 + i];
         if (val != IPT_STAT_OID_STOPPER)
         {
            sourceIP += val * mul;
            mul = mul/0x100;  
         }
         else
         {
            return((int)IPT_INVALID_PAR);
         }
      }

      ret = IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         for (i = 0; i < IPTGLOBAL(pd.recTab.nItems); i++)
         {
            /* Check pointer to table just for sure */
            if (IPTGLOBAL(pd.recTab.pTable))
            {
               /* The table ComId same as requested ComId? */
               if (IPTGLOBAL(pd.recTab.pTable)[i].comId == comId)
               {
                  /* Requested source ip = 0? */
                  if (sourceIP == 0)
                  {
                     /* Any filtered subscribers ? */
                     if ((IPTGLOBAL(pd.recTab.pTable)[i].nItems > 0) &&
                         (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable) &&
                         ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[0].pFiltRecNetCB))
                     {
                        found = 1;
                        foundComId = comId;
                        foundSourceIP = IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[0].filtIpAddr;
                        pdInPackets = ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[0].pFiltRecNetCB)->pdInPackets;
                     }
                  }
                  else
                  {
                     /* Check pointer to table just for sure */
                     if (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable)
                     {
                        /* Search for next filter IP > request source IP */
                        for (j=0; j<IPTGLOBAL(pd.recTab.pTable)[i].nItems; j++)
                        {
                           if ((IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr > sourceIP) &&
                               ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].pFiltRecNetCB))
                           {
                              found = 1;
                              foundComId = comId;
                              foundSourceIP = IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].filtIpAddr;
                              pdInPackets = ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[j].pFiltRecNetCB)->pdInPackets;
                              break;
                           }
                        }   
                     }
                  }  
               }
               /* Next ComId ? */       
               else if (IPTGLOBAL(pd.recTab.pTable)[i].comId > comId)
               {
                  /* Any unfiltered subscriber */
                  if (IPTGLOBAL(pd.recTab.pTable)[i].pRecNetCB)
                  {
                     found = 1;
                     foundComId = IPTGLOBAL(pd.recTab.pTable)[i].comId;
                     foundSourceIP = 0;   
                     pdInPackets = ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pRecNetCB)->pdInPackets;
                  }
                  /* Any filtered subscriber? */
                  else if ((IPTGLOBAL(pd.recTab.pTable)[i].nItems > 0) &&
                           (IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable) &&
                           ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[0].pFiltRecNetCB))
                  {
                     found = 1;
                     foundComId = IPTGLOBAL(pd.recTab.pTable)[i].comId;
                     foundSourceIP = IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[0].filtIpAddr;
                     pdInPackets = ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[i].pFiltTable[0].pFiltRecNetCB)->pdInPackets;
                  }   
               }
            }
            
            if (found)
            {
               pStatData->oid[ind + 1] = foundComId;
               pStatData->oid[ind + 2] = (foundSourceIP & 0xff000000) >> 24;
               pStatData->oid[ind + 3] = (foundSourceIP & 0xff0000) >> 16;
               pStatData->oid[ind + 4] = (foundSourceIP & 0xff00) >> 8;
               pStatData->oid[ind + 5] = foundSourceIP & 0xff;
               pStatData->oid[ind + 6] = IPT_STAT_OID_STOPPER;
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = foundComId;
                     break;

                  case 2:
                     pStatData->value.ipAddress = foundSourceIP;
                     break;

                  case 3:
                     pStatData->value.counter = pdInPackets;
                     break;

                  default:
                     found = 0;
                     break;
               }
               /* Break for loop */
               break;
            }
         }
         if(IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getSubscriber: IPTVosGetSem ERROR\n");
      }
   }
   else
   {
      /* Request without any key */
      ret = IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         /* Any subscriber ? */
         if ((IPTGLOBAL(pd.recTab.nItems) > 0) &&
             (IPTGLOBAL(pd.recTab.pTable)))
         {
            /* Any unfiltered subsrciber ? */
            if (IPTGLOBAL(pd.recTab.pTable)[0].pRecNetCB)
            {
               found = 1;
               foundSourceIP = 0;   
               pdInPackets = ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[0].pRecNetCB)->pdInPackets;
            }
            /* Any filtered subsrciber ? */
            else if ((IPTGLOBAL(pd.recTab.pTable)[0].nItems > 0) &&
                     (IPTGLOBAL(pd.recTab.pTable)[0].pFiltTable) &&
                     ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[0].pFiltTable[0].pFiltRecNetCB))
            {
               found = 1;
               foundSourceIP = IPTGLOBAL(pd.recTab.pTable)[0].pFiltTable[0].filtIpAddr;
               pdInPackets = ((PD_REC_NET_CB *)IPTGLOBAL(pd.recTab.pTable)[0].pFiltTable[0].pFiltRecNetCB)->pdInPackets;
            }
            
            if (found)
            {
               pStatData->oid[ind + 1] = IPTGLOBAL(pd.recTab.pTable)[0].comId;
               pStatData->oid[ind + 2] = (foundSourceIP & 0xff000000) >> 24;
               pStatData->oid[ind + 3] = (foundSourceIP & 0xff0000) >> 16;
               pStatData->oid[ind + 4] = (foundSourceIP & 0xff00) >> 8;
               pStatData->oid[ind + 5] = foundSourceIP & 0xff;
               pStatData->oid[ind + 6] = IPT_STAT_OID_STOPPER;
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = IPTGLOBAL(pd.recTab.pTable)[0].comId;
                     break;

                  case 2:
                     pStatData->value.ipAddress = foundSourceIP;
                     break;

                  case 3:
                     pStatData->value.counter = pdInPackets;
                     break;

                  default:
                     found = 0;
                     break;
               }
            }
         }
         if(IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getSubscriber: IPTVosGetSem ERROR\n");
      }
   }

   if (found)
   {
      return((int)IPT_OK);
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }

}

/*******************************************************************************
NAME:     getSubscriberGrp
ABSTRACT: Get subscriber schedular group data
RETURNS:  -
*/
static int getSubscriberGrp(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)             /* IN OID digit */
{
   char digitstring[16];
   int ret;
   int found = 0;
   int schedIx;
   UINT32 i;
   UINT32 group;
   UINT32 grpIndex;
   UINT32 sourceIp = 0;
   UINT32 foundGroup = 0;
   UINT32 foundIndex = 0;
   UINT32 len;
   UINT32 leftLen = IPT_STAT_STRING_LEN;
   PD_SUB_CB *pSubCB = NULL;
   SUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.subSchedGrpTab);

   group = pStatData->oid[ind + 1];

   ret = IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      /* Key 1 in request? */
      if (group != IPT_STAT_OID_STOPPER)
      {
         grpIndex = pStatData->oid[ind + 2];

         /* Key 2 not in request? */
         if (grpIndex == IPT_STAT_OID_STOPPER)
         {
            ret = (int)IPT_INVALID_PAR;
         }
         else
         {
            if (pdSubGrpTabFind(group, &schedIx))
            {
               pSubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
               i = 0;
               while ((pSubCB != NULL) && (i < grpIndex)) /*lint !e685 Expression i <= grpIndex not always true */
               {
                  i++;
                  pSubCB = pSubCB->pNext;
               }
                
               if (pSubCB != NULL)
               {
                  found = 1;
                  foundGroup = group;
                  foundIndex = i + 1;    
               }
               else
               {
                  schedIx++;
                  while ((schedIx < pSchedGrpTab->nItems) && (!found))
                  {
                     pSubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
                     if (pSubCB)
                     {
                        found = 1;
                        foundGroup = pSchedGrpTab->pTable[schedIx].schedGrp;
                        foundIndex = 1;    
                     }
                     else
                     {
                        schedIx++;
                     }
                  }
               }
            }
         }
      }
      /* Request without any key */
      else
      {
         schedIx = 0;
         while ((schedIx < pSchedGrpTab->nItems) && (!found))
         {
            pSubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
            if (pSubCB)
            {
               found = 1;
               foundGroup = pSchedGrpTab->pTable[schedIx].schedGrp;
               foundIndex = 1;    
            }
            else
            {
               schedIx++;
            }
         }
      }
  
      if ((found) && (pSubCB))
      {
         pStatData->oid[ind + 1] = foundGroup;
         pStatData->oid[ind + 2] = foundIndex;
         pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
         switch (oidDig)
         {
            case 1:
               pStatData->value.counter = foundGroup;
               break;

            case 2:
               pStatData->value.counter = foundIndex;
               break;

            case 3:
               pStatData->value.counter = pSubCB->pdCB.comId;
               break;

            case 4:
               if (pSubCB->noOfNetCB == 0)
               {
                  if (IPTGLOBAL(pd.finishRecAddrResolv))
                  {
                     sprintf(pStatData->value.octetString,
                             "Subscription waiting for TDC");
                  }
                  else
                  {
                     sprintf(pStatData->value.octetString,
                             "Subscription failed");
                  }
               }
               else if ((pSubCB->noOfNetCB == 1) && (pSubCB->pRecNetCB[0]->sourceIp == 0))
               {
                  sprintf(pStatData->value.octetString, "No source IP filter");
               }
               else
               {
                  pStatData->value.octetString[0] = 0;
                  for (i = 0; (i < pSubCB->noOfNetCB) && (ret == IPT_OK) ; i++)
                  {
                     sourceIp = pSubCB->pRecNetCB[i]->sourceIp;
                     sprintf(digitstring,"%d.%d.%d.%d",
                             (sourceIp & 0xff000000) >> 24,
                             (sourceIp & 0xff0000) >> 16,
                             (sourceIp & 0xff00) >> 8,
                             sourceIp & 0xff);
                     len = strlen(digitstring);
                     if (len < leftLen)
                     {
                        strcat(pStatData->value.octetString, digitstring);
                        leftLen -= len;
                        /* Not last? */
                        if (i + 1 < pSubCB->noOfNetCB)
                        {
                           if (2 < leftLen)
                           {
                              strcat(pStatData->value.octetString, ", ");
                              leftLen -= 2;
                           }
                           else
                           {
                              ret = (int)IPT_ILLEGAL_SIZE;
                           }   
                        }
                     }
                     else
                     {
                        ret = (int)IPT_ILLEGAL_SIZE;
                     }
                  }
               }
            
               break;

            /* Status */
            case 5:
               if (pSubCB->pdCB.updatedOnce)
               {
                  if (pSubCB->pdCB.invalid)
                  {
                  sprintf(pStatData->value.octetString, "Old");
                  }
                  else
                  {
                     sprintf(pStatData->value.octetString, "Valid");
                  }
               }
               else
               {
                  sprintf(pStatData->value.octetString, "Never Received");
               }
               break;

            /* Time out value */
            case 6:
               pStatData->value.counter = pSubCB->timeout;
               break;

            /* Validity behaviour */
            case 7:
               if (pSubCB->timeout > 0)
               {
                  if (pSubCB->invalidBehaviour == 0)
                  {
                     sprintf(pStatData->value.octetString, "Zero");
                  }
                  else
                  {
                     sprintf(pStatData->value.octetString, "Keep");
                  }
               }
               else
               {
                  sprintf(pStatData->value.octetString, " ");
               }
               break;

            /* Joined multicast IP address */
            case 8:
               if (isMulticastIpAddr(pSubCB->destIp))
               {
                  sprintf(pStatData->value.octetString,"%d.%d.%d.%d",
                          (pSubCB->destIp & 0xff000000) >> 24,
                          (pSubCB->destIp & 0xff0000) >> 16,
                          (pSubCB->destIp & 0xff00) >> 8,
                          pSubCB->destIp & 0xff);
               }
               else
               {
                  sprintf(pStatData->value.octetString, " ");
               }
               break;

            default:
               found = 0;
               break;
         }
      }
      
      if(IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "getSubscriber: IPTVosGetSem ERROR\n");
   }

   if (ret == IPT_OK)
   {
      if (!found)
      {
         ret = (int)IPT_NOT_FOUND;
      }
   }
   return(ret);
}

/*******************************************************************************
NAME:     getPublisher
ABSTRACT: get publisher counters.
RETURNS:  -
*/
static int getPublisher(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int ret;
   int i;
   int mul;
   int found = 0;
   int foundKey = 0;
   UINT32 val;
   UINT32 comId;
   UINT32 destIP = 0;
   PD_SEND_NET_CB *pSendNetCB;    /* Pointer to netbuffer control block */

   comId = pStatData->oid[ind + 1];
   /* Key 1 in request? */
   if (comId != IPT_STAT_OID_STOPPER)
   {
      /* Key 2 in request? */
      mul = 0x1000000;
      for (i = 0; i < 4; i++)
      {
         val = pStatData->oid[ind + 2 + i];
         if (val != IPT_STAT_OID_STOPPER)
         {
            destIP += val * mul;
            mul = mul/0x100;
         }
         else
         {
            return((int)IPT_INVALID_PAR);
         }
      }

      ret = IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB);
         while (pSendNetCB != NULL)
         {
            if ((pSendNetCB->comId == comId) &&
                (pSendNetCB->destIp == destIP))
            {
               foundKey = 1;
               pSendNetCB = pSendNetCB->pNext;    /* Go to next */
               if (pSendNetCB != NULL)
               {
                  found = 1;
                  pStatData->oid[ind + 1] = pSendNetCB->comId;
                  pStatData->oid[ind + 2] = (pSendNetCB->destIp & 0xff000000) >> 24;
                  pStatData->oid[ind + 3] = (pSendNetCB->destIp & 0xff0000) >> 16;
                  pStatData->oid[ind + 4] = (pSendNetCB->destIp & 0xff00) >> 8;
                  pStatData->oid[ind + 5] = pSendNetCB->destIp & 0xff;
                  pStatData->oid[ind + 6] = IPT_STAT_OID_STOPPER;
                  switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = pSendNetCB->comId;
                        break;

                     case 2:
                        pStatData->value.ipAddress = pSendNetCB->destIp;
                        break;

                     case 3:
                        pStatData->value.counter = pSendNetCB->pdOutPackets;
                        break;

                     default:
                        found = 0;
                        break;
                  }
               }
               /* Break while loop */
               break;
            }

            pSendNetCB = pSendNetCB->pNext;    /* Go to next */
         }

         /* Item with the search keys has been deleted since last request? */
         if (!foundKey)
         {
            /* Answer with the first item. */
            pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB);
            if (pSendNetCB != NULL)
            {
               found = 1;
               pStatData->oid[ind + 1] = pSendNetCB->comId;
               pStatData->oid[ind + 2] = (pSendNetCB->destIp & 0xff000000) >> 24;
               pStatData->oid[ind + 3] = (pSendNetCB->destIp & 0xff0000) >> 16;
               pStatData->oid[ind + 4] = (pSendNetCB->destIp & 0xff00) >> 8;
               pStatData->oid[ind + 5] = pSendNetCB->destIp & 0xff;
               pStatData->oid[ind + 6] = IPT_STAT_OID_STOPPER;
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = pSendNetCB->comId;
                     break;

                  case 2:
                     pStatData->value.ipAddress = pSendNetCB->destIp;
                     break;

                  case 3:
                     pStatData->value.counter = pSendNetCB->pdOutPackets;
                     break;

                  default:
                     found = 0;
                     break;
               }
            }
         }
         if(IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getPublisher: IPTVosGetSem ERROR\n");
      }

   }
   else
   {
      /* Request without any key */
      ret = IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pSendNetCB = IPTGLOBAL(pd.pFirstSendNetCB);
         if (pSendNetCB != NULL)
         {
            found = 1;
            pStatData->oid[ind + 1] = pSendNetCB->comId;
            pStatData->oid[ind + 2] = (pSendNetCB->destIp & 0xff000000) >> 24;
            pStatData->oid[ind + 3] = (pSendNetCB->destIp & 0xff0000) >> 16;
            pStatData->oid[ind + 4] = (pSendNetCB->destIp & 0xff00) >> 8;
            pStatData->oid[ind + 5] = pSendNetCB->destIp & 0xff;
            pStatData->oid[ind + 6] = IPT_STAT_OID_STOPPER;
            switch (oidDig)
            {
               case 1:
                  pStatData->value.counter = pSendNetCB->comId;
                  break;

               case 2:
                  pStatData->value.ipAddress = pSendNetCB->destIp;
                  break;

               case 3:
                  pStatData->value.counter = pSendNetCB->pdOutPackets;
                  break;

               default:
                  found = 0;
                  break;
            }
         }

         if(IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getPublisher: IPTVosGetSem ERROR\n");
      }

   }

   if (found)
   {
      return((int)IPT_OK);
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }

}

/*******************************************************************************
NAME:     getPublisherGrp
ABSTRACT: Get publisher schedular group data
RETURNS:  -
*/
static int getPublisherGrp(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)             /* IN OID digit */
{
   int ret;
   int found = 0;
   int schedIx;
   UINT32 i;
   UINT32 group;
   UINT32 grpIndex;
   UINT32 foundGroup = 0;
   UINT32 foundIndex = 0;
   PD_PUB_CB *pPubCB = NULL;
   PUBGRPTAB *pSchedGrpTab = &IPTGLOBAL(pd.pubSchedGrpTab);

   group = pStatData->oid[ind + 1];

   ret = IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      /* Key 1 in request? */
      if (group != IPT_STAT_OID_STOPPER)
      {
         grpIndex = pStatData->oid[ind + 2];

         /* Key 2 not in request? */
         if (grpIndex == IPT_STAT_OID_STOPPER)
         {
            ret = (int)IPT_INVALID_PAR;
         }
         else
         {
            if (pdPubGrpTabFind(group, &schedIx))
            {
               pPubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
               i= 0;
               while ((pPubCB != NULL) && (i < grpIndex)) /*lint !e685 Expression i <= grpIndex not always true */
               {
                  i++;
                  pPubCB = pPubCB->pNext;
               }
                
               if (pPubCB != NULL)
               {
                  found = 1;
                  foundGroup = group;
                  foundIndex = i +1;    
               }
               else
               {
                  schedIx++;
                  while ((schedIx < pSchedGrpTab->nItems) && (!found))
                  {
                     pPubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
                     if (pPubCB)
                     {
                        found = 1;
                        foundGroup = pSchedGrpTab->pTable[schedIx].schedGrp;
                        foundIndex = 1;    
                     }
                     else
                     {
                        schedIx++;
                     }
                  }
               }
            }
         }
      }
      /* Request without any key */
      else
      {
         schedIx = 0;
         while ((schedIx < pSchedGrpTab->nItems) && (!found))
         {
            pPubCB = pSchedGrpTab->pTable[schedIx].pFirstCB;
            if (pPubCB)
            {
               found = 1;
               foundGroup = pSchedGrpTab->pTable[schedIx].schedGrp;
               foundIndex = 1;    
            }
            else
            {
               schedIx++;
            }
         }
      }
  
      if ((found) && (pPubCB))
      {
         pStatData->oid[ind + 1] = foundGroup;
         pStatData->oid[ind + 2] = foundIndex;
         pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
         switch (oidDig)
         {
            case 1:
               pStatData->value.counter = foundGroup;
               break;

            case 2:
               pStatData->value.counter = foundIndex;
               break;

            case 3:
               pStatData->value.counter = pPubCB->pdCB.comId;
               break;

            case 4:
               sprintf(pStatData->value.octetString,"%d.%d.%d.%d",
                       (pPubCB->destIp & 0xff000000) >> 24,
                       (pPubCB->destIp & 0xff0000) >> 16,
                       (pPubCB->destIp & 0xff00) >> 8,
                       pPubCB->destIp & 0xff);
            
               break;

            /* Cycle time */
            case 5:
               if (pPubCB->pSendNetCB)
               {
                  sprintf(pStatData->value.octetString,
                          "%d", pPubCB->pSendNetCB->cycleMultiple * IPTGLOBAL(task.pdProcCycle));
               }
               else
               {
                  sprintf(pStatData->value.octetString,
                          " ");
               }
               break;

            /* Redundant ID */
            case 6:
               if (pPubCB->pSendNetCB)
               {
                  if (pPubCB->pSendNetCB->redFuncId != 0)
                  {
                     sprintf(pStatData->value.octetString,
                             "%d", pPubCB->pSendNetCB->redFuncId);
                  }
                  else
                  {
                     sprintf(pStatData->value.octetString, " ");
                  }
               }
               else
               {
                  sprintf(pStatData->value.octetString, " ");
               }
               break;

            /* Redundant state */
            case 7:
               if (pPubCB->pSendNetCB)
               {
                  if (pPubCB->pSendNetCB->redFuncId != 0)
                  {
                     if (pPubCB->pSendNetCB->leader)
                     {
                        sprintf(pStatData->value.octetString, "Leader");
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString, "Follower");
                     }
                  }
                  else
                  {
                     sprintf(pStatData->value.octetString, " ");
                  }
               }
               else
               {
                  sprintf(pStatData->value.octetString, " ");
               }
               break;

            default:
               found = 0;
               break;
         }
      }
      
      if(IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "getSubscriber: IPTVosGetSem ERROR\n");
   }

   if (ret == IPT_OK)
   {
      if (!found)
      {
         ret = (int)IPT_NOT_FOUND;
      }
   }
   return(ret);
}

/*******************************************************************************
NAME:       getPdJoinedMcAddr
ABSTRACT:   Get joined PD multicast addresses.
RETURNS:    0 if OK, != if not
*/
static int getPdJoinedMcAddr(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int found = 0;
   int i;
   int mul;
   int ret;
   UINT32 val;
   UINT32 multiCastAddr = 0;
   NET_JOINED_MC *pTabItem;

   val = pStatData->oid[ind + 1];
   /* Key in request? */
   if (val != IPT_STAT_OID_STOPPER)
   {
      mul = 0x1000000;
      for (i = 0; i < 4; i++)
      {
         val = pStatData->oid[ind + 1 + i];
         if (val != IPT_STAT_OID_STOPPER)
         {
            multiCastAddr += val * mul;
            mul = mul/0x100;  
         }
         else
         {
            return((int)IPT_INVALID_PAR);
         }
      }
   }

   ret = IPTVosGetSem(&IPTGLOBAL(pd.recSem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {   
      pTabItem = (NET_JOINED_MC *)((void *)iptTabFindNext((IPT_TAB_HDR *)&IPTGLOBAL(net.pdJoinedMcAddrTable),
                                                          multiCastAddr)); /*lint !e826  Ignore casting warning */
      if (pTabItem)
      {
         found = 1;
         pStatData->oid[ind + 1] = (pTabItem->multiCastAddr >> 24) & 0xFF;
         pStatData->oid[ind + 2] = (pTabItem->multiCastAddr >> 16) & 0xFF;
         pStatData->oid[ind + 3] = (pTabItem->multiCastAddr >> 8)  & 0xFF;
         pStatData->oid[ind + 4] =  pTabItem->multiCastAddr        & 0xFF;
         pStatData->oid[ind + 5] = IPT_STAT_OID_STOPPER;

         switch (oidDig)
         {
            case 1:
               pStatData->value.counter = pTabItem->multiCastAddr;
               break;

            default:
               found = 0;
               break;
         }
      }
     
      if (IPTVosPutSemR(&IPTGLOBAL(pd.recSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "getPdJoinedMcAddr: IPTVosPutSem(recSem) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "getPdJoinedMcAddr: IPTVosGetSem(recSem) ERROR\n");
   }
   
   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }

}

/*******************************************************************************
NAME:       getPdRedundant
ABSTRACT:   Get PD redundant group data
RETURNS:    0 if OK, != if not
*/
static int getPdRedundant(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int ret;
   UINT32 redId;
   int found = 0;
   PD_RED_ID_ITEM *pRedIdItem;

   redId = pStatData->oid[ind + 1];

   /* No Key in request? */
   if (redId == IPT_STAT_OID_STOPPER)
   {
      redId = 0;
   }

   ret = IPTVosGetSem(&IPTGLOBAL(pd.sendSem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      pRedIdItem = (PD_RED_ID_ITEM *)(void*)iptTabFindNext(&IPTGLOBAL(pd.redIdTableHdr), redId);
      if (pRedIdItem)
      {
         found = 1;
         pStatData->oid[ind + 1] = pRedIdItem->redId;
         pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
      
         switch (oidDig)
         {
            case 1:
               pStatData->value.counter = pRedIdItem->redId;
               break;

            case 2:
               if (pRedIdItem->leader)
               {
                  sprintf(pStatData->value.octetString, "Leader");
               }
               else
               {
                  sprintf(pStatData->value.octetString, "Follower");
               }
               break;

            default:
               found = 0;
               break;
         }
      }

      if (IPTVosPutSemR(&IPTGLOBAL(pd.sendSem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSem ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}

/*******************************************************************************
NAME:       getComidListener
ABSTRACT:   Get ComId listerner.
RETURNS:    0 if OK, != if not
*/
static int getComidListener(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int ret;
   UINT32 comId;
   int found = 0;
   COMID_ITEM    *pComIdItem;

   comId = pStatData->oid[ind + 1];

   /* Key in request? */
   if (comId == IPT_STAT_OID_STOPPER)
   {
      comId = 0;
   }

   if (IPTGLOBAL(md.mdComInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr), comId));
         while ((pComIdItem) && (!found))
         {
            if ((pComIdItem->lists.pQueueList) || (pComIdItem->lists.pFuncList))
            {
               found = 1;
               pStatData->oid[ind + 1] = pComIdItem->keyComId;
               pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
            
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = pComIdItem->keyComId;
                     break;

                  case 2:
                     pStatData->value.counter = pComIdItem->lists.mdInPackets;
                     break;

                  default:
                     found = 0;
                     break;
               }
            }
            else
            {
               pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr), pComIdItem->keyComId));
            }
         }
         

         if (IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "getComidListener: IPTVosPutSem(listenerSemId) ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getComidListener: IPTVosGetSem ERROR\n");
      }
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}

/*******************************************************************************
NAME:       getComidFrgListener
ABSTRACT:   Get ComId FRG listener.
RETURNS:    0 if OK, != if not
*/
static int getComidFrgListener(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int ret;
   UINT32 comId;
   int found = 0;
   COMID_ITEM    *pComIdItem;

   comId = pStatData->oid[ind + 1];

   /* Key in request? */
   if (comId == IPT_STAT_OID_STOPPER)
   {
      comId = 0;
   }

   if (IPTGLOBAL(md.mdComInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr), comId));
         while ((pComIdItem) && (!found))
         {
            if ((pComIdItem->lists.pQueueFrgList) || (pComIdItem->lists.pFuncFrgList))
            {
               found = 1;
               pStatData->oid[ind + 1] = pComIdItem->keyComId;
               pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
            
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = pComIdItem->keyComId;
                     break;

                  case 2:
                     pStatData->value.counter = pComIdItem->lists.mdFrgInPackets;
                     break;

                  default:
                     found = 0;
                     break;
               }
            }
            else
            {
               pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr), pComIdItem->keyComId));
            }
         }
         

         if (IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "getComidListener: IPTVosPutSem(listenerSemId) ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getComidListener: IPTVosGetSem ERROR\n");
      }
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}

/*******************************************************************************
NAME:       getComidListenerDetailed
ABSTRACT:   Get ComId listerner.
RETURNS:    0 if OK, != if not
*/
static int getComidListenerDetailed(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   const char *pName;
   int ret;
   int found = 0;
   UINT32 i;
   UINT32 comId;
   UINT32 comIdIndex;
   UINT32 foundComId = 0;
   UINT32 foundIndex = 0;
   UINT32 foundType = 0;
   UINT32 foundDestIpAddr = 0;
   const void *foundCallerRef = NULL;
   MD_QUEUE foundQueueId = 0;
   IPT_REC_FUNCPTR foundFunc = NULL;
   COMID_ITEM    *pComIdItem = NULL;
   QUEUE_LIST    *pQueueList;
   QUEUE_FRG_LIST *pQueueFrgList;
   FUNC_LIST      *pFuncList;
   FUNC_FRG_LIST  *pFuncFrgList;
   FRG_ITEM *pFrgItem;
   const void   *pRedFuncRef = NULL;

   comId = pStatData->oid[ind + 1];


   if (IPTGLOBAL(md.mdComInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         /* Key in request? */
         if (comId != IPT_STAT_OID_STOPPER)
         {
            comIdIndex = pStatData->oid[ind + 2];

            /* Key 2 not in request? */
            if (comIdIndex == IPT_STAT_OID_STOPPER)
            {
               return((int)IPT_INVALID_PAR);
            }
            else
            {
               pComIdItem = (COMID_ITEM *) ((void *)iptTabFind(&IPTGLOBAL(md.listTables.comidListTableHdr),
                                                               comId)); /*lint !e826  Ignore casting warning */
            }
         }
         /* Request without any key */
         else
         {
            comId = 0;
            comIdIndex = 0;
            pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr),
                                                                comId)); /*lint !e826  Ignore casting warning */
         }

         while ((pComIdItem) && (!found))
         {
            comId = pComIdItem->keyComId;
            i = 0;
            pQueueList = pComIdItem->lists.pQueueList;
            while ((pQueueList) && (i < comIdIndex))
            {
               i++;
               pQueueList = pQueueList->pNext;  
            }

            if ((pQueueList) && (i >= comIdIndex))
            {
               found = 1;
               foundComId = comId;
               foundIndex = i;
               foundType = 1;
               foundDestIpAddr = pQueueList->destIpAddr;
               if (pQueueList->pQueue)
               {
                  foundQueueId = pQueueList->pQueue->listenerQueueId;
                  foundCallerRef = pQueueList->pQueue->pCallerRef;
               }
            }
            else
            {
               pFuncList = pComIdItem->lists.pFuncList;
               while ((pFuncList) && (i < comIdIndex))
               {
                  i++;
                  pFuncList = pFuncList->pNext;  
               }

               if ((pFuncList) && (i >= comIdIndex))
               {
                  found = 1;
                  foundComId = comId;
                  foundIndex = i;    
                  foundType = 2;
                  foundDestIpAddr = pFuncList->destIpAddr;
                  if (pFuncList->pFunc)
                  {
                     foundFunc = pFuncList->pFunc->func;
                     foundCallerRef = pFuncList->pFunc->pCallerRef;
                  }
               }
               else
               {
                  pQueueFrgList = pComIdItem->lists.pQueueFrgList;
                  while ((pQueueFrgList) && (i < comIdIndex))
                  {
                     i++;
                     pQueueFrgList = pQueueFrgList->pNext;  
                  }

                  if ((pQueueFrgList) && (i >= comIdIndex))
                  {
                     found = 1;
                     foundComId = comId;
                     foundIndex = i;
                     foundType = 3;
                     foundDestIpAddr = pQueueFrgList->destIpAddr;
                     pRedFuncRef = pQueueFrgList->pRedFuncRef;
                     if (pQueueFrgList->pQueue)
                     {
                        foundQueueId = pQueueFrgList->pQueue->listenerQueueId;    
                        foundCallerRef = pQueueFrgList->pQueue->pCallerRef;
                     }
                  }
                  else
                  {
                     pFuncFrgList = pComIdItem->lists.pFuncFrgList;
                     while ((pFuncFrgList) && (i < comIdIndex))
                     {
                        i++;
                        pFuncFrgList = pFuncFrgList->pNext;  
                     }

                     if ((pFuncFrgList) && (i >= comIdIndex))
                     {
                        found = 1;
                        foundComId = comId;
                        foundIndex = i;    
                        foundType = 4;
                        foundDestIpAddr = pFuncFrgList->destIpAddr;
                        pRedFuncRef = pFuncFrgList->pRedFuncRef;
                        if (pFuncFrgList->pFunc)
                        {
                           foundFunc = pFuncFrgList->pFunc->func;
                           foundCallerRef = pFuncFrgList->pFunc->pCallerRef;
                        }
                     }
                     else
                     {
                        comIdIndex = 0;
                        pComIdItem = (COMID_ITEM *) ((void *)iptTabFindNext(&IPTGLOBAL(md.listTables.comidListTableHdr), comId));
                     }
                  }
               }
            }
         }
         
         if (found)
         {
            pStatData->oid[ind + 1] = foundComId;
            pStatData->oid[ind + 2] = foundIndex + 1;
            pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
         
            switch (oidDig)
            {
               case 1:
                  pStatData->value.counter = foundComId;
                  break;

               case 2:
                  pStatData->value.counter = foundIndex + 1;
                  break;

               case 3:
                  switch (foundType)
                  {
                     case 1:
                        sprintf(pStatData->value.octetString, "Q");
                        break;
                  
                     case 2:
                        sprintf(pStatData->value.octetString, "F");
                        break;
                  
                     case 3:
                        sprintf(pStatData->value.octetString, "FRGQ");
                        break;
                  
                     case 4:
                        sprintf(pStatData->value.octetString, "FRGF");
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;
               
               case 4:
                  switch (foundType)
                  {
                     case 1:
                     case 3:
                        sprintf(pStatData->value.octetString, "%#x", foundQueueId);
                        break;
                  
                     case 2:
                     case 4:
                        sprintf(pStatData->value.octetString, "%#x", (unsigned int)foundFunc);
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;
               
               case 5:
                  switch (foundType)
                  {
                     case 1:
                     case 3:
                        pName = getQueueItemName(foundQueueId);
                        sprintf(pStatData->value.octetString, "%s", (pName != NULL)?pName:" ");
                        break;
                  
                     case 2:
                     case 4:
                        sprintf(pStatData->value.octetString, " ");
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;
               
               case 6:
                  sprintf(pStatData->value.octetString, "%#x", (unsigned int)foundCallerRef);
                  break;
               
               case 7:
                  switch (foundType)
                  {
                     case 1:
                     case 2:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  
                     case 3:
                     case 4:
                        sprintf(pStatData->value.octetString, "%#x", (unsigned int)pRedFuncRef);
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;
               
               case 8:
                  switch (foundType)
                  {
                     case 1:
                     case 2:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  
                     case 3:
                     case 4:
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pRedFuncRef)); /*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              sprintf(pStatData->value.octetString, "Leader");
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString, "Follower");
                           }
                        }
                        else
                        {
                           sprintf(pStatData->value.octetString, " ");
                        }
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;

               case 9:
                  if (isMulticastIpAddr(foundDestIpAddr))
                  {
                     sprintf(pStatData->value.octetString,"%d.%d.%d.%d",
                             (foundDestIpAddr & 0xff000000) >> 24,
                             (foundDestIpAddr & 0xff0000) >> 16,
                             (foundDestIpAddr & 0xff00) >> 8,
                             foundDestIpAddr & 0xff);
                  }
                  else
                  {
                     sprintf(pStatData->value.octetString, " ");
                  }
                  break;
               
               default:
                  found = 0;
                  break;
            }
         }
         
         if (IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "getComidListenerDetailed: IPTVosPutSem(listenerSemId) ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getComidListenerDetailed: IPTVosGetSem ERROR\n");
      }
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}

/*******************************************************************************
NAME:       getUriListener
ABSTRACT:   Get URI listerner.
RETURNS:    0 if OK, != if not
*/
static int getUriListener(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int ret;
   UINT16 nItems = 0;
   UINT32 nItemsTot = 0;
   UINT32 mdUriIndex;
   UINT32 uriIndex;
   int found = 0;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;


   mdUriIndex = pStatData->oid[ind + 1];
   /* Key in request? */
   if (mdUriIndex == IPT_STAT_OID_STOPPER)
   {
      mdUriIndex = 0;
   }
   if (IPTGLOBAL(md.mdComInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pInstXFuncN = (INSTX_FUNCN_ITEM *)IPTGLOBAL(md.listTables.instXFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
         if (pInstXFuncN)
         {
            uriIndex = mdUriIndex;
            nItems = IPTGLOBAL(md.listTables.instXFuncNListTableHdr.nItems);
            nItemsTot = nItems;
            while ((!found) && (uriIndex < nItems)) 
            {
               if ((pInstXFuncN[uriIndex].lists.pQueueList) ||
                   (pInstXFuncN[uriIndex].lists.pFuncList))
               {
                  found = 1;
                  pStatData->oid[ind + 1] = mdUriIndex +1;
                  pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;

                  switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = mdUriIndex +1;
                        break;

                     case 2:
                        sprintf(pStatData->value.octetString,
                                "%s.%s", 
                                pInstXFuncN[uriIndex].instName, 
                                pInstXFuncN[uriIndex].funcName);
                        break;

                     case 3:
                        pStatData->value.counter = pInstXFuncN[uriIndex].lists.mdInPackets;
                        break;

                     default:
                        found = 0;
                        break;
                  }

               }
               else
               {
                  uriIndex++;
                  mdUriIndex++;   
               }
            }
         }
         
         if (!found)
         {
            pAinstFuncN = (AINST_FUNCN_ITEM *)IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
            if (pAinstFuncN)
            {
               uriIndex = mdUriIndex - nItemsTot;
               nItems = IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.nItems);
               nItemsTot += nItems;  
               while ((!found) && (uriIndex < nItems))
               {
                  if ((pAinstFuncN[uriIndex].lists.pQueueList) ||
                      (pAinstFuncN[uriIndex].lists.pFuncList))
                  {
                     found = 1;
                     pStatData->oid[ind + 1] = mdUriIndex +1;
                     pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;

                     switch (oidDig)
                     {
                        case 1:
                           pStatData->value.counter = mdUriIndex +1;
                           break;

                        case 2:
                           sprintf(pStatData->value.octetString,
                                   "aInst.%s", 
                                   pAinstFuncN[uriIndex].funcName);
                           break;

                        case 3:
                           pStatData->value.counter = pAinstFuncN[uriIndex].lists.mdInPackets;
                           break;

                        default:
                           found = 0;
                           break;
                     }
                  }
                  else
                  {
                     uriIndex++;
                     mdUriIndex++;   
                  }  
               } 
            }
         }

         if (!found)
         {
            pInstXaFunc = (INSTX_AFUNC_ITEM *)IPTGLOBAL(md.listTables.instXaFuncListTableHdr.pTable);/*lint !e826 Type cast OK */
            if (pInstXaFunc)
            {
               uriIndex = mdUriIndex - nItemsTot;
               nItems = IPTGLOBAL(md.listTables.instXaFuncListTableHdr.nItems);
               nItemsTot += nItems;  
               while ((!found) && (uriIndex < nItems))
               {
                  if ((pInstXaFunc[uriIndex].lists.pQueueList) ||
                      (pInstXaFunc[uriIndex].lists.pFuncList))
                  {
                     found = 1;
                     pStatData->oid[ind + 1] = mdUriIndex +1;
                     pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;

                     switch (oidDig)
                     {
                        case 1:
                           pStatData->value.counter = mdUriIndex +1;
                           break;

                        case 2:
                           sprintf(pStatData->value.octetString,
                                   "%s.aFunc", 
                                   pInstXaFunc[uriIndex].instName);
                           break;

                        case 3:
                           pStatData->value.counter = pInstXaFunc[uriIndex].lists.mdInPackets;
                           break;

                        default:
                           found = 0;
                           break;
                     }
                  }
                  else
                  {
                     uriIndex++;
                     mdUriIndex++;   
                  }  
               }
            }
         }

         if (!found)
         {
            uriIndex = mdUriIndex - nItemsTot;
            if (uriIndex < 1 )
            {
               if ((IPTGLOBAL(md.listTables.aInstAfunc).pQueueList) ||
                   (IPTGLOBAL(md.listTables.aInstAfunc).pFuncList))
               {
                  found = 1;
                  pStatData->oid[ind + 1] = mdUriIndex +1;
                  pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;

                  switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = mdUriIndex +1;
                        break;

                     case 2:
                        sprintf(pStatData->value.octetString,
                                "aInst.aFunc");
                        break;

                     case 3:
                        pStatData->value.counter = IPTGLOBAL(md.listTables.aInstAfunc).mdInPackets;
                        break;

                     default:
                        found = 0;
                        break;
                  }
               }
            }
         } 
            
         if (IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "getUriListener: IPTVosPutSem(listenerSemId) ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getUriListener: IPTVosGetSem ERROR\n");
      }
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}

/*******************************************************************************
NAME:       getUriFrgListener
ABSTRACT:   Get URI FRG listerner.
RETURNS:    0 if OK, != if not
*/
static int getUriFrgListener(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int ret;
   UINT16 nItems = 0;
   UINT32 nItemsTot = 0;
   UINT32 uriIndex;
   UINT32 mdUriIndex;
   int found = 0;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;


   mdUriIndex = pStatData->oid[ind + 1];
   /* Key in request? */
   if (mdUriIndex == IPT_STAT_OID_STOPPER)
   {
      mdUriIndex = 0;
   }
   
   if (IPTGLOBAL(md.mdComInitiated))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         pInstXFuncN = (INSTX_FUNCN_ITEM *)IPTGLOBAL(md.listTables.instXFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
         if (pInstXFuncN)
         {
            uriIndex = mdUriIndex;
            nItems = IPTGLOBAL(md.listTables.instXFuncNListTableHdr.nItems);
            nItemsTot = nItems;
            while ((!found) && (uriIndex < nItems)) 
            {
               if ((pInstXFuncN[uriIndex].lists.pQueueFrgList) ||
                   (pInstXFuncN[uriIndex].lists.pFuncFrgList))
               {
                  found = 1;
                  pStatData->oid[ind + 1] = mdUriIndex +1;
                  pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;

                  switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = mdUriIndex +1;
                        break;

                     case 2:
                        sprintf(pStatData->value.octetString,
                                "%s.%s", 
                                pInstXFuncN[uriIndex].instName, 
                                pInstXFuncN[uriIndex].funcName);
                        break;

                     case 3:
                        pStatData->value.counter = pInstXFuncN[uriIndex].lists.mdFrgInPackets;
                        break;

                     default:
                        found = 0;
                        break;
                  }

               }
               else
               {
                  uriIndex++;
                  mdUriIndex++;   
               }
            }
         }
         
         if (!found)
         {
            pAinstFuncN = (AINST_FUNCN_ITEM *)IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
            if (pAinstFuncN)
            {
               uriIndex = mdUriIndex - nItemsTot;
               nItems = IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.nItems);
               nItemsTot += nItems;  
               while ((!found) && (uriIndex < nItems))
               {
                  if ((pAinstFuncN[uriIndex].lists.pQueueFrgList) ||
                      (pAinstFuncN[uriIndex].lists.pFuncFrgList))
                  {
                     found = 1;
                     pStatData->oid[ind + 1] = mdUriIndex +1;
                     pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;

                     switch (oidDig)
                     {
                        case 1:
                           pStatData->value.counter = mdUriIndex +1;
                           break;

                        case 2:
                           sprintf(pStatData->value.octetString,
                                   "aInst.%s", 
                                   pAinstFuncN[uriIndex].funcName);
                           break;

                        case 3:
                           pStatData->value.counter = pAinstFuncN[uriIndex].lists.mdFrgInPackets;
                           break;

                        default:
                           found = 0;
                           break;
                     }
                  }
                  else
                  {
                     uriIndex++;
                     mdUriIndex++;   
                  }  
               } 
            }
         }

         if (!found)
         {
            pInstXaFunc = (INSTX_AFUNC_ITEM *)IPTGLOBAL(md.listTables.instXaFuncListTableHdr.pTable);/*lint !e826 Type cast OK */
            if (pInstXaFunc)
            {
               uriIndex = mdUriIndex - nItemsTot;
               nItems = IPTGLOBAL(md.listTables.instXaFuncListTableHdr.nItems);
               nItemsTot += nItems;  
               while ((!found) && (uriIndex < nItems))
               {
                  if ((pInstXaFunc[uriIndex].lists.pQueueFrgList) ||
                      (pInstXaFunc[uriIndex].lists.pFuncFrgList))
                  {
                     found = 1;
                     pStatData->oid[ind + 1] = mdUriIndex +1;
                     pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;

                     switch (oidDig)
                     {
                        case 1:
                           pStatData->value.counter = mdUriIndex +1;
                           break;

                        case 2:
                           sprintf(pStatData->value.octetString,
                                   "%s.aFunc", 
                                   pInstXaFunc[uriIndex].instName);
                           break;

                        case 3:
                           pStatData->value.counter = pInstXaFunc[uriIndex].lists.mdFrgInPackets;
                           break;

                        default:
                           found = 0;
                           break;
                     }
                  }
                  else
                  {
                     uriIndex++;
                     mdUriIndex++;   
                  }  
               }
            }
         }

         if (!found)
         {
            uriIndex = mdUriIndex - nItemsTot;
            if (uriIndex < 1 )
            {
               if ((IPTGLOBAL(md.listTables.aInstAfunc).pQueueFrgList) ||
                   (IPTGLOBAL(md.listTables.aInstAfunc).pFuncFrgList))
               {
                  found = 1;
                  pStatData->oid[ind + 1] = mdUriIndex +1;
                  pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;

                  switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = mdUriIndex +1;
                        break;

                     case 2:
                        sprintf(pStatData->value.octetString,
                                "aInst.aFunc");
                        break;

                     case 3:
                        pStatData->value.counter = IPTGLOBAL(md.listTables.aInstAfunc).mdFrgInPackets;
                        break;

                     default:
                        found = 0;
                        break;
                  }
               }
            }
         } 
            
         if (IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "getUriListener: IPTVosPutSem(listenerSemId) ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getUriListener: IPTVosGetSem ERROR\n");
      }
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}

/*******************************************************************************
NAME:       getUriListenerDetailed
ABSTRACT:   Get URI listerner.
RETURNS:    0 if OK, != if not
*/
static int getUriListenerDetailed(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   const char *pName;
   const char *pInstName = NULL;
   const char *pFuncName = NULL;
   int ret = IPT_OK;
   int found = 0;
   UINT16 nItems = 0;
   UINT32 nItemsTot = 0;
   UINT32 i;
   UINT32 mdUriIndex;
   UINT32 uriIndex;
   UINT32 listenerIndex;
   UINT32 foundUriIndex = 0;
   UINT32 foundListenerIndex = 0;
   UINT32 foundType = 0;
   UINT32 foundDestIpAddr = 0;
   const void *foundCallerRef = NULL;
   MD_QUEUE foundQueueId = 0;
   IPT_REC_FUNCPTR foundFunc = NULL;
   INSTX_FUNCN_ITEM *pInstXFuncN;
   AINST_FUNCN_ITEM *pAinstFuncN;
   INSTX_AFUNC_ITEM *pInstXaFunc;
   QUEUE_LIST    *pQueueList;
   QUEUE_FRG_LIST *pQueueFrgList;
   FUNC_LIST      *pFuncList;
   FUNC_FRG_LIST  *pFuncFrgList;
   FRG_ITEM *pFrgItem;
   const void   *pRedFuncRef = NULL;

   mdUriIndex = pStatData->oid[ind + 1];

   /* Key in request? */
   if (mdUriIndex != IPT_STAT_OID_STOPPER)
   {
      listenerIndex = pStatData->oid[ind + 2];

      /* Key 2 not in request? */
      if (listenerIndex == IPT_STAT_OID_STOPPER)
      {
         ret = (int)IPT_INVALID_PAR;
      }
   }
   /* Request without any key */
   else
   {
      mdUriIndex = 1;
      listenerIndex = 0;
   }

   if ((IPTGLOBAL(md.mdComInitiated)) && (ret == IPT_OK))
   {
      ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
      if (ret == IPT_OK)
      {
         nItems = IPTGLOBAL(md.listTables.instXFuncNListTableHdr.nItems);
         nItemsTot = nItems;
         if (mdUriIndex <= nItems )
         {
            pInstXFuncN = (INSTX_FUNCN_ITEM *)IPTGLOBAL(md.listTables.instXFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
            if (pInstXFuncN)
            {
               uriIndex = mdUriIndex - 1;
               
               while ((!found) && (uriIndex < nItems)) 
               {
                  i = 0;
                  pQueueList = pInstXFuncN[uriIndex].lists.pQueueList;
                  while ((pQueueList) && (i < listenerIndex))
                  {
                     i++;
                     pQueueList = pQueueList->pNext;  
                  }

                  if ((pQueueList) && (i >= listenerIndex))
                  {
                     found = 1;
                     foundUriIndex = mdUriIndex;
                     foundListenerIndex = i;
                     pInstName = pInstXFuncN[uriIndex].instName;
                     pFuncName = pInstXFuncN[uriIndex].funcName;
                     foundType = 1;
                     foundDestIpAddr = pQueueList->destIpAddr;
                     if (pQueueList->pQueue)
                     {
                        foundQueueId = pQueueList->pQueue->listenerQueueId;
                        foundCallerRef = pQueueList->pQueue->pCallerRef;
                     }
                  }
                  else
                  {
                     pFuncList = pInstXFuncN[uriIndex].lists.pFuncList;
                     while ((pFuncList) && (i < listenerIndex))
                     {
                        i++;
                        pFuncList = pFuncList->pNext;  
                     }

                     if ((pFuncList) && (i >= listenerIndex))
                     {
                        found = 1;
                        foundUriIndex = mdUriIndex;
                        foundListenerIndex = i;    
                        pInstName = pInstXFuncN[uriIndex].instName;
                        pFuncName = pInstXFuncN[uriIndex].funcName;
                        foundType = 2;
                        foundDestIpAddr = pFuncList->destIpAddr;
                        if (pFuncList->pFunc)
                        {
                           foundFunc = pFuncList->pFunc->func;
                           foundCallerRef = pFuncList->pFunc->pCallerRef;
                        }
                     }
                     else
                     {
                        pQueueFrgList = pInstXFuncN[uriIndex].lists.pQueueFrgList;
                        while ((pQueueFrgList) && (i < listenerIndex))
                        {
                           i++;
                           pQueueFrgList = pQueueFrgList->pNext;  
                        }

                        if ((pQueueFrgList) && (i >= listenerIndex))
                        {
                           found = 1;
                           foundUriIndex = mdUriIndex;
                           foundListenerIndex = i;
                           pInstName = pInstXFuncN[uriIndex].instName;
                           pFuncName = pInstXFuncN[uriIndex].funcName;
                           foundType = 3;
                           foundDestIpAddr = pQueueFrgList->destIpAddr;
                           pRedFuncRef = pQueueFrgList->pRedFuncRef;
                           if (pQueueFrgList->pQueue)
                           {
                              foundQueueId = pQueueFrgList->pQueue->listenerQueueId;    
                              foundCallerRef = pQueueFrgList->pQueue->pCallerRef;
                           }
                        }
                        else
                        {
                           pFuncFrgList = pInstXFuncN[uriIndex].lists.pFuncFrgList;
                           while ((pFuncFrgList) && (i < listenerIndex))
                           {
                              i++;
                              pFuncFrgList = pFuncFrgList->pNext;  
                           }

                           if ((pFuncFrgList) && (i >= listenerIndex))
                           {
                              found = 1;
                              foundUriIndex = mdUriIndex;
                              foundListenerIndex = i;    
                              pInstName = pInstXFuncN[uriIndex].instName;
                              pFuncName = pInstXFuncN[uriIndex].funcName;
                              foundType = 4;
                              foundDestIpAddr = pFuncFrgList->destIpAddr;
                              pRedFuncRef = pFuncFrgList->pRedFuncRef;
                              if (pFuncFrgList->pFunc)
                              {
                                 foundFunc = pFuncFrgList->pFunc->func;
                                 foundCallerRef = pFuncFrgList->pFunc->pCallerRef;
                              }
                           }
                           else
                           {
                              uriIndex++;
                              mdUriIndex++;
                              listenerIndex = 0;   
                           }
                        }
                     }
                  }
               }
            }
         }

         if (!found)
         {
            nItems = IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.nItems);
            nItemsTot += nItems;
            
            if (mdUriIndex <= nItemsTot)
            {
               uriIndex = mdUriIndex - (nItemsTot - nItems) - 1;
          
               pAinstFuncN = (AINST_FUNCN_ITEM *)IPTGLOBAL(md.listTables.aInstFuncNListTableHdr.pTable);/*lint !e826 Type cast OK */
               if (pAinstFuncN)
               {
                  while ((!found) && (uriIndex < nItems)) 
                  {
                     i = 0;
                     pQueueList = pAinstFuncN[uriIndex].lists.pQueueList;
                     while ((pQueueList) && (i < listenerIndex))
                     {
                        i++;
                        pQueueList = pQueueList->pNext;  
                     }

                     if ((pQueueList) && (i >= listenerIndex))
                     {
                        found = 1;
                        foundUriIndex = mdUriIndex;
                        foundListenerIndex = i;
                        pInstName = "aInst";
                        pFuncName = pAinstFuncN[uriIndex].funcName;
                        foundType = 1;
                        foundDestIpAddr = pQueueList->destIpAddr;
                        if (pQueueList->pQueue)
                        {
                           foundQueueId = pQueueList->pQueue->listenerQueueId;
                           foundCallerRef = pQueueList->pQueue->pCallerRef;
                        }
                     }
                     else
                     {
                        pFuncList = pAinstFuncN[uriIndex].lists.pFuncList;
                        while ((pFuncList) && (i < listenerIndex))
                        {
                           i++;
                           pFuncList = pFuncList->pNext;  
                        }

                        if ((pFuncList) && (i >= listenerIndex))
                        {
                           found = 1;
                           foundUriIndex = mdUriIndex;
                           foundListenerIndex = i;    
                           pInstName = "aInst";
                           pFuncName = pAinstFuncN[uriIndex].funcName;
                           foundType = 2;
                           foundDestIpAddr = pFuncList->destIpAddr;
                           if (pFuncList->pFunc)
                           {
                              foundFunc = pFuncList->pFunc->func;
                              foundCallerRef = pFuncList->pFunc->pCallerRef;
                           }
                        }
                        else
                        {
                           pQueueFrgList = pAinstFuncN[uriIndex].lists.pQueueFrgList;
                           while ((pQueueFrgList) && (i < listenerIndex))
                           {
                              i++;
                              pQueueFrgList = pQueueFrgList->pNext;  
                           }

                           if ((pQueueFrgList) && (i >= listenerIndex))
                           {
                              found = 1;
                              foundUriIndex = mdUriIndex;
                              foundListenerIndex = i;
                              pInstName = "aInst";
                              pFuncName = pAinstFuncN[uriIndex].funcName;
                              foundType = 3;
                              foundDestIpAddr = pQueueFrgList->destIpAddr;
                              pRedFuncRef = pQueueFrgList->pRedFuncRef;
                              if (pQueueFrgList->pQueue)
                              {
                                 foundQueueId = pQueueFrgList->pQueue->listenerQueueId;    
                                 foundCallerRef = pQueueFrgList->pQueue->pCallerRef;
                              }
                           }
                           else
                           {
                              pFuncFrgList = pAinstFuncN[uriIndex].lists.pFuncFrgList;
                              while ((pFuncFrgList) && (i < listenerIndex))
                              {
                                 i++;
                                 pFuncFrgList = pFuncFrgList->pNext;  
                              }

                              if ((pFuncFrgList) && (i >= listenerIndex))
                              {
                                 found = 1;
                                 foundUriIndex = mdUriIndex;
                                 foundListenerIndex = i;    
                                 pInstName = "aInst";
                                 pFuncName = pAinstFuncN[uriIndex].funcName;
                                 foundType = 4;
                                 foundDestIpAddr = pFuncFrgList->destIpAddr;
                                 pRedFuncRef = pFuncFrgList->pRedFuncRef;
                                 if (pFuncFrgList->pFunc)
                                 {
                                    foundFunc = pFuncFrgList->pFunc->func;
                                    foundCallerRef = pFuncFrgList->pFunc->pCallerRef;
                                 }
                              }
                              else
                              {
                                 uriIndex++;
                                 mdUriIndex++;
                                 listenerIndex = 0;   
                              }
                           }
                        }
                     }
                  }
               }
            }
         }

         if (!found)
         {
            nItems = IPTGLOBAL(md.listTables.instXaFuncListTableHdr.nItems);
            nItemsTot += nItems;
            
            if (mdUriIndex <= nItemsTot)
            {
               uriIndex = mdUriIndex - (nItemsTot - nItems) - 1;
          
               pInstXaFunc = (INSTX_AFUNC_ITEM *)IPTGLOBAL(md.listTables.instXaFuncListTableHdr.pTable);/*lint !e826 Type cast OK */
               if (pInstXaFunc)
               {
                  while ((!found) && (uriIndex < nItems)) 
                  {
                     i = 0;
                     pQueueList = pInstXaFunc[uriIndex].lists.pQueueList;
                     while ((pQueueList) && (i < listenerIndex))
                     {
                        i++;
                        pQueueList = pQueueList->pNext;  
                     }

                     if ((pQueueList) && (i >= listenerIndex))
                     {
                        found = 1;
                        foundUriIndex = mdUriIndex;
                        foundListenerIndex = i;
                        pInstName = pInstXaFunc[uriIndex].instName;
                        pFuncName = "aFunc";
                        foundType = 1;
                        foundDestIpAddr = pQueueList->destIpAddr;
                        if (pQueueList->pQueue)
                        {
                           foundQueueId = pQueueList->pQueue->listenerQueueId;
                           foundCallerRef = pQueueList->pQueue->pCallerRef;
                        }
                     }
                     else
                     {
                        pFuncList = pInstXaFunc[uriIndex].lists.pFuncList;
                        while ((pFuncList) && (i < listenerIndex))
                        {
                           i++;
                           pFuncList = pFuncList->pNext;  
                        }

                        if ((pFuncList) && (i >= listenerIndex))
                        {
                           found = 1;
                           foundUriIndex = mdUriIndex;
                           foundListenerIndex = i;    
                           pInstName = pInstXaFunc[uriIndex].instName;
                           pFuncName = "aFunc";
                           foundType = 2;
                           foundDestIpAddr = pFuncList->destIpAddr;
                           if (pFuncList->pFunc)
                           {
                              foundFunc = pFuncList->pFunc->func;
                              foundCallerRef = pFuncList->pFunc->pCallerRef;
                           }
                        }
                        else
                        {
                           pQueueFrgList = pInstXaFunc[uriIndex].lists.pQueueFrgList;
                           while ((pQueueFrgList) && (i < listenerIndex))
                           {
                              i++;
                              pQueueFrgList = pQueueFrgList->pNext;  
                           }

                           if ((pQueueFrgList) && (i >= listenerIndex))
                           {
                              found = 1;
                              foundUriIndex = mdUriIndex;
                              foundListenerIndex = i;
                              pInstName = pInstXaFunc[uriIndex].instName;
                              pFuncName = "aFunc";
                              foundType = 3;
                              foundDestIpAddr = pQueueFrgList->destIpAddr;
                              pRedFuncRef = pQueueFrgList->pRedFuncRef;
                              if (pQueueFrgList->pQueue)
                              {
                                 foundQueueId = pQueueFrgList->pQueue->listenerQueueId;    
                                 foundCallerRef = pQueueFrgList->pQueue->pCallerRef;
                              }
                           }
                           else
                           {
                              pFuncFrgList = pInstXaFunc[uriIndex].lists.pFuncFrgList;
                              while ((pFuncFrgList) && (i < listenerIndex))
                              {
                                 i++;
                                 pFuncFrgList = pFuncFrgList->pNext;  
                              }

                              if ((pFuncFrgList) && (i >= listenerIndex))
                              {
                                 found = 1;
                                 foundUriIndex = mdUriIndex;
                                 foundListenerIndex = i;    
                                 pInstName = pInstXaFunc[uriIndex].instName;
                                 pFuncName = "aFunc";
                                 foundType = 4;
                                 foundDestIpAddr = pFuncFrgList->destIpAddr;
                                 pRedFuncRef = pFuncFrgList->pRedFuncRef;
                                 if (pFuncFrgList->pFunc)
                                 {
                                    foundFunc = pFuncFrgList->pFunc->func;
                                    foundCallerRef = pFuncFrgList->pFunc->pCallerRef;
                                 }
                              }
                              else
                              {
                                 uriIndex++;
                                 mdUriIndex++;
                                 listenerIndex = 0;   
                              }
                           }
                        }
                     }
                  }
               }
            }
         }

         if (!found)
         {
            uriIndex = mdUriIndex - nItemsTot - 1;
            if (uriIndex < 1 )
            {
               i = 0;
               pQueueList = IPTGLOBAL(md.listTables.aInstAfunc).pQueueList;
               while ((pQueueList) && (i < listenerIndex))
               {
                  i++;
                  pQueueList = pQueueList->pNext;  
               }

               if ((pQueueList) && (i >= listenerIndex))
               {
                  found = 1;
                  foundUriIndex = mdUriIndex;
                  foundListenerIndex = i;
                  pInstName = "aInst";
                  pFuncName = "aFunc";
                  foundType = 1;
                  foundDestIpAddr = pQueueList->destIpAddr;
                  if (pQueueList->pQueue)
                  {
                     foundQueueId = pQueueList->pQueue->listenerQueueId;
                     foundCallerRef = pQueueList->pQueue->pCallerRef;
                  }
               }
               else
               {
                  pFuncList = IPTGLOBAL(md.listTables.aInstAfunc).pFuncList;
                  while ((pFuncList) && (i < listenerIndex))
                  {
                     i++;
                     pFuncList = pFuncList->pNext;  
                  }

                  if ((pFuncList) && (i >= listenerIndex))
                  {
                     found = 1;
                     foundUriIndex = mdUriIndex;
                     foundListenerIndex = i;    
                     pInstName = "aInst";
                     pFuncName = "aFunc";
                     foundType = 2;
                     foundDestIpAddr = pFuncList->destIpAddr;
                     if (pFuncList->pFunc)
                     {
                        foundFunc = pFuncList->pFunc->func;
                        foundCallerRef = pFuncList->pFunc->pCallerRef;
                     }
                  }
                  else
                  {
                     pQueueFrgList = IPTGLOBAL(md.listTables.aInstAfunc).pQueueFrgList;
                     while ((pQueueFrgList) && (i < listenerIndex))
                     {
                        i++;
                        pQueueFrgList = pQueueFrgList->pNext;  
                     }

                     if ((pQueueFrgList) && (i >= listenerIndex))
                     {
                        found = 1;
                        foundUriIndex = mdUriIndex;
                        foundListenerIndex = i;
                        pInstName = "aInst";
                        pFuncName = "aFunc";
                        foundType = 3;
                        foundDestIpAddr = pQueueFrgList->destIpAddr;
                        pRedFuncRef = pQueueFrgList->pRedFuncRef;
                        if (pQueueFrgList->pQueue)
                        {
                           foundQueueId = pQueueFrgList->pQueue->listenerQueueId;    
                           foundCallerRef = pQueueFrgList->pQueue->pCallerRef;
                        }
                     }
                     else
                     {
                        pFuncFrgList = IPTGLOBAL(md.listTables.aInstAfunc).pFuncFrgList;
                        while ((pFuncFrgList) && (i < listenerIndex))
                        {
                           i++;
                           pFuncFrgList = pFuncFrgList->pNext;  
                        }

                        if ((pFuncFrgList) && (i >= listenerIndex))
                        {
                           found = 1;
                           foundUriIndex = mdUriIndex;
                           foundListenerIndex = i;    
                           pInstName = "aInst";
                           pFuncName = "aFunc";
                           foundType = 4;
                           foundDestIpAddr = pFuncFrgList->destIpAddr;
                           pRedFuncRef = pFuncFrgList->pRedFuncRef;
                           if (pFuncFrgList->pFunc)
                           {
                              foundFunc = pFuncFrgList->pFunc->func;
                              foundCallerRef = pFuncFrgList->pFunc->pCallerRef;
                           }
                        }
                     }
                  }
               }
            }
         }

         if (found)
         {
            pStatData->oid[ind + 1] = foundUriIndex;
            pStatData->oid[ind + 2] = foundListenerIndex + 1;
            pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
         
            switch (oidDig)
            {
               case 1:
                  pStatData->value.counter = foundUriIndex;
                  break;

               case 2:
                  pStatData->value.counter = foundListenerIndex + 1;
                  break;

               case 3:
                  sprintf(pStatData->value.octetString,
                          "%s.%s", 
                          pInstName, 
                          pFuncName);
                  break;

               case 4:
                  switch (foundType)
                  {
                     case 1:
                        sprintf(pStatData->value.octetString, "Q");
                        break;
                  
                     case 2:
                        sprintf(pStatData->value.octetString, "F");
                        break;
                  
                     case 3:
                        sprintf(pStatData->value.octetString, "FRGQ");
                        break;
                  
                     case 4:
                        sprintf(pStatData->value.octetString, "FRGF");
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;
               
               case 5:
                  switch (foundType)
                  {
                     case 1:
                     case 3:
                        sprintf(pStatData->value.octetString, "%#x", foundQueueId);
                        break;
                  
                     case 2:
                     case 4:
                        sprintf(pStatData->value.octetString, "%#x", (unsigned int)foundFunc);
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;
               
               case 6:
                  switch (foundType)
                  {
                     case 1:
                     case 3:
                        pName = getQueueItemName(foundQueueId);
                        sprintf(pStatData->value.octetString, "%s", (pName != NULL)?pName:" ");
                        break;
                  
                     case 2:
                     case 4:
                        sprintf(pStatData->value.octetString, " ");
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;
               
               case 7:
                  sprintf(pStatData->value.octetString, "%#x", (unsigned int)foundCallerRef);
                  break;
               
               case 8:
                  switch (foundType)
                  {
                     case 1:
                     case 2:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  
                     case 3:
                     case 4:
                        sprintf(pStatData->value.octetString, "%#x", (unsigned int)pRedFuncRef);
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;
               
               case 9:
                  switch (foundType)
                  {
                     case 1:
                     case 2:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  
                     case 3:
                     case 4:
                        pFrgItem = (FRG_ITEM *)((void *)iptTabFind(&IPTGLOBAL(md.frgTableHdr),
                                                                   (UINT32)pRedFuncRef)); /*lint !e826  Ignore casting warning */
                        if (pFrgItem)
                        {
                           if (pFrgItem->frgState == FRG_LEADER)
                           {
                              sprintf(pStatData->value.octetString, "Leader");
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString, "Follower");
                           }
                        }
                        else
                        {
                           sprintf(pStatData->value.octetString, " ");
                        }
                        break;

                     default:
                        sprintf(pStatData->value.octetString, " ");
                        break;
                  }
                  break;

               case 10:
                  if (isMulticastIpAddr(foundDestIpAddr))
                  {
                     sprintf(pStatData->value.octetString,"%d.%d.%d.%d",
                             (foundDestIpAddr & 0xff000000) >> 24,
                             (foundDestIpAddr & 0xff0000) >> 16,
                             (foundDestIpAddr & 0xff00) >> 8,
                             foundDestIpAddr & 0xff);
                  }
                  else
                  {
                     sprintf(pStatData->value.octetString, " ");
                  }
                  break;
               
               default:
                  found = 0;
                  break;
            }
         }
         

         if (IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
         {
            IPTVosPrint0(IPT_ERR, "getUriListenerDetailed: IPTVosPutSem(listenerSemId) ERROR\n");
         }
      }
      else
      {
         IPTVosPrint0(IPT_ERR, "getUriListenerDetailed: IPTVosGetSem ERROR\n");
      }
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}

/*******************************************************************************
NAME:       getMdJoinedMcAddr
ABSTRACT:   Get joined MD multicast addresses.
RETURNS:    0 if OK, != if not
*/
static int getMdJoinedMcAddr(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int found = 0;
   int i;
   int mul;
   int ret;
   UINT32 val;
   UINT32 multiCastAddr = 0;
   NET_JOINED_MC *pTabItem;

   val = pStatData->oid[ind + 1];
   /* Key in request? */
   if (val != IPT_STAT_OID_STOPPER)
   {
      mul = 0x1000000;
      for (i = 0; i < 4; i++)
      {
         val = pStatData->oid[ind + 1 + i];
         if (val != IPT_STAT_OID_STOPPER)
         {
            multiCastAddr += val * mul;
            mul = mul/0x100;  
         }
         else
         {
            return((int)IPT_INVALID_PAR);
         }
      }
   }
   ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {

      pTabItem = (NET_JOINED_MC *)((void *)iptTabFindNext((IPT_TAB_HDR *)&IPTGLOBAL(net.mdJoinedMcAddrTable),
                                                          multiCastAddr)); /*lint !e826  Ignore casting warning */
      if (pTabItem)
      {
         found = 1;
         pStatData->oid[ind + 1] = (pTabItem->multiCastAddr >> 24) & 0xFF;
         pStatData->oid[ind + 2] = (pTabItem->multiCastAddr >> 16) & 0xFF;
         pStatData->oid[ind + 3] = (pTabItem->multiCastAddr >> 8)  & 0xFF;
         pStatData->oid[ind + 4] =  pTabItem->multiCastAddr        & 0xFF;
         pStatData->oid[ind + 5] = IPT_STAT_OID_STOPPER;

         switch (oidDig)
         {
            case 1:
               pStatData->value.counter = pTabItem->multiCastAddr;
               break;

            default:
               found = 0;
               break;
         }
      }

      if(IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "getMdJoinedMcAddr: IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "getMdJoinedMcAddr: IPTVosGetSem ERROR\n");
   }
   
   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }

}

/*******************************************************************************
NAME:       getMdRedundant
ABSTRACT:   Get MD redundant group data
RETURNS:    0 if OK, != if not
*/
static int getMdRedundant(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int ret;
   UINT32 pRedFuncRef;
   int found = 0;
   FRG_ITEM *pFrgItem;

   pRedFuncRef = pStatData->oid[ind + 1];

   /* No Key in request? */
   if (pRedFuncRef == IPT_STAT_OID_STOPPER)
   {
      pRedFuncRef = 0;
   }

   ret = IPTVosGetSem(&IPTGLOBAL(md.listenerSemId), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      pFrgItem = (FRG_ITEM *)(void*)iptTabFindNext(&IPTGLOBAL(md.frgTableHdr),
                                                   pRedFuncRef); /*lint !e826  Ignore casting warning */
      if (pFrgItem)
      {
         found = 1;
         pStatData->oid[ind + 1] = (UINT32)pFrgItem->pRedFuncRef;
         pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
      
         switch (oidDig)
         {
            case 1:
               sprintf(pStatData->value.octetString, "%#x", (unsigned int)pFrgItem->pRedFuncRef);
               break;

            case 2:
               if (pFrgItem->frgState == FRG_FOLLOWER)
               {
                  sprintf(pStatData->value.octetString, "Follower");
               }
               else
               {
                  sprintf(pStatData->value.octetString, "Leader");
               }
               break;

            default:
               found = 0;
               break;
         }
      }

      if (IPTVosPutSemR(&IPTGLOBAL(md.listenerSemId)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSem ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}

/*******************************************************************************
NAME:       getMemBlock
ABSTRACT:   Get memory block statistic.
RETURNS:    0 if OK, != if not
*/
static int getMemBlock(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   UINT32 blockSize;
   UINT32 i;
   int j;
   int found = 0;
   MEM_BLOCK *pBlock;


   blockSize = pStatData->oid[ind + 1];
   /* Key in request? */
   if (IPTVosGetSem(&IPTGLOBAL(mem.sem), IPT_WAIT_FOREVER) == (int)IPT_OK)
   {
      if (blockSize != IPT_STAT_OID_STOPPER)
      {
         for (i=0; i<IPTGLOBAL(mem.noOfBlocks); i++)
         {
            if (IPTGLOBAL(mem.freeBlock)[i].size == blockSize)
            {
               i++;
               if (i<IPTGLOBAL(mem.noOfBlocks))
               {
                  found = 1;
                  pStatData->oid[ind + 1] = IPTGLOBAL(mem.freeBlock)[i].size;
                  pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
                  switch (oidDig)
                  {
                     case 1:
                        pStatData->value.counter = IPTGLOBAL(mem.freeBlock)[i].size;
                        break;

                     case 2:
                        pStatData->value.counter = IPTGLOBAL(mem.memCnt.blockCnt)[i];
                        break;

                     case 3:
                        pBlock = IPTGLOBAL(mem.freeBlock)[i].pFirst;
                        j = 0;
                        /* Count free block free */
                        while (pBlock != NULL)
                        {    
                           j++;
                           pBlock = pBlock->pNext;
                        }
                        /* Used blocks */
                        pStatData->value.counter = IPTGLOBAL(mem.memCnt.blockCnt)[i] - j;
                        break;

                     default:
                        found = 0;
                        break;
                  }
               }
               /* Break for loop */
               break;

            }
         }
      }
      else
      {
         found = 1;
         pStatData->oid[ind + 1] = IPTGLOBAL(mem.freeBlock)[0].size;
         pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
         switch (oidDig)
         {
            case 1:
               pStatData->value.counter = IPTGLOBAL(mem.freeBlock)[0].size;
               break;

            case 2:
               pStatData->value.counter = IPTGLOBAL(mem.memCnt.blockCnt)[0];
               break;

            case 3:
               pBlock = IPTGLOBAL(mem.freeBlock)[0].pFirst;
               j = 0;
               /* Count free block free */
               while (pBlock != NULL)
               {    
                  j++;
                  pBlock = pBlock->pNext;
               }
               /* Used blocks */
               pStatData->value.counter = IPTGLOBAL(mem.memCnt.blockCnt)[0] - j;
               break;

            default:
               found = 0;
               break;
         }
      }
      if (IPTVosPutSemR(&IPTGLOBAL(mem.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSem ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }

   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }

}

/*******************************************************************************
NAME:       getqueueErr
ABSTRACT:   Get no of queue errors.
RETURNS:    0 if OK, != if not
*/
static int getqueueErr(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   pStatData->value.counter = IPTGLOBAL(vos.queueCnt.queuCreateErrCnt) +
                              IPTGLOBAL(vos.queueCnt.queuDestroyErrCnt) +
                              IPTGLOBAL(vos.queueCnt.queuWriteErrCnt) +
                              IPTGLOBAL(vos.queueCnt.queuReadErrCnt);
   return((int)IPT_OK);  
}


/*******************************************************************************
NAME:       getMdDefaultMaxSeqNo
ABSTRACT:   Get MD Default max no of SeqNo
RETURNS:    0 if OK, != if not
*/
static int getMdDefaultMaxSeqNo(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   
   pStatData->value.counter = IPTGLOBAL(md.maxStoredSeqNo);
   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:       getPdDefaultValBeh
ABSTRACT:   Get MD Default max no of SeqNo
RETURNS:    0 if OK, != if not
*/
static int getPdDefaultValBeh(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   
   if (IPTGLOBAL(pd.defInvalidBehaviour))
   {
      sprintf(pStatData->value.octetString,
              "Keep");
   }
   else
   {
      sprintf(pStatData->value.octetString,
              "Zero");
   }
   
   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:       getExchPar
ABSTRACT:   Get exchange parameter configuration data.
RETURNS:    0 if OK, != if not
*/
static int getExchPar(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   UINT32 comId;
   UINT32 i;
   int found = 0;
   IPT_CONFIG_EXCHG_PAR_EXT *p;
   IPT_TAB_HDR *pTableHead;

     
   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      pTableHead = &IPTGLOBAL(configDB.exchgParTable);
      if (pTableHead->initialized)
      {
         p = (IPT_CONFIG_EXCHG_PAR_EXT *)pTableHead->pTable;   /*lint !e826 Type cast OK */ 
         comId = pStatData->oid[ind + 1];
         /* Key in request? */
         if (comId != IPT_STAT_OID_STOPPER)
         {
            for (i=0; i<pTableHead->nItems; i++)
            {
               if (p[i].comId == comId)
               {
                  i++;
                  if (i<pTableHead->nItems)
                  {
                     found = 1;
                     pStatData->oid[ind + 1] = p[i].comId;
                     pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
                     switch (oidDig)
                     {
                        case 1:
                           pStatData->value.counter = p[i].comId;
                           break;

                        case 2:
                           if (p[i].datasetId)
                           {
                              sprintf(pStatData->value.octetString,
                                      "%d",p[i].datasetId);
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;

                        case 3:
                           if (p[i].comParId)
                           {
                              sprintf(pStatData->value.octetString,
                                      "%d",p[i].comParId);
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;

 
                        case 4:
                           if (p[i].pdSendPar.pDestinationURI != NULL)
                           {
                              strncpy(pStatData->value.octetString,
                                      p[i].pdSendPar.pDestinationURI,IPT_STAT_STRING_LEN);
                              pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;

                        case 5:
                           if (p[i].pdSendPar.cycleTime)
                           {
                              sprintf(pStatData->value.octetString,
                                      "%d",p[i].pdSendPar.cycleTime);
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;
 
                        case 6:
                           if (p[i].pdSendPar.redundant)
                           {
                              sprintf(pStatData->value.octetString,
                                      "%d",p[i].pdSendPar.redundant);
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;
 
                        case 7:
                           if (p[i].pdRecPar.pSourceURI != NULL)
                           {
                              strncpy(pStatData->value.octetString,
                                      p[i].pdRecPar.pSourceURI,IPT_STAT_STRING_LEN);
                              pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;

                        case 8:
                           if (p[i].pdRecPar.timeoutValue)
                           {
                              sprintf(pStatData->value.octetString,
                                      "%d",p[i].pdRecPar.timeoutValue);
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;
 
                        case 9:
                           if (p[i].pdRecPar.timeoutValue)
                           {
                              if (p[i].pdRecPar.validityBehaviour)
                              {
                                 sprintf(pStatData->value.octetString,
                                         "Keep");
                              }
                              else
                              {
                                 sprintf(pStatData->value.octetString,
                                         "Zero");
                              }
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;

                        case 10:
                           if (p[i].mdSendPar.pDestinationURI != NULL)
                           {
                              strncpy(pStatData->value.octetString,
                                      p[i].mdSendPar.pDestinationURI,IPT_STAT_STRING_LEN);
                              pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;

                        case 11:
                           if (p[i].mdSendPar.pSourceURI != NULL)
                           {
                              strncpy(pStatData->value.octetString,
                                      p[i].mdSendPar.pSourceURI,IPT_STAT_STRING_LEN);
                              pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;

                        case 12:
                           if (p[i].mdSendPar.ackTimeOut)
                           {
                              sprintf(pStatData->value.octetString,
                                      "%d",p[i].mdSendPar.ackTimeOut);
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;
 
                        case 13:
                           if (p[i].mdSendPar.responseTimeOut)
                           {
                              sprintf(pStatData->value.octetString,
                                      "%d",p[i].mdSendPar.responseTimeOut);
                           }
                           else
                           {
                              sprintf(pStatData->value.octetString,
                                      " ");
                           }
                           break;
 
                        default:
                           found = 0;
                           break;
                     }
                  }
                  /* Break for loop */
                  break;

               }
            }
         }
         else
         {
            if (pTableHead->nItems > 0)
            {
               found = 1;
               pStatData->oid[ind + 1] = p[0].comId;
               pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = p[0].comId;
                     break;

                  case 2:
                     if (p[0].datasetId)
                     {
                        sprintf(pStatData->value.octetString,
                                "%d",p[0].datasetId);
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 3:
                     if (p[0].comParId)
                     {
                        sprintf(pStatData->value.octetString,
                                "%d",p[0].comParId);
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 4:
                     if (p[0].pdSendPar.pDestinationURI != NULL)
                     {
                        strncpy(pStatData->value.octetString,
                                p[0].pdSendPar.pDestinationURI,IPT_STAT_STRING_LEN);
                        pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 5:
                     if (p[0].pdSendPar.cycleTime)
                     {
                        sprintf(pStatData->value.octetString,
                                "%d",p[0].pdSendPar.cycleTime);
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 6:
                     if (p[0].pdSendPar.redundant)
                     {
                        sprintf(pStatData->value.octetString,
                                "%d",p[0].pdSendPar.redundant);
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 7:
                     if (p[0].pdRecPar.pSourceURI != NULL)
                     {
                        strncpy(pStatData->value.octetString,
                                p[0].pdRecPar.pSourceURI,IPT_STAT_STRING_LEN);
                        pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 8:
                     if (p[0].pdRecPar.timeoutValue)
                     {
                        sprintf(pStatData->value.octetString,
                                "%d",p[0].pdRecPar.timeoutValue);
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 9:
                     if (p[0].pdRecPar.timeoutValue)
                     {
                        if (p[0].pdRecPar.validityBehaviour)
                        {
                           sprintf(pStatData->value.octetString,
                                   "Keep");
                        }
                        else
                        {
                           sprintf(pStatData->value.octetString,
                                   "Zero");
                        }
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 10:
                     if (p[0].mdSendPar.pDestinationURI != NULL)
                     {
                        strncpy(pStatData->value.octetString,
                                p[0].mdSendPar.pDestinationURI,IPT_STAT_STRING_LEN);
                        pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 11:
                     if (p[0].mdSendPar.pSourceURI != NULL)
                     {
                        strncpy(pStatData->value.octetString,
                                p[0].mdSendPar.pSourceURI,IPT_STAT_STRING_LEN);
                        pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 12:
                     if (p[0].mdSendPar.ackTimeOut)
                     {
                        sprintf(pStatData->value.octetString,
                                "%d",p[0].mdSendPar.ackTimeOut);
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  case 13:
                     if (p[0].mdSendPar.responseTimeOut)
                     {
                        sprintf(pStatData->value.octetString,
                                "%d",p[0].mdSendPar.responseTimeOut);
                     }
                     else
                     {
                        sprintf(pStatData->value.octetString,
                                " ");
                     }
                     break;

                  default:
                     found = 0;
                     break;
               }
            }
         }
      }
      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }


   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }

}

/*******************************************************************************
NAME:       getSourceFilter
ABSTRACT:   Get exchange parameter source filter configuration data.
RETURNS:    0 if OK, != if not
*/
static int getSourceFilter(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   UINT32 comId;
   UINT32 filterId;
   UINT32 i,j;
   int found = 0;
   IPT_CONFIG_COMID_SRC_FILTER_PAR *p1;
   IPT_CONFIG_SRC_FILTER_PAR *p2;
   IPT_TAB_HDR *pTableHead1;
   IPT_TAB_HDR *pTableHead2;

     
   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      pTableHead1 = &IPTGLOBAL(configDB.pdSrcFilterParTable);
      if (pTableHead1->initialized)
      {
         p1 = (IPT_CONFIG_COMID_SRC_FILTER_PAR *)pTableHead1->pTable;   /*lint !e826 Type cast OK */ 
         comId = pStatData->oid[ind + 1];
         /* Key1 in request? */
         if (comId != IPT_STAT_OID_STOPPER)
         {
            /* Key 2 in request? */
            filterId = pStatData->oid[ind + 2];
            if (filterId == IPT_STAT_OID_STOPPER)
            {
               return((int)IPT_INVALID_PAR);
            }
            
            for (i=0; (i<pTableHead1->nItems) && (found == 0); i++)
            {
               if (p1[i].comId == comId)
               {
                  pTableHead2 = p1[i].pFiltTab;
                  if (pTableHead2->initialized)
                  {
                     p2 = (IPT_CONFIG_SRC_FILTER_PAR *)pTableHead2->pTable;   /*lint !e826 Type cast OK */ 
                     for (j=0; (j<pTableHead2->nItems) && (found == 0); j++)
                     {
                        if (p2[j].filterId == filterId)
                        {
                           j++;
                           if (j<pTableHead2->nItems)
                           {
                              found = 1;
                              pStatData->oid[ind + 1] = p1[i].comId;
                              pStatData->oid[ind + 2] = p2[j].filterId;
                              pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
                              switch (oidDig)
                              {
                                 case 1:
                                    pStatData->value.counter = p1[i].comId;
                                    break;

                                 case 2:
                                    pStatData->value.counter = p2[j].filterId;
                                    break;

                                 case 3:
                                    strncpy(pStatData->value.octetString,
                                            p2[j].pSourceURI,IPT_STAT_STRING_LEN);
                                    pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                                    break;

          
                                 default:
                                    found = 0;
                                    break;
                              }
                           }
                           else
                           {
                              i++;
                              if (i<pTableHead1->nItems)
                              {
                                 pTableHead2 = p1[i].pFiltTab;
                                 if (pTableHead2->initialized)
                                 {
                                    p2 = (IPT_CONFIG_SRC_FILTER_PAR *)pTableHead2->pTable; /*lint !e826 Type cast OK */
                                    if (pTableHead2->nItems > 0)
                                    {
                                       found = 1;
                                       pStatData->oid[ind + 1] = p1[i].comId;
                                       pStatData->oid[ind + 2] = p2[0].filterId;
                                       pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
                                       switch (oidDig)
                                       {
                                          case 1:
                                             pStatData->value.counter = p1[i].comId;
                                             break;

                                          case 2:
                                             pStatData->value.counter = p2[0].filterId;
                                             break;

                                          case 3:
                                             strncpy(pStatData->value.octetString,
                                                     p2[0].pSourceURI,IPT_STAT_STRING_LEN);
                                             pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                                             break;

                   
                                          default:
                                             found = 0;
                                             break;
                                       }
                                    }
                                 }
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
         else
         {
            if (pTableHead1->nItems > 0)
            {
               pTableHead2 = p1[0].pFiltTab;
               if (pTableHead2->initialized)
               {
                  p2 = (IPT_CONFIG_SRC_FILTER_PAR *)pTableHead2->pTable;   /*lint !e826 Type cast OK */ 
                  if (pTableHead2->nItems > 0)
                  {
                     found = 1;
                     pStatData->oid[ind + 1] = p1[0].comId;
                     pStatData->oid[ind + 2] = p2[0].filterId;
                     pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
                     switch (oidDig)
                     {
                        case 1:
                           pStatData->value.counter = p1[0].comId;
                           break;

                        case 2:
                           pStatData->value.counter = p2[0].filterId;
                           break;

                        case 3:
                           strncpy(pStatData->value.octetString,
                                   p2[0].pSourceURI,IPT_STAT_STRING_LEN);
                           pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                           break;

 
                        default:
                           found = 0;
                           break;
                     }
                  }
               }
            }
         }
      }
      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }


   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }

}

/*******************************************************************************
NAME:       getDestId
ABSTRACT:   Get exchange parameter destination ID configuration data.
RETURNS:    0 if OK, != if not
*/
static int getDestId(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   UINT32 comId;
   UINT32 destId;
   UINT32 i,j;
   int found = 0;
   IPT_CONFIG_COMID_DEST_ID_PAR *p1;
   IPT_CONFIG_DEST_ID_PAR *p2;
   IPT_TAB_HDR *pTableHead1;
   IPT_TAB_HDR *pTableHead2;

     
   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      pTableHead1 = &IPTGLOBAL(configDB.destIdParTable);
      if (pTableHead1->initialized)
      {
         p1 = (IPT_CONFIG_COMID_DEST_ID_PAR *)pTableHead1->pTable;   /*lint !e826 Type cast OK */ 
         comId = pStatData->oid[ind + 1];
         /* Key1 in request? */
         if (comId != IPT_STAT_OID_STOPPER)
         {
            /* Key 2 in request? */
            destId = pStatData->oid[ind + 2];
            if (destId == IPT_STAT_OID_STOPPER)
            {
               return((int)IPT_INVALID_PAR);
            }
            
            for (i=0; (i<pTableHead1->nItems) && (found == 0); i++)
            {
               if (p1[i].comId == comId)
               {
                  pTableHead2 = p1[i].pDestIdTab;
                  if (pTableHead2->initialized)
                  {
                     p2 = (IPT_CONFIG_DEST_ID_PAR *)pTableHead2->pTable;   /*lint !e826 Type cast OK */ 
                     for (j=0; (j<pTableHead2->nItems) && (found == 0); j++)
                     {
                        if (p2[j].destId == destId)
                        {
                           j++;
                           if (j<pTableHead2->nItems)
                           {
                              found = 1;
                              pStatData->oid[ind + 1] = p1[i].comId;
                              pStatData->oid[ind + 2] = p2[j].destId;
                              pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
                              switch (oidDig)
                              {
                                 case 1:
                                    pStatData->value.counter = p1[i].comId;
                                    break;

                                 case 2:
                                    pStatData->value.counter = p2[j].destId;
                                    break;

                                 case 3:
                                    strncpy(pStatData->value.octetString,
                                            p2[j].pDestURI,IPT_STAT_STRING_LEN);
                                    pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                                    break;

          
                                 default:
                                    found = 0;
                                    break;
                              }
                           }
                           else
                           {
                              i++;
                              if (i<pTableHead1->nItems)
                              {
                                 pTableHead2 = p1[i].pDestIdTab;
                                 if (pTableHead2->initialized)
                                 {
                                    p2 = (IPT_CONFIG_DEST_ID_PAR *)pTableHead2->pTable; /*lint !e826 Type cast OK */
                                    if (pTableHead2->nItems > 0)
                                    {
                                       found = 1;
                                       pStatData->oid[ind + 1] = p1[i].comId;
                                       pStatData->oid[ind + 2] = p2[0].destId;
                                       pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
                                       switch (oidDig)
                                       {
                                          case 1:
                                             pStatData->value.counter = p1[i].comId;
                                             break;

                                          case 2:
                                             pStatData->value.counter = p2[0].destId;
                                             break;

                                          case 3:
                                             strncpy(pStatData->value.octetString,
                                                     p2[0].pDestURI,IPT_STAT_STRING_LEN);
                                             pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                                             break;

                   
                                          default:
                                             found = 0;
                                             break;
                                       }
                                    }
                                 }
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
         else
         {
            if (pTableHead1->nItems > 0)
            {
               pTableHead2 = p1[0].pDestIdTab;
               if (pTableHead2->initialized)
               {
                  p2 = (IPT_CONFIG_DEST_ID_PAR *)pTableHead2->pTable;   /*lint !e826 Type cast OK */ 
                  if (pTableHead2->nItems > 0)
                  {
                     found = 1;
                     pStatData->oid[ind + 1] = p1[0].comId;
                     pStatData->oid[ind + 2] = p2[0].destId;
                     pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
                     switch (oidDig)
                     {
                        case 1:
                           pStatData->value.counter = p1[0].comId;
                           break;

                        case 2:
                           pStatData->value.counter = p2[0].destId;
                           break;

                        case 3:
                           strncpy(pStatData->value.octetString,
                                   p2[0].pDestURI,IPT_STAT_STRING_LEN);
                           pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                           break;

 
                        default:
                           found = 0;
                           break;
                     }
                  }
               }
            }
         }
      }
      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }


   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }

}

/*******************************************************************************
NAME:     getDatasetData
ABSTRACT: Get information on a dataset.
RETURNS:  -
*/
static int getDatasetData(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int ret;
   UINT32 reqIndex;
   UINT32 variableIndex = 1;
   IPT_DATA_SET_FORMAT_INT *pFormat;  /* Pointer to formatting table */
   IPT_CFG_DATASET_INT *pDataset;

   ret = IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      pDataset = (IPT_CFG_DATASET_INT *) iptTabFind(&IPTGLOBAL(configDB.datasetTable), dataSetHandle); /*lint !e826 Type cast OK */ 
      if (pDataset == NULL)
      {
         ret =  IPT_NOT_FOUND;      /* There is no dataset with specified handle */
      }
      else
      {
         pFormat = pDataset->pFormat;
   
         reqIndex = pStatData->oid[ind + 1];
        

         if (reqIndex == IPT_STAT_OID_STOPPER)
         {
            /* Call without any key, return first item, if there is any */
            if (pDataset->nLines > 0)
            {
               ret = IPT_OK;
            }
         }
         else
         {
            if (reqIndex < pDataset->nLines)
            {
               variableIndex = reqIndex+1;
               ret = IPT_OK;
            }
            else
            {
               ret = IPT_NOT_FOUND;
            }
         }

         if (ret == IPT_OK)
         {
            pStatData->oid[ind + 1] = variableIndex;
            pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
      
            switch (oidDig)
            {
            case 1:
               pStatData->value.counter = variableIndex;
               break;
         
            case 2:
               if (pFormat[variableIndex-1].id < 0)
               {
                  sprintf(pStatData->value.octetString,
                  "%s",
                  datatypeInt2String(pFormat[variableIndex-1].id));
               }
               else if (pFormat[variableIndex-1].id > 0)
               {
                  sprintf(pStatData->value.octetString,
                         "DataSetId=%d",
                         pFormat[variableIndex-1].id);
               }
               else
               {
                  sprintf(pStatData->value.octetString,
                         "Illegal type");
               }
               break;
         
            case 3:
               pStatData->value.counter = pFormat[variableIndex-1].size;
               break;
         
            default:
               ret = IPT_NOT_FOUND;
               break;
            }
         }
      }
   
      if (IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "getAllDatasetData: IPTVosPutSem(listenerSemId) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "getAllDatasetData: IPTVosGetSem ERROR\n");
   }

   return ret;
}

/*******************************************************************************
NAME:     getAllDatasetData
ABSTRACT: Get information on all dataset.
RETURNS:  -
*/
static int getAllDatasetData(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   int ret;
   int found = 0;
   UINT32 dataSetId;
   UINT32 reqIndex;
   UINT32 variableIndex = 0;
   IPT_DATA_SET_FORMAT_INT *pFormat;  /* Pointer to formatting table */
   IPT_CFG_DATASET_INT *pDataset;

   ret = IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER);
   if (ret == IPT_OK)
   {
      dataSetId = pStatData->oid[ind + 1];
      
      /* Key in request? */
      if (dataSetId != IPT_STAT_OID_STOPPER)
      {
         reqIndex = pStatData->oid[ind + 2];

         /* Key 2 not in request? */
         if (reqIndex == IPT_STAT_OID_STOPPER)
         {
            return((int)IPT_INVALID_PAR);
         }
         else
         {
            pDataset = (IPT_CFG_DATASET_INT *) iptTabFind(&IPTGLOBAL(configDB.datasetTable), dataSetId); /*lint !e826 Type cast OK */ 
         }
      }
      /* Request without any key */
      else
      {
         dataSetId = 0;
         reqIndex = 0;
         pDataset = (IPT_CFG_DATASET_INT *) iptTabFindNext(&IPTGLOBAL(configDB.datasetTable), dataSetId); /*lint !e826 Type cast OK */ 
      }
   
      while ((pDataset) && (!found))
      {
         if (reqIndex < pDataset->nLines)
         {
            variableIndex = reqIndex+1;
            found = 1;
         }
         else
         {
            pDataset = (IPT_CFG_DATASET_INT *) iptTabFindNext(&IPTGLOBAL(configDB.datasetTable), pDataset->datasetId); /*lint !e826 Type cast OK */ 
            reqIndex = 0;
         }
      }

      if (pDataset)
      {
         pStatData->oid[ind + 1] = pDataset->datasetId;
         pStatData->oid[ind + 2] = variableIndex;
         pStatData->oid[ind + 3] = IPT_STAT_OID_STOPPER;
      
         switch (oidDig)
         {
            case 1:
               pStatData->value.counter = pDataset->datasetId;
               break;

            case 2:
               pStatData->value.counter = variableIndex;
               break;

            case 3:
               pFormat = pDataset->pFormat;
               if (pFormat)
               {
                  if (pFormat[variableIndex-1].id < 0)
                  {
                     sprintf(pStatData->value.octetString,
                  			 "%s",
                             datatypeInt2String(pFormat[variableIndex-1].id));
                  }
                  else if (pFormat[variableIndex-1].id > 0)
                  {
                     sprintf(pStatData->value.octetString,
                            "DataSetId=%d",
                            pFormat[variableIndex-1].id);
                  }
                  else
                  {
                     sprintf(pStatData->value.octetString, "Illegal type");
                  }
               }
               else
               {
                  /* This shall not be possible */
                  sprintf(pStatData->value.octetString, "No Format for the dataSet");
               }
               break;
            
            case 4:
               pFormat = pDataset->pFormat;
               if (pFormat)
               {
                  if (pFormat[variableIndex-1].size > 0)
                  {
                     sprintf(pStatData->value.octetString, "%d", pFormat[variableIndex-1].size);
                  }
                  else
                  {
                     sprintf(pStatData->value.octetString, "Dynamic size");
                  }
               }
               else
               {
                  /* This shall not be possible */
                  sprintf(pStatData->value.octetString, "No Format for the dataSet");
               }
               break;
            
            default:
               found = 0;
               break;
         }
      }
      
      if (IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "getAllDatasetData: IPTVosPutSem(listenerSemId) ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "getAllDatasetData: IPTVosGetSem ERROR\n");
   }
   
   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }
}

/*******************************************************************************
NAME:       getComPar
ABSTRACT:   Get communication parameter configuration data.
RETURNS:    0 if OK, != if not
*/
static int getComPar(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   UINT32 ind,               /* IN index of OID array */
   UINT32 oidDig)            /* IN OID digit */
{
   UINT32 comParId;
   UINT32 i;
   int found = 0;
   IPT_CONFIG_COM_PAR_EXT *p;
   IPT_TAB_HDR *pTableHead;

     
   if (IPTVosGetSem(&IPTGLOBAL(configDB.sem), IPT_WAIT_FOREVER) == IPT_OK)
   {
      pTableHead = &IPTGLOBAL(configDB.comParTable);
      if (pTableHead->initialized)
      {
         p = (IPT_CONFIG_COM_PAR_EXT *)pTableHead->pTable;   /*lint !e826 Type cast OK */ 
         comParId = pStatData->oid[ind + 1];
         /* Key in request? */
         if (comParId != IPT_STAT_OID_STOPPER)
         {
            for (i=0; i<pTableHead->nItems; i++)
            {
               if (p[i].comParId == comParId)
               {
                  i++;
                  if (i<pTableHead->nItems)
                  {
                     found = 1;
                     pStatData->oid[ind + 1] = p[i].comParId;
                     pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
                     switch (oidDig)
                     {
                        case 1:
                           pStatData->value.counter = p[i].comParId;
                           break;

                        case 2:
                           pStatData->value.counter = p[i].qos;
                           break;

                        case 3:
                           pStatData->value.counter = p[i].ttl;
                           break;

                        default:
                           found = 0;
                           break;
                     }
                  }
                  /* Break for loop */
                  break;

               }
            }
         }
         else
         {
            if (pTableHead->nItems > 0)
            {
               found = 1;
               pStatData->oid[ind + 1] = p[0].comParId;
               pStatData->oid[ind + 2] = IPT_STAT_OID_STOPPER;
               switch (oidDig)
               {
                  case 1:
                     pStatData->value.counter = p[0].comParId;
                     break;

                  case 2:
                     pStatData->value.counter = p[0].qos;
                     break;

                  case 3:
                     pStatData->value.counter = p[0].ttl;
                     break;

                  default:
                     found = 0;
                     break;
               }
            }
         }
      }
      if(IPTVosPutSemR(&IPTGLOBAL(configDB.sem)) != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "IPTVosPutSemR ERROR\n");
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "IPTVosGetSem ERROR\n");
   }


   if (found)
   {
      return((int)IPT_OK);         
   }
   else
   {
      return((int)IPT_NOT_FOUND);
   }

}

/*******************************************************************************
NAME:     getSetDebugLevel
ABSTRACT: Get/Set debug level 
RETURNS:  -
*/
static int getSetDebugLevel(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   int    set)
{
   unsigned int i;
   UINT16 mask;

   
   if (set)
   {
      mask = 0;
      for(i=0; i < strlen(pStatData->value.octetString); ++i)
      {
         switch(pStatData->value.octetString[i])
         {
             case 'E':
             case 'e':
                 mask |= IPT_ERR;
                 break;
             case 'w':
             case 'W':
                 mask |= IPT_WARN;
                 break;

             default:
                 break;
         }
      }
      IPTVosSetPrintMask(mask);
   }
   else
   {
      mask = IPTVosGetPrintMask();
      pStatData->value.octetString[0] = 0;
      if (mask & IPT_ERR)
      {
         strcat(pStatData->value.octetString,"E");
      }
      if (mask & IPT_WARN)
      {
         strcat(pStatData->value.octetString,"W");
      }
      if (mask == 0)
      {
         strcat(pStatData->value.octetString,"Off");
      }
   }

   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:     getSetDebugInfo
ABSTRACT: Get/Set debug info.
RETURNS:  -
*/
static int getSetDebugInfo(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   int    set)
{
   unsigned int i;
   UINT16 mask;

   if (set)
   {
      mask = 0;
      for(i=0; i < strlen(pStatData->value.octetString); ++i)
      {
         switch(pStatData->value.octetString[i])
         {
             case 'A':
             case 'a':
                 mask |= INF_ALL;
                 break;
             case 'D':
             case 'd':
             case 'T':
             case 't':
                 mask |= INF_DATETIME;
                 break;
             case 'C':
             case 'c':
                 mask |= INF_CATEGORY;
                 break;
             case 'F':
             case 'f':
                 mask |= INF_FILE;
                 break;
             case 'L':
             case 'l':
                 mask |= INF_LINE;
                 break;
             default:
                 break;
         }
      }
      IPTVosSetInfoMask(mask);
   }
   else
   {
      mask = IPTVosGetInfoMask();
      pStatData->value.octetString[0] = 0;
      if (mask & INF_DATETIME)
      {
         strcat(pStatData->value.octetString,"D");
      }
      if (mask & INF_CATEGORY)
      {
         strcat(pStatData->value.octetString,"C");
      }
      if (mask & INF_FILE)
      {
         strcat(pStatData->value.octetString,"F");
      }
      if (mask & INF_LINE)
      {
         strcat(pStatData->value.octetString,"L");
      }
      if (mask == 0)
      {
         strcat(pStatData->value.octetString,"Only text");
      }
   }

   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:     getSetDebugFileName
ABSTRACT: Get/Set debug file name
RETURNS:  -
*/
static int getSetDebugFileName(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   int    set)
{
   const char* pFileName;

   if (set)
   {
      IPTVosDFile(pStatData->value.octetString);
   }
   else
   {
      pFileName = IPTVosGetDebugFileName();
      if (pFileName)
      {
         sprintf(pStatData->value.octetString, "%s", pFileName);
      }
      else
      {
         sprintf(pStatData->value.octetString, "No log file in use");
      }
   }

   return((int)IPT_OK);  
}

/*******************************************************************************
NAME:     getSetDebugFileSize
ABSTRACT: Get/Set debug file size
RETURNS:  -
*/
static int getSetDebugFileSize(
   IPT_STAT_DATA *pStatData, /* IN/OUT Pointer to statistic data structure */
   int    set)
{
   if (set)
   {
      IPTVosSetLogFileSize(pStatData->value.counter);
   }
   else
   {
      pStatData->value.counter = IPTVosGetLogFileSize();
   }

   return((int)IPT_OK);
}

/*******************************************************************************
NAME:       clearStatistic
ABSTRACT:   Clear statistics.
RETURNS:    0 if OK, != if not
*/
static int clearStatistic(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   int ret;
   int res = IPT_OK;

   IPT_UNUSED (pStatData);

   ret = PDCom_clearStatistic();
   if (ret != IPT_OK )
   {
      res = ret;
   }
   ret = MDCom_clearStatistic();
   if (ret != IPT_OK )
   {
      res = ret;
   }
  

/* shall not be cleared 
   IPTGLOBAL(mem.memSize) = 0;
   IPTGLOBAL(mem.memCnt.freeSize) = 0;
*/
   IPTGLOBAL(mem.memCnt.minFreeSize) = IPTGLOBAL(mem.memCnt.freeSize);
/* shall not be cleared 
   IPTGLOBAL(mem.memCnt.allocCnt) = 0;
*/
   IPTGLOBAL(mem.memCnt.allocErrCnt) = 0;
   IPTGLOBAL(mem.memCnt.freeErrCnt) = 0;

/* shall not be cleared 
   IPTGLOBAL(vos.queueCnt.queueAllocated) = 0;
*/
   IPTGLOBAL(vos.queueCnt.queueMax) = IPTGLOBAL(vos.queueCnt.queueAllocated);
   
   IPTGLOBAL(vos.queueCnt.queuCreateErrCnt)  = 0;
   IPTGLOBAL(vos.queueCnt.queuDestroyErrCnt) = 0;
   IPTGLOBAL(vos.queueCnt.queuWriteErrCnt) = 0;
   IPTGLOBAL(vos.queueCnt.queuReadErrCnt) = 0;

   return(res);  
}

/*******************************************************************************
NAME:       pdClearStatistic
ABSTRACT:   Clear PD statistics.
RETURNS:    0 if OK, != if not
*/
static int pdClearStatistic(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   int ret;
   IPT_UNUSED(pStatData);
   
   ret = PDCom_clearStatistic();
   return(ret);  
}

/*******************************************************************************
NAME:       mdClearStatistic
ABSTRACT:   Clear MD statistics.
RETURNS:    0 if OK, != if not
*/
static int mdClearStatistic(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   int ret;
   IPT_UNUSED (pStatData);
   
   ret = MDCom_clearStatistic();
   return(ret);  
}

/*******************************************************************************
NAME:       iptStat
ABSTRACT:   Get, get next or set statistic variable.
RETURNS:    0 if OK, != if not
*/
static int iptStat(
   const char *pRequest,  /* Pointer to request buffer */
   char *pResponse,       /* Pointer to response buffer */
   UINT32 length,         /* Length of response buffer */
   int action)
{
   const char *pReq;
   const char *pNextMultReq;   
   char request[MAX_IPTCOM_OID+1] = {0} ;
   char digitstring[16];
   char *pToken = NULL;
   char *pVal = NULL;
   UINT32 oidStart[] = IPT_START_OID;
   int startLen;
   int i = 0;
   int ret = (int)IPT_OK;
   UINT32 len;
   UINT32 leftLen = length;
   IPT_STAT_DATA reqStruct = {{0}, 0, {0}};

   if (pResponse == NULL)
   {
      return((int)IPT_INVALID_PAR);
   }
   
   pResponse[0] = 0;

   /* Check if it is a multiple request */
   pNextMultReq = strchr(pRequest,' ');
   if (pNextMultReq != NULL)
   {
      pNextMultReq++; 
   }

   do
   {
      /* Complete OID ? */
      pReq = strchr(pRequest,'.');
      if (pReq == pRequest)
      {
         len = strlen(".");
         if (len < leftLen)
         {
            strcat(pResponse, ".");
            leftLen -= len;
            pReq += len;

            startLen = sizeof(oidStart) / sizeof(UINT32);
            for (i = 0; (i < startLen) && (ret == (int)IPT_OK); i++)
            {
               sprintf(digitstring, "%u.", oidStart[i]);
               len = strlen(digitstring);
               if (len < leftLen)
               {
                  /* Check that OID is a Bombardier OID */
                  if (strncmp(pReq, digitstring, len) == 0)
                  {
                     strcat(pResponse, digitstring);
                     leftLen -= len;
                     pReq += len;
                  }
                  else
                  {
                     ret = (int)IPT_INVALID_PAR;
                  }
               }
               else
               {
                  ret = (int)IPT_ILLEGAL_SIZE;
               }
            }
         }
         else
         {
            ret = (int)IPT_ILLEGAL_SIZE;
         }
      }
      else
      {
         pReq = pRequest;
      }
     
      if (ret == (int)IPT_OK)
      {
         /* Not multiple request or last request ? */
         if (pNextMultReq == NULL)
         {
            if (strlen(pReq) < MAX_IPTCOM_OID)
            {
               strncpy(request,pReq,MAX_IPTCOM_OID);
               request[MAX_IPTCOM_OID] = 0;
            }
            else
            {
               ret = (int)IPT_ILLEGAL_SIZE;
            }
         }
         else
         {
            if (pNextMultReq - pReq - 1< MAX_IPTCOM_OID)
            {
               strncpy(request,pReq,pNextMultReq - pReq -1);
               request[pNextMultReq - pReq -1] = 0;
            }
            else
            {
               ret = (int)IPT_ILLEGAL_SIZE;
            }
         }

         if (ret == (int)IPT_OK)
         {
            pVal = strchr(request,'=');
            if (pVal != NULL)
            {
               *pVal = 0;
               pVal++;
               if (isdigit((int)(*pVal)))
               {
                  reqStruct.value.integer = atoi(pVal);
               }
               else
               {
                  ret = (int)IPT_INVALID_PAR;
               }
            }
         }

         if (ret == (int)IPT_OK)
         {
            pToken = strtok(request,".");
            i = 0;
            while ((pToken != NULL) && (i < IPT_STAT_OID_LEN - 1))
            {
               if (isdigit((int)(*pToken)))
               {
                  reqStruct.oid[i] = atoi(pToken);
                  pToken = strtok((char *)NULL,".");
                  i++;
               }
               else
               {
                  ret = (int)IPT_INVALID_PAR;
                  break;
               }
            }
         }

         if (ret == (int)IPT_OK)
         {
            /* All OID's read? */
            if (pToken == NULL)
            {
               reqStruct.oid[i] = IPT_STAT_OID_STOPPER;

               switch (action)
               {
                  case IPTCOM_STAT_GET:
                     if (pVal == NULL)
                     {
                        ret = iptStatGet(&reqStruct);
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  case IPTCOM_STAT_GETNEXT:
                     if (pVal == NULL)
                     {
                        ret = iptStatGetNext(&reqStruct);
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  case IPTCOM_STAT_SET:
                     if (pVal)
                     {
                        ret = iptStatSet(&reqStruct);
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  default:
                     ret = (int)IPT_ERROR;
                     break;
               }
            }
            else
            {
               ret = (int)IPT_ILLEGAL_SIZE;
            }
         }
      }

      if (ret == (int)IPT_OK)
      {
         i = 0;
         while (ret == (int)IPT_OK)
         {
            if (i < IPT_STAT_OID_LEN - 1)
            {
               if (reqStruct.oid[i+1] != IPT_STAT_OID_STOPPER)
               {
                  sprintf(digitstring, "%u.", reqStruct.oid[i]);
                  len = strlen(digitstring);
                  if (len < leftLen)
                  {
                     strcat(pResponse, digitstring);
                     leftLen -= len;
                  }
                  else
                  {
                     ret = (int)IPT_ILLEGAL_SIZE;
                  }
                  i++; 
               }
               else
               {
                  sprintf(digitstring, "%u=", reqStruct.oid[i]);
                  len = strlen(digitstring);
                  if (len < leftLen)
                  {
                     strcat(pResponse, digitstring);
                     leftLen -= len;
                  }
                  else
                  {
                     ret = (int)IPT_ILLEGAL_SIZE;
                  }

                  switch (reqStruct.type)
                  {
                     case IPT_STAT_TYPE_INTEGER:
                        sprintf(digitstring, "%d", reqStruct.value.integer);
                        len = strlen(digitstring);
                        if (len < leftLen)
                        {
                           strcat(pResponse, digitstring);
                           leftLen -= len;
                        }
                        else
                        {
                           ret = (int)IPT_ILLEGAL_SIZE;
                        }
                        break;

                     case IPT_STAT_TYPE_OCTET_STRING:
                        len = strlen(reqStruct.value.octetString);
                        if (len < leftLen)
                        {
                           strcat(pResponse, reqStruct.value.octetString);
                           leftLen -= len;
                        }
                        else
                        {
                           ret = (int)IPT_ILLEGAL_SIZE;
                        }
                        break;

                     case IPT_STAT_TYPE_IPADDRESS:
                        sprintf(digitstring,"%d.%d.%d.%d",
                                (reqStruct.value.ipAddress & 0xff000000) >> 24,
                                (reqStruct.value.ipAddress & 0xff0000) >> 16,
                                (reqStruct.value.ipAddress & 0xff00) >> 8,
                                reqStruct.value.ipAddress & 0xff);
                        len = strlen(digitstring);
                        if (len < leftLen)
                        {
                           strcat(pResponse, digitstring);
                           leftLen -= len;
                        }
                        else
                        {
                           ret = (int)IPT_ILLEGAL_SIZE;
                        }
                        break;

                     case IPT_STAT_TYPE_COUNTER:
                        sprintf(digitstring, "%u", reqStruct.value.counter);
                        len = strlen(digitstring);
                        if (len < leftLen)
                        {
                           strcat(pResponse, digitstring);
                           leftLen -= len;
                        }
                        else
                        {
                           ret = (int)IPT_ILLEGAL_SIZE;
                        }
                        break;

                     case IPT_STAT_TYPE_TIMETICKS:
                        sprintf(digitstring, "%u", reqStruct.value.timeTicks);
                        len = strlen(digitstring);
                        if (len < leftLen)
                        {
                           strcat(pResponse, digitstring);
                           leftLen -= len;
                        }
                        else
                        {
                           ret = (int)IPT_ILLEGAL_SIZE;
                        }
                        break;

                     default:
                        ret = (int)IPT_ERROR;
                        pResponse[0] = 0;
                        break;

                  } 

                  /* Break while (ret == (int)IPT_OK) loop */
                  break; 
               }
            }
            else
            {
               ret = (int)IPT_ERROR;
               pResponse[0] = 0;
               break;
            }
         }
         /* Multiple request ?*/
         if (pNextMultReq != NULL)
         {
            if (leftLen > 1)
            {
               strcat(pResponse, " ");
               leftLen--;
               pRequest = pNextMultReq;
               pNextMultReq = strchr(pNextMultReq,' ');
               if (pNextMultReq != NULL)
               {
                  pNextMultReq++; 
               }
            }
            else
            {
               ret = (int)IPT_ILLEGAL_SIZE;
            }
         }
      }
      else
      {
         pResponse[0] = 0; 
      }
   }
   while(pNextMultReq != NULL && ret == (int)IPT_OK);
   return(ret);
}

/*******************************************************************************
NAME:       getDataStructure
ABSTRACT:   Get statistic structure.
RETURNS:    0 if OK, != if not
*/
static int getDataStructure(
   IPT_STAT_DATA *pStatData,     /* IN/OUT Pointer to statistic data structure */
   UINT32        *pArraySize,    /* OUT Size of statistic structure */
   IPT_OID_DATA  **ppStatStruct, /* OUT Pointer to pointer to statistic structure */
   UINT32        *pNextOidInd)   /* OUT Pointer to Next index of OID array */
{
   UINT32 k;
   UINT32 oidDig;
   int ret = (int)IPT_OK;
   int done = 0;
   IPT_TAB_HDR *pOidTable;
   STAT_TABLE_STRUCT *pTabItem;

#ifdef LINUX_MULTIPROC
   if (!initiated)
   {
      ret = iptStatOwnProcessInit();
      if (ret != IPT_OK)
      {
         return(IPT_ERROR);   
      }   
   }
#endif   
   pOidTable = pOidStartTable;
   k = 0;
   while ((k < IPT_STAT_OID_LEN) &&
          (pStatData->oid[k] != IPT_STAT_OID_STOPPER) &&
          (ret == (int)IPT_OK) &&
          (done == 0))
   {
      oidDig = pStatData->oid[k];
      if (oidDig != IPT_STAT_OID_STOPPER)
      {
         pTabItem = (STAT_TABLE_STRUCT *)((void *)iptTabFind((IPT_TAB_HDR *)pOidTable,
                                                             oidDig)); /*lint !e826  Ignore casting warning */
         if (pTabItem)
         {
            if (pTabItem->tableType == IPTCOM_OID_TABLE)
            {
               pOidTable = &(pTabItem->tabOrData.nextOidTable);   
            }
            else
            {
               *ppStatStruct = pTabItem->tabOrData.oidHdr.pStatStruct;
               *pArraySize = pTabItem->tabOrData.oidHdr.size;
               *pNextOidInd = k + 1;
               done = 1;
            }
         }
         else
         {
            ret = (int)IPT_INVALID_PAR;
         }
      }
      else
      {
         ret = (int)IPT_INVALID_PAR;
      }
   
      k++;
   }

   return(ret);
}

/*******************************************************************************
NAME:       iptStatInit
ABSTRACT:   Intitiate.
RETURNS:    -
*/
static int statInit(void)
{
   int ret;
   IPT_OID_DEF oidDef[] =
   {
      {IPT_OID_BTROOT_DIG, IPT_OID_BTROOT_NAME},
      {IPT_OID_BTPRODUCT_DIG, IPT_OID_BTPRODUCT_NAME},
      {IPT_OID_ONBOARD_DIG, IPT_OID_ONBOARD_NAME},
      {IPT_OID_COM_DIG, IPT_OID_COM_NAME}
   };

   ret = iptTabInit((IPT_TAB_HDR *)pOidStartTable, sizeof(STAT_TABLE_STRUCT));
   if (ret == (int)IPT_OK)
   {
      ret = iptStatAddSubAgent(sizeof(oidDef)/sizeof(IPT_OID_DEF),
                               oidDef,
                               iptcom_stat,
                               sizeof(iptcom_stat)/sizeof(IPT_OID_DATA));
   }
   
   pd_stat[0].ptr.pCounter = &(IPTGLOBAL(pd.pdCnt.pdInPackets));
   pd_stat[1].ptr.pCounter = &(IPTGLOBAL(pd.pdCnt.pdInFCSErrors));
   pd_stat[2].ptr.pCounter = &(IPTGLOBAL(pd.pdCnt.pdInProtocolErrors));
   pd_stat[3].ptr.pCounter = &(IPTGLOBAL(pd.pdCnt.pdInTopoErrors));
   pd_stat[4].ptr.pCounter = &(IPTGLOBAL(pd.pdCnt.pdInNoSubscriber));
   pd_stat[5].ptr.pCounter = &(IPTGLOBAL(pd.pdCnt.pdOutPackets));

   md_stat[0].ptr.pCounter = &(IPTGLOBAL(md.mdCnt.mdInPackets));
   md_stat[1].ptr.pCounter = &(IPTGLOBAL(md.mdCnt.mdInFCSErrors));
   md_stat[2].ptr.pCounter = &(IPTGLOBAL(md.mdCnt.mdInProtocolErrors));
   md_stat[3].ptr.pCounter = &(IPTGLOBAL(md.mdCnt.mdInTopoErrors));
   md_stat[4].ptr.pCounter = &(IPTGLOBAL(md.mdCnt.mdInNoListeners));
   md_stat[5].ptr.pCounter = &(IPTGLOBAL(md.mdCnt.mdNotReturnedBuffers));
   md_stat[6].ptr.pCounter = &(IPTGLOBAL(md.mdCnt.mdOutPackets));
   md_stat[7].ptr.pCounter = &(IPTGLOBAL(md.mdCnt.mdOutRetransmissions));

   mem_stat[0].ptr.pCounter = &(IPTGLOBAL(mem.memSize));
   mem_stat[1].ptr.pCounter = &(IPTGLOBAL(mem.memCnt.freeSize));
   mem_stat[2].ptr.pCounter = &(IPTGLOBAL(mem.memCnt.minFreeSize));
   mem_stat[3].ptr.pCounter = &(IPTGLOBAL(mem.memCnt.allocCnt));
   mem_stat[4].ptr.pCounter = &(IPTGLOBAL(mem.memCnt.allocErrCnt));
   mem_stat[5].ptr.pCounter = &(IPTGLOBAL(mem.memCnt.freeErrCnt));

   queue_stat[0].ptr.pCounter =&(IPTGLOBAL(vos.queueCnt.queueAllocated));
   queue_stat[1].ptr.pCounter =&(IPTGLOBAL(vos.queueCnt.queueMax));

   config_stat[0].ptr.pCounter = &(IPTGLOBAL(pd.defTimeout));
   config_stat[2].ptr.pCounter = &(IPTGLOBAL(pd.defCycle));
   config_stat[3].ptr.pCounter = &(IPTGLOBAL(md.defAckTimeOut));
   config_stat[4].ptr.pCounter = &(IPTGLOBAL(md.defResponseTimeOut));

   return(ret);
}


#ifdef LINUX_MULTIPROC
/*******************************************************************************
NAME:       iptStatInit
ABSTRACT:   Intitiate.
RETURNS:    -
*/
static int iptStatOwnProcessInit(void)
{
   int ret;
   pOidStartTable = &oidTable;
   ret = statInit(); 
   if (ret == (int)IPT_OK)
   {
      initiated = 1;
   }
      
   return(ret);
}
#endif

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       iptStatInit
ABSTRACT:   Intitiate.
RETURNS:    -
*/
int iptStatInit(void)
{
   int ret;

   pOidStartTable = &IPTGLOBAL(stat.oidTable);

   ret = statInit(); 
   
   return(ret);
}

/*******************************************************************************
NAME:       iptStatAddSubAgent
ABSTRACT:   Intitiate.
RETURNS:    -
*/
#ifdef LINUX_MULTIPROC
static int iptStatAddSubAgent(
#else
int iptStatAddSubAgent(
#endif
   UINT32       len,          /* Length of OID array */
   IPT_OID_DEF  oidDef[],     /* Pointer to OID array of OID definition */
   IPT_OID_DATA *pStatStruct, /* Pointer to start of statistic structures */
   UINT32       size)         /* No of items in first statistic structure */
{
   int ret = (int)IPT_OK;
   UINT32 i;
   IPT_TAB_HDR *pOidTable = pOidStartTable;
   STAT_TABLE_STRUCT *pTabItem;
   STAT_TABLE_STRUCT newItem;

   for (i = 0; i < len; i++)
   {
      pTabItem = (STAT_TABLE_STRUCT *)((void *)iptTabFind((IPT_TAB_HDR *)pOidTable,
                                                          oidDef[i].oidDig)); /*lint !e826  Ignore casting warning */
      if (pTabItem)
      {
         if (pTabItem->tableType == IPTCOM_OID_TABLE)
         {
            pOidTable = &(pTabItem->tabOrData.nextOidTable);   
         }
         else
         {
            return((int)IPT_TAB_ERR_EXISTS);
         }
      }
      else
      {
         do
         {
            newItem.oidDig = oidDef[i].oidDig;
            strncpy(newItem.name, oidDef[i].name, MAX_OID_NAME);
            
            if (i == len -1)
            {
               newItem.tableType = IPTCOM_OID_DATA;   
               newItem.tabOrData.oidHdr.pStatStruct = pStatStruct;
               newItem.tabOrData.oidHdr.size = size;
               ret = iptTabAdd(pOidTable, (IPT_TAB_ITEM_HDR *) &newItem);
            }
            else
            {
               ret = iptTabInit((IPT_TAB_HDR *)&(newItem.tabOrData.nextOidTable), sizeof(STAT_TABLE_STRUCT));
               if (ret == (int)IPT_OK)
               {
                  newItem.tableType = IPTCOM_OID_TABLE;   
                  ret = iptTabAdd((IPT_TAB_HDR *)pOidTable, (IPT_TAB_ITEM_HDR *) &newItem);
                  pOidTable = &(((STAT_TABLE_STRUCT *)((void *)pOidTable->pTable))->tabOrData.nextOidTable);   
               }
            }
         
            i++;   
         }
         while((i < len ) && (ret == (int)IPT_OK));
         /* Break for loop */
         break;
      }
   }

   return(ret);
}

/*******************************************************************************
NAME:       iptStatGet
ABSTRACT:   Get statistic variable.
RETURNS:    0 if OK, != if not
*/
int iptStatGet(
   IPT_STAT_DATA *pStatData)  /* IN/OUT Pointer to statistic data structure */
{
   UINT32 i;
   UINT32 oidInd;
   UINT32 startOidInd;      /* Start index for structure OID */
   int ret = (int)IPT_OK;
   UINT32 oidDig;
   int done = 0;
   UINT32 arraySize;
   IPT_OID_DATA *pStatStruct;
  
   /* Get data structure */
   ret = getDataStructure(pStatData, &arraySize, &pStatStruct, &startOidInd);  
   if (ret == (int)IPT_OK)
   {
      oidInd = startOidInd;
      while ((oidInd < IPT_STAT_OID_LEN) &&
             (ret == (int)IPT_OK) &&
             (done == 0))
      {
         oidDig = pStatData->oid[oidInd];
         if (oidDig == IPT_STAT_OID_STOPPER)
         {
            /* Get next OID digit from the current structure */
            oidDig = pStatStruct[0].oidDig;
            pStatData->oid[oidInd] = oidDig;
            if (oidInd < IPT_STAT_OID_LEN - 1)
            {
               pStatData->oid[oidInd+1] = IPT_STAT_OID_STOPPER;
            }
         }
       
         i = 0;
         while (ret == (int)IPT_OK)
         {
            if (oidDig == pStatStruct[i].oidDig)
            {
               switch (pStatStruct[i].accessType)
               {
                  case IPTCOM_STAT_DATA_STRUCT:
                     arraySize = pStatStruct[i].size;
                     pStatStruct = (IPT_OID_DATA *)(pStatStruct[i].ptr.pNext);
                     oidInd++;
                     break;
      
                  case IPTCOM_STAT_DATA_PTR_R:
                  case IPTCOM_STAT_DATA_PTR_RW:
                     if (oidInd < IPT_STAT_OID_LEN - 2)
                     {
                        oidDig = pStatData->oid[oidInd+1];
                        if ((oidDig == 0) && (pStatData->oid[oidInd+2] == IPT_STAT_OID_STOPPER))
                        {
                           pStatData->type = pStatStruct[i].dataType;
/*
                           ret = getData(pStatData, pStatStruct[i].ptr.pVal);
*/
                           switch (pStatData->type)
                           {
                              case IPT_STAT_TYPE_INTEGER:
                                 pStatData->value.integer = *(pStatStruct[i].ptr.pInteger);
                                 break;

                              case IPT_STAT_TYPE_OCTET_STRING:
                                 strncpy(pStatData->value.octetString, pStatStruct[i].ptr.pOctetString, IPT_STAT_STRING_LEN);
                                 pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                                 break;

                              case IPT_STAT_TYPE_IPADDRESS:
                                 pStatData->value.ipAddress = *(pStatStruct[i].ptr.pIpAddress);
                                 break;

                              case IPT_STAT_TYPE_COUNTER:
                                 pStatData->value.counter = *(pStatStruct[i].ptr.pCounter);
                                 break;

                              case IPT_STAT_TYPE_TIMETICKS:
                                 pStatData->value.timeTicks = *(pStatStruct[i].ptr.pTimeTicks);
                                 break;

                              default:
                                 pStatData->value.counter = 0;
                                 ret = (int)IPT_INVALID_PAR;
                                 break;
                           }
                           done = 1;
                        }  
                        else
                        {
                           ret = (int)IPT_INVALID_PAR;
                        }
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  case IPTCOM_STAT_FUNC_NOKEY_R:
                     if (oidInd < IPT_STAT_OID_LEN - 2)
                     {
                        oidDig = pStatData->oid[oidInd+1];
                        if ((oidDig == 0) && (pStatData->oid[oidInd+2] == IPT_STAT_OID_STOPPER))
                        {
                           ret = (pStatStruct[i].ptr.pRfunc)(pStatData);
                           pStatData->type = pStatStruct[i].dataType;
                           done = 1;
                        }  
                        else
                        {
                           ret = (int)IPT_INVALID_PAR;
                        }
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }

                     break;

                  case IPTCOM_STAT_FUNC_NOKEY_RW:
                     if (oidInd < IPT_STAT_OID_LEN - 2)
                     {
                        oidDig = pStatData->oid[oidInd+1];
                        if ((oidDig == 0) && (pStatData->oid[oidInd+2] == IPT_STAT_OID_STOPPER))
                        {
                           ret = (pStatStruct[i].ptr.pRWfunc)(pStatData, 0);
                           pStatData->type = pStatStruct[i].dataType;
                           done = 1;
                        }  
                        else
                        {
                           ret = (int)IPT_INVALID_PAR;
                        }
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }

                     break;

                  case IPTCOM_STAT_FUNC_KEY_R:
                     ret = (pStatStruct[i].ptr.pRKeyfunc)(pStatData, oidInd, oidDig);
                     pStatData->type = pStatStruct[i].dataType;
                     done = 1;
                     break;
               
                  case IPTCOM_STAT_FUNC_KEY_RW:
                     ret = (pStatStruct[i].ptr.pRWKeyfunc)(pStatData, oidInd, oidDig, 0);
                     pStatData->type = pStatStruct[i].dataType;
                     done = 1;
                     break;
               
               
                  default:
                     ret = (int)IPT_INVALID_PAR;
                     break;
               }

               /* Break while loop */
               break;
            }
            else
            {
               i++;
               if (i >= arraySize)
               {
                  ret = (int)IPT_INVALID_PAR;
               }
            }
         }
      }
   }

   return(ret);
}

/*******************************************************************************
NAME:       iptStatGetNext
ABSTRACT:   Get or get next statistic variable.
RETURNS:    0 if OK, != if not
*/
int iptStatGetNext(
   IPT_STAT_DATA *pStatData)  /* IN/OUT Pointer to statistic data structure */
{
   UINT32 i;
   int k;
   UINT32 oidInd;
   UINT32 startOidInd;
   int ret = (int)IPT_OK;
   UINT32 oidDig;
   int done = 0;
   UINT32 nextOid[IPT_STAT_OID_LEN];
   int noMoreOidInReq;
   int endOfData = 1;
   UINT32 startArraySize;
   UINT32 arraySize;
   IPT_OID_DATA *pStartStatStruct;
   IPT_OID_DATA *pStatStruct;
   int getNextOid = 0;

   /* Get data structure */
   ret = getDataStructure(pStatData, &arraySize, &pStatStruct, &startOidInd);  
   startArraySize = arraySize;
   pStartStatStruct = pStatStruct;  
   if (ret == (int)IPT_OK)
   {
      oidInd = startOidInd;
      while ((oidInd < IPT_STAT_OID_LEN) && 
             (ret == (int)IPT_OK) &&
             (done == 0))
      {
         /* Next OID digit */
         oidDig = pStatData->oid[oidInd];

         if (oidDig == IPT_STAT_OID_STOPPER)
         {
            /* Get next OID digit from the current structure */
            oidDig = pStatStruct[0].oidDig;
            pStatData->oid[oidInd] = oidDig;
            if (oidInd < IPT_STAT_OID_LEN - 1)
            {
               pStatData->oid[oidInd+1] = IPT_STAT_OID_STOPPER;
            }
            noMoreOidInReq = 1;
         }
         else
         {
            noMoreOidInReq = 0;
         }

         /* Search for OID digit in the current structure */
         i = 0;
         while (ret == (int)IPT_OK)
         {
            if (oidDig == pStatStruct[i].oidDig)
            {
               /* Save next OID digit of the current structure */
               if (i < arraySize - 1)
               {
                  nextOid[oidInd] = pStatStruct[i + 1].oidDig;
                  endOfData = 0;
               }
               else
               {
                  nextOid[oidInd] = IPT_STAT_OID_STOPPER;
               }
            
               switch (pStatStruct[i].accessType)
               {
                  case IPTCOM_STAT_DATA_STRUCT:
                     arraySize = pStatStruct[i].size;
                     pStatStruct = pStatStruct[i].ptr.pNext;
                     oidInd++;
                     break;
      
                  case IPTCOM_STAT_DATA_PTR_R:
                  case IPTCOM_STAT_DATA_PTR_RW:
                  case IPTCOM_STAT_FUNC_NOKEY_R:
                  case IPTCOM_STAT_FUNC_NOKEY_RW:
                     /* Get next OID digit if any */
                     if (oidInd < IPT_STAT_OID_LEN - 2)
                     {
                        if (noMoreOidInReq)
                        {
                           oidDig = IPT_STAT_OID_STOPPER;
                        }
                        else
                        {
                           oidDig = pStatData->oid[oidInd+1];
                        }

                        /* Last OID digit in request, i.e. get data ? */
                        if (oidDig == IPT_STAT_OID_STOPPER)
                        {
                           pStatData->oid[oidInd+1] = 0;
                           pStatData->oid[oidInd+2] = IPT_STAT_OID_STOPPER;
                           pStatData->type = pStatStruct[i].dataType;
                        
                           switch (pStatStruct[i].accessType)
                           {
                              case IPTCOM_STAT_DATA_PTR_R:
                              case IPTCOM_STAT_DATA_PTR_RW:
                                 switch (pStatData->type)
                                 {
                                    case IPT_STAT_TYPE_INTEGER:
                                       pStatData->value.integer = *(pStatStruct[i].ptr.pInteger);
                                       break;

                                    case IPT_STAT_TYPE_OCTET_STRING:
                                       strncpy(pStatData->value.octetString, pStatStruct[i].ptr.pOctetString, IPT_STAT_STRING_LEN);
                                       pStatData->value.octetString[IPT_STAT_STRING_LEN] = 0;
                                       break;

                                    case IPT_STAT_TYPE_IPADDRESS:
                                       pStatData->value.ipAddress = *(pStatStruct[i].ptr.pIpAddress);
                                       break;

                                    case IPT_STAT_TYPE_COUNTER:
                                       pStatData->value.counter = *(pStatStruct[i].ptr.pCounter);
                                       break;

                                    case IPT_STAT_TYPE_TIMETICKS:
                                       pStatData->value.timeTicks = *(pStatStruct[i].ptr.pTimeTicks);
                                       break;

                                    default:
                                       pStatData->value.counter = 0;
                                       ret = (int)IPT_INVALID_PAR;
                                       break;
                                 }
                                 break;

                              case IPTCOM_STAT_FUNC_NOKEY_R:
                                 ret = (pStatStruct[i].ptr.pRfunc)(pStatData);
                                 break;

                              case IPTCOM_STAT_FUNC_NOKEY_RW:
                                 ret = (pStatStruct[i].ptr.pRWfunc)(pStatData, 0);
                                 break;

                              default:
                                 ret = (int)IPT_ERROR;
                                 break;

                           }
                           if (ret == (int)IPT_OK)
                           {
                              done = 1;
                           }
                           else
                           {
                              ret = (int)IPT_OK;
                              getNextOid = 1;;
                           }
                        }  
                        else
                        {
                           /* Get next OID, i.e OID for data request */
                           /* Correct OID in request? */
                           if ((oidDig == 0) && (pStatData->oid[oidInd+2] == IPT_STAT_OID_STOPPER))
                           {
                              getNextOid = 1;;
                           }  
                           else
                           {
                              ret = (int)IPT_INVALID_PAR;
                           }
                        }
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  case IPTCOM_STAT_FUNC_KEY_R:
                     ret = (pStatStruct[i].ptr.pRKeyfunc)(pStatData,oidInd,oidDig);
                     if (ret == (int)IPT_OK)
                     {
                        pStatData->type = pStatStruct[i].dataType;
                        done = 1;
                     }
                     else
                     {
                        ret = (int)IPT_OK;
                        getNextOid = 1;;
                     }
                     break;

                  case IPTCOM_STAT_FUNC_KEY_RW:
                     ret = (pStatStruct[i].ptr.pRWKeyfunc)(pStatData, oidInd, oidDig, 0);
                     if (ret == (int)IPT_OK)
                     {
                        pStatData->type = pStatStruct[i].dataType;
                        done = 1;
                     }
                     else
                     {
                        ret = (int)IPT_OK;
                        getNextOid = 1;;
                     }
                     break;

                  case IPTCOM_STAT_DATA_PTR_W:
                  case IPTCOM_STAT_FUNC_NOKEY_W:
                  case IPTCOM_STAT_FUNC_KEY_W:
                     /* Get next OID, i.e OID for data request.
                        Prepare OID for next/data request */
                        getNextOid = 1;;
                     break;
               
                     /* CR-3477, BL, 2012-02-10, add default case */
                  default:
                     ret = (int)IPT_INVALID_PAR;
                     break;
               }

               if (getNextOid)
               {
                  getNextOid = 0;

                  if (endOfData)
                  {
                     pStatData->type = pStatStruct[i].dataType;
                     pStatData->value.counter = 0;
                     done = 1;
                  }
                  else
                  {
                     if (oidInd < IPT_STAT_OID_LEN - 1)
                     {
                        pStatData->oid[oidInd+1] = IPT_STAT_OID_STOPPER;
                     }
                     for (k = oidInd; k >= 0; k--)
                     {
                        if (nextOid[k] != IPT_STAT_OID_STOPPER)
                        {
                           pStatData->oid[k] = nextOid[k];
                           break;
                        }
                        else
                        {
                           pStatData->oid[k] = IPT_STAT_OID_STOPPER;
                        }
                     }
                     arraySize = startArraySize;
                     pStatStruct = pStartStatStruct;
                     oidInd = startOidInd;
                  }
               }

               /* Break while loop */
               break;
            }
            else
            {
               i++;
               if (i >= arraySize)
               {
                  ret = (int)IPT_INVALID_PAR;
               }
            }
         }
      }
   }

   return(ret);
}

/*******************************************************************************
NAME:       iptStatSet
ABSTRACT:   Set data
RETURNS:    0 if OK, != if not
*/
int iptStatSet(
   IPT_STAT_DATA *pStatData) /* IN/OUT Pointer to statistic data structure */
{
   UINT32 i;
   UINT32 oidInd;
   UINT32 startOidInd;
   int ret = (int)IPT_OK;
   UINT32 oidDig;
   int done = 0;
   UINT32 arraySize;
   IPT_OID_DATA *pStatStruct;

   
   /* Get data structure */
   ret = getDataStructure(pStatData, &arraySize, &pStatStruct, &startOidInd);  
   if (ret == (int)IPT_OK)
   {
      oidInd = startOidInd;
      while ((pStatData->oid[oidInd] != IPT_STAT_OID_STOPPER) &&
             (oidInd < IPT_STAT_OID_LEN) && 
             (ret == (int)IPT_OK) &&
             (done == 0))
      {
         oidDig = pStatData->oid[oidInd];
         i = 0;
         while (ret == (int)IPT_OK)
         {
            if (oidDig == pStatStruct[i].oidDig)
            {
               switch (pStatStruct[i].accessType)
               {
                  case IPTCOM_STAT_DATA_STRUCT:
                     arraySize = pStatStruct[i].size;
                     pStatStruct = (IPT_OID_DATA *)(pStatStruct[i].ptr.pNext);
                     oidInd++;
                     break;
      
                  case IPTCOM_STAT_DATA_PTR_W:
                  case IPTCOM_STAT_DATA_PTR_RW:
                     if (oidInd < IPT_STAT_OID_LEN - 2)
                     {
                        oidDig = pStatData->oid[oidInd+1];
                        if ((oidDig == 0) && (pStatData->oid[oidInd+2] == IPT_STAT_OID_STOPPER))
                        {
                           pStatData->type = pStatStruct[i].dataType;
                           switch (pStatData->type)
                           {
                              case IPT_STAT_TYPE_INTEGER:
                                 *(pStatStruct[i].ptr.pInteger) = pStatData->value.integer;
                                 break;

                              case IPT_STAT_TYPE_OCTET_STRING:
                                 if (strlen(pStatData->value.octetString) <= IPT_STAT_STRING_LEN)
                                 {
                                    strcpy(pStatStruct[i].ptr.pOctetString, pStatData->value.octetString);
                                 }
                                 else
                                 {
                                    ret = (int)IPT_INVALID_PAR;
                                 }
                                 break;

                              case IPT_STAT_TYPE_IPADDRESS:
                                 *(pStatStruct[i].ptr.pIpAddress) = pStatData->value.ipAddress;
                                 break;

                              case IPT_STAT_TYPE_COUNTER:
                                 *(pStatStruct[i].ptr.pCounter) = pStatData->value.counter;
                                 break;

                              case IPT_STAT_TYPE_TIMETICKS:
                                 *(pStatStruct[i].ptr.pTimeTicks) = pStatData->value.timeTicks;
                                 break;

                              default:
                                 ret = (int)IPT_INVALID_PAR;
                                 break;
                           }
                           done = 1;
                        }  
                        else
                        {
                           ret = (int)IPT_INVALID_PAR;
                        }
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  case IPTCOM_STAT_FUNC_NOKEY_W:
                     if (oidInd < IPT_STAT_OID_LEN - 2)
                     {
                        oidDig = pStatData->oid[oidInd+1];
                        if ((oidDig == 0) && (pStatData->oid[oidInd+2] == IPT_STAT_OID_STOPPER))
                        {
                           pStatData->type = pStatStruct[i].dataType;
                           ret = (pStatStruct[i].ptr.pWfunc)(pStatData);
                           done = 1;
                        }  
                        else
                        {
                           ret = (int)IPT_INVALID_PAR;
                        }
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  case IPTCOM_STAT_FUNC_NOKEY_RW:
                     if (oidInd < IPT_STAT_OID_LEN - 2)
                     {
                        oidDig = pStatData->oid[oidInd+1];
                        if ((oidDig == 0) && (pStatData->oid[oidInd+2] == IPT_STAT_OID_STOPPER))
                        {
                           pStatData->type = pStatStruct[i].dataType;
                           ret = (pStatStruct[i].ptr.pRWfunc)(pStatData, 1);
                           done = 1;
                        }  
                        else
                        {
                           ret = (int)IPT_INVALID_PAR;
                        }
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  case IPTCOM_STAT_FUNC_KEY_W:
                     if (oidInd < IPT_STAT_OID_LEN - 2)
                     {
                        oidDig = pStatData->oid[oidInd+1];
                        if ((oidDig == 0) && (pStatData->oid[oidInd+2] == IPT_STAT_OID_STOPPER))
                        {
                           pStatData->type = pStatStruct[i].dataType;
                           ret = (pStatStruct[i].ptr.pWKeyfunc)(pStatData,oidInd,oidDig);
                           done = 1;
                        }  
                        else
                        {
                           ret = (int)IPT_INVALID_PAR;
                        }
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  case IPTCOM_STAT_FUNC_KEY_RW:
                     if (oidInd < IPT_STAT_OID_LEN - 2)
                     {
                        oidDig = pStatData->oid[oidInd+1];
                        if ((oidDig == 0) && (pStatData->oid[oidInd+2] == IPT_STAT_OID_STOPPER))
                        {
                           pStatData->type = pStatStruct[i].dataType;
                           ret = (pStatStruct[i].ptr.pRWKeyfunc)(pStatData, oidInd, oidDig , 1);
                           done = 1;
                        }  
                        else
                        {
                           ret = (int)IPT_INVALID_PAR;
                        }
                     }
                     else
                     {
                        ret = (int)IPT_INVALID_PAR;
                     }
                     break;

                  default:
                     ret = (int)IPT_INVALID_PAR;
                     break;
               }

               /* Break while (ret == (int)IPT_OK) loop */
               break;
            }
            else
            {
               i++;
               if (i >= arraySize)
               {
                  ret = (int)IPT_INVALID_PAR;
               }
            }
         }
      }
   }

   return(ret);

}

/*******************************************************************************
NAME:       IPTCom_statGet
ABSTRACT:   Get statistic variable.
RETURNS:    0 if OK, != if not
*/
int IPTCom_statGet(
   const char *pRequest,  /* Pointer to request buffer */
   char *pResponse,       /* Pointer to response buffer */
   UINT32 length)         /* Length of response buffer */
{
#ifdef TARGET_SIMU
   IPT_UNUSED (pRequest)
   IPT_UNUSED (pResponse)
   IPT_UNUSED (length)
   
   return(IPT_NOT_FOUND);
#else
   return(iptStat(pRequest, pResponse, length, IPTCOM_STAT_GET));
#endif
}

/*******************************************************************************
NAME:       IPTCom_statGetNext
ABSTRACT:   Get statistic variable.
RETURNS:    0 if OK, != if not
*/
int IPTCom_statGetNext(
   const char *pRequest,  /* Pointer to request buffer */
   char *pResponse,       /* Pointer to response buffer */
   UINT32 length)         /* Length of response buffer */
{
#ifdef TARGET_SIMU
   IPT_UNUSED (pRequest)
   IPT_UNUSED (pResponse)
   IPT_UNUSED (length)

   return(IPT_NOT_FOUND);
#else
   return(iptStat(pRequest, pResponse, length, IPTCOM_STAT_GETNEXT));
#endif
}

/*******************************************************************************
NAME:       IPTCom_statSet
ABSTRACT:   Set statistic variable, i.e clear statistics
RETURNS:    0 if OK, != if not
*/
int IPTCom_statSet(
   const char *pRequest,  /* Pointer to request buffer */
   char *pResponse,       /* Pointer to response buffer */
   UINT32 length)         /* Length of response buffer */
{
#ifdef TARGET_SIMU
   IPT_UNUSED (pRequest)
   IPT_UNUSED (pResponse)
   IPT_UNUSED (length)
   
   return(IPT_NOT_FOUND);
#else
   return(iptStat(pRequest, pResponse, length, IPTCOM_STAT_SET));
#endif
}

