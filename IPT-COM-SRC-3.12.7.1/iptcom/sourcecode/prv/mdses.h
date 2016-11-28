/*******************************************************************************
*  COPYRIGHT      : (c) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT        : IPTrain
*
*  MODULE         : mdses.h
*
*  ABSTRACT       : Message data communication session layer definitions
*
********************************************************************************
* HISTORY         :
*	
* $Id: mdses.h 11620 2010-08-16 13:06:37Z bloehr $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*
*******************************************************************************/

#ifndef _SESSION_H
#define _SESSION_H

/*******************************************************************************
* INCLUDES */


#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
*  DEFINES
*/
   
#define UNICAST_SESSION_TYPE    1
#define MULTICAST_SESSION_TYPE  2

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

int seInit(void);

void seTerminate(void);
 
int createSeInstance(
        int type,
        int expectedNoOfReplies,
        char *pSendMsg,
        const void *pCallerRef,
        MD_QUEUE callerQueueId,
        IPT_REC_FUNCPTR callerFunc,
        UINT32 comId,
        MD_COM_PAR comPar,
        SESSION_INSTANCE **ppNewInstance);

int searchSeQueue(MD_QUEUE queue);
int removeSeQueue(MD_QUEUE queue);

void insertSeInstance(SESSION_INSTANCE *pSeInstance);

SESSION_INSTANCE* searchSession(UINT32 sessionId);

void seSendTask( void );
 
#ifdef __cplusplus
}
#endif

#endif

