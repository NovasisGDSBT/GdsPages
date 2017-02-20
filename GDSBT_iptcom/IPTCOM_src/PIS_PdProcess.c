#include "PIS_Common.h"
#include "PIS_IptCom.h"
#include "PIS_App.h"
#include "PIS_Diag.h"

extern unsigned int CSystemMode;
extern unsigned char ErrorDescription;
extern  int CDurationCounter,CBackOnCounter,CNormalStartsCounter,CWdogResetsCounter;

extern unsigned char FBcklightFault,FTempSensor,FTempORHigh,FTempORLow,FAmbLightSensor;
extern  char    chromium_server[256];

unsigned short ISystemLifeSign=0;

/* VAR DIAG DATA */
int     test_webpage=0; /* If set do not update web page */

unsigned char ccmodTimeoutFault=0;
unsigned char FApiMod=0;
unsigned char FWatchdogApiMod=0;

int     ShutDownReceived = 0;
extern  int infotainment_timeout;

void pd_CCUProcess(CCCUC_INFDIS  pd_InData)
{
    static unsigned char      flag=FALSE;
    static unsigned char      flagPostCalling=FALSE;
    static unsigned char      prev_IInfdMode=0;
    static unsigned short int prev_lifecounter=0;
    static unsigned short int spacetime=0;
    static unsigned short int flagLifesignBack=FALSE;
    static unsigned short int logCountersTimer=0;
    static char               sys_URL[256];
    static char               URL[256];
    static char               txtURL[256];
    char                      cmd[256];
    FILE                      *fp;
    int                       defaultPageTimeOut = infotainment_timeout;


    memset(cmd,0,sizeof(cmd));
    /* Placeholders */
    CDurationCounter++;
    CBackOnCounter++;
    CNormalStartsCounter++;
    CWdogResetsCounter++;
    /* Placeholders ends */
/*
    sprintf(cmd,"echo 0 > %s",URL_COM);
    system(cmd);
    */

    logCountersTimer++;

    if((logCountersTimer%MINUTE)==0)
    {
        snprintf(cmd,sizeof(cmd)-1,"TCMS – counter:lifesign=%d",prev_lifecounter);
        LOG_SYS(DATAREC,DEBUG, "CCUCInfdis_TASK",cmd);
        snprintf(cmd,sizeof(cmd)-1,"TCMS – counter:CDurationCounter=%d",CDurationCounter);
        LOG_SYS(DATAREC,DEBUG, "CCUCInfdis_TASK",cmd);
        snprintf(cmd,sizeof(cmd)-1,"TCMS – counter:CNormalStartsCounter=%d",CNormalStartsCounter);
        LOG_SYS(DATAREC,DEBUG, "CCUCInfdis_TASK",cmd);
        snprintf(cmd,sizeof(cmd)-1,"TCMS – counter:CWdogResetsCounter=%d",CWdogResetsCounter);
        LOG_SYS(DATAREC,DEBUG, "CCUCInfdis_TASK",cmd);
        memset(cmd,0,sizeof(cmd));

    }


    if(flag==FALSE)
    {
        prev_lifecounter=pd_InData.Lifesign.ICCUCLifeSign;
        flag=TRUE;
    }
    else{

        if((prev_lifecounter==pd_InData.Lifesign.ICCUCLifeSign) || ((pd_InData.Lifesign.ICCUCLifeSign==0)&&(prev_lifecounter!= sizeof(unsigned short int))))
        {
            //ERRORE
            spacetime++;

            if(spacetime>=TIMEOUT)
            {
                ErrorDescription=ErrorDescription|IPMODCCUTIMEOUT;
                ccmodTimeoutFault=1;
            }
            if(spacetime >= defaultPageTimeOut)
            {
                system("echo \"http://127.0.0.1:8080/test_default_page/default_page.html\" > /tmp/www/url.txt");
                system("echo CHROMIUM_SERVER=\"http://127.0.0.1:8080/test_default_page/default_page.html\" > /etc/sysconfig/chromium_var");
                system("echo CHROMIUM_SERVER=\"http://127.0.0.1:8080/test_default_page/default_page.html\" > /tmp/chromium_var");
                sprintf(chromium_server,"CHROMIUM_SERVER=\"http://127.0.0.1:8080/test_default_page/default_page.html");
                sprintf(cmd,"echo 1 > %s",URL_COM);
                system(cmd);
                MON_PRINTF("%s : defaultPageTimeOut=%d\n",__FUNCTION__,defaultPageTimeOut);

                if(flagLifesignBack == FALSE)
                {
                    snprintf(cmd,sizeof(cmd)-1,"NO lifesign within %d. Showing default page",defaultPageTimeOut);
                    flagLifesignBack=TRUE;
                    LOG_SYS(APPACTI,ERR, "CCUCInfdis_TASK",cmd);
                }
            }
            MON_PRINTF("%s : spacetime :%d ErrorDescription=%x\n",__FUNCTION__,spacetime,ErrorDescription);
            MON_PRINTF("%s : pd_InData.Lifesign.ICCUCLifeSign :%d\n",__FUNCTION__,pd_InData.Lifesign.ICCUCLifeSign);
        }
        else
        {
            /* upload inner lifecounter*/
            prev_lifecounter=pd_InData.Lifesign.ICCUCLifeSign;
            ErrorDescription &= (~IPMODCCUTIMEOUT);
            spacetime=0;
            ccmodTimeoutFault=0;
            if(flagPostCalling==FALSE)
            {
                flagPostCalling=TRUE;
                system("echo 1 > /tmp/www/POST_enable");
            }
            if(flagLifesignBack == TRUE)
            {
                flagLifesignBack=FALSE;
                LOG_SYS(APPACTI,INFO, "CCUCInfdis_TASK","Lifesign is back");
            }

        }
    }

    if ( test_webpage == 0)
    {
        if ( strlen(pd_InData.LoadResource2.IURL) != 0)
        {
            sprintf(URL,"CHROMIUM_SERVER=%s",pd_InData.LoadResource2.IURL);
            sprintf(txtURL,"%s",pd_InData.LoadResource2.IURL);

            if ( strcmp(chromium_server,URL) != 0 )
            {
                snprintf(cmd,sizeof(cmd)-1,"IURL received:%s",txtURL);
                LOG_SYS(DATAREC,INFO, "CCUCInfdis_TASK",cmd);
                printf("\n");
                printf("URL             : %s!\n",txtURL);
                sprintf(cmd,"/tmp/www/CheckIfPageExists %s &",txtURL);
                system(cmd);
                IPTVosTaskDelay(4000);

                fp=fopen("/tmp/chromium_server_exists","r");
                if ( fp == NULL)
                {
                    sprintf(chromium_server,"%s",URL);
                    printf("URL %s NOT FOUND !!!\n",txtURL);
                    sprintf(cmd,"echo 1 > %s",URL_COM);
                    system(cmd);
                    sprintf(cmd,"URL %s cannot be reached. Showing default page",txtURL);
                    LOG_SYS(APPACTI,ERR, "PD_CCUCInfdis_Task",cmd);

                }
                else
                {
                    close(fp);
                    sprintf(cmd,"echo 0 > %s",URL_COM);
                    system(cmd);
                    sprintf(sys_URL,"echo %s > /tmp/chromium_var",URL);
                    system(sys_URL);
                    system("rm -rf /tmp/chromium_var_mountpoint");
                    system("mkdir /tmp/chromium_var_mountpoint");
                    system("mount /dev/mmcblk0p3 /tmp/chromium_var_mountpoint");
                    system("cp /tmp/chromium_var /tmp/chromium_var_mountpoint/sysconfig/etc/sysconfig/chromium_var");
                    system("cp /tmp/chromium_var /etc/sysconfig/chromium_var");
                    system("umount /tmp/chromium_var_mountpoint");
                    sprintf(chromium_server,"%s",URL);
                    printf("chromium_server is now %s !!!\n",txtURL);
                    sprintf(cmd,"echo %s > /tmp/www/url.txt",txtURL);
                    system(cmd);
                    sprintf(cmd,"Starting chrome pointing at %s",txtURL);
                    LOG_SYS(APPACTI,INFO,"PD_CCUCInfdis_Task",cmd);
                }
            }
        }
    }
    /*
    MON_PRINTF("%s : Lifesign.ICCUCLifeSign:%d\n",__FUNCTION__,pd_InData.Lifesign.ICCUCLifeSign);
    */

    if(prev_IInfdMode != pd_InData.LoadResource.IInfdMode)
    {
        prev_IInfdMode=pd_InData.LoadResource.IInfdMode;

        snprintf(cmd,sizeof(cmd)-1,"IInfdMode received value:%d",prev_IInfdMode);
        LOG_SYS(DATAREC,INFO, "CCUCInfdis_TASK",cmd);

    }
}

int pd_get_value()
{
FILE *fp;
char    p[32];
    fp=fopen("/tmp/ta","r");
    if ( fp !=NULL)
        fscanf(fp,"%s",p);
    fclose(fp);
    return atoi(p);
}

void pd_ReportProcess(BYTE *byte_pd_OutData)
{
    static unsigned char logCountersTimer=0;
    static unsigned int  prev_ISystemMode=0;
    static unsigned int  prev_CSystemMode=0;
    static unsigned char prev_BacklightStatus=0;
    unsigned char        BacklightStatus=0;
    char                 cmd[256];


    logCountersTimer++;



    CINFDISReport *pd_OutData = (CINFDISReport *)byte_pd_OutData;

    pd_OutData->StatusData.ISystemLifeSign=ISystemLifeSign++;

    system("cat /tmp/backlight_on_counter | sed 's/BACKLIGHT_ON_COUNTER=//g' > /tmp/ta");
    pd_OutData->CntData.ITFTBacklight = pd_get_value();

    system("cat /tmp/monitor_on_counter | sed 's/MONITOR_ON_COUNTER=//g' > /tmp/ta");
    pd_OutData->CntData.ITFTWorkTime  = pd_get_value();

    system("cat /tmp/reboot_counter | sed 's/REBOOT_COUNTER=//g' > /tmp/ta");
    pd_OutData->CntData.ITFTPowerUp   = pd_get_value();


    system("cat /sys/class/backlight/backlight_lvds0.28/brightness > /tmp/ta");
    BacklightStatus=(unsigned char)pd_get_value();
    if(BacklightStatus>=5)
        BacklightStatus=5;

    BacklightStatus=BacklightStatus*20;
    pd_OutData->StatusData.IBacklightStatus = BacklightStatus;

    if (prev_BacklightStatus != BacklightStatus)
    {
        snprintf(cmd,sizeof(cmd)-1,"TCMS – New IBacklightStatus sent:%d",BacklightStatus);
        LOG_SYS(SYSDIAG,INFO, "InfdisReport_TASK",cmd);
        prev_BacklightStatus=BacklightStatus;
    }


    if(prev_ISystemMode != curr_mode)
    {
        prev_ISystemMode=curr_mode;
        snprintf(cmd,sizeof(cmd)-1,"TCMS – New ISystemMode sent:%d",prev_ISystemMode);
        LOG_SYS(SYSDIAG,INFO, "InfdisReport_TASK",cmd);
    }

    if ( ShutDownReceived == 0)
        pd_OutData->StatusData.ISystemMode = curr_mode;
    else
    {
        pd_OutData->StatusData.ISystemMode = 4;
        LOG_SYS(SYSDIAG,INFO, "InfdisReport_TASK","TCMS – New ISystemMode sent:4");
    }


    pd_OutData->StatusData.ITestMode = CSystemMode;
    if(prev_CSystemMode!=CSystemMode)
    {
        snprintf(cmd,sizeof(cmd)-1,"TCMS – New ITestMode sent:%d",CSystemMode);
        LOG_SYS(SYSDIAG,INFO, "InfdisReport_TASK",cmd);
        prev_CSystemMode=CSystemMode;
    }

    system("cat /tmp/wdog_counter | sed 's/WATCHDOG_COUNTER=//g' > /tmp/ta");
    pd_OutData->CntData.ITFTWatchdog  = pd_get_value();

    if((logCountersTimer%MINUTE)==0)
    {
        snprintf(cmd,sizeof(cmd)-1,"TCMS – counter:lifesign=%d",ISystemLifeSign);
        LOG_SYS(SYSDIAG,DEBUG, "InfdisReport_TASK",cmd);
        snprintf(cmd,sizeof(cmd)-1,"TCMS – counter:ITFTBacklight=%d",pd_OutData->CntData.ITFTBacklight);
        LOG_SYS(SYSDIAG,DEBUG, "InfdisReport_TASK",cmd);
        snprintf(cmd,sizeof(cmd)-1,"TCMS – counter:ITFTWorkTime=%d",pd_OutData->CntData.ITFTWorkTime);
        LOG_SYS(SYSDIAG,DEBUG, "InfdisReport_TASK",cmd);
        snprintf(cmd,sizeof(cmd)-1,"TCMS – counter:ITFTPowerUp=%d",pd_OutData->CntData.ITFTPowerUp);
        LOG_SYS(SYSDIAG,DEBUG, "InfdisReport_TASK",cmd);
        snprintf(cmd,sizeof(cmd)-1,"TCMS – counter:ITFTWatchdog=%d",pd_OutData->CntData.ITFTWatchdog);
        LOG_SYS(SYSDIAG,DEBUG, "InfdisReport_TASK",cmd);
        memset(cmd,0,sizeof(cmd));

    }

    pd_OutData->DiagData.COMModule.FTimeoutComMod=ccmodTimeoutFault;
    pd_OutData->DiagData.TFTFaults.FBcklightFault=FBcklightFault;
    pd_OutData->DiagData.TFTFaults.FTempSensor=FTempSensor;
    pd_OutData->DiagData.TFTFaults.FTempORHigh=FTempORHigh;
    pd_OutData->DiagData.TFTFaults.FTempORLow=FTempORLow;
    pd_OutData->DiagData.TFTFaults.FAmbLightSensor=FAmbLightSensor;
    system("cat /tmp/api_mod > /tmp/ta");
    FApiMod=pd_get_value();
    pd_OutData->DiagData.ApplicationModule.FApiMod=FApiMod;
    system("cat /tmp/wdog_api_mod > /tmp/ta");
    FWatchdogApiMod=pd_get_value();
    pd_OutData->DiagData.ApplicationModule.FWatchdogApiMod =FWatchdogApiMod;
}


