/*
 *  pdSender.c
 *  iptcom-tdc
 *
 *  Created by Bernd Löhr on 07.03.13.
 *  Copyright 2013 NewTec GmbH. All rights reserved.
 *
 */


/***********************************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
/* #include <sys/time.h> */
#include <sys/select.h>

#include "ipt.h"        /* Common type definitions for IPT */

#include "tdcSyl.h"
#include "tdcApi.h"

#include "vos.h"        /* OS independent system calls  */
#include "iptcom.h"

/* Some sample comId definitions    */

/* Expect receiving AND Send as echo    */
#define PD_COMID1               2000
#define PD_COMID1_CYCLE         1000
#define PD_COMID1_TIMEOUT       15000
#define PD_COMID1_DATA_SIZE     sizeof(DATASET_ID1_T)

#define APP_VERSION         "0.02"

#define GBUFFER_SIZE        128

typedef struct datasetid1 {
    UINT16	DS1_a;
    INT16	DS1_b;
    INT8	DS1_c;
    char	DS1_d[10];
} __attribute__ ((__packed__)) DATASET_ID1_T;

DATASET_ID1_T	gDataSet1 = {1,2,3,"Hi World"};

BYTE   gBuffer[GBUFFER_SIZE]       = "Hello World";
BYTE   gInputBuffer[GBUFFER_SIZE]  = "";

int	gKeepOnRunning = 1;

/*******************************************************************************
NAME:       terminationHandler
*/

static void terminationHandler(int signal_number)
{
   gKeepOnRunning = 0;
}

/******************************************************************************/
/** Add config for the packets we want to receive or send.
 *
 *  IPTCom should zero out the packet if we lost connection - this allows us
 *  to create a default packet to send to iServ...
 *
 *  @pre            IPTCom must be initialized.
 *  @post           IPTCom recognizes supplied data format.
 *
 *  @param[in]      comid           The desired comID.
 *  @param[in]      dataset_id      The desired data set ID.
 *  @param[in]      size            The size of the packet.
 *  @param[in]      cycletime       The periodicy in milli seconds.
 *  @retval         IPT_OK:         no error
 *  @retval         or error code from iptcom.h
 */
static int config_pd_md(int comid,
                        int dataset_id,
                        int size,
                        int cycletime,
                        int comParId)
{
    IPT_CONFIG_EXCHG_PAR exchg;
    IPT_DATASET_FORMAT   pFormat[3];
    IPT_CONFIG_DATASET   dataset;
    //IPT_CONFIG_COM_PAR 	 comPar = { IPT_DEF_COMPAR_PD_ID, 6, 32 };
    int rv;
    
    exchg.comId     				 = comid;
    exchg.datasetId 				 = dataset_id;
    exchg.comParId  				 = 1;
    exchg.mdRecPar.pSourceURI        = "";
    exchg.mdSendPar.pDestinationURI  = "";
    exchg.pdRecPar.pSourceURI        = "";
    exchg.pdRecPar.validityBehaviour = IPT_INVALID_KEEP;
    exchg.pdRecPar.timeoutValue      = cycletime * 2;
    exchg.pdSendPar.pDestinationURI  = "";
    exchg.pdSendPar.cycleTime        = cycletime;
    exchg.pdSendPar.redundant        = 0;
    
    
    if ((rv = iptConfigAddExchgPar(&exchg)) < 0)
    {
        printf("Error iptConfigAddExchgPar: %d\n", rv);
        return rv;
    }
    
    if (dataset_id)
    {
        dataset.datasetId = dataset_id;
        dataset.nLines    = 1;
        dataset.size      = 0;    /* Size and alignment get actually */
        dataset.alignment = 0;    /* Calculated by iptConfigAddDataset() */
        
        pFormat[0].id   = IPT_INT8;
        pFormat[0].size = size;
        
        if ((rv = iptConfigAddDataset(&dataset, pFormat)) < 0)
        {
            printf("Error iptConfigAddDataset: %d\n",
                   rv);
            return rv;
        }
    }
    //iptConfigAddComPar(&comPar);
    return 0;
}


/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool sends PD messages to an ED.\n"
           "Arguments are:\n"
           "-o own IP address\n"
           "-t target IP address\n"
           "-s sending comID\n"
           "-p path to TDC host simulation file\n"
           "-x path to xml config file\n"
           "-v print version and quit\n"
           );
}

/**********************************************************************************************************************/
/** main entry
 *
 *  @retval         0        no error
 *  @retval         1        some error
 */
int main (int argc, char * *argv)
{
    PD_HANDLE   snd_cst_pd = 0;   /* Publish handle for packets   */
    PD_HANDLE   rcv_cst_pd = 0;   /* Receive handle for packets   */
    
    int     rv = 0;
    int     ip[4];
    UINT32  destIP = 0;
    int     ch;
    UINT32  hugeCounter = 0;
    UINT32  ownIP       = 0;
    UINT32  comId_Out = PD_COMID1;
    char 	hostsFile[IPT_SIZE_OF_FILEPATH] = "";
    char 	configFile[IPT_SIZE_OF_FILEPATH] = "";
    struct sigaction sa;
    
    /****** Parsing the command line arguments */
    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }
    
    while ((ch = getopt(argc, argv, "t:o:s:p:x:h?v")) != -1)
    {
        switch (ch)
        {
        	case 'p':
            	if (sscanf(optarg, "%s", hostsFile) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
        	case 'x':
            	if (sscanf(optarg, "%s", configFile) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            case 's':
            {   /*  read comId    */
                if (sscanf(optarg, "%u",
                           &comId_Out) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            }
            case 'o':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                ownIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 't':
            {   /*  read ip    */
                if (sscanf(optarg, "%u.%u.%u.%u",
                           &ip[3], &ip[2], &ip[1], &ip[0]) < 4)
                {
                    usage(argv[0]);
                    exit(1);
                }
                destIP = (ip[3] << 24) | (ip[2] << 16) | (ip[1] << 8) | ip[0];
                break;
            }
            case 'v':   /*  version */
                printf("%s: Version %s\t(%s - %s)\n",
                       argv[0], APP_VERSION, __DATE__, __TIME__);
                exit(0);
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0]);
                return 1;
        }
    }
    
    if (destIP == 0)
    {
        fprintf(stderr, "No destination address given!\n");
        usage(argv[0]);
        return 1;
    }
    
	if (strlen(hostsFile) > 0)
    {
		IPTCom_enableIPTDirEmulation(hostsFile);
    }
    
	/*  Initialize IPTCom  */
    
    rv = IPTCom_prepareInit(0, (strlen(configFile)? configFile : NULL));
    if (rv != 0)
    {
        printf("IPTCom_prepareInit() returned %d\n", rv);
        return 1;
    }
    
   	memset(&sa, 0, sizeof(sa));
   	sa.sa_handler = &terminationHandler;
   	sigaction(SIGINT, &sa, NULL);
    
    tdcSetEnableLog  ("TRUE", NULL, NULL);
    
    rv = config_pd_md(comId_Out,
                      comId_Out,
                      PD_COMID1_DATA_SIZE,
                      PD_COMID1_CYCLE,
                      0);
    if (rv < 0)
    {
        printf("config_pd %d failed with %d\n",
               comId_Out, rv);
        return 1;
    }
    
    /* Publish demo packet - deferred  */
    snd_cst_pd = PDComAPI_publish(1, comId_Out, "dcu1.car01.lCst");
    if (snd_cst_pd == 0)
    {
        printf("PDComAPI_publishDef failed\n");
        return 1;
    }
    
    /* Register the object in the C scheduler */
    
    IPTVosRegisterCyclicThread(IPTCom_process, "pdcom", 100,
                               SCHED_OTHER, 20, 4096 * 4);
    
    
    sprintf((char *)gBuffer, "Ping for the %dth. time.", hugeCounter++);
    
    /*
     Enter the main processing loop.
     */
    printf ("\nEntering main loop:\n");
    while (gKeepOnRunning && hugeCounter < 10)
    {
        fd_set  rfds;
        INT32   noOfDesc = 0;
        struct timeval  tv = {0, 500000};
        
        /*
         Prepare the file descriptor set for the select call.
         Additional descriptors can be added here.
         */
        FD_ZERO(&rfds);
        
        /*
         Select() will wait for ready descriptors or timeout,
         what ever comes first.
         */
        usleep(tv.tv_usec);
        //rv = select((int)noOfDesc + 1, &rfds, NULL, NULL, &tv);
        
        /*
         Handle other ready descriptors...
         */
        //if (rv > -1)
        {
            /* printf("looping...\n"); */
        	/* Update the information, that is sent */
            sprintf((char *)gBuffer, "Ping for the %dth. time.", hugeCounter++);
        	PDComAPI_put(snd_cst_pd, (BYTE *) &gDataSet1);
        	PDComAPI_source(1);
        }
        printf (".");
        fflush (stdout);
        
    }   /*    Bottom of while-loop    */
    
    /*
     *    We always clean up behind us!
     */
    
    IPTVosThreadTerminate();
    IPTCom_terminate(1000);
    IPTCom_destroy();
    
    return rv;
}
