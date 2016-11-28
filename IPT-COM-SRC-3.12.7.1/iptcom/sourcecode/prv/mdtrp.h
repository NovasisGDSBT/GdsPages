/*******************************************************************************
*  COPYRIGHT      : (c) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT        : IPTrain
*
*  MODULE         : mdtrp.h
*
*  ABSTRACT       : Message data communication transport layer definitions
*
********************************************************************************
* HISTORY         :
*	
* $Id: mdtrp.h 11620 2010-08-16 13:06:37Z bloehr $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*
*******************************************************************************/
#ifndef _TRANSPORT_H
#define _TRANSPORT_H

/*******************************************************************************
* INCLUDES */
   
#ifdef __cplusplus
extern "C" {
#endif
   
/*******************************************************************************
* DEFINES */
   
#define UNICAST_OR_FRG_NO_REPORT_TRANSPORT_TYPE 1
#define UNICAST_OR_FRG_REPORT_TRANSPORT_TYPE    2
#define UNICAST_OR_FRG_REQUEST_TRANSPORT_TYPE   3
#define MULTICAST_TRANSPORT_TYPE                4
   
#define NOT_SYNC      0
#define WAIT_FOR_SYNC 1
#define SYNC          2
   
/*******************************************************************************
* TYPEDEFS */
   
/*******************************************************************************
* GLOBALS */
   
/*******************************************************************************
* LOCALS */
   
/*******************************************************************************
* LOCAL FUNCTIONS */
   
/*******************************************************************************
* GLOBAL FUNCTIONS */
   
int trInit(void);

void trTerminate(void);

int createTrInstance(
   int type,
   const void *pCallerRef,
   MD_QUEUE callerQueueId,
   IPT_REC_FUNCPTR callerFunc,
   UINT32 comId,
   UINT32 sendMsgLength,
   char *pSendMsg,
   MD_COM_PAR comPar,
   TRANSPORT_INSTANCE **ppNewInstance);

int createSeTrInstance(
   int type,
   const void *pCallerRef,
   SESSION_INSTANCE *pSeInstance,
   UINT32 comId,
   UINT32 sendMsgLength,
   char *pSendMsg,
   MD_COM_PAR comPar,
   TRANSPORT_INSTANCE **ppNewInstance);

int searchTrQueue(MD_QUEUE queue);
void removeTrQueue(MD_QUEUE queue);

void insertTrInstance(TRANSPORT_INSTANCE *pTrInstance);

void trSendTask( void );

#ifdef TARGET_SIMU
void trReceive(
   UINT32 srcIpAddr,  
   UINT32 simDevIpAddr,  
   char *pInBuf,      
   UINT32 inBufBufLen);
#else
void trReceive(
   UINT32 srcIpAddr,  
   char *pInBuf,      
   UINT32 inBufBufLen);
#endif

void showSequenceCounterLists(void);

#ifdef __cplusplus
}
#endif
#endif

