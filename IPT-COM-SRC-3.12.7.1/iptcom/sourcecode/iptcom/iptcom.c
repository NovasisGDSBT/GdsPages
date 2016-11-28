/*******************************************************************************
 *    COPYRIGHT   : (C) 2006-2015 Bombardier Transportation
 *******************************************************************************
 *    PROJECT     :  IPTrain
 *
 *    MODULE      :  iptcom.c
 *
 *    ABSTRACT    :  Public methods for IPT communication classes:
 *
 *******************************************************************************
 *    HISTORY     :
 *	
 *    $Id: iptcom.c 36150 2015-03-24 09:45:11Z gweiss $
 *
 *    CR-10076 Gerhard Weiss, 205-03-24
 *          Correction to support 8 byte alignment
 *
 *    CR-7241  (Bernd Loehr, 2014-01-27)
 *             Wait a second in IPTCom_destroy() to allow threads terminating in
 *             Simulation Mode.
 *
 *    CR-480   (Gerhard Weiss, 2011-09-07)
 *             Windows Support for OSBuild
 *             (null pXMLPath if file not found in init)
 *
 *    CR-432   (Bernd Loehr, 2010-10-07)
 *             Elimination of compiler warnings
 *             (include stdlib.h instead of malloc.h)
 *
 *    CR-62    (Bernd Loehr, 2010-08-25)
 *             Additional function handle_event() to renew all MC memberships
 *             after link down/up event
 *
 *    CR-685   (Gerhard Weiss, 2010-08-24)
 *             Add interlock mechanism during termination, ensuring no Memory is
 *             freed before all threads are terminated.
 *
 *    Internal (Bernd Loehr, 2010-08-13)
 *             Old obsolete CVS history removed
 *
 ******************************************************************************/

/*******************************************************************************
*  INCLUDES */
#if defined(VXWORKS)
 #include <vxWorks.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "iptcom.h"        /* Common type definitions for IPT */
#include "vos.h"           /* OS independent system calls */
#include "netdriver.h"
#include "iptDef.h"        /* IPT definitions */
#include "iptcom_priv.h"


/*******************************************************************************
*  DEFINES
*/

/*******************************************************************************
*  TYPEDEFS
*/

/*******************************************************************************
*  GLOBAL DATA
*/

/* Global data for complete IPTCom */
#ifndef LINUX_MULTIPROC
IPT_GLOBAL iptGlobal;  /* The variable iptComInitiated is set to zero by default */
#endif

#ifndef LINUX_CLIENT
int iptEnableTDCSimulation = 0;   /* <> 0 if TDC should be simulated */

/* Search path for the host file for tdc simulation */
char theHostFilePath[IPT_SIZE_OF_FILEPATH+1];
#endif

/*******************************************************************************
*  LOCAL DATA
*/

/*******************************************************************************
*  LOCAL FUNCTION DECLARATIONS
*/

/*******************************************************************************
*  LOCAL FUNCTIONS
*/

/*******************************************************************************
*  GLOBAL FUNCTIONS
*/

/*******************************************************************************
NAME:       IPTCom_prepareInit
ABSTRACT:   Initialize the IPTCom and IPTDir components.
RETURNS:    0 if OK, != if not
*/
int IPTCom_prepareInit(
   int startupMode,      /* Startup mode according to CM */
   const char *pXMLPath) /* path to configuration XML file */
{
#ifndef LINUX_CLIENT
   int retCode = (int)IPT_OK;  /* Return code */
   UINT32 memSize = 0;
   char buildTime[30];           /* Build date and time */

#ifdef LINUX_MULTIPROC
   unsigned int shmSize;
#endif
   
#if defined(LINUX) || defined(DARWIN)
   char linuxFile[LINUX_FILE_SIZE] = {0};
#endif

   UINT32 ifWait = 0;

   IPT_UNUSED(startupMode)

   sprintf(buildTime, "%s, %s" ,__DATE__, __TIME__);
   MON_PRINTF ("\nIPTCom - Ver. %d.%d.%d.%d Buildtime: %s\n",
          IPT_VERSION, IPT_RELEASE, IPT_UPDATE, IPT_EVOLUTION, buildTime);

   /* Initiate memory size */
   if (pXMLPath == NULL)
      memSize = IPTCOM_MEMORY_DEFAULT;
   else
   {
#if defined(LINUX) || defined(DARWIN)
      retCode = iptConfigParseXMLInit(pXMLPath, &memSize, &ifWait, linuxFile);
#else
      retCode = iptConfigParseXMLInit(pXMLPath, &memSize, &ifWait);
#endif
      if (retCode != (int)IPT_OK && retCode != (int)IPT_NOT_FOUND)
         return retCode;

      if (retCode == (int)IPT_NOT_FOUND) 
         pXMLPath = NULL;
      
      if (memSize == 0)
         memSize = IPTCOM_MEMORY_DEFAULT;
   }

#ifndef IF_WAIT_ENABLE
   ifWait = 0;
#endif

   /* Initiate system */
#if defined(LINUX) || defined(DARWIN)
   retCode = IPTVosSystemStartup(linuxFile);
#else
   retCode = IPTVosSystemStartup();
#endif
   if (retCode != (int)IPT_OK)
     return retCode;

   /* Allocate complete memory area */
#ifdef LINUX_MULTIPROC
   /* If Linux and multi-processing: create shared memory for both globals and memory and align them*/
   IPTVosPrint0(IPT_ERR, "\nAlignment check\n");
   IPTVosPrint1(IPT_ERR, "Alignof IPT_GLOBAL = %lu\n", ALIGNOF(IPT_GLOBAL) );

   shmSize = sizeof(IPT_GLOBAL);
   
   IPTVosPrint1(IPT_ERR, "sizeof(IPT_GLOBAL) = %lu\n", shmSize );
   
   shmSize += (ALIGNOF(UINT64ST) - 1);
   shmSize &= ~(ALIGNOF(UINT64ST) - 1);
   
   IPTVosPrint1(IPT_ERR, "sizeof(IPT_GLOBAL) aligned to %lu\n", shmSize );

   pIptGlobal = (IPT_GLOBAL *)((void *) IPTVosCreateSharedMemory(shmSize + memSize));
 
   IPTVosPrint1(IPT_ERR, "Memory address of pIptGlobal = 0x%08X\n", (void*)pIptGlobal ); 
 
   if (pIptGlobal == NULL)
      return (int)IPT_MEM_ERROR;

   /* Set pointer to memory area after the aligned globals */
   IPTGLOBAL(mem.pArea) = (BYTE *) pIptGlobal + shmSize;

   IPTVosPrint1(IPT_ERR, "IPTGLOBAL(mem.pArea) = 0x%08X\n", (void*)IPTGLOBAL(mem.pArea) );
 
#else
   IPTGLOBAL(mem.pArea) = (BYTE *) malloc(memSize);
   if (IPTGLOBAL(mem.pArea) == NULL)
      return (int)IPT_MEM_ERROR;     /* We could not get enough memory */

#endif

   /* Start time */
   IPTGLOBAL(iptcomStarttime) = IPTVosGetMilliSecTimer();
   strncpy(IPTGLOBAL(buildTime), buildTime, sizeof(IPTGLOBAL(buildTime)));
   /* Initialize memory */
   IPTGLOBAL(mem.memSize) = memSize;
   IPTGLOBAL(mem.allocSize) = 0;
   IPTGLOBAL(mem.pFreeArea) = IPTGLOBAL(mem.pArea);
   IPTGLOBAL(mem.noOfBlocks) = 0;
   IPTGLOBAL(mem.memCnt.freeSize) = memSize;
   IPTGLOBAL(mem.memCnt.minFreeSize) = memSize;
   IPTGLOBAL(mem.memCnt.allocCnt) = 0;
   IPTGLOBAL(mem.memCnt.allocErrCnt) = 0;
   IPTGLOBAL(mem.memCnt.freeErrCnt) = 0;

   /* Do basic initializing of globals */
   IPTGLOBAL(vos.queueCnt.queueAllocated) = 0;
   IPTGLOBAL(vos.queueCnt.queueMax) = 0;
   IPTGLOBAL(vos.queueCnt.queuCreateErrCnt) = 0;
   IPTGLOBAL(vos.queueCnt.queuDestroyErrCnt) = 0;
   IPTGLOBAL(vos.queueCnt.queuWriteErrCnt) = 0;
   IPTGLOBAL(vos.queueCnt.queuReadErrCnt) = 0;

   IPTGLOBAL(disableMarshalling) = 0;

   IPTGLOBAL(enableFrameSizeCheck) = 0;

   IPTGLOBAL(systemShutdown) = 0;
 
#ifdef IF_WAIT_ENABLE
   IPTGLOBAL(ifWaitRec) = ifWait;
   IPTGLOBAL(ifWaitSend) = ifWait;
   IPTGLOBAL(ifRecReadyPD) = 0;
   IPTGLOBAL(ifRecReadyMD) = 0;
#endif

   IPTGLOBAL(tdcsim.tdcSimNoOfHosts) = 0;
   IPTGLOBAL(tdcsim.uriBufCurPos) = (char *)NULL;

   IPTGLOBAL(configDB.exchgParTable.initialized) = FALSE;
   IPTGLOBAL(configDB.pdSrcFilterParTable.initialized) = FALSE;
   IPTGLOBAL(configDB.destIdParTable.initialized) = FALSE;
   IPTGLOBAL(configDB.datasetTable.initialized) = FALSE;
   IPTGLOBAL(configDB.comParTable.initialized) = FALSE;
   IPTGLOBAL(configDB.fileTable.initialized) = FALSE;
   IPTGLOBAL(configDB.finish_addr_resolv) = 0;
#ifdef IF_WAIT_ENABLE
   IPTGLOBAL(configDB.finish_socket_creation) = 0;
#endif   
   IPTGLOBAL(net.pdJoinedMcAddrTable.initialized) = FALSE;
   IPTGLOBAL(net.mdJoinedMcAddrTable.initialized) = FALSE;

   IPTGLOBAL(task.pdRecPriority) = PD_REC_PRIO;
   IPTGLOBAL(task.mdRecPriority) = MD_REC_PRIO;
   IPTGLOBAL(task.snmpRecPriority) = SNMP_REC_PRIO;
   IPTGLOBAL(task.pdProcCycle) = PD_SND_CYCLE;
   IPTGLOBAL(task.pdProcPriority) = PD_SND_PRIO;
   IPTGLOBAL(task.mdProcCycle) = MD_SND_CYCLE;
   IPTGLOBAL(task.mdProcPriority) = MD_SND_PRIO;
   IPTGLOBAL(task.iptComProcCycle) = 0;
   IPTGLOBAL(task.iptComProcPriority) = IPTCOM_SND_PRIO;
#ifdef LINUX_MULTIPROC
   IPTGLOBAL(task.netCtrlPriority) = NET_CTRL_PRIO;
#endif                     
   IPTGLOBAL(pd.defInvalidBehaviour) = PD_REC_VAL_BEH_DEF;
   IPTGLOBAL(pd.defTimeout) = PD_REC_TIMEOUT_DEF;
   IPTGLOBAL(pd.defCycle) = PD_CYCL_TIME_DEF;
   IPTGLOBAL(pd.finishRecAddrResolv) = 0;
   IPTGLOBAL(pd.finishSendAddrResolv) = 0;

   IPTGLOBAL(pd.pdCnt.pdStatisticsStarttime) = IPTGLOBAL(iptcomStarttime);

   IPTGLOBAL(md.defAckTimeOut) = ACK_TIME_OUT_VAL_DEF;
   IPTGLOBAL(md.defResponseTimeOut) = REPLY_TIME_OUT_VAL_DEF;
   IPTGLOBAL(md.maxStoredSeqNo) = MAX_NO_OF_ACTIVE_SEQ_NO_DEF;
   IPTGLOBAL(md.finish_addr_resolv) = 0;

   IPTGLOBAL(md.mdCnt.mdStatisticsStarttime) = IPTGLOBAL(iptcomStarttime);

   /* Create memory allocation semaphore */
   if (IPTVosCreateSem(&IPTGLOBAL(mem.sem), IPT_SEM_FULL) != (int)IPT_OK)
      return (int)IPT_SEM_ERR;  /* Could not create sempahore */
               
   /* Get enable TDC from global. Can have been set by IPTCom_enableIPTDirEmulation
   before arriving here, other functions should use pIptGlobal->enableTDCSimulation */
   IPTGLOBAL(tdcsim.enableTDCSimulation) = iptEnableTDCSimulation;
   strncpy(IPTGLOBAL(tdcsim.hostFilePath), theHostFilePath, IPT_SIZE_OF_FILEPATH);

   /* Initiate device parameters */
   if (pXMLPath != NULL)
   {
      retCode = iptConfigParseXML4DevConfig(pXMLPath);
      if (retCode != (int)IPT_OK)
        return retCode;
   }

   /* Initialize memory */
   retCode = IPTVosMemInit();
   if (retCode != (int)IPT_OK)
     return retCode;

   /* Initiate the Configuration Data base */
   retCode = iptConfigInit();
   if (retCode != (int)IPT_OK)
     return retCode;

   /* Initiate PD */
   retCode = PDCom_prepareInit();
   if (retCode != (int)IPT_OK)
     return retCode;

   /* Initiate MD */
   retCode = MDCom_prepareInit();
   if (retCode != (int)IPT_OK)
     return retCode;

   /* Initiate netdriver */
   retCode = IPTDrivePrepareInit();
   if (retCode != (int)IPT_OK)
     return retCode;

   /* Initiate statistics */
   retCode = iptStatInit();
   if (retCode != (int)IPT_OK)
     return retCode;

   if (IPTGLOBAL(tdcsim.enableTDCSimulation))
   {
      /* Initiate TDC simulation data */
      retCode = tdcSimInit (IPTGLOBAL(tdcsim.hostFilePath));
   }
   else
   {
      /* Initiate IPTDir client */
      retCode = tdcPrepareInit (0, 0);
      if (retCode != (int)IPT_OK)
         return retCode;
   }

   if (pXMLPath != NULL)
   {
      /* Ignore return code is done of compabilty reason */
      (void)iptAddConfig(pXMLPath);

   }

   IPTGLOBAL(iptComInitiated) = 1;

   return retCode;
#else
   IPT_UNUSED(startupMode)
   IPT_UNUSED(pXMLPath)

   return(IPT_ERROR);
#endif
}

/*******************************************************************************
NAME:       IPTCom_terminate
ABSTRACT:
RETURNS:    -
*/
void IPTCom_terminate(int remainingTime)
{
    IPT_UNUSED (remainingTime)

    IPTVosThreadDone();
}

/*******************************************************************************
NAME:       IPTCom_process
ABSTRACT:   Only here for compability reasons
RETURNS:    -
*/
void IPTCom_process(void)
{
   /* Do nothing */
}

/*******************************************************************************
NAME:       IPTCom_send
ABSTRACT:   The mainroutine for the IPTCom send task
RETURNS:    -
*/
void IPTCom_send(void)
{
   /* Cycle MD */
   MDCom_send();

   /* Cycle PD */
   PDCom_send();
}

/*******************************************************************************
NAME:       IPTCom_enableIPTDirEmulation
ABSTRACT:   Enable emulation of the IPTDir client (tdc).
            Needs to be called before the call to IPTCom_prepareInit().
RETURNS:    -
*/
void IPTCom_enableIPTDirEmulation(
   const char *pHostPath)  /* Search path to tdc simulation host file */
{
#ifndef LINUX_CLIENT
   /* Set global intermediate flag so that prepareInit start emulator */
   iptEnableTDCSimulation = 1;

   if (pHostPath == NULL)
   {
      strncpy(theHostFilePath, IPT_TDC_SIM_HOST_FILENAME, sizeof(IPT_TDC_SIM_HOST_FILENAME));
   }
   else
   {
      strncpy(theHostFilePath, pHostPath, IPT_SIZE_OF_FILEPATH);
      theHostFilePath[IPT_SIZE_OF_FILEPATH] = 0;
   }
#else
   IPT_UNUSED(pHostPath)
#endif
}

/*******************************************************************************
NAME:       IPTCom_isIPTDirEmulated
ABSTRACT:   Check if emulation of the IPTDir client (tdc) is enabled.
RETURNS:    Returns a non-zero value if IPTDir is emulated.
*/
int IPTCom_isIPTDirEmulated(void)
{
    return (IPTGLOBAL(tdcsim.enableTDCSimulation));
}

/*******************************************************************************
NAME:       IPTCom_destroy
ABSTRACT:   Releases memory and stops threads
RETURNS:    -
*/
void IPTCom_destroy(void)
{
#ifndef LINUX_CLIENT
#ifdef LINUX_MULTIPROC
   if (pIptGlobal != NULL)
#endif
   {
      if (IPTGLOBAL(tdcsim.enableTDCSimulation))
      {
         /* Destroy TDC simulation data */
         tdcSimDestroy();
         iptEnableTDCSimulation = 0;
      }
      else
      {
         /* Destroy IPTDir client */
         (void) tdcDestroy();
      }

      /* Destroy PD */
      PDCom_destroy();

      /* Destroy MD */
      MDCom_destroy();

      /* Destroy netdriver */
      IPTDriveDestroy();

      IPTVosTaskDelay(1000);

      /* Destroy Configuration DB */
      iptConfigDestroy();

      /* Destroy memory */
      (void) IPTVosMemDestroy();

   #ifdef LINUX_MULTIPROC
      /* If Linux and multi-processing: destroy shared memory */
      IPTVosDestroySharedMemory((char *) pIptGlobal);

   #else
      /* Return memory area */
      free(IPTGLOBAL(mem.pArea));

   #endif
   }

   /* Shutdown system */
   IPTVosSystemShutdown();
#endif
}

/*******************************************************************************
NAME:       IPTCom_setTopoCnt
ABSTRACT:   Setting the local copy of the current topo counter value.
            Called from the TDC
RETURNS:    0 if OK, != if not
*/
int IPTCom_setTopoCnt(UINT32 topoCnt)
{
   IPTGLOBAL(topoCnt) = topoCnt;
   return(IPT_OK);   
}

/*******************************************************************************
NAME:       IPTCom_handleEvent
ABSTRACT:   Handle the event as defined through the selector
            Currently, if selector == 1, link up event will be handled
RETURNS:    0 if OK, != if not
*/
int IPTCom_handleEvent(UINT32 selector, void* pArg)
{
	int ret = IPT_OK;
	pArg = pArg;

	switch (selector)
	{
   		case IPTCOM_EVENT_LINK_UP:
        	ret = PD_sub_renew_all();
            MD_renew_mc_listeners();
        	break;
   		case IPTCOM_EVENT_NONE:
        default:
        	break;
	}
	return ret;   
}
