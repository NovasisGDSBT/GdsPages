/*
 *  pdcom.c
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
#define PD_COMID1               2001
#define PD_COMID1_CYCLE         1000
#define PD_COMID1_TIMEOUT       15000
#define PD_COMID1_DATA_SIZE     32

#define APP_VERSION         "0.01"

#define GBUFFER_SIZE        128

BYTE   gBuffer[GBUFFER_SIZE]       = "Hello World";
BYTE   gInputBuffer[GBUFFER_SIZE]  = "";

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
/*    IPT_CONFIG_COM_PAR comPar = { IPT_DEF_COMPAR_MD_ID, 
        6, 
        MD_DEF_TTL };
    iptConfigAddComPar(&comPar);
*/    return 0;
}

/* Print a sensible usage message */
void usage (const char *appName)
{
    printf("Usage of %s\n", appName);
    printf("This tool sends PD messages to an ED and displayes received PD packages.\n"
           "Arguments are:\n"
           "-o own IP address\n"
           "-t target IP address\n"
           "-c expecting comID\n"
           "-s sending comID\n"
           "-p path to TDC host simulation file\n"
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
    PD_HANDLE   rcv_cst_pd1 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd2 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd3 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd4 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd5 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd6 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd7 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd8 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd9 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd10 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd11 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd12 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd13 = 0;   /* Receive handle for packets   */
    PD_HANDLE   rcv_cst_pd14 = 0;   /* Receive handle for packets   */

    int     rv = 0;
    int     ip[4];
    UINT32  destIP = 0;
    int     ch;
    UINT32  hugeCounter = 0;
    UINT32  ownIP       = 0;
    UINT32  comId_In    = PD_COMID1, comId_Out = PD_COMID1;
    char 	hostsFile[IPT_SIZE_OF_FILEPATH] = "";
    
    /****** Parsing the command line arguments */
    if (argc <= 1)
    {
        usage(argv[0]);
        return 1;
    }

    while ((ch = getopt(argc, argv, "t:o:c:s:p:h?v")) != -1)
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
            case 'c':
            {   /*  read comId    */
                if (sscanf(optarg, "%u",
                           &comId_In) < 1)
                {
                    usage(argv[0]);
                    exit(1);
                }
                break;
            }
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

    rv = IPTCom_prepareInit(0, NULL);
    if (rv != 0)
    {
        printf("IPTCom_prepareInit() returned %d\n",
                     rv);
        return 1;
    }

    
    tdcSetEnableLog  ("TRUE", NULL, NULL);

    rv = config_pd_md(	comId_In,
                      	comId_In,
                   		PD_COMID1_DATA_SIZE,
                        PD_COMID1_CYCLE,
                        0);
    if (rv < 0)
    {
        printf("config_pd %d failed with %d\n",
                     comId_In, rv);
        return 1;
    }
    
    /* Subscribe to demo packets   */
    rcv_cst_pd1 = PDComAPI_subscribe(1, comId_In, "esdno1");
    rcv_cst_pd2 = PDComAPI_subscribe(1, comId_In, "esdno2");
    rcv_cst_pd3 = PDComAPI_subscribe(1, comId_In, "esdno3");
    rcv_cst_pd4 = PDComAPI_subscribe(1, comId_In, "esdno4");
    rcv_cst_pd5 = PDComAPI_subscribe(1, comId_In, "esdno5");
    rcv_cst_pd6 = PDComAPI_subscribe(1, comId_In, "esdno6");
    rcv_cst_pd7 = PDComAPI_subscribe(1, comId_In, "esdno7");
    rcv_cst_pd8 = PDComAPI_subscribe(1, comId_In, "esdno8");
    rcv_cst_pd9 = PDComAPI_subscribe(1, comId_In, "esdno9");
    rcv_cst_pd10 = PDComAPI_subscribe(1, comId_In, "esdno10");
    rcv_cst_pd11 = PDComAPI_subscribe(1, comId_In, "esdno11");
    rcv_cst_pd12 = PDComAPI_subscribe(1, comId_In, "esdno12");
    rcv_cst_pd13 = PDComAPI_subscribe(1, comId_In, "esdno13");
    rcv_cst_pd14 = PDComAPI_subscribe(1, comId_In, "esdno14");
    if (rcv_cst_pd14 == 0)
    {
        printf("PDComAPI_subscribe failed\n");
        return 1;
    }

 /*   /* Publish demo packet - deferred 
    snd_cst_pd = PDComAPI_publish(1, comId_Out, "testdest");
    if (snd_cst_pd == 0)
    {
        printf("PDComAPI_publishDef failed\n");
        return 1;
    }
*/
    /* Register the object in the C scheduler */

    IPTVosRegisterCyclicThread(IPTCom_process, "pdcom", 100,
                               SCHED_OTHER, 20, 4096 * 4);


    sprintf((char *)gBuffer, "Ping for the %dth. time.", hugeCounter++);

    /*
     Enter the main processing loop.
     */
    while (1)
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
        
        rv = select((int)noOfDesc + 1, &rfds, NULL, NULL, &tv);
                
        /*
         Handle other ready descriptors...
         */
        //if (rv > -1)
        {
        	int inValid = TRUE;
            /* printf("looping...\n"); */

            PDComAPI_sink(1);

			if (PDComAPI_getWStatus(rcv_cst_pd1, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
			{
                printf("Ping from the esdno1.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd2, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno2.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd3, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno3.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd4, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno4.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd5, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno5.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd6, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno6.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd7, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno7.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd8, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno8.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd9, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno9.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd10, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno10.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd11, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno11.\n");
            }
            if (PDComAPI_getWStatus(rcv_cst_pd12, (BYTE *) &gInputBuffer, sizeof(gInputBuffer), &inValid) == IPT_OK &&
            	inValid == FALSE)
            {
            	printf("Ping from the esdno12.\n");
            }
                                  /* Update the information, that is sent */
        	//PDComAPI_put(snd_cst_pd, (BYTE *) &gBuffer);
        	//PDComAPI_source(1);
        }
        
        
        /* Display received information */
        if (gInputBuffer[0] > 0) /* FIXME Better solution would be: global flag, that is set in the callback function to
                                  indicate new data */
        {
            printf("# %s ", gInputBuffer);
            memset(gInputBuffer, 0, sizeof(gInputBuffer));
        }
    }   /*    Bottom of while-loop    */
    
    /*
     *    We always clean up behind us!
     */
    
    IPTCom_terminate(1);
    
    return rv;
}
