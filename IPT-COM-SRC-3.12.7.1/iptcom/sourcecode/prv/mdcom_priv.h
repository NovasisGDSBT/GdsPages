/*******************************************************************************
*  COPYRIGHT   : (C) 2006-2010 Bombardier Transportation
********************************************************************************
*  PROJECT     : IPTrain
*
*  MODULE      : mdcom_priv.h
*
*  ABSTRACT    : Private header file for MDCom, part of IPTCom
*
********************************************************************************
*  HISTORY     :
*	
* $Id: mdcom_priv.h 11658 2010-08-26 08:36:33Z bloehr $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*  CR-62 (Bernd Loehr, 2010-08-25)
* 			Additional function MD_renew_mc_listeners() to renew all MC
* 			memberships after link down/up event
*
*
*******************************************************************************/

#ifndef MDCOM_PRIV_H
#define MDCOM_PRIV_H

/*******************************************************************************
*  INCLUDES */
   
#ifdef __cplusplus
extern "C" {
#endif
   
/*******************************************************************************
*  DEFINES
*/
   
/* Protocol version */
#define MD_PROTOCOL_VERSION 0x02010000
   
/* Message types used in communication frame */
#define ACK_MSG          0x4d41  /* 'MA' Acknowledge message */
#define FRG_ACK_MSG      0x4d61  /* 'Ma' Acknowledge message for a function redundancy
                                         group multicast message*/
#define MD_ACK_MSG       0x4d44  /* 'MD' Message data message when acknowledge message 
                                         is expected from the receiver transport layer */
#define MD_NOACK_MSG     0x4d64  /* 'Md' Message data message when no acknowledge message 
                                         is expected from the receiver transport layer */
#define MD_FRG_MSG       0x4d67  /* 'Mg, Message data message to a function redundancy
                                         group. Acknowledge message is expected from the
                                         receiver transport layer */ 
#define MD_REQ_ACK_MSG   0x4d51  /* 'MQ' Request data message when acknowledge message 
                                         is expected from the receiver transport layer */
#define MD_REQ_NOACK_MSG 0x4d71  /* 'Mq' Request data message when no acknowledge message 
                                         is expected from the receiver transport layer */
#define MD_REQ_FRG_MSG   0x4d47  /* 'MG' Request data message to a function redundancy
                                         group. Acknowledge message is expected from the
                                         receiver transport layer */
#define MD_REPLY_MSG     0x4d52  /* 'MR' Reply message, acknowledge expected from 
                                         the receiver transport layer */
   
/* Acknowledge code used in communication acknowledge messages */
#define ACK_OK                    0
#define ACK_WRONG_FCS             1
#define ACK_DEST_UKNOWN           2
#define ACK_WRONG_DATA            3
#define ACK_BUFFER_NOT_AVAILABLE  4
   
/* Default value in milliseconds for waiting on acknowledge message */
#define ACK_TIME_OUT_VAL_DEF 500
   
/* Default value in milliseconds for waiting on reply message */
#define REPLY_TIME_OUT_VAL_DEF 5000
   
/* Default value for number of resending including the first on transport layer */
#define MAX_RESEND 3
   
/* Definition used for destination URI addresses */
#define ONE_INST_ONE_FUNCT  0
#define ALL_INST_ONE_FUNCT  1
#define ALL_INST_ALL_FUNCT  2
#define ONE_INST_ALL_FUNCT  3
#define MULTIPLE_LISTENER   4
   
   /* For transport */
#define MAX_NO_OF_ACTIVE_SEQ_NO_DEF 50
   
/* Funtion redundancy group */
#define FRG_FOLLOWER 0
#define FRG_LEADER   1


#define IPT_MAX_HOST_URI_LEN	(4*(IPT_MAX_LABEL_LEN+1))

/* User URI types */
#define NO_USER_URI     0 
#define INSTX_FUNCN_URI 1
#define AINST_FUNCN_URI 2
#define INSTX_AFUNC_URI 3
#define AINST_AFUNC_URI 4


/*******************************************************************************
*  TYPEDEFS
*/
   
/* Stucture for internal queues */
typedef struct
{
   long     type;       /* Only used for Linux */
   MSG_INFO msgInfo;
   UINT32   msgLength;  /* Message length */
   char     *pMsgData;  /* Pointer to dataset */
} QUEUE_MSG;
   
/* Communication parameter used for a communication instance */
typedef struct mdComPar
{
   UINT32 destIpAddr;      /* Destination IP address */
#ifdef TARGET_SIMU
   UINT32 simDevIpAddr;    /* Simulated IP address */
#endif
   int maxResend;          /* Maximum number of resending on transport
   layer */
   UINT32 ackTimeOutVal;   /* Time-out value in milliseconds for waiting on acknowledge  */
   UINT32 replyTimeOutVal; /* Time-out value in milliseconds for waiting for
                              a reply on after request message */
   int   mdSendSocket;
} MD_COM_PAR;
   
typedef struct sessionInstance
{
   struct sessionInstance *pNext;
   int castType;
   int state;
   UINT32 sessionId;
   const void *pCallerRef;
   UINT32 replierIpAddr;
   UINT32 comId;
   MD_QUEUE callerQueueId;
   IPT_REC_FUNCPTR callerFunc;       /* Pointer to callback function */
   char *pSendMsg;
   int ackReceived;
   int trResCode;
   UINT16 expectedNoOfReplies;
   UINT16 replyCount;
   UINT32 replyTimeOutVal;          /* in milliseconds */
   UINT32 timeOutTime;              /* in milliseconds */
   int recActive;
} SESSION_INSTANCE;

typedef struct seqNoListItem
{
   UINT16 seqNo;
   UINT16 lastAckCode;
} SEQ_NO_LIST_ITEM;

typedef struct recSeqCnt
{
   struct recSeqCnt *pNext; /* Pointer to next instance */
   UINT32 srcIpAddr;
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;
#endif
   int    lastIndex;
   UINT16 lastSeqNo;
   SEQ_NO_LIST_ITEM *pSeqNoList;
} REC_SEQ_CNT;

typedef struct recFrgSeqCnt
{
   struct recFrgSeqCnt *pNext; /* Pointer to next instance */
   UINT32 srcIpAddr;
   UINT32 destIpAddr;
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;
#endif
   int    lastIndex;
   UINT16 lastSeqNo;
   SEQ_NO_LIST_ITEM *pSeqNoList;
} REC_FRG_SEQ_CNT;

typedef struct sendSeqCnt
{
   struct sendSeqCnt *pNext; /* Pointer to next instance */
   UINT32 destIpAddr;        /* Destination IP address */
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;
#endif
   UINT16 sendSequenceNo;    /* last sequence number for this IP address */
   UINT16 *pActiveSeqNoList; /* Pointer to list of active TR instances to this IP address */
   int seqNoSync;
   UINT16 maxActiveSeqNo;
} SEND_SEQ_CNT;
   
   
typedef struct transportInstance
{
   struct transportInstance *pNext; /* Pointer to next TR instance */
   int trType;                      /* Transport layer communication type */
   int state;
   UINT16 sequenceNumber;
   SEND_SEQ_CNT *pSendSeqCntStr;
   int seqCntInd;
   const void *pCallerRef;
   UINT32 replierIpAddr;
   UINT32 comId;
   MD_QUEUE callerQueueId;
   IPT_REC_FUNCPTR callerFunc;       /* Pointer to callback function */
   SESSION_INSTANCE *pSeInstance;
   MD_COM_PAR comPar;
   UINT32 sendMsgLength;
   char *pSendMsg;
   int ackReceived;
   UINT16 ackCode;
   int sendCnt;
   UINT32 timeOutTime;         /* Acknowledge time-out in milliseconds */
} TRANSPORT_INSTANCE;

/* MD statistic */
typedef struct
{
   UINT32 mdInPackets;           /* Number of received MD packets */
   UINT32 mdInFCSErrors;         /* Number of received MD packets with FCS errors */
   UINT32 mdInProtocolErrors;    /* Number of received MD packets with wrong protocol */
   UINT32 mdInTopoErrors;        /* Number of received MD packets with wrong topocounter */
   UINT32 mdInNoListeners;       /* Number of received MD packets without listener */
   UINT32 mdNotReturnedBuffers;  /* Number of not returned buffer borrowed by the application */
   UINT32 mdOutPackets;          /* Number of transmitted MD packets */
   UINT32 mdOutRetransmissions;  /* Number of re-transmitted MD packets */
   UINT32 mdStatisticsStarttime; /* Start time for counting statitics */
} MD_STATISTIC;

/* Structure for items of frgTableHdr table with registred FRG functions */
typedef struct 
{
   const void   *pRedFuncRef;
   int    frgState;
   UINT32 noOfListeners;
} FRG_ITEM;
   
/* Structure for the linked list of queues to be removed */
typedef struct remQueueItem
{
   struct remQueueItem *pNext;
   MD_QUEUE queueId;
} REM_QUEUE_ITEM;

/* Structure for items of the queuetableHdr table with created queues */
typedef struct
{
   MD_QUEUE queueId;
   char     *pName;
} MD_QUEUE_ITEM;

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

void MDCom_send(void);
int extractUri(
   const char *pDestURI,    
   char       *pUriInstName,
   char       *pUriFuncName,
   int        *pUriType);
void MD_finish_addListener(void);
void MD_renew_mc_listeners(void);
void MDCom_showStatistic(void);
int MDCom_clearStatistic(void);
int mdAnyListener(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,
#endif
   UINT32 comid,
   int    uriType,      
   char   *pUriInstName,
   char   *pUriFuncName);
int mdPutMsgOnListenerQueue(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,
#endif
   UINT32 lastRecMsgNo,
   UINT32 dataLength,    
   char   **ppMsgData,
   QUEUE_MSG *pMsg, 
   int    uriType,      
   char   *pUriInstName,
   char   *pUriFuncName,
   int    *pCallBackListener);
void mdPutMsgOnListenerFunc(
#ifdef TARGET_SIMU
   UINT32 simuIpAddr,   
#endif
   UINT32 lastRecMsgNo,
   UINT32 dataLength,   
   char   *pMsgData,    
   MSG_INFO *pMsgInfo,  
   int    uriType,      
   char   *pUriInstName,
   char   *pUriFuncName);
void mdTerminateListener(void);
char *getQueueItemName(MD_QUEUE queueId);
int mdAnyListenerOnQueue(
   MD_QUEUE queue);
    
#ifdef __cplusplus
}
#endif

#endif
