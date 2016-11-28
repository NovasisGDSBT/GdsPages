/*******************************************************************************
 *  COPYRIGHT      : (c) 2006-2012 Bombardier Transportation
 *******************************************************************************
 *  PROJECT        : IPTrain
 *
 *  MODULE         : netdriver.c
 *
 *  ABSTRACT       : Low level Ethernet communication
 *
 *******************************************************************************
 *  HISTORY 
 *	
 * $Id: netdriver.c 33111 2014-06-18 11:48:30Z gweiss $
 *
 *  CR-4093 (Gerhard Weiss, 2012-04-24)
 *           Correction of "Delayed Signal with IPTCom"
 *
 *  CR-3477 (Gerhard Weiss, 2012-04-18)
 *           Findings from TUEV-Assessment
 *
 *  CR-3326 (Bernd Loehr, 2012-02-10)
 *           Improvement for 3rd party use / Darwin port added.
 *
 *  CR-432 (Gerhard Weiss, 2010-11-24)
 *          Added more missing UNUSED Parameter Macros
 *
 *  CR_432 (Bernd Loehr, 2010-08-22) 
 * 			Compiler error on VS 2008 (with new pcap-library)
 *
 *  Internal (Bernd Loehr, 2010-08-16) 
 * 			Old obsolete CVS history removed
 *
 ******************************************************************************/

/*******************************************************************************
* INCLUDES */

#ifdef TARGET_SIMU
#include <stdio.h>
#include "pcap.h"                      /* pcap necessary for RAW sockets on windows */
#include "remote-ext.h"
#endif


#include "vos_socket.h"                /* OS independent socket definitions */

#if defined(WIN32)
#define INT32_ALREADY_DEFINED
#endif

#if defined(VXWORKS)
#include "cfg_api.h"
#include "ps_api.h"
#include "os_api.h"
#endif

#include "iptcom.h"                    /* Common type definitions for IPT  */

#if defined(WIN32)
#undef INT32_ALREADY_DEFINED
#endif

#if defined(LINUX) || defined(DARWIN)
#include <unistd.h> 
#endif

#include "vos.h"                       /* OS independent system calls */
#include "netdriver.h"                 /* Public header file */

#include "iptcom_priv.h"
#include "mdcom_priv.h"
#include "mdses.h"
#include "mdtrp.h"                     /* MD transport defintions */

#include "iptDef.h"        /* IPT definitions */

#ifdef TARGET_SIMU
   #include "sock_simu.h"
#endif

/*******************************************************************************
* DEFINES */

/* Fix for CM (IP_TTL is redefined by Qt) */
#if defined(WIN32)
#undef IP_TTL
#define IP_TTL 4
#endif


/**************** Communication parameters *************************************/

/* Definitions for the UDP ports used */
#define IP_PD_UDP_PORT 20548           /* IP-PD UDP port */
#define IP_MD_UDP_PORT 20550           /* IP-MD UDP port */
#define IP_SNMP_PORT   12030           /* SNMP port */

/* Memory allocation definitions */
#define IPT_DRIVE_PD_MEM_SIZE 0xFFFF
#define IPT_DRIVE_MD_MEM_SIZE 0xFFFF
#define IPT_DRIVE_SNMP_MEM_SIZE 0xFFFF

/*******************************************************************************
* TYPEDEFS */
/* The different states of the network layer */
typedef enum {DLOpen, DLClosed} DL_STATE;


/*******************************************************************************
* GLOBALS */

/*******************************************************************************
* LOCALS */

/* The state of the data link (i.e. Open or Closed) */
static DL_STATE gState;

/* ************************** MD specific parameters ************************ */
static SOCKET gMDSocket;                   /* The MD socket */

/* The main method for the MD receive task */
void THREAD_METHOD IPTDriveMDRecTaskMain(void *pExtThis);

/* ************************** PD specific parameters ************************ */
static SOCKET gPDSocket;                   /* The PD socket */

/* The main method for the PD receive task */
void THREAD_METHOD IPTDrivePDRecTaskMain(void *pExtThis);

/* ************************** SNMP specific parameters ************************ */
static SOCKET gSNMPSocket = 0;            /* The SNMP socket */

/* The main method for the PD receive task */
void THREAD_METHOD IPTDriveSNMPRecTaskMain(void *pExtThis);

#ifdef LINUX_MULTIPROC
/* The main method for the net control task */
void THREAD_METHOD IPTDriveNetCtrlTaskMain(void *pExtThis);
#endif

/*******************************************************************************
* LOCAL FUNCTIONS */

#if defined(VXWORKS)
/******************************************************************************
 * LOCAL FUNCTION DEFINITIONS
 */
static void PD_MD_send(void)
{
   UINT16 timerIndex;
   mon_broadcast_printf("IPTCom: PD_MD_send cycle=%d\n",IPTGLOBAL(task.iptComProcCycle));
   ps_add(IPTGLOBAL(task.iptComProcCycle), 0, AS_WDOG_OFF, &timerIndex);
   
   while (1)
   {
      ps_wait(timerIndex);
      IPTCom_send();
   }
}

static void PD_send(void)
{
   UINT16 timerIndex;
   mon_broadcast_printf("IPTCom: PD_send cycle=%d\n",IPTGLOBAL(task.pdProcCycle));
   ps_add(IPTGLOBAL(task.pdProcCycle), 0, AS_WDOG_OFF, &timerIndex);
   
   while (1)
   {
      ps_wait(timerIndex);
      PDCom_send();
   }
}

static void MD_send(void)
{
   UINT16 timerIndex;
   mon_broadcast_printf("IPTCom: MD_send cycle=%d\n",IPTGLOBAL(task.mdProcCycle));
   ps_add(IPTGLOBAL(task.mdProcCycle), 0, AS_WDOG_OFF, &timerIndex);
   
   while (1)
   {
      ps_wait(timerIndex);
      MDCom_send();
   }
}
#endif

/*******************************************************************************
NAME:       IPTPDReceiveSocketCreate
ABSTRACT:   Create PD receive socket
RETURNS:    IPT_OK if succeded, !=0 if error
*/
static int IPTPDReceiveSocketCreate(void){
   SOCKADDR_IN theSourceAddr;

   /* Create the PD socket. */
   if ((gPDSocket = socket (AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
   {
      IPTVosPrint0(IPT_ERR, "PD receive socket create failed");
      IPTVosPrintSocketError();
      return (int)IPT_ERR_PD_SOCKET_CREATE_FAILED;
   } 
   
#if defined(VXWORKS)
   theSourceAddr.sin_len = (u_char) sizeof(theSourceAddr);
#endif
   
   /* Fill out PD address information, i.e. set IP address and port number */
   theSourceAddr.sin_family = AF_INET;
   
   /* Use the process data port */
   theSourceAddr.sin_port = htons (IP_PD_UDP_PORT);
   
   /* Let DHCP decide the IP address of the interface */
   theSourceAddr.sin_addr.s_addr = htonl (INADDR_ANY);
   
   /* Associate the address above with the gPDSocket. */
   if (bind (gPDSocket, (struct sockaddr FAR *) &theSourceAddr, /*lint !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */
             sizeof (theSourceAddr)) == SOCKET_ERROR)
   {
      IPTVosPrint0(IPT_ERR, "Bind PD receive socket failed"); 
      IPTVosPrintSocketError();
      return (int)IPT_ERR_PD_SOCKET_BIND_FAILED;
   }              

#if defined(IF_WAIT_ENABLE)
   IPTGLOBAL(ifRecReadyPD) = 1;
#endif

   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       IPTMDReceiveSocketCreate
ABSTRACT:   Create MD receive socket and send sockets for ack messages
RETURNS:    IPT_OK if succeded, !=0 if error
*/
static int IPTMDReceiveSocketCreate(void)
{
   SOCKADDR_IN theSourceAddr;
   IPT_CONFIG_COM_PAR_EXT comPar;
   int theIntVal;               /* Integer value used with setsockopt */
   char theCharVal;             /* Char value used with setsockopt
                                  (multicast ttl) */
   
   /* Read default MD com settings from XML file */
   if (iptConfigGetComPar(IPT_DEF_COMPAR_MD_ID, &comPar) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_WARN,
       "Could not read MD QoS and TTL values from Database. Default is used\n");
      comPar.qos = MD_DEF_QOS;
      comPar.ttl = MD_DEF_TTL;
   }
   
   /* Create the MD socket. */
   if ((gMDSocket = socket (AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) 
   {
      IPTVosPrint0(IPT_ERR, "MD Socket create failed" );
      IPTVosPrintSocketError();
      return (int)IPT_ERR_MD_SOCKET_CREATE_FAILED;
   } 
   
#if defined(VXWORKS)
   theSourceAddr.sin_len = (u_char) sizeof(theSourceAddr);  
#endif
   
   /* Fill out MD address information, i.e. set IP address and port number */
   theSourceAddr.sin_family = AF_INET;
   
   /* Use the message data port */
   theSourceAddr.sin_port = htons (IP_MD_UDP_PORT);      
   
   /* Let DHCP decide the IP address of the interface */
#ifndef TS_HAS_FIXED_IP
   theSourceAddr.sin_addr.s_addr = htonl (INADDR_ANY);   
#else
   /* Temporary fix for trainswitch. Currently forced to be on 10.0.0.1 anyway 
      (tgkamp/2008-09-23). Required since TS has two IP addresses on interface
    */
   theSourceAddr.sin_addr.s_addr = htonl (0x0a000001);   
#endif 
   /* Associate the address above with the gMDSocket. */
   if (bind (gMDSocket, (struct sockaddr FAR *) &theSourceAddr, /*lint !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */
             sizeof (theSourceAddr)) == SOCKET_ERROR)
   {
      IPTVosPrint0(IPT_ERR, "Bind MD socket failed"); 
      IPTVosPrintSocketError();
      return (int)IPT_ERR_MD_SOCKET_BIND_FAILED;
   }              
   
   /* Set TTL */
   theIntVal = (int) comPar.ttl;
   if (setsockopt(gMDSocket, IPPROTO_IP, IP_TTL, (char *) &theIntVal, 
                  sizeof (theIntVal)) == SOCKET_ERROR)
   {
      IPTVosPrint1(IPT_ERR, "SetSockOpt IPPROTO_IP, IP_TTL TTL=%d",theIntVal); 
      IPTVosPrintSocketError();
   }

   /* Set Multicast TTL */
   theCharVal = comPar.ttl;
   if (setsockopt(gMDSocket, IPPROTO_IP, IP_MULTICAST_TTL, &theCharVal, 
                  sizeof (theCharVal)) == SOCKET_ERROR)
   {
      IPTVosPrint1(IPT_ERR, "SetSockOpt IPPROTO_IP, IP_MULTICAST_TTL TTL=%d",theCharVal);
      IPTVosPrintSocketError();
   }

   /* Set MD QoS */
   /* The QoS value (0-7) is mapped to MSB bits 7-5, bit 2 is set for local use */
   theIntVal = (int) ((comPar.qos << 5) | 4);
   if (setsockopt(gMDSocket, IPPROTO_IP, IP_TOS, (char *) &theIntVal, 
                  sizeof (theIntVal)) == SOCKET_ERROR)
   {
      IPTVosPrint1(IPT_ERR, "SetSockOpt IPPROTO_IP, IP_TOS QOS=%d",comPar.qos);
      IPTVosPrintSocketError();
   }

#if defined(IF_WAIT_ENABLE)
   IPTGLOBAL(ifRecReadyMD) = 1;
#endif

   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       IPTSNMPSocketCreate
ABSTRACT:   The mainroutine for the SNMP receive task
RETURNS:    IPT_OK if succeded, !=0 if error
*/
static int IPTSNMPSocketCreate(void)
{
   SOCKADDR_IN theSourceAddr;
   
   /* Create the SNMP socket. */
   if ((gSNMPSocket = socket (AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) 
   {
      IPTVosPrint0(IPT_ERR, "SNMP Socket create failed");
      IPTVosPrintSocketError();
      return (int)IPT_ERR_SNMP_SOCKET_CREATE_FAILED;
   } 
   
#if defined(VXWORKS)
   theSourceAddr.sin_len = (u_char) sizeof(theSourceAddr);
#endif
   
   /* Fill out SNMP address information, i.e. set IP address and port number */
   theSourceAddr.sin_family = AF_INET;
   
   /* Use the process data port */
   theSourceAddr.sin_port = htons (IP_SNMP_PORT);
   
   /* Let DHCP decide the IP address of the interface */
   theSourceAddr.sin_addr.s_addr = htonl (INADDR_ANY);
   
   /* Associate the address above with the gPDSocket. */
   if (bind (gSNMPSocket, (struct sockaddr FAR *) &theSourceAddr, /*lint !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */
             sizeof (theSourceAddr)) == SOCKET_ERROR) 
   {
      IPTVosPrint0(IPT_ERR, "Bind SNMP socket failed"); 
      IPTVosPrintSocketError();
      return (int)IPT_ERR_MD_SOCKET_BIND_FAILED;
   }              
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       IPTDriveStart
ABSTRACT:   Start the driver. Creates sockets and threds
RETURNS:    IPT_OK if succeded, !=0 if error
*/
static int IPTDriveStart()
{
#ifdef LINUX_MULTIPROC
   int res;
#endif
#if defined(VXWORKS)
   UINT32 task_id;
#endif
   
   /* Already open ? */
   if (gState == DLOpen)
      return (int)IPT_ERR_DRIVER_ALREADY_STARTED;
   
   gState = DLOpen;
   IPTGLOBAL(systemShutdown) = 0;
  
#if defined(IF_WAIT_ENABLE)
   if (!IPTGLOBAL(ifWaitRec))
#endif
   {
      if (IPTMDReceiveSocketCreate() != IPT_OK)
      {
         return (int)IPT_ERR_MD_SOCKET_CREATE_FAILED;
      }

      if (IPTPDReceiveSocketCreate() != IPT_OK)
      {
         return (int)IPT_ERR_PD_SOCKET_CREATE_FAILED;
      }

      if (IPTSNMPSocketCreate() != IPT_OK)
      {
         return (int)IPT_ERR_SNMP_SOCKET_CREATE_FAILED;
      }
   }

   /******************************** Threads ***********************************/
   /* Start PD receive task */
   (void)IPTVosThreadSpawn("PDRec", PD_REC_POLICY, IPTGLOBAL(task.pdRecPriority), 
                     PD_REC_STACK, &IPTDrivePDRecTaskMain, NULL);
   
   /* Start MD receive task */
   (void)IPTVosThreadSpawn("MDRec", MD_REC_POLICY, IPTGLOBAL(task.mdRecPriority), 
                     MD_REC_STACK, &IPTDriveMDRecTaskMain, NULL);

   /* Start SNMP receive task */
   (void)IPTVosThreadSpawn("SNRec", MD_REC_POLICY, IPTGLOBAL(task.snmpRecPriority), 
                     MD_REC_STACK, &IPTDriveSNMPRecTaskMain, NULL);

#if defined(VXWORKS)
   /* Use common task for sending PD and MD? */
   if (IPTGLOBAL(task.iptComProcCycle) != 0)
   {
      mon_broadcast_printf("IPTCom: IPTComSend priority=%d\n",IPTGLOBAL(task.iptComProcPriority));
      os_t_spawn("IPTCom", AS_TYPE_AP_C, "IPTComSend",
                 IPTGLOBAL(task.iptComProcPriority), 10000,
                 (FUNCPTR)PD_MD_send, 0,
                 0, &task_id);
   }
   else
   {
      mon_broadcast_printf("IPTCom: PDSend priority=%d\n",IPTGLOBAL(task.pdProcPriority));
      os_t_spawn("IPTCom", AS_TYPE_AP_C, "PDSend",
                 IPTGLOBAL(task.pdProcPriority), 10000,
                 (FUNCPTR)PD_send, 0,
                 0, &task_id);
      
      mon_broadcast_printf("IPTCom: MdSend priority=%d\n",IPTGLOBAL(task.mdProcPriority));
      os_t_spawn("IPTCom", AS_TYPE_AP_C, "MDsend",
                 IPTGLOBAL(task.mdProcPriority), 10000,
                 (FUNCPTR)MD_send, 0,
                 0, &task_id);
   }
#else
   /* Use common task for sending PD and MD? */
   if (IPTGLOBAL(task.iptComProcCycle) != 0)
   {
      /* Register IPTCom process in the C scheduler */
      IPTVosRegisterCyclicThread(IPTCom_send,"IPTComSend", 
                                 IPTGLOBAL(task.iptComProcCycle),
                                 IPTCOM_SND_POLICY,
                                 IPTGLOBAL(task.iptComProcPriority),
                                 IPTCOM_SND_STACK); 
   }
   else
   {
      /* Register PD process in the C scheduler */
      IPTVosRegisterCyclicThread(PDCom_send,"PDSend", 
                                 IPTGLOBAL(task.pdProcCycle),
                                 PD_SND_POLICY,
                                 IPTGLOBAL(task.pdProcPriority),
                                 PD_SND_STACK); 

      /* Register MD process in the C scheduler */
      IPTVosRegisterCyclicThread(MDCom_send,"MDSend", 
                                 IPTGLOBAL(task.mdProcCycle),
                                 MD_SND_POLICY,
                                 IPTGLOBAL(task.mdProcPriority),
                                 MD_SND_STACK); 
   }
#endif
   
#ifdef LINUX_MULTIPROC
   /* Create net ctrl task  queue */
   res = IPTVosCreateMsgQueue(&IPTGLOBAL(net.netCtrlQueueId),
                              10,
                              sizeof(NET_CTRL_QUEUE_MSG)); 
   if (res != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_ERR, "ERROR creating  mdSendQueue\n");
      return(res);
   }
   /* Start net ctrl task */
   (void)IPTVosThreadSpawn("NetCtl", NET_CTRL_POLICY, IPTGLOBAL(task.netCtrlPriority), 
                     NET_CTRL_STACK, &IPTDriveNetCtrlTaskMain, NULL);
#endif
      
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       IPTDriveStop
ABSTRACT:   Stop driver. Closes sockets and shutdown threads
RETURNS:    IPT_OK if succeded, !=0 if error
*/
static int IPTDriveStop()
{
#ifdef LINUX_MULTIPROC
   int res;
#endif
   /* Already closed ? */
   if (gState == DLClosed)
      return (int)IPT_ERR_DRIVER_ALREADY_CLOSED;
   
   IPTGLOBAL(systemShutdown) = 1;
   /******************************* MD Socket *********************************/
   
   if (shutdown (gMDSocket, SD_BOTH) != 0)
   {
      IPTVosPrint0(IPT_ERR, "Shutdown of MD socket failed");
      IPTVosPrintSocketError();
   }
   
   if (closesocket (gMDSocket) != 0)
   {
      IPTVosPrint0(IPT_ERR, "closesocket (MD) failed");
      IPTVosPrintSocketError();
   }
   
   /******************************* PD Socket *********************************/
   
   if (shutdown (gPDSocket, SD_BOTH) != 0)
   {
      IPTVosPrint0(IPT_ERR, "Shutdown of PD socket failed");
      IPTVosPrintSocketError();
   }
   
   if(closesocket (gPDSocket) != 0)
   {
      IPTVosPrint0(IPT_ERR, "closesocket (MD) failed");
      IPTVosPrintSocketError();
   }
   
   /******************************* SNMP Socket *********************************/
   
   if(shutdown (gSNMPSocket, SD_BOTH) != 0)
   {
      IPTVosPrint0(IPT_ERR, "Shutdown of SNMP socket failed");
      IPTVosPrintSocketError();
   }
   
   if(closesocket (gSNMPSocket) != 0)
   {
      IPTVosPrint0(IPT_ERR, "closesocket (SNMP) failed");
      IPTVosPrintSocketError();
   }

   gState = DLClosed;

#ifdef LINUX_MULTIPROC
   /* Destroy net ctrl task  queue */
   res   =  IPTVosDestroyMsgQueue(&IPTGLOBAL(net.netCtrlQueueId)); 
   if (res  != (int)IPT_OK)
   {
      IPTVosPrint1(IPT_ERR, "ERROR destroying  netCtrlQueueId error=%#x\n",res);
   }
#endif
      
   return (int)IPT_OK;
}

/*******************************************************************************
* GLOBAL FUNCTIONS */

/*******************************************************************************
NAME:       IPTMDSendSocketCreate
ABSTRACT:   Creates sockets
RETURNS:    IPT_OK if succeded, !=0 if error
*/
int IPTMDSendSocketCreate(
   IPT_CONFIG_COM_PAR_EXT *pComPar)
{
   int ret = IPT_OK;
   int theIntVal;    /* Integer value used with setsockopt */
   char theCharVal;  /* Char value used with setsockopt (multicast ttl) */

   /******************************** MD Socket ********************************/
   
   /*  CR-3477, findings from TUEV-Assessment - GW, 2012-04-11 */
   if (pComPar != NULL)
   {
      /* Create the MD socket. */
      pComPar->mdSendSocket = socket (AF_INET, SOCK_DGRAM, 0);
      if (pComPar->mdSendSocket == (int)INVALID_SOCKET)
      { 
         IPTVosPrint1(IPT_ERR, "MD Socket create failed for compar ID=%d",pComPar->comParId );
         IPTVosPrintSocketError();
         pComPar->mdSendSocket =0;
         ret = IPT_ERR_MD_SOCKET_CREATE_FAILED;
      }
      else
      {
         /* Set TTL */
         theIntVal = (int) pComPar->ttl;
         if (setsockopt((SOCKET)pComPar->mdSendSocket, IPPROTO_IP, IP_TTL, 
                        (char *) &theIntVal, sizeof (theIntVal)) == SOCKET_ERROR)
         {
            IPTVosPrint1(IPT_ERR, "SetSockOpt IPPROTO_IP, IP_TTL TTL=%d",theIntVal);
            IPTVosPrintSocketError();
         }

         /* Set Multicast TTL */
         theCharVal = pComPar->ttl;
         if (setsockopt((SOCKET)pComPar->mdSendSocket, IPPROTO_IP, IP_MULTICAST_TTL,
                        &theCharVal, sizeof (theCharVal)) == SOCKET_ERROR)
         {
            IPTVosPrint1(IPT_ERR, "SetSockOpt IPPROTO_IP, IP_MULTICAST_TTL TTL=%d",theCharVal);
            IPTVosPrintSocketError();
         }

         /* Set MD QoS */
         /* The QoS value (0-7) is mapped to MSB bits 7-5, bit 2 is set for local use */
         theIntVal = (int) ((pComPar->qos << 5) | 4);
         if (setsockopt((SOCKET)pComPar->mdSendSocket, IPPROTO_IP, IP_TOS, 
                        (char *) &theIntVal, sizeof (theIntVal)) == SOCKET_ERROR)
         {
            IPTVosPrint1(IPT_ERR, "SetSockOpt IPPROTO_IP, IP_TOS QOS=%d",pComPar->qos);
            IPTVosPrintSocketError();
         }
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "MD Socket create failed; pComPar == NULL");
      ret = IPT_ERR_MD_SOCKET_CREATE_FAILED;
   }
   
  return(ret);
}

/*******************************************************************************
NAME:       IPTPDSendSocketCreate
ABSTRACT:   Creates sockets
RETURNS:    IPT_OK if succeded, !=0 if error
*/
int IPTPDSendSocketCreate(
   IPT_CONFIG_COM_PAR_EXT *pComPar)
{
   int ret = IPT_OK;
   int theIntVal;    /* Integer value used with setsockopt */
   char theCharVal;  /* Char value used with setsockopt (multicast ttl) */

   /******************************** PD Socket *********************************/
   
   /*  CR-3477, findings from TUEV-Assessment - GW, 2012-04-11 */
   if (pComPar != NULL)
   {
      /* Create the PD socket. */
      pComPar->pdSendSocket = socket (AF_INET, SOCK_DGRAM, 0);
      if (pComPar->pdSendSocket == (int)INVALID_SOCKET)
      {
         IPTVosPrint1(IPT_ERR, "PD Socket create failed for compar ID=%d",pComPar->comParId );
         IPTVosPrintSocketError();
         pComPar->pdSendSocket =0;
         ret = IPT_ERR_PD_SOCKET_CREATE_FAILED;
      } 
      else
      {
         /* Set TTL */
         theIntVal = (int) pComPar->ttl;
         if (setsockopt((SOCKET)pComPar->pdSendSocket, IPPROTO_IP, IP_TTL,
                        (char *) &theIntVal, sizeof (theIntVal)) == SOCKET_ERROR)
         {
            IPTVosPrint1(IPT_ERR, "SetSockOpt IPPROTO_IP, IP_TTL TTL=%d",theIntVal);
            IPTVosPrintSocketError();
         }
         
         /* Set Multicast TTL */
         theCharVal = pComPar->ttl;
         if (setsockopt((SOCKET)pComPar->pdSendSocket, IPPROTO_IP, IP_MULTICAST_TTL,
                        &theCharVal, sizeof (theCharVal)) == SOCKET_ERROR)
         {
            IPTVosPrint1(IPT_ERR, "SetSockOpt IPPROTO_IP, IP_MULTICAST_TTL TTL=%d",theCharVal);
            IPTVosPrintSocketError();
         }
      
         /* Set PD QoS */
         /* The QoS value (0-7) is mapped to MSB bits 7-5, bit 2 is set for local use */
         theIntVal = (int) ((pComPar->qos << 5) | 4);
         if (setsockopt((SOCKET)pComPar->pdSendSocket, IPPROTO_IP, IP_TOS,
                        (char *) &theIntVal, sizeof (theIntVal)) == SOCKET_ERROR)
         {
            IPTVosPrint1(IPT_ERR, "SetSockOpt IPPROTO_IP, IP_TOS QOS=%d",pComPar->qos);
            IPTVosPrintSocketError();
         }
      }
   }
   else
   {
      IPTVosPrint0(IPT_ERR, "PD Socket create failed; pComPar == NULL");
      ret = IPT_ERR_PD_SOCKET_CREATE_FAILED;
   }

   return(ret);
}

/*******************************************************************************
NAME:       IPTDriveJoinPDMultiCast
ABSTRACT:   Joins a PD multicast group.
RETURNS:    IPT_OK if succeded, !=0 if error
*/
int IPTDriveJoinPDMultiCast(
   unsigned long multiCastAddr  /* Adress of the multicast group */
#ifdef TARGET_SIMU
   ,unsigned long simdevAddr    /* Adress of the simulated device */
#endif   
)
{

#ifdef TARGET_SIMU
   IPTCom_SimJoinMulticastgroup(simdevAddr, multiCastAddr, 1, 0);
#else
   struct ip_mreq ipMreq;     /* Argument structure to setsockopt */
   
   /* Fill in the argument structure to join the multicast group */   
   ipMreq.imr_multiaddr.s_addr = htonl(multiCastAddr);
   
   /* Unicast interface addr from which to receive the multicast packets */
   ipMreq.imr_interface.s_addr = htonl (INADDR_ANY);/* inet_addr (ifAddr);  */
   
   /* Set the socket option to join the MULTICAST group */
   if (setsockopt(gPDSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,  
                  (char *)&ipMreq,  sizeof (ipMreq)) == SOCKET_ERROR) 
   {  
      IPTVosPrint1(IPT_ERR, "PD SetSockOpt IPPROTO_IP, IP_ADD_MEMBERSHIP IP Address=%#x",multiCastAddr);
      IPTVosPrintSocketError();
      return (int)IPT_ERR_PD_SOCKOPT_FAILED;
   } 
  
#endif
  
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       IPTDriveLeavePDMultiCast
ABSTRACT:   Leaves a PD multicast group.
RETURNS:    IPT_OK if succeded, !=0 if error
*/
int IPTDriveLeavePDMultiCast(
   unsigned long multiCastAddr  /* Adress of the multicast group */
#ifdef TARGET_SIMU
   ,unsigned long simdevAddr    /* Adress of the simulated device */
#endif   
)
{

#ifdef TARGET_SIMU
   IPTCom_SimLeaveMulticastgroup(simdevAddr, multiCastAddr, 1 , 0);
#else
   struct ip_mreq ipMreq;     /* Argument structure to setsockopt */
   
   /* Which multicast group to leave */
   ipMreq.imr_multiaddr.s_addr = htonl(multiCastAddr);    /* inet_addr (mcastAddr); */
   
   /* Unicast interface addr from which to receive the multicast packets */
   ipMreq.imr_interface.s_addr = htonl (INADDR_ANY);/* inet_addr (ifAddr);  */
   
   /* Set the socket option to leave the MULTICAST group */
   if (setsockopt(gPDSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP,  
                  (char *)&ipMreq,  sizeof (ipMreq)) == SOCKET_ERROR) 
   {  
      IPTVosPrint1(IPT_ERR, "PD SetSockOpt IPPROTO_IP, IP_DROP_MEMBERSHIP IP Address=%#x", multiCastAddr);
      IPTVosPrintSocketError();
      return (int)IPT_ERR_PD_SOCKOPT_FAILED;
   } 
   
#endif
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       IPTDriveJoinMDMultiCast
ABSTRACT:   Joins a MD multicast group.
RETURNS:    IPT_OK if succeded, !=0 if error
*/
int IPTDriveJoinMDMultiCast(
   unsigned long multiCastAddr  /* Adress of the multicast group */
#ifdef TARGET_SIMU
   ,unsigned long simdevAddr    /* Adress of the simulated device */
#endif   
   )
{
#ifdef TARGET_SIMU
   IPTCom_SimJoinMulticastgroup(simdevAddr, multiCastAddr, 0, 1);
#else
   struct ip_mreq ipMreq;     /* Argument structure to setsockopt */

   /* Fill in the argument structure to join the multicast group */
   ipMreq.imr_multiaddr.s_addr = htonl(multiCastAddr);
   
   /* Unicast interface addr from which to receive the multicast packets */
   ipMreq.imr_interface.s_addr = htonl (INADDR_ANY);/* inet_addr (ifAddr);  */
   
   /* Set the socket option to join the MULTICAST group */
   if (setsockopt(gMDSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,  
                  (char *)&ipMreq,  sizeof (ipMreq)) == SOCKET_ERROR) 
   {  
      IPTVosPrint1(IPT_ERR, "MD SetSockOpt IPPROTO_IP, IP_ADD_MEMBERSHIP IP Address=%#x",multiCastAddr);
      IPTVosPrintSocketError();
      return (int)IPT_ERR_MD_SOCKOPT_FAILED;
   } 
#endif   

   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       IPTDriveLeaveMDMultiCast
ABSTRACT:   Leaves a MD multicast group.
RETURNS:    IPT_OK if succeded, !=0 if error
*/
int IPTDriveLeaveMDMultiCast(
   unsigned long multiCastAddr  /* Adress of the multicast group */
#ifdef TARGET_SIMU
   ,unsigned long simdevAddr    /* Adress of the simulated device */
#endif   
)
{
#ifdef TARGET_SIMU
   IPTCom_SimLeaveMulticastgroup(simdevAddr, multiCastAddr, 0, 1);
#else
   struct ip_mreq ipMreq;     /* Argument structure to setsockopt */

  /* Which multicast group to leave */
   ipMreq.imr_multiaddr.s_addr = htonl(multiCastAddr); /*inet_addr (mcastAddr);  */
   
   /* Unicast interface addr from which to receive the multicast packets */
   ipMreq.imr_interface.s_addr = htonl (INADDR_ANY);/* inet_addr (ifAddr);  */
   
   /* Set the socket option to leave the MULTICAST group */
   if (setsockopt(gMDSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP,  
                  (char *)&ipMreq,  sizeof (ipMreq)) == SOCKET_ERROR) 
   {  
      IPTVosPrint1(IPT_ERR, "MD SetSockOpt IPPROTO_IP, IP_DROP_MEMBERSHIP IP Address=%#x",multiCastAddr);
      IPTVosPrintSocketError();
      return (int)IPT_ERR_MD_SOCKOPT_FAILED;
   } 
#endif   
   
   return (int)IPT_OK;
}

/*******************************************************************************
NAME:       IPTDriveMDSend
ABSTRACT:   The send routine for MD data
RETURNS:    IPT_OK if succeded, !=0 if error
*/
#ifdef TARGET_SIMU      
int IPTDriveMDSend(
   UINT32 destIPAddr,    /* The IP address of the destination */
   UINT32 sourceIPAddr,  /* The IP address of the simulated source */
   BYTE *pFrame,         /* Pointer to message to send */
   int frameLen)         /* Size of the message to send */
{
   SOCKADDR_IN theDestination;         /* Socket address of the destination */
   SOCKADDR_IN theSource;              /* Socket address of the source (simulated) */
      
   theDestination.sin_family = AF_INET;  
   theDestination.sin_addr.s_addr = destIPAddr;
   theDestination.sin_port = htons(IP_MD_UDP_PORT); 
   theSource.sin_family = AF_INET;
   theSource.sin_addr.s_addr = sourceIPAddr;
   theSource.sin_port = htons(IP_MD_UDP_PORT);
      
   /* Send simulated IPT-PD telegram with specified source address */
   if (IPTCom_SimSendUDP(pFrame, frameLen, (struct sockaddr *) &theSource, 
                     (struct sockaddr *) &theDestination) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_WARN, "IPTSimSendUDP(): Failed\n");   
      return (int)IPT_ERR_MD_SENDTO_FAILED;
   }
   
   IPTGLOBAL(md.mdCnt.mdOutPackets)++;
   
   return (int)IPT_OK;
}
#else
int IPTDriveMDSend(
   UINT32 destIPAddr,  /* The IP address of the destination */
   BYTE *pFrame,       /* Pointer to message to send */
   int frameLen)       /* Size of the message to send */
{
   SOCKADDR_IN theDestination;         /* Socket address of the destination */
   
   
   /* Update the socket address */
#if defined(VXWORKS)
   theDestination.sin_len = (u_char) sizeof(SOCKADDR_IN);  
#endif
   
   theDestination.sin_family = AF_INET;  
   theDestination.sin_addr.s_addr = destIPAddr;
   theDestination.sin_port = htons(IP_MD_UDP_PORT); 
   
   
   /* Send data to socket */
   if (sendto(gMDSocket, (char *)pFrame, frameLen, 0, 
      (struct sockaddr *)&theDestination, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) /*lint !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */
   {
      IPTVosPrint4(IPT_ERR, "MD to IP %d.%d.%d.%d sendto", 
                             (destIPAddr & 0xff000000) >> 24,
                             (destIPAddr & 0xff0000) >> 16,
                             (destIPAddr & 0xff00) >> 8,
                             (destIPAddr & 0xff));
      IPTVosPrintSocketError();
      
      return (int)IPT_ERR_MD_SENDTO_FAILED;
   }
   
   IPTGLOBAL(md.mdCnt.mdOutPackets)++;
   
   return (int)IPT_OK;
}
#endif

/*******************************************************************************
NAME:       IPTDriveSNMPSend
ABSTRACT:   The send routine for SNMP data
RETURNS:    IPT_OK if succeded, !=0 if error
*/
#ifdef TARGET_SIMU      
void IPTDriveSNMPSend(
   UINT32 destIPAddr,    /* Destination IP address... */
   UINT16 destPort,      /* ...and port number */
   UINT32 sourceIPAddr,  /* The IP address of the simulated source */
   BYTE *pFrame,         /* Pointer to message to send */
   int frameLen)         /* Size of the message to send */
{
   
   SOCKADDR_IN theDestination;         /* Socket address of the destination */
   SOCKADDR_IN theSource;              /* Socket address of the source (simulated) */
      
   theDestination.sin_family = AF_INET;  
   theDestination.sin_addr.s_addr = destIPAddr;
   theDestination.sin_port = destPort; 
   theSource.sin_family = AF_INET;
   theSource.sin_addr.s_addr = sourceIPAddr;
   theSource.sin_port = htons(IP_SNMP_PORT);
      
   /* Send simulated IPT-PD telegram with specified source address */
   if (IPTCom_SimSendUDP(pFrame, frameLen, (struct sockaddr *) &theSource, 
                     (struct sockaddr *) &theDestination) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_WARN, "IPTSimSendUDP(): Failed\n");   
   }
}

#else
void IPTDriveSNMPSend(
   UINT32 destIPAddr,   /* Destination IP address... */
   UINT16 destPort,     /* ...and port number */
   BYTE *pFrame,        /* Pointer to message to send */
   int frameLen)        /* Size of the message to send */
{
   SOCKADDR_IN theDestination;         /* Socket address of the destination */
      
   /* Update the socket address */
#if defined(VXWORKS)
   theDestination.sin_len = (u_char) sizeof(SOCKADDR_IN);  
#endif
   
   theDestination.sin_family = AF_INET;  
   theDestination.sin_addr.s_addr = destIPAddr;
   theDestination.sin_port = destPort; 
   
   if (gSNMPSocket)
   {
      /* Send data to socket */
      if (sendto(gSNMPSocket, (char *)pFrame, frameLen, 0, 
         (struct sockaddr *)&theDestination, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) /*lint !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */
      {
         IPTVosPrint4(IPT_ERR, "SNMP to IP %d.%d.%d.%d sendto", 
                                (destIPAddr & 0xff000000) >> 24,
                                (destIPAddr & 0xff0000) >> 16,
                                (destIPAddr & 0xff00) >> 8,
                                (destIPAddr & 0xff));
         IPTVosPrintSocketError();
      }
   }
}
#endif

/*******************************************************************************
NAME:       IPTDriveMDSocketSend
ABSTRACT:   The send routine for MD data
RETURNS:    IPT_OK if succeded, !=0 if error
*/
#ifdef TARGET_SIMU      
int IPTDriveMDSocketSend(
   UINT32 destIPAddr,  /* The IP address of the destination */
   UINT32 sourceIPAddr,  /* The IP address of the simulated source */
   BYTE *pFrame,       /* Pointer to message to send */
   int frameLen,       /* Size of the message to send */
   int sendSocket)
{
   
   SOCKADDR_IN theDestination;         /* Socket address of the destination */
   SOCKADDR_IN theSource;              /* Socket address of the source (simulated) */
 
   IPT_UNUSED (sendSocket)

   theDestination.sin_family = AF_INET;  
   theDestination.sin_addr.s_addr = destIPAddr;
   theDestination.sin_port = htons(IP_MD_UDP_PORT); 
   theSource.sin_family = AF_INET;
   theSource.sin_addr.s_addr = sourceIPAddr;
   theSource.sin_port = htons(IP_MD_UDP_PORT);
      
   /* Send simulated IPT-PD telegram with specified source address */
   if (IPTCom_SimSendUDP(pFrame, frameLen, (struct sockaddr *) &theSource, 
                     (struct sockaddr *) &theDestination) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_WARN, "IPTSimSendUDP(): Failed\n");   
      return (int)IPT_ERR_MD_SENDTO_FAILED;
   }
   
   IPTGLOBAL(md.mdCnt.mdOutPackets)++;
   
   return (int)IPT_OK;
}

#else
int IPTDriveMDSocketSend(
   UINT32 destIPAddr,  /* The IP address of the destination */
   BYTE *pFrame,       /* Pointer to message to send */
   int frameLen,       /* Size of the message to send */
   int sendSocket)
{
   SOCKADDR_IN theDestination;         /* Socket address of the destination */
   
   /* Update the socket address */
#if defined(VXWORKS)
   theDestination.sin_len = (u_char) sizeof(SOCKADDR_IN);  
#endif
   
   theDestination.sin_family = AF_INET;  
   theDestination.sin_addr.s_addr = destIPAddr;
   theDestination.sin_port = htons(IP_MD_UDP_PORT); 
   
   
   /* Send data to socket */ 
   if (sendto((SOCKET)sendSocket, (char *)pFrame, frameLen, 0, 
      (struct sockaddr *)&theDestination, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) /*lint !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */
   {
      IPTVosPrint4(IPT_ERR, "MD to IP %d.%d.%d.%d sendto", 
                             (destIPAddr & 0xff000000) >> 24,
                             (destIPAddr & 0xff0000) >> 16,
                             (destIPAddr & 0xff00) >> 8,
                             (destIPAddr & 0xff));
      IPTVosPrintSocketError();
      
      return (int)IPT_ERR_MD_SENDTO_FAILED;
   }
   
   IPTGLOBAL(md.mdCnt.mdOutPackets)++;
   
   return (int)IPT_OK;
}
#endif

/*******************************************************************************
NAME:       IPTDrivePDSocketSend
ABSTRACT:   The send routine for PD data
RETURNS:    IPT_OK if succeded, !=0 if error
*/
#ifdef TARGET_SIMU      
void IPTDrivePDSocketSend(
   unsigned long  destIPAddr,  /* The IP address of the destination */
   unsigned long  sourceIPAddr,  /* The IP address of the simulated source */
   BYTE *pFrame,       /* Pointer to message to send */
   int frameLen,       /* Size of the message to send */
   int sendSocket)
{
   
   SOCKADDR_IN theDestination;         /* Socket address of the destination */
   SOCKADDR_IN theSource;              /* Socket address of the source (simulated) */

   IPT_UNUSED (sendSocket)
   
   theDestination.sin_family = AF_INET;  
   theDestination.sin_addr.s_addr = htonl(destIPAddr);
   theDestination.sin_port = htons(IP_PD_UDP_PORT); 
   theSource.sin_family = AF_INET;
   theSource.sin_addr.s_addr = htonl(sourceIPAddr);
   theSource.sin_port = htons(IP_PD_UDP_PORT);
      
   /* Send simulated IPT-PD telegram with specified source address */
   if (IPTCom_SimSendUDP(pFrame, frameLen, (struct sockaddr *) &theSource, 
                     (struct sockaddr *) &theDestination) != (int)IPT_OK)
   {
      IPTVosPrint0(IPT_WARN, "IPTSimSendUDP(): Failed\n");   
   }
   
   IPTGLOBAL(pd.pdCnt.pdOutPackets)++;
}

#else
void IPTDrivePDSocketSend(
   unsigned long  destIPAddr,  /* The IP address of the destination */
   BYTE *pFrame,       /* Pointer to message to send */
   int frameLen,       /* Size of the message to send */
   int sendSocket)
{
   SOCKADDR_IN theDestination;         /* Socket address of the destination */
   
   
   /* Update the socket address */
#if defined(VXWORKS)
   theDestination.sin_len = (u_char) sizeof(SOCKADDR_IN);  
#endif
   
   theDestination.sin_family = AF_INET;  
   theDestination.sin_addr.s_addr = htonl(destIPAddr);
   theDestination.sin_port = htons(IP_PD_UDP_PORT); 
   
   
   /* Send data to socket */ 
   if (sendto((SOCKET)sendSocket, (char *)pFrame, frameLen, 0, 
      (struct sockaddr *)&theDestination, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) /*lint !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */
   {
      IPTVosPrint4(IPT_ERR, "PD to IP %d.%d.%d.%d sendto", 
                             (destIPAddr & 0xff000000) >> 24,
                             (destIPAddr & 0xff0000) >> 16,
                             (destIPAddr & 0xff00) >> 8,
                             (destIPAddr & 0xff));
      IPTVosPrintSocketError();
      
   }
 
   IPTGLOBAL(pd.pdCnt.pdOutPackets)++;
   
}
#endif

/*******************************************************************************
NAME:       IPTDrivePDRecTaskMain
ABSTRACT:   The mainroutine for the PD receive task
RETURNS:    IPT_OK if succeded, !=0 if error
*/
void IPTDrivePDRecTaskMain(
   void *pArgs)   /* Pointer to startup arguments */
{
   int noOfBytesReturned;         /* The number of bytes read from the socket */
#ifndef TARGET_SIMU
   SOCKADDR_IN theSourceAddr;     /* Address of the sender */
 #if defined(LINUX) || defined(DARWIN)
   socklen_t theSourceAddrSize;   /* Size of theSourceAddr struct */
 #else
   int theSourceAddrSize;         /* Size of theSourceAddr struct */
 #endif
#else
   UINT32 srcAddr, destAddr;
   UINT16 srcPort;
#endif
#if defined(VXWORKS)
   IP_STATUS_T res;
#endif

   char *pRecBuf = (char *)NULL; /* Pointer to the PD message buffer */

   IPT_UNUSED (pArgs)

#if defined(IF_WAIT_ENABLE)
   if (IPTGLOBAL(ifWaitRec))
   {
#if defined(VXWORKS)
      MON_PRINTF("IPTCom: PD receive task waiting for ethernet interface\n");
      if ((res = ip_status_get(NULL, IPT_WAIT_FOREVER)) != IP_RUNNING)
      {
         IPTVosPrint2(IPT_ERR, "PD receive thread exit. ip_status_get Failed CSS return code = %d %#x\n", res, res);
         return;
      }
      MON_PRINTF("IPTCom: PD receive task ethernet interface ready\n");
#endif   
      if (IPTPDReceiveSocketCreate() != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "PD receive thread exit. PD Socket create failed");
         return;
      }
   }
#endif  
   
   /* Allocate memory */
   pRecBuf = (char *)IPTVosMalloc(IPT_DRIVE_PD_MEM_SIZE);
   if (pRecBuf == NULL)
   {
      IPTVosPrint0(IPT_ERR, "Out of memory. PD rec task exits\n");
      return;
   }

   /* Do this until communication is closed */
   while (!IPTGLOBAL(systemShutdown))
   {
#ifdef TARGET_SIMU
      /* Parse IP package and call PDCom_receive */
      if (IPTCom_SimProcessUDPData(pRecBuf, IP_PD_UDP_PORT, &srcAddr, &srcPort, &destAddr, &noOfBytesReturned) != (int)IPT_OK)
      {
         IPTVosTaskDelay(1000);
      }
      else
      {
         if (noOfBytesReturned > 0)
         {
            /* This is IP-PD data. Copy it to its destination 
               Convert IP addresses to local format */
            PDCom_receive(srcAddr, destAddr, pRecBuf, noOfBytesReturned);
         }
      }
#else
      theSourceAddrSize = sizeof(SOCKADDR_IN);

      /* Read data from socket  */
      noOfBytesReturned = recvfrom(gPDSocket, pRecBuf,
                                   IPT_DRIVE_PD_MEM_SIZE, 0, 
                                   (struct sockaddr *)&theSourceAddr, 
                                   &theSourceAddrSize);/*lint !e64 !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */

      if (noOfBytesReturned > 0)
      {
         /* This is IP-PD data. Copy it to its destination */
         PDCom_receive(theSourceAddr.sin_addr.s_addr, (unsigned char *)pRecBuf, 
                       noOfBytesReturned);
      }
#endif
   }
   /* Return memory to heap */
   IPTVosFree ((unsigned char *)pRecBuf);         

   IPTVosRawPrint0(IPT_NETDRIVER, "PDRecTask exits\n");
}

/*******************************************************************************
NAME:       IPTDriveMDRecTaskMain
ABSTRACT:   The mainroutine for the MD receive task
RETURNS:    IPT_OK if succeded, !=0 if error
*/
void IPTDriveMDRecTaskMain(
   void *pArgs)   /* Pointer to startup arguments */
{
   int noOfBytesReturned;         /* The number of bytes read from the socket */
   char *pRecBuf = (char *)NULL;  /* Pointer to the buffer with the MD message */

#ifdef TARGET_SIMU
   UINT32 srcAddr;
   UINT32 destAddr;
   UINT16 srcPort;
#else
   SOCKADDR_IN theSourceAddr;     /* Address of the sender */
 #if defined(LINUX) || defined(DARWIN)
   socklen_t theSourceAddrSize;   /* Size of theSourceAddr struct */
 #else
   int theSourceAddrSize;         /* Size of theSourceAddr struct */
 #endif
#endif
#if defined(VXWORKS)
   IP_STATUS_T res;
#endif

   IPT_UNUSED (pArgs)

#if defined(IF_WAIT_ENABLE)
   if (IPTGLOBAL(ifWaitRec))
   {
#if defined(VXWORKS)
      MON_PRINTF("IPTCom: MD receive task waiting for ethernet interface\n");
      if ((res = ip_status_get(NULL, IPT_WAIT_FOREVER)) != IP_RUNNING)
      {
         IPTVosPrint2(IPT_ERR, "MD receive thread exit. ip_status_get Failed CSS return code = %d %#x\n", res, res);
         return;
      }
      MON_PRINTF("IPTCom: MD receive task ethernet interface ready\n");
#endif 
      if (IPTMDReceiveSocketCreate() != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "MD receive thread exit. MD Socket create failed");
         return;
      }
   }
#endif 
   
   /* Allocate memory */
   pRecBuf = (char *)IPTVosMalloc(IPT_DRIVE_MD_MEM_SIZE);
   if (pRecBuf == NULL)
   {
      IPTVosPrint0(IPT_ERR, "Out of memory. MD receive thread exit\n");
      return;
   }
   
   /* Do this until communication is closed */
   while (!IPTGLOBAL(systemShutdown))
   {
#ifdef TARGET_SIMU

      /* Parse IP package and call PDCom_receive */
      if (IPTCom_SimProcessUDPData(pRecBuf, IP_MD_UDP_PORT, &srcAddr, &srcPort, &destAddr, &noOfBytesReturned) != (int)IPT_OK)
      {
         IPTVosTaskDelay(1000);
      }
      else
      {
         if (noOfBytesReturned > 0)
         {
            /* Yes. Copy it to its destination */
            trReceive(srcAddr, destAddr, pRecBuf, noOfBytesReturned);
         }
      }
#else    

      theSourceAddrSize = sizeof(SOCKADDR_IN);

      /* Read data from socket  */
      noOfBytesReturned = recvfrom(gMDSocket, pRecBuf,
                                   IPT_DRIVE_MD_MEM_SIZE, 0, 
                                   (struct sockaddr *)&theSourceAddr, 
                                   &theSourceAddrSize);/*lint !e64 !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */
      
      if (noOfBytesReturned > 0)
      {
         /* Yes. Copy it to its destination */
         trReceive( theSourceAddr.sin_addr.s_addr, pRecBuf, noOfBytesReturned);
      }
#endif
   }

   /* Return memory to heap */
   IPTVosFree ((unsigned char *)pRecBuf);

   IPTVosRawPrint0(IPT_NETDRIVER, "MDRecTask exits\n");
}

/*******************************************************************************
NAME:       IPTDriveSNMPRecTaskMain
ABSTRACT:   The mainroutine for the SNMP receive task
RETURNS:    IPT_OK if succeded, !=0 if error
*/
void IPTDriveSNMPRecTaskMain(
   void *pArgs)   /* Pointer to startup arguments */
{
#ifdef TARGET_SIMU
   UINT32 srcAddr;
   UINT32 destAddr;
   UINT16 srcPort;
#endif
   int noOfBytesReturned;         /* The number of bytes read from the socket */
   char *pRecBuf = (char *)NULL; /* Pointer to the buffer with the SNMP message */
   SOCKADDR_IN theSourceAddr;     /* Address of the sender */
 #if defined(LINUX) || defined(DARWIN)
   socklen_t theSourceAddrSize;   /* Size of theSourceAddr struct */
#else
   int theSourceAddrSize;         /* Size of theSourceAddr struct */
#endif
#if defined(VXWORKS)
   IP_STATUS_T res;
#endif

   IPT_UNUSED (pArgs)

#if defined(IF_WAIT_ENABLE)
   if (IPTGLOBAL(ifWaitRec))
   {
#if defined(VXWORKS)
      MON_PRINTF("IPTCom: SNMP receive task waiting for ethernet interface\n");
      if ((res = ip_status_get(NULL, IPT_WAIT_FOREVER)) != IP_RUNNING)
      {
         IPTVosPrint2(IPT_ERR, "SNMP receive thread exit. ip_status_get Failed CSS return code = %d %#x\n", res, res);
         return;
      }
      MON_PRINTF("IPTCom: SNMP receive task ethernet interface ready\n");
#endif  
      if (IPTSNMPSocketCreate() != IPT_OK)
      {
         IPTVosPrint0(IPT_ERR, "SNMP receive thread exit. SNMP Socket create failed");
         return;
      }
   }
#endif  

   /* Allocate memory */
   pRecBuf = (char *)IPTVosMalloc(IPT_DRIVE_SNMP_MEM_SIZE);
   if (pRecBuf == NULL)
   {
      IPTVosPrint0(IPT_ERR, "Out of memory. SNMP rec task exits\n");
      return;
   }
   
   /* Do this until communication is closed */
   while (!IPTGLOBAL(systemShutdown))
   {
      theSourceAddrSize = sizeof(SOCKADDR_IN);

#ifdef TARGET_SIMU
      /* Parse IP package and call PDCom_receive */
      noOfBytesReturned = 0;
       while ((noOfBytesReturned == 0) && (!IPTGLOBAL(systemShutdown)))
      {
         if (IPTCom_SimProcessUDPData(pRecBuf, IP_SNMP_PORT, &srcAddr, &srcPort, &destAddr, &noOfBytesReturned) != (int)IPT_OK)
         {
           IPTVosTaskDelay(1000);
         }
      }
      theSourceAddr.sin_addr.s_addr = srcAddr;
      theSourceAddr.sin_port = srcPort;
#else
      /* Read data from socket  */
      noOfBytesReturned = recvfrom(gSNMPSocket, pRecBuf,
                                   IPT_DRIVE_SNMP_MEM_SIZE, 0, 
                                   (struct sockaddr *) &theSourceAddr, 
                                   &theSourceAddrSize);/*lint !e64 !e826 Conversion between struct sockaddr and struct sockaddr_in allowed */
#endif      

      if (noOfBytesReturned > 0)
      {
         /* Yes. Copy it to its destination */
         iptSnmpInMessage(theSourceAddr.sin_addr.s_addr, 
            theSourceAddr.sin_port, 
#ifdef TARGET_SIMU
            destAddr,
#endif
            (unsigned char *)pRecBuf, noOfBytesReturned);
      }
   }

   /* Return memory to heap */
   IPTVosFree ((const unsigned char *)pRecBuf);

   IPTVosRawPrint0(IPT_NETDRIVER, "SNMPRecTask exits\n");
}

#ifdef LINUX_MULTIPROC
/*******************************************************************************
NAME:       IPTDriveNetCtrlTaskMain
ABSTRACT:   The mainroutine for the MD receive task
RETURNS:    IPT_OK if succeded, !=0 if error
*/
void IPTDriveNetCtrlTaskMain(
   void *pArgs)   /* Pointer to startup arguments */
{
   NET_CTRL_QUEUE_MSG newMsg;

   IPT_UNUSED (pArgs)

   /* Do this until communication is closed */
   while (!IPTGLOBAL(systemShutdown))
   {
      /* get new message from queue */
      while (IPTVosReceiveMsgQueue(&IPTGLOBAL(net.netCtrlQueueId),
                                   (char*)&newMsg,
                                   sizeof(NET_CTRL_QUEUE_MSG),
                                   IPT_WAIT_FOREVER) > 0)
      {
         switch (newMsg.ctrl)
         {
            case JOIN_MD_MULTICAST:
#ifdef TARGET_SIMU
               (void)IPTDriveJoinMDMultiCast(newMsg.multiCastAddr, newMsg.simuIpAddr);
#else
               (void)IPTDriveJoinMDMultiCast(newMsg.multiCastAddr);
#endif
               break;

            case JOIN_PD_MULTICAST:
#ifdef TARGET_SIMU
               (void)IPTDriveJoinPDMultiCast(newMsg.multiCastAddr, newMsg.simuIpAddr);
#else
               (void)IPTDriveJoinPDMultiCast(newMsg.multiCastAddr);
#endif
               break;

            case LEAVE_MD_MULTICAST:
#ifdef TARGET_SIMU
               (void)IPTDriveLeaveMDMultiCast(newMsg.multiCastAddr, newMsg.simuIpAddr);
#else
               (void)IPTDriveLeaveMDMultiCast(newMsg.multiCastAddr);
#endif
               break;

            case LEAVE_PD_MULTICAST:
#ifdef TARGET_SIMU
               (void)IPTDriveLeavePDMultiCast(newMsg.multiCastAddr, newMsg.simuIpAddr);
#else
               (void)IPTDriveLeavePDMultiCast(newMsg.multiCastAddr);
#endif
               break;

            default:
               
               break;

         }
      }
   }
  

   IPTVosRawPrint0(IPT_NETDRIVER, "NetCtrlTask exits\n");
}
#endif

/*******************************************************************************
NAME:       IPTDrivePrepareInit
ABSTRACT:   Start driver
RETURNS:    0 if OK, !=0 if not
*/
int IPTDrivePrepareInit(void)
{
   int ret = 0;  /* Return code */
   
   gState = DLClosed;
   
   ret = IPTDriveStart();
   if (ret != (int)IPT_OK)
   {
      IPTVosPrint1(IPT_ERR, "Driver could not be started error=%#x", ret);
      IPTDriveDestroy();
      return ret; 
   }
   
#ifdef TARGET_SIMU
   if ((ret = iptTab2Init(&IPTGLOBAL(net.pdJoinedMcAddrTable), sizeof(NET_JOINED_MC))) != (int)IPT_OK)
#else
   if ((ret = iptTabInit(&IPTGLOBAL(net.pdJoinedMcAddrTable), sizeof(NET_JOINED_MC))) != (int)IPT_OK)
#endif
      return ret;

#ifdef TARGET_SIMU
   if ((ret = iptTab2Init(&IPTGLOBAL(net.mdJoinedMcAddrTable), sizeof(NET_JOINED_MC))) != (int)IPT_OK)
#else
   if ((ret = iptTabInit(&IPTGLOBAL(net.mdJoinedMcAddrTable), sizeof(NET_JOINED_MC))) != (int)IPT_OK)
#endif
      return ret;


   return 0;
}

/*******************************************************************************
NAME:       IPTDriveTerminate
ABSTRACT:   Exit driver
RETURNS:    -
*/
void IPTDriveDestroy(void) 
{ 
   int retCode = 0;  /* Return code */
   
   retCode = IPTDriveStop();
   if (retCode != (int)IPT_OK)
   {
      IPTVosPrint1(IPT_ERR, "Driver could not be stopped error=%#x", retCode);
   }   
}

