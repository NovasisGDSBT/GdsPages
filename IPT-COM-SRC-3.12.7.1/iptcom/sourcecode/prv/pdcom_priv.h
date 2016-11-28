/*******************************************************************************
 *  COPYRIGHT   : (C) 2006-2014 Bombardier Transportation
 ********************************************************************************
 *  PROJECT     : IPTrain
 *
 *  MODULE      : pdcom_priv.h
 *
 *  ABSTRACT    : Private header file for pdCom, part of IPTCom
 *
 ********************************************************************************
 *  HISTORY     :
 *
 * $Id: pdcom_priv.h 33666 2014-07-17 14:43:01Z gweiss $
 *
 *  CR-7779 (Gerhard Weiss 2014-07-01)
 *          added check for receiving MD frame len (configurable)
 *
 *  CR-1356 (Bernd Loehr, 2010-10-08)
 * 			Additional function iptConfigAddDatasetExt to define dataset
 *          dependent un/marshalling.
 *
 *  CR-62 (Bernd Loehr, 2010-08-25)
 * 			Additional function pdSub_renew_all() to renew all MC memberships
 * 			after link down/up event
 *
 *  Internal (Bernd Loehr, 2010-08-16)
 * 			Old obsolete CVS history removed
 *
 *******************************************************************************/

#ifndef PDCOM_PRIV_H
#define PDCOM_PRIV_H

/*******************************************************************************
   *  INCLUDES */

#ifdef __cplusplus
extern "C" {
#endif
   
/*******************************************************************************
*  DEFINES
*/
#define PD_NSCHEDGRP       20    /* No of schedule groups + netbuffer */
#define PD_NETBUFFER_IX    0     /* Schedule group index for netbuffer */
#define PD_NETBUFFER_ID    0     /* Fake schedule group id for netbuffer */
#define PD_ALL_SOURCES_IP  0     /* IP address for ALL sources */
   
#define PD_DATASET_MAXSIZE 1000  /* Max size of a PD dataset */
#define PD_PROTOCOL_VERSION   0x02000000
#define PD_TYPE            0x5044 /* "PD" */  
   
#define IP_ADDRESS_NOT_RESOLVED 0xffffffff

/* Status for subscribe control block and net control block */
#define CB_READY  0
#define WAIT_FOR_DATA 1
#define WAIT_FOR_TDC  2
#define CB_ADDED 1

/*******************************************************************************
*  TYPEDEFS
*/

/* Config dataset formatting table (internal) */
struct iptDatasetFormatInt
{
   INT32 id;      /* Data type (IPT_UINT32 etc, <0) or Dataset number (>0) */
   UINT32 size;   /* Number of items, or IPT_VAR_SIZE */
   UINT16 nLines;               /* No of lines in format table below */
   UINT16 alignment;            /* Memory alignment for this dataset (size of
                                   largest data in the dataset)*/
   struct iptDatasetFormatInt *pFormat; /* Pointer to formatting table when id > 0 */
};
typedef struct iptDatasetFormatInt IPT_DATA_SET_FORMAT_INT;

/* Config dataset parameters  (internal) */
typedef struct
{
   UINT32 datasetId;            /* Dataset ID */
   UINT32 size;                 /* Size of dataset if fixed, 0 if variable */
   UINT16 alignment;            /* Memory alignment for this dataset (size of
                                   largest data in the dataset)*/
   UINT16 nLines;               /* No of lines in format table below */
   UINT16 prepared;             /* Set TRUE when size and alignment have been
                                   calculated by IPTCom */
   UINT16 varSize;              /* Set TRUE if size is variable */
   UINT8  disableMarshalling;   /* 1 if this dataset should not be handled */
   IPT_DATA_SET_FORMAT_INT *pFormat; /* Pointer to formatting table */
} IPT_CFG_DATASET_INT;

/* PD Frame Header structure. Do not change without knowledge about alignment. */
typedef struct pdHeader
{
   UINT32 timeStamp;
   UINT32 protocolVersion;
   UINT32 topoCount;
   UINT32 comId;
   UINT16 type;
   UINT16 datasetLength;
   UINT16 reserved;
   UINT16 headerLength;
   UINT32 hdFCS;
} PD_HEADER;

/* Receive communication parameters */
/* Send ComId net control block */
struct pdSendNetCB
{
   struct pdSendNetCB *pNext;       /* Pointer to next CB */
   struct pdSendNetCB *pNextToSend; /* Pointer to next CB to be sent in the same cycle */
   UINT32 comId;                    /* Com ID */
#if defined(IF_WAIT_ENABLE)
   UINT32 comParId;                 /* Communication parameter ID */
#endif
   UINT16 size;                     /* Size of sendbuffer including header and data with checksum(s) */
   UINT16 nPublisher;               /* No of publishers registered for this comid in this sched group */
   UINT8 updatedOnceNs;             /* True if data has been changed in schedGrpBuffer */
   UINT8 status;                    /*  */
   BYTE *pSendBuffer;               /* Pointer to send buffer */
   UINT32 redFuncId;                /* Redundant function ID. 0 = not redundant */
   UINT32 leader;                   /* TRUE if this comid is redundant leader
                                       or not a redundant comid */
   UINT32 cycleMultiple;            /* Multiple of the PD send thread cycle time when to send the telegram */
   UINT32 destIp;                   /* Destination IP address */
#ifdef TARGET_SIMU
   UINT32 simDeviceIp;              /* Source IP Address simulation, used for simulation */
#endif
   int pdSendSocket;                /* socket used for PD sending with selected ttl and qos. */
   UINT32 pdOutPackets;             /* Number of transmitted PD packets */
};
typedef struct pdSendNetCB PD_SEND_NET_CB;

/* Receive ComId net control block */
struct pdRecNetCB
{
   struct pdRecNetCB *pNext; /* Pointer to next CB */
   struct pdRecNetCB *pPrev; /* Pointer to previous CB */
   UINT32 comId;             /* Com ID */
   UINT16 size;              /* Size of dataset for this com id */
   UINT16 alignment;            /* Memory alignment for this dataset (size of
                                   largest data in the dataset)*/
   UINT16 nLines;               /* No of lines in format table below */
   UINT8	 disableMarshalling;
   IPT_DATA_SET_FORMAT_INT *pDatasetFormat; /* Pointer to formatting table */
   UINT16 prepared;             /* Set TRUE when size and alignment have been
                                   calculated by IPTCom */
   UINT16 varSize;              /* Set TRUE if size is variable */
   UINT16 nSubscriber;       /* No of subscribers registered for this comid in this sched group */
   UINT16 updatedOnceNr;       /* True if data has been receive at least once */
   BYTE *pDataBuffer;        /* Pointer to data buffer */
   UINT16 invalid;           /* Set TRUE if value is invalid */
   UINT32 timeRec;           /* Time when received, in ms */
   UINT32 sourceIp;          /* Source IP address */
#ifdef TARGET_SIMU
	UINT32 simDeviceIp;		  /* Destination device IP (simulation) */
#endif	
   UINT32 pdInPackets;       /* Number of received PD packets */
};
typedef struct pdRecNetCB PD_REC_NET_CB;

/* Common for Publish and Subscription control block */
typedef struct 
{
   UINT32 comId;            /* Com ID */
   UINT8 invalid;           /* Set TRUE if value is invalid */
   UINT8 updatedOnce;       /* True if data has been updated at least once */
   UINT16 size;             /* Size of dataset for this com id */
   BYTE *pDataBuffer;       /* Pointer to data buffer */
} PD_CB;

/* Publish control block */
struct pdPubCB
{
   struct pdPubCB *pNext;   /* Pointer to next CB */
   struct pdPubCB *pPrev;   /* Pointer to previous CB */
   PD_CB  pdCB;             /* Common structure for publish and subscribe */
   UINT8 pubCBchanged;      /* True if data has been changed in schedGrpBuffer */
   UINT8 waitingTdc;        /* True if waiting for TDC configuration */
   UINT32 schedGrp;         /* Schedule group */
   UINT32 destId;           /* Destination ID */
   char  *pDestUri;         /* Pointer to desination URI */
   UINT32 destIp;           /* Destination IP address */
   UINT8  defStart;         /* True if defered start */
   UINT16 alignment;        /* Memory alignment for this dataset (size of
                               largest data in the dataset)*/
   UINT16 nLines;            /* No of lines in format table below */
   UINT8  disableMarshalling;  /* Whether this dataset should not be changed */
   IPT_DATA_SET_FORMAT_INT *pDatasetFormat; /* Pointer to formatting table */
   struct pdSendNetCB *pSendNetCB; /* Pointer to control block for netbuffer */
   UINT32 netDatabufferSize; /* max size of net control data buffer */
   BYTE *pNetDatabuffer;    /* Pointer to the data in netbuffer */
   PD_HEADER *pPdHeader;    /* Pointer to the PD header in netbuffer */
#ifdef TARGET_SIMU
   char  *pSimUri;          /* Pointer to source URI (used for simulation) */
#endif   
};
typedef struct pdPubCB PD_PUB_CB;

/* Subscribe control block */
struct pdSubCB
{
   struct pdSubCB *pNext;  /* Pointer to next CB */
   struct pdSubCB *pPrev;  /* Pointer to previous CB */
   PD_CB  pdCB;             /* Common structure for publish and subscribe */
   UINT8 noOfNetCB;        /* Number of net control nlocks */
   UINT8 invalidBehaviour; /* Behavior at invalid data */
   UINT8 waitingTdc;       /* True if waiting for TDC configuration */
   UINT32 schedGrp;        /* Schedule group */
   UINT32 timeout;         /* Timeout before considering invalid, in ms */
   UINT32 destId;          /* Destination ID */
   char  *pDestUri;        /* Pointer to desination URI */
   UINT32 destIp;          /* Destination IP address */
   UINT32 filterId;        /* Source filter id */
   char  *pSourceUri;      /* Pointer to filter source URI */
   struct pdRecNetCB *pRecNetCB[MAX_SRC_FILTER]; /* Pointers to control block
                                                    for netbuffer */
   BYTE *pNetbuffer[MAX_SRC_FILTER];             /* Pointers to the data in
                                                    netbuffer */
#ifdef TARGET_SIMU   
   UINT32 simDeviceIp;     /* Source IP Address simulation, used for simulation */
   char  *pSimUri;         /* Pointer to destination URI (used for simulation) */
#endif   
};
typedef struct pdSubCB PD_SUB_CB;

/* Receive look up table */
typedef struct
{
   UINT32 filtIpAddr;         /* Source IP address, 0 if "all" */
   PD_REC_NET_CB *pFiltRecNetCB;
} FILTRECTAB_ITEM;

typedef struct
{
   UINT32 comId;         /* ComId */
#ifdef TARGET_SIMU   
   UINT32 simIpAddr;         /* Destination device IP Address, 0 if not used (simulation) */
#endif   
   PD_REC_NET_CB *pRecNetCB;
   UINT16 nItems;       /* No of items in table */
   UINT16 maxItems;     /* Current max no of items in table */
   FILTRECTAB_ITEM *pFiltTable; /* Pointer to table with source fliter IP addresses */
   UINT32 tableSize;    /* Current table size */
} RECTAB_ITEM;
typedef struct 
{
   UINT16 nItems;       /* No of items in table */
   UINT16 maxItems;     /* Current max no of items in table */
   RECTAB_ITEM *pTable; /* Pointer to table, dynamically allocated */
   UINT32 tableSize;    /* Current table size */
} RECTAB;

typedef struct 
{
   UINT32 schedGrp;          /* Schedule group */
   struct pdPubCB *pFirstCB; /* Pointer to first CB */
} PD_PUBSHED_GRP;

typedef struct 
{
   UINT16 initialized;     /* True if table is initialized */
   UINT16 nItems;          /* No of items in table */
   UINT16 maxItems;        /* Current max no of items in table */
   PD_PUBSHED_GRP *pTable; /* Pointer to table items, dynamically allocated */
} PUBGRPTAB;

typedef struct 
{
   UINT32 schedGrp;              /* Schedule group */
   struct pdSubCB *pFirstCB;   /* Pointer to first CB */
} PD_SUBSHED_GRP;

typedef struct 
{
   UINT16 initialized;  /* True if table is initialized */
   UINT16 nItems;       /* No of items in table */
   UINT16 maxItems;     /* Current max no of items in table */
   PD_SUBSHED_GRP *pTable; /* Pointer to table items, dynamically allocated */
} SUBGRPTAB;

/* Publish control block not yet resolved, i.e waiting for TDC to be ready */
struct pdNotResPubCB
{
   struct pdNotResPubCB *pNext; /* Pointer to next CB */
   struct pdNotResPubCB *pPrev; /* Pointer to previous CB */
   struct pdPubCB *pPubCB;      /* Pointer to first not resolved publish CB */
};
typedef struct pdNotResPubCB PD_NOT_RESOLVED_PUB_CB;

/* Subscribe control block not yet resolved, i.e waiting for TDC to be ready */
struct pdNotResSubCB
{
   struct pdNotResSubCB *pNext; /* Pointer to next CB */
   struct pdNotResSubCB *pPrev; /* Pointer to previous CB */
   struct pdSubCB *pSubCB;      /* Pointer to first not resolved publish CB */
};
typedef struct pdNotResSubCB PD_NOT_RESOLVED_SUB_CB;

/* PD statistic */
typedef struct
{
   UINT32 pdInPackets;           /* Number of received PD packets */
   UINT32 pdInFCSErrors;         /* Number of received PD packets with FCS errors */
   UINT32 pdInProtocolErrors;    /* Number of received PD packets with wrong protocol */
   UINT32 pdInTopoErrors;        /* Number of received PD packets with wrong topocounter */
   UINT32 pdInNoSubscriber;      /* Number of received PD packets without subscriber */
   UINT32 pdOutPackets;          /* Number of transmitted PD packets */
   UINT32 pdStatisticsStarttime; /* Start time for counting statitics */
} PD_STATISTIC;

typedef struct
{ 
   UINT32 noOfNcb;              /* Number of net control block in the linked list */
   PD_SEND_NET_CB *pSendItems;  /* Pointer to linked list of net control blocks */
} PD_CYCLE_SLOT_ITEM;

typedef struct
{ 
   UINT32 cycleMultiple;             /* Multiple of PD send thread cycle time */
   UINT32 sendIndex;                 /* Index of the slot with net control blocks to be send */
   UINT32 tableSize;                 /* Size of the send table */
   UINT32 curMaxNoOfNcb;             /* The current maximum of number of net control in a slot */
   PD_CYCLE_SLOT_ITEM *pSendTable;   /* Pointer to send table of slots with a linked list of net control blocks */
} PD_CYCLE_ITEM;

typedef struct
{
   UINT32 redId;                     /* Redundant ID */
   UINT32 leader;                    /* Status */
} PD_RED_ID_ITEM;

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/
int pdGrpTabInit(void);
int pdPubGrpTabFind(UINT32 schedGrp, int *pIndex);
int pdSubGrpTabFind(UINT32 schedGrp, int *pIndex);
void pdGrpTabTerminate(void);

void pdSendNetCB_destroy(PD_SEND_NET_CB *pNetCB);
void pdRecNetCB_destroy(PD_REC_NET_CB *pNetCB);

#ifdef TARGET_SIMU
PD_SUB_CB *pdSubComidCB_get(UINT32 schedGrp, UINT32 comId, UINT32 filterId, const char *pSource, UINT32 destId, const char *pDest, const char *pSimUri);
PD_PUB_CB *pdPubComidCB_get(UINT32 schedGrp, UINT32 comId, UINT32 destId, const char *pDest, UINT8 defStart, const char *pSource);
#else
PD_SUB_CB *pdSubComidCB_get(UINT32 schedGrp, UINT32 comId, UINT32 filterId, const char *pSource, UINT32 destId, const char *pDest);
PD_PUB_CB *pdPubComidCB_get(UINT32 schedGrp, UINT32 comId, UINT32 destId, const char *pDest, UINT8 defStart);
#endif
int pdSub_renew(PD_SUB_CB *pSubCB);
int pdPub_renew(PD_PUB_CB *pPubCB);
int PD_sub_renew_all(void);
void pdComidPubCB_cleanup(PD_PUB_CB *pPubCB);
void pdComidSubCB_cleanup(PD_SUB_CB *pSubCB);
void PD_finish_subscribe_publish(void);

/* Receive table functions */
int pdRecTabInit(void);
void pdRecTabTerminate(void);

void PDCom_send(void);

#ifdef TARGET_SIMU
int pdRecTabAdd(UINT32 comId, UINT32 simIpAddr, UINT32 filtIpAddr, PD_REC_NET_CB *pNetCB);
   /* For simulation target we also retrieve the source ip address of the net control block */
void PDCom_receive(UINT32 sourceIPAddr, UINT32 simDevIPAddr, BYTE *pPayLoad,	int frameLen);
void pdRecTabDelete(UINT32 comId, UINT32 simIpAddr, UINT32 filtIpAddr);
#else
int pdRecTabAdd(UINT32 comId, UINT32 filtIpAddr, PD_REC_NET_CB *pNetCB);
void PDCom_receive(UINT32 destIPAddr, BYTE *pPayLoad,	int frameLen);
void pdRecTabDelete(UINT32 comId, UINT32 filtIpAddr);
#endif

void PDCom_showStatistic(void);
int PDCom_clearStatistic(void);

#ifdef __cplusplus
}
#endif

#endif
