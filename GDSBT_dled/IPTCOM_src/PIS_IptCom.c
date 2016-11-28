
#include "PIS_Common.h"
#include "PIS_IptCom.h"
#include "PIS_App.h"

/* FUNC PROCESS DATA */
static void PD_CCUCInfdis_Core(void);
static void PD_InfdisReport_Core(void);
int PD_CCUCInfdis_Init(void);
int PD_InfdisReport_Init(void);

/* FUNC MESSAGE DATA*/
int MD_InfDisCtrlOp_Init(void);
static void MD_InfDisCtrlOp_Core(void);
int MD_InfDisCtrlMaint_Init(void);
static void MD_InfDisCtrlMaint_Core(void);

/* VAR PROCESS DATA */
static PD_HANDLE hpd_in;
static UINT32 pd_InComId = COMID_ICCUC_INFDIS;
static PD_HANDLE hpd_out;
static UINT32 pd_OutComId = COMID_OINFDISREPORT;
static UINT32 schedGroupPDIN=1;
static UINT32 schedGroupPDOUT=2;
static CCCUC_INFDIS pd_InData;
static CINFDISReport pd_OutData;

/* VAR MESSAGE DATA*/
static MD_QUEUE mdq_Op_In;
static MD_QUEUE mdq_Maint_In;
static UINT32 md_InComId_Op[2] = {COMID_IINFDISCTRLOP,0};
static UINT32 md_InComId_Maint[2] = {COMID_IINFDISCTRLMAINT,0};
static CINFDISCtrlOp md_InDataOp;
static CINFDISCtrlMaint md_InDataMaint;

void IptComClean(void);

int WrapIPTCom_handleEvent(void)
{
int retval=IPT_OK;

    printf("IPTCom_handleEvent IPTCOM_EVENT_LINK_UP called...\n");
    if (IPTCom_handleEvent(IPTCOM_EVENT_LINK_UP, NULL) != 0)
    {
        printf("IPTCom_handleEvent IPTCOM_EVENT_LINK_UP failed\n");
        retval=-1;
    }
    printf("IPTCom_handleEvent IPTCOM_EVENT_LINK_UP passed\n");
    return retval;
}

int IptComStart(void)
{
int res=0;

    /***************************/
    /* ProcessData CCUC_INFDIS IN */
    /***************************/
    res = PD_CCUCInfdis_Init();
    if (res != IPT_OK)
        return(res);

    /***************************/
    /* ProcessData InfdisReport OUT*/
    /***************************/
    res = PD_InfdisReport_Init();
    if (res != IPT_OK)
        return(res);
    /* Register IPTCom PD-process in the C scheduler */
    IPTVosRegisterCyclicThread(PD_CCUCInfdis_Core,"PD_CCUCInfdis_Core",
            1000,
            POLICY,
            APPL2000_PRIO,
            STACK);

    IPTVosRegisterCyclicThread(PD_InfdisReport_Core,"PD_InfdisReport_Core",
            1000,
            POLICY,
            APPL1000_PRIO,
            STACK);
    /***************************/
    /* MessageData InfDisCtrlOp IN*/
    /***************************/
    res = MD_InfDisCtrlOp_Init();
    if (res != IPT_OK)
        return(res);

    /***************************/
    /* MessageData InfDisCtrlMaint IN*/
    /***************************/
    res = MD_InfDisCtrlMaint_Init();
    if (res != IPT_OK)
        return(res);

    res = WrapIPTCom_handleEvent();

    IPTVosRegisterCyclicThread(MD_InfDisCtrlOp_Core,"MD_InfDisCtrlOp_Core",
            1000,
            POLICY,
            APPL100_PRIO,
            STACK);

    IPTVosRegisterCyclicThread(MD_InfDisCtrlMaint_Core,"MD_InfDisCtrlMaint_Core",
            1000,
            POLICY,
            APPL200_PRIO,
            STACK);

    dleds_thread();
    MON_PRINTF("\nDLEDS waiting\n\n");
    return res;
}

/*******************************************************************************
* GLOBAL FUNCTIONS */

/*******************************************************************************
NAME:       PD_CCUCInfdis_Init
ABSTRACT:   Publishing unicast ComId to all devices
            Subscription without filtering

RETURNS:    -
*/
int PD_CCUCInfdis_Init(void)
{
    /* Subscribe data from own and the other devices to be echoed */
    hpd_in= PDComAPI_sub(schedGroupPDIN, pd_InComId, 0, NULL, 0, NULL);

    if (hpd_in == 0)
    {
        MON_PRINTF("PDComAPI_sub FAILED hpd_in ComId=%d\n", pd_InComId);
        return(-1);
    }
    else
        MON_PRINTF("PDComAPI_sub comid=%d\n", pd_InComId);
    return(IPT_OK);
}

 /*******************************************************************************
NAME:       PD_CCUCInfdis_Core
ABSTRACT:   application with 100 ms cycle time

RETURNS:    -
*/
static void PD_CCUCInfdis_Core(void)
{
int res;
int ret=IPT_ERROR;
static UINT8 oldTopo = 0;
UINT8 state;
UINT8 topo;

    res = tdcGetIptState(&state, &topo);
    if ( res == TDC_OK)
    {
        if (state != TDC_IPT_INAUGSTATE_OK)
        {
            /* Avoid printout at start-up */
            if (oldTopo != 0)
            {
                MON_PRINTF("PD_CCUCInfdis_Core: tdcGetIptState returned state=%d topoCnt=%d old topo= %d\n",
                    state, topo, oldTopo);
            }
        }
        else if ((topo != oldTopo) && (topo != 0))
        {
            ret = IPT_OK;
            oldTopo = topo;

            res = PDComAPI_renewSub(hpd_in);
            if (res != IPT_OK)
                ret = IPT_ERROR;
            if (ret == IPT_OK)
                MON_PRINTF("PD_CCUCInfdis_Core: Pub and sub renewed OK. topoCnt=%d\n", topo);
            else
                MON_PRINTF("PD_CCUCInfdis_Core: pub and sub not renewed topoCnt=%d\n", topo);
        }
    }
    else
        MON_PRINTF("PD_CCUCInfdis_Core: tdcGetIptState error=%#x\n",res);

    PDComAPI_sink(schedGroupPDIN);
    /* get */
    res=PDComAPI_get(hpd_in, (BYTE*) &pd_InData, sizeof(CCCUC_INFDIS));

  //  MON_PRINTF("%s : Lifesign :%d\n",__FUNCTION__,pd_InData.Lifesign.ICCUCLifeSign);

    if(res!=IPT_OK)
        MON_PRINTF("%s : PDComAPI_get retcode %d , ILLEGAL!\n",__FUNCTION__,res);
}

/*******************************************************************************
NAME:       PD_InfdisReport_Init
ABSTRACT:   Publishing unicast ComId to all devices
            Subscription without filtering

RETURNS:    -
*/
int PD_InfdisReport_Init(void)
{
    MON_PRINTF("PD_InfdisReport_Init...........\n");
    hpd_out = PDComAPI_pub (schedGroupPDOUT, pd_OutComId, 0, NULL);

    if (hpd_out == 0)
    {
        MON_PRINTF("PDComAPI_pub   FAILED hpd_out ComId=%d\n", pd_OutComId);
        return(-1);
    }
    else
    {
        MON_PRINTF("PDComAPI_pub comid=%d\n", pd_OutComId);
    }
    return(IPT_OK);
}

/*******************************************************************************
NAME:       PD_InfdisReport_Core
ABSTRACT:   application with 100 ms cycle time

RETURNS:    -
*/
static void PD_InfdisReport_Core(void)
{
int res;
int ret=IPT_ERROR;
static UINT8 oldTopo = 0;
UINT8 state;
UINT8 topo;

    res = tdcGetIptState(&state, &topo);
    if ( res == TDC_OK)
    {
        if (state != TDC_IPT_INAUGSTATE_OK)
        {
            /* Avoid printout at start-up */
            if (oldTopo != 0)
            {
                MON_PRINTF("PD_InfdisReport_Core: tdcGetIptState returned state=%d topoCnt=%d old topo= %d\n",
                state, topo, oldTopo);
            }
        }
        else if ((topo != oldTopo) && (topo != 0))
        {
            ret = IPT_OK;
            oldTopo = topo;
            res = PDComAPI_renewPub(hpd_out);
            if (res != IPT_OK)
            {
                ret = IPT_ERROR;
            }
            if (ret == IPT_OK)
            {
                MON_PRINTF("PD_InfdisReport_Core: Pub and sub renewed OK. topoCnt=%d\n", topo);
            }
            else
            {
                MON_PRINTF("PD_InfdisReport_Core: pub and sub not renewed topoCnt=%d\n", topo);
            }
        }
    }
    else
    {
        MON_PRINTF("PD_InfdisReport_Core: tdcGetIptState error=%#x\n",res);
    }

//    pd_ReportProcess((BYTE*) &pd_OutData);

    PDComAPI_put(hpd_out, (BYTE*) &pd_OutData);
    PDComAPI_source(schedGroupPDOUT);
}

/*******************************************************************************
NAME:       MD_InfDisCtrlOp_Init
ABSTRACT:


RETURNS:    -
*/
int MD_InfDisCtrlOp_Init(void)
{
int res;

    MON_PRINTF("MD_InfDisCtrlOp_Init........\n");

    mdq_Op_In = MDComAPI_queueCreate(NUM_OF_MSG, NULL);
    if (mdq_Op_In == 0)
    {
        MON_PRINTF("MDComAPI_queueCreate mdq_Op_In failed\n");
        return(-1);
    }
    MON_PRINTF("MDComAPI_queueCreate OK\n");
    res = MDComAPI_comIdListener(mdq_Op_In, 0, 0, md_InComId_Op, NULL, 0, NULL);
    if (res)
    {
        MON_PRINTF("MDComAPI_comIdListener mdq_Op_In failed res=%d\n",res);
        return(-1);
    }

    MON_PRINTF("MDComAPI_comIdListener OK\n");
    return(IPT_OK);
}


/*******************************************************************************
NAME:       MD_InfDisCtrlOp_Core
ABSTRACT:


RETURNS:    -
*/
static void MD_InfDisCtrlOp_Core(void)
{
int res;
int status;
UINT32 mdsize;
char *pRecBuf;
MSG_INFO mdgetMsgInfo;

   status = IPTCom_getStatus();

   do
   {
      mdsize = sizeof(CINFDISCtrlOp);
      pRecBuf = (char *)(&md_InDataOp);
      res = MDComAPI_getMsg(mdq_Op_In, &mdgetMsgInfo, &pRecBuf, &mdsize, IPT_NO_WAIT);
      if ((res == MD_QUEUE_NOT_EMPTY) && (status == IPTCOM_RUN))
      {
          /*
        MON_PRINTF("mdq_Op_In received ComId=%d ip=%d.%d.%d.%d\n", mdgetMsgInfo.comId,
            (mdgetMsgInfo.srcIpAddr >> 24) & 0xff,  (mdgetMsgInfo.srcIpAddr >> 16) & 0xff,
            (mdgetMsgInfo.srcIpAddr >> 8) & 0xff,  mdgetMsgInfo.srcIpAddr  & 0xff);
*/
        if (mdgetMsgInfo.msgType == MD_MSGTYPE_DATA )
         {
            #ifdef TIME_MEASURE
            int start_time , end_time;
            start_time = IPTVosGetMicroSecTimer();
            #endif
            //md_Op(md_InDataOp, mdgetMsgInfo.comId,mdgetMsgInfo.srcIpAddr);
            #ifdef TIME_MEASURE
            end_time = IPTVosGetMicroSecTimer();
            printf("md_Op time : %d\n",end_time - start_time);
            #endif
            /* Handle received data */
            /**/
            MON_PRINTF("MD DATA RECEIVED - CINFDISCtrlOp:\nCBacklightCmd=%d \nCShutdownCmd=%d \nCSystemMode=%d\n\n",md_InDataOp.CtrlCommands.CBacklightCmd,
                md_InDataOp.CtrlCommands.CShutdownCmd,md_InDataOp.CtrlCommands.CSystemMode);
                /**/
         }
      }
   }
   while(res == MD_QUEUE_NOT_EMPTY);
}

/*******************************************************************************
NAME:       MD_InfDisCtrlMaint_Init
ABSTRACT:

md_Maint
RETURNS:    -
*/
int MD_InfDisCtrlMaint_Init(void)
{
int res;

    //MON_PRINTF("MD_InfDisCtrlMaint_Init........\n");

    mdq_Maint_In = MDComAPI_queueCreate(NUM_OF_MSG, NULL);
    if (mdq_Maint_In == 0)
    {
        MON_PRINTF("MDComAPI_queueCreate mdq_Maint_In failed\n");
        return(-1);
    }
    res = MDComAPI_comIdListener(mdq_Maint_In, 0, 0, md_InComId_Maint, NULL, 0, NULL);
    if (res)
    {
        MON_PRINTF("MDComAPI_comIdListener mdq_Maint_In failed res=%d\n",res);
        return(-1);
    }
    return(IPT_OK);
}


/*******************************************************************************
NAME:       MD_InfDisCtrlMaint_Core
ABSTRACT:


RETURNS:    -
*/
static void MD_InfDisCtrlMaint_Core(void)
{
int res;
int status;
UINT32 mdsize;
char *pRecBuf;
MSG_INFO mdgetMsgInfo;

   status = IPTCom_getStatus();

   do
   {
      mdsize = sizeof(CINFDISCtrlMaint);
      pRecBuf = (char *)(&md_InDataMaint);
      res = MDComAPI_getMsg(mdq_Maint_In, &mdgetMsgInfo, &pRecBuf, &mdsize, IPT_NO_WAIT);
      if ((res == MD_QUEUE_NOT_EMPTY) && (status == IPTCOM_RUN))
      {
          /*
        MON_PRINTF("mdq_Maint_In received ComId=%d ip=%d.%d.%d.%d\n", mdgetMsgInfo.comId,
               (mdgetMsgInfo.srcIpAddr >> 24) & 0xff,  (mdgetMsgInfo.srcIpAddr >> 16) & 0xff,
               (mdgetMsgInfo.srcIpAddr >> 8) & 0xff,  mdgetMsgInfo.srcIpAddr  & 0xff);
*/
        if (mdgetMsgInfo.msgType == MD_MSGTYPE_DATA )
         {
            /* Handle received data */
            #ifdef TIME_MEASURE
            int start_time , end_time;
            start_time = IPTVosGetMicroSecTimer();
            #endif
            //md_Maint(md_InDataMaint,mdgetMsgInfo.comId,mdgetMsgInfo.srcIpAddr);
            #ifdef TIME_MEASURE
            end_time = IPTVosGetMicroSecTimer();
            printf("md_Maint time : %d\n",end_time - start_time);
            #endif
            /**/
            MON_PRINTF("MD DATA RECEIVED - CINFDISCtrlMaint:\n CReset=%d \nCINFDISCtrlMaint:\nCBackOnCounterReset=%d \nCDurationCounterReset=%d \nCNormalStartsCounterReset=%d \nCResetAllCounters=%d \nCWdogResetsCounterReset=%d\n\n",
               md_InDataMaint.Commands.CReset,
               md_InDataMaint.INFDCounterCommands.CBackOnCounterReset,md_InDataMaint.INFDCounterCommands.CDurationCounterReset,
               md_InDataMaint.INFDCounterCommands.CNormalStartsCounterReset,md_InDataMaint.INFDCounterCommands.CResetAllCounters,
               md_InDataMaint.INFDCounterCommands.CWdogResetsCounterReset);
               /**/
         }
      }
   }
   while(res == MD_QUEUE_NOT_EMPTY);
}


/*******************************************************************************
NAME:       IptComClean
ABSTRACT:
RETURNS:    -
*/
void IptComClean(void)
{

    MON_PRINTF("IptComClean ......\n");
    PDComAPI_unsubscribe(&hpd_in);
    PDComAPI_unpublish(&hpd_out);
    MDComAPI_removeQueue(mdq_Op_In, REMOVE_QUEUE_ALL_USE);
    MDComAPI_removeQueue(mdq_Maint_In, REMOVE_QUEUE_ALL_USE);

    IPTVosThreadTerminate();
    IPTCom_terminate(0);
    IPTCom_destroy();
    MON_PRINTF("Demo terminated\n");
}
