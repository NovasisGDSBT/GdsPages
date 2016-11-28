/*******************************************************************************
*  COPYRIGHT      : (c) 2006 Bombardier Transportation
********************************************************************************
*  PROJECT        : IPTrain
*
*  MODULE         : netdriver.h
*
*  ABSTRACT       : Low level Ethernet communication
*
********************************************************************************
*  HISTORY 
*	
* $Id: netdriver.h 33111 2014-06-18 11:48:30Z gweiss $
*
*  Internal (Bernd Loehr, 2010-08-16) 
* 			Old obsolete CVS history removed
*
*
*******************************************************************************/
#ifndef NETDRIVER_H
#define NETDRIVER_H

/*******************************************************************************
* INCLUDES */
/*
#include "iptcom_priv.h"
*/

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* DEFINES */

#define JOIN_MD_MULTICAST  1
#define JOIN_PD_MULTICAST  2
#define LEAVE_MD_MULTICAST 3
#define LEAVE_PD_MULTICAST 4

#if defined(VXWORKS)
#define IF_WAIT_ENABLE
#endif
/*******************************************************************************
* TYPEDEFS */

typedef struct
{
   UINT32 multiCastAddr;                  /* Joined multicast IP address */
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;                     /* Simualted device IP address */
#endif
   UINT32 noOfJoiners;                    /* No of joiners */
} NET_JOINED_MC;

typedef  struct
{
   long type;  /* Only used for Linux */
   int ctrl;
   UINT32 multiCastAddr;
#ifdef TARGET_SIMU
   UINT32 simuIpAddr;                     /* Simualted device IP address */
#endif
} NET_CTRL_QUEUE_MSG;

/* Config communication parameters extended with socket ID's */
typedef struct
{
   UINT32 comParId;      /* Communication parameter ID */
   UINT8  qos;           /* quality of service */
   UINT8  ttl;           /* time to live */
   int    pdSendSocket;  /* socket used for PD sending. Set by IPTCom */
   int    mdSendSocket;  /* socket used for MD sending. Set by IPTCom */
} IPT_CONFIG_COM_PAR_EXT;

/*******************************************************************************
* GLOBALS */

/*******************************************************************************
* LOCALS */

/*******************************************************************************
* LOCAL FUNCTIONS */

/*******************************************************************************
* GLOBAL FUNCTIONS */

/************************** Normal operation ***********************************/
/* Init network driver. Called once at system startup */
extern int IPTDrivePrepareInit(void);

/* Exit network driver. Called once at system shutdown */
extern void IPTDriveDestroy(void);

extern int IPTPDSendSocketCreate(IPT_CONFIG_COM_PAR_EXT *pComPar);
extern int IPTMDSendSocketCreate(IPT_CONFIG_COM_PAR_EXT *pComPar);

#ifndef LINUX_MULTIPROC
/************************** Multicast PD ***************************************/
/* Must be called once for all multicast groups used by PD */
#ifdef TARGET_SIMU
extern int IPTDriveJoinPDMultiCast(unsigned long MultiCastAddr, unsigned long simdevAddr);
#else
extern int IPTDriveJoinPDMultiCast(unsigned long MultiCastAddr);
#endif   

/* Must be called when PD leaves a multicast group */
#ifdef TARGET_SIMU
extern int IPTDriveLeavePDMultiCast(unsigned long MultiCastAddr, unsigned long simdevAddr);
#else
extern int IPTDriveLeavePDMultiCast(unsigned long MultiCastAddr);
#endif   
/************************** Multicast MD ***************************************/
/* Must be called once for all multicast groups used by MD */
#ifdef TARGET_SIMU
extern int IPTDriveJoinMDMultiCast(unsigned long MultiCastAddr, unsigned long simdevAddr);
#else
extern int IPTDriveJoinMDMultiCast(unsigned long MultiCastAddr);
#endif   

/* Must be called when MD leaves a multicast group */
#ifdef TARGET_SIMU
extern int IPTDriveLeaveMDMultiCast(unsigned long MultiCastAddr, unsigned long simdevAddr);
#else
extern int IPTDriveLeaveMDMultiCast(unsigned long MultiCastAddr);
#endif   

#endif


/************************** Send PD message *************************************/
/* Send routine for PD data */
#ifdef TARGET_SIMU      
extern void IPTDrivePDSocketSend(unsigned long  destIPAddr, unsigned long  sourceIPAddr, BYTE *pFrame, int frameLen, int sendSocket);
#else
extern void IPTDrivePDSocketSend(unsigned long  destIPAddr, BYTE *pFrame, int frameLen, int sendSocket);
#endif

/************************** Send MD message *************************************/
/* Send routine for MD data */
#ifdef TARGET_SIMU      
extern int IPTDriveMDSend(UINT32 destIPAddr, UINT32 sourceIPAddr, unsigned char *pFrame, int frameLen);
extern int IPTDriveMDSocketSend(UINT32 destIPAddr, UINT32 sourceIPAddr, BYTE *pFrame, int frameLen, int sendSocket);
#else
extern int IPTDriveMDSend(UINT32 destIPAddr, unsigned char *pFrame, int frameLen);
extern int IPTDriveMDSocketSend(UINT32 destIPAddr, BYTE *pFrame, int frameLen, int sendSocket);
#endif
/************************** Send SNMP message *************************************/
/* Send routine for SNMP data */
#ifdef TARGET_SIMU      
extern void IPTDriveSNMPSend(UINT32 destIPAddr, UINT16 destPort, UINT32 sourceIPAddr, BYTE *pFrame, int frameLen);
#else
extern void IPTDriveSNMPSend(UINT32 destIPAddr, UINT16 destPort, BYTE *pFrame, int frameLen);
#endif

#ifdef __cplusplus
}
#endif

#endif
